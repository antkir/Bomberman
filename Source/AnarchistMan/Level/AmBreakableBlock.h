// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "AmExplosiveInterface.h"

#include "AmBreakableBlock.generated.h"

UCLASS()
class AAmBreakableBlock : public AActor, public IAmExplosiveInterface
{
	GENERATED_BODY()
	
public:

	// Sets default values for this actor's properties
	AAmBreakableBlock();

protected:

	void BeginPlay() override;

	virtual bool IsBlockingExplosion_Implementation() override;

	virtual void BlowUp_Implementation() override;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComponent;
};
