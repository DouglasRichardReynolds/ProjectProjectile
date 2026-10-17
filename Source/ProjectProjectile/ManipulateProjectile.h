// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ManipulateProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

UENUM(BlueprintType)
enum class EProjectileState : uint8
{
    Flying,
    Captured,
    Redirected
};

UCLASS()
class PROJECTPROJECTILE_API AManipulateProjectile : public AActor
{
    GENERATED_BODY()

public:
    AManipulateProjectile();

    // Current state of the projectile
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
    EProjectileState ProjectileState = EProjectileState::Flying;

    // Stops the projectile so it can be captured
    UFUNCTION(BlueprintCallable, Category = "Projectile")
    void CaptureProjectile();

    // Returns whether the projectile is available to capture
    UFUNCTION(BlueprintPure, Category = "Projectile")
    bool CanBeCaptured() const;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnProjectileHit(
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        FVector NormalImpulse,
        const FHitResult& Hit
    );

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* ProjectileMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    float Damage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    float ProjectileSpeed = 1200.0f;
};
