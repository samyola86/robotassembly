using UnrealBuildTool;
public class RobotRescueEditorTarget : TargetRules { public RobotRescueEditorTarget(TargetInfo Target) : base(Target) { Type = TargetType.Editor; DefaultBuildSettings = BuildSettingsVersion.Latest; ExtraModuleNames.Add("RobotRescue"); } }
