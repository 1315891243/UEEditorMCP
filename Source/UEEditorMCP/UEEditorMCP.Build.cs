// Copyright (c) 2025 zolnoor. All rights reserved.

using System.Reflection;
using UnrealBuildTool;

public class UEEditorMCP : ModuleRules
{
	public UEEditorMCP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"InputCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"EditorSubsystem",
			"BlueprintGraph",
			"Kismet",
			"KismetCompiler",
			"GraphEditor",
			"Json",
			"JsonUtilities",
			"Networking",
			"Sockets",
			"UMG",
			"UMGEditor",
			"EnhancedInput",
			"InputBlueprintNodes",
			"EditorScriptingUtilities",
			"AssetTools",
			"SourceControl",      // For Diff Against Depot (ISourceControlModule, ISourceControlProvider, etc.)
			"MaterialEditor",     // For UMaterialEditingLibrary and material expression manipulation
			"RenderCore",         // For material shader compilation
			"RHI",                // For GMaxRHIShaderPlatform (compile diagnostics)
			"ModelViewViewModel", // MVVM runtime types (available since UE5.2 experimental)
		});

		// ModelViewViewModelBlueprint and FieldNotification were restructured in UE5.3.
		// Guard them so the plugin compiles cleanly on UE5.2 as well.
		// Use reflection because the version property name varies across UE versions:
		//   UE 5.2 exposes MinorVersion, later versions may expose MinorEngineVersion.
		if (GetMinorEngineVersion(Target.Version) >= 3)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"ModelViewViewModelBlueprint",  // MVVM editor-time binding API (UMVVMBlueprintView, etc.)
				"FieldNotification",            // INotifyFieldValueChanged interface
			});
		}

		// Ensure proper RTTI/exceptions for crash handling
		bUseRTTI = true;
		bEnableExceptions = true;
	}

	/// <summary>
	/// Retrieve the minor engine version from ReadOnlyBuildVersion via reflection
	/// so that the same .Build.cs compiles on UE 5.2 through 5.7+.
	/// </summary>
	private static int GetMinorEngineVersion(object buildVersion)
	{
		System.Type type = buildVersion.GetType();
		PropertyInfo prop = type.GetProperty("MinorVersion")
						 ?? type.GetProperty("MinorEngineVersion");
		if (prop != null)
		{
			return System.Convert.ToInt32(prop.GetValue(buildVersion));
		}
		return 0;
	}
}
