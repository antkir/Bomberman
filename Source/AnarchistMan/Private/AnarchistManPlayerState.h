// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "AnarchistManPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class AAnarchistManPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

    AAnarchistManPlayerState();

public:

    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void SetPlayerDead();

    void SetPlayerAlive();

    bool IsDead();

    void WinRound();

    uint8 GetRoundWins();

    void ResetRoundWins();

    void SetPlayerColor(FColor Color);

    FColor GetPlayerColor();

    void SetActiveBombsCount(uint32 Count);

    uint32 GetActiveBombsCount();

    void SetActiveBombsLimit(uint32 Limit);

    bool CanPlaceBomb();

protected:

    UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsDead;

    UPROPERTY(Replicated, BlueprintReadOnly)
    uint8 RoundWins;

    UPROPERTY(Replicated, BlueprintReadOnly)
    FColor PlayerColor;

    UPROPERTY(Replicated, BlueprintReadOnly)
    int64 ActiveBombsCount;

    UPROPERTY(Replicated, BlueprintReadOnly)
    int64 ActiveBombsLimit;
	
};
