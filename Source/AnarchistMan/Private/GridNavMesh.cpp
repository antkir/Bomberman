// Fill out your copyright notice in the Description page of Project Settings.

#include "GridNavMesh.h"

#include <Bomb.h>
#include <BreakableBlock.h>
#include <PlayerCharacter.h>
#include <UtilsFunctionLibrary.h>

#include <AIModule/Public/GraphAStar.h>
#include <Kismet/GameplayStatics.h>

// TODO move this to another file
FGridQueryFilter::FGridQueryFilter(const AGridNavMesh* NavMesh) : GridNavMesh(NavMesh)
{
}

void FGridQueryFilter::Reset()
{}

void FGridQueryFilter::SetAreaCost(uint8 AreaType, float Cost)
{}

void FGridQueryFilter::SetFixedAreaEnteringCost(uint8 AreaType, float Cost)
{}

void FGridQueryFilter::SetExcludedArea(uint8 AreaType)
{}

void FGridQueryFilter::SetAllAreaCosts(const float* CostArray, const int32 Count)
{}

void FGridQueryFilter::GetAllAreaCosts(float* CostArray, float* FixedCostArray, const int32 Count) const
{}

void FGridQueryFilter::SetBacktrackingEnabled(const bool bBacktracking)
{}

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
{}

uint16 FGridQueryFilter::GetIncludeFlags() const
{
    return 0;
}

void FGridQueryFilter::SetExcludeFlags(uint16 Flags)
{}

uint16 FGridQueryFilter::GetExcludeFlags() const
{
    return 0;
}

FVector FGridQueryFilter::GetAdjustedEndLocation(const FVector& EndLocation) const
{
    FVector Location = Utils::RoundToUnitCenter(EndLocation);
    // TODO
    Location.Z = Utils::Unit;
    return Location;
}

INavigationQueryFilterInterface* FGridQueryFilter::CreateCopy() const
{
    return new FGridQueryFilter(GridNavMesh);
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
    // If EndNodeRef is a valid index we return the tile cost, 
    // if not we return 1, because the traversal cost need to be > 0 or the FGraphAStar will stop the execution
    // look at GraphAStar.h line 244: ensure(NewTraversalCost > 0);

    FNodeRef EndNodeRef = EndNode.NodeRef;

    // We use approximately estimated values.

    // Derived from max default character speed of 600 units/sec.
    float TileDefaultMinPassingTime = 0.167f;
    // Worst case of time to pass 100 units (can be worse, but it rarely happens).
    float TileDefaultMaxPassingTime = 0.35f;
    // Average case of time to pass 100 units (derived from observations).
    float TileDefaultAveragePassingTime = 0.25f;
    // Default bomb lifetime + 300 units.
    float TileBlockPassingTime = 3.f + TileDefaultAveragePassingTime * 3;
    // Default bomb lifetime + 500 units.
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

    FVector Location = GridNavMesh->NodeRefToLocation(EndNodeRef);

    float TileTimeout = GridNavMesh->GetTileTimeout(Location);

    int64 PathCost = ETileNavCost::DEFAULT;

    if (TileTimeout != AGridNavMesh::TIMEOUT_DEFAULT)
    {
        // Check if tile explodes while we run through it.
        if (GridNavMesh->IsTileDangerous(Location, TimeBeforeEndNodeMin, TimeAfterEndNodeMax))
        {
            PathCost = ETileNavCost::BOMB;
            Location.Z = 300.f;
            DrawDebugSphere(GridNavMesh->GetWorld(), Location, 25.f, 8, FColor::Black, false, 0.05f);
        }
    }
    else
    {
        // TODO GetTileCost variant with NodeRef parameter
        PathCost = GridNavMesh->GetTileCost(Location);
    }

    return PathCost;
}

