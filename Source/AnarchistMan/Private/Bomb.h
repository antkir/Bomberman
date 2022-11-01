// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ExplosiveInterface.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Bomb.generated.h"

class AExplosion;
class AGridNavMesh;
class UAMNavModifierComponent;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBombExploded);

struct FExplosionInfo
{
    struct FExplosionSideInfo
    {
        uint64 Blocks;
        bool Blocking;
    };

    FExplosionSideInfo Left;
    FExplosionSideInfo Right;
    FExplosionSideInfo Up;
    FExplosionSideInfo Down;

    AExplosion* CenterExplosion;
};

UCLASS()
class ABomb : public AActor, public IExplosiveInterface
{
	GENERATED_BODY()
	
public:

	// Sets default values for this actor's properties
	ABomb();

public:

    void SetExplosionRadiusTiles(uint64 Blocks);

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

    uint32 LineTraceExplosion(FVector Start, FVector End);

    void UpdateExplosionConstraints();

    void SetExplosionTilesNavTimeout(AGridNavMesh* GridNavMesh, float BombExplosionTimeout);

    void SetTileTimeout(AGridNavMesh* GridNavMesh, FVector Location, float Timeout);

    void ExplosionTimeoutExpired();

    void SetChainExplosionLifeSpan(float Timeout);

public:

    FBombExploded OnBombExploded;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* OverlapComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	UAnimSequence* IdleAnimation;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	float ExplosionTimeout;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	TSubclassOf<AExplosion> ExplosionClass;

	UPROPERTY(ReplicatedUsing = OnRep_BlockPawns)
	uint16 BlockPawnsMask;

    UPROPERTY(EditAnywhere, Category = "Parameters")
    float TileExplosionDelay;

private:

	struct ExplosionInfo
	{
        struct InfoDirection
        {
            uint64 Blocks;
            bool Blocking;
        };
		
        InfoDirection Left;
        InfoDirection Right;
        InfoDirection Up;
        InfoDirection Down;

        AExplosion* CenterExplosion;
	} ExplosionInfo;

    uint64 ExplosionRadiusTiles;

	bool bExplosionTriggered;

    FTimerHandle TimerHandle_ExplosionTimeoutExpired;
};
