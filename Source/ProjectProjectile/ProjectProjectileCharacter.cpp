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

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AProjectProjectileCharacter

AProjectProjectileCharacter::AProjectProjectileCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	PrimaryActorTick.bCanEverTick = true;

}

void AProjectProjectileCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

//////////////////////////////////////////////////////////////////////////// Input

void AProjectProjectileCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AProjectProjectileCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AProjectProjectileCharacter::Look);

		// Capturing
		if (CaptureAction)
		{
			EnhancedInputComponent->BindAction(
				CaptureAction,
				ETriggerEvent::Started,
				this,
				&AProjectProjectileCharacter::AttemptCapture
			);
		}

		// Releasing
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
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AProjectProjectileCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AProjectProjectileCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

AManipulateProjectile* AProjectProjectileCharacter::FindCapturableProjectile()
{
	if (!FirstPersonCameraComponent)
	{
		return nullptr;
	}

	const FVector CameraLocation =
		FirstPersonCameraComponent->GetComponentLocation();

	const FVector CameraForward =
		FirstPersonCameraComponent->GetForwardVector();

	const FVector TraceEnd =
		CameraLocation + CameraForward * CaptureRange;

	TArray<FHitResult> HitResults;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

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
			AManipulateProjectile* Projectile =
				Cast<AManipulateProjectile>(Hit.GetActor());

			if (!IsValid(Projectile) || !Projectile->CanBeCaptured())
			{
				continue;
			}

			const float Distance = FVector::Dist(
				CameraLocation,
				Projectile->GetActorLocation()
			);

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
	if (IsValid(CapturedProjectile))
	{
		return;
	}

	AManipulateProjectile* BestProjectile = FindCapturableProjectile();

	if (BestProjectile)
	{
		BestProjectile->CaptureProjectile();
		CapturedProjectile = BestProjectile;

		UE_LOG(LogTemp, Warning, TEXT("Player captured a projectile!"));
	}
}

void AProjectProjectileCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Keep a captured projectile in front of the player
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
		CapturedProjectile = nullptr;
	}

	// Check whether a projectile is available to capture
	const bool bNewCaptureReady =
		(!IsValid(CapturedProjectile) && IsValid(FindCapturableProjectile()));

	bCaptureReady = bNewCaptureReady;

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
			GEngine->AddOnScreenDebugMessage(
				100,
				0.0f,
				FColor::White,
				TEXT(" ")
			);
		}
	}
}

void AProjectProjectileCharacter::AttemptRelease()
{
	if (!IsValid(CapturedProjectile))
	{
		return;
	}

	const FVector LaunchDirection =
		FirstPersonCameraComponent->GetForwardVector();

	CapturedProjectile->ReleaseProjectile(LaunchDirection);

	CapturedProjectile = nullptr;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Player released projectile!")
	);
}
