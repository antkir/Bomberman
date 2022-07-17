// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnarchistManGameMode.h"

#include <AnarchistManGameState.h>
#include <AnarchistManPlayerController.h>
#include <AnarchistManPlayerState.h>
#include <BreakableBlock.h>
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

    GameOverTimeout = 3.f;

    RoundsToWin = 3;
}

void AAnarchistManGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (LevelObserverCameraClass == nullptr)
    {
        UE_LOG(LogGame, Error, TEXT("LevelObserverCamera property is not set!"));
    }

    auto* AMGameState = GetGameState<AAnarchistManGameState>();
    AMGameState->SetRoundsToWin(RoundsToWin);

    CurrentMatchState = MatchState::Lobby;

    GetWorldTimerManager().SetTimer(TimerHandle, this, &AAnarchistManGameMode::BeginPreGame, 5.f);
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
    AMPlayerState->SetPawnInputState(PawnInput::MOVEMENT_ONLY);

    AActor* LevelObserverCamera = UGameplayStatics::GetActorOfClass(this, LevelObserverCameraClass);
    if (LevelObserverCamera)
    {
        NewPlayer->SetViewTarget(LevelObserverCamera);
    }

    RestartPlayer(NewPlayer);
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
    AMGameState->PlayerDeath();

    auto* PlayerController = Cast<AAnarchistManPlayerController>(Controller);
    auto* PlayerState = PlayerController->GetPlayerState<AAnarchistManPlayerState>();
    PlayerState->SetPlayerDead();

    if (AMGameState->GetPlayersAlive() > 1)
    {
        APlayerState* NextPlayerState = PlayerController->GetNextViewablePlayer(1);
        auto* NextPawn = NextPlayerState->GetPawn<APlayerCharacter>();
        float BlendTime = 3.f;

        for (const TObjectPtr<APlayerState>& InnerPlayerState : GameState->PlayerArray)
        {
            auto* AMInnerPlayerState = Cast<AAnarchistManPlayerState>(InnerPlayerState);
            if (AMInnerPlayerState->IsDead())
            {
                auto* InnerPlayerController = Cast<AAnarchistManPlayerController>(AMInnerPlayerState->GetPlayerController());
                InnerPlayerController->SetViewTarget(NextPawn, CreateViewTargetTransitionParams(BlendTime));
            }
        }
    }
    else if (AMGameState->GetPlayersAlive() == 1)
    {
        AAnarchistManPlayerState* AMPlayerState = nullptr;
        for (const TObjectPtr<APlayerState>& InnerPlayerState : GameState->PlayerArray)
        {
            auto* AMInnerPlayerState = Cast<AAnarchistManPlayerState>(InnerPlayerState);
            if (!AMInnerPlayerState->IsDead())
            {
                AMPlayerState = AMInnerPlayerState;
            }
        }
        check(AMPlayerState != nullptr);

        AMPlayerState->WinRound();

        if (AMPlayerState->GetRoundWins() < RoundsToWin)
        {
            FString PlayerName = AMPlayerState->GetPlayerName();
            BeginRoundOver(PlayerName);
        }
        else
        {
            FString PlayerName = AMPlayerState->GetPlayerName();
            BeginGameOver(PlayerName);
        }
    }
    else
    {
        PlayerState->WinRound();

        if (PlayerState->GetRoundWins() < RoundsToWin)
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

void AAnarchistManGameMode::RestartGame()
{
    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* AMPlayerState = Cast<AAnarchistManPlayerState>(PlayerState);
        AMPlayerState->ResetRoundWins();
    }

    BeginPreGame();
}

