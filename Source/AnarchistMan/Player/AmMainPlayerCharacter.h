// Copyright 2022 Kiryl Antonik

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Level/AmExplosiveInterface.h"
#include "Game/AmUtils.h"

#include "AmMainPlayerCharacter.generated.h"

class UCameraComponent;
class AAmBomb;

/**
 * @brief Delegate executed when a player dies from a bomb explosion.
 * @param PlayerController.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerCharacterDeath, AController*, Controller);

UCLASS()
class AAmMainPlayerCharacter : public ACharacter, public IAmExplosiveInterface
{
	GENERATED_BODY()

public:

	// Sets default values for this character's properties
	AAmMainPlayerCharacter();

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

	int32 GetExplosionRadiusTiles() const;

	float GetDefaultMaxWalkSpeed() const;

protected:

	void BeginPlay() override;

	void Tick(float DeltaTime) override;

	void OnRep_PlayerState() override;

	void PossessedBy(AController* NewController) override;

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void PlaceBomb();

private:

	void MoveVertical(float Val);

	void MoveHorizontal(float Val);

	UFUNCTION()
	void OnBombExploded();

	bool CanPlaceBomb();

public:

	UPROPERTY(BlueprintAssignable)
	FPlayerCharacterDeath OnPlayerCharacterDeath;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, Category = "Classes")
	TSubclassOf<AAmBomb> BombClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters")
	FVector CameraLocationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta = (ClampMin = "0"))
	int32 ExplosionRadiusTiles;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta = (ClampMin = "0"))
	int32 ActiveBombsLimit;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bInputEnabled;

	UPROPERTY(Replicated, EditInstanceOnly, BlueprintReadOnly)
	bool bInvincible;

	UPROPERTY(Transient, Replicated, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float MaxWalkSpeed;

	UPROPERTY(BlueprintReadOnly)
	float DefaultMaxWalkSpeed;
};
