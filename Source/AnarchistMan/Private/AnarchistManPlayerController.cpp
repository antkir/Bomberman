// Fill out your copyright notice in the Description page of Project Settings.

#include "AnarchistManPlayerController.h"
#include <Blueprint/UserWidget.h>

AAnarchistManPlayerController::AAnarchistManPlayerController()
{
    bAutoManageActiveCameraTarget = false;
}

void AAnarchistManPlayerController::ServerSetViewTarget_Implementation(AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams)
{
    SetViewTarget(NewViewTarget, TransitionParams);
}

void AAnarchistManPlayerController::RoundOver_Implementation(TSubclassOf<UUserWidget> RoundOverWidgetClass)
{
    UUserWidget* RoundOverWidget = CreateWidget<UUserWidget>(this, RoundOverWidgetClass);
    if (RoundOverWidget)
    {
        RoundOverWidget->AddToViewport();
    }
}

void AAnarchistManPlayerController::GameOver_Implementation(TSubclassOf<UUserWidget> GameOverWidgetClass, const FString& PlayerName)
{
    UUserWidget* GameOverWidget = CreateWidget<UUserWidget>(this, GameOverWidgetClass);
    if (GameOverWidget)
    {
        GameOverWidget->AddToViewport();
    }
}
