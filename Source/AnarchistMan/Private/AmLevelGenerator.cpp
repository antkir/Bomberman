// Fill out your copyright notice in the Description page of Project Settings.

#include "AmLevelGenerator.h"

#include <Engine/Public/EngineUtils.h>
#include <Kismet/GameplayStatics.h>

#include <AmBreakableBlock.h>
#include <AmPowerUp.h>
#include <AmUtils.h>

AAmLevelGenerator::AAmLevelGenerator()
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
void AAmLevelGenerator::BeginPlay()
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

void AAmLevelGenerator::BlockDestroyed(AActor* DestroyedActor)
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
    Location.Z += FAmUtils::Unit / 2;
    Transform.SetLocation(Location);
    Transform.SetRotation(FQuat::Identity);
    int64 RandomIndex = FMath::RandHelper(PowerUpClasses.Num());
    GetWorld()->SpawnActorAbsolute<AAmPowerUp>(PowerUpClasses[RandomIndex], Transform);
}

void AAmLevelGenerator::RegenerateLevel()
{
	if (!HasAuthority())
	{
		return;
	}

    for (TActorIterator<AAmBreakableBlock> It(GetWorld()); It; ++It)
    {
        It->Destroy();
    }

    for (TActorIterator<AAmPowerUp> It(GetWorld()); It; ++It)
    {
        It->Destroy();
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

			FVector Location = FVector(RootLocation.X + Row * FAmUtils::Unit + FAmUtils::Unit / 2, RootLocation.Y + Column * FAmUtils::Unit + FAmUtils::Unit / 2, RootLocation.Z);
			FTransform Transform;
			Transform.SetLocation(Location);
			Transform.SetRotation(FQuat::Identity);
			auto* BreakableBlock = GetWorld()->SpawnActorAbsolute<AAmBreakableBlock>(BreakableBlockClass, Transform);
            BreakableBlock->OnDestroyed.AddDynamic(this, &AAmLevelGenerator::BlockDestroyed);
		}
	}
}

void AAmLevelGenerator::SpawnPowerUpsBatch()
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
            FVector Location = FVector(RootLocation.X + Row * FAmUtils::Unit + FAmUtils::Unit / 2, RootLocation.Y + Column * FAmUtils::Unit + FAmUtils::Unit / 2, RootLocation.Z + FAmUtils::Unit / 2);
            FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(FAmUtils::Unit / 2));
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
            GetWorld()->SpawnActorAbsolute<AAmPowerUp>(PowerUpClasses[RandomIndex], Transform);
        }
    }
}
