// Copyright 2022 Kiryl Antonik

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"

#include "AmEnvQueryTest_PlaceBomb.generated.h"

/**
 * 
 */
UCLASS()
class UAmEnvQueryTest_PlaceBomb : public UEnvQueryTest
{
	GENERATED_BODY()

public:

	UAmEnvQueryTest_PlaceBomb(const FObjectInitializer& ObjectInitializer);

public:

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

private:

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
	
};
