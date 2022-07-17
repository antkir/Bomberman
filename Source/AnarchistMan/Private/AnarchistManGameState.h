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

    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void AddPlayerState(APlayerState* PlayerState) override;

	void RemovePlayerState(APlayerState* PlayerState) override;

	void PlayerDeath();

	uint64 GetPlayersAlive();

	void SetPlayersAlive(uint64 Num);

    void SetRoundsToWin(uint8 Num);

protected:

    UPROPERTY(Replicated, BlueprintReadOnly)
    uint8 RoundsToWin;

private:

	uint64 PlayersAlive;
	
};