bool FGridQueryFilter::IsTraversalAllowed(const FNodeRef NodeA, const FNodeRef NodeB) const
{
    // Here you can make a more complex operation like use a line trace to see if there is some obstacles (like an enemy).

    //TArray<FOverlapResult> OutOverlaps{};
    //FVector Location = GridNavMesh->NodeRefToLocation(NodeB);
    //FCollisionObjectQueryParams QueryParams;
    //QueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
    //FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(Utils::Unit / 8, Utils::Unit / 8, Utils::Unit));
    //GridNavMesh->GetWorld()->OverlapMultiByObjectType(OutOverlaps, Location, FQuat::Identity, QueryParams, CollisionShape);
    //
    //for (const FOverlapResult& Overlap : OutOverlaps)
    //{
    //    AActor* Actor = Overlap.GetActor();
    //    if (IsValid(Actor) && Cast<ABomb>(Actor))
    //    {
    //        return false;
    //    }
    //}

    bool bTraversalAllowed = true;

    FVector Location = GridNavMesh->NodeRefToLocation(NodeB);
    if (GridNavMesh->GetTileCost(Location) >= ETileNavCost::BOMB)
    {
        bTraversalAllowed = false;

        Location.Z = 300.f;
        DrawDebugSphere(GridNavMesh->GetWorld(), Location, 25.f, 8, FColor::White, false, 0.05f);
    }

    return bTraversalAllowed;
}

bool FGridQueryFilter::WantsPartialSolution() const
{
    return true;
}

AGridNavMesh::AGridNavMesh()
{
    PrimaryActorTick.bCanEverTick = true;

    // TODO should be editable
    Rows = 20;
    Columns = 20;

    FindPathImplementation = FindPath;
    TestPathImplementation = TestPath;

    //DefaultQueryFilter->SetFilterType<FGridQueryFilter>();
    //auto QueryFilter = (FGridQueryFilter*) DefaultQueryFilter->GetImplementation();
    //QueryFilter->SetNavMesh(this);

    FGridQueryFilter QueryFilter(this);
    DefaultQueryFilter->SetFilterImplementation(&QueryFilter);

    int64 Cost = 1;
    TileCosts.Init(Cost, Columns * Rows);

    float Timeout = TIMEOUT_DEFAULT;
    TileTimeouts.Init(Timeout, Columns * Rows);
}

void AGridNavMesh::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bDrawDebug)
    {
        for (FNodeRef NodeRef = 0; NodeRef < TileCosts.Num(); NodeRef++)
        {
            FVector Location = NodeRefToLocation(NodeRef);
            Location.Z += 100.f;

            FString Text;
            FColor Color;

            // TODO set universal variable
            if (TileTimeouts[NodeRef] > 0.7f)
            {
                Color = FColor::Yellow;
            }
            else if (TileTimeouts[NodeRef] >= 0.f)
            {
                Color = FColor::Red;
            }
            else if (TileTimeouts[NodeRef] != TIMEOUT_DEFAULT)
            {
                Color = FColor::White;
            }
            else
            {
                switch (TileCosts[NodeRef])
                {
                case ETileNavCost::DEFAULT:
                    Color = FColor::Green;
                    break;
                case ETileNavCost::BLOCK:
                    Color = FColor::Black;
                    break;
                case ETileNavCost::BOMB:
                    Color = FColor::Red;
                    break;
                default:
                    Color = FColor::White;
                }
            }

            DrawDebugSphere(GetWorld(), Location, 25.f, 8, Color, false, 0.0f);
        }
    }
}

void AGridNavMesh::BeginPlay()
{
    Super::BeginPlay();

    for (uint64 Index = 0; Index < TileCosts.Num(); Index++)
    {
        TileCosts[Index] = 1;
    }

    for (uint64 Index = 0; Index < TileTimeouts.Num(); Index++)
    {
        TileTimeouts[Index] = TIMEOUT_DEFAULT;
    }
}

