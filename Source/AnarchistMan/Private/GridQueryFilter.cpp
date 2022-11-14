// Fill out your copyright notice in the Description page of Project Settings.

#include "GridQueryFilter.h"

#include <GridNavMesh.h>

FGridQueryFilter::FGridQueryFilter(const AGridNavMesh* NavMesh, float SpeedMultiplier, bool bDrawDebugShapes) :
    GridNavMesh(NavMesh), SpeedMultiplier(SpeedMultiplier), bDrawDebugShapes(bDrawDebugShapes)
{
}

void FGridQueryFilter::Reset()
{
}

void FGridQueryFilter::SetAreaCost(uint8 AreaType, float Cost)
{
}

void FGridQueryFilter::SetFixedAreaEnteringCost(uint8 AreaType, float Cost)
{
}

void FGridQueryFilter::SetExcludedArea(uint8 AreaType)
{
}

void FGridQueryFilter::SetAllAreaCosts(const float* CostArray, const int32 Count)
{
}

void FGridQueryFilter::GetAllAreaCosts(float* CostArray, float* FixedCostArray, const int32 Count) const
{
}

void FGridQueryFilter::SetBacktrackingEnabled(const bool bBacktracking)
{
}

bool FGridQueryFilter::IsBacktrackingEnabled() const
{
    return false;
}

float FGridQueryFilter::GetHeuristicScale() const
{
    return 1.0f;
}

bool FGridQueryFilter::IsEqual(const INavigationQueryFilterInterface* Other) const
{
    return false;
}

void FGridQueryFilter::SetIncludeFlags(uint16 Flags)
{
}

uint16 FGridQueryFilter::GetIncludeFlags() const
{
    return 0;
}

void FGridQueryFilter::SetExcludeFlags(uint16 Flags)
{
}

uint16 FGridQueryFilter::GetExcludeFlags() const
{
    return 0;
}

FVector FGridQueryFilter::GetAdjustedEndLocation(const FVector& EndLocation) const
{
    FVector Location = Utils::RoundToUnitCenter(EndLocation);
    Location.Z = GridNavMesh->GetActorLocation().Z;
    return Location;
}

INavigationQueryFilterInterface* FGridQueryFilter::CreateCopy() const
{
    return new FGridQueryFilter(GridNavMesh, SpeedMultiplier, bDrawDebugShapes);
}

float FGridQueryFilter::GetHeuristicCost(const FSearchNode& StartNode, const FSearchNode& EndNode) const
{
    FNodeRef StartNodeRef = StartNode.NodeRef;
    FNodeRef EndNodeRef = EndNode.NodeRef;

    FIntVector StartNodeLocation;
    StartNodeLocation.X = (StartNodeRef % GridNavMesh->Columns);
    StartNodeLocation.Y = (StartNodeRef / GridNavMesh->Columns);

    FIntVector EndNodeLocation;
    EndNodeLocation.X = (EndNodeRef % GridNavMesh->Columns);
    EndNodeLocation.Y = (EndNodeRef / GridNavMesh->Columns);

    FIntVector Delta = EndNodeLocation - StartNodeLocation;

    return FMath::Abs(Delta.X) + FMath::Abs(Delta.Y);
}

float FGridQueryFilter::GetTraversalCost(const FSearchNode& StartNode, const FSearchNode& EndNode) const
{
    FNodeRef EndNodeRef = EndNode.NodeRef;

    // We use approximately estimated values.

    // Derived from max default character speed of 600 units/sec.
    float TileDefaultMinPassingTime = 0.167f / SpeedMultiplier;
    // Worst case of time to pass 100 units (can be worse, but it rarely happens).
    float TileDefaultMaxPassingTime = 0.35f / SpeedMultiplier;
    // Average case of time to pass 100 units (derived from observations).
    float TileDefaultAveragePassingTime = 0.25f / SpeedMultiplier;
    // Average bomb lifetime + 3 tiles.
    float TileBlockPassingTime = 3.f + TileDefaultAveragePassingTime * 3;
    // Average bomb lifetime + 5 tiles.
    float TileBombPassingTime = 3.f + TileDefaultAveragePassingTime * 5;

    float TimeBeforeEndNodeMin = 0;
    // + 2 so character's hit box does not overlap explosion tile.
    // First one to be in the center of EndNode, second one to be in the center of EndNode + 1 node.
    float TimeAfterEndNodeMax = TileDefaultMaxPassingTime * 2;

    int64 TraversalCost = StartNode.TraversalCost;

    TimeBeforeEndNodeMin += TraversalCost / ETileNavCost::BOMB * TileBombPassingTime;
    TimeAfterEndNodeMax += TraversalCost / ETileNavCost::BOMB * TileBombPassingTime;
    TraversalCost %= ETileNavCost::BOMB;

    TimeBeforeEndNodeMin += TraversalCost / ETileNavCost::BLOCK * TileBlockPassingTime;
    TimeAfterEndNodeMax += TraversalCost / ETileNavCost::BLOCK * TileBlockPassingTime;
    TraversalCost %= ETileNavCost::BLOCK;

    TimeBeforeEndNodeMin += TraversalCost / ETileNavCost::DEFAULT * TileDefaultMinPassingTime;
    TimeAfterEndNodeMax += TraversalCost / ETileNavCost::DEFAULT * TileDefaultAveragePassingTime;
    TraversalCost %= ETileNavCost::DEFAULT;

    FVector EndNodeLocation = GridNavMesh->NodeRefToLocation(EndNodeRef);

    int64 PathCost = GridNavMesh->GetTileCost(EndNodeLocation);

    if (GridNavMesh->GetTileTimeout(EndNodeLocation) != AGridNavMesh::TIMEOUT_UNSET)
    {
        // Check if the tile explodes while we run through it.
        if (GridNavMesh->IsTileDangerous(EndNodeLocation, TimeBeforeEndNodeMin, TimeAfterEndNodeMax))
        {
            PathCost = ETileNavCost::BOMB;

            if (bDrawDebugShapes)
            {
                EndNodeLocation.Z = 300.f;
                DrawDebugSphere(GridNavMesh->GetWorld(), EndNodeLocation, 25.f, 8, FColor::Black, false, 0.05f);
            }
        }
    }

    return PathCost;
}

bool FGridQueryFilter::IsTraversalAllowed(const FNodeRef NodeA, const FNodeRef NodeB) const
{
    bool bTraversalAllowed = true;

    FVector Location = GridNavMesh->NodeRefToLocation(NodeB);
    if (GridNavMesh->GetTileCost(Location) >= ETileNavCost::BOMB)
    {
        bTraversalAllowed = false;

        if (bDrawDebugShapes)
        {
            Location.Z = 300.f;
            DrawDebugSphere(GridNavMesh->GetWorld(), Location, 25.f, 8, FColor::White, false, 0.05f);
        }
    }

    return bTraversalAllowed;
}

bool FGridQueryFilter::WantsPartialSolution() const
{
    return true;
}

void FGridQueryFilter::SetSpeedMultiplier(float Multiplier) const
{
    SpeedMultiplier = Multiplier;
}
