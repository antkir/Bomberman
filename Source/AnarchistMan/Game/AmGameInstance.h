// Copyright 2022 Kiryl Antonik

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "AmGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class UAmGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UAmGameInstance();

public:

	UPROPERTY(BlueprintReadWrite, Category = "Properties")
	int32 ConnectedPlayersNum;
};
