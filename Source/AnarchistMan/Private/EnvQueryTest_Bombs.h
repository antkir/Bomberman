// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"

#include "EnvQueryTest_Bombs.generated.h"

/**
 * 
 */
UCLASS()
class UEnvQueryTest_Bombs : public UEnvQueryTest
{
	GENERATED_BODY()

public:

    UEnvQueryTest_Bombs(const FObjectInitializer& ObjectInitializer);
	
public:

    virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

private:

    virtual FText GetDescriptionTitle() const override;
    virtual FText GetDescriptionDetails() const override;

protected:

    UPROPERTY(EditDefaultsOnly, Category = "Parameters")
    FAIDataProviderBoolValue AddMovementStartDelay;
};
