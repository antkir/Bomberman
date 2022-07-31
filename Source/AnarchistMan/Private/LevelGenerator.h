// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "LevelGenerator.generated.h"

class ABreakableBlock;
class APowerUp;

UCLASS()
class ALevelGenerator : public AActor
{
	GENERATED_BODY()
	
public:	

	// Sets default values for this actor's properties
	ALevelGenerator();

public:

	void RegenerateLevel();

    void SpawnPowerUpsBatch();

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:

    UFUNCTION()
    void BlockDestroyed(AActor* DestroyedActor);

protected:

	UPROPERTY(EditAnywhere, Category = "Properties")
	uint64 Rows;

	UPROPERTY(EditAnywhere, Category = "Properties")
	uint64 Columns;

	UPROPERTY(EditAnywhere, Category = "Properties")
	float BreakableBlockSpawnChance;

    UPROPERTY(EditAnywhere, Category = "Properties")
    float PowerUpSpawnChance;

    UPROPERTY(EditAnywhere, Category = "Properties")
    float PowerUpsBatchSpawnChance;

	UPROPERTY(EditAnywhere, Category = "Classes")
	TSubclassOf<ABreakableBlock> BreakableBlockClass;

    UPROPERTY(EditDefaultsOnly, Category = "Classes")
    TArray<TSubclassOf<APowerUp>> PowerUpClasses;

};
