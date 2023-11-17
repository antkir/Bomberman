// Copyright Epic Games, Inc. All Rights Reserved.

#include "AmMainGameMode.h"
#include "AIModule/Classes/AIController.h"
#include "Camera/CameraActor.h"
#include "Engine/Public/EngineUtils.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Game/AmGameInstance.h"
#include "GameModes/AmMainGameState.h"
#include "Level/AmBomb.h"
#include "Level/AmExplosion.h"
#include "AI/AmGridNavMesh.h"
#include "Level/AmLevelGenerator.h"
#include "Game/AmUtils.h"
#include "Player/AmMainPlayerCharacter.h"
#include "Player/AmMainPlayerController.h"
#include "Player/AmMainPlayerState.h"

namespace MatchState
{
	const FName Lobby = FName(TEXT("Lobby"));
	const FName PreGame = FName(TEXT("PreGame"));
	const FName InProgress = FName(TEXT("InProgress"));
	const FName RoundOver = FName(TEXT("RoundOver"));
	const FName GameOver = FName(TEXT("GameOver"));
}

AAmMainGameMode::AAmMainGameMode()
{
	bResetLevelOnBeginPreGame = true;

	bStartPlayersAsSpectators = true;

	CurrentMatchState = MatchState::Lobby;

	RoundCountdownTime = 3.f;

	CameraBlendTime = 1.f;

	RoundDrawTimeThreshold = 0.15f;

	RecentDeaths = 0;
}

void AAmMainGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (GetLevelOverviewCamera() == nullptr)
	{
		UE_LOG(LogGame, Error, TEXT("Level Overview Camera instance must be present in this level!"));
	}

	AActor* LevelGenerator = UGameplayStatics::GetActorOfClass(this, AAmLevelGenerator::StaticClass());
	if (LevelGenerator == nullptr)
	{
		UE_LOG(LogGame, Error, TEXT("Level Generator instance must be present in this level!"));
	}

	if (AIControllerClass == nullptr)
	{
		UE_LOG(LogGame, Error, TEXT("AIControllerClass property is not set!"));
	}

	SpawnAIControllers();

	auto* GameInstance = GetWorld()->GetGameInstance<UAmGameInstance>();
	check(GameInstance);
	if (GameInstance->ConnectedPlayersNum > 1)
	{
		GetWorldTimerManager().SetTimer(BeginPreGameTimerHandle, this, &AAmMainGameMode::BeginPreGame, 10.f);
	}
	else
	{
		BeginPreGame();
	}
}

void AAmMainGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	if (GetNumPlayers() >= 4)
	{
		ErrorMessage = TEXT("Server is full!");
		FGameModeEvents::GameModePreLoginEvent.Broadcast(this, UniqueId, ErrorMessage);
		return;
	}

	if (CurrentMatchState != MatchState::Lobby)
	{
		ErrorMessage = TEXT("Match is already in progress!");
		FGameModeEvents::GameModePreLoginEvent.Broadcast(this, UniqueId, ErrorMessage);
		return;
	}

	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

void AAmMainGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	SetControllerName(NewPlayer);
	SetControllerColor(NewPlayer);

	ACameraActor* LevelOverviewCamera = GetLevelOverviewCamera();
	if (LevelOverviewCamera)
	{
		NewPlayer->SetViewTarget(LevelOverviewCamera);
	}

	RestartPlayer(NewPlayer);

	auto* GameInstance = GetWorld()->GetGameInstance<UAmGameInstance>();
	check(GameInstance);
	if (GetWorldTimerManager().IsTimerActive(BeginPreGameTimerHandle) && GetNumPlayers() >= GameInstance->ConnectedPlayersNum)
	{
		GetWorldTimerManager().ClearTimer(BeginPreGameTimerHandle);

		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &AAmMainGameMode::BeginPreGame, 1.f);
	}
}

void AAmMainGameMode::Destroyed()
{
	Super::Destroyed();

	GetWorldTimerManager().ClearAllTimersForObject(this);
}

