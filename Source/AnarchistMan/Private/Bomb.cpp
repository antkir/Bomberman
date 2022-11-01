// Fill out your copyright notice in the Description page of Project Settings.

#include "Bomb.h"

#include <AMNavModifierComponent.h>
#include <Explosion.h>
#include <GridNavMesh.h>
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
    //PrimaryActorTick.TickInterval = 0.5f;

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

    SetExplosionRadiusTiles(2);

    TileExplosionDelay = 0.1f;

    ExplosionTimeout = 3.f;

	BlockPawnsMask = std::numeric_limits<uint16>::max();
}

void ABomb::SetExplosionRadiusTiles(uint64 Radius)
{
    ExplosionRadiusTiles = Radius;
}

void ABomb::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	auto DoRepLifetimeParams = FDoRepLifetimeParams();
	DoRepLifetimeParams.RepNotifyCondition = ELifetimeRepNotifyCondition::REPNOTIFY_Always;
	DOREPLIFETIME_WITH_PARAMS(ABomb, BlockPawnsMask, DoRepLifetimeParams);
}

void ABomb::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    auto* GridNavMesh = Cast<AGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AGridNavMesh::StaticClass()));
    if (HasAuthority() && GridNavMesh)
    {
        UpdateExplosionConstraints();

        float TimeRemaining = GetWorldTimerManager().GetTimerRemaining(TimerHandle_ExplosionTimeoutExpired);
        if (TimeRemaining == -1.f)
        {
            float LifeSpanAfterExplosion = TileExplosionDelay * ExplosionRadiusTiles;
            float LifeSpanRemaining = GetWorldTimerManager().GetTimerRemaining(TimerHandle_LifeSpanExpired);
            TimeRemaining = 0.f - (LifeSpanAfterExplosion - LifeSpanRemaining);
        }

        uint64 Identifier = GetActorLocation().X;
        GEngine->AddOnScreenDebugMessage(Identifier + 1, 0.0f, FColor::Yellow, GetActorLocation().ToString());
        GEngine->AddOnScreenDebugMessage(Identifier + 2, 0.0f, FColor::Green, FString::Printf(TEXT("Time: %f"), TimeRemaining));

        SetExplosionTilesNavTimeout(GridNavMesh, TimeRemaining);
    }
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

    if (ExplosionTimeout > 0.0f)
    {
        GetWorldTimerManager().SetTimer(TimerHandle_ExplosionTimeoutExpired, this, &ABomb::ExplosionTimeoutExpired, ExplosionTimeout);
    }
    else
    {
        GetWorldTimerManager().ClearTimer(TimerHandle_ExplosionTimeoutExpired);
    }

	SetLifeSpan(ExplosionTimeout + TileExplosionDelay * ExplosionRadiusTiles);

	if (IdleAnimation)
	{
		MeshComponent->PlayAnimation(IdleAnimation, true);
	}

	if (HasAuthority())
	{
		// Do not block players if they are overlapping a bomb
        {
            TArray<FOverlapResult> OutOverlaps{};
            FVector Location = GetActorLocation();
            Location.Z = Utils::RoundToUnitCenter(Location.Z);
            FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(Utils::Unit / 2));
            FCollisionObjectQueryParams QueryParams;
            QueryParams.AddObjectTypesToQuery(ECC_Pawn);
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
        }

		OverlapComponent->OnComponentEndOverlap.AddDynamic(this, &ABomb::HandleEndOverlap);

        auto* GridNavMesh = Cast<AGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AGridNavMesh::StaticClass()));
        FVector Location = GetActorLocation();
        auto Cost = FMath::Max<int64>(GridNavMesh->GetTileCost(Location), ETileNavCost::BOMB);
        GridNavMesh->SetTileCost(Location, Cost);
        GridNavMesh->SetTileTimeout(Location, AGridNavMesh::TIMEOUT_DEFAULT);
	}
}

