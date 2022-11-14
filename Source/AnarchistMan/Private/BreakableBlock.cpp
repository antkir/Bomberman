// Fill out your copyright notice in the Description page of Project Settings.

#include "BreakableBlock.h"

#include <Kismet/GameplayStatics.h>

#include <GridNavMesh.h>
#include <Utils.h>

ABreakableBlock::ABreakableBlock()
{
	bReplicates = true;

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a static mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));

	// Set as root component
	RootComponent = MeshComponent;
}

void ABreakableBlock::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        auto* GridNavMesh = Cast<AGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AGridNavMesh::StaticClass()));
        FVector Location = GetActorLocation();
        auto Cost = FMath::Max<int64>(GridNavMesh->GetTileCost(Location), ETileNavCost::BLOCK);
        GridNavMesh->SetTileCost(Location, Cost);
    }
}

bool ABreakableBlock::IsBlockingExplosion_Implementation()
{
    return true;
}

void ABreakableBlock::BlowUp_Implementation()
{
    check(HasAuthority());

    auto* GridNavMesh = Cast<AGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AGridNavMesh::StaticClass()));
    if (GridNavMesh)
    {
        FVector Location = GetActorLocation();
        GridNavMesh->SetTileCost(Location, 1);
    }

    Destroy();
}
