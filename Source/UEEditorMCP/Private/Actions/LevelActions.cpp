// Copyright (c) 2025 zolnoor. All rights reserved.

#include "Actions/LevelActions.h"
#include "MCPCommonUtils.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/LevelScriptBlueprint.h"
#include "Engine/Level.h"
#include "Model.h"
#include "FileHelpers.h"
#include "EditorLevelUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "Subsystems/AssetEditorSubsystem.h"

// ============================================================================
// Helpers
// ============================================================================

namespace LevelActionHelpers
{
	/** Get the editor world safely */
	static UWorld* GetEditorWorld()
	{
		if (!GEditor)
		{
			return nullptr;
		}
		return GEditor->GetEditorWorldContext().World();
	}

	/** Get the Level Script Blueprint for the current persistent level */
	static ULevelScriptBlueprint* GetLevelScriptBlueprint(bool bCreateIfNotFound = true)
	{
		UWorld* World = GetEditorWorld();
		if (!World || !World->PersistentLevel)
		{
			return nullptr;
		}
		return World->PersistentLevel->GetLevelScriptBlueprint(bCreateIfNotFound);
	}
}


// ============================================================================
// FCreateLevelAction
// ============================================================================

bool FCreateLevelAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Path;
	if (!GetRequiredString(Params, TEXT("path"), Path, OutError))
	{
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FCreateLevelAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Path = Params->GetStringField(TEXT("path"));

	// Resolve the filename on disk
	FString PackageFilename;
	if (!FPackageName::TryConvertLongPackageNameToFilename(Path, PackageFilename, FPackageName::GetMapPackageExtension()))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Invalid level path: %s"), *Path));
	}

	// Check if file already exists on disk
	if (IFileManager::Get().FileExists(*PackageFilename))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Level already exists at path: %s"), *Path));
	}

	// Ensure the output directory exists
	FString Dir = FPaths::GetPath(PackageFilename);
	IFileManager::Get().MakeDirectory(*Dir, true);

	// Create a package with the target path name for saving.
	UPackage* Package = CreatePackage(*Path);
	if (!Package)
	{
		return CreateErrorResponse(TEXT("Failed to create level package"));
	}

	// Create a minimal UWorld using NewObject — NOT UWorld::CreateWorld.
	// UWorld::CreateWorld initializes tick groups, physics, AI subsystems etc.
	// which creates dangling references that crash during LoadMap cleanup.
	// NewObject<UWorld> creates a bare UWorld that SavePackage can serialize
	// as a valid .umap but avoids all the engine registration side-effects.
	FName WorldName = FName(*FPackageName::GetShortName(Path));
	UWorld* NewWorld = NewObject<UWorld>(Package, WorldName);
	NewWorld->WorldType = EWorldType::None;
	NewWorld->SetFlags(RF_Public | RF_Standalone);

	// Create a minimal persistent level for the world
	ULevel* PersistentLevel = NewObject<ULevel>(NewWorld, TEXT("PersistentLevel"));
	PersistentLevel->OwningWorld = NewWorld;
	PersistentLevel->Model = NewObject<UModel>(PersistentLevel);
	PersistentLevel->Model->Initialize(nullptr, false);
	NewWorld->PersistentLevel = PersistentLevel;

	// Save the package
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	bool bSaved = UPackage::SavePackage(Package, NewWorld, *PackageFilename, SaveArgs);

	// Cleanup: Make the temp world and package collectible.
	// Since we used NewObject (not CreateWorld), there are no tick/physics
	// registrations to tear down.
	NewWorld->ClearFlags(RF_Public | RF_Standalone | RF_Transactional);
	NewWorld->SetFlags(RF_Transient);
	Package->ClearFlags(RF_Standalone);
	Package->SetFlags(RF_Transient);

	// Force GC to reclaim immediately so LoadMap never sees stale worlds
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

	if (!bSaved)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to save level to: %s"), *PackageFilename));
	}

	// Notify asset registry about the new on-disk file
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FString> ScanPaths;
	ScanPaths.Add(PackageFilename);
	ARM.Get().ScanFilesSynchronous(ScanPaths, /*bForceRescan=*/true);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("path"), Path);
	ResultData->SetStringField(TEXT("filename"), PackageFilename);

	return CreateSuccessResponse(ResultData);
}


// ============================================================================
// FOpenLevelAction
// ============================================================================

bool FOpenLevelAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Path;
	if (!GetRequiredString(Params, TEXT("path"), Path, OutError))
	{
		return false;
	}

	// Check if the level exists
	if (!FPackageName::DoesPackageExist(Path))
	{
		OutError = FString::Printf(TEXT("Level not found: %s"), *Path);
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FOpenLevelAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Path = Params->GetStringField(TEXT("path"));

	// Resolve to filename
	FString MapFilename;
	if (!FPackageName::TryConvertLongPackageNameToFilename(Path, MapFilename, FPackageName::GetMapPackageExtension()))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Cannot resolve level path: %s"), *Path));
	}

	// Clear context before loading new level
	Context.Clear();

	// Load the level using the official editor API
	FEditorFileUtils::LoadMap(MapFilename);

	// Verify the level loaded
	UWorld* World = LevelActionHelpers::GetEditorWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("Level loaded but editor world is null"));
	}

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("path"), Path);
	ResultData->SetStringField(TEXT("world_name"), World->GetName());

	return CreateSuccessResponse(ResultData);
}


// ============================================================================
// FSaveLevelAction
// ============================================================================

