// Fill out your copyright notice in the Description page of Project Settings.

#include "AnarchistManPlayerController.h"

#include <AnarchistManGameMode.h>
#include <PlayerCharacter.h>

#include <Blueprint/UserWidget.h>
#include <Net/UnrealNetwork.h>

AAnarchistManPlayerController::AAnarchistManPlayerController()
{
    bAutoManageActiveCameraTarget = false;
}

void AAnarchistManPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAction("Game Menu", IE_Pressed, this, &AAnarchistManPlayerController::ToggleGameMenu);
}

void AAnarchistManPlayerController::BeginPreGame_Implementation(float Countdown)
{
    if (IsLocalController())
    {
        OnBeginPreGame(Countdown);
    }
}

void AAnarchistManPlayerController::BeginGame_Implementation()
{
    if (IsLocalController())
    {
        OnBeginGame();
    }
}

void AAnarchistManPlayerController::BeginRoundOver_Implementation(const FString& PlayerName)
{
    if (IsLocalController())
    {
        OnBeginRoundOver(PlayerName);
    }
}

void AAnarchistManPlayerController::BeginGameOver_Implementation(const FString& PlayerName)
{
    if (IsLocalController())
    {
        OnBeginGameOver(PlayerName);
    }
}

void AAnarchistManPlayerController::BeginPlay()
{
    Super::BeginPlay();

    SetInputMode(FInputModeGameAndUI());
}

void AAnarchistManPlayerController::ToggleGameMenu()
{
    bIsGameMenuOpen = !bIsGameMenuOpen;
    OnToggleGameMenu();
}
