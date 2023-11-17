// Copyright 2022 Kiryl Antonik

#pragma once

#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "AmExplosiveInterface.h"

#include "AmPowerUp.generated.h"

class AAmMainPlayerCharacter;
class UBoxComponent;

UCLASS()
class AAmPowerUp : public AActor, public IAmExplosiveInterface
{
	GENERATED_BODY()

public:
	AAmPowerUp();

public:

	UFUNCTION(BlueprintImplementableEvent)
	void Consume(AAmMainPlayerCharacter* PlayerCharacter);

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
	float ZDistance;

private:

	FTimeline CurveTimeline;

	FVector StartLocation;

};
