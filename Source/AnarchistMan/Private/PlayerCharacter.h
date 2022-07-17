// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "PlayerCharacter.generated.h"

class UCameraComponent;
class ABomb;

UCLASS()
class APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	// Sets default values for this character's properties
	APlayerCharacter();

public:

	// Called to bind functionality to input
	void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void BlowUp();

protected:

	void BeginPlay() override;

	void Tick(float DeltaTime) override;

	void OnRep_PlayerState() override;

	void PossessedBy(AController* NewController) override;

private:

	/** Handles moving up/down */
	void MoveVertical(float Val);

	/** Handles strafing movement, left and right */
	void MoveHorizontal(float Val);

	void ToggleGameMenu();

    UFUNCTION(Server, Reliable)
    void PlaceBomb();

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector CameraLocationOffset;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	TSubclassOf<ABomb> BombClass;

};