void AAnarchistManGameMode::BeginPreGame()
{
    CurrentMatchState = MatchState::PreGame;

    for (TActorIterator<ABreakableBlock> It(GetWorld()); It; ++It)
    {
        ABreakableBlock* BreakableBlock = *It;
        BreakableBlock->Destroy();
    }

    AActor* LevelGeneratorActor = UGameplayStatics::GetActorOfClass(this, ALevelGenerator::StaticClass());
    if (LevelGeneratorActor)
    {
        auto* LevelGenerator = Cast<ALevelGenerator>(LevelGeneratorActor);
        LevelGenerator->SpawnBreakableBlocks();
    }
    else
    {
        UE_LOG(LogGame, Error, TEXT("At least one Level Generator must be present in this level!"));
    }

    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* AMPlayerState = Cast<AAnarchistManPlayerState>(PlayerState);
        AMPlayerState->SetPlayerAlive();
        AMPlayerState->SetPawnInputState(PawnInput::DISABLED);

        auto* PlayerController = Cast<AAnarchistManPlayerController>(AMPlayerState->GetPlayerController());
        APawn* Pawn = PlayerController->GetPawn();
        PlayerController->UnPossess();
        if (Pawn)
        {
            Pawn->Destroy();
        }

        RestartPlayer(PlayerController);
    }

    auto* AMGameState = GetGameState<AAnarchistManGameState>();
    AMGameState->SetPlayersAlive(GetNumPlayers());

    float Countdown = 5.f;
    float BlendTime = 3.f;

    if (Countdown > BlendTime)
    {
        GetWorldTimerManager().SetTimer(TimerHandle, this, &AAnarchistManGameMode::PrepareGame, Countdown - BlendTime);
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
        PlayerController->BeginPreGame(Countdown);
    }
}

void AAnarchistManGameMode::PrepareGame()
{
    float BlendTime = 3.f;

    for (const TObjectPtr<APlayerState>& InnerPlayerState : GameState->PlayerArray)
    {
        auto* PlayerController = Cast<AAnarchistManPlayerController>(InnerPlayerState->GetPlayerController());
        auto* PlayerCharacter = PlayerController->GetPawn<APlayerCharacter>();
        PlayerController->SetViewTarget(PlayerCharacter, CreateViewTargetTransitionParams(BlendTime));
    }

    GetWorldTimerManager().SetTimer(TimerHandle, this, &AAnarchistManGameMode::BeginGame, BlendTime);
}

void AAnarchistManGameMode::BeginGame()
{
    CurrentMatchState = MatchState::InProgress;

    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* AMPlayerState = Cast<AAnarchistManPlayerState>(PlayerState);
        AMPlayerState->SetPawnInputState(PawnInput::ENABLED);
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

    float BlendTime = 3.f;

    if (LevelObserverCameraClass)
    {
        AActor* LevelObserverCamera = UGameplayStatics::GetActorOfClass(this, LevelObserverCameraClass);

        if (LevelObserverCamera)
        {
            for (const TObjectPtr<APlayerState>& InnerPlayerState : GameState->PlayerArray)
            {
                auto* PlayerController = Cast<AAnarchistManPlayerController>(InnerPlayerState->GetPlayerController());
                PlayerController->SetViewTarget(LevelObserverCamera, CreateViewTargetTransitionParams(BlendTime));
            }
        }
        else
        {
            UE_LOG(LogGame, Error, TEXT("At least one Level Observer Camera must be present in this level!"));
        }
    }

    GetWorldTimerManager().SetTimer(TimerHandle, this, &AAnarchistManGameMode::BeginPreGame, BlendTime);

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
            float BlendTime = 3.f;

            for (const TObjectPtr<APlayerState>& PlayerState : AMGameState->PlayerArray)
            {
                auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetPlayerController());
                PlayerController->SetViewTarget(LevelObserverCamera, CreateViewTargetTransitionParams(BlendTime));
            }
        }
    }

    for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
    {
        auto* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetPlayerController());
        PlayerController->BeginGameOver(PlayerName);
    }
}

void AAnarchistManGameMode::OnGameOverTimeout()
{
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    GameInstance->ReturnToMainMenu();
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
