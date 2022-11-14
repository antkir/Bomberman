// Fill out your copyright notice in the Description page of Project Settings.

#include "AmEnvQueryTest_Bombs.h"

#include <EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h>
#include <NavigationSystem.h>

#include <AmGridNavMesh.h>
#include <AmUtils.h>

UAmEnvQueryTest_Bombs::UAmEnvQueryTest_Bombs(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    Cost = EEnvTestCost::High;

    ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();

    SetWorkOnFloatValues(true);
}

void UAmEnvQueryTest_Bombs::RunTest(FEnvQueryInstance& QueryInstance) const
{
    UObject* QueryOwner = QueryInstance.Owner.Get();
    AddMovementStartDelay.BindData(QueryOwner, QueryInstance.QueryID);

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(QueryInstance.World);
    if (NavSys == nullptr || QueryOwner == nullptr)
    {
        return;
    }

    ANavigationData* NavData;
    INavAgentInterface* NavAgent = Cast<INavAgentInterface>(QueryOwner);
    if (NavAgent)
    {
        NavData = NavSys->GetNavDataForProps(NavAgent->GetNavAgentPropertiesRef(), NavAgent->GetNavAgentLocation());
    }
    else
    {
        NavData = NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
    }

    if (NavData == nullptr)
    {
        return;
    }

    auto* GridNavMesh = Cast<AAmGridNavMesh>(NavData);
    if (GridNavMesh == nullptr)
    {
        UE_LOG(LogGame, Error, TEXT("At least one Grid Nav Mesh must be present in this level!"));
        return;
    }

    FVector ActorLocation = Cast<AActor>(QueryOwner)->GetActorLocation();
    ActorLocation = FAmUtils::RoundToUnitCenter(ActorLocation);
    FNodeRef ActorNode = GridNavMesh->LocationToNodeRef(ActorLocation);

    NavData->BeginBatchQuery();

    int32 ItemsNum = QueryInstance.Items.Num();

    TArray<float> ReachableTilesCost;
    GridNavMesh->GetReachableTiles(Cast<APawn>(QueryOwner)->GetController(), ReachableTilesCost, AddMovementStartDelay.GetValue());

    FEnvQueryInstance::ItemIterator It(this, QueryInstance);
    It.IgnoreTimeLimit();
    for ( ; It; ++It)
    {
        FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());
        FNodeRef NodeRef = GridNavMesh->LocationToNodeRef(ItemLocation);

        float Score = TNumericLimits<float>::Max();

        if (ReachableTilesCost.IsValidIndex(NodeRef) &&
            ReachableTilesCost[NodeRef] != TNumericLimits<int64>::Max() &&
            GridNavMesh->GetTileCost(ItemLocation) <= ETileNavCost::DEFAULT &&
            GridNavMesh->GetTileTimeout(ItemLocation) == AAmGridNavMesh::TIMEOUT_UNSET)
        {
            Score = float(ReachableTilesCost[NodeRef]);
        }

        It.SetScore(TestPurpose, FilterType, Score, FloatValueMin.GetValue(), FloatValueMax.GetValue());
    }

    NavData->FinishBatchQuery();
}

FText UAmEnvQueryTest_Bombs::GetDescriptionTitle() const
{
    return FText::FromString(FString::Printf(TEXT("%s"), *Super::GetDescriptionTitle().ToString()));
}

FText UAmEnvQueryTest_Bombs::GetDescriptionDetails() const
{
    return DescribeBoolTestParams("");
}
