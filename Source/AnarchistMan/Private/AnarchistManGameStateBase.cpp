// Fill out your copyright notice in the Description page of Project Settings.

#include "AnarchistManGameStateBase.h"
#include "AnarchistManGameStateBase.h"

AAnarchistManGameStateBase::AAnarchistManGameStateBase()
{
    PlayersAlive = 0;
}

void AAnarchistManGameStateBase::AddPlayerState(APlayerState* PlayerState)
{
    Super::AddPlayerState(PlayerState);
    PlayersAlive++;
}

void AAnarchistManGameStateBase::RemovePlayerState(APlayerState* PlayerState)
{
    Super::RemovePlayerState(PlayerState);
    PlayersAlive--;
}

void AAnarchistManGameStateBase::PlayerDeath()
{
    if (PlayersAlive > 0)
    {
        PlayersAlive--;
    }
}

uint64 AAnarchistManGameStateBase::GetPlayersAlive()
{
    return PlayersAlive;
}
