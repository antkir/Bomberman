// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "AmMainGameMode.generated.h"

class AAIController;

/**
 * 
 */
UCLASS()
class AAmMainGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AAmMainGameMode();

protected:

	void BeginPlay() override;

	void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	void PostLogin(APlayerController* NewPlayer) override;

    void Destroyed() override;

	void PlayerDeath(AController* Controller);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
    void RestartGame();

    APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

    UFUNCTION()
    void OnPlayerCharacterDeath(AController* PlayerController);

private:

    void BeginPreGame();

    void PrepareGame();

    void BeginGame();

	void BeginRoundOver(FString PlayerName);

	void BeginGameOver(FString PlayerName);

    void SpawnAIControllers();

    void SetControllerName(AController* Controller);

    void SetControllerColor(AController* Controller);

    AActor* GetNextViewTarget() const;

    virtual bool ShouldSpawnAtStartSpot(AController* Player) override;

    ACameraActor* GetLevelOverviewCamera() const;

    FORCEINLINE FViewTargetTransitionParams CreateViewTargetTransitionParams(float BlendTime) const
    {
        FViewTargetTransitionParams TransitionParams;
        TransitionParams.BlendTime = BlendTime;
        TransitionParams.BlendFunction = EViewTargetBlendFunction::VTBlend_Cubic;
        TransitionParams.BlendExp = 0;
        TransitionParams.bLockOutgoing = true;
        return TransitionParams;
    }

protected:

    UPROPERTY(BlueprintReadOnly)
    FName CurrentMatchState;

    UPROPERTY(EditDefaultsOnly, Category = "Properties")
    float RoundCountdownTime;

    UPROPERTY(EditDefaultsOnly, Category = "Properties")
    float CameraBlendTime;

    UPROPERTY(EditDefaultsOnly, Category = "Properties")
    float RoundDrawTimeThreshold;

    UPROPERTY(EditDefaultsOnly, Category = "Properties")
    bool bResetLevelOnBeginPreGame;

    UPROPERTY(EditDefaultsOnly, Category = "Classes")
    TSubclassOf<AAIController> AIControllerClass;

    UPROPERTY(BlueprintReadOnly)
    int32 RecentDeaths;

    FTimerHandle BeginPreGameTimerHandle;
	
};
