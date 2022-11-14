// Fill out your copyright notice in the Description page of Project Settings.

#include "Bomb.h"

#include <Components/BoxComponent.h>
#include <Components/CapsuleComponent.h>
#include <Kismet/GameplayStatics.h>
#include <Net/UnrealNetwork.h>

#include <Explosion.h>
#include <GridNavMesh.h>
#include <PlayerCharacter.h>
#include <Utils.h>

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
	MeshComponent->SetupAttachment(RootComponent);

    TileExplosionDelay = 0.1f;

    ExplosionTimeout = 3.f;

    ExplosionMaxRadiusTiles = 2;

    bExplosionTriggered = 0;

    BlockPawnsMask = 0;
}

void ABomb::SetExplosionRadiusTiles(int32 Radius)
{
    ExplosionMaxRadiusTiles = Radius;
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

    if (HasAuthority())
    {
        auto* GridNavMesh = Cast<AGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AGridNavMesh::StaticClass()));
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
void ABomb::BeginPlay()
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
            GetWorldTimerManager().SetTimer(TimerHandle_ExplosionTimeoutExpired, this, &ABomb::ExplosionTimeoutExpired, ExplosionTimeout);
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
            Location.Z = FAMUtils::RoundToUnitCenter(Location.Z);
            FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(FAMUtils::Unit / 2));
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
                    BlockMask ^= FAMUtils::GetPlayerIdFromPawnECC(PlayerCollisionChannel);
                }
            }

            BlockPawnsMask = BlockMask;
            OnRep_BlockPawns();
        }

		OverlapComponent->OnComponentEndOverlap.AddDynamic(this, &ABomb::HandleEndOverlap);

        auto* GridNavMesh = Cast<AGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AGridNavMesh::StaticClass()));
        FVector Location = GetActorLocation();
        auto Cost = FMath::Max<int64>(GridNavMesh->GetTileCost(Location), ETileNavCost::BOMB);
        GridNavMesh->SetTileCost(Location, Cost);
        GridNavMesh->SetTileTimeout(Location, AGridNavMesh::TIMEOUT_UNSET);
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
		BlockPawnsMask |= FAMUtils::GetPlayerIdFromPawnECC(PlayerCollisionChannel);
	}
	OnRep_BlockPawns();
}

void ABomb::OnRep_BlockPawns()
{
	for (int32 Index = 0; Index < FAMUtils::MaxPlayers; Index++)
	{
		if (BlockPawnsMask & (1 << Index))
		{
			OverlapComponent->SetCollisionResponseToChannel(FAMUtils::PlayerECCs[Index], ECollisionResponse::ECR_Block);
		}
		else
		{
			OverlapComponent->SetCollisionResponseToChannel(FAMUtils::PlayerECCs[Index], ECollisionResponse::ECR_Overlap);
		}
	}
}

bool ABomb::IsBlockingExplosion_Implementation()
{
    return true;
}

void ABomb::BlowUp_Implementation()
{
    check(HasAuthority());

    if (bExplosionTriggered)
    {
        return;
    }

    auto* GridNavMesh = Cast<AGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AGridNavMesh::StaticClass()));
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

void ABomb::LifeSpanExpired()
{
    auto* GridNavMesh = Cast<AGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AGridNavMesh::StaticClass()));
    if (HasAuthority() && GridNavMesh)
    {
        UpdateExplosionConstraints();

        SetExplosionTilesNavTimeout(GridNavMesh, AGridNavMesh::TIMEOUT_UNSET);
    }

    Destroy();
}

void ABomb::BeginExplosion()
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
        Location.X = FAMUtils::RoundToUnitCenter(Location.X) - FAMUtils::Unit * Index;
        Location.Y = FAMUtils::RoundToUnitCenter(Location.Y);
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, TileExplosionDelay * Index);
    }

    // Right
    for (int32 Index = 1; Index <= ExplosionInfo.RightTiles; Index++)
    {
        FVector Location = GetActorLocation();
        Location.X = FAMUtils::RoundToUnitCenter(Location.X) + FAMUtils::Unit * Index;
        Location.Y = FAMUtils::RoundToUnitCenter(Location.Y);
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, TileExplosionDelay * Index);
    }

    // Up
    for (int32 Index = 1; Index <= ExplosionInfo.UpTiles; Index++)
    {
        FVector Location = GetActorLocation();
        Location.X = FAMUtils::RoundToUnitCenter(Location.X);
        Location.Y = FAMUtils::RoundToUnitCenter(Location.Y) - FAMUtils::Unit * Index;
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, TileExplosionDelay * Index);
    }

    // Down
    for (int32 Index = 1; Index <= ExplosionInfo.DownTiles; Index++)
    {
        FVector Location = GetActorLocation();
        Location.X = FAMUtils::RoundToUnitCenter(Location.X);
        Location.Y = FAMUtils::RoundToUnitCenter(Location.Y) + FAMUtils::Unit * Index;
        FTransform Transform;
        Transform.SetLocation(Location);
        Transform.SetRotation(FQuat::Identity);

        ScheduleTileExplosion(Transform, TileExplosionDelay * Index);
    }
}

void ABomb::ScheduleTileExplosion(FTransform Transform, float Delay)
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

void ABomb::ExplodeTile(UWorld* World, TSubclassOf<AExplosion> ExplosionClass, FTransform Transform)
{
    if (World == nullptr)
    {
        return;
    }

    check(!World->IsNetMode(NM_Client));

    FVector Location = Transform.GetLocation();
    Location.Z = FAMUtils::RoundToUnitCenter(Location.Z);

    FCollisionObjectQueryParams QueryParams;
    QueryParams.AddObjectTypesToQuery(ECC_Pawn);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn1);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn2);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn3);
    QueryParams.AddObjectTypesToQuery(ECC_Pawn4);
    QueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

    FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(FAMUtils::Unit / 8, FAMUtils::Unit / 8, FAMUtils::Unit));

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

