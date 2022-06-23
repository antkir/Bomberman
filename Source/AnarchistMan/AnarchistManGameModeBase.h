// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AnarchistManGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class ANARCHISTMAN_API AAnarchistManGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	void PostLogin(APlayerController* NewPlayer) override;

	void PlayerDeath(AController* Controller);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Spectating")
	TSubclassOf<AActor> GameOverCamera;
	
};
