// Copyright 2022 Kiryl Antonik

#include "AmExplosion.h"
#include "Components/BoxComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Game/AmUtils.h"

AAmExplosion::AAmExplosion()
{
	bReplicates = true;

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
void AAmExplosion::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(LifeSpan);
}
