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

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:

	void LifeSpanExpired() override;

	void BlowUp();

};
