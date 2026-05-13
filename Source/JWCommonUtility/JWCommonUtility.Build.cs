// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

using UnrealBuildTool;

public class JWCommonUtility : ModuleRules
{
	public JWCommonUtility(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			[
				// ... add public include paths required here ...
			]
		);

		PrivateIncludePaths.AddRange(
			[
				// ... add other private include paths required here ...
			]
		);

		PublicDependencyModuleNames.AddRange(
			[
				"Core",
				"UMG",							// Widget Blueprint
				"GameplayTags"					// FGameplayTag, FGameplayTagContainer
			]
		);

		PrivateDependencyModuleNames.AddRange(
			[
				"CoreUObject",
				"Engine",						// AActor, UCurveFloat, DrawDebug
				"Slate",
				"SlateCore",
			]
		);

		DynamicallyLoadedModuleNames.AddRange(
			[
				// ... add any modules that your module loads dynamically here ...
			]
		);
	}
}
