// Fill out your copyright notice in the Description page of Project Settings.

#include "ManipulateProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

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
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Projectile collision detected!")
    );

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            5.0f,
            FColor::Red,
            TEXT("PROJECTILE HIT!")
        );
    }

    if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
    {
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Projectile hit: %s"),
        *OtherActor->GetName()
    );

    Destroy();
}