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
	AAnarchistManPlayerController();

public:

	UFUNCTION(Server, Reliable)
	void ServerSetViewTarget(AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams);

	UFUNCTION(Client, Reliable)
	void RoundOver(TSubclassOf<UUserWidget> RoundOverWidgetClass);
	
	UFUNCTION(Client, Reliable)
	void GameOver(TSubclassOf<UUserWidget> GameOverWidgetClass, const FString& PlayerName);

};
