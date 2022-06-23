// Fill out your copyright notice in the Description page of Project Settings.

#include "Bomb.h"
#include <Utils.h>
#include <BreakableBlock.h>
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

	OverlapComponent->OnComponentEndOverlap.AddDynamic(this, &ABomb::HandleEndOverlap);

	LifeSpan = 3.f;
	RadiusBlocks = 2;

	ExplosionConstraints = {
		RadiusBlocks,
		RadiusBlocks,
		RadiusBlocks,
		RadiusBlocks
	};

	ExplosionTriggered = false;
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

	for (AActor* OverlappingActor : OverlappingActors)
	{
		APlayerCharacter* Character = Cast<APlayerCharacter>(OverlappingActor);
		if (Character)
		{
			OverlapComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		}
	}
}

void ABomb::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacter* Character = Cast<APlayerCharacter>(OtherActor);
	if (Character)
	{
		// Start blocking players as soon as they stop overlapping a bomb
		TArray<FOverlapResult> OutOverlaps{};
		FVector Location = GetActorLocation();
		FRotator Rotation = FRotator(0.f);
		FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(Utils::Unit / 2));
		FCollisionObjectQueryParams QueryParams = FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn);
		bool PawnOverlap = GetWorld()->OverlapAnyTestByObjectType(Location, Rotation.Quaternion(), QueryParams, CollisionShape);
		if (!PawnOverlap)
		{
			OverlapComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
		}
	}
}

void ABomb::LifeSpanExpired()
{
	BlowUp();
	Destroy();
}

void ABomb::BlowUp()
{
	if (!HasAuthority())
	{
		return;
	}

	if (ExplosionClass == nullptr)
	{
		UE_LOG(LogGame, Error, TEXT("ExplosionClass property is not set!"));
		return;
	}

	ExplosionTriggered = true;

	// Right
	{
		FVector Start = GetActorLocation();
		FVector End = GetActorLocation();
		End.X = Utils::RoundUnitCenter(End.X) + Utils::Unit * RadiusBlocks;

		uint64 Constraint = LineTraceExplosion(Start, End);
		ExplosionConstraints.Right = std::min(ExplosionConstraints.Down, Constraint);
	}

	// Left
	{
		FVector Start = GetActorLocation();
		FVector End = GetActorLocation();
		End.X = Utils::RoundUnitCenter(End.X) - Utils::Unit * RadiusBlocks;

		uint64 Constraint = LineTraceExplosion(Start, End);
		ExplosionConstraints.Left = std::min(ExplosionConstraints.Down, Constraint);
	}

	// Up
	{
		FVector Start = GetActorLocation();
		FVector End = GetActorLocation();
		End.Y = Utils::RoundUnitCenter(End.Y) - Utils::Unit * RadiusBlocks;

		uint64 Constraint = LineTraceExplosion(Start, End);
		ExplosionConstraints.Up = std::min(ExplosionConstraints.Down, Constraint);
	}

	// Down
	{
		FVector Start = GetActorLocation();
		FVector End = GetActorLocation();
		End.Y = Utils::RoundUnitCenter(End.Y) + Utils::Unit * RadiusBlocks;

		uint64 Constraint = LineTraceExplosion(Start, End);
		ExplosionConstraints.Down = std::min(ExplosionConstraints.Down, Constraint);
	}

	{
		FVector Location = GetActorLocation();
		FRotator Rotation = FRotator(0.f);
		FTransform Transform;
		Transform.SetLocation(GetActorLocation());
		Transform.SetRotation(Rotation.Quaternion());
		GetWorld()->SpawnActorAbsolute(ExplosionClass, Transform);
	}

	for (uint8 Index = 1; Index <= ExplosionConstraints.Right; Index++)
	{
		// Right
		FVector Location = GetActorLocation();
		FRotator Rotation = FRotator(0.f);
		Location.X = Utils::RoundUnitCenter(Location.X) + Utils::Unit * Index;
		Location.Y = Utils::RoundUnitCenter(Location.Y);
		FTransform Transform;
		Transform.SetLocation(Location);
		Transform.SetRotation(Rotation.Quaternion());
		GetWorld()->SpawnActorAbsolute(ExplosionClass, Transform);
	}

	for (uint8 Index = 1; Index <= ExplosionConstraints.Left; Index++)
	{
		// Left
		FVector Location = GetActorLocation();
		FRotator Rotation = FRotator(0.f);
		Location.X = Utils::RoundUnitCenter(Location.X) - Utils::Unit * Index;
		Location.Y = Utils::RoundUnitCenter(Location.Y);
		FTransform Transform;
		Transform.SetLocation(Location);
		Transform.SetRotation(Rotation.Quaternion());
		GetWorld()->SpawnActorAbsolute(ExplosionClass, Transform);
	}

	for (uint8 Index = 1; Index <= ExplosionConstraints.Up; Index++)
	{
		// Up
		FVector Location = GetActorLocation();
		FRotator Rotation = FRotator(0.f);
		Location.X = Utils::RoundUnitCenter(Location.X);
		Location.Y = Utils::RoundUnitCenter(Location.Y) - Utils::Unit * Index;
		FTransform Transform;
		Transform.SetLocation(Location);
		Transform.SetRotation(Rotation.Quaternion());
		GetWorld()->SpawnActorAbsolute<AExplosion>(ExplosionClass, Transform);
	}

	for (uint8 Index = 1; Index <= ExplosionConstraints.Down; Index++)
	{
		// Down
		{
			FVector Location = GetActorLocation();
			FRotator Rotation = FRotator(0.f);
			Location.X = Utils::RoundUnitCenter(Location.X);
			Location.Y = Utils::RoundUnitCenter(Location.Y) + Utils::Unit * Index;
			FTransform Transform;
			Transform.SetLocation(Location);
			Transform.SetRotation(Rotation.Quaternion());
			GetWorld()->SpawnActorAbsolute<AExplosion>(ExplosionClass, Transform);
		}
	}
}

uint32 ABomb::LineTraceExplosion(FVector Start, FVector End)
{
	uint32 BlockingDistance = std::numeric_limits<uint32>::max();

	TArray<FHitResult> OutHits{};
	GetWorld()->LineTraceMultiByChannel(OutHits, Start, End, ECollisionChannel::ECC_GameExplosion);

	for (const FHitResult& HitResult : OutHits)
	{
		if (!HitResult.bBlockingHit)
		{
			AActor* Actor = HitResult.GetActor();

			if (!IsValid(Actor))
			{
				continue;
			}

			if (Actor->IsA(ABomb::StaticClass()))
			{
				ABomb* Bomb = Cast<ABomb>(Actor);
				if (Bomb != this && !Bomb->ExplosionTriggered)
				{
					Bomb->BlowUp();
					Bomb->Destroy();

					float DistanceRounded = FMath::RoundToZero(HitResult.Distance / Utils::Unit);
					BlockingDistance = static_cast<uint32>(DistanceRounded);

					break;
				}
			}
			else if (Actor->IsA(ABreakableBlock::StaticClass()))
			{
				Actor->Destroy();

				float DistanceRounded = FMath::RoundToZero(HitResult.Distance / Utils::Unit);
				BlockingDistance = static_cast<uint32>(DistanceRounded) + 1;

				break;
			}
		}
		else
		{
			float DistanceRounded = FMath::RoundToZero(HitResult.Distance / Utils::Unit);
			BlockingDistance = static_cast<uint32>(DistanceRounded);
		}
	}

	return BlockingDistance;
}
