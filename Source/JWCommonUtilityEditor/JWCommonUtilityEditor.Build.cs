// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class JWCommonUtilityEditor : ModuleRules
{
	public JWCommonUtilityEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "Slate", "SlateCore", "PropertyEditor", "DeveloperSettings", "JWCommonUtility", "UnrealEd", "GraphEditor", "BlueprintGraph", "Kismet", "ToolMenus", "InputCore", "Projects" });
	}
}
