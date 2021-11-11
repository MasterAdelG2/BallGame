// Copyright Epic Games, Inc. All Rights Reserved.

#include "StreamlineTestGameMode.h"
#include "StreamlineTestHUD.h"
#include "StreamlineTestCharacter.h"
#include "UObject/ConstructorHelpers.h"

AStreamlineTestGameMode::AStreamlineTestGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AStreamlineTestHUD::StaticClass();
}
