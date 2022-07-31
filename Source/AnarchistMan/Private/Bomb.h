// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ExplosiveInterface.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Bomb.generated.h"

class AExplosion;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBombExploded);

UCLASS()
class ABomb : public AActor, public IExplosiveInterface
{
	GENERATED_BODY()
	
public:

	// Sets default values for this actor's properties
	ABomb();

public:

    void SetExplosionConsttraintBlocks(uint64 Blocks);

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_BlockPawns();

    bool IsBlockingExplosion_Implementation() override;

    void BlowUp_Implementation() override;

private:

	void LifeSpanExpired() override;

	void StartExplosion();

    static void ExplodeTile(UWorld* World, TSubclassOf<AExplosion> ExplosionClass, FTransform Transform);

	uint32 LineTraceExplosion(FVector Start, FVector End);

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
	float LifeSpan;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	TSubclassOf<AExplosion> ExplosionClass;

	UPROPERTY(ReplicatedUsing = OnRep_BlockPawns)
	uint8 BlockPawnsMask;

    UPROPERTY(EditAnywhere, Category = "Parameters")
    float TileExplosionDelay;

private:

	struct ExplosionConstraints
	{
		uint64 Right;
		uint64 Left;
		uint64 Up;
		uint64 Down;
	};

    uint64 ExplosionConstraintBlocks;

	bool ExplosionTriggered;

};
