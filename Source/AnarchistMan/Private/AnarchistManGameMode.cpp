// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnarchistManGameMode.h"

#include <AIModule/Classes/AIController.h>
#include <Camera/CameraActor.h>
#include <Engine/Public/EngineUtils.h>
#include <GameFramework/GameSession.h>
#include <GameFramework/PlayerStart.h>
#include <Kismet/GameplayStatics.h>

#include <AMGameInstance.h>
#include <AnarchistManGameState.h>
#include <AnarchistManPlayerController.h>
#include <AnarchistManPlayerState.h>
#include <Bomb.h>
#include <Explosion.h>
#include <GridNavMesh.h>
#include <LevelGenerator.h>
#include <PlayerCharacter.h>
#include <Utils.h>

namespace MatchState
{
    const FName Lobby = FName(TEXT("Lobby"));
    const FName PreGame = FName(TEXT("PreGame"));
    const FName InProgress = FName(TEXT("InProgress"));
    const FName RoundOver = FName(TEXT("RoundOver"));
    const FName GameOver = FName(TEXT("GameOver"));
}

AAnarchistManGameMode::AAnarchistManGameMode()
{
    bResetLevelOnBeginPreGame = true;

    bStartPlayersAsSpectators = true;

    CurrentMatchState = MatchState::Lobby;

    RoundCountdownTime = 3.f;

    CameraBlendTime = 1.f;

    RoundDrawTimeThreshold = 0.15f;

    RecentDeaths = 0;
}

void AAnarchistManGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (GetLevelOverviewCamera() == nullptr)
    {
        UE_LOG(LogGame, Error, TEXT("Level Overview Camera instance must be present in this level!"));
    }

    AActor* LevelGeneratorActor = UGameplayStatics::GetActorOfClass(this, ALevelGenerator::StaticClass());
    if (LevelGeneratorActor == nullptr)
    {
        UE_LOG(LogGame, Error, TEXT("Level Generator instance must be present in this level!"));
    }

    if (AIControllerClass == nullptr)
    {
        UE_LOG(LogGame, Error, TEXT("AIControllerClass property is not set!"));
    }

    SpawnAIControllers();

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AAnarchistManGameMode::BeginPreGame, 1.f);
}

void AAnarchistManGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
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

void AAnarchistManGameMode::PostLogin(APlayerController* NewPlayer)
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
}

void AAnarchistManGameMode::Destroyed()
{
    Super::Destroyed();

    GetWorldTimerManager().ClearAllTimersForObject(this);
}

