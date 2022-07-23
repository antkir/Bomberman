// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"

#include <AnarchistManGameState.h>
#include <AnarchistManPlayerState.h>
#include <Bomb.h>
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

void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(APlayerCharacter, InputState);
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

        AMPlayerState->SetActiveBombsLimit(3);
        AMPlayerState->SetActiveBombsCount(0);
	}
}

bool APlayerCharacter::HasOwnExplosionVisualEffect_Implementation()
{
    return false;
}

void APlayerCharacter::BlowUp_Implementation()
{
    if (!IsValid(this))
    {
        return;
    }

    auto* AMGameState = GetWorld()->GetGameState<AAnarchistManGameState>();
    if (AMGameState->PlayerArray.Num() > 1 && AMGameState->GetPlayersAlive() == 1)
    {
        return;
    }

    auto* PlayerController = Cast<APlayerController>(GetController());
    PlayerController->UnPossess();

    Destroy();

    OnPlayerCharacterDeath.Broadcast(PlayerController);
}

void APlayerCharacter::SetPawnInputState(EPawnInput PawnInput)
{
    InputState = PawnInput;
}

void APlayerCharacter::MoveVertical(float Value)
{
    if (GetPlayerState())
    {
        if (InputState == EPawnInput::DISABLED)
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
        if (InputState == EPawnInput::DISABLED)
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

void APlayerCharacter::OnBombExploded()
{
    auto* AMPlayerState = GetPlayerState<AAnarchistManPlayerState>();
    uint32 ActiveBombsCount = AMPlayerState->GetActiveBombsCount();
    AMPlayerState->SetActiveBombsCount(ActiveBombsCount - 1);
}

bool APlayerCharacter::CanPlaceBomb()
{
    bool CanPlaceBomb = true;

    if (GetPlayerState())
    {
        if (InputState == EPawnInput::DISABLED || InputState == EPawnInput::MOVEMENT_ONLY)
        {
            CanPlaceBomb = false;
        }

        auto* AMPlayerState = GetPlayerState<AAnarchistManPlayerState>();
        if (!AMPlayerState->CanPlaceBomb())
        {
            CanPlaceBomb = false;
        }
    }

    TArray<FOverlapResult> OutOverlaps{};
    FVector Location = GetActorLocation();
    Location.X = Utils::RoundUnitCenter(Location.X);
    Location.Y = Utils::RoundUnitCenter(Location.Y);
    Location.Z -= GetCapsuleComponent()->Bounds.BoxExtent.Z;
    Location.Z = Utils::RoundUnitCenter(Location.Z);
    FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(Utils::Unit / 20));
    GetWorld()->OverlapMultiByChannel(OutOverlaps, Location, FQuat::Identity, ECollisionChannel::ECC_WorldDynamic, CollisionShape);

    for (const FOverlapResult& Overlap : OutOverlaps)
    {
        auto* Bomb = Cast<ABomb>(Overlap.GetActor());
        if (Bomb)
        {
            CanPlaceBomb = false;
        }
    }

    return CanPlaceBomb;
}

void APlayerCharacter::PlaceBomb_Implementation()
{
    if (!CanPlaceBomb())
    {
        return;
    }

    if (GetPlayerState())
    {
        auto* AMPlayerState = GetPlayerState<AAnarchistManPlayerState>();
        uint32 ActiveBombsCount = AMPlayerState->GetActiveBombsCount();
        AMPlayerState->SetActiveBombsCount(ActiveBombsCount + 1);
    }

    FVector Location = GetActorLocation();
    Location.X = Utils::RoundUnitCenter(Location.X);
    Location.Y = Utils::RoundUnitCenter(Location.Y);
    Location.Z -= GetCapsuleComponent()->Bounds.BoxExtent.Z;
    FTransform Transform;
    Transform.SetLocation(Location);
    Transform.SetRotation(FQuat::Identity);
    FActorSpawnParameters SpawnParameters;
    ABomb* Bomb = GetWorld()->SpawnActorAbsolute<ABomb>(BombClass, Transform, SpawnParameters);
    Bomb->OnBombExploded.AddDynamic(this, &APlayerCharacter::OnBombExploded);
}
