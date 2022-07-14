// Fill out your copyright notice in the Description page of Project Settings.

#include "AnarchistManPlayerState.h"

void AAnarchistManPlayerState::RoundWin()
{
    RoundWins++;
}

uint64 AAnarchistManPlayerState::GetRoundWins()
{
    return RoundWins;
}
