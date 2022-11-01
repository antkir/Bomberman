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

    bool IsBlockingExplosion_Implementation() override;

    void BlowUp_Implementation() override;

    void SetInputEnabled(bool InputEnabled);

    void SetInvincible(bool Invincible);

    UFUNCTION(BlueprintCallable)
    void IncreaseMovementSpeed(float Percentage);

    UFUNCTION(BlueprintCallable)
    void IncrementExplosionRadiusTiles();

    UFUNCTION(BlueprintCallable)
    void IncrementActiveBombsLimit();

protected:

	void BeginPlay() override;

	void Tick(float DeltaTime) override;

	void OnRep_PlayerState() override;

	void PossessedBy(AController* NewController) override;

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void PlaceBomb();

private:

	/** Handles moving up/down */
	void MoveVertical(float Val);

	/** Handles strafing movement, left and right */
	void MoveHorizontal(float Val);

    UFUNCTION()
    void OnBombExploded();

    bool CanPlaceBomb();

public:

    FPlayerCharacterDeath OnPlayerCharacterDeath;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	FVector CameraLocationOffset;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	TSubclassOf<ABomb> BombClass;

    UPROPERTY(EditAnywhere, Category = "Parameters")
    uint64 ExplosionRadiusTiles;

    UPROPERTY(EditAnywhere, Category = "Parameters")
    uint64 ActiveBombsLimit;

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bInputEnabled;

    UPROPERTY(Replicated, EditInstanceOnly, BlueprintReadOnly)
    bool bInvincible;

};