void AAmMainGameMode::PlayerDeath(AController* Controller)
{
	auto* CurrentPlayerState = Controller->GetPlayerState<AAmMainPlayerState>();
	check(CurrentPlayerState);
	CurrentPlayerState->SetPlayerDead();

	auto* AmGameState = GetGameState<AAmMainGameState>();
	check(AmGameState);
	if (AmGameState->GetPlayersAlive() > 1)
	{
		AActor* NextViewTarget = GetNextViewTarget();
		check(NextViewTarget);

		for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
		{
			auto* AmPlayerState = Cast<AAmMainPlayerState>(PlayerState);
			check(AmPlayerState);
			if (AmPlayerState->IsDead())
			{
				auto* AmPlayerController = Cast<AAmMainPlayerController>(AmPlayerState->GetOwningController());
				if (AmPlayerController)
				{
					AmPlayerController->SetViewTarget(NextViewTarget, CreateViewTargetTransitionParams(CameraBlendTime));
				}
			}
		}

		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this, Controller]()
		{
			if (CurrentMatchState != MatchState::InProgress)
			{
				return;
			}

			auto* AmGameState = GetGameState<AAmMainGameState>();

			if (AmGameState->GetPlayersAlive() == 1)
			{
				TObjectPtr<APlayerState>* WinnerPlayerState = GameState->PlayerArray.FindByPredicate([](const TObjectPtr<APlayerState>& PlayerState)
				{
					auto* AmMainPlayerState = Cast<AAmMainPlayerState>(PlayerState);
					check(AmMainPlayerState);
					return !AmMainPlayerState->IsDead();
				});

				check(WinnerPlayerState);
				auto* WinnerAmPlayerState = Cast<AAmMainPlayerState>(WinnerPlayerState->Get());
				check(WinnerAmPlayerState);

				WinnerAmPlayerState->WinRound();

				auto* WinnerPlayerCharacter = WinnerAmPlayerState->GetOwningController()->GetPawn<AAmMainPlayerCharacter>();
				check(WinnerPlayerCharacter);
				WinnerPlayerCharacter->SetInvincible(true);

				if (WinnerAmPlayerState->GetRoundWins() < AmGameState->GetRoundsToWin())
				{
					FString PlayerName = WinnerAmPlayerState->GetPlayerName();
					BeginRoundOver(PlayerName);
				}
				else
				{
					FString PlayerName = WinnerAmPlayerState->GetPlayerName();
					BeginGameOver(PlayerName);
				}
			}
			else
			{
				AActor* LevelGenerator = UGameplayStatics::GetActorOfClass(this, AAmLevelGenerator::StaticClass());
				if (LevelGenerator)
				{
					auto* AmLevelGenerator = Cast<AAmLevelGenerator>(LevelGenerator);
					AmLevelGenerator->SpawnPowerUpsBatch();
				}
			}

			RecentDeaths--;
		}, RoundDrawTimeThreshold, false);
	}
	else
	{
		if (RecentDeaths > 0)
		{
			BeginRoundOver("");
		}
		else
		{
			// Happens only when there is one player playing.
			CurrentPlayerState->WinRound();

			if (CurrentPlayerState->GetRoundWins() < AmGameState->GetRoundsToWin())
			{
				FString PlayerName = CurrentPlayerState->GetPlayerName();
				BeginRoundOver(PlayerName);
			}
			else
			{
				FString PlayerName = CurrentPlayerState->GetPlayerName();
				BeginGameOver(PlayerName);
			}
		}
	}

	AmGameState->PlayerDeath();

	RecentDeaths++;
}

void AAmMainGameMode::RestartGame()
{
	for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
	{
		auto* AmPlayerState = Cast<AAmMainPlayerState>(PlayerState);
		check(AmPlayerState);
		AmPlayerState->ResetRoundWins();
	}

	BeginPreGame();
}

APawn* AAmMainGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	// Don't allow pawn to be spawned with any pitch or roll
	FRotator StartRotation(ForceInit);
	StartRotation.Yaw = StartSpot->GetActorRotation().Yaw;
	FVector StartLocation = StartSpot->GetActorLocation();
	
	FTransform Transform = FTransform(StartRotation, StartLocation);
	APawn* Pawn = SpawnDefaultPawnAtTransform(NewPlayer, Transform);
	auto* PlayerCharacter = Cast<AAmMainPlayerCharacter>(Pawn);
	if (PlayerCharacter)
	{
		PlayerCharacter->OnPlayerCharacterDeath.AddDynamic(this, &AAmMainGameMode::OnPlayerCharacterDeath);
		if (CurrentMatchState == MatchState::Lobby)
		{
			PlayerCharacter->SetInputEnabled(true);
		}
		else
		{
			PlayerCharacter->SetInputEnabled(false);
		}
	}

	return PlayerCharacter;
}