FPathFindingResult AGridNavMesh::FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
    SCOPE_CYCLE_COUNTER(STAT_Grid_Navigation_Pathfinding);
    CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Pathfinding);

    const ANavigationData* Self = Query.NavData.Get();
    check(Cast<const AGridNavMesh>(Self));

    const AGridNavMesh* NavMesh = (const AGridNavMesh*) Self;
    if (Self == NULL)
    {
        return ENavigationQueryResult::Error;
    }

    FPathFindingResult Result(ENavigationQueryResult::Error);

    FNavigationPath* NavPath = Query.PathInstanceToFill.Get();
    FGridNavMeshPath* NavMeshPath = NavPath ? NavPath->CastPath<FGridNavMeshPath>() : nullptr;

    if (NavMeshPath)
    {
        Result.Path = Query.PathInstanceToFill;
        NavMeshPath->ResetForRepath();
    }
    else
    {
        Result.Path = Self->CreatePathInstance<FGridNavMeshPath>(Query);
        NavPath = Result.Path.Get();
        NavMeshPath = NavPath ? NavPath->CastPath<FGridNavMeshPath>() : nullptr;
    }

    float DebugCost = 0.f;

    const FNavigationQueryFilter* NavFilter = Query.QueryFilter.Get();
    if (NavMeshPath && NavFilter)
    {
        NavMeshPath->ApplyFlags(Query.NavDataFlags);

        FVector StartLocation = Utils::RoundToUnitCenter(Query.StartLocation);
        // TODO
        StartLocation.Z = Utils::Unit;

        FVector EndLocation = Utils::RoundToUnitCenter(Query.EndLocation);
        // TODO
        EndLocation.Z = Utils::Unit;

        if ((StartLocation - EndLocation).IsNearlyZero(25.f) == true)
        {
            Result.Path->GetPathPoints().Reset();
            Result.Path->GetPathPoints().Add(FNavPathPoint(EndLocation));
            Result.Result = ENavigationQueryResult::Success;
        }
        else
        {
            // Reset path points.
            Result.Path->GetPathPoints().Reset();

            FNodeRef StartNodeRef = NavMesh->LocationToNodeRef(StartLocation);
            FNodeRef EndNodeRef = NavMesh->LocationToNodeRef(EndLocation);

            FGraphAStar<AGridNavMesh> Pathfinder(*NavMesh);
            // TODO check if we need to check if it's set / check if it can be set from BP
            const auto* GridQueryFilter = static_cast<const FGridQueryFilter*>(NavFilter->GetImplementation());

            TArray<FNodeRef> PathNodes;
            EGraphAStarResult AStarResult = Pathfinder.FindPath(StartNodeRef, EndNodeRef, *GridQueryFilter, PathNodes);

            GEngine->AddOnScreenDebugMessage(-3, 0.1f, FColor::Green, FString::Printf(TEXT("FindPath result: %d"), AStarResult));

            switch (AStarResult)
            {
            case GoalUnreachable:
            {
                Result.Result = ENavigationQueryResult::Invalid;
                break;
            }
            case InfiniteLoop:
                Result.Result = ENavigationQueryResult::Error;
                break;
            case SearchFail:
                Result.Result = ENavigationQueryResult::Invalid;
                break;
            case SearchSuccess:
                Result.Result = ENavigationQueryResult::Success;

                // We don't include the starting point in PathNodes, because it can be a bomb and we don't need to check if it's dangerous.
                FVector StartTileLocation = Utils::RoundToUnitCenter(Query.StartLocation);
                Result.Path->GetPathPoints().Add(FNavPathPoint(StartTileLocation));

                for (const FNodeRef& PathNode : PathNodes)
                {
                    FVector PathNodeLocation = NavMesh->NodeRefToLocation(PathNode);

                    // If the path is blocked by a breakable block, end the path in front of the block and report path as partial.
                    if (NavMesh->GetTileCost(PathNodeLocation) == ETileNavCost::BLOCK)
                    {
                        Result.Path->SetIsPartial(true);
                        break;
                    }

                    // If the path is blocked by a bomb or something more dangerous, mark this path as invalid.
                    if (NavMesh->GetTileCost(PathNodeLocation) >= ETileNavCost::BOMB)
                    {
                        Result.Result = ENavigationQueryResult::Invalid;
                        break;
                    }

                    Result.Path->GetPathPoints().Add(FNavPathPoint(PathNodeLocation));

                    DebugCost += NavMesh->GetTileCost(PathNodeLocation);
                }

                // Mark the path as ready.
                Result.Path->MarkReady();
                break;
            }
        }
    }

    GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green, FString::Printf(TEXT("FindPath cost: %f"), DebugCost));

    return Result;
}

