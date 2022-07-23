#pragma once

#include "ExplosiveInterface.generated.h"

UINTERFACE(Blueprintable)
class UExplosiveInterface : public UInterface
{
    GENERATED_BODY()
};

class IExplosiveInterface
{    
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    bool HasOwnExplosionVisualEffect();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void BlowUp();
};