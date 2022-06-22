// Fill out your copyright notice in the Description page of Project Settings.

#include "Bomb.h"
#include <Utils.h>
#include <Explosion.h>
#include <PlayerCharacter.h>
#include <Components/BoxComponent.h>

// Sets default values
ABomb::ABomb()
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
	USkeletalMesh* BombMesh = ConstructorHelpers::FObjectFinder<USkeletalMesh>(TEXT("SkeletalMesh'/Engine/EngineMeshes/SkeletalCube'")).Object;
	MeshComponent->SetSkeletalMesh(BombMesh);

	MeshComponent->SetGenerateOverlapEvents(true);

	MeshComponent->SetupAttachment(RootComponent);

	OverlapComponent->OnComponentEndOverlap.AddDynamic(this, &ABomb::HandleOverlapEnd);

	LifeSpan = 3.f;
	RadiusBlocks = 2;
}

// Called when the game starts or when spawned
void ABomb::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(LifeSpan);

	if (IdleAnimation)
	{
		MeshComponent->PlayAnimation(IdleAnimation, true);
	}

	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, APlayerCharacter::StaticClass());

	// loop through TArray
	for (AActor* OverlappingActor : OverlappingActors)
	{
		APlayerCharacter* Character = Cast<APlayerCharacter>(OverlappingActor);
		if (Character)
		{
			OverlapComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		}
	}
}

void ABomb::HandleOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacter* Character = Cast<APlayerCharacter>(OtherActor);
	if (Character)
	{
		OverlapComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	}
}

void ABomb::LifeSpanExpired()
{
	BlowUp();
	Destroy();
}

