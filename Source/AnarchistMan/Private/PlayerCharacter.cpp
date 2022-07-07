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
#include <Blueprint/UserWidget.h>

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set camera component
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(GetRootComponent());

	CameraComponent->SetUsingAbsoluteLocation(true);
	CameraComponent->SetUsingAbsoluteRotation(true);
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind movement events
	PlayerInputComponent->BindAxis("Move Up / Down", this, &APlayerCharacter::MoveVertical);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &APlayerCharacter::MoveHorizontal);

	// Bind actions
	PlayerInputComponent->BindAction("Place Bomb", IE_Pressed, this, &APlayerCharacter::PlaceBomb);
	PlayerInputComponent->BindAction("Game Menu", IE_Pressed, this, &APlayerCharacter::ToggleGameMenu);
}

void APlayerCharacter::BlowUp()
{
	AController* PlayerController = GetController();

	SetActorEnableCollision(false);
	GetMesh()->SetVisibility(false);
	DisableInput(nullptr);

	AAnarchistManGameModeBase* GameMode = Cast<AAnarchistManGameModeBase>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		GameMode->PlayerDeath(PlayerController);
	}
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector CameraLocation = GetActorLocation();
	CameraLocation.X += CameraLocationOffset.X;
	CameraLocation.Y += CameraLocationOffset.Y;
	CameraLocation.Z += CameraLocationOffset.Z;
	CameraComponent->SetWorldLocation(CameraLocation);
}

void APlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	check(GetPlayerState());

	uint32 PlayerId = GetPlayerState()->GetPlayerId() % GetNum(Utils::PawnECCs);
	GetCapsuleComponent()->SetCollisionObjectType(Utils::PawnECCs[PlayerId]);
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	check(GetPlayerState());

	uint32 PlayerId = GetPlayerState()->GetPlayerId() % GetNum(Utils::PawnECCs);
	GetCapsuleComponent()->SetCollisionObjectType(Utils::PawnECCs[PlayerId]);
}

void APlayerCharacter::MoveVertical(float Value)
{
	if (Value != 0.f) {
		float Rotation = Value > 0.f ? 90.f : -90.f;
		Controller->SetControlRotation(FRotator(0.f, Rotation, 0.f));

		// add movement in that direction
		AddMovementInput(FVector::YAxisVector, Value);
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

void APlayerCharacter::ToggleGameMenu()
{
	if (GameMenuWidgetClass)
	{
		APlayerController* PlayerController = Cast<APlayerController>(GetController());

		if (!bGameMenuOpen)
		{
			GameMenuWidget = CreateWidget<UUserWidget>(PlayerController, GameMenuWidgetClass);
			if (GameMenuWidget)
			{
				GameMenuWidget->AddToViewport();
			}

			PlayerController->SetShowMouseCursor(true);

			bGameMenuOpen = true;
		}
		else
		{
			if (GameMenuWidget)
			{
				GameMenuWidget->RemoveFromViewport();
			}

			PlayerController->SetShowMouseCursor(false);

			bGameMenuOpen = false;
		}
	}
	else
	{
		UE_LOG(LogGame, Error, TEXT("GameOverWidgetClass property is not set!"));
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
	FTransform Transform;
	Transform.SetLocation(Location);
	Transform.SetRotation(FQuat::Identity);
	FActorSpawnParameters SpawnParameters;
	GetWorld()->SpawnActorAbsolute<ABomb>(BombClass, Transform, SpawnParameters);
}
