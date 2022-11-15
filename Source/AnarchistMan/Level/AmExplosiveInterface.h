#pragma once

#include "AmExplosiveInterface.generated.h"

UINTERFACE(Blueprintable)
class UAmExplosiveInterface : public UInterface
{
    GENERATED_BODY()
};

class IAmExplosiveInterface
{    
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    bool IsBlockingExplosion();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void BlowUp();
};