// Copyright Epic Games, Inc. All Rights Reserved.

#include "CosmoSimGameMode.h"
#include "CosmoSimCharacter.h"
#include "UObject/ConstructorHelpers.h"

ACosmoSimGameMode::ACosmoSimGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
