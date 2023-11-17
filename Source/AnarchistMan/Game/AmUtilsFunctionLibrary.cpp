// Copyright 2022 Kiryl Antonik

#include "AmUtilsFunctionLibrary.h"

int64 UAmUtilsFunctionLibrary::GetValue(ETileType TileType)
{
	int64 Value = TNumericLimits<int64>::Max();
	switch (TileType)
	{
	case ETileType::DEFAULT:
		Value = ETileNavCost::DEFAULT;
		break;
	case ETileType::BLOCK:
		Value = ETileNavCost::BLOCK;
		break;
	case ETileType::BOMB:
		Value = ETileNavCost::BOMB;
		break;
	default:
		UE_LOG(LogGame, Error, TEXT("Incorrect ETileType value!"));
	}
	return Value;
}