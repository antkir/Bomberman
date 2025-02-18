// Copyright 2022 Kiryl Antonik

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "AmLevelGenerator.generated.h"

class AAmBreakableBlock;
class AAmPowerUp;

UCLASS()
class AAmLevelGenerator : public AActor
{
	GENERATED_BODY()

public:

	// Sets default values for this actor's properties
	AAmLevelGenerator();

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

	UPROPERTY(EditInstanceOnly, Category = "Properties", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float BreakableBlockSpawnChance;

	UPROPERTY(EditInstanceOnly, Category = "Properties", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float PowerUpSpawnChance;

	UPROPERTY(EditInstanceOnly, Category = "Properties", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float PowerUpsBatchSpawnChance;

	UPROPERTY(EditInstanceOnly, Category = "Classes")
	TSubclassOf<AAmBreakableBlock> BreakableBlockClass;

	UPROPERTY(EditInstanceOnly, Category = "Classes")
	TArray<TSubclassOf<AAmPowerUp>> PowerUpClasses;

};
