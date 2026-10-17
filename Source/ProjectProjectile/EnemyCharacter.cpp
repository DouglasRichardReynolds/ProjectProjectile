// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyCharacter.h"
#include "ManipulateProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"

// ------------------------------------------------------------
// Enemy setup and initialization
// ------------------------------------------------------------

AEnemyCharacter::AEnemyCharacter()
{
    // The enemy uses a repeating timer to fire projectiles,
    // so it does not need to run custom logic every frame.
    PrimaryActorTick.bCanEverTick = false;
}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Start a repeating timer that attempts to fire a projectile
    // every FireRate seconds. The FireProjectile function handles
    // checking whether the player is within attack range.
    GetWorldTimerManager().SetTimer(
        FireTimerHandle,
        this,
        &AEnemyCharacter::FireProjectile,
        FireRate,
        true
    );

    // Initialize health using the configurable maximum value.
    CurrentHealth = MaxHealth;
}

// ------------------------------------------------------------
// Enemy targeting and projectile spawning
// ------------------------------------------------------------

void AEnemyCharacter::FireProjectile()
{
    // Find the player pawn and confirm that a projectile class
    // has been assigned before attempting to spawn anything.
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);

    if (!Player || !ProjectileClass)
    {
        return;
    }

    const FVector EnemyLocation = GetActorLocation();
    const FVector PlayerLocation = Player->GetActorLocation();

    // Only attack when the player is close enough.
    // The enemy does not pursue the player when out of range.
    if (FVector::Dist(EnemyLocation, PlayerLocation) > FireRange)
    {
        return;
    }

    // Calculate the direction from the enemy toward the player.
    // Normalizing the vector produces a direction independent
    // of the distance between the two characters.
    const FVector DirectionToPlayer =
        (PlayerLocation - EnemyLocation).GetSafeNormal();

    // Spawn the projectile slightly in front of the enemy
    // and above the ground.
    //
    // This offset is important because spawning directly inside
    // the enemy's collision capsule could cause the projectile
    // to collide with its owner immediately.
    const FVector SpawnLocation =
        EnemyLocation +
        (DirectionToPlayer * 120.0f) +
        FVector(0.0f, 0.0f, 60.0f);

    // Aim toward a point above the player's actor location.
    // This helps direct the projectile toward the player's body
    // rather than toward the ground near their feet.
    const FRotator SpawnRotation =
        (PlayerLocation + FVector(0.0f, 0.0f, 50.0f) - SpawnLocation).Rotation();

    // Record which enemy created the projectile.
    // Ownership is used by the projectile to ignore its
    // original firing enemy while traveling normally.
    //
    // Once the projectile is captured and redirected, that
    // enemy can become a valid target for its own attack.
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;

    // Spawn the projectile with the calculated location,
    // rotation, and ownership information.
    //
    // The projectile class handles its own movement and
    // collision behavior after it has been spawned.
    GetWorld()->SpawnActor<AManipulateProjectile>(
        ProjectileClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );
}

// ------------------------------------------------------------
// Enemy health and damage
// ------------------------------------------------------------

float AEnemyCharacter::TakeDamage(
    float DamageAmount,
    FDamageEvent const& DamageEvent,
    AController* EventInstigator,
    AActor* DamageCauser
)
{
    // Reject negative damage by converting it to zero.
    const float ActualDamage = FMath::Max(0.0f, DamageAmount);

    // Ignore damage if the enemy has already been defeated
    // or if there is no positive damage to apply.
    if (CurrentHealth <= 0.0f || ActualDamage <= 0.0f)
    {
        return 0.0f;
    }

    // Subtract damage while preventing health from dropping
    // below zero, even if the incoming damage is greater
    // than the enemy's remaining health.
    CurrentHealth = FMath::Max(0.0f, CurrentHealth - ActualDamage);

    // Log the remaining health for testing and debugging.
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

        // Stop future firing attempts before removing the enemy.
        // This prevents the defeated character from continuing
        // to attack while the actor is being destroyed.
        GetWorldTimerManager().ClearTimer(FireTimerHandle);

        // Remove the defeated enemy from the level.
        Destroy();
    }

    // Return the damage amount accepted by the function.
    // This can be greater than the enemy's remaining health
    // before the hit; health itself is clamped to zero.
    return ActualDamage;
}
