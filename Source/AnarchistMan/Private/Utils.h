// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Utils.generated.h"

#define ECC_GameExplosion ECC_GameTraceChannel1
#define ECC_Pawn1 ECC_GameTraceChannel2
#define ECC_Pawn2 ECC_GameTraceChannel3
#define ECC_Pawn3 ECC_GameTraceChannel4
#define ECC_Pawn4 ECC_GameTraceChannel5
#define ECC_BombVisibility ECC_GameTraceChannel6

DECLARE_LOG_CATEGORY_EXTERN(LogGame, Log, All);

UENUM(BlueprintType)
enum class ETileType : uint8
{
    DEFAULT,
    BLOCK,
    BOMB,
};

namespace ETileNavCost {
enum Type : int64
{
    DEFAULT = 1,
    BLOCK = 1000000,
    BOMB = 1000000000000,
};
}

struct FAMUtils
{

static constexpr float Unit = 100.f;

static constexpr uint8 MaxPlayers = 4;

static constexpr ECollisionChannel PlayerECCs[MaxPlayers]
{
	ECC_Pawn1,
	ECC_Pawn2,
	ECC_Pawn3,
	ECC_Pawn4
};

static constexpr FColor PlayerColors[MaxPlayers]
{
	FColor(255, 0, 0),
	FColor(0, 255, 0),
	FColor(0, 0, 255),
	FColor(255, 255, 0),
};

static FORCEINLINE float RoundToUnitCenter(float Num)
{
    return FMath::Floor(Num / Unit) * Unit + Unit / 2;
}

static FORCEINLINE FVector RoundToUnitCenter(FVector Vector)
{
    Vector.X = RoundToUnitCenter(Vector.X);
    Vector.Y = RoundToUnitCenter(Vector.Y);
    Vector.Z = RoundToUnitCenter(Vector.Z);
    return Vector;
}

static FORCEINLINE uint8 GetPlayerIdFromPawnECC(ECollisionChannel ECC)
{
	switch (ECC)
	{
	case ECC_Pawn1:
		return 1 << 0;
	case ECC_Pawn2:
		return 1 << 1;
	case ECC_Pawn3:
		return 1 << 2;
	case ECC_Pawn4:
        return 1 << 3;
	default:
		return 1 << 4;
	}
}

};
