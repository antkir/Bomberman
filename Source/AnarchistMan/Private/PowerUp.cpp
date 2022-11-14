// Fill out your copyright notice in the Description page of Project Settings.

#include "PowerUp.h"

#include <Components/BoxComponent.h>

#include <PlayerCharacter.h>
#include <Utils.h>

APowerUp::APowerUp()
{
    bReplicates = true;

    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    OverlapComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapComponent"));
    RootComponent = OverlapComponent;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootComponent);

    ZDistance = 25.f;
}

void APowerUp::BeginPlay()
{
    Super::BeginPlay();

    StartLocation = GetActorLocation();

    if (CurveFloat)
    {
        FOnTimelineFloat TimelineProgress;
        TimelineProgress.BindUFunction(this, FName("TimelineProgress"));
        CurveTimeline.AddInterpFloat(CurveFloat, TimelineProgress);
        CurveTimeline.SetLooping(true);
        CurveTimeline.PlayFromStart();
    }

    OverlapComponent->OnComponentBeginOverlap.AddDynamic(this, &APowerUp::HandleBeginOverlap);
}

void APowerUp::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    CurveTimeline.TickTimeline(DeltaSeconds);
}

void APowerUp::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (HasAuthority())
    {
        auto* PlayerCharacter = Cast<APlayerCharacter>(OtherActor);
        if (PlayerCharacter)
        {
            Consume(PlayerCharacter);
        }
    }
}

bool APowerUp::IsBlockingExplosion_Implementation()
{
    return false;
}

void APowerUp::BlowUp_Implementation()
{
    Destroy();
}

void APowerUp::TimelineProgress(float Delta)
{
    FVector EndLocation = StartLocation;
    EndLocation.Z += ZDistance;

    FVector Location = FMath::Lerp(StartLocation, EndLocation, Delta);
    SetActorLocation(Location);
}
