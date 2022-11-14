// Fill out your copyright notice in the Description page of Project Settings.

#include "AmEnvQueryGenerator_StaticGrid.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

UAmEnvQueryGenerator_StaticGrid::UAmEnvQueryGenerator_StaticGrid(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    GridSize.DefaultValue = 500.0f;

    SpaceBetween.DefaultValue = 100.0f;
}

void UAmEnvQueryGenerator_StaticGrid::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
    UObject* BindOwner = QueryInstance.Owner.Get();
    GridSize.BindData(BindOwner, QueryInstance.QueryID);
    SpaceBetween.BindData(BindOwner, QueryInstance.QueryID);

    float RadiusValue = GridSize.GetValue();
    float DensityValue = SpaceBetween.GetValue();

    const int32 ItemCount = FPlatformMath::TruncToInt((RadiusValue * 2.0f / DensityValue) + 1);
    const int32 ItemCountHalf = ItemCount / 2;

    TArray<FNavLocation> GridPoints;
    GridPoints.Reserve(ItemCount * ItemCount);

    for (int32 IndexX = 0; IndexX < ItemCount; ++IndexX)
    {
        for (int32 IndexY = 0; IndexY < ItemCount; ++IndexY)
        {
            if (IndexX % 2 == 1 && IndexY % 2 == 1)
            {
                continue;
            }

            const FNavLocation TestPoint = FNavLocation(GenerateAroundLocation + FVector(DensityValue * (IndexX - ItemCountHalf), DensityValue * (IndexY - ItemCountHalf), 0));
            GridPoints.Add(TestPoint);
        }
    }

    ProjectAndFilterNavPoints(GridPoints, QueryInstance);
    StoreNavPoints(GridPoints, QueryInstance);
}

FText UAmEnvQueryGenerator_StaticGrid::GetDescriptionTitle() const
{
    return FText::Format(
        LOCTEXT("StaticGridDescriptionGenerateAroundContext", "{0}: generate around {1}"),
        Super::GetDescriptionTitle(),
        FText::FromString(GenerateAroundLocation.ToString())
    );
};

FText UAmEnvQueryGenerator_StaticGrid::GetDescriptionDetails() const
{
    FText Desc = FText::Format(
        LOCTEXT("StaticGridDescription", "radius: {0}, space between: {1}"),
        FText::FromString(GridSize.ToString()),
        FText::FromString(SpaceBetween.ToString())
    );

    FText ProjDesc = ProjectionData.ToText(FEnvTraceData::Brief);
    if (!ProjDesc.IsEmpty())
    {
        FFormatNamedArguments ProjArgs;
        ProjArgs.Add(TEXT("Description"), Desc);
        ProjArgs.Add(TEXT("ProjectionDescription"), ProjDesc);
        Desc = FText::Format(
            LOCTEXT("StaticGridDescriptionWithProjection", "{Description}, {ProjectionDescription}"),
            ProjArgs
        );
    }

    return Desc;
}

#undef LOCTEXT_NAMESPACE
