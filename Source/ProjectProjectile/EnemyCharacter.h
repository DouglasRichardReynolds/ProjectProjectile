// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class AManipulateProjectile;

UCLASS()
class PROJECTPROJECTILE_API AEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemyCharacter();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category = "Combat")
    TSubclassOf<AManipulateProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float FireRate = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float FireRange = 1500.0f;

    FTimerHandle FireTimerHandle;

    void FireProjectile();
};
