// Fill out your copyright notice in the Description page of Project Settings.


#include "NavGrid.h"

#include "Utils.h"

// Sets default values
ANavGrid::ANavGrid()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ANavGrid::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ANavGrid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
