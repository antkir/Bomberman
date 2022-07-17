// Fill out your copyright notice in the Description page of Project Settings.

#include "OverviewCamera.h"

#include <Camera/CameraComponent.h>

// Sets default values
AOverviewCamera::AOverviewCamera()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set camera component
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	
	RootComponent = CameraComponent;
}
