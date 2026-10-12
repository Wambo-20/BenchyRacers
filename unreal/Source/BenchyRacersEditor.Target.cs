using UnrealBuildTool;
using System.Collections.Generic;
public class BenchyRacersEditorTarget : TargetRules {
    public BenchyRacersEditorTarget(TargetInfo Target) : base(Target) {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("BenchyRacers");
    }
}