bool AGridNavMesh::TestPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query, int32* NumVisitedNodes)
{
    // TODO code should match FindPath
    SCOPE_CYCLE_COUNTER(STAT_Grid_Navigation_Pathfinding);
    CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Pathfinding);

    const ANavigationData* Self = Query.NavData.Get();
    check(Cast<const AGridNavMesh>(Self));

    const AGridNavMesh* NavMesh = (const AGridNavMesh*) Self;
    if (Self == NULL)
    {
        return false;
    }

    bool bPathExists = true;

    const FNavigationQueryFilter* NavFilter = Query.QueryFilter.Get();
    if (NavFilter)
    {
        const FVector AdjustedEndLocation = NavFilter->GetAdjustedEndLocation(Query.EndLocation);
        if ((Query.StartLocation - AdjustedEndLocation).IsNearlyZero() == false)
        {
            FVector StartLocation = Utils::RoundToUnitCenter(Query.StartLocation);
            FVector EndLocation = Utils::RoundToUnitCenter(Query.EndLocation);

            FNodeRef StartIdx = NavMesh->LocationToNodeRef(StartLocation);
            FNodeRef EndIdx = NavMesh->LocationToNodeRef(EndLocation);
            FGraphAStar<AGridNavMesh> Pathfinder(*NavMesh);
            // TODO check if we need to check if it's set / check if it can be set from BP
            const auto* GridQueryFilter = static_cast<const FGridQueryFilter*>(NavFilter->GetImplementation());
            TArray<FNodeRef> PathNodes;
            EGraphAStarResult AStarResult = Pathfinder.FindPath(StartIdx, EndIdx, *GridQueryFilter, PathNodes);

            switch (AStarResult)
            {
            case SearchSuccess:
            {
                for (const FNodeRef& PathNode : PathNodes)
                {
                    if (NumVisitedNodes)
                    {
                        NumVisitedNodes[PathNode]++;
                    }

                    // TODO check if correct
                    // If the path is blocked by a breakable block, end the path in front of it and report path as partial.
                    FVector PathNodeLocation = NavMesh->NodeRefToLocation(PathNode);
                    if (NavMesh->GetTileCost(PathNodeLocation) == ETileNavCost::BLOCK)
                    {
                        bPathExists = false;
                        break;
                    }

                    if (NavMesh->GetTileCost(PathNodeLocation) == ETileNavCost::BOMB)
                    {
                        bPathExists = false;
                        break;
                    }
                }
                break;
            }
            case GoalUnreachable:
            case InfiniteLoop:
            case SearchFail:
                bPathExists = false;
                break;
            }
        }
    }

    GEngine->AddOnScreenDebugMessage(-2, 0.5f, FColor::Green, FString::Printf(TEXT("TestPath exists: %d"), bPathExists));

    return bPathExists;
}

void AGridNavMesh::BatchProjectPoints(TArray<FNavigationProjectionWork>& Workload, const FVector& Extent, FSharedConstNavQueryFilter Filter, const UObject* Querier) const
{
    if (Workload.Num() == 0)
    {
        return;
    }

    const FNavigationQueryFilter& FilterToUse = GetRightFilterRef(Filter);

    const FGridQueryFilter* QueryFilter = static_cast<const FGridQueryFilter*>(FilterToUse.GetImplementation());

    if (ensure(QueryFilter))
    {
        for (int32 Idx = 0; Idx < Workload.Num(); Idx++)
        {
            FNodeRef NodeRef = LocationToNodeRef(Workload[Idx].Point);

            Workload[Idx].OutLocation = FNavLocation(Workload[Idx].Point, NodeRef);
            Workload[Idx].bResult = true;
        }
    }
}

bool AGridNavMesh::IsValidRef(FNodeRef NodeRef) const
{
    uint32_t X = NodeRef % Columns;
    uint32_t Y = NodeRef / Columns;
    return X != 0 && Y != 0 && (Y % 2 == 1 || X % 2 == 1) && NodeRef >= 0 && NodeRef < TileCosts.Num();
}

