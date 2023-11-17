// Copyright 2022 Kiryl Antonik

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Level/AmExplosiveInterface.h"
#include "Game/AmUtils.h"

#include "AmMainPlayerCharacter.generated.h"

class UCameraComponent;
class AAmBomb;
class AAmMainPlayerState;

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

	void SetInputEnabled(bool bEnabled);

	void SetInvincible(bool bInvincible);

	UFUNCTION(BlueprintCallable)
	void IncreaseMovementSpeed(float Percentage);

	UFUNCTION(BlueprintCallable)
	void IncrementExplosionRadiusTiles();

	UFUNCTION(BlueprintCallable)
	void IncrementActiveBombsLimit();

	int32 GetExplosionRadiusTiles() const;

	float GetDefaultMaxWalkSpeed() const;

protected:

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool IsBlockingExplosion_Implementation() override;

	virtual void BlowUp_Implementation() override;

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void OnRep_PlayerState() override;

	virtual void PossessedBy(AController* NewController) override;

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void PlaceBomb();

private:
	void MoveVertical(float Val);

	void MoveHorizontal(float Val);

	UFUNCTION()
	void OnBombExploded();

	bool CanPlaceBomb();

	void SetPlayerCollision(AAmMainPlayerState* AMPlayerState);

	void SetPlayerColor(AAmMainPlayerState* AMPlayerState);

	UFUNCTION()
	void OnRep_MaxWalkSpeed();

public:
	UPROPERTY(BlueprintAssignable)
	FPlayerCharacterDeath OnPlayerCharacterDeath;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCameraComponent* CameraComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AAmBomb> BombClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parameters")
	FVector CameraLocationOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parameters", meta = (ClampMin = "0"))
	int32 ExplosionRadiusTiles;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parameters", meta = (ClampMin = "0"))
	int32 ActiveBombsLimit;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bInputEnabled;

	UPROPERTY(Replicated, EditInstanceOnly, BlueprintReadOnly)
	bool bInvincible;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_MaxWalkSpeed, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float MaxWalkSpeed;

	UPROPERTY(Transient, BlueprintReadOnly)
	float DefaultMaxWalkSpeed;
};
