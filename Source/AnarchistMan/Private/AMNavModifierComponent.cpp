// Fill out your copyright notice in the Description page of Project Settings.

#include "AMNavModifierComponent.h"

#include <Utils.h>

void UAMNavModifierComponent::CalcAndCacheBounds() const
{
    AActor* MyOwner = GetOwner();
    if (MyOwner)
    {
        CachedTransform = MyOwner->GetActorTransform();

        if (TransformUpdateHandle.IsValid() == false && MyOwner->GetRootComponent())
        {
            // binding to get notifies when the root component moves. We need
            // this only when the rootcomp is nav-irrelevant (since the default 
            // mechanisms won't kick in) but we're binding without checking it since
            // this property can change without re-running CalcAndCacheBounds.
            // We're filtering for nav relevancy in OnTransformUpdated.
            TransformUpdateHandle = MyOwner->GetRootComponent()->TransformUpdated.AddUObject(const_cast<UAMNavModifierComponent*>(this), &UAMNavModifierComponent::OnTransformUpdated);
        }

        Bounds = FBox(ForceInit);

        ComponentBounds.Reset();

        Bounds = FBox::BuildAABB(MyOwner->GetActorLocation(), FailsafeExtent);
        ComponentBounds.Add(FRotatedBox(Bounds, FQuat::Identity));

        for (int32 Idx = 0; Idx < ComponentBounds.Num(); Idx++)
        {
            const FVector BoxOrigin = ComponentBounds[Idx].Box.GetCenter();
            const FVector BoxExtent = ComponentBounds[Idx].Box.GetExtent();

            const FVector NavModBoxOrigin = FTransform(ComponentBounds[Idx].Quat).InverseTransformPosition(BoxOrigin);
            ComponentBounds[Idx].Box = FBox::BuildAABB(NavModBoxOrigin, BoxExtent);
        }

        LeafBounds.Reset();

        for (uint64 Index = 1; Index <= RadiusBlocks; Index++)
        {
            // Right
            {
                FVector Location = MyOwner->GetActorLocation();
                Location.X = Utils::RoundToUnitCenter(Location.X) + Utils::Unit * Index;
                Location.Y = Utils::RoundToUnitCenter(Location.Y);

                Bounds = FBox::BuildAABB(Location, FailsafeExtent);
                LeafBounds.Add(FRotatedBox(Bounds, FQuat::Identity));
            }

            // Left
            {
                FVector Location = MyOwner->GetActorLocation();
                Location.X = Utils::RoundToUnitCenter(Location.X) - Utils::Unit * Index;
                Location.Y = Utils::RoundToUnitCenter(Location.Y);

                Bounds = FBox::BuildAABB(Location, FailsafeExtent);
                LeafBounds.Add(FRotatedBox(Bounds, FQuat::Identity));
            }

            // Up
            {
                FVector Location = MyOwner->GetActorLocation();
                Location.X = Utils::RoundToUnitCenter(Location.X);
                Location.Y = Utils::RoundToUnitCenter(Location.Y) - Utils::Unit * Index;

                Bounds = FBox::BuildAABB(Location, FailsafeExtent);
                LeafBounds.Add(FRotatedBox(Bounds, FQuat::Identity));
            }

            // Down
            {
                FVector Location = MyOwner->GetActorLocation();
                Location.X = Utils::RoundToUnitCenter(Location.X);
                Location.Y = Utils::RoundToUnitCenter(Location.Y) + Utils::Unit * Index;

                Bounds = FBox::BuildAABB(Location, FailsafeExtent);
                LeafBounds.Add(FRotatedBox(Bounds, FQuat::Identity));
            }
        }

        for (int32 Idx = 0; Idx < LeafBounds.Num(); Idx++)
        {
            const FVector BoxOrigin = LeafBounds[Idx].Box.GetCenter();
            const FVector BoxExtent = LeafBounds[Idx].Box.GetExtent();

            const FVector NavModBoxOrigin = FTransform(LeafBounds[Idx].Quat).InverseTransformPosition(BoxOrigin);
            LeafBounds[Idx].Box = FBox::BuildAABB(NavModBoxOrigin, BoxExtent);
        }
    }
}

void UAMNavModifierComponent::GetNavigationData(FNavigationRelevantData& Data) const
{
    Super::GetNavigationData(Data);

    for (int32 Idx = 0; Idx < LeafBounds.Num(); Idx++)
    {
        Data.Modifiers.Add(FAreaNavModifier(LeafBounds[Idx].Box, FTransform(LeafBounds[Idx].Quat), LeafAreaClass).SetIncludeAgentHeight(bIncludeAgentHeight));
    }
}

void UAMNavModifierComponent::SetRadiusBlocks(uint8 Radius)
{
    RadiusBlocks = Radius;
}

void UAMNavModifierComponent::SetLeafAreaClass(TSubclassOf<UNavArea> NewLeafAreaClass)
{
    if (LeafAreaClass != NewLeafAreaClass)
    {
        AreaClass = NewLeafAreaClass;
        RefreshNavigationModifiers();
    }
}
