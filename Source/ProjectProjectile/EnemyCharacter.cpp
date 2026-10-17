// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyCharacter.h"
#include "ManipulateProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"

AEnemyCharacter::AEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    GetWorldTimerManager().SetTimer(
        FireTimerHandle,
        this,
        &AEnemyCharacter::FireProjectile,
        FireRate,
        true
    );
}

void AEnemyCharacter::FireProjectile()
{
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);

    if (!Player || !ProjectileClass)
    {
        return;
    }

    const FVector EnemyLocation = GetActorLocation();
    const FVector PlayerLocation = Player->GetActorLocation();

    if (FVector::Dist(EnemyLocation, PlayerLocation) > FireRange)
    {
        return;
    }


    const FVector DirectionToPlayer =
        (PlayerLocation - EnemyLocation).GetSafeNormal();

    const FVector SpawnLocation =
        EnemyLocation +
        (DirectionToPlayer * 120.0f) +
        FVector(0.0f, 0.0f, 60.0f);

    const FRotator SpawnRotation =
        (PlayerLocation + FVector(0.0f, 0.0f, 50.0f) - SpawnLocation).Rotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;

    GetWorld()->SpawnActor<AManipulateProjectile>(
        ProjectileClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );
}
