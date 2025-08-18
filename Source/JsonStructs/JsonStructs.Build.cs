// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JsonStructs : ModuleRules
{
	public JsonStructs(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new[] {
			"Core",
			// ... add other public dependencies that you statically link with here ...
		});

		PrivateDependencyModuleNames.AddRange(new[] {
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"Json",
			"AssetRegistry"
			// ... add private dependencies that you statically link with here ...
		});
	}
}
