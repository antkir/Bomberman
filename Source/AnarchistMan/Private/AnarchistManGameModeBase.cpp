// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnarchistManGameModeBase.h"
#include <AnarchistManGameStateBase.h>
#include <AnarchistManPlayerController.h>
#include <AnarchistManPlayerState.h>
#include <PlayerCharacter.h>
#include <Utils.h>
#include <Kismet/GameplayStatics.h>
#include <Blueprint/UserWidget.h>
#include <GameFramework/PlayerStart.h>
#include <Engine/PlayerStartPIE.h>
#include <Engine/Public/EngineUtils.h>

AAnarchistManGameModeBase::AAnarchistManGameModeBase()
{
    GameOverTimeout = 3.f;
}

void AAnarchistManGameModeBase::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    if (GetNumPlayers() >= 4)
    {
        ErrorMessage = TEXT("Server is full!");
        FGameModeEvents::GameModePreLoginEvent.Broadcast(this, UniqueId, ErrorMessage);
        return;
    }

    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

AActor* AAnarchistManGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
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

void AAnarchistManGameModeBase::PlayerDeath(AController* Controller)
{
    AAnarchistManPlayerController* AMPlayerController = Cast<AAnarchistManPlayerController>(Controller);
    if (AMPlayerController)
    {
        AAnarchistManGameStateBase* MyGameState = GetGameState<AAnarchistManGameStateBase>();
        if (MyGameState)
        {
            MyGameState->PlayerDeath();

            if (MyGameState->GetPlayersAlive() > 0)
            {
                APlayerState* PlayerState = AMPlayerController->GetNextViewablePlayer(1);
                APawn* Pawn = PlayerState->GetPawn();
                AMPlayerController->SetViewTargetWithBlend(Pawn, 0.5f, EViewTargetBlendFunction::VTBlend_Cubic);
            }
            else
            {
                if (GameOverCameraClass)
                {
                    AActor* GameOverCamera = UGameplayStatics::GetActorOfClass(this, GameOverCameraClass);

                    if (GameOverCamera)
                    {
                        for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
                        {
                            AAnarchistManPlayerController* PlayerController = Cast<AAnarchistManPlayerController>(PlayerState->GetPlayerController());
                            if (PlayerController)
                            {
                                PlayerController->SetViewTargetWithBlend(GameOverCamera, 0.5f, EViewTargetBlendFunction::VTBlend_Cubic);

                                if (GameOverWidgetClass)
                                {
                                    PlayerController->GameOver(GameOverWidgetClass);
                                }
                                else
                                {
                                    UE_LOG(LogGame, Error, TEXT("GameOverWidgetClass property is not set!"));
                                }
                            }
                        }

                        // Initialize a timer for returning to main
                        GetWorldTimerManager().SetTimer(TimerHandle_GameOverTimeout, this, &AAnarchistManGameModeBase::OnGameOverTimeout, GameOverTimeout);
                    }
                }
                else
                {
                    UE_LOG(LogGame, Error, TEXT("GameOverCamera property is not set!"));
                }
            }
        }
    }
}

void AAnarchistManGameModeBase::OnGameOverTimeout()
{
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    GameInstance->ReturnToMainMenu();
}
