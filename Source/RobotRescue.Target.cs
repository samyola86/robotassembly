using UnrealBuildTool;
public class RobotRescueTarget : TargetRules { public RobotRescueTarget(TargetInfo Target) : base(Target) { Type = TargetType.Game; DefaultBuildSettings = BuildSettingsVersion.Latest; ExtraModuleNames.Add("RobotRescue"); } }
