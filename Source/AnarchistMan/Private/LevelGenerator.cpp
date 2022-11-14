// Fill out your copyright notice in the Description page of Project Settings.

#include "LevelGenerator.h"

#include <Engine/Public/EngineUtils.h>
#include <Kismet/GameplayStatics.h>

#include <BreakableBlock.h>
#include <PowerUp.h>
#include <Utils.h>

ALevelGenerator::ALevelGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Rows = 5;
	Columns = 5;

    BreakableBlockSpawnChance = 100.f;
    PowerUpSpawnChance = 100.f;
    PowerUpsBatchSpawnChance = 100.f;
}

// Called when the game starts or when spawned
void ALevelGenerator::BeginPlay()
{
	Super::BeginPlay();

    if (BreakableBlockClass == nullptr)
    {
        UE_LOG(LogGame, Error, TEXT("BreakableBlockClass property is not set!"));
        return;
    }

    if (PowerUpClasses.IsEmpty())
    {
        UE_LOG(LogGame, Error, TEXT("PowerUpClasses property is not set!"));
    }
}

void ALevelGenerator::BlockDestroyed(AActor* DestroyedActor)
{
    if (PowerUpClasses.IsEmpty())
    {
        return;
    }

    if (FMath::RandHelper(100) >= PowerUpSpawnChance)
    {
        return;
    }

    FTransform Transform;
    FVector Location = DestroyedActor->GetActorLocation();
    Location.Z += FAMUtils::Unit / 2;
    Transform.SetLocation(Location);
    Transform.SetRotation(FQuat::Identity);
    int64 RandomIndex = FMath::RandHelper(PowerUpClasses.Num());
    GetWorld()->SpawnActorAbsolute<APowerUp>(PowerUpClasses[RandomIndex], Transform);
}

void ALevelGenerator::RegenerateLevel()
{
	if (!HasAuthority())
	{
		return;
	}

    for (TActorIterator<ABreakableBlock> It(GetWorld()); It; ++It)
    {
        ABreakableBlock* BreakableBlock = *It;
        BreakableBlock->Destroy();
    }

    for (TActorIterator<APowerUp> It(GetWorld()); It; ++It)
    {
        APowerUp* PowerUp = *It;
        PowerUp->Destroy();
    }

	FVector RootLocation = GetActorLocation();

	for (uint64 Row = 1; Row < Rows; Row++)
	{
		for (uint64 Column = 1; Column < Columns; Column++)
		{
			if (Row % 2 == 0 && Column % 2 == 0)
			{
				continue;
			}

            if (FMath::RandHelper(100) >= BreakableBlockSpawnChance)
            {
                continue;
            }

			if (Row < 3 || Row + 3 > Rows)
			{
				if (Column < 3 || Column + 3 > Columns)
				{
					continue;
				}
			}

			FVector Location = FVector(RootLocation.X + Row * FAMUtils::Unit + FAMUtils::Unit / 2, RootLocation.Y + Column * FAMUtils::Unit + FAMUtils::Unit / 2, RootLocation.Z);
			FTransform Transform;
			Transform.SetLocation(Location);
			Transform.SetRotation(FQuat::Identity);
			auto* BreakableBlock = GetWorld()->SpawnActorAbsolute<ABreakableBlock>(BreakableBlockClass, Transform);
            BreakableBlock->OnDestroyed.AddDynamic(this, &ALevelGenerator::BlockDestroyed);
		}
	}
}

void ALevelGenerator::SpawnPowerUpsBatch()
{
    FVector RootLocation = GetActorLocation();

    for (uint64 Row = 1; Row < Rows; Row++)
    {
        for (uint64 Column = 1; Column < Columns; Column++)
        {
            if (Row % 2 == 0 && Column % 2 == 0)
            {
                continue;
            }

            if (FMath::RandHelper(100) >= PowerUpsBatchSpawnChance)
            {
                continue;
            }

            TArray<FOverlapResult> OutOverlaps{};
            FVector Location = FVector(RootLocation.X + Row * FAMUtils::Unit + FAMUtils::Unit / 2, RootLocation.Y + Column * FAMUtils::Unit + FAMUtils::Unit / 2, RootLocation.Z + FAMUtils::Unit / 2);
            FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(FAMUtils::Unit / 2));
            FCollisionObjectQueryParams QueryParams;
            QueryParams.AddObjectTypesToQuery(ECC_Pawn1);
            QueryParams.AddObjectTypesToQuery(ECC_Pawn2);
            QueryParams.AddObjectTypesToQuery(ECC_Pawn3);
            QueryParams.AddObjectTypesToQuery(ECC_Pawn4);
            QueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
            bool IsOverlapping = GetWorld()->OverlapMultiByObjectType(OutOverlaps, Location, FQuat::Identity, QueryParams, CollisionShape);

            if (IsOverlapping)
            {
                continue;
            }

            FTransform Transform;
            Transform.SetLocation(Location);
            Transform.SetRotation(FQuat::Identity);
            int64 RandomIndex = FMath::RandHelper(PowerUpClasses.Num());
            GetWorld()->SpawnActorAbsolute<APowerUp>(PowerUpClasses[RandomIndex], Transform);
        }
    }
}
