// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ExplosiveInterface.h"

#include "Bomb.generated.h"

class AExplosion;
class AGridNavMesh;
class UAMNavModifierComponent;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBombExploded);

struct FExplosionInfo
{
    int32 LeftTiles;
    int32 RightTiles;
    int32 UpTiles;
    int32 DownTiles;
};

UCLASS()
class ABomb : public AActor, public IExplosiveInterface
{
	GENERATED_BODY()
	
public:

	// Sets default values for this actor's properties
	ABomb();

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

    static void ExplodeTile(UWorld* World, TSubclassOf<AExplosion> ExplosionClass, FTransform Transform);

    int32 LineTraceExplosion(FVector Start, FVector End);

    void UpdateExplosionConstraints();

    void SetExplosionTilesNavTimeout(AGridNavMesh* GridNavMesh, float BombExplosionTimeout);

    void SetTileTimeout(AGridNavMesh* GridNavMesh, FVector Location, float Timeout);

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

	UPROPERTY(EditAnywhere, Category = "Parameters")
	float ExplosionTimeout;

    UPROPERTY(EditAnywhere, Category = "Parameters")
    float TileExplosionDelay;

    UPROPERTY(EditAnywhere, Category = "Classes")
    TSubclassOf<AExplosion> ExplosionClass;

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
