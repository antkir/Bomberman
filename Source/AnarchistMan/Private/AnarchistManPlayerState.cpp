// Fill out your copyright notice in the Description page of Project Settings.

#include "AnarchistManPlayerState.h"

#include <Net/UnrealNetwork.h>

AAnarchistManPlayerState::AAnarchistManPlayerState()
{
    bIsDead = false;

    RoundWins = 0;

    ActiveBombsCount = 0;

    PlayerColor = FColor(255, 255, 255);
}

void AAnarchistManPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAnarchistManPlayerState, bIsDead);
    DOREPLIFETIME(AAnarchistManPlayerState, RoundWins);
    DOREPLIFETIME(AAnarchistManPlayerState, PlayerColor);
    DOREPLIFETIME(AAnarchistManPlayerState, ActiveBombsCount);
}

void AAnarchistManPlayerState::SetPlayerDead()
{
    bIsDead = true;
}

void AAnarchistManPlayerState::SetPlayerAlive()
{
    bIsDead = false;
}

bool AAnarchistManPlayerState::IsDead() const
{
    return bIsDead;
}

void AAnarchistManPlayerState::WinRound()
{
    RoundWins++;
}

uint8 AAnarchistManPlayerState::GetRoundWins() const
{
    return RoundWins;
}

void AAnarchistManPlayerState::ResetRoundWins()
{
    RoundWins = 0;
}

void AAnarchistManPlayerState::SetPlayerColor(FColor Color)
{
    PlayerColor = Color;
}

FColor AAnarchistManPlayerState::GetPlayerColor() const
{
    return PlayerColor;
}

void AAnarchistManPlayerState::SetActiveBombsCount(int32 Count)
{
    check(Count >= 0);

    ActiveBombsCount = Count;
}

int32 AAnarchistManPlayerState::GetActiveBombsCount() const
{
    return ActiveBombsCount;
}
