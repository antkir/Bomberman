// Fill out your copyright notice in the Description page of Project Settings.

#include "Bomb.h"

#include <Explosion.h>
#include <PlayerCharacter.h>
#include <Utils.h>

#include <Components/BoxComponent.h>
#include <Components/CapsuleComponent.h>
#include <Kismet/GameplayStatics.h>
#include <Net/UnrealNetwork.h>

// Sets default values
ABomb::ABomb()
{
	bReplicates = true;

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create an overlap component
	OverlapComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapComponent"));

	// Set as root component
	RootComponent = OverlapComponent;

	// Create a mesh component
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));

	// Set the component's mesh
	USkeletalMesh* BombMesh = ConstructorHelpers::FObjectFinder<USkeletalMesh>(TEXT("SkeletalMesh'/Engine/EngineMeshes/SkeletalCube'")).Object;
	MeshComponent->SetSkeletalMesh(BombMesh);

	MeshComponent->SetupAttachment(RootComponent);

	LifeSpan = 3.f;

    ExplosionConstraintBlocks = 3;

	BlockPawnsMask = 0b1111;

    TileExplosionDelay = 0.1f;
}

void ABomb::SetExplosionConsttraintBlocks(uint64 Blocks)
{
    ExplosionConstraintBlocks = Blocks;
}

void ABomb::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	auto DoRepLifetimeParams = FDoRepLifetimeParams();
	DoRepLifetimeParams.RepNotifyCondition = ELifetimeRepNotifyCondition::REPNOTIFY_Always;
	DOREPLIFETIME_WITH_PARAMS(ABomb, BlockPawnsMask, DoRepLifetimeParams);
}

// Called when the game starts or when spawned
void ABomb::BeginPlay()
{
	Super::BeginPlay();

    if (ExplosionClass == nullptr)
    {
        UE_LOG(LogGame, Error, TEXT("ExplosionClass property is not set!"));
        return;
    }

	SetLifeSpan(LifeSpan);

	if (IdleAnimation)
	{
		MeshComponent->PlayAnimation(IdleAnimation, true);
	}

	if (HasAuthority())
	{
		// Do not block players if they are overlapping a bomb
		TArray<FOverlapResult> OutOverlaps{};
		FVector Location = GetActorLocation();
		Location.Z = Utils::RoundUnitCenter(Location.Z);
		FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(Utils::Unit / 2));
		FCollisionObjectQueryParams QueryParams;
		QueryParams.AddObjectTypesToQuery(ECC_Pawn1);
		QueryParams.AddObjectTypesToQuery(ECC_Pawn2);
		QueryParams.AddObjectTypesToQuery(ECC_Pawn3);
		QueryParams.AddObjectTypesToQuery(ECC_Pawn4);
		GetWorld()->OverlapMultiByObjectType(OutOverlaps, Location, FQuat::Identity, QueryParams, CollisionShape);

		for (const FOverlapResult& Overlap : OutOverlaps)
		{
			auto* PlayerCharacter = Cast<APlayerCharacter>(Overlap.GetActor());
			if (PlayerCharacter)
			{
				ECollisionChannel PlayerCollisionChannel = PlayerCharacter->GetCapsuleComponent()->GetCollisionObjectType();
				BlockPawnsMask ^= Utils::GetPlayerIdFromPawnECC(PlayerCollisionChannel);
			}
		}

		OnRep_BlockPawns();

		OverlapComponent->OnComponentEndOverlap.AddDynamic(this, &ABomb::HandleEndOverlap);
	}
}

void ABomb::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor);
	if (PlayerCharacter)
	{
		ECollisionChannel PlayerCollisionChannel = PlayerCharacter->GetCapsuleComponent()->GetCollisionObjectType();
		BlockPawnsMask |= Utils::GetPlayerIdFromPawnECC(PlayerCollisionChannel);
	}
	OnRep_BlockPawns();
}

void ABomb::OnRep_BlockPawns()
{
	for (uint32 Index = 0; Index < GetNum(Utils::PlayerECCs); Index++)
	{
		if (BlockPawnsMask & (1 << Index))
		{
			OverlapComponent->SetCollisionResponseToChannel(Utils::PlayerECCs[Index], ECollisionResponse::ECR_Block);
		}
		else
		{
			OverlapComponent->SetCollisionResponseToChannel(Utils::PlayerECCs[Index], ECollisionResponse::ECR_Overlap);
		}
	}
}

bool ABomb::IsBlockingExplosion_Implementation()
{
    return true;
}

void ABomb::BlowUp_Implementation()
{
    if (ExplosionTriggered)
    {
        return;
    }

    StartExplosion();
    Destroy();

    OnBombExploded.Broadcast();
}

void ABomb::LifeSpanExpired()
{
    BlowUp_Implementation();
}

