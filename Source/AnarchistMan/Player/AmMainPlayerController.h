// Copyright 2022 Kiryl Antonik

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "AmMainPlayerController.generated.h"

class UUserWidget;

/**
 * 
 */
UCLASS()
class AAmMainPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAmMainPlayerController();

public:

	UFUNCTION(NetMulticast, Reliable)
	virtual void BeginPreGame(float Countdown);

	UFUNCTION(NetMulticast, Reliable)
	void BeginGame();

	UFUNCTION(NetMulticast, Reliable)
	void BeginRoundOver(const FString& PlayerName);

	UFUNCTION(NetMulticast, Reliable)
	void BeginGameOver(const FString& PlayerName);

protected:

	virtual void SetupInputComponent() override;

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
