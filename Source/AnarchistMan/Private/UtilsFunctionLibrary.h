// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Utils.h"

#include "UtilsFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class UUtilsFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Utilities")
    static int64 GetValue(ETileType TileType);
};