void AAmMainGameMode::OnPlayerCharacterDeath(AController* Controller)
{
	PlayerDeath(Controller);
}

void AAmMainGameMode::BeginPreGame()
{
	CurrentMatchState = MatchState::PreGame;

	for (TActorIterator<AAmBomb> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	for (TActorIterator<AAmExplosion> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	if (bResetLevelOnBeginPreGame)
	{
		auto* GridNavMesh = Cast<AAmGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AAmGridNavMesh::StaticClass()));
		if (GridNavMesh)
		{
			GridNavMesh->ResetTiles();
		}

		auto* LevelGenerator = Cast<AAmLevelGenerator>(UGameplayStatics::GetActorOfClass(this, AAmLevelGenerator::StaticClass()));
		if (LevelGenerator)
		{
			LevelGenerator->RegenerateLevel();
		}
	}

	for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
	{
		auto* AmPlayerState = Cast<AAmMainPlayerState>(PlayerState);
		check(AmPlayerState);
		AmPlayerState->SetPlayerAlive();

		AController* Controller = AmPlayerState->GetOwningController();

		APawn* Pawn = Controller->GetPawn();
		if (Pawn)
		{
			Pawn->Destroy();
		}

		Controller->UnPossess();

		RestartPlayer(Controller);

		Controller->ClientSetRotation(FRotator(0.f, 90.f, 0.f));
	}

	auto* AmGameState = GetGameState<AAmMainGameState>();
	check(AmGameState);
	AmGameState->SetPlayersAlive(GameState->PlayerArray.Num());

	RecentDeaths = 0;

	if (RoundCountdownTime > CameraBlendTime)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &AAmMainGameMode::PrepareGame, RoundCountdownTime - CameraBlendTime);
	}
	else
	{
		// When a client needs to verify that server has a pawn for its controller, UE ignores TransitionParams for SetViewTarget
		UE_LOG(LogGame, Error, TEXT("Some clients do not have updated controllers at this point of time, so smooth view target is broken!"));
		PrepareGame();
	}

	for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
	{
		auto* PlayerController = Cast<AAmMainPlayerController>(PlayerState->GetOwningController());
		if (PlayerController)
		{
			PlayerController->BeginPreGame(RoundCountdownTime);
		}
	}
}

void AAmMainGameMode::PrepareGame()
{
	for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
	{
		auto* PlayerController = Cast<AAmMainPlayerController>(PlayerState->GetOwningController());
		if (PlayerController)
		{
			auto* PlayerCharacter = PlayerController->GetPawn<AAmMainPlayerCharacter>();
			check(PlayerCharacter);
			PlayerController->SetViewTarget(PlayerCharacter, CreateViewTargetTransitionParams(CameraBlendTime));
		}
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AAmMainGameMode::BeginGame, CameraBlendTime);
}

void AAmMainGameMode::BeginGame()
{
	CurrentMatchState = MatchState::InProgress;

	for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
	{
		AController* Controller = PlayerState->GetOwningController();
		auto* PlayerCharacter = Controller->GetPawn<AAmMainPlayerCharacter>();
		check(PlayerCharacter);
		PlayerCharacter->SetInputEnabled(true);
		PlayerCharacter->SetInvincible(false);

		auto* PlayerController = Cast<AAmMainPlayerController>(Controller);
		if (PlayerController)
		{
			PlayerController->BeginGame();
		}
	}
}

void AAmMainGameMode::BeginRoundOver(FString PlayerName)
{
	CurrentMatchState = MatchState::RoundOver;

	ACameraActor* LevelOverviewCamera = GetLevelOverviewCamera();
	if (LevelOverviewCamera)
	{
		for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
		{
			auto* PlayerController = Cast<AAmMainPlayerController>(PlayerState->GetOwningController());
			if (PlayerController)
			{
				PlayerController->SetViewTarget(LevelOverviewCamera, CreateViewTargetTransitionParams(CameraBlendTime));
			}
		}
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AAmMainGameMode::BeginPreGame, CameraBlendTime);

	for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
	{
		auto* PlayerController = Cast<AAmMainPlayerController>(PlayerState->GetOwningController());
		if (PlayerController)
		{
			PlayerController->BeginRoundOver(PlayerName);
		}
	}
}

