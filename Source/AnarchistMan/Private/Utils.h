// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#define ECC_GameExplosion ECC_GameTraceChannel1
#define ECC_Pawn1 ECC_GameTraceChannel2
#define ECC_Pawn2 ECC_GameTraceChannel3
#define ECC_Pawn3 ECC_GameTraceChannel4
#define ECC_Pawn4 ECC_GameTraceChannel5

DECLARE_LOG_CATEGORY_EXTERN(LogGame, Log, All);

namespace Utils {

constexpr float Unit = 100.f;

constexpr ECollisionChannel PawnECCs[] = {
	ECC_Pawn1,
	ECC_Pawn2,
	ECC_Pawn3,
	ECC_Pawn4,
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
	default:
		return 8;
	}
}

}
