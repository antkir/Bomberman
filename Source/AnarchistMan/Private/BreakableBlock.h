// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ExplosiveInterface.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "BreakableBlock.generated.h"

class UAMNavModifierComponent;

UCLASS()
class ABreakableBlock : public AActor, public IExplosiveInterface
{
	GENERATED_BODY()
	
public:

	// Sets default values for this actor's properties
	ABreakableBlock();

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:

    void BeginPlay() override;

    virtual bool IsBlockingExplosion_Implementation() override;

    virtual void BlowUp_Implementation() override;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComponent;
};
