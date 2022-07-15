// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnarchistManGameMode.h"
#include <AnarchistManGameState.h>
#include <AnarchistManPlayerController.h>
#include <AnarchistManPlayerState.h>
#include <BreakableBlock.h>
#include <LevelGenerator.h>
#include <PlayerCharacter.h>
#include <Utils.h>
#include <Kismet/GameplayStatics.h>
#include <Blueprint/UserWidget.h>
#include <GameFramework/PlayerStart.h>
#include <Engine/PlayerStartPIE.h>
#include <Engine/Public/EngineUtils.h>
#include <Camera/CameraComponent.h>

AAnarchistManGameMode::AAnarchistManGameMode()
{
    GameOverTimeout = 3.f;

    RoundsToWin = 3;
}

void AAnarchistManGameMode::BeginPlay()
{
    Super::BeginPlay();
}

void AAnarchistManGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    if (GetNumPlayers() >= 4)
    {
        ErrorMessage = TEXT("Server is full!");
        FGameModeEvents::GameModePreLoginEvent.Broadcast(this, UniqueId, ErrorMessage);
        return;
    }

    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

void AAnarchistManGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    NewPlayer->SetViewTarget(NewPlayer->GetPawn());
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
    if (LevelObserverCameraClass == nullptr)
    {
        UE_LOG(LogGame, Error, TEXT("LevelObserverCamera property is not set!"));
    }

    if (GameOverWidgetClass)
    {
        UE_LOG(LogGame, Error, TEXT("GameOverWidgetClass property is not set!"));
    }

    auto* AMGameState = GetGameState<AAnarchistManGameState>();
    auto* PlayerController = Cast<AAnarchistManPlayerController>(Controller);
    auto* PlayerState = PlayerController->GetPlayerState<AAnarchistManPlayerState>();

    AMGameState->PlayerDeath();

    PlayerState->bIsDead = true;

    if (AMGameState->GetPlayersAlive() > 0)
    {
        AActor* LevelObserverCamera = UGameplayStatics::GetActorOfClass(this, LevelObserverCameraClass);

        APlayerState* NextPlayerState = PlayerController->GetNextViewablePlayer(1);
        auto* NextPawn = NextPlayerState->GetPawn<APlayerCharacter>();

        for (const TObjectPtr<APlayerState>& InnerPlayerState : AMGameState->PlayerArray)
        {
            auto* AMInnerPlayerState = Cast<AAnarchistManPlayerState>(InnerPlayerState);
            if (AMInnerPlayerState->bIsDead)
            {
                AAnarchistManPlayerController* InnerPlayerController = Cast<AAnarchistManPlayerController>(AMInnerPlayerState->GetPlayerController());
                FRotator NextPawnCameraRotation = NextPawn->GetCameraComponent()->GetRelativeRotation();
                InnerPlayerController->ClientSetRotation(NextPawnCameraRotation);

                FViewTargetTransitionParams TransitionParams;
                TransitionParams.BlendTime = 5.f;
                TransitionParams.BlendFunction = EViewTargetBlendFunction::VTBlend_Cubic;
                TransitionParams.BlendExp = 0;
                TransitionParams.bLockOutgoing = true;
                InnerPlayerController->SetViewTarget(NextPawn, TransitionParams);
            }
        }
    }
    else
    {
        PlayerState->RoundWin();

        if (PlayerState->GetRoundWins() < RoundsToWin)
        {
            if (LevelObserverCameraClass)
            {
                AActor* LevelObserverCamera = UGameplayStatics::GetActorOfClass(this, LevelObserverCameraClass);

                if (LevelObserverCamera)
                {
                    for (const TObjectPtr<APlayerState>& InnerPlayerState : AMGameState->PlayerArray)
                    {
                        AAnarchistManPlayerController* InnerPlayerController = Cast<AAnarchistManPlayerController>(InnerPlayerState->GetPlayerController());
                        FRotator LevelObserverCameraRotation = LevelObserverCamera->GetActorRotation();
                        InnerPlayerController->ClientSetRotation(LevelObserverCameraRotation);

                        FViewTargetTransitionParams TransitionParams;
                        TransitionParams.BlendTime = 5.f;
                        TransitionParams.BlendFunction = EViewTargetBlendFunction::VTBlend_Cubic;
                        TransitionParams.BlendExp = 0;
                        TransitionParams.bLockOutgoing = true;
                        InnerPlayerController->SetViewTarget(LevelObserverCamera, TransitionParams);
                    }
                }
            }

            GetWorldTimerManager().SetTimer(TimerHandle, this, &AAnarchistManGameMode::RoundOver, 8.f);
        }
        else
        {
            GameOver(PlayerState);
        }
    }
}

void AAnarchistManGameMode::RoundOver()
{
    for (TActorIterator<ABreakableBlock> It(GetWorld()); It; ++It)
    {
        ABreakableBlock* BreakableBlock = *It;
        BreakableBlock->Destroy();
    }

    AActor* LevelGeneratorActor = UGameplayStatics::GetActorOfClass(this, ALevelGenerator::StaticClass());
    auto* LevelGenerator = Cast<ALevelGenerator>(LevelGeneratorActor);
    LevelGenerator->SpawnBreakableBlocks();

    auto* AMGameState = GetGameState<AAnarchistManGameState>();

    for (const TObjectPtr<APlayerState>& PlayerState : AMGameState->PlayerArray)
    {
        auto* AMPlayerState = Cast<AAnarchistManPlayerState>(PlayerState);
        AMPlayerState->bIsDead = false;

        auto* PlayerController = Cast<AAnarchistManPlayerController>(AMPlayerState->GetPlayerController());
        RestartPlayer(PlayerController);
    }

    AMGameState->SetPlayersAlive(GetNumPlayers());
}

void AAnarchistManGameMode::GameOver(AAnarchistManPlayerState* CurrentPlayerState)
{
    auto* AMGameState = GetGameState<AAnarchistManGameState>();

    if (LevelObserverCameraClass)
    {
        AActor* LevelObserverCamera = UGameplayStatics::GetActorOfClass(this, LevelObserverCameraClass);

        if (LevelObserverCamera)
        {
            for (const TObjectPtr<APlayerState>& PlayerState : AMGameState->PlayerArray)
            {
                AAnarchistManPlayerController* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetPlayerController());
                FRotator LevelObserverCameraRotation = LevelObserverCamera->GetActorRotation();
                PlayerController->ClientSetRotation(LevelObserverCameraRotation);

                FViewTargetTransitionParams TransitionParams;
                TransitionParams.BlendTime = 5.f;
                TransitionParams.BlendFunction = EViewTargetBlendFunction::VTBlend_Cubic;
                TransitionParams.BlendExp = 0;
                TransitionParams.bLockOutgoing = true;
                PlayerController->SetViewTarget(LevelObserverCamera, TransitionParams);

                if (GameOverWidgetClass)
                {
                    uint32 PlayerId = CurrentPlayerState->GetPlayerId() % GetNum(Utils::PlayerECCs);
                    FString PlayerName = Utils::PlayerColors[PlayerId].ToString();
                    //PlayerState->GetPlayerName();
                    PlayerController->GameOver(GameOverWidgetClass, PlayerName);
                }
            }
        }
    }

    // Initialize a timer for returning to main
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AAnarchistManGameMode::OnGameOverTimeout, GameOverTimeout);
}

void AAnarchistManGameMode::OnGameOverTimeout()
{
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    GameInstance->ReturnToMainMenu();
}
