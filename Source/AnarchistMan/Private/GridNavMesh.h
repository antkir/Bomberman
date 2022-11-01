// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "NavigationData.h"

#include <GraphAStar.h>
#include <Navigation/NavLocalGridData.h>
#include <NavMesh/NavMeshPath.h>
#include <Utils.h>

#include "GridNavMesh.generated.h"

DECLARE_CYCLE_STAT(TEXT("Grid A* Pathfinding"), STAT_Grid_Navigation_Pathfinding, STATGROUP_Navigation);

typedef FNavLocalGridData::FNodeRef FNodeRef;
typedef FGraphAStarDefaultNode<AGridNavMesh> FSearchNode;

/**
 * 
 */
UCLASS()
class AGridNavMesh : public ANavigationData
{
	GENERATED_BODY()

public:

    static constexpr float TIMEOUT_DEFAULT = TNumericLimits<float>::Lowest();

    AGridNavMesh();

    virtual void Tick(float DeltaSeconds) override;

    void BeginPlay() override;

    /**
     * the function is static for a reason, (wiki copy-paste->)
     * comments in the code explain it's for performance reasons: Epic are concerned
     * that if a lot of agents call the pathfinder in the same frame the virtual call overhead will accumulate and take too long,
     * so instead the function is declared static and stored in the FindPathImplementation function pointer.
     * Which means you need to manually set the function pointer in your new navigation class constructor
     * (or in some other function like we do here in SetHexGrid().
     */
    static FPathFindingResult FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);

    static bool TestPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query, int32* NumVisitedNodes);

    FORCEINLINE const FNavigationQueryFilter& GetRightFilterRef(FSharedConstNavQueryFilter Filter) const
    {
        return *(Filter.IsValid() ? Filter.Get() : GetDefaultQueryFilter().Get());
    }

    /** Project batch of points using shared search extent and filter */
    virtual void BatchProjectPoints(TArray<FNavigationProjectionWork>& Workload, const FVector& Extent, FSharedConstNavQueryFilter Filter = NULL, const UObject* Querier = NULL) const override;

    //////////////////////////////////////////////////////////////////////////

    /*  Type used as identification of nodes in the graph. */
    typedef FNodeRef FNodeRef;

    /* Returns whether given node identification is correct */
    bool IsValidRef(FNodeRef NodeRef) const;

    /* Returns neighbor ref */
    FNodeRef GetNeighbour(const FSearchNode& SearchNodeRef, const int32 NeighbourIndex) const;

    /* Returns number of neighbors that the graph node identified with NodeRef has */
    int32 GetNeighbourCount(FNodeRef NodeRef) const;

    //////////////////////////////////////////////////////////////////////////
    UFUNCTION(BlueprintCallable)
    int64 GetTileCost(FVector Location) const;

    UFUNCTION(BlueprintCallable)
    void SetTileCost(FVector Location, int64 Cost);

    UFUNCTION(BlueprintCallable)
    float GetTileTimeout(FVector Location) const;

    UFUNCTION(BlueprintCallable)
    void SetTileTimeout(FVector Location, float Timeout);

    UFUNCTION(BlueprintCallable)
    bool IsTileDangerous(FVector Location, float TimeBeforeTileMin, float TimeAfterTileMax) const;

    UFUNCTION(BlueprintCallable)
    bool IsPathSafe(FVector CharacterLocation, const TArray<FVector>& PathPoints);

    UFUNCTION(BlueprintCallable)
    FVector FindNearestCharacter(AActor* CurrentCharacter);

    UFUNCTION(BlueprintCallable)
    bool IsCharacterNearby(AActor* CurrentCharacter, int64 RadiusTiles);

private:

    struct FNodeDescription
    {
        FNodeRef NodeRef;
        int64 TraversalCost;
    };

    void BFS(const FNodeDescription& StartNode, std::function<bool(const FNodeDescription& CurrentNode)> NodeRefFunc, ETileNavCost::Type MaxTileNavCostAllowed);

public:

    FVector NodeRefToLocation(FNodeRef NodeRef) const;
    FNodeRef LocationToNodeRef(FVector Location) const;

    void ResetTiles();

    void GetReachableTiles(FVector ActorLocation, TArray<int64>& OutCosts, bool bAddMovementDelay = false);

    ///////////////////////////////////////////////////////////////////////////

    /** Array of tile costs that compose the grid. */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "NavMesh")
    TArray<int64> TileCosts;

    /** Array of tile costs that compose the grid. */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "NavMesh")
    TArray<float> TileTimeouts;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NavMesh")
    float PathPointZOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh")
    int32 Rows;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh")
    int32 Columns;

    /**
     * Toggle our debug drawing.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GraphAStarExample|PathFollowingComponent")
    bool bDrawDebug;
};

struct FGridNavMeshPath : public FNavMeshPath
{
    /*FORCEINLINE*/ virtual float GetCostFromIndex(int32 PathPointIndex) const override
    {
        return CurrentPathCost;
    }

    /*FORCEINLINE*/ virtual float GetLengthFromPosition(FVector SegmentStart, uint32 NextPathPointIndex) const override
    {
        // Excluding the starting point.
        return PathPoints.Num() - 1;
    }

    float CurrentPathCost = 0;
};

/**
 * TQueryFilter (FindPath's parameter) filter class is what decides which graph edges can be used and at what cost.
 * Use FRecastQueryFilter as a reference.
 */
struct FGridQueryFilter : public INavigationQueryFilterInterface
{
public:
    FGridQueryFilter(const AGridNavMesh* NavMesh);

    virtual void Reset() override;

    virtual void SetAreaCost(uint8 AreaType, float Cost) override;
    virtual void SetFixedAreaEnteringCost(uint8 AreaType, float Cost) override;
    virtual void SetExcludedArea(uint8 AreaType) override;
    virtual void SetAllAreaCosts(const float* CostArray, const int32 Count) override;
    virtual void GetAllAreaCosts(float* CostArray, float* FixedCostArray, const int32 Count) const override;
    virtual void SetBacktrackingEnabled(const bool bBacktracking) override;
    virtual bool IsBacktrackingEnabled() const override;
    virtual float GetHeuristicScale() const override;
    virtual bool IsEqual(const INavigationQueryFilterInterface* Other) const override;
    virtual void SetIncludeFlags(uint16 Flags) override;
    virtual uint16 GetIncludeFlags() const override;
    virtual void SetExcludeFlags(uint16 Flags) override;
    virtual uint16 GetExcludeFlags() const override;
    virtual FVector GetAdjustedEndLocation(const FVector& EndLocation) const override;
    virtual INavigationQueryFilterInterface* CreateCopy() const override;

    ////////////////////////////////////////////////////////////////////////////////////

    // FGraphAStar functions.

    /* Estimate of cost from StartNode to EndNode from a search node */
    float GetHeuristicCost(const FSearchNode& StartNode, const FSearchNode& EndNode) const;

    /* Real cost of traveling from StartNode directly to EndNode from a search node */
    float GetTraversalCost(const FSearchNode& StartNode, const FSearchNode& EndNode) const;

    /* Whether traversing given edge is allowed from a NodeRef */
    bool IsTraversalAllowed(const FNodeRef NodeA, const FNodeRef NodeB) const;

    /* Whether to accept solutions that do not reach the goal */
    bool WantsPartialSolution() const;

private:

    const AGridNavMesh* GridNavMesh;
};
