// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AnarchistManGameMode.generated.h"

class AAnarchistManPlayerState;
class UUserWidget;

/**
 * 
 */
UCLASS()
class ANARCHISTMAN_API AAnarchistManGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AAnarchistManGameMode();

public:

	void BeginPlay() override;

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	void PlayerDeath(AController* Controller);

private:

	void OnGameOverTimeout();

	void RoundOver();

	void GameOver(AAnarchistManPlayerState* CurrentPlayerState);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Properties")
	uint32 RoundsToWin;

	UPROPERTY(EditDefaultsOnly, Category = "Properties")
	TSubclassOf<AActor> LevelObserverCameraClass;

	UPROPERTY(EditDefaultsOnly, Category = "Properties")
	TSubclassOf<UUserWidget> GameOverWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Properties")
	TSubclassOf<UUserWidget> RoundOverWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Properties")
	float GameOverTimeout;

private:

	FTimerHandle TimerHandle;
	
};
