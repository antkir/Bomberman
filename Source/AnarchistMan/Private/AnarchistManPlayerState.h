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

	void RoundWin();

	uint64 GetRoundWins();

public:

	bool bIsDead;

private:

	uint64 RoundWins;
	
};
