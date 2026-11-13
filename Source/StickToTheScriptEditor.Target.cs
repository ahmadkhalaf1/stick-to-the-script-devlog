using UnrealBuildTool;

public class StickToTheScriptEditorTarget : TargetRules
{
	public StickToTheScriptEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("StickToTheScript");
	}
}
