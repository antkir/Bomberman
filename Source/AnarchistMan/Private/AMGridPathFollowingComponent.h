// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"

#include "AMGridPathFollowingComponent.generated.h"

class AAStarNavMesh;
class AGridNavMesh;

/**
 * 
 */
UCLASS()
class UAMGridPathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()

public:

    virtual void BeginPlay() override;

    /**
     * Toggle our debug drawing.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GraphAStarExample|PathFollowingComponent")
    bool bDrawDebug;

    /**
     * The PathFollowingComponent has a pointer to the ANavigationData class but it isn't expose to Blueprint,
     * so we will expose it casted to our AGraphAStarNavMesh in the constructor.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GraphAStarExample|PathFollowingComponent")
    AGridNavMesh* NavMesh;

protected:

    /** follow current path segment */
    virtual void FollowPathSegment(float DeltaTime) override;
	
};
