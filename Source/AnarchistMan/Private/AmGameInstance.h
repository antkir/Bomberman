// Fill out your copyright notice in the Description page of Project Settings.

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
