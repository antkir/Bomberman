// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnarchistManGameModeBase.h"
#include <AnarchistManGameStateBase.h>
#include <AnarchistManPlayerController.h>
#include <AnarchistManPlayerState.h>
#include <PlayerCharacter.h>
#include <Utils.h>
#include <Kismet/GameplayStatics.h>

void AAnarchistManGameModeBase::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
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
                if (GameOverCamera)
                {
                    TArray<AActor*> Actors;
                    UGameplayStatics::GetAllActorsOfClass(this, GameOverCamera, Actors);

                    if (Actors.Num() > 0)
                    {
                        for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
                        {
                            APlayerController* PlayerController = PlayerState->GetPlayerController();
                            PlayerController->SetViewTargetWithBlend(Actors[0], 0.5f, EViewTargetBlendFunction::VTBlend_Cubic);
                        }
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
