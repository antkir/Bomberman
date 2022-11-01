// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "AnarchistManGameMode.generated.h"

class AAnarchistManPlayerController;
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

protected:

	void BeginPlay() override;

	void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	void PostLogin(APlayerController* NewPlayer) override;

    void Destroyed() override;

	AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	void PlayerDeath(AController* Controller);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
    void RestartGame();

    APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

    UFUNCTION()
    void OnPlayerCharacterDeath(APlayerController* PlayerController);

private:

    void BeginPreGame();

    void PrepareGame();

    void BeginGame();

	void BeginRoundOver(FString PlayerName);

	void BeginGameOver(FString PlayerName);

    inline FViewTargetTransitionParams CreateViewTargetTransitionParams(float BlendTime);

protected:

    UPROPERTY()
    FName CurrentMatchState;

	UPROPERTY(EditDefaultsOnly, Category = "Properties")
	TSubclassOf<AActor> LevelObserverCameraClass;

    UPROPERTY(EditDefaultsOnly, Category = "Properties")
    float RoundCountdownTime;

    UPROPERTY(EditDefaultsOnly, Category = "Properties")
    float CameraBlendTime;

    UPROPERTY(EditDefaultsOnly, Category = "Properties")
    float DrawTimeThreshold;

    UPROPERTY(EditDefaultsOnly, Category = "Properties")
    bool bResetLevelOnBeginPreGame;

private:

    uint32 RecentDeaths;
	
};
