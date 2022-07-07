// Fill out your copyright notice in the Description page of Project Settings.

#include "AnarchistManPlayerController.h"
#include <Blueprint/UserWidget.h>

void AAnarchistManPlayerController::GameOver_Implementation(TSubclassOf<UUserWidget> GameOverWidgetClass)
{
    UUserWidget* GameOverWidget = CreateWidget<UUserWidget>(this, GameOverWidgetClass);
    if (GameOverWidget)
    {
        GameOverWidget->AddToViewport();
    }
}
