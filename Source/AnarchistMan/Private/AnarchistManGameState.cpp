// Fill out your copyright notice in the Description page of Project Settings.

#include "AnarchistManGameState.h"

#include <Net/UnrealNetwork.h>

AAnarchistManGameState::AAnarchistManGameState()
{
    RoundsToWin = 3;
}

void AAnarchistManGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAnarchistManGameState, RoundsToWin);
}

void AAnarchistManGameState::AddPlayerState(APlayerState* PlayerState)
{
    Super::AddPlayerState(PlayerState);
    PlayersAlive++;
}

void AAnarchistManGameState::RemovePlayerState(APlayerState* PlayerState)
{
    Super::RemovePlayerState(PlayerState);
    PlayersAlive--;
}

void AAnarchistManGameState::PlayerDeath()
{
    if (PlayersAlive > 0)
    {
        PlayersAlive--;
    }
}

uint64 AAnarchistManGameState::GetPlayersAlive()
{
    return PlayersAlive;
}

void AAnarchistManGameState::SetPlayersAlive(uint64 Num)
{
    PlayersAlive = Num;
}

uint8 AAnarchistManGameState::GetRoundsToWin()
{
    return RoundsToWin;
}
