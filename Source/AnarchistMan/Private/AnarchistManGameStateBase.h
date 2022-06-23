// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AnarchistManGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class AAnarchistManGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:

	AAnarchistManGameStateBase();

public:

	void AddPlayerState(APlayerState* PlayerState) override;

	void RemovePlayerState(APlayerState* PlayerState) override;

	void PlayerDeath();

	uint64 GetPlayersAlive();

private:

	uint64 PlayersAlive;
	
};
