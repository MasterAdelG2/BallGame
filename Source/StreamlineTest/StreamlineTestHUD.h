// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "StreamlineTestHUD.generated.h"

UCLASS()
class AStreamlineTestHUD : public AHUD
{
	GENERATED_BODY()

public:
	AStreamlineTestHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

