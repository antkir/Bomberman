// Copyright 2022 Kiryl Antonik

#pragma once

#include "CoreMinimal.h"
#include "GraphAStar.h"
#include "Navigation/NavLocalGridData.h"
#include "NavMesh/NavMeshPath.h"
#include "Game/AmUtils.h"

#include "AmGridNavMesh.generated.h"

DECLARE_CYCLE_STAT(TEXT("Grid A* Pathfinding"), STAT_Grid_Navigation_Pathfinding, STATGROUP_Navigation);

typedef FNavLocalGridData::FNodeRef FNodeRef;

/**
 * AAmGridNavMesh class contains methods for finding or testing a navigation path using A* algorithm.
 */
UCLASS()
class AAmGridNavMesh : public ANavigationData
{
	GENERATED_BODY()

	typedef FGraphAStarDefaultNode<AAmGridNavMesh> FSearchNode;

	struct FNodeDescription
	{
		FNodeRef NodeRef;
		int64 TraversalCost;
	};

	struct FResultPathNodes : TArray<FNodeDescription>
	{
		FNodeRef SetPathInfo(const int32 Index, const FSearchNode& SearchNode)
		{
			GetData()[Index].NodeRef = SearchNode.NodeRef;
			GetData()[Index].TraversalCost = SearchNode.TraversalCost;
			return -1;
		}
	};

public:

	static constexpr float TIMEOUT_UNSET = TNumericLimits<float>::Lowest();

	// Type used for identification of nodes in the graph.
	typedef FNodeRef FNodeRef;

public:

	AAmGridNavMesh();

protected:

	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginPlay() override;

public:

	/**
	 * Epic are concerned that if a lot of agents call the pathfinder in the same frame,
	 * the virtual call overhead will accumulate and take too long,
	 * so instead the function is declared static and stored in a function pointer,
	 * which means we need to manually set the function pointer in our new navigation class constructor.
	 */
	static FPathFindingResult FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);

	static bool TestPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query, int32* NumVisitedNodes);

	// Project batch of points using shared search extent and filter.
	virtual void BatchProjectPoints(TArray<FNavigationProjectionWork>& Workload, const FVector& Extent, FSharedConstNavQueryFilter Filter = NULL, const UObject* Querier = NULL) const override;

	bool IsValidRef(FNodeRef NodeRef) const;

	FNodeRef GetNeighbour(const FSearchNode& SearchNodeRef, const int32 NeighbourIndex) const;

	int32 GetNeighbourCount(FNodeRef NodeRef) const;

	FVector NodeRefToLocation(FNodeRef NodeRef) const;

	FNodeRef LocationToNodeRef(FVector Location) const;

	void ResetTiles();

	void GetReachableTiles(AController* Controller, TArray<float>& OutCosts, bool bAddMovementDelay = false) const;

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
	bool IsPathSafe(AController* Controller, const TArray<FVector>& PathPoints) const;

	UFUNCTION(BlueprintCallable)
	FVector FindNearestCharacter(AController* Controller) const;

	UFUNCTION(BlueprintCallable)
	bool IsCharacterNearby(AController* Controller, int64 RadiusTiles) const;

private:

	FORCEINLINE const FNavigationQueryFilter& GetRightFilterRef(FSharedConstNavQueryFilter Filter) const
	{
		return *(Filter.IsValid() ? Filter.Get() : GetDefaultQueryFilter().Get());
	}

	void BFS(AController* Controller, const FNodeDescription& StartNode, std::function<bool(const FNodeDescription& CurrentNode)> NodeRefFunc, ETileNavCost::Type MaxTileNavCostAllowed) const;

public:

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Properties", meta = (ClampMin = "0"))
	int32 Rows;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Properties", meta = (ClampMin = "0"))
	int32 Columns;

