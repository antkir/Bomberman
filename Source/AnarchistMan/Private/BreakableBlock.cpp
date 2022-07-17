// Fill out your copyright notice in the Description page of Project Settings.

#include "BreakableBlock.h"

#include <Utils.h>

// Sets default values
ABreakableBlock::ABreakableBlock()
{
	bReplicates = true;

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a static mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));

	// Set the component's mesh
	UStaticMesh* CubeMesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/EngineMeshes/Cube'")).Object;
	MeshComponent->SetStaticMesh(CubeMesh);

	// Set as root component
	RootComponent = MeshComponent;
}

void ABreakableBlock::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABreakableBlock* Bomb = Cast<ABreakableBlock>(OtherActor);
	if (Bomb)
	{
		Destroy();
	}
}
