// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

// Forward declaration for the projectile class used by enemies.
class AManipulateProjectile;

/**
 * Enemy character used in Project: Projectile.
 *
 * The enemy remains stationary and periodically fires projectiles
 * toward the player when they are within range. These projectiles
 * can be captured and redirected back at the enemy.
 *
 * The enemy also tracks its own health and can be defeated by
 * taking enough damage from redirected projectiles.
 */
UCLASS()
class PROJECTPROJECTILE_API AEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:

    AEnemyCharacter();

protected:

    /**
     * Initializes the enemy's health and starts the timer
     * responsible for repeatedly attempting to fire projectiles.
     */
    virtual void BeginPlay() override;

    /**
     * Projectile class that the enemy spawns when attacking.
     * Assigned through the enemy Blueprint so different
     * projectile types can be used without changing C++ code.
     */
    UPROPERTY(EditAnywhere, Category = "Combat")
    TSubclassOf<AManipulateProjectile> ProjectileClass;

    /**
     * Time in seconds between attempts to fire a projectile.
     * The enemy will only fire if the player is within range.
     */
    UPROPERTY(EditAnywhere, Category = "Combat")
    float FireRate = 2.0f;

    /**
     * Maximum distance at which the enemy can attack the player.
     * Measured in Unreal units, which are centimeters by default.
     */
    UPROPERTY(EditAnywhere, Category = "Combat")
    float FireRange = 1500.0f;

    /**
     * Stores the timer used to trigger repeated firing attempts.
     * The timer is cleared when the enemy is defeated.
     */
    FTimerHandle FireTimerHandle;

    /**
     * Attempts to fire a projectile toward the player.
     * Checks that the player exists, that a projectile class
     * has been assigned, and that the player is within range.
     */
    void FireProjectile();

    /**
     * Maximum health available to the enemy.
     * Can be adjusted in the Blueprint to change its durability.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float MaxHealth = 100.0f;

    /**
     * Enemy's remaining health.
     * Reduced when redirected projectiles deal damage.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
    float CurrentHealth = 100.0f;

    /**
     * Processes damage received by the enemy.
     *
     * Updates the enemy's remaining health and destroys the
     * character when its health reaches zero. The firing timer
     * is cleared before destruction so the defeated enemy
     * can no longer spawn projectiles.
     *
     * @return The nonnegative damage amount accepted by the function,
     *         or zero if the enemy is defeated or the damage is invalid.
     */
    virtual float TakeDamage(
        float DamageAmount,
        struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator,
        AActor* DamageCauser
    ) override;
};