void ABomb::StartExplosion()
{
	if (!HasAuthority())
	{
		return;
	}

	ExplosionTriggered = true;

    ExplosionConstraints Constraints {
        ExplosionConstraintBlocks,
        ExplosionConstraintBlocks,
        ExplosionConstraintBlocks,
        ExplosionConstraintBlocks
    };

	// Right
	{
		FVector Start = GetActorLocation();
		FVector End = GetActorLocation();
		End.X = Utils::RoundUnitCenter(End.X) + Utils::Unit * ExplosionConstraintBlocks;

		uint64 Constraint = LineTraceExplosion(Start, End);
        Constraints.Right = std::min(Constraints.Right, Constraint);
	}

	// Left
	{
		FVector Start = GetActorLocation();
		FVector End = GetActorLocation();
		End.X = Utils::RoundUnitCenter(End.X) - Utils::Unit * ExplosionConstraintBlocks;

		uint64 Constraint = LineTraceExplosion(Start, End);
        Constraints.Left = std::min(Constraints.Left, Constraint);
	}

	// Up
	{
		FVector Start = GetActorLocation();
		FVector End = GetActorLocation();
		End.Y = Utils::RoundUnitCenter(End.Y) - Utils::Unit * ExplosionConstraintBlocks;

		uint64 Constraint = LineTraceExplosion(Start, End);
        Constraints.Up = std::min(Constraints.Up, Constraint);
	}

	// Down
	{
		FVector Start = GetActorLocation();
		FVector End = GetActorLocation();
		End.Y = Utils::RoundUnitCenter(End.Y) + Utils::Unit * ExplosionConstraintBlocks;

		uint64 Constraint = LineTraceExplosion(Start, End);
        Constraints.Down = std::min(Constraints.Down, Constraint);
	}

	{
		FTransform Transform;
		Transform.SetLocation(GetActorLocation());
		Transform.SetRotation(FQuat::Identity);

        ExplodeTile(GetWorld(), ExplosionClass, Transform);
	}

    for (uint8 Index = 1; Index < Constraints.Right; Index++)
    {
        // Right
        FVector Location = GetActorLocation();
        Location.X = Utils::RoundUnitCenter(Location.X) + Utils::Unit * Index;
        Location.Y = Utils::RoundUnitCenter(Location.Y);
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, [World = GetWorld(), ExplosionClass = ExplosionClass, Transform]()
        {
            ExplodeTile(World, ExplosionClass, Transform);
        }, TileExplosionDelay* Index, false);
    }

    for (uint8 Index = 1; Index < Constraints.Left; Index++)
    {
        // Left
        FVector Location = GetActorLocation();
        Location.X = Utils::RoundUnitCenter(Location.X) - Utils::Unit * Index;
        Location.Y = Utils::RoundUnitCenter(Location.Y);
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, [World = GetWorld(), ExplosionClass = ExplosionClass, Transform]()
        {
            ExplodeTile(World, ExplosionClass, Transform);
        }, TileExplosionDelay* Index, false);
    }

    for (uint8 Index = 1; Index < Constraints.Up; Index++)
    {
        // Up
        FVector Location = GetActorLocation();
        Location.X = Utils::RoundUnitCenter(Location.X);
        Location.Y = Utils::RoundUnitCenter(Location.Y) - Utils::Unit * Index;
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, [World = GetWorld(), ExplosionClass = ExplosionClass, Transform]()
        {
            ExplodeTile(World, ExplosionClass, Transform);
        }, TileExplosionDelay* Index, false);
    }

    for (uint8 Index = 1; Index < Constraints.Down; Index++)
    {
        // Down
        FVector Location = GetActorLocation();
        Location.X = Utils::RoundUnitCenter(Location.X);
        Location.Y = Utils::RoundUnitCenter(Location.Y) + Utils::Unit * Index;
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, [World = GetWorld(), ExplosionClass = ExplosionClass, Transform]()
        {
            ExplodeTile(World, ExplosionClass, Transform);
        }, TileExplosionDelay * Index, false);
    }
}

void ABomb::ExplodeTile(UWorld* World, TSubclassOf<AExplosion> ExplosionClass, FTransform Transform)
{
    FVector Location = Transform.GetLocation();
    Location.Z = Utils::RoundUnitCenter(Location.Z);

    FCollisionObjectQueryParams QueryParams;
    QueryParams.AddObjectTypesToQuery(ECC_Pawn1);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn2);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn3);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn4);
    QueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

    FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(Utils::Unit / 8, Utils::Unit / 2);

    TArray<FOverlapResult> OutOverlaps{};
    World->OverlapMultiByObjectType(OutOverlaps, Location, FQuat::Identity, QueryParams, CollisionShape);

    for (const FOverlapResult& Overlap : OutOverlaps)
    {
        AActor* Actor = Overlap.GetActor();
        if (IsValid(Actor) && Actor->Implements<UExplosiveInterface>())
        {
            IExplosiveInterface::Execute_BlowUp(Actor);
        }
    }

    World->SpawnActorAbsolute(ExplosionClass, Transform);
}

uint32 ABomb::LineTraceExplosion(FVector Start, FVector End)
{
	uint32 BlockingDistance = std::numeric_limits<uint32>::max();

	TArray<FHitResult> OutHits{};
	GetWorld()->LineTraceMultiByChannel(OutHits, Start, End, ECollisionChannel::ECC_GameExplosion);

	for (const FHitResult& HitResult : OutHits)
	{
		if (!HitResult.bBlockingHit)
		{
			AActor* Actor = HitResult.GetActor();

			if (!IsValid(Actor) || Actor == this)
			{
				continue;
			}

            if (Actor->Implements<UExplosiveInterface>())
            {
                if (IExplosiveInterface::Execute_IsBlockingExplosion(Actor))
                {
                    float DistanceRounded = FMath::RoundHalfFromZero(HitResult.Distance / Utils::Unit);
                    BlockingDistance = static_cast<uint32>(DistanceRounded) + 1;

                    break;
                }
            }
		}
		else
		{
			float DistanceRounded = FMath::RoundHalfFromZero(HitResult.Distance / Utils::Unit);
			BlockingDistance = static_cast<uint32>(DistanceRounded);
		}
	}

	return BlockingDistance;
}
