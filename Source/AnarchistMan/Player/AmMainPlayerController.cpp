// Copyright 2022 Kiryl Antonik

#include "AmMainPlayerController.h"

AAmMainPlayerController::AAmMainPlayerController()
{
	bIsGameMenuOpen = false;

	bAutoManageActiveCameraTarget = false;
}

void AAmMainPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("Game Menu", IE_Pressed, this, &AAmMainPlayerController::ToggleGameMenu);
}

void AAmMainPlayerController::BeginPreGame_Implementation(float Countdown)
{
	if (IsLocalController())
	{
		OnBeginPreGame(Countdown);
	}
}

void AAmMainPlayerController::BeginGame_Implementation()
{
	if (IsLocalController())
	{
		OnBeginGame();
	}
}

void AAmMainPlayerController::BeginRoundOver_Implementation(const FString& PlayerName)
{
	if (IsLocalController())
	{
		if (!PlayerName.IsEmpty())
		{
			OnBeginRoundOver(PlayerName);
		}
		else
		{
			OnBeginRoundDraw();
		}
	}
}

void AAmMainPlayerController::BeginGameOver_Implementation(const FString& PlayerName)
{
	if (IsLocalController())
	{
		OnBeginGameOver(PlayerName);
	}
}

void AAmMainPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetInputMode(FInputModeGameAndUI());
}

void AAmMainPlayerController::ToggleGameMenu()
{
	bIsGameMenuOpen = !bIsGameMenuOpen;
	OnToggleGameMenu();
}