void AAnarchistManGameMode::PlayerDeath(AController* Controller)
{
    auto* CurrentPlayerState = Controller->GetPlayerState<AAnarchistManPlayerState>();
    check(CurrentPlayerState);
    CurrentPlayerState->SetPlayerDead();

    auto* AMGameState = GetGameState<AAnarchistManGameState>();
    check(AMGameState);
    if (AMGameState->GetPlayersAlive() > 1)
    {
        AActor* NextViewTarget = GetNextViewTarget();
        check(NextViewTarget);

        for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
        {
            auto* AMPlayerState = Cast<AAnarchistManPlayerState>(PlayerState);
            check(AMPlayerState);
            if (AMPlayerState->IsDead())
            {
                auto* AMPlayerController = Cast<AAnarchistManPlayerController>(AMPlayerState->GetOwningController());
                if (AMPlayerController)
                {
                    AMPlayerController->SetViewTarget(NextViewTarget, CreateViewTargetTransitionParams(CameraBlendTime));
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

            auto* AMGameState = GetGameState<AAnarchistManGameState>();

            if (AMGameState->GetPlayersAlive() == 1)
            {
                TObjectPtr<APlayerState>* WinnerPlayerState = GameState->PlayerArray.FindByPredicate([](const TObjectPtr<APlayerState>& APS)
                {
                    auto* AAMPS = Cast<AAnarchistManPlayerState>(APS);
                    return !AAMPS->IsDead();
                });

                check(WinnerPlayerState);

                AAnarchistManPlayerState* WinnerAMPlayerState = Cast<AAnarchistManPlayerState>(WinnerPlayerState->Get());
                check(WinnerAMPlayerState);
                WinnerAMPlayerState->WinRound();

                auto* WinnerPlayerCharacter = WinnerAMPlayerState->GetOwningController()->GetPawn<APlayerCharacter>();
                check(WinnerPlayerCharacter);
                WinnerPlayerCharacter->SetInvincible(true);

                if (WinnerAMPlayerState->GetRoundWins() < AMGameState->GetRoundsToWin())
                {
                    FString PlayerName = WinnerAMPlayerState->GetPlayerName();
                    BeginRoundOver(PlayerName);
                }
                else
                {
                    FString PlayerName = WinnerAMPlayerState->GetPlayerName();
                    BeginGameOver(PlayerName);
                }
            }
            else
            {
                AActor* LevelGeneratorActor = UGameplayStatics::GetActorOfClass(this, ALevelGenerator::StaticClass());
                if (LevelGeneratorActor)
                {
                    auto* LevelGenerator = Cast<ALevelGenerator>(LevelGeneratorActor);
                    LevelGenerator->SpawnPowerUpsBatch();
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

            if (CurrentPlayerState->GetRoundWins() < AMGameState->GetRoundsToWin())
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

    AMGameState->PlayerDeath();

    RecentDeaths++;
}

void AAnarchistManGameMode::RestartGame()
{
    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* AMPlayerState = Cast<AAnarchistManPlayerState>(PlayerState);
        check(AMPlayerState);
        AMPlayerState->ResetRoundWins();
    }

    BeginPreGame();
}

APawn* AAnarchistManGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
    // Don't allow pawn to be spawned with any pitch or roll
    FRotator StartRotation(ForceInit);
    StartRotation.Yaw = StartSpot->GetActorRotation().Yaw;
    FVector StartLocation = StartSpot->GetActorLocation();
    
    FTransform Transform = FTransform(StartRotation, StartLocation);
    APawn* Pawn = SpawnDefaultPawnAtTransform(NewPlayer, Transform);
    auto* PlayerCharacter = Cast<APlayerCharacter>(Pawn);
    if (PlayerCharacter)
    {
        PlayerCharacter->OnPlayerCharacterDeath.AddDynamic(this, &AAnarchistManGameMode::OnPlayerCharacterDeath);
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

void AAnarchistManGameMode::OnPlayerCharacterDeath(AController* Controller)
{
    PlayerDeath(Controller);
}

void AAnarchistManGameMode::BeginPreGame()
{
    CurrentMatchState = MatchState::PreGame;

    for (TActorIterator<ABomb> It(GetWorld()); It; ++It)
    {
        It->Destroy();
    }

    for (TActorIterator<AExplosion> It(GetWorld()); It; ++It)
    {
        It->Destroy();
    }

    if (bResetLevelOnBeginPreGame)
    {
        auto* GridNavMesh = Cast<AGridNavMesh>(UGameplayStatics::GetActorOfClass(this, AGridNavMesh::StaticClass()));
        if (GridNavMesh)
        {
            GridNavMesh->ResetTiles();
        }

        auto* LevelGenerator = Cast<ALevelGenerator>(UGameplayStatics::GetActorOfClass(this, ALevelGenerator::StaticClass()));
        if (LevelGenerator)
        {
            LevelGenerator->RegenerateLevel();
        }
    }

    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* AMPlayerState = Cast<AAnarchistManPlayerState>(PlayerState);
        check(AMPlayerState);
        AMPlayerState->SetPlayerAlive();

        AController* Controller = AMPlayerState->GetOwningController();

        APawn* Pawn = Controller->GetPawn();
        if (Pawn)
        {
            Pawn->Destroy();
        }

        Controller->UnPossess();

        RestartPlayer(Controller);

        Controller->ClientSetRotation(FRotator(0.f, 90.f, 0.f));
    }

    auto* AMGameState = GetGameState<AAnarchistManGameState>();
    check(AMGameState);
    AMGameState->SetPlayersAlive(GameState->PlayerArray.Num());

    RecentDeaths = 0;

    if (RoundCountdownTime > CameraBlendTime)
    {
        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, this, &AAnarchistManGameMode::PrepareGame, RoundCountdownTime - CameraBlendTime);
    }
    else
    {
        // When a client needs to verify that server has a pawn for its controller, UE ignores TransitionParams for SetViewTarget
        UE_LOG(LogGame, Error, TEXT("Some clients do not have updated controllers at this point of time, so smooth view target is broken!"));
        PrepareGame();
    }

    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetOwningController());
        if (PlayerController)
        {
            PlayerController->BeginPreGame(RoundCountdownTime);
        }
    }
}

void AAnarchistManGameMode::PrepareGame()
{
    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetOwningController());
        if (PlayerController)
        {
            auto* PlayerCharacter = PlayerController->GetPawn<APlayerCharacter>();
            check(PlayerCharacter);
            PlayerController->SetViewTarget(PlayerCharacter, CreateViewTargetTransitionParams(CameraBlendTime));
        }
    }

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AAnarchistManGameMode::BeginGame, CameraBlendTime);
}

