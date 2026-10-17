// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ProjectProjectileCharacter.generated.h"

// Forward declarations for components and classes used by the player.
class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class AManipulateProjectile;

struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 * Player character for Project: Projectile.
 *
 * Handles the player's movement, camera controls, health, and
 * interactions with incoming projectiles. The main gameplay
 * mechanic allows the player to capture enemy projectiles,
 * hold them in front of the camera, and redirect them.
 */
UCLASS(config = Game)
class PROJECTPROJECTILE_API AProjectProjectileCharacter : public ACharacter
{
	GENERATED_BODY()

	/** First-person arms that are only visible to the local player. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh1P;

	/** Camera used for the player's view and projectile targeting. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Input action responsible for jumping. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Input action responsible for player movement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

public:

	AProjectProjectileCharacter();

	/**
	 * Updates the character every frame.
	 * Keeps captured projectiles in position and checks whether
	 * another incoming projectile is available to capture.
	 */
	virtual void Tick(float DeltaTime) override;

	/** Maximum health the player can have. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.0f;

	/**
	 * Player's remaining health.
	 * This value is also exposed to Blueprints so the HUD
	 * can display the player's current health.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 100.0f;

	/**
	 * Processes damage received by the player.
	 * Reduces health and disables movement and input when
	 * the player's health reaches zero.
	 *
	 * @return The amount of damage actually applied.
	 */
	virtual float TakeDamage(
		float DamageAmount,
		const FDamageEvent& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	) override;

protected:

	/** Initializes the player's health when gameplay begins. */
	virtual void BeginPlay() override;

	/** Input action used to capture incoming projectiles. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* CaptureAction = nullptr;

	/** Input action used to release and redirect a held projectile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ReleaseAction = nullptr;

	/**
	 * Maximum distance, in Unreal units, at which a projectile
	 * can be captured. The search starts at the player camera.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Capture")
	float CaptureRange = 500.0f;

	/**
	 * Distance that a captured projectile is held in front
	 * of the camera while waiting to be released.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Capture")
	float HoldDistance = 150.0f;

	/**
	 * Reference to the projectile currently held by the player.
	 * A null reference means the player is not holding anything.
	 */
	UPROPERTY()
	AManipulateProjectile* CapturedProjectile = nullptr;

	/**
	 * Attempts to capture a projectile.
	 * Only succeeds if the player is not already holding one
	 * and a valid projectile is within capture range.
	 */
	void AttemptCapture();

	/**
	 * Indicates whether a projectile is currently available
	 * to capture. Exposed to Blueprints for HUD functionality.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile Capture")
	bool bCaptureReady = false;

	/**
	 * Searches for the closest capturable projectile in front
	 * of the player using a sphere sweep.
	 *
	 * @return A valid projectile or nullptr if none are found.
	 */
	AManipulateProjectile* FindCapturableProjectile();

public:

	/** Input action responsible for looking around. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

protected:

	/** Processes directional movement input. */
	void Move(const FInputActionValue& Value);

	/** Processes camera rotation input. */
	void Look(const FInputActionValue& Value);

	/**
	 * Releases the currently captured projectile in the
	 * direction the player's camera is facing.
	 */
	void AttemptRelease();

protected:

	/**
	 * Connects player input actions to their corresponding
	 * gameplay functions using Unreal's Enhanced Input system.
	 */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:

	/** Returns the first-person arms mesh. */
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }

	/** Returns the camera used by the player. */
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
};
