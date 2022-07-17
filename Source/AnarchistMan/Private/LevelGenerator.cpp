// Fill out your copyright notice in the Description page of Project Settings.

#include "LevelGenerator.h"

#include <BreakableBlock.h>
#include <Utils.h>

#include <Kismet/GameplayStatics.h>

// Sets default values
ALevelGenerator::ALevelGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Rows = 5;
	Columns = 5;
	BreakableBlockDensity = 3;
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
}

void ALevelGenerator::SpawnBreakableBlocks()
{
	if (!HasAuthority())
	{
		return;
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

			if (FMath::RandHelper(BreakableBlockDensity))
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

			FVector Location = FVector(RootLocation.X + Row * Utils::Unit + Utils::Unit / 2, RootLocation.Y + Column * Utils::Unit + Utils::Unit / 2, RootLocation.Z);
			FTransform Transform;
			Transform.SetLocation(Location);
			Transform.SetRotation(FQuat::Identity);
			GetWorld()->SpawnActorAbsolute<ABreakableBlock>(BreakableBlockClass, Transform);
		}
	}
}
