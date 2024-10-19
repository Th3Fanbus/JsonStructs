// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JsonStructs : ModuleRules
{
	public JsonStructs(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
				
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Json",
				"AssetRegistry"
				// ... add private dependencies that you statically link with here ...	
			}
			);
	}
}
