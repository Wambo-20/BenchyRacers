using UnrealBuildTool;
using System.Collections.Generic;
public class BenchyRacersTarget : TargetRules {
    public BenchyRacersTarget(TargetInfo Target) : base(Target) {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("BenchyRacers");
    }
}
