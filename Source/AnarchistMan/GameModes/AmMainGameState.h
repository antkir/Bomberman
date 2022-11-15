// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "AmMainGameState.generated.h"

/**
 * 
 */
UCLASS()
class AAmMainGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

    AAmMainGameState();

public:

    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void AddPlayerState(APlayerState* PlayerState) override;

	void RemovePlayerState(APlayerState* PlayerState) override;

	void PlayerDeath();

    uint8 GetPlayersAlive() const;

    void SetPlayersAlive(uint8 Num);

    uint8 GetRoundsToWin() const;

protected:

    UPROPERTY(Replicated, BlueprintReadOnly, EditDefaultsOnly, Category = "Properties")
    uint8 RoundsToWin;

    UPROPERTY(BlueprintReadOnly)
    uint8 PlayersAlive;
	
};
