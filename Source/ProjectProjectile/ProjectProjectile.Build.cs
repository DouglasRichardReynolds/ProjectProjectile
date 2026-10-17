// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectProjectile : ModuleRules
{
	public ProjectProjectile(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
