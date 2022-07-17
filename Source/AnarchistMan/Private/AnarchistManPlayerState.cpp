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
    DOREPLIFETIME(AAnarchistManPlayerState, PawnInputState);
    DOREPLIFETIME(AAnarchistManPlayerState, RoundWins);
    DOREPLIFETIME(AAnarchistManPlayerState, PlayerColor);
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

void AAnarchistManPlayerState::SetPawnInputState(PawnInput InputState)
{
    PawnInputState = InputState;
}

PawnInput AAnarchistManPlayerState::GetPawnInputState()
{
    return static_cast<PawnInput>(PawnInputState);
}
