// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Paradise : ModuleRules
{
	public Paradise(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // 기본 필수 모듈 (Core + Enhanced Input)
        PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"EnhancedInput"
			});
        // GAS (Gameplay Ability System)
        PublicDependencyModuleNames.AddRange(new string[] {
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks"
			});

        // UI (UMG & Slate)
        PublicDependencyModuleNames.AddRange(new string[] {
			"UMG",
            "CommonUI", 
			"CommonInput"
            });


        // AI & Navigation
        PublicDependencyModuleNames.AddRange(new string[] {
			"AIModule",
			"NavigationSystem",
			});

        // FX (Niagara)
        PublicDependencyModuleNames.AddRange(new string[] {
		"Niagara"
		});

        //  Cinematics & Sequence (가챠 연출용)
        PublicDependencyModuleNames.AddRange(new string[] {
            "LevelSequence",
            "MovieScene"
        });

        // Private 의존성 (구현부에서만 필요한 모듈)
        PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore"
			});

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
