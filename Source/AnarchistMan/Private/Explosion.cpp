// Fill out your copyright notice in the Description page of Project Settings.

#include "Explosion.h"

#include <PlayerCharacter.h>
#include <Utils.h>

#include <Components/BoxComponent.h>
#include <Particles/ParticleSystemComponent.h>

// Sets default values
AExplosion::AExplosion()
{
	bReplicates = true;

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create an overlap component
	OverlapComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapComponent"));

	// Set as root component
	RootComponent = OverlapComponent;

    // Create a particle system component
    ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystemComponent"));
    ParticleSystemComponent->SetupAttachment(RootComponent);

	LifeSpan = 1.f;
}

// Called when the game starts or when spawned
void AExplosion::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(LifeSpan);
}