void ABomb::BlowUp()
{
	if (ExplosionClass == nullptr)
	{
		UE_LOG(LogGame, Error, TEXT("ExplosionClass property is not set!"));
		return;
	}

	if (!HasAuthority())
	{
		return;
	}

	struct
	{
		uint32 Right;
		uint32 Left;
		uint32 Up;
		uint32 Down;
	} ExplosionConstraints{
		RadiusBlocks,
		RadiusBlocks,
		RadiusBlocks,
		RadiusBlocks
	};

	FVector Start = GetActorLocation();

	// Right
	{
		TArray<FHitResult> OutHits{};
		FVector End = GetActorLocation();
		End.X = Utils::RoundUnitCenter(End.X) + Utils::Unit * RadiusBlocks;
		GetWorld()->LineTraceMultiByChannel(OutHits, Start, End, ECollisionChannel::ECC_GameExplosion);

		for (const FHitResult& HitResult : OutHits)
		{
			if (!HitResult.bBlockingHit)
			{
				AActor* Actor = HitResult.GetActor();
				APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Actor);
				if (PlayerCharacter == nullptr && IsValid(Actor))
				{
					Actor->Destroy();
				}
			}
			else
			{
				float BlockingDistance = HitResult.Distance;
				ExplosionConstraints.Right = FMath::RoundToZero(BlockingDistance / Utils::Unit);
			}
		}
	}

	// Left
	{
		TArray<FHitResult> OutHits{};
		FVector End = GetActorLocation();
		End.X = Utils::RoundUnitCenter(End.X) - Utils::Unit * RadiusBlocks;
		GetWorld()->LineTraceMultiByChannel(OutHits, Start, End, ECollisionChannel::ECC_GameExplosion);

		for (const FHitResult& HitResult : OutHits)
		{
			if (!HitResult.bBlockingHit)
			{
				AActor* Actor = HitResult.GetActor();
				APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Actor);
				if (PlayerCharacter == nullptr && IsValid(Actor))
				{
					Actor->Destroy();
				}
			}
			else
			{
				float BlockingDistance = HitResult.Distance;
				ExplosionConstraints.Left = FMath::RoundToZero(BlockingDistance / Utils::Unit);
			}
		}
	}

	// Up
	{
		TArray<FHitResult> OutHits{};
		FVector End = GetActorLocation();
		End.Y = Utils::RoundUnitCenter(End.Y) - Utils::Unit * RadiusBlocks;
		GetWorld()->LineTraceMultiByChannel(OutHits, Start, End, ECollisionChannel::ECC_GameExplosion);

		for (const FHitResult& HitResult : OutHits)
		{
			if (!HitResult.bBlockingHit)
			{
				AActor* Actor = HitResult.GetActor();
				APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Actor);
				if (PlayerCharacter == nullptr && IsValid(Actor))
				{
					Actor->Destroy();
				}
			}
			else
			{
				float BlockingDistance = HitResult.Distance;
				ExplosionConstraints.Up = FMath::RoundToZero(BlockingDistance / Utils::Unit);
			}
		}
	}

	// Down
	{
		TArray<FHitResult> OutHits{};
		FVector End = GetActorLocation();
		End.Y = Utils::RoundUnitCenter(End.Y) + Utils::Unit * RadiusBlocks;
		GetWorld()->LineTraceMultiByChannel(OutHits, Start, End, ECollisionChannel::ECC_GameExplosion);

		for (const FHitResult& HitResult : OutHits)
		{
			if (!HitResult.bBlockingHit)
			{
				AActor* Actor = HitResult.GetActor();
				APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Actor);
				if (PlayerCharacter == nullptr && IsValid(Actor))
				{
					Actor->Destroy();
				}
			}
			else
			{
				float BlockingDistance = HitResult.Distance;
				ExplosionConstraints.Down = FMath::RoundToZero(BlockingDistance / Utils::Unit);
			}
		}
	}

	FRotator Rotation = FRotator(0.f);
	FTransform Transform;
	Transform.SetLocation(Start);
	Transform.SetRotation(Rotation.Quaternion());
	FActorSpawnParameters SpawnParameters;
	GetWorld()->SpawnActorAbsolute<AExplosion>(ExplosionClass, Transform, SpawnParameters);

	for (uint8 Index = 1; Index <= ExplosionConstraints.Right; Index++)
	{
		// Right
		{
			FVector Location = GetActorLocation();
			Location.X = Utils::RoundUnitCenter(Location.X) + Utils::Unit * Index;
			Location.Y = Utils::RoundUnitCenter(Location.Y);
			Transform.SetLocation(Location);
			Transform.SetRotation(Rotation.Quaternion());
			GetWorld()->SpawnActorAbsolute<AExplosion>(ExplosionClass, Transform, SpawnParameters);
		}
	}

	for (uint8 Index = 1; Index <= ExplosionConstraints.Left; Index++)
	{
		// Left
		{
			FVector Location = GetActorLocation();
			Location.X = Utils::RoundUnitCenter(Location.X) - Utils::Unit * Index;
			Location.Y = Utils::RoundUnitCenter(Location.Y);
			Transform.SetLocation(Location);
			Transform.SetRotation(Rotation.Quaternion());
			GetWorld()->SpawnActorAbsolute<AExplosion>(ExplosionClass, Transform, SpawnParameters);
		}
	}

	for (uint8 Index = 1; Index <= ExplosionConstraints.Up; Index++)
	{
		// Up
		{
			FVector Location = GetActorLocation();
			Location.X = Utils::RoundUnitCenter(Location.X);
			Location.Y = Utils::RoundUnitCenter(Location.Y) - Utils::Unit * Index;
			Transform.SetLocation(Location);
			Transform.SetRotation(Rotation.Quaternion());
			GetWorld()->SpawnActorAbsolute<AExplosion>(ExplosionClass, Transform, SpawnParameters);
		}
	}

	for (uint8 Index = 1; Index <= ExplosionConstraints.Down; Index++)
	{
		// Down
		{
			FVector Location = GetActorLocation();
			Location.X = Utils::RoundUnitCenter(Location.X);
			Location.Y = Utils::RoundUnitCenter(Location.Y) + Utils::Unit * Index;
			Transform.SetLocation(Location);
			Transform.SetRotation(Rotation.Quaternion());
			GetWorld()->SpawnActorAbsolute<AExplosion>(ExplosionClass, Transform, SpawnParameters);
		}
	}
}
