// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

using UnrealBuildTool;

public class JWCommonUtilityEditor : ModuleRules
{
	public JWCommonUtilityEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
		[
			"Core"
		]);

		PrivateDependencyModuleNames.AddRange(
		[
			"AssetRegistry",
			"CoreUObject",
			"Engine",
			"UnrealEd"
		]);
	}
}
