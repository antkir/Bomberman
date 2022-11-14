// Fill out your copyright notice in the Description page of Project Settings.

#include "AnarchistManGameState.h"

#include <Net/UnrealNetwork.h>

AAnarchistManGameState::AAnarchistManGameState()
{
    PlayersAlive = 0;

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

    if (HasAuthority())
    {
        PlayersAlive++;
    }
}

void AAnarchistManGameState::RemovePlayerState(APlayerState* PlayerState)
{
    Super::RemovePlayerState(PlayerState);

    if (HasAuthority())
    {
        PlayersAlive--;
    }
}

void AAnarchistManGameState::PlayerDeath()
{
    check(HasAuthority());

    if (PlayersAlive > 0)
    {
        PlayersAlive--;
    }
}

uint8 AAnarchistManGameState::GetPlayersAlive() const
{
    return PlayersAlive;
}

void AAnarchistManGameState::SetPlayersAlive(uint8 Num)
{
    check(HasAuthority());

    PlayersAlive = Num;
}

uint8 AAnarchistManGameState::GetRoundsToWin() const
{
    return RoundsToWin;
}
