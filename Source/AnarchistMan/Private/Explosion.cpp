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
    PrimaryActorTick.TickInterval = 0.5f;

	// Create an overlap component
	OverlapComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapComponent"));

	// Set as root component
	RootComponent = OverlapComponent;

    // Create a particle system component
    ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystemComponent"));
    ParticleSystemComponent->SetupAttachment(RootComponent);

	LifeSpan = 1.f;
}

//void AExplosion::Tick(float DeltaTime)
//{
//    Super::Tick(DeltaTime);
//
//    TArray<FHitResult> OutHits{};
//    GetWorld()->LineTraceMultiByChannel(OutHits, GetActorLocation(), ParentBombLocation, ECollisionChannel::ECC_BombVisibility);
//
//    for (const FHitResult& HitResult : OutHits)
//    {
//        if (!HitResult.bBlockingHit)
//        {
//            bCanSeeParentBomb = true;
//        }
//    }
//}

//void AExplosion::SetParentBombLocation(FVector Location)
//{
//    Location.Z += 50.f;
//    ParentBombLocation = Location;
//}

//void AExplosion::ScheduleExplosion(float Delay)
//{
//    if (Delay != 0.f)
//    {
//        FTimerHandle TimerHandle;
//        GetWorldTimerManager().SetTimer(TimerHandle, this, &AExplosion::BeginExplosion, Delay, false);
//    }
//    else
//    {
//        BeginExplosion();
//    }
//}

//void AExplosion::BeginExplosion()
//{
//    if (bCanSeeParentBomb)
//    {
//        ParticleSystemComponent->Activate();
//
//        // TODO
//        //PathCostComponent->SetCost(0);
//
//        SetLifeSpan(LifeSpan);
//    }
//    else
//    {
//        Destroy();
//    }
//}

// Called when the game starts or when spawned
void AExplosion::BeginPlay()
{
	Super::BeginPlay();

    SetLifeSpan(LifeSpan);
}
