// Fill out your copyright notice in the Description page of Project Settings.

#include "AmBomb.h"

#include <Components/BoxComponent.h>
#include <Components/CapsuleComponent.h>
#include <Kismet/GameplayStatics.h>
#include <Net/UnrealNetwork.h>

#include <AmExplosion.h>
#include <AmGridNavMesh.h>
#include <AmMainPlayerCharacter.h>
#include <AmUtils.h>

AAmBomb::AAmBomb()
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
	MeshComponent->SetupAttachment(RootComponent);

    TileExplosionDelay = 0.1f;

    ExplosionTimeout = 3.f;

    ExplosionMaxRadiusTiles = 2;

    bExplosionTriggered = 0;

    BlockPawnsMask = 0;
}

void AAmBomb::SetExplosionRadiusTiles(int32 Radius)
{
    ExplosionMaxRadiusTiles = Radius;
}

void AAmBomb::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	auto DoRepLifetimeParams = FDoRepLifetimeParams();
	DoRepLifetimeParams.RepNotifyCondition = ELifetimeRepNotifyCondition::REPNOTIFY_Always;
	DOREPLIFETIME_WITH_PARAMS(AAmBomb, BlockPawnsMask, DoRepLifetimeParams);
}

void AAmBomb::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (HasAuthority())
    {
        auto* GridNavMesh = Cast<AAmGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AAmGridNavMesh::StaticClass()));
        if (GridNavMesh)
        {
            UpdateExplosionConstraints();

            float ExplosionTimeRemaining = GetWorldTimerManager().GetTimerRemaining(TimerHandle_ExplosionTimeoutExpired);
            if (ExplosionTimeRemaining == -1.f)
            {
                float LifeSpanAfterExplosion = TileExplosionDelay * ExplosionMaxRadiusTiles;
                float LifeSpanRemaining = GetWorldTimerManager().GetTimerRemaining(TimerHandle_LifeSpanExpired);
                ExplosionTimeRemaining = -(LifeSpanAfterExplosion - LifeSpanRemaining);
            }

            SetExplosionTilesNavTimeout(GridNavMesh, ExplosionTimeRemaining);
        }
    }
}

// Called when the game starts or when spawned
void AAmBomb::BeginPlay()
{
	Super::BeginPlay();

    if (ExplosionClass == nullptr)
    {
        UE_LOG(LogGame, Error, TEXT("ExplosionClass property is not set!"));
        return;
    }

	if (HasAuthority())
    {
        if (ExplosionTimeout > 0.0f)
        {
            GetWorldTimerManager().SetTimer(TimerHandle_ExplosionTimeoutExpired, this, &AAmBomb::ExplosionTimeoutExpired, ExplosionTimeout);
        }
        else
        {
            GetWorldTimerManager().ClearTimer(TimerHandle_ExplosionTimeoutExpired);
        }

        SetLifeSpan(ExplosionTimeout + TileExplosionDelay * ExplosionMaxRadiusTiles);

		// Do not block players if they are overlapping a bomb
        {
            uint8 BlockMask = std::numeric_limits<uint8>::max();

            TArray<FOverlapResult> OutOverlaps{};
            FVector Location = GetActorLocation();
            Location.Z = FAmUtils::RoundToUnitCenter(Location.Z);
            FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(FAmUtils::Unit / 2));
            FCollisionObjectQueryParams QueryParams;
            QueryParams.AddObjectTypesToQuery(ECC_Pawn);
            QueryParams.AddObjectTypesToQuery(ECC_Pawn1);
            QueryParams.AddObjectTypesToQuery(ECC_Pawn2);
            QueryParams.AddObjectTypesToQuery(ECC_Pawn3);
            QueryParams.AddObjectTypesToQuery(ECC_Pawn4);
            GetWorld()->OverlapMultiByObjectType(OutOverlaps, Location, FQuat::Identity, QueryParams, CollisionShape);

            for (const FOverlapResult& Overlap : OutOverlaps)
            {
                auto* PlayerCharacter = Cast<AAmMainPlayerCharacter>(Overlap.GetActor());
                if (PlayerCharacter)
                {
                    ECollisionChannel PlayerCollisionChannel = PlayerCharacter->GetCapsuleComponent()->GetCollisionObjectType();
                    BlockMask ^= FAmUtils::GetPlayerIdFromPawnECC(PlayerCollisionChannel);
                }
            }

            BlockPawnsMask = BlockMask;
            OnRep_BlockPawns();
        }

		OverlapComponent->OnComponentEndOverlap.AddDynamic(this, &AAmBomb::HandleEndOverlap);

        auto* GridNavMesh = Cast<AAmGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AAmGridNavMesh::StaticClass()));
        FVector Location = GetActorLocation();
        auto Cost = FMath::Max<int64>(GridNavMesh->GetTileCost(Location), ETileNavCost::BOMB);
        GridNavMesh->SetTileCost(Location, Cost);
        GridNavMesh->SetTileTimeout(Location, AAmGridNavMesh::TIMEOUT_UNSET);
	}
}

