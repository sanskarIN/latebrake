using UnrealBuildTool;

public class Latebrake : ModuleRules
{
    public Latebrake(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "PhysicsCore"
        });
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "ChaosVehicles"
        });
    }
}
