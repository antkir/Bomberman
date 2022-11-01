// Fill out your copyright notice in the Description page of Project Settings.


#include "AMGridPathFollowingComponent.h"

#include "GridNavMesh.h"

void UAMGridPathFollowingComponent::BeginPlay()
{
    Super::BeginPlay();

    NavMesh = Cast<AGridNavMesh>(MyNavData);
}

void UAMGridPathFollowingComponent::FollowPathSegment(float DeltaTime)
{
    Super::FollowPathSegment(DeltaTime);

    /**
     * FollowPathSegment is the main UE4 Path Follow tick function, and so when you want to add completely
     * custom coding you can use this function as your starting point to adjust normal UE4 path behavior!
     *
     * Let me show you a simple example with some debug drawings.
     */

    if (Path && bDrawDebug)
    {
        // Just draw the current path
        Path->DebugDraw(MyNavData, FColor::White, nullptr, false, 5.f);

        // Draw the start point of the current path segment we are traveling.
        FNavPathPoint CurrentPathPoint{};
        FNavigationPath::GetPathPoint(&Path->AsShared().Get(), GetCurrentPathIndex(), CurrentPathPoint);
        DrawDebugLine(GetWorld(), CurrentPathPoint.Location, CurrentPathPoint.Location + FVector(0.f, 0.f, 200.f), FColor::Blue);
        DrawDebugSphere(GetWorld(), CurrentPathPoint.Location + FVector(0.f, 0.f, 200.f), 25.f, 16, FColor::Blue);

        // Draw the end point of the current path segment we are traveling.
        FNavPathPoint NextPathPoint{};
        FNavigationPath::GetPathPoint(&Path->AsShared().Get(), GetNextPathIndex(), NextPathPoint);
        DrawDebugLine(GetWorld(), NextPathPoint.Location, NextPathPoint.Location + FVector(0.f, 0.f, 200.f), FColor::Green);
        DrawDebugSphere(GetWorld(), NextPathPoint.Location + FVector(0.f, 0.f, 200.f), 25.f, 16, FColor::Green);
    }
}