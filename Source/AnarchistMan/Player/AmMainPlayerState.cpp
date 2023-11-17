// Copyright 2022 Kiryl Antonik

#include "AmMainPlayerState.h"

#include <Net/UnrealNetwork.h>

AAmMainPlayerState::AAmMainPlayerState()
{
	bIsDead = false;

	RoundWins = 0;

	ActiveBombsCount = 0;

	PlayerColor = FColor(255, 255, 255);
}

void AAmMainPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAmMainPlayerState, bIsDead);
	DOREPLIFETIME(AAmMainPlayerState, RoundWins);
	DOREPLIFETIME(AAmMainPlayerState, PlayerColor);
	DOREPLIFETIME(AAmMainPlayerState, ActiveBombsCount);
}

void AAmMainPlayerState::SetPlayerDead()
{
	bIsDead = true;
}

void AAmMainPlayerState::SetPlayerAlive()
{
	bIsDead = false;
}

bool AAmMainPlayerState::IsDead() const
{
	return bIsDead;
}

void AAmMainPlayerState::WinRound()
{
	RoundWins++;
}

uint8 AAmMainPlayerState::GetRoundWins() const
{
	return RoundWins;
}

void AAmMainPlayerState::ResetRoundWins()
{
	RoundWins = 0;
}

void AAmMainPlayerState::SetPlayerColor(FColor Color)
{
	PlayerColor = Color;
}

FColor AAmMainPlayerState::GetPlayerColor() const
{
	return PlayerColor;
}

void AAmMainPlayerState::SetActiveBombsCount(int32 Count)
{
	check(Count >= 0);

	ActiveBombsCount = Count;
}

int32 AAmMainPlayerState::GetActiveBombsCount() const
{
	return ActiveBombsCount;
}
