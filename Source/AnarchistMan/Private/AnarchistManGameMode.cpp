// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnarchistManGameMode.h"

#include <AnarchistManGameState.h>
#include <AnarchistManPlayerController.h>
#include <AnarchistManPlayerState.h>
#include <Bomb.h>
#include <BreakableBlock.h>
#include <Explosion.h>
#include <LevelGenerator.h>
#include <PlayerCharacter.h>
#include <Utils.h>

#include <Blueprint/UserWidget.h>
#include <Camera/CameraComponent.h>
#include <Engine/PlayerStartPIE.h>
#include <Engine/Public/EngineUtils.h>
#include <GameFramework/PlayerStart.h>
#include <Kismet/GameplayStatics.h>

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
    bStartPlayersAsSpectators = true;

    CurrentMatchState = MatchState::Lobby;

    RoundCountdownTime = 3.f;

    CameraBlendTime = 1.f;

    DrawTimeThreshold = 0.15f;
}

void AAnarchistManGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (LevelObserverCameraClass == nullptr)
    {
        UE_LOG(LogGame, Error, TEXT("LevelObserverCamera property is not set!"));
    }

    AActor* LevelGeneratorActor = UGameplayStatics::GetActorOfClass(this, ALevelGenerator::StaticClass());
    if (LevelGeneratorActor == nullptr)
    {
        UE_LOG(LogGame, Error, TEXT("At least one Level Generator must be present in this level!"));
    }

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

    uint32 PlayerId = NewPlayer->PlayerState->GetPlayerId() % GetNum(Utils::PlayerECCs);
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
    NewPlayer->PlayerState->SetPlayerName(PlayerName);

    auto* PlayerController = Cast<AAnarchistManPlayerController>(NewPlayer);
    auto* AMPlayerState = PlayerController->GetPlayerState<AAnarchistManPlayerState>();
    FColor PlayerColor = Utils::PlayerColors[PlayerId];
    AMPlayerState->SetPlayerColor(PlayerColor);

    AActor* LevelObserverCamera = UGameplayStatics::GetActorOfClass(this, LevelObserverCameraClass);
    if (LevelObserverCamera)
    {
        NewPlayer->SetViewTarget(LevelObserverCamera);
    }

    RestartPlayer(NewPlayer);
}

void AAnarchistManGameMode::Destroyed()
{
    Super::Destroyed();

    GetWorldTimerManager().ClearAllTimersForObject(this);
}

AActor* AAnarchistManGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    // Choose a player start
    APlayerStart* FoundPlayerStart = nullptr;
    UClass* PawnClass = GetDefaultPawnClassForController(Player);
    APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
    TArray<APlayerStart*> UnOccupiedStartPoints;
    TArray<APlayerStart*> OccupiedStartPoints;
    UWorld* World = GetWorld();

    for (TActorIterator<APlayerStart> It(World); It; ++It)
    {
        APlayerStart* PlayerStart = *It;

        FVector ActorLocation = PlayerStart->GetActorLocation();
        FRotator ActorRotation = PlayerStart->GetActorRotation();
        if (!World->EncroachingBlockingGeometry(PawnToFit, ActorLocation, ActorRotation))
        {
            UnOccupiedStartPoints.Add(PlayerStart);
        }
        else if (World->FindTeleportSpot(PawnToFit, ActorLocation, ActorRotation))
        {
            OccupiedStartPoints.Add(PlayerStart);
        }
    }

    if (FoundPlayerStart == nullptr)
    {
        if (UnOccupiedStartPoints.Num() > 0)
        {
            int32 NumPlayers = GetNumPlayers();
            if (NumPlayers <= UnOccupiedStartPoints.Num())
            {
                APlayerStart** FoundPlayerStartPointer = UnOccupiedStartPoints.FindByPredicate([&NumPlayers](APlayerStart* PlayerStart)
                {
                    return PlayerStart->PlayerStartTag.ToString() == FString::FromInt(NumPlayers);
                });
                if (FoundPlayerStartPointer)
                {
                    FoundPlayerStart = *FoundPlayerStartPointer;
                }
            }
            else
            {
                FoundPlayerStart = UnOccupiedStartPoints[0];
            }
        }
    }
    return FoundPlayerStart;
}

