// Fill out your copyright notice in the Description page of Project Settings.

#include "AmGridNavMesh.h"

#include "AIModule/Public/GraphAStar.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#include "AmGridQueryFilter.h"
#include "Player/AmMainPlayerCharacter.h"

static float GetSpeedMultiplier(const AActor* Owner)
{
	const auto* Controller = Cast<const AController>(Owner);
	const auto* PlayerCharacter = Controller->GetPawn<const AAmMainPlayerCharacter>();
	return PlayerCharacter->GetCharacterMovement()->GetMaxSpeed() / PlayerCharacter->GetDefaultMaxWalkSpeed();
}

AAmGridNavMesh::AAmGridNavMesh()
{
	PrimaryActorTick.bCanEverTick = true;

	bDrawDebugShapes = false;
	bShowDebugText = false;

	Rows = 5;
	Columns = 5;

	FindPathImplementation = FindPath;
	TestPathImplementation = TestPath;
}

void AAmGridNavMesh::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDrawDebugShapes)
	{
		for (FNodeRef NodeRef = 0; NodeRef < TileCosts.Num(); NodeRef++)
		{
			FVector Location = NodeRefToLocation(NodeRef);
			Location.Z += FAmUtils::Unit;

			FString Text;
			FColor Color;

			// Worst case of time to pass 100 units * 2 = 0.7
			if (TileTimeouts[NodeRef] > 0.7f)
			{
				Color = FColor::Yellow;
			}
			else if (TileTimeouts[NodeRef] >= 0.f)
			{
				Color = FColor::Red;
			}
			else if (TileTimeouts[NodeRef] != TIMEOUT_UNSET)
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

void AAmGridNavMesh::BeginPlay()
{
	Super::BeginPlay();

	FAmGridQueryFilter QueryFilter(this, 1.f, bDrawDebugShapes);
	DefaultQueryFilter->SetFilterImplementation(&QueryFilter);

	TileCosts.Init(ETileNavCost::DEFAULT, Columns * Rows);

	TileTimeouts.Init(TIMEOUT_UNSET, Columns * Rows);
}

FPathFindingResult AAmGridNavMesh::FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
	SCOPE_CYCLE_COUNTER(STAT_Grid_Navigation_Pathfinding);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Pathfinding);

	const ANavigationData* Self = Query.NavData.Get();
	check(Cast<const AAmGridNavMesh>(Self));

	const AAmGridNavMesh* NavMesh = (const AAmGridNavMesh*) Self;
	if (Self == NULL)
	{
		return ENavigationQueryResult::Error;
	}

	FPathFindingResult Result(ENavigationQueryResult::Error);

	FNavigationPath* NavPath = Query.PathInstanceToFill.Get();
	FNavMeshPath* NavMeshPath = NavPath ? NavPath->CastPath<FNavMeshPath>() : nullptr;

	if (NavMeshPath)
	{
		Result.Path = Query.PathInstanceToFill;
		NavMeshPath->ResetForRepath();
	}
	else
	{
		Result.Path = Self->CreatePathInstance<FNavMeshPath>(Query);
		NavPath = Result.Path.Get();
		NavMeshPath = NavPath ? NavPath->CastPath<FNavMeshPath>() : nullptr;
	}

	float TraversalCost = 0.f;

	FVector StartLocation = FAmUtils::RoundToUnitCenter(Query.StartLocation);
	StartLocation.Z = NavMesh->GetActorLocation().Z;

	FVector EndLocation = FAmUtils::RoundToUnitCenter(Query.EndLocation);
	EndLocation.Z = NavMesh->GetActorLocation().Z;

	FNodeRef StartNodeRef = NavMesh->LocationToNodeRef(StartLocation);
	FNodeRef EndNodeRef = NavMesh->LocationToNodeRef(EndLocation);

	if (NavMesh->bShowDebugText)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("FindPath from %d to %d"), StartNodeRef, EndNodeRef));
	}

	const FNavigationQueryFilter* NavFilter = Query.QueryFilter.Get();
	if (NavMeshPath && NavFilter && NavFilter->GetImplementation())
	{
		NavMeshPath->ApplyFlags(Query.NavDataFlags);

		if ((StartLocation - EndLocation).IsNearlyZero(10.f) == true)
		{
			Result.Path->GetPathPoints().Reset();
			Result.Path->GetPathPoints().Add(FNavPathPoint(EndLocation));
			Result.Result = ENavigationQueryResult::Success;
		}
		else
		{
			// Reset path points.
			Result.Path->GetPathPoints().Reset();

			FGridGraphAStar Pathfinder(*NavMesh);
			const auto* GridQueryFilter = static_cast<const FAmGridQueryFilter*>(NavFilter->GetImplementation());
			GridQueryFilter->SetSpeedMultiplier(GetSpeedMultiplier(Cast<const AActor>(Query.Owner)));
			FResultPathNodes PathNodes;
			EGraphAStarResult AStarResult = Pathfinder.FindPath(StartNodeRef, EndNodeRef, *GridQueryFilter, PathNodes);

			switch (AStarResult)
			{
			case GoalUnreachable:
				Result.Result = ENavigationQueryResult::Invalid;
				break;
			case InfiniteLoop:
				Result.Result = ENavigationQueryResult::Error;
				break;
			case SearchFail:
				Result.Result = ENavigationQueryResult::Fail;
				break;
			case SearchSuccess:
			{
				Result.Result = ENavigationQueryResult::Success;

				// Add the starting tile manually, because we can also leave it, so we don't need to check if it's dangerous.
				Result.Path->GetPathPoints().Add(FNavPathPoint(StartLocation));

				FVector PrevNodeLocation = StartLocation;

				for (const FNodeDescription& PathNode : PathNodes)
				{
					// If the path is blocked by a bomb or something more dangerous, mark this path as invalid.
					if (PathNode.TraversalCost >= ETileNavCost::BOMB)
					{
						Result.Result = ENavigationQueryResult::Invalid;
						break;
					}
					// If the path is blocked by a breakable block, end the path in front of the block and report path as partial.
					else if (PathNode.TraversalCost >= ETileNavCost::BLOCK)
					{
						Result.Path->SetIsPartial(true);

						// A path should have at least two points to be valid.
						if (Result.Path->GetPathPoints().Num() < 2)
						{
							Result.Path->GetPathPoints().Add(FNavPathPoint(PrevNodeLocation));
						}

						break;
					}

					FVector PathNodeLocation = NavMesh->NodeRefToLocation(PathNode.NodeRef);
					Result.Path->GetPathPoints().Add(FNavPathPoint(PathNodeLocation));

					TraversalCost = PathNode.TraversalCost;
					PrevNodeLocation = PathNodeLocation;
				}

				if (NavMesh->bDrawDebugShapes)
				{
					Self->DrawDebugPath(Result.Path.Get(), FColor::Red, nullptr, false, 0.1f);
				}

				// Mark the path as ready.
				Result.Path->MarkReady();
				break;
			}
			}
		}
	}

	if (NavMesh->bShowDebugText)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green, FString::Printf(TEXT("FindPath cost: %f"), TraversalCost));
	}

	return Result;
}

