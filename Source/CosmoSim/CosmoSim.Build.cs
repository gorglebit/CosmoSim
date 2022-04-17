// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CosmoSim : ModuleRules
{
	public CosmoSim(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
