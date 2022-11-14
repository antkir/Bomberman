// Fill out your copyright notice in the Description page of Project Settings.

#include "AmMainGameState.h"

#include <Net/UnrealNetwork.h>

AAmMainGameState::AAmMainGameState()
{
    PlayersAlive = 0;

    RoundsToWin = 3;
}

void AAmMainGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAmMainGameState, RoundsToWin);
}

void AAmMainGameState::AddPlayerState(APlayerState* PlayerState)
{
    Super::AddPlayerState(PlayerState);

    if (HasAuthority())
    {
        PlayersAlive++;
    }
}

void AAmMainGameState::RemovePlayerState(APlayerState* PlayerState)
{
    Super::RemovePlayerState(PlayerState);

    if (HasAuthority())
    {
        PlayersAlive--;
    }
}

void AAmMainGameState::PlayerDeath()
{
    check(HasAuthority());

    if (PlayersAlive > 0)
    {
        PlayersAlive--;
    }
}

uint8 AAmMainGameState::GetPlayersAlive() const
{
    return PlayersAlive;
}

void AAmMainGameState::SetPlayersAlive(uint8 Num)
{
    check(HasAuthority());

    PlayersAlive = Num;
}

uint8 AAmMainGameState::GetRoundsToWin() const
{
    return RoundsToWin;
}
