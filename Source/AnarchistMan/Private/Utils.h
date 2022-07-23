// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#define ECC_GameExplosion ECC_GameTraceChannel1
#define ECC_Pawn1 ECC_GameTraceChannel2
#define ECC_Pawn2 ECC_GameTraceChannel3
#define ECC_Pawn3 ECC_GameTraceChannel4
#define ECC_Pawn4 ECC_GameTraceChannel5

DECLARE_LOG_CATEGORY_EXTERN(LogGame, Log, All);

UENUM(BlueprintType)
enum class EPawnInput : uint8
{
    DISABLED UMETA(DisplayName = "Disabled"),
    MOVEMENT_ONLY UMETA(DisplayName = "Movement Only"),
    ENABLED UMETA(DisplayName = "Enabled"),
};

namespace Utils {

constexpr float Unit = 100.f;

constexpr ECollisionChannel PlayerECCs[]
{
	ECC_Pawn1,
	ECC_Pawn2,
	ECC_Pawn3,
	ECC_Pawn4,
};

constexpr FColor PlayerColors[]
{
	FColor(255, 0, 0),
	FColor(0, 255, 0),
	FColor(0, 0, 255),
	FColor(255, 255, 0),
};

inline float RoundUnitCenter(float Num)
{
    return FMath::RoundToNegativeInfinity(Num / Unit) * Unit + Unit / 2;
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
		return 0;
	}
}

}
