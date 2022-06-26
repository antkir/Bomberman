// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"
#include <Utils.h>
#include <Bomb.h>
#include <Explosion.h>
#include <AnarchistMan/AnarchistManGameModeBase.h>
#include <AnarchistMan/Private/AnarchistManPlayerState.h>
#include <Camera/CameraComponent.h>
#include <Components/CapsuleComponent.h>
#include <GameFramework/CharacterMovementComponent.h>

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set camera component
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(GetRootComponent());
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind movement events
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &APlayerCharacter::MoveVertical);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &APlayerCharacter::MoveHorizontal);

	// Bind actions
	PlayerInputComponent->BindAction("Place Bomb", IE_Pressed, this, &APlayerCharacter::PlaceBomb);
}

void APlayerCharacter::BlowUp()
{
	BlowUp_Private();
	SetActorEnableCollision(false);
	GetMesh()->SetVisibility(false);
	GetCapsuleComponent()->SetVisibility(false);
	DisableInput(nullptr);
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APlayerCharacter::MoveVertical(float Value)
{
	if (Value != 0.f) {
		float Rotation = Value > 0.f ? -90.f : 90.f;
		Controller->SetControlRotation(FRotator(0.f, Rotation, 0.f));

		// add movement in that direction
		AddMovementInput(FVector::YAxisVector, Value * -1.f);
	}
}

void APlayerCharacter::MoveHorizontal(float Value)
{
	if (Value != 0.f) {
		float Rotation = Value > 0.f ? 0.f : 180.f;
		Controller->SetControlRotation(FRotator(0.f, Rotation, 0.f));
		
		// add movement in that direction
		AddMovementInput(FVector::XAxisVector, Value);
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

void APlayerCharacter::BlowUp_Private()
{
	AAnarchistManGameModeBase* GameMode = Cast<AAnarchistManGameModeBase>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		GameMode->PlayerDeath(GetController());
	}
}
