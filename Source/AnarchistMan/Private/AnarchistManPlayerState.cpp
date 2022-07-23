// Fill out your copyright notice in the Description page of Project Settings.

#include "AnarchistManPlayerState.h"

#include <Net/UnrealNetwork.h>

AAnarchistManPlayerState::AAnarchistManPlayerState()
{
    PlayerColor = FColor(255, 255, 255);
}

void AAnarchistManPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAnarchistManPlayerState, bIsDead);
    DOREPLIFETIME(AAnarchistManPlayerState, RoundWins);
    DOREPLIFETIME(AAnarchistManPlayerState, PlayerColor);
    DOREPLIFETIME(AAnarchistManPlayerState, ActiveBombsCount);
    DOREPLIFETIME(AAnarchistManPlayerState, ActiveBombsLimit);
}

void AAnarchistManPlayerState::SetPlayerDead()
{
    bIsDead = true;
}

void AAnarchistManPlayerState::SetPlayerAlive()
{
    bIsDead = false;
}

bool AAnarchistManPlayerState::IsDead()
{
    return bIsDead;
}

void AAnarchistManPlayerState::WinRound()
{
    RoundWins++;
}

uint8 AAnarchistManPlayerState::GetRoundWins()
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

FColor AAnarchistManPlayerState::GetPlayerColor()
{
    return PlayerColor;
}

void AAnarchistManPlayerState::SetActiveBombsCount(uint32 Count)
{
    ActiveBombsCount = Count;
}

uint32 AAnarchistManPlayerState::GetActiveBombsCount()
{
    return ActiveBombsCount;
}

void AAnarchistManPlayerState::SetActiveBombsLimit(uint32 Limit)
{
    ActiveBombsLimit = Limit;
}

bool AAnarchistManPlayerState::CanPlaceBomb()
{
    return ActiveBombsCount < ActiveBombsLimit;
}