TSharedPtr<FJsonObject> FSaveLevelAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	UWorld* World = LevelActionHelpers::GetEditorWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"));
	}

	UPackage* Package = World->GetOutermost();
	if (!Package)
	{
		return CreateErrorResponse(TEXT("Cannot get world package"));
	}

	FString PackageName = Package->GetName();

	// Check if caller provided a specific save path
	FString SavePath = GetOptionalString(Params, TEXT("path"));

	if (!SavePath.IsEmpty())
	{
		// Save-as to a new path using the official editor API
		bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, SavePath);
		if (!bSaved)
		{
			return CreateErrorResponse(TEXT("Failed to save level"));
		}

		FString PackageFilename;
		FPackageName::TryConvertLongPackageNameToFilename(SavePath, PackageFilename, FPackageName::GetMapPackageExtension());

		TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
		ResultData->SetStringField(TEXT("path"), SavePath);
		ResultData->SetStringField(TEXT("filename"), PackageFilename);
		return CreateSuccessResponse(ResultData);
	}
	else
	{
		// Save in place
		if (PackageName.StartsWith(TEXT("/Temp/")))
		{
			return CreateErrorResponse(TEXT("Current level is an unsaved temporary level. Provide a 'path' parameter for save-as."));
		}

		bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, PackageName);
		if (!bSaved)
		{
			return CreateErrorResponse(TEXT("Failed to save level"));
		}

		FString PackageFilename;
		FPackageName::TryConvertLongPackageNameToFilename(PackageName, PackageFilename, FPackageName::GetMapPackageExtension());

		TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
		ResultData->SetStringField(TEXT("path"), PackageName);
		ResultData->SetStringField(TEXT("filename"), PackageFilename);
		return CreateSuccessResponse(ResultData);
	}
}


// ============================================================================
// FGetCurrentLevelAction
// ============================================================================

TSharedPtr<FJsonObject> FGetCurrentLevelAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	UWorld* World = LevelActionHelpers::GetEditorWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"));
	}

	UPackage* Package = World->GetOutermost();

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("world_name"), World->GetName());
	ResultData->SetStringField(TEXT("map_name"), Package ? Package->GetName() : TEXT("Unknown"));
	ResultData->SetStringField(TEXT("path"), Package ? Package->GetName() : TEXT(""));
	ResultData->SetBoolField(TEXT("is_dirty"), Package ? Package->IsDirty() : false);

	// Level Blueprint info
	ULevelScriptBlueprint* LevelBP = LevelActionHelpers::GetLevelScriptBlueprint(false);
	ResultData->SetBoolField(TEXT("has_level_blueprint"), LevelBP != nullptr);
	if (LevelBP)
	{
		ResultData->SetStringField(TEXT("level_blueprint_name"), LevelBP->GetName());
	}

	return CreateSuccessResponse(ResultData);
}


// ============================================================================
// FGetLevelBlueprintAction
// ============================================================================

TSharedPtr<FJsonObject> FGetLevelBlueprintAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	UWorld* World = LevelActionHelpers::GetEditorWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"));
	}

	// Get or create the Level Blueprint
	ULevelScriptBlueprint* LevelBP = LevelActionHelpers::GetLevelScriptBlueprint(true);
	if (!LevelBP)
	{
		return CreateErrorResponse(TEXT("Failed to get Level Script Blueprint"));
	}

	// Set as current blueprint in context so subsequent commands can omit blueprint_name
	Context.SetCurrentBlueprint(LevelBP);

	// Collect graph names
	TArray<TSharedPtr<FJsonValue>> GraphNames;
	bool bHasEventGraph = false;

	for (UEdGraph* Graph : LevelBP->UbergraphPages)
	{
		if (Graph)
		{
			GraphNames.Add(MakeShared<FJsonValueString>(Graph->GetName()));
			if (Graph->GetFName() == TEXT("EventGraph"))
			{
				bHasEventGraph = true;
			}
		}
	}
	for (UEdGraph* Graph : LevelBP->FunctionGraphs)
	{
		if (Graph)
		{
			GraphNames.Add(MakeShared<FJsonValueString>(Graph->GetName()));
		}
	}
	for (UEdGraph* Graph : LevelBP->MacroGraphs)
	{
		if (Graph)
		{
			GraphNames.Add(MakeShared<FJsonValueString>(Graph->GetName()));
		}
	}

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("blueprint_name"), LevelBP->GetName());
	ResultData->SetStringField(TEXT("blueprint_class"), LevelBP->GetClass()->GetName());
	ResultData->SetArrayField(TEXT("graphs"), GraphNames);
	ResultData->SetBoolField(TEXT("has_event_graph"), bHasEventGraph);

	return CreateSuccessResponse(ResultData);
}


// ============================================================================
// FOpenLevelBlueprintEditorAction
// ============================================================================

TSharedPtr<FJsonObject> FOpenLevelBlueprintEditorAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	UWorld* World = LevelActionHelpers::GetEditorWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"));
	}

	ULevelScriptBlueprint* LevelBP = LevelActionHelpers::GetLevelScriptBlueprint(true);
	if (!LevelBP)
	{
		return CreateErrorResponse(TEXT("Failed to get Level Script Blueprint"));
	}

	// Open the Blueprint editor for the Level Blueprint
	if (GEditor)
	{
		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (AssetEditorSubsystem)
		{
			AssetEditorSubsystem->OpenEditorForAsset(LevelBP);
		}
	}

	// Also set as current in context
	Context.SetCurrentBlueprint(LevelBP);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("blueprint_name"), LevelBP->GetName());
	ResultData->SetBoolField(TEXT("opened"), true);

	return CreateSuccessResponse(ResultData);
}
