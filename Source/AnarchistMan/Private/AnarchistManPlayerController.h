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

    void SetupInputComponent() override;

    UFUNCTION(NetMulticast, Reliable)
    void BeginPreGame(float Countdown);

    UFUNCTION(NetMulticast, Reliable)
    void BeginGame();

    UFUNCTION(NetMulticast, Reliable)
    void BeginRoundOver(const FString& PlayerName);

    UFUNCTION(NetMulticast, Reliable)
    void BeginGameOver(const FString& PlayerName);

protected:

    void BeginPlay() override;

    UFUNCTION(BlueprintImplementableEvent)
    void OnBeginPreGame(float Countdown);

    UFUNCTION(BlueprintImplementableEvent)
    void OnBeginGame();

    UFUNCTION(BlueprintImplementableEvent)
    void OnBeginRoundOver(const FString& PlayerName);

    UFUNCTION(BlueprintImplementableEvent)
    void OnBeginRoundDraw();

    UFUNCTION(BlueprintImplementableEvent)
    void OnBeginGameOver(const FString& PlayerName);

    UFUNCTION(BlueprintImplementableEvent)
    void OnToggleGameMenu();

private:

    void ToggleGameMenu();

protected:

    UPROPERTY(BlueprintReadOnly)
    bool bIsGameMenuOpen;

};