bool AAmGridNavMesh::TestPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query, int32* NumVisitedNodes)
{
	SCOPE_CYCLE_COUNTER(STAT_Grid_Navigation_Pathfinding);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Pathfinding);

	const ANavigationData* Self = Query.NavData.Get();
	check(Cast<const AAmGridNavMesh>(Self));

	const AAmGridNavMesh* NavMesh = (const AAmGridNavMesh*) Self;
	if (Self == NULL)
	{
		return false;
	}

	bool bPathExists = true;

	FVector StartLocation = FAmUtils::RoundToUnitCenter(Query.StartLocation);
	FVector EndLocation = FAmUtils::RoundToUnitCenter(Query.EndLocation);

	FNodeRef StartNodeRef = NavMesh->LocationToNodeRef(StartLocation);
	FNodeRef EndNodeRef = NavMesh->LocationToNodeRef(EndLocation);

	const FNavigationQueryFilter* NavFilter = Query.QueryFilter.Get();
	if (NavFilter && NavFilter->GetImplementation())
	{
		const FVector AdjustedEndLocation = NavFilter->GetAdjustedEndLocation(Query.EndLocation);
		if ((Query.StartLocation - AdjustedEndLocation).IsNearlyZero() == false)
		{
			FGridGraphAStar Pathfinder(*NavMesh);
			const auto* GridQueryFilter = static_cast<const FAmGridQueryFilter*>(NavFilter->GetImplementation());
			GridQueryFilter->SetSpeedMultiplier(GetSpeedMultiplier(Cast<const AActor>(Query.Owner)));
			FResultPathNodes PathNodes;
			EGraphAStarResult AStarResult = Pathfinder.FindPath(StartNodeRef, EndNodeRef, *GridQueryFilter, PathNodes);

			switch (AStarResult)
			{
			case SearchSuccess:
			{
				for (const FNodeDescription& PathNode : PathNodes)
				{
					if (NumVisitedNodes)
					{
						NumVisitedNodes[PathNode.NodeRef]++;
					}

					// If the path is blocked by a breakable block, bomb or something more dangerous, mark this path as non-existent.
					if (PathNode.TraversalCost >= ETileNavCost::BLOCK)
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

	if (NavMesh->bShowDebugText)
	{
		GEngine->AddOnScreenDebugMessage(-2, 0.5f, FColor::Green, FString::Printf(TEXT("TestPath exists: %d"), bPathExists));
	}

	return bPathExists;
}

void AAmGridNavMesh::BatchProjectPoints(TArray<FNavigationProjectionWork>& Workload, const FVector& Extent, FSharedConstNavQueryFilter Filter, const UObject* Querier) const
{
	if (Workload.Num() == 0)
	{
		return;
	}

	const FNavigationQueryFilter& FilterToUse = GetRightFilterRef(Filter);

	const FAmGridQueryFilter* QueryFilter = static_cast<const FAmGridQueryFilter*>(FilterToUse.GetImplementation());

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

bool AAmGridNavMesh::IsValidRef(FNodeRef NodeRef) const
{
	int32 X = NodeRef % Columns;
	int32 Y = NodeRef / Columns;
	return X != 0 && Y != 0 && (Y % 2 == 1 || X % 2 == 1) && NodeRef >= 0 && NodeRef < TileCosts.Num();
}

FNodeRef AAmGridNavMesh::GetNeighbour(const FSearchNode& SearchNodeRef, const int32 NeighbourIndex) const
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

int32 AAmGridNavMesh::GetNeighbourCount(FNodeRef NodeRef) const
{
	return 4;
}

int64 AAmGridNavMesh::GetTileCost(FVector Location) const
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

void AAmGridNavMesh::SetTileCost(FVector Location, int64 Cost)
{
	FNodeRef NodeRef = LocationToNodeRef(Location);
	if (TileCosts.IsValidIndex(NodeRef))
	{
		TileCosts[NodeRef] = Cost;
	}
}

float AAmGridNavMesh::GetTileTimeout(FVector Location) const
{
	FNodeRef NodeRef = LocationToNodeRef(Location);
	if (TileTimeouts.IsValidIndex(NodeRef))
	{
		return TileTimeouts[NodeRef];
	}
	else
	{
		return TIMEOUT_UNSET;
	}
}

void AAmGridNavMesh::SetTileTimeout(FVector Location, float Timeout)
{
	FNodeRef NodeRef = LocationToNodeRef(Location);
	if (TileTimeouts.IsValidIndex(NodeRef))
	{
		TileTimeouts[NodeRef] = Timeout;
	}
}

bool AAmGridNavMesh::IsTileDangerous(FVector Location, float TimeBeforeTileMin, float TimeAfterTileMax) const
{
	bool bIsDangerous = false;

	float TileTimeout = GetTileTimeout(Location);

	if (TileTimeout != TIMEOUT_UNSET)
	{
		// Check if tile explodes while we run through it.
		if (TimeBeforeTileMin <= TileTimeout && TileTimeout <= TimeAfterTileMax)
		{
			bIsDangerous = true;
		}
	}

	return bIsDangerous;
}

bool AAmGridNavMesh::IsPathSafe(AController* Controller, const TArray<FVector>& PathPoints) const
{
	FVector CharacterLocation = Controller->GetPawn()->GetActorLocation();

	if (bShowDebugText)
	{
		GEngine->AddOnScreenDebugMessage(-5, 0.1f, FColor::Yellow, FString::Printf(TEXT("Is Path Safe: %f, %f"), CharacterLocation.X, CharacterLocation.Y));
	}

	if (PathPoints.IsEmpty())
	{
		if (bShowDebugText)
		{
			GEngine->AddOnScreenDebugMessage(-15, 0.1f, FColor::Red, FString::Printf(TEXT("Path is empty: %f, %f"), CharacterLocation.X, CharacterLocation.Y));
		}

		return true;
	}

	bool bIsSafe = true;

	FAmGridQueryFilter QueryFilter(this, GetSpeedMultiplier(Controller), bDrawDebugShapes);
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

			if (bShowDebugText)
			{
				GEngine->AddOnScreenDebugMessage(-10, 0.1f, FColor::Yellow, FString::Printf(TEXT("Found current tile: %f, %f"), StartLocation.X, StartLocation.Y));
			}
		}

		if (!bFoundCurrentTile)
		{
			continue;
		}

		if (bShowDebugText)
		{
			GEngine->AddOnScreenDebugMessage(-100 - Index, 0.1f, FColor::Green, FString::Printf(TEXT("Found safe path tile: %f, %f"), StartLocation.X, StartLocation.Y));
		}

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

FVector AAmGridNavMesh::FindNearestCharacter(AController* Controller) const
{
	TSet<FNodeRef> CharacterNodeRefs;

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, AAmMainPlayerCharacter::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		if (Controller->GetPawn() != Actor)
		{
			FVector ActorLocation = FAmUtils::RoundToUnitCenter(Actor->GetActorLocation());
			FNodeRef ActorNodeRef = LocationToNodeRef(ActorLocation);
			CharacterNodeRefs.Add(ActorNodeRef);
		}
	}

	FNodeDescription StartNode{ LocationToNodeRef(Controller->GetPawn()->GetActorLocation()), 0 };

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

	BFS(Controller, StartNode, CheckNearest, ETileNavCost::BLOCK);

	FVector CharacterLocation = NodeRefToLocation(CharacterNodeRef);
	CharacterLocation.Z = GetActorLocation().Z;
	return CharacterLocation;
}

bool AAmGridNavMesh::IsCharacterNearby(AController* Controller, int64 RadiusTiles) const
{
	TSet<FNodeRef> CharacterNodeRefs;

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, AAmMainPlayerCharacter::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		if (Controller->GetPawn() != Actor)
		{
			FVector ActorLocation = FAmUtils::RoundToUnitCenter(Actor->GetActorLocation());
			FNodeRef ActorNodeRef = LocationToNodeRef(ActorLocation);
			CharacterNodeRefs.Add(ActorNodeRef);
		}
	}

	FNodeDescription StartNode{ LocationToNodeRef(Controller->GetPawn()->GetActorLocation()), 0 };

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

	BFS(Controller, StartNode, CheckNearest, ETileNavCost::DEFAULT);

	return IsNearby;
}

void AAmGridNavMesh::BFS(AController* Controller, const FNodeDescription& StartNode, std::function<bool(const FNodeDescription& CurrentNode)> NodeRefFunc, ETileNavCost::Type MaxTileNavCostAllowed) const
{
	if (!IsValidRef(StartNode.NodeRef))
	{
		UE_LOG(LogGame, Error, TEXT("Node Ref is not valid, Rows/Columns properties are probably incorrect!"));
		return;
	}

	FAmGridQueryFilter QueryFilter(this, GetSpeedMultiplier(Controller), bDrawDebugShapes);

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

FVector AAmGridNavMesh::NodeRefToLocation(FNodeRef NodeRef) const
{
	FVector NodeLocation;
	NodeLocation.X = (NodeRef % Columns) * FAmUtils::Unit + FAmUtils::Unit / 2;
	NodeLocation.Y = (NodeRef / Columns) * FAmUtils::Unit + FAmUtils::Unit / 2;
	NodeLocation.Z = GetActorLocation().Z;
	return NodeLocation;
}

FNodeRef AAmGridNavMesh::LocationToNodeRef(FVector Location) const
{
	FIntVector LocationIndex;
	LocationIndex.X = Location.X / FAmUtils::Unit;
	LocationIndex.Y = Location.Y / FAmUtils::Unit;

	return LocationIndex.Y * Columns + LocationIndex.X;
}

void AAmGridNavMesh::ResetTiles()
{
	for (FNodeRef NodeRef = 0; NodeRef < TileCosts.Num(); NodeRef++)
	{
		TileCosts[NodeRef] = 1;
	}

	for (FNodeRef NodeRef = 0; NodeRef < TileTimeouts.Num(); NodeRef++)
	{
		TileTimeouts[NodeRef] = TIMEOUT_UNSET;
	}
}

void AAmGridNavMesh::GetReachableTiles(AController* Controller, TArray<float>& OutCosts, bool bAddMovementDelay) const
{
	OutCosts.Init(TNumericLimits<float>::Max(), Columns * Rows);

	int64 StartCost = bAddMovementDelay ? ETileNavCost::DEFAULT : 0;
	FNodeDescription StartNode{ LocationToNodeRef(Controller->GetPawn()->GetActorLocation()), StartCost };

	auto SetOutCost = [&OutCosts](const FNodeDescription& CurrentNode) -> bool
	{
		OutCosts[CurrentNode.NodeRef] = CurrentNode.TraversalCost;
		return true;
	};

	BFS(Controller, StartNode, SetOutCost, ETileNavCost::DEFAULT);
}
