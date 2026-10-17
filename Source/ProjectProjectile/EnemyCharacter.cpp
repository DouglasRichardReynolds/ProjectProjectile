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

    CurrentHealth = MaxHealth;
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

float AEnemyCharacter::TakeDamage(
    float DamageAmount,
    FDamageEvent const& DamageEvent,
    AController* EventInstigator,
    AActor* DamageCauser
)
{
    const float ActualDamage = FMath::Max(0.0f, DamageAmount);

    if (CurrentHealth <= 0.0f || ActualDamage <= 0.0f)
    {
        return 0.0f;
    }

    CurrentHealth = FMath::Max(0.0f, CurrentHealth - ActualDamage);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Enemy Health: %.0f / %.0f"),
        CurrentHealth,
        MaxHealth
    );

    if (CurrentHealth <= 0.0f)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Enemy defeated!")
        );

        GetWorldTimerManager().ClearTimer(FireTimerHandle);

        Destroy();
    }

    return ActualDamage;
}