// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT
using UnrealBuildTool;
public class JWCommonUtilityAudioTest : ModuleRules
{
    public JWCommonUtilityAudioTest(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "JWCommonUtilityAudio", "UMG", "Slate", "SlateCore" });
        if (Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "BlueprintGraph", "KismetCompiler" });
    }
}
