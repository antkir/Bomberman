// Copyright 2022 Kiryl Antonik

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "AmExplosion.generated.h"

class UBoxComponent;

UCLASS()
class AAmExplosion : public AActor
{
	GENERATED_BODY()
	
public:

	// Sets default values for this actor's properties
	AAmExplosion();

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* OverlapComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UParticleSystemComponent* ParticleSystemComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Parameters")
	float LifeSpan;
};
