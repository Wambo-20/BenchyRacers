using UnrealBuildTool;
public class BenchyRacers : ModuleRules {
    public BenchyRacers(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent", "Slate", "SlateCore", "HTTP", "Sockets", "Networking", "ApplicationCore", "ImageWrapper", "ImageCore", "Json", "EOSShared", "SocketSubsystemEOS", "OnlineSubsystemUtils"
        });
        if(Target.Platform==UnrealTargetPlatform.Win64)PublicSystemLibraries.Add("Comdlg32.lib");
        AddEngineThirdPartyPrivateStaticDependencies(Target,"EOSSDK");
    }
}
