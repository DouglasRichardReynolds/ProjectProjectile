// Fill out your copyright notice in the Description page of Project Settings.

#include "ManipulateProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectProjectileCharacter.h"
#include "EnemyCharacter.h"

// ------------------------------------------------------------
// Projectile setup
// ------------------------------------------------------------

AManipulateProjectile::AManipulateProjectile()
{
    // Projectile movement is handled by ProjectileMovement,
    // so the actor does not need to update every frame.
    PrimaryActorTick.bCanEverTick = false;

    // Create the collision sphere that determines when the
    // projectile makes contact with other objects.
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(
        TEXT("CollisionComponent")
    );

    CollisionComponent->InitSphereRadius(20.0f);
    CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    // Enable hit notifications so collisions can trigger
    // the OnProjectileHit function.
    CollisionComponent->SetNotifyRigidBodyCollision(true);

    // Make the collision sphere the root component so
    // projectile movement is based on its collision shape.
    RootComponent = CollisionComponent;

    // Create a separate mesh to visually represent the projectile.
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("ProjectileMesh")
    );

    ProjectileMesh->SetupAttachment(CollisionComponent);

    // Disable mesh collision because the sphere component
    // is responsible for detecting projectile impacts.
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Create the movement component used to move the projectile
    // without requiring custom movement calculations every frame.
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(
        TEXT("ProjectileMovement")
    );

    ProjectileMovement->UpdatedComponent = CollisionComponent;

    // Set the initial movement values.
    // These are also configured using ProjectileSpeed in BeginPlay.
    ProjectileMovement->InitialSpeed = 1200.0f;
    ProjectileMovement->MaxSpeed = 1200.0f;

    // Rotate the projectile to face its movement direction
    // and prevent it from bouncing after a collision.
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;

    // Destroy projectiles that have not been captured or
    // redirected within five seconds.
    InitialLifeSpan = 5.0f;
}

void AManipulateProjectile::BeginPlay()
{
    Super::BeginPlay();

    // Ensure the collision component generates hit events.
    CollisionComponent->SetNotifyRigidBodyCollision(true);

    // Register the collision callback after the actor has spawned.
    // AddUniqueDynamic prevents the same callback from being
    // registered more than once.
    CollisionComponent->OnComponentHit.AddUniqueDynamic(
        this,
        &AManipulateProjectile::OnProjectileHit
    );

    // Apply the configurable projectile speed to the movement component.
    ProjectileMovement->InitialSpeed = ProjectileSpeed;
    ProjectileMovement->MaxSpeed = ProjectileSpeed;

    // Launch the projectile in its forward direction.
    // The enemy sets the projectile's rotation when spawning it.
    ProjectileMovement->Velocity =
        GetActorForwardVector() * ProjectileSpeed;

    ProjectileMovement->Activate(true);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Projectile spawned and collision event registered!")
    );
}

// ------------------------------------------------------------
// Projectile collision and damage
// ------------------------------------------------------------

void AManipulateProjectile::OnProjectileHit(
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    FVector NormalImpulse,
    const FHitResult& Hit
)
{
    // Captured projectiles should not cause damage or be destroyed.
    // Collision is also disabled when a projectile is captured,
    // but this provides an additional safeguard.
    if (ProjectileState == EProjectileState::Captured)
    {
        return;
    }

    // Ignore collisions that do not involve a valid actor
    // or that somehow reference the projectile itself.
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    // A newly fired projectile should not immediately collide
    // with the enemy that created it.
    //
    // This check only applies while the projectile is flying.
    // Once redirected, the original enemy must become a valid
    // target so the player can return its attack.
    if (ProjectileState == EProjectileState::Flying &&
        OtherActor == GetOwner())
    {
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Projectile hit: %s"),
        *OtherActor->GetName()
    );

    // Normal enemy-fired projectiles only apply damage
    // when they collide with the player character.
    if (ProjectileState == EProjectileState::Flying)
    {
        if (OtherActor->IsA(AProjectProjectileCharacter::StaticClass()))
        {
            // Use Unreal's damage system so the player character
            // can handle health reduction through TakeDamage().
            UGameplayStatics::ApplyDamage(
                OtherActor,
                20.0f,
                nullptr,
                this,
                UDamageType::StaticClass()
            );
        }
    }
    else if (ProjectileState == EProjectileState::Redirected)
    {
        // Redirected projectiles can damage enemies instead
        // of the player, supporting the central gameplay mechanic.
        if (OtherActor->IsA(AEnemyCharacter::StaticClass()))
        {
            // Use the configurable Damage value so enemy damage
            // can be adjusted without modifying this function.
            UGameplayStatics::ApplyDamage(
                OtherActor,
                Damage,
                nullptr,
                this,
                UDamageType::StaticClass()
            );
        }
    }

    // Once a valid collision has been processed, destroy the
    // projectile to prevent repeated impacts or damage.
    Destroy();
}

// ------------------------------------------------------------
// Projectile capture
// ------------------------------------------------------------

void AManipulateProjectile::CaptureProjectile()
{
    // Only flying projectiles are eligible for capture.
    // Captured or redirected projectiles cannot be captured again.
    if (!CanBeCaptured())
    {
        return;
    }

    // Change the projectile's state before modifying its behavior.
    ProjectileState = EProjectileState::Captured;

    // Stop any existing velocity and disable the movement component
    // so the projectile no longer travels toward the player.
    ProjectileMovement->StopMovementImmediately();
    ProjectileMovement->Deactivate();

    // Disable collision while the projectile is being held.
    // This prevents it from hitting the player or nearby objects.
    CollisionComponent->SetCollisionEnabled(
        ECollisionEnabled::NoCollision
    );

    // Remove the projectile's expiration timer.
    // Otherwise, a captured projectile could disappear
    // while the player is holding it.
    SetLifeSpan(0.0f);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Projectile captured!")
    );
}

bool AManipulateProjectile::CanBeCaptured() const
{
    // Only projectiles in their original flying state
    // can be captured by the player.
    return ProjectileState == EProjectileState::Flying;
}

// ------------------------------------------------------------
// Projectile redirection
// ------------------------------------------------------------

void AManipulateProjectile::ReleaseProjectile(const FVector& Direction)
{
    // Prevent projectiles from being redirected unless
    // they have already been captured.
    if (ProjectileState != EProjectileState::Captured)
    {
        return;
    }

    // Change the state so future collisions will damage enemies.
    ProjectileState = EProjectileState::Redirected;

    // Normalize the direction to ensure projectile speed
    // is not affected by the length of the input vector.
    const FVector LaunchDirection = Direction.GetSafeNormal();

    // Restore collision so the projectile can interact
    // with enemies and other objects again.
    CollisionComponent->SetCollisionEnabled(
        ECollisionEnabled::QueryAndPhysics
    );

    // Rotate the projectile so its forward direction
    // matches the direction chosen by the player.
    SetActorRotation(LaunchDirection.Rotation());

    // Restore velocity using the projectile's configured speed
    // and reactivate the movement component.
    ProjectileMovement->Velocity =
        LaunchDirection * ProjectileSpeed;

    ProjectileMovement->Activate(true);

    // Restore the five-second lifetime so released projectiles
    // do not remain in the world indefinitely.
    SetLifeSpan(5.0f);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Projectile redirected!")
    );
}
