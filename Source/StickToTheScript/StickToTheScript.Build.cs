using UnrealBuildTool;

public class StickToTheScript : ModuleRules
{
	public StickToTheScript(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AudioMixer",
			"Json",
			"JsonUtilities", "Slate", "SlateCore"
		});
	}
}
