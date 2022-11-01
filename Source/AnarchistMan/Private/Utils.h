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

namespace Utils {

constexpr float Unit = 100.f;

constexpr ECollisionChannel PlayerECCs[]
{
	ECC_Pawn1,
	ECC_Pawn2,
	ECC_Pawn3,
	ECC_Pawn4,
    ECC_Pawn
};

constexpr FColor PlayerColors[]
{
	FColor(255, 0, 0),
	FColor(0, 255, 0),
	FColor(0, 0, 255),
	FColor(255, 255, 0),
};

inline float RoundToUnitCenter(float Num)
{
    return FMath::RoundToNegativeInfinity(Num / Unit) * Unit + Unit / 2;
}

inline FVector RoundToUnitCenter(FVector Vector)
{
    Vector.X = FMath::RoundToNegativeInfinity(Vector.X / Unit) * Unit + Unit / 2;
    Vector.Y = FMath::RoundToNegativeInfinity(Vector.Y / Unit) * Unit + Unit / 2;
    Vector.Z = FMath::RoundToNegativeInfinity(Vector.Z / Unit) * Unit + Unit / 2;
    return Vector;
}

inline uint32 GetPlayerIdFromPawnECC(ECollisionChannel ECC)
{
	switch (ECC)
	{
	case ECC_Pawn1:
		return 1;
	case ECC_Pawn2:
		return 2;
	case ECC_Pawn3:
		return 4;
	case ECC_Pawn4:
        return 8;
	default:
		return 16;
	}
}

}
