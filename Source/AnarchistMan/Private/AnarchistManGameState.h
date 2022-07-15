// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AnarchistManGameState.generated.h"

/**
 * 
 */
UCLASS()
class AAnarchistManGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	AAnarchistManGameState();

public:

	void AddPlayerState(APlayerState* PlayerState) override;

	void RemovePlayerState(APlayerState* PlayerState) override;

	void PlayerDeath();

	uint64 GetPlayersAlive();

	void SetPlayersAlive(uint64 Num);

	void RoundPlayed();

	uint64 GetRoundsPlayed();

private:

	uint64 PlayersAlive;

	uint64 RoundsPlayed;
	
};
