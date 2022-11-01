// Fill out your copyright notice in the Description page of Project Settings.


#include "EnvQueryTest_PlaceBomb.h"

#include "Bomb.h"
#include "BreakableBlock.h"
#include "Utils.h"

#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

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
    FVector ActorLocation = Cast<AActor>(DataOwner)->GetActorLocation();
    ActorLocation = Utils::RoundToUnitCenter(ActorLocation);

    uint64 ExplosionRadiusUnits = 2;

    float MinX = ActorLocation.X - Utils::Unit / 2 - ExplosionRadiusUnits * Utils::Unit;
    float MinY = ActorLocation.Y - Utils::Unit / 2 - ExplosionRadiusUnits * Utils::Unit;;
    float MaxX = ActorLocation.X + Utils::Unit / 2 + ExplosionRadiusUnits * Utils::Unit;;
    float MaxY = ActorLocation.Y + Utils::Unit / 2 + ExplosionRadiusUnits * Utils::Unit;;

    for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
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
    return FText::FromString(FString::Printf(TEXT("%s"),
                                             *Super::GetDescriptionTitle().ToString()));
}

FText UEnvQueryTest_PlaceBomb::GetDescriptionDetails() const
{
    return DescribeBoolTestParams("");
}

#undef LOCTEXT_NAMESPACE
