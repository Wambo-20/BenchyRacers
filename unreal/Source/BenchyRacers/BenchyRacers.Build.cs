using UnrealBuildTool;
public class BenchyRacers : ModuleRules {
    public BenchyRacers(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent", "Slate", "SlateCore", "HTTP", "Sockets", "Networking", "ApplicationCore"
        });
    }
}
