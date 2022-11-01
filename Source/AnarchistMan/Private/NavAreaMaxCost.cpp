// Fill out your copyright notice in the Description page of Project Settings.


#include "NavAreaMaxCost.h"

UNavAreaMaxCost::UNavAreaMaxCost(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    //DefaultCost = BIG_NUMBER;
    FixedAreaEnteringCost = MAX_FLT;
}