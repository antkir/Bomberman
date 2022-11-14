// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "AMPlayerStart.generated.h"

/**
 * 
 */
UCLASS()
class AAMPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, Category = "Properties")
    bool bHasAssignedCharacter;

    UPROPERTY(BlueprintReadWrite, Category = "Properties")
    bool bIsUsedByAI;
	
};
