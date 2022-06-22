// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BreakableBlock.generated.h"

UCLASS()
class ABreakableBlock : public AActor
{
	GENERATED_BODY()
	
public:

	// Sets default values for this actor's properties
	ABreakableBlock();

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComponent;

};
