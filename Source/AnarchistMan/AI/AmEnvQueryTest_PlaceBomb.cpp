// Fill out your copyright notice in the Description page of Project Settings.

#include "AmEnvQueryTest_PlaceBomb.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "Game/AmUtils.h"
#include "Player/AmMainPlayerCharacter.h"

UAmEnvQueryTest_PlaceBomb::UAmEnvQueryTest_PlaceBomb(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    Cost = EEnvTestCost::High;

    ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();

    SetWorkOnFloatValues(false);
}

void UAmEnvQueryTest_PlaceBomb::RunTest(FEnvQueryInstance& QueryInstance) const
{
    UObject* DataOwner = QueryInstance.Owner.Get();
    BoolValue.BindData(DataOwner, QueryInstance.QueryID);
    bool bWantsHit = BoolValue.GetValue();

    auto* PlayerCharacter = Cast<AAmMainPlayerCharacter>(DataOwner);

    FVector ActorLocation = PlayerCharacter->GetActorLocation();
    ActorLocation = FAmUtils::RoundToUnitCenter(ActorLocation);

    uint64 ExplosionRadiusTiles = PlayerCharacter->GetExplosionRadiusTiles();
    float MinX = ActorLocation.X - ExplosionRadiusTiles * FAmUtils::Unit - FAmUtils::Unit / 2;
    float MaxX = ActorLocation.X + ExplosionRadiusTiles * FAmUtils::Unit + FAmUtils::Unit / 2;
    float MinY = ActorLocation.Y - ExplosionRadiusTiles * FAmUtils::Unit - FAmUtils::Unit / 2;
    float MaxY = ActorLocation.Y + ExplosionRadiusTiles * FAmUtils::Unit + FAmUtils::Unit / 2;

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

FText UAmEnvQueryTest_PlaceBomb::GetDescriptionTitle() const
{
    return FText::FromString(FString::Printf(TEXT("%s"), *Super::GetDescriptionTitle().ToString()));
}

FText UAmEnvQueryTest_PlaceBomb::GetDescriptionDetails() const
{
    return DescribeBoolTestParams("");
}