FNodeRef AGridNavMesh::GetNeighbour(const FSearchNode& SearchNodeRef, const int32 NeighbourIndex) const
{
    FNodeRef NodeRef = SearchNodeRef.NodeRef;
    FIntVector NeighbourLocationTiles;
    NeighbourLocationTiles.X = NodeRef % Columns;
    NeighbourLocationTiles.Y = NodeRef / Columns;

    switch (NeighbourIndex)
    {
    case 0:
        NeighbourLocationTiles.X--;
        break;
    case 1:
        NeighbourLocationTiles.X++;
        break;
    case 2:
        NeighbourLocationTiles.Y--;
        break;
    case 3:
        NeighbourLocationTiles.Y++;
        break;
    }

    return NeighbourLocationTiles.Y * Columns + NeighbourLocationTiles.X;
}

int32 AGridNavMesh::GetNeighbourCount(FNodeRef NodeRef) const
{
    return 4;
}

int64 AGridNavMesh::GetTileCost(FVector Location) const
{
    FNodeRef NodeRef = LocationToNodeRef(Location);
    if (TileCosts.IsValidIndex(NodeRef))
    {
        return TileCosts[NodeRef];
    }
    else
    {
        return TNumericLimits<int64>::Min();
    }
}

void AGridNavMesh::SetTileCost(FVector Location, int64 Cost)
{
    FNodeRef NodeRef = LocationToNodeRef(Location);
    if (TileCosts.IsValidIndex(NodeRef))
    {
        TileCosts[NodeRef] = Cost;
    }
}

float AGridNavMesh::GetTileTimeout(FVector Location) const
{
    FNodeRef NodeRef = LocationToNodeRef(Location);
    if (TileTimeouts.IsValidIndex(NodeRef))
    {
        return TileTimeouts[NodeRef];
    }
    else
    {
        return TIMEOUT_DEFAULT;
    }
}

void AGridNavMesh::SetTileTimeout(FVector Location, float Timeout)
{
    FNodeRef NodeRef = LocationToNodeRef(Location);
    if (TileTimeouts.IsValidIndex(NodeRef))
    {
        TileTimeouts[NodeRef] = Timeout;
    }
}

bool AGridNavMesh::IsTileDangerous(FVector Location, float TimeBeforeTileMin, float TimeAfterTileMax) const
{
    bool bIsDangerous = false;

    float TileTimeout = GetTileTimeout(Location);

    if (TileTimeout != TIMEOUT_DEFAULT)
    {
        // Check if tile explodes while we run through it.
        if (TimeBeforeTileMin <= TileTimeout && TileTimeout <= TimeAfterTileMax)
        {
            bIsDangerous = true;
        }
    }

    return bIsDangerous;
}

bool AGridNavMesh::IsPathSafe(FVector CharacterLocation, const TArray<FVector>& PathPoints)
{
    bool bIsSafe = true;

    GEngine->AddOnScreenDebugMessage(-5, 0.1f, FColor::Yellow, FString::Printf(TEXT("Is Path Safe: %f, %f"), CharacterLocation.X, CharacterLocation.Y));

    if (PathPoints.IsEmpty())
    {
        GEngine->AddOnScreenDebugMessage(-15, 0.1f, FColor::Red, FString::Printf(TEXT("Path is empty: %f, %f"), CharacterLocation.X, CharacterLocation.Y));
        return bIsSafe;
    }

    FGridQueryFilter QueryFilter(this);
    FNodeRef CharacterNodeRef = LocationToNodeRef(CharacterLocation);
    int64 TraversalCost = 0;
    bool bFoundCurrentTile = false;

    for (uint64 Index = 0; Index + 1 < PathPoints.Num(); Index++)
    {
        FVector StartLocation = PathPoints[Index];
        FNodeRef StartNodeRef = LocationToNodeRef(StartLocation);

        FVector EndLocation = PathPoints[Index + 1];
        FNodeRef EndNodeRef = LocationToNodeRef(EndLocation);

        if (CharacterNodeRef == StartNodeRef)
        {
            bFoundCurrentTile = true;
            GEngine->AddOnScreenDebugMessage(-10, 0.1f, FColor::Yellow, FString::Printf(TEXT("Found current tile: %f, %f"), StartLocation.X, StartLocation.Y));
        }

        if (!bFoundCurrentTile)
        {
            continue;
        }

        GEngine->AddOnScreenDebugMessage(-100 - Index, 0.1f, FColor::Green, FString::Printf(TEXT("Found safe path tile: %f, %f"), StartLocation.X, StartLocation.Y));

        if (QueryFilter.IsTraversalAllowed(StartNodeRef, EndNodeRef))
        {
            FSearchNode StartSearchNode(StartNodeRef);
            StartSearchNode.TraversalCost = TraversalCost;

            FSearchNode EndSearchNode(EndNodeRef);

            TraversalCost += QueryFilter.GetTraversalCost(StartSearchNode, EndSearchNode);

            if (TraversalCost >= ETileNavCost::BOMB)
            {
                bIsSafe = false;
                break;
            }
        }
        else
        {
            bIsSafe = false;
            break;
        }
    }

    return bIsSafe;
}

