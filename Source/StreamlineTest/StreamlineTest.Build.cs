// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class StreamlineTest : ModuleRules
{
	public StreamlineTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
