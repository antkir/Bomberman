// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"
#include <Utils.h>
#include <Bomb.h>
#include <Camera/CameraComponent.h>
#include <Components/CapsuleComponent.h>

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set camera component
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(GetRootComponent());

	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind movement events
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &APlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &APlayerCharacter::MoveRight);

	// Bind actions
	PlayerInputComponent->BindAction("Place Bomb", IE_Pressed, this, &APlayerCharacter::PlaceBomb);
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerCharacter::MoveForward(float Value)
{
	if (Value != 0.f) {
		if (Value > 0.f)
		{
			GetMesh()->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
		} else
		{
			GetMesh()->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
		}

		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value * -1.f);
	}
}

void APlayerCharacter::MoveRight(float Value)
{
	if (Value != 0.f) {
		if (Value > 0.f)
		{
			GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		}
		else
		{
			GetMesh()->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		}
		
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void APlayerCharacter::PlaceBomb_Implementation()
{
	if (BombClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("BombClass property is not set!"));
		return;
	}

	FVector Location = GetActorLocation();
	Location.X = Utils::RoundUnitCenter(Location.X);
	Location.Y = Utils::RoundUnitCenter(Location.Y);
	Location.Z -= GetCapsuleComponent()->Bounds.BoxExtent.Z;
	FRotator Rotation = FRotator(0.f);
	FTransform Transform;
	Transform.SetLocation(Location);
	Transform.SetRotation(Rotation.Quaternion());
	FActorSpawnParameters SpawnParameters;
	GetWorld()->SpawnActorAbsolute<ABomb>(BombClass, Transform, SpawnParameters);
}