void ABomb::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (bExplosionTriggered)
    {
        return;
    }

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
    if (bExplosionTriggered)
    {
        return;
    }

    // TODO HasAuthority?

    float CurrentExplosionTimeout = GetWorldTimerManager().GetTimerRemaining(TimerHandle_ExplosionTimeoutExpired);

    float CurrentLifeSpanTimeout = GetWorldTimerManager().GetTimerRemaining(TimerHandle_LifeSpanExpired);

    auto* GridNavMesh = Cast<AGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AGridNavMesh::StaticClass()));
    if (HasAuthority() && GridNavMesh)
    {
        FVector Location = GetActorLocation();
        GridNavMesh->SetTileCost(Location, 1);
    }

    BeginExplosion();

    OnBombExploded.Broadcast();

    MeshComponent->DestroyComponent();

    OverlapComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABomb::LifeSpanExpired()
{
    auto* GridNavMesh = Cast<AGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AGridNavMesh::StaticClass()));
    if (HasAuthority() && GridNavMesh)
    {
        UpdateExplosionConstraints();

        SetExplosionTilesNavTimeout(GridNavMesh, AGridNavMesh::TIMEOUT_DEFAULT);
    }

    Destroy();
}

void ABomb::BeginExplosion()
{
	if (!HasAuthority())
	{
		return;
	}

    bExplosionTriggered = true;

    UpdateExplosionConstraints();

	{
		FTransform Transform;
		Transform.SetLocation(GetActorLocation());
		Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, 0.f);
	}

    // Left
    for (uint8 Index = 1; Index <= ExplosionInfo.Left.Blocks; Index++)
    {
        FVector Location = GetActorLocation();
        Location.X = Utils::RoundToUnitCenter(Location.X) - Utils::Unit * Index;
        Location.Y = Utils::RoundToUnitCenter(Location.Y);
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, TileExplosionDelay * Index);
    }

    // Right
    for (uint8 Index = 1; Index <= ExplosionInfo.Right.Blocks; Index++)
    {
        FVector Location = GetActorLocation();
        Location.X = Utils::RoundToUnitCenter(Location.X) + Utils::Unit * Index;
        Location.Y = Utils::RoundToUnitCenter(Location.Y);
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, TileExplosionDelay * Index);
    }

    // Up
    for (uint8 Index = 1; Index <= ExplosionInfo.Up.Blocks; Index++)
    {
        FVector Location = GetActorLocation();
        Location.X = Utils::RoundToUnitCenter(Location.X);
        Location.Y = Utils::RoundToUnitCenter(Location.Y) - Utils::Unit * Index;
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, TileExplosionDelay * Index);
    }

    // Down
    for (uint8 Index = 1; Index <= ExplosionInfo.Down.Blocks; Index++)
    {
        FVector Location = GetActorLocation();
        Location.X = Utils::RoundToUnitCenter(Location.X);
        Location.Y = Utils::RoundToUnitCenter(Location.Y) + Utils::Unit * Index;
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, TileExplosionDelay * Index);
    }
}

void ABomb::ScheduleTileExplosion(FTransform Transform, float Delay)
{
    if (Delay != 0.f)
    {
        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, [World = GetWorld(), ExplosionClass = ExplosionClass, Transform]()
        {
            ExplodeTile(World, ExplosionClass, Transform);
        }, Delay, false);
    }
    else
    {
        ExplodeTile(GetWorld(), ExplosionClass, Transform);
    }
}

void ABomb::ExplodeTile(UWorld* World, TSubclassOf<AExplosion> ExplosionClass, FTransform Transform)
{
    FVector Location = Transform.GetLocation();
    Location.Z = Utils::RoundToUnitCenter(Location.Z);

    FCollisionObjectQueryParams QueryParams;
    QueryParams.AddObjectTypesToQuery(ECC_Pawn);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn1);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn2);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn3);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn4);
    QueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

    FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(Utils::Unit / 8, Utils::Unit / 8, Utils::Unit));

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
                if (Actor->IsA<ABomb>())
                {
                    FVector Delta = (Actor->GetActorLocation() - Start) / Utils::Unit;
                    float Timeout = GetWorldTimerManager().GetTimerRemaining(TimerHandle_ExplosionTimeoutExpired);
                    if (Timeout != -1.f)
                    {
                        Cast<ABomb>(Actor)->SetChainExplosionLifeSpan(Timeout + TileExplosionDelay * Delta.GetAbsMax());
                    }
                }

                if (IExplosiveInterface::Execute_IsBlockingExplosion(Actor))
                {
                    float DistanceRounded = FMath::RoundHalfFromZero(HitResult.Distance / Utils::Unit);
                    BlockingDistance = static_cast<uint32>(DistanceRounded);

                    break;
                }
            }
		}
		else
		{
            // Static walls
			float DistanceRounded = FMath::RoundToZero(HitResult.Distance / Utils::Unit);
			BlockingDistance = static_cast<uint32>(DistanceRounded);
		}
	}

	return BlockingDistance;
}

