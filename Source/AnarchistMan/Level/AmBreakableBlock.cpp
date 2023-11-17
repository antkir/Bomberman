// Copyright 2022 Kiryl Antonik

#include "AmBreakableBlock.h"
#include "Kismet/GameplayStatics.h"
#include "AI/AmGridNavMesh.h"
#include "Game/AmUtils.h"

AAmBreakableBlock::AAmBreakableBlock()
{
	bReplicates = true;

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
