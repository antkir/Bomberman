// Copyright 2022 Kiryl Antonik

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

	void PlayerDeath();

	uint8 GetPlayersAlive() const;

	void SetPlayersAlive(uint8 Num);

	uint8 GetRoundsToWin() const;

protected:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void AddPlayerState(APlayerState* PlayerState) override;

	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	UPROPERTY(Replicated, BlueprintReadOnly, EditDefaultsOnly, Category = "Properties")
	uint8 RoundsToWin;

	UPROPERTY(BlueprintReadOnly)
	uint8 PlayersAlive;
	
};
