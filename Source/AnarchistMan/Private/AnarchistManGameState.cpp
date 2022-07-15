// Fill out your copyright notice in the Description page of Project Settings.

#include "AnarchistManGameState.h"

AAnarchistManGameState::AAnarchistManGameState()
{
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

void AAnarchistManGameState::RoundPlayed()
{
    RoundsPlayed++;
}

uint64 AAnarchistManGameState::GetRoundsPlayed()
{
    return RoundsPlayed;
}
