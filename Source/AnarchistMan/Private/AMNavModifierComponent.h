// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavModifierComponent.h"
#include "AMNavModifierComponent.generated.h"

/**
 * 
 */
UCLASS()
class UAMNavModifierComponent : public UNavModifierComponent
{
	GENERATED_BODY()

public:

    virtual void CalcAndCacheBounds() const override;

    virtual void GetNavigationData(FNavigationRelevantData& Data) const override;

    UFUNCTION(BlueprintCallable, Category = "AI|Navigation")
    void SetRadiusBlocks(uint8 Radius);

    UFUNCTION(BlueprintCallable, Category = "AI|Navigation")
    void SetLeafAreaClass(TSubclassOf<UNavArea> NewLeafAreaClass);

protected:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Navigation)
    TSubclassOf<UNavArea> LeafAreaClass;

private:

    uint8 RadiusBlocks;

    mutable TArray<FRotatedBox> LeafBounds;
	
};
