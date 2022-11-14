// Fill out your copyright notice in the Description page of Project Settings.

#include "EnvQueryTest_PlaceBomb.h"

#include <EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h>
#include <EnvironmentQuery/Contexts/EnvQueryContext_Querier.h>

#include <PlayerCharacter.h>
#include <Utils.h>

UEnvQueryTest_PlaceBomb::UEnvQueryTest_PlaceBomb(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    Cost = EEnvTestCost::High;

    ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();

    SetWorkOnFloatValues(false);
}

void UEnvQueryTest_PlaceBomb::RunTest(FEnvQueryInstance& QueryInstance) const
{
    UObject* DataOwner = QueryInstance.Owner.Get();
    BoolValue.BindData(DataOwner, QueryInstance.QueryID);
    bool bWantsHit = BoolValue.GetValue();

    auto* PlayerCharacter = Cast<APlayerCharacter>(DataOwner);

    FVector ActorLocation = PlayerCharacter->GetActorLocation();
    ActorLocation = FAMUtils::RoundToUnitCenter(ActorLocation);

    uint64 ExplosionRadiusTiles = PlayerCharacter->GetExplosionRadiusTiles();
    float MinX = ActorLocation.X - ExplosionRadiusTiles * FAMUtils::Unit - FAMUtils::Unit / 2;
    float MaxX = ActorLocation.X + ExplosionRadiusTiles * FAMUtils::Unit + FAMUtils::Unit / 2;
    float MinY = ActorLocation.Y - ExplosionRadiusTiles * FAMUtils::Unit - FAMUtils::Unit / 2;
    float MaxY = ActorLocation.Y + ExplosionRadiusTiles * FAMUtils::Unit + FAMUtils::Unit / 2;

    FEnvQueryInstance::ItemIterator It(this, QueryInstance);
    It.IgnoreTimeLimit();
    for (; It; ++It)
    {
        bool bScore = true;

        FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());
        if ((ItemLocation.X > MinX && ItemLocation.X < MaxX && ItemLocation.Y == ActorLocation.Y) ||
            (ItemLocation.Y > MinY && ItemLocation.Y < MaxY && ItemLocation.X == ActorLocation.X))
        {
            bScore = false;
        }

        It.SetScore(TestPurpose, FilterType, bScore, bWantsHit);
    }
}

FText UEnvQueryTest_PlaceBomb::GetDescriptionTitle() const
{
    return FText::FromString(FString::Printf(TEXT("%s"), *Super::GetDescriptionTitle().ToString()));
}

FText UEnvQueryTest_PlaceBomb::GetDescriptionDetails() const
{
    return DescribeBoolTestParams("");
}