void AAmBomb::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (bExplosionTriggered)
    {
        return;
    }

	auto* PlayerCharacter = Cast<AAmMainPlayerCharacter>(OtherActor);
	if (PlayerCharacter)
	{
		ECollisionChannel PlayerCollisionChannel = PlayerCharacter->GetCapsuleComponent()->GetCollisionObjectType();
		BlockPawnsMask |= FAmUtils::GetPlayerIdFromPawnECC(PlayerCollisionChannel);
	}
	OnRep_BlockPawns();
}

void AAmBomb::OnRep_BlockPawns()
{
	for (int32 Index = 0; Index < FAmUtils::MaxPlayers; Index++)
	{
		if (BlockPawnsMask & (1 << Index))
		{
			OverlapComponent->SetCollisionResponseToChannel(FAmUtils::PlayerECCs[Index], ECollisionResponse::ECR_Block);
		}
		else
		{
			OverlapComponent->SetCollisionResponseToChannel(FAmUtils::PlayerECCs[Index], ECollisionResponse::ECR_Overlap);
		}
	}
}

bool AAmBomb::IsBlockingExplosion_Implementation()
{
    return true;
}

void AAmBomb::BlowUp_Implementation()
{
    check(HasAuthority());

    if (bExplosionTriggered)
    {
        return;
    }

    auto* GridNavMesh = Cast<AAmGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AAmGridNavMesh::StaticClass()));
    if (GridNavMesh)
    {
        FVector Location = GetActorLocation();
        GridNavMesh->SetTileCost(Location, 1);
    }

    BeginExplosion();

    OnBombExploded.Broadcast();

    // Allow going through the bomb.
    BlockPawnsMask = 0;
    OnRep_BlockPawns();
}

void AAmBomb::LifeSpanExpired()
{
    auto* GridNavMesh = Cast<AAmGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AAmGridNavMesh::StaticClass()));
    if (HasAuthority() && GridNavMesh)
    {
        UpdateExplosionConstraints();

        SetExplosionTilesNavTimeout(GridNavMesh, AAmGridNavMesh::TIMEOUT_UNSET);
    }

    Destroy();
}

