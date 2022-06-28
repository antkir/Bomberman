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
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void BlowUp();

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

private:

	/** Handles moving up/down */
	void MoveVertical(float Val);

	/** Handles strafing movement, left and right */
	void MoveHorizontal(float Val);

	/** Bomb placing */
	UFUNCTION(Server, Reliable)
	void PlaceBomb();

	void BlowUp_Private();

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector CameraLocationOffset;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	TSubclassOf<ABomb> BombClass;

};
