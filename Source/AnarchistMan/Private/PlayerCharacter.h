// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ExplosiveInterface.h"
#include "Utils.h"

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "PlayerCharacter.generated.h"

class UCameraComponent;
class ABomb;

/**
 * @brief Delegate executed when a player dies from a bomb explosion.
 * @param PlayerController.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerCharacterDeath, APlayerController*, PlayerController);

UCLASS()
class APlayerCharacter : public ACharacter, public IExplosiveInterface
{
	GENERATED_BODY()

public:

	// Sets default values for this character's properties
	APlayerCharacter();

public:

	// Called to bind functionality to input
	void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    bool HasOwnExplosionVisualEffect_Implementation() override;

    void BlowUp_Implementation() override;

    void SetPawnInputState(EPawnInput PawnInput);

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

    UFUNCTION()
    void OnBombExploded();

    bool CanPlaceBomb();

public:

    FPlayerCharacterDeath OnPlayerCharacterDeath;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector CameraLocationOffset;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	TSubclassOf<ABomb> BombClass;

    UPROPERTY(Replicated, BlueprintReadOnly)
    EPawnInput InputState;

};
