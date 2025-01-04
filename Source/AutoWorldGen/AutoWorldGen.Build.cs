// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AutoWorldGen : ModuleRules
{
	public AutoWorldGen(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
            "Landscape",
			"AssetTools",
			"UnrealEd",
			"EditorScriptingUtilities",
			"Json",
            "JsonUtilities"
        });
	}
}
