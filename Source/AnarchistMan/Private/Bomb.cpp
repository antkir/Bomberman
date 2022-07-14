// Fill out your copyright notice in the Description page of Project Settings.

#include "Bomb.h"
#include <Utils.h>
#include <BreakableBlock.h>
#include <Explosion.h>
#include <PlayerCharacter.h>
#include <Components/BoxComponent.h>
#include <Components/CapsuleComponent.h>
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
	RadiusBlocks = 2;

	ExplosionConstraints = {
		RadiusBlocks,
		RadiusBlocks,
		RadiusBlocks,
		RadiusBlocks
	};

	ExplosionTriggered = false;

	BlockPawnsMask = 0b1111;
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
			auto* PC = Cast<APlayerCharacter>(Overlap.GetActor());
			if (PC)
			{
				ECollisionChannel PlayerCollisionChannel = PC->GetCapsuleComponent()->GetCollisionObjectType();
				BlockPawnsMask ^= Utils::GetPlayerIdFromPawnECC(PlayerCollisionChannel);
			}
		}

		OnRep_BlockPawns();

		OverlapComponent->OnComponentEndOverlap.AddDynamic(this, &ABomb::HandleEndOverlap);
	}
}

void ABomb::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacter* Character = Cast<APlayerCharacter>(OtherActor);
	if (Character)
	{
		ECollisionChannel PlayerCollisionChannel = Character->GetCapsuleComponent()->GetCollisionObjectType();
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

void ABomb::LifeSpanExpired()
{
	BlowUp();
	Destroy();
}

void ABomb::BlowUp()
{
	if (ExplosionClass == nullptr)
	{
		UE_LOG(LogGame, Error, TEXT("ExplosionClass property is not set!"));
		return;
	}

	if (!HasAuthority())
	{
		return;
	}

	ExplosionTriggered = true;

	// Right
	{
		FVector Start = GetActorLocation();
		FVector End = GetActorLocation();
		End.X = Utils::RoundUnitCenter(End.X) + Utils::Unit * RadiusBlocks;

		uint64 Constraint = LineTraceExplosion(Start, End);
		ExplosionConstraints.Right = std::min(ExplosionConstraints.Down, Constraint);
	}

	// Left
	{
		FVector Start = GetActorLocation();
		FVector End = GetActorLocation();
		End.X = Utils::RoundUnitCenter(End.X) - Utils::Unit * RadiusBlocks;

		uint64 Constraint = LineTraceExplosion(Start, End);
		ExplosionConstraints.Left = std::min(ExplosionConstraints.Down, Constraint);
	}

	// Up
	{
		FVector Start = GetActorLocation();
		FVector End = GetActorLocation();
		End.Y = Utils::RoundUnitCenter(End.Y) - Utils::Unit * RadiusBlocks;

		uint64 Constraint = LineTraceExplosion(Start, End);
		ExplosionConstraints.Up = std::min(ExplosionConstraints.Down, Constraint);
	}

	// Down
	{
		FVector Start = GetActorLocation();
		FVector End = GetActorLocation();
		End.Y = Utils::RoundUnitCenter(End.Y) + Utils::Unit * RadiusBlocks;

		uint64 Constraint = LineTraceExplosion(Start, End);
		ExplosionConstraints.Down = std::min(ExplosionConstraints.Down, Constraint);
	}

	{
		FVector Location = GetActorLocation();
		FTransform Transform;
		Transform.SetLocation(GetActorLocation());
		Transform.SetRotation(FQuat::Identity);
		GetWorld()->SpawnActorAbsolute(ExplosionClass, Transform);
	}

	for (uint8 Index = 1; Index <= ExplosionConstraints.Right; Index++)
	{
		// Right
		FVector Location = GetActorLocation();
		Location.X = Utils::RoundUnitCenter(Location.X) + Utils::Unit * Index;
		Location.Y = Utils::RoundUnitCenter(Location.Y);
		FTransform Transform;
		Transform.SetLocation(Location);
		Transform.SetRotation(FQuat::Identity);
		GetWorld()->SpawnActorAbsolute(ExplosionClass, Transform);
	}

	for (uint8 Index = 1; Index <= ExplosionConstraints.Left; Index++)
	{
		// Left
		FVector Location = GetActorLocation();
		Location.X = Utils::RoundUnitCenter(Location.X) - Utils::Unit * Index;
		Location.Y = Utils::RoundUnitCenter(Location.Y);
		FTransform Transform;
		Transform.SetLocation(Location);
		Transform.SetRotation(FQuat::Identity);
		GetWorld()->SpawnActorAbsolute(ExplosionClass, Transform);
	}

	for (uint8 Index = 1; Index <= ExplosionConstraints.Up; Index++)
	{
		// Up
		FVector Location = GetActorLocation();
		Location.X = Utils::RoundUnitCenter(Location.X);
		Location.Y = Utils::RoundUnitCenter(Location.Y) - Utils::Unit * Index;
		FTransform Transform;
		Transform.SetLocation(Location);
		Transform.SetRotation(FQuat::Identity);
		GetWorld()->SpawnActorAbsolute<AExplosion>(ExplosionClass, Transform);
	}

	for (uint8 Index = 1; Index <= ExplosionConstraints.Down; Index++)
	{
		// Down
		{
			FVector Location = GetActorLocation();
			Location.X = Utils::RoundUnitCenter(Location.X);
			Location.Y = Utils::RoundUnitCenter(Location.Y) + Utils::Unit * Index;
			FTransform Transform;
			Transform.SetLocation(Location);
			Transform.SetRotation(FQuat::Identity);
			GetWorld()->SpawnActorAbsolute<AExplosion>(ExplosionClass, Transform);
		}
	}
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

			if (!IsValid(Actor))
			{
				continue;
			}

			if (Actor->IsA(ABomb::StaticClass()))
			{
				ABomb* Bomb = Cast<ABomb>(Actor);
				if (Bomb != this && !Bomb->ExplosionTriggered)
				{
					Bomb->BlowUp();
					Bomb->Destroy();

					float DistanceRounded = FMath::RoundToZero(HitResult.Distance / Utils::Unit);
					BlockingDistance = static_cast<uint32>(DistanceRounded);

					break;
				}
			}
			else if (Actor->IsA(ABreakableBlock::StaticClass()))
			{
				Actor->Destroy();

				float DistanceRounded = FMath::RoundToZero(HitResult.Distance / Utils::Unit);
				BlockingDistance = static_cast<uint32>(DistanceRounded) + 1;

				break;
			}
		}
		else
		{
			float DistanceRounded = FMath::RoundToZero(HitResult.Distance / Utils::Unit);
			BlockingDistance = static_cast<uint32>(DistanceRounded);
		}
	}

	return BlockingDistance;
}
