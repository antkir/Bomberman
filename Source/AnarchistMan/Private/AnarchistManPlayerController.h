// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AnarchistManPlayerController.generated.h"

class UUserWidget;

/**
 * 
 */
UCLASS()
class AAnarchistManPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	
	UFUNCTION(Client, Reliable)
	void GameOver(TSubclassOf<UUserWidget> GameOverWidgetClass);

};
