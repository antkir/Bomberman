// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#define ECC_GameExplosion ECC_GameTraceChannel1

DECLARE_LOG_CATEGORY_EXTERN(LogGame, Log, All);

namespace Utils {

constexpr float Unit = 100.f;

inline float RoundUnitCenter(float Num)
{
    return FMath::RoundToNegativeInfinity(Num / Unit) * Unit + Unit / 2;
}

}
