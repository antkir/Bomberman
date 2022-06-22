// Fill out your copyright notice in the Description page of Project Settings.

#include "Explosion.h"
#include <Utils.h>

// Sets default values
AExplosion::AExplosion()
{
	bReplicates = true;

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a mesh component
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));

	// Set the component's mesh
	USkeletalMesh* ExplosionMesh = ConstructorHelpers::FObjectFinder<USkeletalMesh>(TEXT("SkeletalMesh'/Engine/EngineMeshes/SkeletalCube'")).Object;
	MeshComponent->SetSkeletalMesh(ExplosionMesh);

	// Set as root component
	RootComponent = MeshComponent;

	LifeSpan = 1.f;
}

// Called when the game starts or when spawned
void AExplosion::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(LifeSpan);

	if (IdleAnimation)
	{
		MeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		MeshComponent->SetAnimation(IdleAnimation);
		MeshComponent->Play(true);
	}
}
