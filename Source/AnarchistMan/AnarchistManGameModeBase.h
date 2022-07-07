// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AnarchistManGameModeBase.generated.h"

class UUserWidget;

/**
 * 
 */
UCLASS()
class ANARCHISTMAN_API AAnarchistManGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	AAnarchistManGameModeBase();

public:

	void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	void PlayerDeath(AController* Controller);

private:

	void OnGameOverTimeout();

protected:

	UPROPERTY(EditDefaultsOnly, Category = "GameOver")
	TSubclassOf<AActor> GameOverCameraClass;

	UPROPERTY(EditDefaultsOnly, Category = "GameOver")
	TSubclassOf<UUserWidget> GameOverWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "GameOver")
	float GameOverTimeout;

private:

	FTimerHandle TimerHandle_GameOverTimeout;
	
};