FVector AGridNavMesh::FindNearestCharacter(AActor* CurrentCharacter)
{
    TSet<FNodeRef> CharacterNodeRefs;

    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(this, APlayerCharacter::StaticClass(), Actors);
    for (AActor* Actor : Actors)
    {
        if (CurrentCharacter != Actor)
        {
            FVector ActorLocation = Utils::RoundToUnitCenter(Actor->GetActorLocation());
            FNodeRef ActorNodeRef = LocationToNodeRef(ActorLocation);
            CharacterNodeRefs.Add(ActorNodeRef);
        }
    }

    FNodeDescription StartNode{ LocationToNodeRef(CurrentCharacter->GetActorLocation()), 0 };

    FNodeRef CharacterNodeRef = StartNode.NodeRef;
    float BestCost = TNumericLimits<float>::Max();

    auto CheckNearest = [this, &CharacterNodeRefs, &BestCost, &CharacterNodeRef](const FNodeDescription& CurrentNode) -> bool
    {
        bool Continue = true;
        if (CharacterNodeRefs.Contains(CurrentNode.NodeRef) && CurrentNode.TraversalCost < BestCost)
        {
            BestCost = CurrentNode.TraversalCost;
            CharacterNodeRef = CurrentNode.NodeRef;
            Continue = false;
        }
        return Continue;
    };

    BFS(StartNode, CheckNearest, ETileNavCost::BLOCK);

    FVector CharacterLocation = NodeRefToLocation(CharacterNodeRef);
    // TODO get level height from somewhere
    CharacterLocation.Z = Utils::Unit;
    return CharacterLocation;
}

bool AGridNavMesh::IsCharacterNearby(AActor* CurrentCharacter, int64 RadiusTiles)
{
    TSet<FNodeRef> CharacterNodeRefs;

    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(this, APlayerCharacter::StaticClass(), Actors);
    for (AActor* Actor : Actors)
    {
        if (CurrentCharacter != Actor)
        {
            FVector ActorLocation = Utils::RoundToUnitCenter(Actor->GetActorLocation());
            FNodeRef ActorNodeRef = LocationToNodeRef(ActorLocation);
            CharacterNodeRefs.Add(ActorNodeRef);
        }
    }

    FNodeDescription StartNode{ LocationToNodeRef(CurrentCharacter->GetActorLocation()), 0 };

    bool IsNearby = false;

    auto CheckNearest = [this, &CharacterNodeRefs, &IsNearby, RadiusTiles](const FNodeDescription& CurrentNode) -> bool
    {
        bool Continue = true;

        int64 TilesCount = CurrentNode.TraversalCost / ETileNavCost::DEFAULT;
        if (TilesCount > RadiusTiles)
        {
            Continue = false;
            return Continue;
        }

        if (CharacterNodeRefs.Contains(CurrentNode.NodeRef))
        {
            IsNearby = true;
            Continue = false;
        }

        return Continue;
    };

    BFS(StartNode, CheckNearest, ETileNavCost::DEFAULT);

    return IsNearby;
}