void ABomb::UpdateExplosionConstraints()
{
    FVector Start = GetActorLocation();

    // Left
    {
        FVector End = GetActorLocation();
        End.X = Utils::RoundToUnitCenter(End.X) - Utils::Unit * ExplosionRadiusTiles;

        uint64 Constraint = LineTraceExplosion(Start, End);
        ExplosionInfo.Left.Blocks = std::min(ExplosionRadiusTiles, Constraint);
    }

    // Right
    {
        FVector End = GetActorLocation();
        End.X = Utils::RoundToUnitCenter(End.X) + Utils::Unit * ExplosionRadiusTiles;

        uint64 Constraint = LineTraceExplosion(Start, End);
        ExplosionInfo.Right.Blocks = std::min(ExplosionRadiusTiles, Constraint);
    }

    // Up
    {
        FVector End = GetActorLocation();
        End.Y = Utils::RoundToUnitCenter(End.Y) - Utils::Unit * ExplosionRadiusTiles;

        uint64 Constraint = LineTraceExplosion(Start, End);
        ExplosionInfo.Up.Blocks = std::min(ExplosionRadiusTiles, Constraint);
    }

    // Down
    {
        FVector End = GetActorLocation();
        End.Y = Utils::RoundToUnitCenter(End.Y) + Utils::Unit * ExplosionRadiusTiles;

        uint64 Constraint = LineTraceExplosion(Start, End);
        ExplosionInfo.Down.Blocks = std::min(ExplosionRadiusTiles, Constraint);
    }
}

void ABomb::SetExplosionTilesNavTimeout(AGridNavMesh* GridNavMesh, float BombExplosionTimeout)
{
    for (uint32 Index = 1; Index <= ExplosionRadiusTiles; Index++)
    {
        float Timeout = BombExplosionTimeout + TileExplosionDelay * Index;

        if (Index <= ExplosionInfo.Left.Blocks)
        {
            FVector Location = GetActorLocation();
            Location.X = Utils::RoundToUnitCenter(Location.X) - Utils::Unit * Index;
            SetTileTimeout(GridNavMesh, Location, Timeout);
        }

        if (Index <= ExplosionInfo.Right.Blocks)
        {
            FVector Location = GetActorLocation();
            Location.X = Utils::RoundToUnitCenter(Location.X) + Utils::Unit * Index;
            SetTileTimeout(GridNavMesh, Location, Timeout);
        }

        if (Index <= ExplosionInfo.Up.Blocks)
        {
            FVector Location = GetActorLocation();
            Location.Y = Utils::RoundToUnitCenter(Location.Y) - Utils::Unit * Index;
            SetTileTimeout(GridNavMesh, Location, Timeout);
        }

        if (Index <= ExplosionInfo.Down.Blocks)
        {
            FVector Location = GetActorLocation();
            Location.Y = Utils::RoundToUnitCenter(Location.Y) + Utils::Unit * Index;
            SetTileTimeout(GridNavMesh, Location, Timeout);
        }
    }
}

void ABomb::SetTileTimeout(AGridNavMesh* GridNavMesh, FVector Location, float Timeout)
{
    if (Timeout > 0.f)
    {
        int64 CurrentCost = GridNavMesh->GetTileCost(Location);
        float CurrentTileTimeout = GridNavMesh->GetTileTimeout(Location);
        if (CurrentCost <= ETileNavCost::DEFAULT && (CurrentTileTimeout < 0 || CurrentTileTimeout > Timeout))
        {
            GridNavMesh->SetTileTimeout(Location, Timeout);
        }
    }
    else
    {
        GridNavMesh->SetTileTimeout(Location, AGridNavMesh::TIMEOUT_DEFAULT);
    }
}

void ABomb::ExplosionTimeoutExpired()
{
    BlowUp_Implementation();
}

void ABomb::SetChainExplosionLifeSpan(float Timeout)
{
    if (bExplosionTriggered)
    {
        return;
    }

    float CurrentExplosionTimeout = GetWorldTimerManager().GetTimerRemaining(TimerHandle_ExplosionTimeoutExpired);
    if (CurrentExplosionTimeout != -1.f && CurrentExplosionTimeout > Timeout)
    {
        GetWorldTimerManager().SetTimer(TimerHandle_ExplosionTimeoutExpired, this, &ABomb::ExplosionTimeoutExpired, Timeout);
        GetWorldTimerManager().SetTimer(TimerHandle_LifeSpanExpired, this, &AActor::LifeSpanExpired, Timeout + TileExplosionDelay * ExplosionRadiusTiles);
    }
}