int32 ABomb::LineTraceExplosion(FVector Start, FVector End)
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

            if (Actor->Implements<UExplosiveInterface>())
            {
                if (Actor->IsA<ABomb>())
                {
                    FVector Delta = (Actor->GetActorLocation() - Start) / FAMUtils::Unit;
                    float Timeout = GetWorldTimerManager().GetTimerRemaining(TimerHandle_ExplosionTimeoutExpired);
                    if (Timeout != -1.f)
                    {
                        Cast<ABomb>(Actor)->SetChainExplosionLifeSpan(Timeout + TileExplosionDelay * Delta.GetAbsMax());
                    }
                }

                if (IExplosiveInterface::Execute_IsBlockingExplosion(Actor))
                {
                    float DistanceRounded = FMath::RoundHalfFromZero(HitResult.Distance / FAMUtils::Unit);
                    ExplosionRadiusTiles = static_cast<int32>(DistanceRounded);

                    break;
                }
            }
		}
		else
		{
            // Static walls
			float DistanceRounded = FMath::RoundToZero(HitResult.Distance / FAMUtils::Unit);
            ExplosionRadiusTiles = static_cast<int32>(DistanceRounded);
		}
	}

	return ExplosionRadiusTiles;
}

void ABomb::UpdateExplosionConstraints()
{
    FVector Start = GetActorLocation();

    // Left
    {
        FVector End = GetActorLocation();
        End.X = FAMUtils::RoundToUnitCenter(End.X) - FAMUtils::Unit * ExplosionMaxRadiusTiles;

        int32 Constraint = LineTraceExplosion(Start, End);
        ExplosionInfo.LeftTiles = std::min(ExplosionMaxRadiusTiles, Constraint);
    }

    // Right
    {
        FVector End = GetActorLocation();
        End.X = FAMUtils::RoundToUnitCenter(End.X) + FAMUtils::Unit * ExplosionMaxRadiusTiles;

        int32 Constraint = LineTraceExplosion(Start, End);
        ExplosionInfo.RightTiles = std::min(ExplosionMaxRadiusTiles, Constraint);
    }

    // Up
    {
        FVector End = GetActorLocation();
        End.Y = FAMUtils::RoundToUnitCenter(End.Y) - FAMUtils::Unit * ExplosionMaxRadiusTiles;

        int32 Constraint = LineTraceExplosion(Start, End);
        ExplosionInfo.UpTiles = std::min(ExplosionMaxRadiusTiles, Constraint);
    }

    // Down
    {
        FVector End = GetActorLocation();
        End.Y = FAMUtils::RoundToUnitCenter(End.Y) + FAMUtils::Unit * ExplosionMaxRadiusTiles;

        int32 Constraint = LineTraceExplosion(Start, End);
        ExplosionInfo.DownTiles = std::min(ExplosionMaxRadiusTiles, Constraint);
    }
}

void ABomb::SetExplosionTilesNavTimeout(AGridNavMesh* GridNavMesh, float BombExplosionTimeout)
{
    for (int32 Index = 1; Index <= ExplosionMaxRadiusTiles; Index++)
    {
        float Timeout = BombExplosionTimeout + TileExplosionDelay * Index;

        if (Index <= ExplosionInfo.LeftTiles)
        {
            FVector Location = GetActorLocation();
            Location.X = FAMUtils::RoundToUnitCenter(Location.X) - FAMUtils::Unit * Index;
            SetTileTimeout(GridNavMesh, Location, Timeout);
        }

        if (Index <= ExplosionInfo.RightTiles)
        {
            FVector Location = GetActorLocation();
            Location.X = FAMUtils::RoundToUnitCenter(Location.X) + FAMUtils::Unit * Index;
            SetTileTimeout(GridNavMesh, Location, Timeout);
        }

        if (Index <= ExplosionInfo.UpTiles)
        {
            FVector Location = GetActorLocation();
            Location.Y = FAMUtils::RoundToUnitCenter(Location.Y) - FAMUtils::Unit * Index;
            SetTileTimeout(GridNavMesh, Location, Timeout);
        }

        if (Index <= ExplosionInfo.DownTiles)
        {
            FVector Location = GetActorLocation();
            Location.Y = FAMUtils::RoundToUnitCenter(Location.Y) + FAMUtils::Unit * Index;
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
        GridNavMesh->SetTileTimeout(Location, AGridNavMesh::TIMEOUT_UNSET);
    }
}

void ABomb::ExplosionTimeoutExpired()
{
    Execute_BlowUp(this);
}

void ABomb::SetChainExplosionLifeSpan(float Timeout)
{
    check(HasAuthority());

    if (bExplosionTriggered)
    {
        return;
    }

    float CurrentExplosionTimeout = GetWorldTimerManager().GetTimerRemaining(TimerHandle_ExplosionTimeoutExpired);
    if (CurrentExplosionTimeout != -1.f && CurrentExplosionTimeout > Timeout)
    {
        GetWorldTimerManager().SetTimer(TimerHandle_ExplosionTimeoutExpired, this, &ABomb::ExplosionTimeoutExpired, Timeout);
        GetWorldTimerManager().SetTimer(TimerHandle_LifeSpanExpired, this, &AActor::LifeSpanExpired, Timeout + TileExplosionDelay * ExplosionMaxRadiusTiles);
    }
}