void AAnarchistManGameMode::BeginGame()
{
    CurrentMatchState = MatchState::InProgress;

    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        AController* Controller = PlayerState->GetOwningController();
        auto* PlayerCharacter = Controller->GetPawn<APlayerCharacter>();
        check(PlayerCharacter);
        PlayerCharacter->SetInputEnabled(true);
        PlayerCharacter->SetInvincible(false);

        auto* PlayerController = Cast<AAnarchistManPlayerController>(Controller);
        if (PlayerController)
        {
            PlayerController->BeginGame();
        }
    }
}

void AAnarchistManGameMode::BeginRoundOver(FString PlayerName)
{
    CurrentMatchState = MatchState::RoundOver;

    ACameraActor* LevelOverviewCamera = GetLevelOverviewCamera();
    if (LevelOverviewCamera)
    {
        for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
        {
            auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetOwningController());
            if (PlayerController)
            {
                PlayerController->SetViewTarget(LevelOverviewCamera, CreateViewTargetTransitionParams(CameraBlendTime));
            }
        }
    }

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AAnarchistManGameMode::BeginPreGame, CameraBlendTime);

    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetOwningController());
        if (PlayerController)
        {
            PlayerController->BeginRoundOver(PlayerName);
        }
    }
}

void AAnarchistManGameMode::BeginGameOver(FString PlayerName)
{
    CurrentMatchState = MatchState::GameOver;

    auto* AMGameState = GetGameState<AAnarchistManGameState>();
    check(AMGameState);

    ACameraActor* LevelOverviewCamera = GetLevelOverviewCamera();
    if (LevelOverviewCamera)
    {
        for (const TObjectPtr<APlayerState>& PlayerState : AMGameState->PlayerArray)
        {
            auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetOwningController());
            if (PlayerController)
            {
                PlayerController->SetViewTarget(LevelOverviewCamera, CreateViewTargetTransitionParams(CameraBlendTime));
            }
        }
    }

    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetOwningController());
        if (PlayerController)
        {
            PlayerController->BeginGameOver(PlayerName);
        }
    }
}

void AAnarchistManGameMode::SpawnAIControllers()
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

    auto* GameInstance = GetWorld()->GetGameInstanceChecked<UAMGameInstance>();
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

void AAnarchistManGameMode::SetControllerName(AController* Controller)
{
    auto* PlayerState = Controller->GetPlayerState<AAnarchistManPlayerState>();
    check(PlayerState);
    int32 PlayerId = PlayerState->GetPlayerId() % Utils::MAX_PLAYERS;
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

    PlayerState->SetPlayerName(PlayerName);
}

void AAnarchistManGameMode::SetControllerColor(AController* Controller)
{
    auto* PlayerState = Controller->GetPlayerState<AAnarchistManPlayerState>();
    check(PlayerState);
    int32 PlayerId = PlayerState->GetPlayerId() % Utils::MAX_PLAYERS;
    FColor PlayerColor = Utils::PlayerColors[PlayerId];
    PlayerState->SetPlayerColor(PlayerColor);
}

AActor* AAnarchistManGameMode::GetNextViewTarget() const
{
    // If we fail to find another player to view, return level overview camera.
    AActor* NextViewTarget = GetLevelOverviewCamera();
    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* AMPlayerState = Cast<AAnarchistManPlayerState>(PlayerState);
        if (!AMPlayerState->IsDead())
        {
            auto* PlayerCharacter = AMPlayerState->GetPawn<APlayerCharacter>();
            if (PlayerCharacter)
            {
                NextViewTarget = AMPlayerState->GetPawn<APlayerCharacter>();;
                break;
            }
        }
    }
    return NextViewTarget;
}

bool AAnarchistManGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
    return false;
}

ACameraActor* AAnarchistManGameMode::GetLevelOverviewCamera() const
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