void AAnarchistManGameMode::PlayerDeath(AController* Controller)
{
    auto* AMGameState = GetGameState<AAnarchistManGameState>();
    auto* PlayerController = Cast<AAnarchistManPlayerController>(Controller);
    auto* PlayerState = PlayerController->GetPlayerState<AAnarchistManPlayerState>();

    PlayerState->SetPlayerDead();

    if (AMGameState->GetPlayersAlive() > 1)
    {
        APlayerState* NextPlayerState = PlayerController->GetNextViewablePlayer(1);
        auto* NextPawn = NextPlayerState->GetPawn<APlayerCharacter>();

        for (const TObjectPtr<APlayerState>& InnerPlayerState : GameState->PlayerArray)
        {
            auto* AMInnerPlayerState = Cast<AAnarchistManPlayerState>(InnerPlayerState);
            if (AMInnerPlayerState->IsDead())
            {
                auto* InnerPlayerController = Cast<AAnarchistManPlayerController>(AMInnerPlayerState->GetPlayerController());
                InnerPlayerController->SetViewTarget(NextPawn, CreateViewTargetTransitionParams(CameraBlendTime));
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
                check(WinnerPlayerState != nullptr);

                AAnarchistManPlayerState* WinnerAMPlayerState = Cast<AAnarchistManPlayerState>(WinnerPlayerState->Get());

                WinnerAMPlayerState->WinRound();

                auto* WinnerPlayerCharacter = WinnerAMPlayerState->GetPlayerController()->GetPawn<APlayerCharacter>();
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
        }, DrawTimeThreshold, false);
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
            PlayerState->WinRound();

            if (PlayerState->GetRoundWins() < AMGameState->GetRoundsToWin())
            {
                FString PlayerName = PlayerState->GetPlayerName();
                BeginRoundOver(PlayerName);
            }
            else
            {
                FString PlayerName = PlayerState->GetPlayerName();
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
    PlayerCharacter->OnPlayerCharacterDeath.AddDynamic(this, &AAnarchistManGameMode::OnPlayerCharacterDeath);
    if (CurrentMatchState == MatchState::Lobby)
    {
        PlayerCharacter->SetInputEnabled(true);
    }
    else
    {
        PlayerCharacter->SetInputEnabled(false);
    }

    return PlayerCharacter;
}

void AAnarchistManGameMode::OnPlayerCharacterDeath(APlayerController* PlayerController)
{
    PlayerDeath(PlayerController);
}

void AAnarchistManGameMode::BeginPreGame()
{
    CurrentMatchState = MatchState::PreGame;

    for (TActorIterator<ABomb> It(GetWorld()); It; ++It)
    {
        ABomb* Bomb = *It;
        Bomb->Destroy();
    }

    for (TActorIterator<AExplosion> It(GetWorld()); It; ++It)
    {
        AExplosion* Explosion = *It;
        Explosion->Destroy();
    }

    AActor* LevelGeneratorActor = UGameplayStatics::GetActorOfClass(this, ALevelGenerator::StaticClass());
    if (LevelGeneratorActor)
    {
        auto* LevelGenerator = Cast<ALevelGenerator>(LevelGeneratorActor);
        LevelGenerator->RegenerateLevel();
    }

    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* AMPlayerState = Cast<AAnarchistManPlayerState>(PlayerState);
        AMPlayerState->SetPlayerAlive();

        auto* PlayerController = Cast<AAnarchistManPlayerController>(AMPlayerState->GetPlayerController());
        APawn* Pawn = PlayerController->GetPawn();
        PlayerController->UnPossess();
        if (Pawn)
        {
            Pawn->Destroy();
        }

        RestartPlayer(PlayerController);

        PlayerController->ClientSetRotation(FRotator(0.f, 90.f, 0.f));
    }

    auto* AMGameState = GetGameState<AAnarchistManGameState>();
    AMGameState->SetPlayersAlive(GetNumPlayers());

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
        auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetPlayerController());
        PlayerController->BeginPreGame(RoundCountdownTime);
    }
}

void AAnarchistManGameMode::PrepareGame()
{
    for (const TObjectPtr<APlayerState>& InnerPlayerState : GameState->PlayerArray)
    {
        auto* PlayerController = Cast<AAnarchistManPlayerController>(InnerPlayerState->GetPlayerController());
        auto* PlayerCharacter = PlayerController->GetPawn<APlayerCharacter>();
        PlayerController->SetViewTarget(PlayerCharacter, CreateViewTargetTransitionParams(CameraBlendTime));
    }

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AAnarchistManGameMode::BeginGame, CameraBlendTime);
}

void AAnarchistManGameMode::BeginGame()
{
    CurrentMatchState = MatchState::InProgress;

    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetPlayerController());
        auto* PlayerCharacter = PlayerController->GetPawn<APlayerCharacter>();
        PlayerCharacter->SetInputEnabled(true);
        PlayerCharacter->SetInvincible(false);
    }

    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetPlayerController());
        PlayerController->BeginGame();
    }
}

void AAnarchistManGameMode::BeginRoundOver(FString PlayerName)
{
    CurrentMatchState = MatchState::RoundOver;

    if (LevelObserverCameraClass)
    {
        AActor* LevelObserverCamera = UGameplayStatics::GetActorOfClass(this, LevelObserverCameraClass);

        if (LevelObserverCamera)
        {
            for (const TObjectPtr<APlayerState>& InnerPlayerState : GameState->PlayerArray)
            {
                auto* PlayerController = Cast<AAnarchistManPlayerController>(InnerPlayerState->GetPlayerController());
                PlayerController->SetViewTarget(LevelObserverCamera, CreateViewTargetTransitionParams(CameraBlendTime));
            }
        }
        else
        {
            UE_LOG(LogGame, Error, TEXT("At least one Level Observer Camera must be present in this level!"));
        }
    }

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AAnarchistManGameMode::BeginPreGame, CameraBlendTime);

    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetPlayerController());
        PlayerController->BeginRoundOver(PlayerName);
    }
}

void AAnarchistManGameMode::BeginGameOver(FString PlayerName)
{
    CurrentMatchState = MatchState::GameOver;

    auto* AMGameState = GetGameState<AAnarchistManGameState>();

    if (LevelObserverCameraClass)
    {
        AActor* LevelObserverCamera = UGameplayStatics::GetActorOfClass(this, LevelObserverCameraClass);

        if (LevelObserverCamera)
        {
            for (const TObjectPtr<APlayerState>& PlayerState : AMGameState->PlayerArray)
            {
                auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetPlayerController());
                PlayerController->SetViewTarget(LevelObserverCamera, CreateViewTargetTransitionParams(CameraBlendTime));
            }
        }
    }

    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetPlayerController());
        PlayerController->BeginGameOver(PlayerName);
    }
}

inline FViewTargetTransitionParams AAnarchistManGameMode::CreateViewTargetTransitionParams(float BlendTime)
{
    FViewTargetTransitionParams TransitionParams;
    TransitionParams.BlendTime = BlendTime;
    TransitionParams.BlendFunction = EViewTargetBlendFunction::VTBlend_Cubic;
    TransitionParams.BlendExp = 0;
    TransitionParams.bLockOutgoing = true;
    return TransitionParams;
}
