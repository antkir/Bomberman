// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ExplosiveInterface.h"

#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "PowerUp.generated.h"

class APlayerCharacter;
class UBoxComponent;

UCLASS()
class APowerUp : public AActor, public IExplosiveInterface
{
    GENERATED_BODY()

public:
    APowerUp();

public:

    UFUNCTION(BlueprintImplementableEvent)
    void Consume(APlayerCharacter* PlayerCharacter);

    void Tick(float DeltaSeconds) override;

protected:

    void BeginPlay() override;

    UFUNCTION()
    void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    bool IsBlockingExplosion_Implementation() override;

    void BlowUp_Implementation() override;

private:

    UFUNCTION()
    void TimelineProgress(float Delta);

protected:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UBoxComponent* OverlapComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(EditAnywhere, Category = "Timeline")
    UCurveFloat* CurveFloat;

    UPROPERTY(EditAnywhere, Category = "Timeline")
    float ZOffset;

private:

    FTimeline CurveTimeline;

    FVector StartLocation;

};