void AGridNavMesh::BFS(const FNodeDescription& StartNode, std::function<bool(const FNodeDescription& CurrentNode)> NodeRefFunc, ETileNavCost::Type MaxTileNavCostAllowed)
{
    FGridQueryFilter QueryFilter(this);

    TArray<int64> VisitedCost;
    VisitedCost.Init(TNumericLimits<int64>::Max(), Columns * Rows);
    VisitedCost[StartNode.NodeRef] = 0;

    // Create a queue for BFS
    TQueue<FNodeDescription> Queue;
    Queue.Enqueue(StartNode);

    while (!Queue.IsEmpty())
    {
        FNodeDescription CurrentNode;
        Queue.Dequeue(CurrentNode);

        if (!NodeRefFunc(CurrentNode))
        {
            return;
        }

        FSearchNode CurrentSearchNode(CurrentNode.NodeRef);
        CurrentSearchNode.TraversalCost = CurrentNode.TraversalCost;

        for (int32 NeighbourIdx = 0; NeighbourIdx < GetNeighbourCount(CurrentSearchNode.NodeRef); NeighbourIdx++)
        {
            FNodeRef NeighbourNodeRef = GetNeighbour(CurrentSearchNode, NeighbourIdx);
            if (IsValidRef(NeighbourNodeRef))
            {
                FSearchNode NeighbourSearchNode(NeighbourNodeRef);
                FVector NeighbourLocation = NodeRefToLocation(NeighbourNodeRef);
                float NeighbourCost = QueryFilter.GetTraversalCost(CurrentSearchNode, NeighbourSearchNode);
                float NeighbourTraversalCost = CurrentSearchNode.TraversalCost + NeighbourCost;

                if (VisitedCost.IsValidIndex(NeighbourNodeRef) &&
                    VisitedCost[NeighbourNodeRef] > NeighbourTraversalCost &&
                    QueryFilter.IsTraversalAllowed(CurrentSearchNode.NodeRef, NeighbourNodeRef) &&
                    NeighbourCost <= MaxTileNavCostAllowed)
                {
                    VisitedCost[NeighbourNodeRef] = NeighbourTraversalCost;

                    FNodeDescription NeighbourNode{ NeighbourNodeRef, NeighbourTraversalCost };
                    Queue.Enqueue(NeighbourNode);
                }
            }
        }
    }
}

FVector AGridNavMesh::NodeRefToLocation(FNodeRef NodeRef) const
{
    FVector NodeLocation;
    NodeLocation.X = (NodeRef % Columns) * Utils::Unit + Utils::Unit / 2;
    NodeLocation.Y = (NodeRef / Columns) * Utils::Unit + Utils::Unit / 2;
    // TODO get parent Z
    NodeLocation.Z = Utils::Unit;

    return NodeLocation;
}

FNodeRef AGridNavMesh::LocationToNodeRef(FVector Location) const
{
    FIntVector LocationIndex;
    LocationIndex.X = Location.X / 100.f;
    LocationIndex.Y = Location.Y / 100.f;

    return LocationIndex.Y * Columns + LocationIndex.X;
}

void AGridNavMesh::ResetTiles()
{
    for (FNodeRef NodeRef = 0; NodeRef < TileCosts.Num(); NodeRef++)
    {
        TileCosts[NodeRef] = 1;
    }

    for (FNodeRef NodeRef = 0; NodeRef < TileTimeouts.Num(); NodeRef++)
    {
        TileTimeouts[NodeRef] = TIMEOUT_DEFAULT;
    }
}

void AGridNavMesh::GetReachableTiles(FVector ActorLocation, TArray<int64>& OutCosts, bool bAddMovementDelay)
{
    OutCosts.Init(TNumericLimits<int64>::Max(), Columns * Rows);

    int64 StartCost = bAddMovementDelay ? 1 : 0;
    FNodeDescription StartNode{ LocationToNodeRef(ActorLocation), StartCost };

    auto SetOutCost = [&OutCosts](const FNodeDescription& CurrentNode) -> bool
    {
        OutCosts[CurrentNode.NodeRef] = CurrentNode.TraversalCost;
        return true;
    };

    BFS(StartNode, SetOutCost, ETileNavCost::DEFAULT);
}
