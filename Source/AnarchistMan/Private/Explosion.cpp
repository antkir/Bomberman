// Fill out your copyright notice in the Description page of Project Settings.

#include "Explosion.h"
#include <PlayerCharacter.h>
#include <Utils.h>
#include <Components/BoxComponent.h>

// Sets default values
AExplosion::AExplosion()
{
	bReplicates = true;

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create an overlap component
	OverlapComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapComponent"));

	// Set as root component
	RootComponent = OverlapComponent;

	// Create a mesh component
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));

	// Set the component's mesh
	USkeletalMesh* ExplosionMesh = ConstructorHelpers::FObjectFinder<USkeletalMesh>(TEXT("SkeletalMesh'/Engine/EngineMeshes/SkeletalCube'")).Object;
	MeshComponent->SetSkeletalMesh(ExplosionMesh);

	MeshComponent->SetupAttachment(RootComponent);

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

	if (HasAuthority())
	{
		OverlapComponent->OnComponentBeginOverlap.AddDynamic(this, &AExplosion::HandleBeginOverlap);
	}
}

void AExplosion::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerCharacter* Character = Cast<APlayerCharacter>(OtherActor);
	if (Character)
	{
		Character->BlowUp();
	}
}
