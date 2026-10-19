using UnrealBuildTool;
using System.Collections.Generic;

public class LatebrakeEditorTarget : TargetRules
{
    public LatebrakeEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        CppStandard = CppStandardVersion.Cpp20;
        ExtraModuleNames.Add("Latebrake");
    }
}
