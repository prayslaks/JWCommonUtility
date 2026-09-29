// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class JWCommonUtility : ModuleRules
{
	public JWCommonUtility(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemLibraries.Add("Crypt32.lib");
		}

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
				"CoreUObject",
				"Engine",
				"UMG",							// Widget Blueprint
				"GameplayTags"					// FGameplayTag, FGameplayTagContainer
			]
		);

		PrivateDependencyModuleNames.AddRange(
			[
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
