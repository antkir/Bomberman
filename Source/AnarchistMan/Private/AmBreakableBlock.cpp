// Fill out your copyright notice in the Description page of Project Settings.

#include "AmBreakableBlock.h"

#include <Kismet/GameplayStatics.h>

#include <AmGridNavMesh.h>
#include <AmUtils.h>

AAmBreakableBlock::AAmBreakableBlock()
{
	bReplicates = true;

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a static mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));

	// Set as root component
	RootComponent = MeshComponent;
}

void AAmBreakableBlock::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        auto* GridNavMesh = Cast<AAmGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AAmGridNavMesh::StaticClass()));
        if (GridNavMesh)
        {
            FVector Location = GetActorLocation();
            auto Cost = FMath::Max<int64>(GridNavMesh->GetTileCost(Location), ETileNavCost::BLOCK);
            GridNavMesh->SetTileCost(Location, Cost);
        }
        else
        {
            UE_LOG(LogGame, Error, TEXT("AmGridNavMesh instance must be present in this level!"));
        }
    }
}

bool AAmBreakableBlock::IsBlockingExplosion_Implementation()
{
    return true;
}

void AAmBreakableBlock::BlowUp_Implementation()
{
    check(HasAuthority());

    auto* GridNavMesh = Cast<AAmGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AAmGridNavMesh::StaticClass()));
    if (GridNavMesh)
    {
        FVector Location = GetActorLocation();
        GridNavMesh->SetTileCost(Location, 1);
    }

    Destroy();
}
