// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectProjectileGameMode.h"
#include "ProjectProjectileCharacter.h"
#include "UObject/ConstructorHelpers.h"

AProjectProjectileGameMode::AProjectProjectileGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