void AAmMainGameMode::BeginGameOver(FString PlayerName)
{
	CurrentMatchState = MatchState::GameOver;

	ACameraActor* LevelOverviewCamera = GetLevelOverviewCamera();
	if (LevelOverviewCamera)
	{
		for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
		{
			auto* PlayerController = Cast<AAmMainPlayerController>(PlayerState->GetOwningController());
			if (PlayerController)
			{
				PlayerController->SetViewTarget(LevelOverviewCamera, CreateViewTargetTransitionParams(CameraBlendTime));
			}
		}
	}

	for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
	{
		auto* PlayerController = Cast<AAmMainPlayerController>(PlayerState->GetOwningController());
		if (PlayerController)
		{
			PlayerController->BeginGameOver(PlayerName);
		}
	}
}

void AAmMainGameMode::SpawnAIControllers()
{
	if (AIControllerClass == nullptr)
	{
		return;
	}

	TArray<APlayerStart*> StartPoints;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		StartPoints.Add(*It);
	}

	auto* GameInstance = GetWorld()->GetGameInstance<UAmGameInstance>();
	check(GameInstance);
	for (int32 AIPlayerStartId = GameInstance->ConnectedPlayersNum; AIPlayerStartId < StartPoints.Num(); AIPlayerStartId++)
	{
		auto* AIController = GetWorld()->SpawnActor<AAIController>(AIControllerClass, FVector::ZeroVector, FRotator::ZeroRotator);
		if (AIController)
		{
			// Set the player's ID.
			check(AIController->PlayerState);
			AIController->PlayerState->SetPlayerId(GameSession->GetNextPlayerID());

			SetControllerName(AIController);
			SetControllerColor(AIController);

			RestartPlayer(AIController);
		}
	}
}

void AAmMainGameMode::SetControllerName(AController* Controller)
{
	auto* AmPlayerState = Controller->GetPlayerState<AAmMainPlayerState>();
	check(AmPlayerState);
	int32 PlayerId = AmPlayerState->GetPlayerId() % FAmUtils::MaxPlayers;
	FString PlayerName;

	switch (PlayerId)
	{
	case 0:
		PlayerName = "Red";
		break;
	case 1:
		PlayerName = "Green";
		break;
	case 2:
		PlayerName = "Blue";
		break;
	case 3:
		PlayerName = "Yellow";
		break;
	default:
		PlayerName = "Default";
	}

	AmPlayerState->SetPlayerName(PlayerName);
}

void AAmMainGameMode::SetControllerColor(AController* Controller)
{
	auto* AmPlayerState = Controller->GetPlayerState<AAmMainPlayerState>();
	check(AmPlayerState);

	int32 PlayerId = AmPlayerState->GetPlayerId() % FAmUtils::MaxPlayers;
	FColor PlayerColor = FAmUtils::PlayerColors[PlayerId];
	AmPlayerState->SetPlayerColor(PlayerColor);
}

AActor* AAmMainGameMode::GetNextViewTarget() const
{
	// If we fail to find another player to view, return level overview camera.
	AActor* NextViewTarget = GetLevelOverviewCamera();
	for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
	{
		auto* AmPlayerState = Cast<AAmMainPlayerState>(PlayerState);
		if (!AmPlayerState->IsDead())
		{
			auto* PlayerCharacter = AmPlayerState->GetPawn<AAmMainPlayerCharacter>();
			if (PlayerCharacter)
			{
				NextViewTarget = AmPlayerState->GetPawn<AAmMainPlayerCharacter>();;
				break;
			}
		}
	}
	return NextViewTarget;
}

bool AAmMainGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false;
}

ACameraActor* AAmMainGameMode::GetLevelOverviewCamera() const
{
	ACameraActor* CameraActor = nullptr;
	TArray<AActor*> CameraActors;
	UGameplayStatics::GetAllActorsOfClassWithTag(this, ACameraActor::StaticClass(), FName("Overview"), CameraActors);
	if (!CameraActors.IsEmpty())
	{
		CameraActor = Cast<ACameraActor>(CameraActors[0]);
	}
	return CameraActor;
}
