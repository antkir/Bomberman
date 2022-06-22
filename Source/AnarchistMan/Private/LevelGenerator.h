// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelGenerator.generated.h"

class ABreakableBlock;

UCLASS()
class ALevelGenerator : public AActor
{
	GENERATED_BODY()
	
public:	

	// Sets default values for this actor's properties
	ALevelGenerator();

protected:

	UPROPERTY(EditAnywhere, Category = "Generation")
	uint64 Rows;

	UPROPERTY(EditAnywhere, Category = "Generation")
	uint64 Columns;

	UPROPERTY(EditAnywhere, Category = "Generation")
	uint64 BreakableBlockDensity;

	UPROPERTY(EditAnywhere, Category = "Items")
	TSubclassOf<ABreakableBlock> BreakableBlockClass;

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:

	void SpawnBreakableBlocks();

};
