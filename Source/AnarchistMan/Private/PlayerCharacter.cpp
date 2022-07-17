// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"

#include <AnarchistManGameMode.h>
#include <AnarchistManPlayerController.h>
#include <AnarchistManPlayerState.h>
#include <Bomb.h>
#include <Explosion.h>
#include <OverviewCamera.h>
#include <Utils.h>

#include <Blueprint/UserWidget.h>
#include <Camera/CameraComponent.h>
#include <Components/CapsuleComponent.h>
#include <GameFramework/CharacterMovementComponent.h>
#include <Net/UnrealNetwork.h>

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
}

void APlayerCharacter::BlowUp()
{
    auto* PlayerController = Cast<APlayerController>(GetController());

    auto* GameMode = Cast<AAnarchistManGameMode>(GetWorld()->GetAuthGameMode());
    GameMode->PlayerDeath(PlayerController);

    PlayerController->UnPossess();
    Destroy();
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

    if (BombClass == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("BombClass property is not set!"));
        return;
    }
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

    if (GetPlayerState())
    {
        auto* AMPlayerState = GetPlayerState<AAnarchistManPlayerState>();

        uint32 PlayerId = AMPlayerState->GetPlayerId() % GetNum(Utils::PlayerECCs);
        GetCapsuleComponent()->SetCollisionObjectType(Utils::PlayerECCs[PlayerId]);

        FColor PlayerColor = AMPlayerState->GetPlayerColor();
        UMaterialInstanceDynamic* MaterialInstanceMesh = GetMesh()->CreateAndSetMaterialInstanceDynamic(0);
        MaterialInstanceMesh->SetVectorParameterValue(TEXT("PlayerColor"), PlayerColor);
    }
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

	if (GetPlayerState())
	{
        auto* AMPlayerState = GetPlayerState<AAnarchistManPlayerState>();

        uint32 PlayerId = AMPlayerState->GetPlayerId() % GetNum(Utils::PlayerECCs);
        GetCapsuleComponent()->SetCollisionObjectType(Utils::PlayerECCs[PlayerId]);

        FColor PlayerColor = AMPlayerState->GetPlayerColor();
        UMaterialInstanceDynamic* MaterialInstanceMesh = GetMesh()->CreateAndSetMaterialInstanceDynamic(0);
        MaterialInstanceMesh->SetVectorParameterValue(TEXT("PlayerColor"), PlayerColor);
	}
}

void APlayerCharacter::MoveVertical(float Value)
{
    if (GetPlayerState())
    {
        auto* AMPlayerState = GetPlayerState<AAnarchistManPlayerState>();
        if (AMPlayerState->GetPawnInputState() == PawnInput::DISABLED)
        {
            return;
        }
    }

    if (Value != 0.f) {
        float Rotation = Value > 0.f ? 90.f : -90.f;
        Controller->SetControlRotation(FRotator(0.f, Rotation, 0.f));

        // add movement in that direction
        AddMovementInput(FVector::YAxisVector, Value);
    }
}

void APlayerCharacter::MoveHorizontal(float Value)
{
    if (GetPlayerState())
    {
        auto* AMPlayerState = GetPlayerState<AAnarchistManPlayerState>();
        if (AMPlayerState->GetPawnInputState() == PawnInput::DISABLED)
        {
            return;
        }
    }

    if (Value != 0.f) {
        float Rotation = Value > 0.f ? 0.f : 180.f;
        Controller->SetControlRotation(FRotator(0.f, Rotation, 0.f));

        // add movement in that direction
        AddMovementInput(FVector::XAxisVector, Value);
    }
}

void APlayerCharacter::PlaceBomb_Implementation()
{
    if (GetPlayerState())
    {
        auto* AMPlayerState = GetPlayerState<AAnarchistManPlayerState>();
        PawnInput PawnInputState = AMPlayerState->GetPawnInputState();
        if (PawnInputState == PawnInput::DISABLED || PawnInputState == PawnInput::MOVEMENT_ONLY)
        {
            return;
        }
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