protected:

	/** Array of tile costs that compose the grid. */
	UPROPERTY(VisibleAnywhere, Category = "Properties")
	TArray<int64> TileCosts;

	/** Array of tile timeouts that compose the grid. */
	UPROPERTY(VisibleAnywhere, Category = "Properties")
	TArray<float> TileTimeouts;

	/** Toggle debug drawing. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Properties")
	bool bDrawDebugShapes;

	/** Toggle debug text. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Properties")
	bool bShowDebugText;
};

struct FGridGraphAStar : FGraphAStar<AAmGridNavMesh, FGraphAStarDefaultPolicy>
{
	FGridGraphAStar(const AAmGridNavMesh& InGraph) : FGraphAStar(InGraph)
	{
	}

	template<typename TQueryFilter, typename TResultPathInfo = TArray<FGraphNodeRef> >
	EGraphAStarResult FindPath(const FSearchNode& StartNode, const FSearchNode& EndNode, const TQueryFilter& Filter, TResultPathInfo& OutPath)
	{
		UE_GRAPH_ASTAR_LOG(Display, TEXT(""));
		UE_GRAPH_ASTAR_LOG(Display, TEXT("Starting FindPath request..."));

		if (!(Graph.IsValidRef(StartNode.NodeRef) && Graph.IsValidRef(EndNode.NodeRef)))
		{
			return SearchFail;
		}

		if (StartNode.NodeRef == EndNode.NodeRef)
		{
			return SearchSuccess;
		}

		if (FGraphAStarDefaultPolicy::bReuseNodePoolInSubsequentSearches)
		{
			NodePool.ReinitNodes();
		}
		else
		{
			NodePool.Reset();
		}
		OpenList.Reset();

		// kick off the search with the first node
		FSearchNode& StartPoolNode = NodePool.Add(StartNode);
		StartPoolNode.TraversalCost = 0;
		StartPoolNode.TotalCost = GetHeuristicCost(Filter, StartNode, EndNode) * Filter.GetHeuristicScale();

		OpenList.Push(StartPoolNode);

		int32 BestNodeIndex = StartPoolNode.SearchNodeIndex;
		float BestNodeCost = StartPoolNode.TotalCost;

		// Fixed StartPoolNode usage. Store these values now, so they are valid after NodePool resizing in ProcessSingleNode.
		const int32 StartNodeSearchIndex = StartPoolNode.SearchNodeIndex;
		const int32 StartNodeRef = StartPoolNode.NodeRef;

		EGraphAStarResult Result = EGraphAStarResult::SearchSuccess;
		const bool bIsBound = true;

		bool bProcessNodes = true;
		while (OpenList.Num() > 0 && bProcessNodes)
		{
			bProcessNodes = ProcessSingleNode(EndNode, bIsBound, Filter, BestNodeIndex, BestNodeCost);
		}

		// check if we've reached the goal
		if (BestNodeCost != 0.f)
		{
			Result = EGraphAStarResult::GoalUnreachable;
		}

		// no point to waste perf creating the path if querier doesn't want it
		if (Result == EGraphAStarResult::SearchSuccess || Filter.WantsPartialSolution())
		{
			// store the path. Note that it will be reversed!
			int32 SearchNodeIndex = BestNodeIndex;
			int32 PathLength = ShouldIncludeStartNodeInPath(Filter) && BestNodeIndex != StartNodeSearchIndex ? 1 : 0;
			do
			{
				PathLength++;
				SearchNodeIndex = NodePool[SearchNodeIndex].ParentNodeIndex;
			} while (NodePool.IsValidIndex(SearchNodeIndex) && NodePool[SearchNodeIndex].NodeRef != StartNodeRef && ensure(PathLength < FGraphAStarDefaultPolicy::FatalPathLength));

			if (PathLength >= FGraphAStarDefaultPolicy::FatalPathLength)
			{
				Result = EGraphAStarResult::InfiniteLoop;
			}

			OutPath.Reset(PathLength);
			OutPath.AddZeroed(PathLength);

			// store the path
			UE_GRAPH_ASTAR_LOG(Display, TEXT("Storing path result (length=%i)..."), PathLength);
			SearchNodeIndex = BestNodeIndex;
			int32 ResultNodeIndex = PathLength - 1;
			do
			{
				UE_GRAPH_ASTAR_LOG(Display, TEXT("  NodeRef %i"), NodePool[SearchNodeIndex].NodeRef);
				SetPathInfo(OutPath, ResultNodeIndex--, NodePool[SearchNodeIndex]);
				SearchNodeIndex = NodePool[SearchNodeIndex].ParentNodeIndex;
			} while (ResultNodeIndex >= 0);
		}

		return Result;
	}
};