void AAmBomb::BeginExplosion()
{
    check(HasAuthority());

    bExplosionTriggered = true;

    UpdateExplosionConstraints();

	{
		FTransform Transform;
		Transform.SetLocation(GetActorLocation());
		Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, 0.f);
	}

    // Left
    for (int32 Index = 1; Index <= ExplosionInfo.LeftTiles; Index++)
    {
        FVector Location = GetActorLocation();
        Location.X = FAmUtils::RoundToUnitCenter(Location.X) - FAmUtils::Unit * Index;
        Location.Y = FAmUtils::RoundToUnitCenter(Location.Y);
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, TileExplosionDelay * Index);
    }

    // Right
    for (int32 Index = 1; Index <= ExplosionInfo.RightTiles; Index++)
    {
        FVector Location = GetActorLocation();
        Location.X = FAmUtils::RoundToUnitCenter(Location.X) + FAmUtils::Unit * Index;
        Location.Y = FAmUtils::RoundToUnitCenter(Location.Y);
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, TileExplosionDelay * Index);
    }

    // Up
    for (int32 Index = 1; Index <= ExplosionInfo.UpTiles; Index++)
    {
        FVector Location = GetActorLocation();
        Location.X = FAmUtils::RoundToUnitCenter(Location.X);
        Location.Y = FAmUtils::RoundToUnitCenter(Location.Y) - FAmUtils::Unit * Index;
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, TileExplosionDelay * Index);
    }

    // Down
    for (int32 Index = 1; Index <= ExplosionInfo.DownTiles; Index++)
    {
        FVector Location = GetActorLocation();
        Location.X = FAmUtils::RoundToUnitCenter(Location.X);
        Location.Y = FAmUtils::RoundToUnitCenter(Location.Y) + FAmUtils::Unit * Index;
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, TileExplosionDelay * Index);
    }
}

void AAmBomb::ScheduleTileExplosion(FTransform Transform, float Delay)
{
    check(HasAuthority());

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

void AAmBomb::ExplodeTile(UWorld* World, TSubclassOf<AAmExplosion> ExplosionClass, FTransform Transform)
{
    if (World == nullptr)
    {
        return;
    }

    check(!World->IsNetMode(NM_Client));

    FVector Location = Transform.GetLocation();
    Location.Z = FAmUtils::RoundToUnitCenter(Location.Z);

    FCollisionObjectQueryParams QueryParams;
    QueryParams.AddObjectTypesToQuery(ECC_Pawn);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn1);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn2);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn3);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn4);
    QueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

    FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(FAmUtils::Unit / 8, FAmUtils::Unit / 8, FAmUtils::Unit));

    TArray<FOverlapResult> OutOverlaps{};
    World->OverlapMultiByObjectType(OutOverlaps, Location, FQuat::Identity, QueryParams, CollisionShape);

    for (const FOverlapResult& Overlap : OutOverlaps)
    {
        AActor* Actor = Overlap.GetActor();
        if (IsValid(Actor) && Actor->Implements<UAmExplosiveInterface>())
        {
            IAmExplosiveInterface::Execute_BlowUp(Actor);
        }
    }

    World->SpawnActorAbsolute(ExplosionClass, Transform);
}

int32 AAmBomb::LineTraceExplosion(FVector Start, FVector End)
{
	int32 ExplosionRadiusTiles = std::numeric_limits<int32>::max();

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

            if (Actor->Implements<UAmExplosiveInterface>())
            {
                if (Actor->IsA<AAmBomb>())
                {
                    FVector Delta = (Actor->GetActorLocation() - Start) / FAmUtils::Unit;
                    float Timeout = GetWorldTimerManager().GetTimerRemaining(TimerHandle_ExplosionTimeoutExpired);
                    if (Timeout != -1.f)
                    {
                        Cast<AAmBomb>(Actor)->SetChainExplosionLifeSpan(Timeout + TileExplosionDelay * Delta.GetAbsMax());
                    }
                }

                if (IAmExplosiveInterface::Execute_IsBlockingExplosion(Actor))
                {
                    float DistanceRounded = FMath::RoundHalfFromZero(HitResult.Distance / FAmUtils::Unit);
                    ExplosionRadiusTiles = static_cast<int32>(DistanceRounded);

                    break;
                }
            }
		}
		else
		{
            // Static walls
			float DistanceRounded = FMath::RoundToZero(HitResult.Distance / FAmUtils::Unit);
            ExplosionRadiusTiles = static_cast<int32>(DistanceRounded);
		}
	}

	return ExplosionRadiusTiles;
}

