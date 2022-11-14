// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"

#include <Camera/CameraComponent.h>
#include <Components/CapsuleComponent.h>
#include <GameFramework/CharacterMovementComponent.h>
#include <Net/UnrealNetwork.h>

#include <AnarchistManGameState.h>
#include <AnarchistManPlayerState.h>
#include <Bomb.h>
#include <Utils.h>

APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set camera component
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(GetRootComponent());

	CameraComponent->SetUsingAbsoluteLocation(true);
	CameraComponent->SetUsingAbsoluteRotation(true);

    bInputEnabled = true;
    bInvincible = true;

    ExplosionRadiusTiles = 1;
    ActiveBombsLimit = 1;
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

    DOREPLIFETIME(APlayerCharacter, bInputEnabled);
    DOREPLIFETIME(APlayerCharacter, bInvincible);
    DOREPLIFETIME(APlayerCharacter, MaxWalkSpeed);
}

bool APlayerCharacter::IsBlockingExplosion_Implementation()
{
    return false;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

    if (BombClass == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("BombClass property is not set!"));
        return;
    }

    DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

    MaxWalkSpeed = DefaultMaxWalkSpeed;
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector CameraLocation = GetActorLocation();
	CameraLocation.X += CameraLocationOffset.X;
	CameraLocation.Y += CameraLocationOffset.Y;
	CameraLocation.Z += CameraLocationOffset.Z;
	CameraComponent->SetWorldLocation(CameraLocation);

    if (!bInputEnabled)
    {
        GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
    }
    else
    {
        GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
    }

    GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeed;
}

void APlayerCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    if (GetPlayerState())
    {
        auto* AMPlayerState = GetPlayerState<AAnarchistManPlayerState>();

        int32 PlayerId = AMPlayerState->GetPlayerId() % Utils::MAX_PLAYERS;
        GetCapsuleComponent()->SetCollisionObjectType(Utils::PlayerECCs[PlayerId]);

        for (int32 Index = 0; Index < Utils::MAX_PLAYERS; Index++)
        {
            GetCapsuleComponent()->SetCollisionResponseToChannel(Utils::PlayerECCs[Index], ECollisionResponse::ECR_Ignore);
        }

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
        auto* AMGameState = GetWorld()->GetGameState<AAnarchistManGameState>();
        auto* AMPlayerState = GetPlayerState<AAnarchistManPlayerState>();

        int32 PlayerId = AMPlayerState->GetPlayerId() % Utils::MAX_PLAYERS;
        GetCapsuleComponent()->SetCollisionObjectType(Utils::PlayerECCs[PlayerId]);

        for (int32 Index = 0; Index < Utils::MAX_PLAYERS; Index++)
        {
            GetCapsuleComponent()->SetCollisionResponseToChannel(Utils::PlayerECCs[Index], ECollisionResponse::ECR_Ignore);
        }

        FColor PlayerColor = AMPlayerState->GetPlayerColor();
        UMaterialInstanceDynamic* MaterialInstanceMesh = GetMesh()->CreateAndSetMaterialInstanceDynamic(0);
        MaterialInstanceMesh->SetVectorParameterValue(TEXT("PlayerColor"), PlayerColor);

        AMPlayerState->SetActiveBombsCount(0);
	}
}

void APlayerCharacter::BlowUp_Implementation()
{
    if (!IsValid(this) || bInvincible)
    {
        return;
    }

    if (Controller)
    {
        OnPlayerCharacterDeath.Broadcast(Controller);

        Controller->UnPossess();
    }

    Destroy();
}

void APlayerCharacter::SetInputEnabled(bool InputEnabled)
{
    bInputEnabled = InputEnabled;
}

void APlayerCharacter::SetInvincible(bool Invincible)
{
    bInvincible = Invincible;
}

void APlayerCharacter::IncreaseMovementSpeed(float Percentage)
{
    MaxWalkSpeed += DefaultMaxWalkSpeed * Percentage / 100.f;
}

void APlayerCharacter::IncrementExplosionRadiusTiles()
{
    ExplosionRadiusTiles++;
}

void APlayerCharacter::IncrementActiveBombsLimit()
{
    ActiveBombsLimit++;
}

int32 APlayerCharacter::GetExplosionRadiusTiles() const
{
    return ExplosionRadiusTiles;
}

float APlayerCharacter::GetDefaultMaxWalkSpeed() const
{
    return DefaultMaxWalkSpeed;
}

void APlayerCharacter::MoveVertical(float Value)
{
    if (Value != 0.f)
    {
        // Add movement in that direction.
        AddMovementInput(FVector::YAxisVector, Value);
    }
}

void APlayerCharacter::MoveHorizontal(float Value)
{
    if (Value != 0.f) 
    {
        // Add movement in that direction.
        AddMovementInput(FVector::XAxisVector, Value);
    }
}

void APlayerCharacter::OnBombExploded()
{
    auto* AmPlayerState = GetPlayerState<AAnarchistManPlayerState>();
    if (AmPlayerState)
    {
        int32 ActiveBombsCount = AmPlayerState->GetActiveBombsCount();
        AmPlayerState->SetActiveBombsCount(ActiveBombsCount - 1);
    }
}

bool APlayerCharacter::CanPlaceBomb()
{
    bool CanPlaceBomb = true;

    if (GetPlayerState())
    {
        auto* AMPlayerState = GetPlayerState<AAnarchistManPlayerState>();
        if (AMPlayerState->GetActiveBombsCount() >= ActiveBombsLimit)
        {
            CanPlaceBomb = false;
        }
    }

    TArray<FOverlapResult> OutOverlaps{};
    FVector Location = GetActorLocation();
    Location.Z -= GetCapsuleComponent()->Bounds.BoxExtent.Z;
    Location = Utils::RoundToUnitCenter(Location);

    FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(Utils::Unit / 8, Utils::Unit / 8, Utils::Unit));
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
    if (!bInputEnabled)
    {
        return;
    }

    if (!CanPlaceBomb())
    {
        return;
    }

    if (GetPlayerState())
    {
        auto* AmPlayerState = GetPlayerState<AAnarchistManPlayerState>();
        check(AmPlayerState);
        int32 ActiveBombsCount = AmPlayerState->GetActiveBombsCount();
        AmPlayerState->SetActiveBombsCount(ActiveBombsCount + 1);
    }

    FVector Location = GetActorLocation();
    Location.Z -= GetCapsuleComponent()->Bounds.BoxExtent.Z;
    Location = Utils::RoundToUnitCenter(Location);

    FTransform Transform;
    Transform.SetLocation(Location);
    Transform.SetRotation(FQuat::Identity);
    FActorSpawnParameters SpawnParameters;
    ABomb* Bomb = GetWorld()->SpawnActorAbsolute<ABomb>(BombClass, Transform, SpawnParameters);
    Bomb->SetExplosionRadiusTiles(ExplosionRadiusTiles);
    Bomb->OnBombExploded.AddDynamic(this, &APlayerCharacter::OnBombExploded);
}
