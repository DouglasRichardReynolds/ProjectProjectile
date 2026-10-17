// Fill out your copyright notice in the Description page of Project Settings.

#include "ManipulateProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AManipulateProjectile::AManipulateProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    // Collision
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(
        TEXT("CollisionComponent")
    );

    CollisionComponent->InitSphereRadius(20.0f);
    CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    CollisionComponent->SetNotifyRigidBodyCollision(true);

    RootComponent = CollisionComponent;

    // Visible mesh
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("ProjectileMesh")
    );

    ProjectileMesh->SetupAttachment(CollisionComponent);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Movement
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(
        TEXT("ProjectileMovement")
    );

    ProjectileMovement->UpdatedComponent = CollisionComponent;

    ProjectileMovement->InitialSpeed = 1200.0f;
    ProjectileMovement->MaxSpeed = 1200.0f;

    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;

    InitialLifeSpan = 5.0f;
}

void AManipulateProjectile::BeginPlay()
{
    Super::BeginPlay();

    // Make sure collision hit events are enabled.
    CollisionComponent->SetNotifyRigidBodyCollision(true);

    // Register the collision event when the projectile spawns.
    CollisionComponent->OnComponentHit.AddUniqueDynamic(
        this,
        &AManipulateProjectile::OnProjectileHit
    );

    // Initialize projectile movement.
    ProjectileMovement->InitialSpeed = ProjectileSpeed;
    ProjectileMovement->MaxSpeed = ProjectileSpeed;

    ProjectileMovement->Velocity =
        GetActorForwardVector() * ProjectileSpeed;

    ProjectileMovement->Activate(true);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Projectile spawned and collision event registered!")
    );
}

void AManipulateProjectile::OnProjectileHit(
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    FVector NormalImpulse,
    const FHitResult& Hit
)
{
    if (ProjectileState == EProjectileState::Captured)
    {
        return;
    }

    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    // Ignore the original owner only while the projectile is flying.
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

    // Only redirected projectiles deal damage.
    if (ProjectileState == EProjectileState::Redirected)
    {
        UGameplayStatics::ApplyDamage(
            OtherActor,
            Damage,
            nullptr,
            this,
            UDamageType::StaticClass()
        );
    }

    Destroy();
}

void AManipulateProjectile::CaptureProjectile()
{
    if (!CanBeCaptured())
    {
        return;
    }

    ProjectileState = EProjectileState::Captured;

    // Stop the projectile immediately.
    ProjectileMovement->StopMovementImmediately();
    ProjectileMovement->Deactivate();

    // Prevent the projectile from hitting anything while held.
    CollisionComponent->SetCollisionEnabled(
        ECollisionEnabled::NoCollision
    );

    // Keep the projectile alive while captured.
    SetLifeSpan(0.0f);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Projectile captured!")
    );
}

bool AManipulateProjectile::CanBeCaptured() const
{
    return ProjectileState == EProjectileState::Flying;
}

void AManipulateProjectile::ReleaseProjectile(const FVector& Direction)
{
    if (ProjectileState != EProjectileState::Captured)
    {
        return;
    }

    ProjectileState = EProjectileState::Redirected;

    const FVector LaunchDirection = Direction.GetSafeNormal();

    // Re-enable collision.
    CollisionComponent->SetCollisionEnabled(
        ECollisionEnabled::QueryAndPhysics
    );

    // Point the projectile toward the target.
    SetActorRotation(LaunchDirection.Rotation());

    // Resume projectile movement.
    ProjectileMovement->Velocity =
        LaunchDirection * ProjectileSpeed;

    ProjectileMovement->Activate(true);

    // Restore the projectile's lifetime.
    SetLifeSpan(5.0f);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Projectile redirected!")
    );
}
