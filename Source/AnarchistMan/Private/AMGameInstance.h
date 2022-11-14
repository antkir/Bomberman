// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "AMGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class UAMGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

    UAMGameInstance();

public:

    UPROPERTY(BlueprintReadWrite, Category = "Properties")
    int32 ConnectedPlayersNum;
};