void AAmBomb::UpdateExplosionConstraints()
{
    FVector Start = GetActorLocation();

    // Left
    {
        FVector End = GetActorLocation();
        End.X = FAmUtils::RoundToUnitCenter(End.X) - FAmUtils::Unit * ExplosionMaxRadiusTiles;

        int32 Constraint = LineTraceExplosion(Start, End);
        ExplosionInfo.LeftTiles = std::min(ExplosionMaxRadiusTiles, Constraint);
    }

    // Right
    {
        FVector End = GetActorLocation();
        End.X = FAmUtils::RoundToUnitCenter(End.X) + FAmUtils::Unit * ExplosionMaxRadiusTiles;

        int32 Constraint = LineTraceExplosion(Start, End);
        ExplosionInfo.RightTiles = std::min(ExplosionMaxRadiusTiles, Constraint);
    }

    // Up
    {
        FVector End = GetActorLocation();
        End.Y = FAmUtils::RoundToUnitCenter(End.Y) - FAmUtils::Unit * ExplosionMaxRadiusTiles;

        int32 Constraint = LineTraceExplosion(Start, End);
        ExplosionInfo.UpTiles = std::min(ExplosionMaxRadiusTiles, Constraint);
    }

    // Down
    {
        FVector End = GetActorLocation();
        End.Y = FAmUtils::RoundToUnitCenter(End.Y) + FAmUtils::Unit * ExplosionMaxRadiusTiles;

        int32 Constraint = LineTraceExplosion(Start, End);
        ExplosionInfo.DownTiles = std::min(ExplosionMaxRadiusTiles, Constraint);
    }
}

void AAmBomb::SetExplosionTilesNavTimeout(AAmGridNavMesh* GridNavMesh, float BombExplosionTimeout)
{
    for (int32 Index = 1; Index <= ExplosionMaxRadiusTiles; Index++)
    {
        float Timeout = BombExplosionTimeout + TileExplosionDelay * Index;

        if (Index <= ExplosionInfo.LeftTiles)
        {
            FVector Location = GetActorLocation();
            Location.X = FAmUtils::RoundToUnitCenter(Location.X) - FAmUtils::Unit * Index;
            SetTileTimeout(GridNavMesh, Location, Timeout);
        }

        if (Index <= ExplosionInfo.RightTiles)
        {
            FVector Location = GetActorLocation();
            Location.X = FAmUtils::RoundToUnitCenter(Location.X) + FAmUtils::Unit * Index;
            SetTileTimeout(GridNavMesh, Location, Timeout);
        }

        if (Index <= ExplosionInfo.UpTiles)
        {
            FVector Location = GetActorLocation();
            Location.Y = FAmUtils::RoundToUnitCenter(Location.Y) - FAmUtils::Unit * Index;
            SetTileTimeout(GridNavMesh, Location, Timeout);
        }

        if (Index <= ExplosionInfo.DownTiles)
        {
            FVector Location = GetActorLocation();
            Location.Y = FAmUtils::RoundToUnitCenter(Location.Y) + FAmUtils::Unit * Index;
            SetTileTimeout(GridNavMesh, Location, Timeout);
        }
    }
}

void AAmBomb::SetTileTimeout(AAmGridNavMesh* GridNavMesh, FVector Location, float Timeout)
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
        GridNavMesh->SetTileTimeout(Location, AAmGridNavMesh::TIMEOUT_UNSET);
    }
}

void AAmBomb::ExplosionTimeoutExpired()
{
    Execute_BlowUp(this);
}

void AAmBomb::SetChainExplosionLifeSpan(float Timeout)
{
    check(HasAuthority());

    if (bExplosionTriggered)
    {
        return;
    }

    float CurrentExplosionTimeout = GetWorldTimerManager().GetTimerRemaining(TimerHandle_ExplosionTimeoutExpired);
    if (CurrentExplosionTimeout != -1.f && CurrentExplosionTimeout > Timeout)
    {
        GetWorldTimerManager().SetTimer(TimerHandle_ExplosionTimeoutExpired, this, &AAmBomb::ExplosionTimeoutExpired, Timeout);
        GetWorldTimerManager().SetTimer(TimerHandle_LifeSpanExpired, this, &AActor::LifeSpanExpired, Timeout + TileExplosionDelay * ExplosionMaxRadiusTiles);
    }
}
