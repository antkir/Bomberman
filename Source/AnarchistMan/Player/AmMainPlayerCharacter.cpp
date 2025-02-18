// Copyright 2022 Kiryl Antonik

#include "AmMainPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameModes/AmMainGameState.h"
#include "Level/AmBomb.h"
#include "Player/AmMainPlayerState.h"
#include "Game/AmUtils.h"

AAmMainPlayerCharacter::AAmMainPlayerCharacter()
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
void AAmMainPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind movement events
	PlayerInputComponent->BindAxis("Move Up / Down", this, &AAmMainPlayerCharacter::MoveVertical);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AAmMainPlayerCharacter::MoveHorizontal);

	// Bind actions
	PlayerInputComponent->BindAction("Place Bomb", IE_Pressed, this, &AAmMainPlayerCharacter::PlaceBomb);
}

void AAmMainPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAmMainPlayerCharacter, bInputEnabled);
	DOREPLIFETIME(AAmMainPlayerCharacter, bInvincible);
	DOREPLIFETIME(AAmMainPlayerCharacter, MaxWalkSpeed);
}

bool AAmMainPlayerCharacter::IsBlockingExplosion_Implementation()
{
	return false;
}

void AAmMainPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (BombClass == nullptr)
	{
		UE_LOG(LogGame, Error, TEXT("BombClass property is not set!"));
	}

	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	MaxWalkSpeed = DefaultMaxWalkSpeed;
}

void AAmMainPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector CameraLocation = GetActorLocation();
	CameraLocation += CameraLocationOffset;
	CameraComponent->SetWorldLocation(CameraLocation);

	if (!bInputEnabled)
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	}
}

void AAmMainPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	auto* AMPlayerState = GetPlayerState<AAmMainPlayerState>();
	check(AMPlayerState);

	SetPlayerCollision(AMPlayerState);
	SetPlayerColor(AMPlayerState);
}

void AAmMainPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	auto* AMPlayerState = GetPlayerState<AAmMainPlayerState>();
	check(AMPlayerState);

	SetPlayerCollision(AMPlayerState);
	SetPlayerColor(AMPlayerState);

	AMPlayerState->SetActiveBombsCount(0);
}

void AAmMainPlayerCharacter::BlowUp_Implementation()
{
	check(HasAuthority());

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

void AAmMainPlayerCharacter::SetInputEnabled(bool bEnabled)
{
	bInputEnabled = bEnabled;
}

void AAmMainPlayerCharacter::SetInvincible(bool bEnabled)
{
	bInvincible = bEnabled;
}

void AAmMainPlayerCharacter::IncreaseMovementSpeed(float Percentage)
{
	MaxWalkSpeed += DefaultMaxWalkSpeed * Percentage / 100.f;
	OnRep_MaxWalkSpeed();
}

void AAmMainPlayerCharacter::IncrementExplosionRadiusTiles()
{
	ExplosionRadiusTiles++;
}

void AAmMainPlayerCharacter::IncrementActiveBombsLimit()
{
	ActiveBombsLimit++;
}

int32 AAmMainPlayerCharacter::GetExplosionRadiusTiles() const
{
	return ExplosionRadiusTiles;
}

float AAmMainPlayerCharacter::GetDefaultMaxWalkSpeed() const
{
	return DefaultMaxWalkSpeed;
}

void AAmMainPlayerCharacter::MoveVertical(float Value)
{
	if (Value != 0.f)
	{
		AddMovementInput(FVector::YAxisVector, Value);
	}
}

void AAmMainPlayerCharacter::MoveHorizontal(float Value)
{
	if (Value != 0.f)
	{
		AddMovementInput(FVector::XAxisVector, Value);
	}
}

void AAmMainPlayerCharacter::OnBombExploded()
{
	check(HasAuthority());

	auto* AMPlayerState = GetPlayerState<AAmMainPlayerState>();
	check(AMPlayerState);
	int32 ActiveBombsCount = AMPlayerState->GetActiveBombsCount();
	AMPlayerState->SetActiveBombsCount(ActiveBombsCount - 1);
}

bool AAmMainPlayerCharacter::CanPlaceBomb()
{
	check(HasAuthority());

	auto* AMPlayerState = GetPlayerState<AAmMainPlayerState>();
	check(AMPlayerState);
	if (AMPlayerState->GetActiveBombsCount() >= ActiveBombsLimit)
	{
		return false;
	}

	TArray<FOverlapResult> OutOverlaps{};
	FVector Location = GetActorLocation();
	Location.Z -= GetCapsuleComponent()->Bounds.BoxExtent.Z;
	Location = FAmUtils::RoundToUnitCenter(Location);

	FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(FAmUtils::Unit / 8, FAmUtils::Unit / 8, FAmUtils::Unit));
	GetWorld()->OverlapMultiByChannel(OutOverlaps, Location, FQuat::Identity, ECollisionChannel::ECC_WorldDynamic, CollisionShape);

	for (const FOverlapResult& Overlap : OutOverlaps)
	{
		auto* Bomb = Cast<AAmBomb>(Overlap.GetActor());
		if (Bomb)
		{
			return false;
		}
	}

	return true;
}

void AAmMainPlayerCharacter::SetPlayerCollision(AAmMainPlayerState* AMPlayerState)
{
	int32 PlayerId = AMPlayerState->GetPlayerId() % FAmUtils::MaxPlayers;
	GetCapsuleComponent()->SetCollisionObjectType(FAmUtils::PlayerECCs[PlayerId]);

	for (int32 Index = 0; Index < FAmUtils::MaxPlayers; Index++)
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(FAmUtils::PlayerECCs[Index], ECollisionResponse::ECR_Ignore);
	}
}

void AAmMainPlayerCharacter::SetPlayerColor(AAmMainPlayerState* AMPlayerState)
{
	FColor PlayerColor = AMPlayerState->GetPlayerColor();
	UMaterialInstanceDynamic* MaterialInstanceMesh = GetMesh()->CreateAndSetMaterialInstanceDynamic(0);
	MaterialInstanceMesh->SetVectorParameterValue(TEXT("PlayerColor"), PlayerColor);
}

void AAmMainPlayerCharacter::OnRep_MaxWalkSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeed;
}

void AAmMainPlayerCharacter::PlaceBomb_Implementation()
{
	check(HasAuthority());

	if (!bInputEnabled)
	{
		return;
	}

	if (!CanPlaceBomb())
	{
		return;
	}

	auto* AMPlayerState = GetPlayerState<AAmMainPlayerState>();
	check(AMPlayerState);
	int32 ActiveBombsCount = AMPlayerState->GetActiveBombsCount();
	AMPlayerState->SetActiveBombsCount(ActiveBombsCount + 1);

	FVector Location = GetActorLocation();
	Location.Z -= GetCapsuleComponent()->Bounds.BoxExtent.Z;
	Location = FAmUtils::RoundToUnitCenter(Location);

	FTransform Transform;
	Transform.SetLocation(Location);
	Transform.SetRotation(FQuat::Identity);
	AAmBomb* Bomb = GetWorld()->SpawnActorAbsolute<AAmBomb>(BombClass, Transform);
	Bomb->SetExplosionRadiusTiles(ExplosionRadiusTiles);
	Bomb->OnBombExploded.AddDynamic(this, &AAmMainPlayerCharacter::OnBombExploded);
}
