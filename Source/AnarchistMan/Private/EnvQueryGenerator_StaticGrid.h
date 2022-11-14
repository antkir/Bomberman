// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataProviders/AIDataProvider.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"

#include "EnvQueryGenerator_StaticGrid.generated.h"

/**
 * 
 */
UCLASS()
class UEnvQueryGenerator_StaticGrid : public UEnvQueryGenerator_ProjectedPoints
{
    GENERATED_BODY()

public:
    UEnvQueryGenerator_StaticGrid(const FObjectInitializer& ObjectInitializer);

private:

    virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;

    virtual FText GetDescriptionTitle() const override;
    virtual FText GetDescriptionDetails() const override;

protected:

    /** half of square's extent, like a radius */
    UPROPERTY(EditDefaultsOnly, Category = Generator, meta = (DisplayName = "GridHalfSize"))
    FAIDataProviderFloatValue GridSize;

    /** generation density */
    UPROPERTY(EditDefaultsOnly, Category = Generator)
    FAIDataProviderFloatValue SpaceBetween;

    /** context */
    UPROPERTY(EditDefaultsOnly, Category = Generator)
    FVector GenerateAroundLocation;
	
};
