// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "AmUtils.h"

#include "AmUtilsFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class UAmUtilsFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Utilities")
    static int64 GetValue(ETileType TileType);
};
