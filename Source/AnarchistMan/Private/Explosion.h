// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Explosion.generated.h"

class UBoxComponent;

UCLASS()
class AExplosion : public AActor
{
	GENERATED_BODY()
	
public:

	// Sets default values for this actor's properties
	AExplosion();

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* OverlapComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	UAnimSequence* IdleAnimation;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	float LifeSpan;

};
