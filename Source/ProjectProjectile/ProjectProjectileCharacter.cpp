// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectProjectileCharacter.h"
#include "ProjectProjectileProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "ManipulateProjectile.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

// ------------------------------------------------------------
// Character setup and initialization
// ------------------------------------------------------------

AProjectProjectileCharacter::AProjectProjectileCharacter()
{
	// Set the collision capsule size for the player's character.
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create the first-person camera and attach it to the capsule.
	// The camera follows the player's controller rotation.
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create the first-person arms mesh.
	// Only the player can see this mesh, preventing it from
	// appearing in views belonging to other characters.
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	// Tick is required to update the held projectile's position
	// and continuously check for available capture targets.
	PrimaryActorTick.bCanEverTick = true;
}

void AProjectProjectileCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Start each play session with the player's maximum health.
	CurrentHealth = MaxHealth;
}

// ------------------------------------------------------------
// Player input
// ------------------------------------------------------------

void AProjectProjectileCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Use Enhanced Input to connect input actions to character functions.
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Start jumping when the input is pressed and stop when released.
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Update movement and camera rotation while input is received.
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AProjectProjectileCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AProjectProjectileCharacter::Look);

		// Bind the capture action if it has been assigned in the Blueprint.
		if (CaptureAction)
		{
			EnhancedInputComponent->BindAction(
				CaptureAction,
				ETriggerEvent::Started,
				this,
				&AProjectProjectileCharacter::AttemptCapture
			);
		}

		// Bind the release action separately so capturing and
		// redirecting can be controlled with different inputs.
		if (ReleaseAction)
		{
			EnhancedInputComponent->BindAction(
				ReleaseAction,
				ETriggerEvent::Started,
				this,
				&AProjectProjectileCharacter::AttemptRelease
			);
		}
	}
	else
	{
		// Report an error if the character does not have the
		// expected Enhanced Input component.
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AProjectProjectileCharacter::Move(const FInputActionValue& Value)
{
	// Enhanced Input provides movement as a two-dimensional vector.
	// The Y value controls forward/backward movement and X controls strafing.
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AProjectProjectileCharacter::Look(const FInputActionValue& Value)
{
	// Read horizontal and vertical camera input.
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Rotate the camera through the player's controller.
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

// ------------------------------------------------------------
// Projectile detection and capture
// ------------------------------------------------------------

AManipulateProjectile* AProjectProjectileCharacter::FindCapturableProjectile()
{
	// A camera is required because the capture search uses
	// the direction the player is currently looking.
	if (!FirstPersonCameraComponent)
	{
		return nullptr;
	}

	const FVector CameraLocation =
		FirstPersonCameraComponent->GetComponentLocation();

	const FVector CameraForward =
		FirstPersonCameraComponent->GetForwardVector();

	// Determine the endpoint of the capture search.
	const FVector TraceEnd =
		CameraLocation + CameraForward * CaptureRange;

	TArray<FHitResult> HitResults;

	// Only search for objects using the WorldDynamic collision channel.
	// This includes our projectile collision components.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	// Ignore the player's own collision so it cannot interfere
	// with the search for nearby projectiles.
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	// Sweep a sphere forward from the camera.
	// The radius provides some forgiveness so the player does
	// not need to aim directly at the center of a projectile.
	const bool bFound = GetWorld()->SweepMultiByObjectType(
		HitResults,
		CameraLocation,
		TraceEnd,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(100.0f),
		QueryParams
	);

	AManipulateProjectile* BestProjectile = nullptr;
	float BestDistance = CaptureRange + 1.0f;

	if (bFound)
	{
		for (const FHitResult& Hit : HitResults)
		{
			// Check whether the detected actor is one of our
			// custom projectiles.
			AManipulateProjectile* Projectile =
				Cast<AManipulateProjectile>(Hit.GetActor());

			// Ignore invalid projectiles and projectiles that
			// have already been captured or redirected.
			if (!IsValid(Projectile) || !Projectile->CanBeCaptured())
			{
				continue;
			}

			const float Distance = FVector::Dist(
				CameraLocation,
				Projectile->GetActorLocation()
			);

			// Select the closest eligible projectile.
			// The distance check also ensures that the selected
			// projectile is inside the intended capture range.
			if (Distance <= CaptureRange && Distance < BestDistance)
			{
				BestProjectile = Projectile;
				BestDistance = Distance;
			}
		}
	}

	return BestProjectile;
}

void AProjectProjectileCharacter::AttemptCapture()
{
	// Prevent the player from holding more than one projectile.
	if (IsValid(CapturedProjectile))
	{
		return;
	}

	AManipulateProjectile* BestProjectile = FindCapturableProjectile();

	if (BestProjectile)
	{
		// Tell the projectile to stop moving and enter its
		// captured state before storing a reference to it.
		BestProjectile->CaptureProjectile();
		CapturedProjectile = BestProjectile;

		UE_LOG(LogTemp, Warning, TEXT("Player captured a projectile!"));
	}
}

// ------------------------------------------------------------
// Per-frame projectile updates
// ------------------------------------------------------------

void AProjectProjectileCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Keep a captured projectile positioned directly in front
	// of the player's camera as the player moves or looks around.
	if (IsValid(CapturedProjectile))
	{
		const FVector CameraLocation =
			FirstPersonCameraComponent->GetComponentLocation();

		const FVector CameraForward =
			FirstPersonCameraComponent->GetForwardVector();

		const FVector HoldLocation =
			CameraLocation + CameraForward * HoldDistance;

		CapturedProjectile->SetActorLocation(HoldLocation);
	}
	else
	{
		// Clear the stored reference if the projectile was
		// destroyed or is otherwise no longer valid.
		CapturedProjectile = nullptr;
	}

	// Check whether an eligible projectile is available.
	// The result is exposed to Blueprints for HUD functionality.
	const bool bNewCaptureReady =
		(!IsValid(CapturedProjectile) && IsValid(FindCapturableProjectile()));

	bCaptureReady = bNewCaptureReady;

	// Display a temporary visual indicator when a projectile
	// is in range. A fixed message key lets subsequent updates
	// replace the existing message rather than create new ones.
	if (GEngine)
	{
		if (bCaptureReady)
		{
			GEngine->AddOnScreenDebugMessage(
				100,
				0.0f,
				FColor::Green,
				TEXT("CAPTURE READY")
			);
		}
		else
		{
			// Replace the previous message with blank text
			// when there is no projectile available to capture.
			GEngine->AddOnScreenDebugMessage(
				100,
				0.0f,
				FColor::White,
				TEXT(" ")
			);
		}
	}
}

// ------------------------------------------------------------
// Projectile release
// ------------------------------------------------------------

void AProjectProjectileCharacter::AttemptRelease()
{
	// There is nothing to release if the player is not
	// currently holding a valid projectile.
	if (!IsValid(CapturedProjectile))
	{
		return;
	}

	// Use the camera direction so the player can aim where
	// the captured projectile will travel after release.
	const FVector LaunchDirection =
		FirstPersonCameraComponent->GetForwardVector();

	// The projectile handles restoring its movement and collision.
	CapturedProjectile->ReleaseProjectile(LaunchDirection);

	// Clear the reference so another projectile can be captured.
	CapturedProjectile = nullptr;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Player released projectile!")
	);
}

// ------------------------------------------------------------
// Health and damage
// ------------------------------------------------------------

float AProjectProjectileCharacter::TakeDamage(
	float DamageAmount,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	// Ignore damage after defeat and reject invalid damage amounts.
	if (CurrentHealth <= 0.0f || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	// Limit the damage to the player's remaining health so
	// CurrentHealth never drops below zero.
	const float ActualDamage = FMath::Min(DamageAmount, CurrentHealth);

	CurrentHealth -= ActualDamage;

	// Log health changes for testing and debugging.
	// The gameplay HUD reads CurrentHealth directly.
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Player Health: %.0f / %.0f"),
		CurrentHealth,
		MaxHealth
	);

	if (CurrentHealth <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player defeated!"));

		// Disable movement and player input when health reaches zero.
		// This provides a basic defeat state without requiring
		// a separate game-over screen for the prototype.
		GetCharacterMovement()->DisableMovement();
		DisableInput(Cast<APlayerController>(GetController()));
	}

	return ActualDamage;
}
