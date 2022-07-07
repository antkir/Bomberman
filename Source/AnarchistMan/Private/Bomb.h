// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Bomb.generated.h"

class AExplosion;
class UBoxComponent;

UCLASS()
class ABomb : public AActor
{
	GENERATED_BODY()
	
public:

	// Sets default values for this actor's properties
	ABomb();

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_BlockPawns();

private:

	void LifeSpanExpired() override;

	void BlowUp();

	uint32 LineTraceExplosion(FVector Start, FVector End);

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

	UPROPERTY(EditAnywhere, Category = "Parameters")
	uint64 RadiusBlocks;

	/** A Replicated Boolean Flag */
	UPROPERTY(ReplicatedUsing = OnRep_BlockPawns)
	uint8 BlockPawnsMask;

private:

	struct
	{
		uint64 Right;
		uint64 Left;
		uint64 Up;
		uint64 Down;
	} ExplosionConstraints;

	bool ExplosionTriggered;

};
