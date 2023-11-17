// Copyright 2022 Kiryl Antonik

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "AmExplosiveInterface.h"

#include "AmBomb.generated.h"

class AAmExplosion;
class AAmGridNavMesh;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBombExploded);

UCLASS()
class AAmBomb : public AActor, public IAmExplosiveInterface
{
	GENERATED_BODY()

	struct FExplosionInfo
	{
		int32 LeftTiles;
		int32 RightTiles;
		int32 UpTiles;
		int32 DownTiles;
	};
	
public:

	// Sets default values for this actor's properties
	AAmBomb();

public:

	void SetExplosionRadiusTiles(int32 Blocks);

	virtual void Tick(float DeltaTime) override;

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_BlockPawns();

	virtual bool IsBlockingExplosion_Implementation() override;

	virtual void BlowUp_Implementation() override;

private:

	virtual void LifeSpanExpired() override;

	void BeginExplosion();

	void ScheduleTileExplosion(FTransform Transform, float Delay);

	static void ExplodeTile(UWorld* World, TSubclassOf<AAmExplosion> ExplosionClass, FTransform Transform);

	int32 LineTraceExplosion(FVector Start, FVector End);

	void UpdateExplosionConstraints();

	void SetExplosionTilesNavTimeout(AAmGridNavMesh* GridNavMesh, float BombExplosionTimeout);

	void SetTileTimeout(AAmGridNavMesh* GridNavMesh, FVector Location, float Timeout);

	void ExplosionTimeoutExpired();

	void SetChainExplosionLifeSpan(float Timeout);

public:

	UPROPERTY(BlueprintAssignable)
	FBombExploded OnBombExploded;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* OverlapComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Parameters")
	float ExplosionTimeout;

	UPROPERTY(EditDefaultsOnly, Category = "Parameters")
	float TileExplosionDelay;

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AAmExplosion> ExplosionClass;

	UPROPERTY(ReplicatedUsing = OnRep_BlockPawns)
	uint8 BlockPawnsMask;

	UPROPERTY(BlueprintReadOnly)
	int32 ExplosionMaxRadiusTiles;

	UPROPERTY(BlueprintReadOnly)
	bool bExplosionTriggered;

private:

	FExplosionInfo ExplosionInfo;

	FTimerHandle TimerHandle_ExplosionTimeoutExpired;
};
