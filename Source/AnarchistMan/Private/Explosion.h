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

public:

    //virtual void Tick(float DeltaTime) override;

    //void SetParentBombLocation(FVector Location);

    //void ScheduleExplosion(float Delay);

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:

    //void BeginExplosion();

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* OverlapComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UParticleSystemComponent* ParticleSystemComponent;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	float LifeSpan;

private:

    FVector ParentBombLocation;

    bool bCanSeeParentBomb;

};
