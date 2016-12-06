// By Polyakov Pavel

using UnrealBuildTool;

public class PracticeProject : ModuleRules
{
    public PracticeProject(TargetInfo Target)
    {
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "Engine", "InputCore", "AIModule", "GameplayTasks", "RHI", "RenderCore", "ShaderCore" });
        PrivateDependencyModuleNames.AddRange(new string[] {  });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
        
        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");
        // if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
        // {
        //      if (UEBuildConfiguration.bCompileSteamOSS == true)
        //      {
        //          DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
        //      }
        // }
    }
}
