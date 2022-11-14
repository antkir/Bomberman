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

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Properties", meta = (ClampMin = "0"))
    int32 Rows;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Properties", meta = (ClampMin = "0"))
	int32 Columns;

	UPROPERTY(EditAnywhere, Category = "Properties", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float BreakableBlockSpawnChance;

    UPROPERTY(EditAnywhere, Category = "Properties", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float PowerUpSpawnChance;

    UPROPERTY(EditAnywhere, Category = "Properties", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float PowerUpsBatchSpawnChance;

	UPROPERTY(EditAnywhere, Category = "Classes")
	TSubclassOf<ABreakableBlock> BreakableBlockClass;

    UPROPERTY(EditAnywhere, Category = "Classes")
    TArray<TSubclassOf<APowerUp>> PowerUpClasses;

};
