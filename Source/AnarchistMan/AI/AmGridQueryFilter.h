// Copyright 2022 Kiryl Antonik

#pragma once

#include "CoreMinimal.h"
#include "GraphAStar.h"
#include "Navigation/NavLocalGridData.h"
#include "Game/AmUtils.h"

class AAmGridNavMesh;

/**
 * TQueryFilter (FindPath's parameter) filter class is what decides which graph edges can be used and at what cost.
 * Use FRecastQueryFilter as a reference.
 */
class FAmGridQueryFilter : public INavigationQueryFilterInterface
{
	typedef FNavLocalGridData::FNodeRef FNodeRef;
	typedef FGraphAStarDefaultNode<AAmGridNavMesh> FSearchNode;

public:
	FAmGridQueryFilter(const AAmGridNavMesh* NavMesh, float SpeedMultiplier, bool bDrawDebug);

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

	/* FGraphAStar functions. */

	// Estimate of cost from StartNode to EndNode from a search node
	float GetHeuristicCost(const FSearchNode& StartNode, const FSearchNode& EndNode) const;

	// Real cost of traveling from StartNode directly to EndNode from a search node
	float GetTraversalCost(const FSearchNode& StartNode, const FSearchNode& EndNode) const;

	// Whether traversing given edge is allowed from a NodeRef
	bool IsTraversalAllowed(const FNodeRef NodeA, const FNodeRef NodeB) const;

	// Whether to accept solutions that do not reach the goal
	bool WantsPartialSolution() const;

	void SetSpeedMultiplier(float Multiplier) const;

private:

	const AAmGridNavMesh* GridNavMesh;

	mutable float SpeedMultiplier;

	bool bDrawDebugShapes;
};
