// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BallGame : ModuleRules
{
	public BallGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
