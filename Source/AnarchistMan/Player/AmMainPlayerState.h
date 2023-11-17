// Copyright 2022 Kiryl Antonik

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "AmMainPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class AAmMainPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	AAmMainPlayerState();

public:

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetPlayerDead();

	void SetPlayerAlive();

	bool IsDead() const;

	void WinRound();

	uint8 GetRoundWins() const;

	void ResetRoundWins();

	void SetPlayerColor(FColor Color);

	FColor GetPlayerColor() const;

	void SetActiveBombsCount(int32 Count);

	int32 GetActiveBombsCount() const;

protected:

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsDead;

	UPROPERTY(Replicated, BlueprintReadOnly)
	uint8 RoundWins;

	UPROPERTY(Replicated, BlueprintReadOnly)
	FColor PlayerColor;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 ActiveBombsCount;
	
};
