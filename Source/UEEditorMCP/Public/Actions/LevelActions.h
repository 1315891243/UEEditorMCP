// Copyright (c) 2025 zolnoor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorAction.h"

class ULevelScriptBlueprint;

// ============================================================================
// Level Management Actions
// ============================================================================

/**
 * FCreateLevelAction
 *
 * Creates a new, empty level (map) asset at the specified path.
 * The newly created level is NOT automatically opened — use open_level for that.
 *
 * Parameters:
 *   - path (required): Content path for the level, e.g. "/Game/Maps/MyLevel"
 *   - template (optional): "Empty" (default), "Default" (with floor/light/sky)
 *
 * Returns:
 *   - path: The created level's content path
 *   - filename: On-disk filename
 *
 * Command type: create_level
 */
class UEEDITORMCP_API FCreateLevelAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("create_level"); }
};


/**
 * FOpenLevelAction
 *
 * Opens (loads) an existing level in the editor.
 * This replaces the currently loaded level.
 *
 * Parameters:
 *   - path (required): Content path of the level to open, e.g. "/Game/Maps/MyLevel"
 *
 * Returns:
 *   - path: The opened level's content path
 *   - world_name: The UWorld name
 *
 * Command type: open_level
 */
class UEEDITORMCP_API FOpenLevelAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("open_level"); }
};


/**
 * FSaveLevelAction
 *
 * Saves the currently loaded level. If the level has never been saved,
 * a path must be provided.
 *
 * Parameters:
 *   - path (optional): Content path to save as. If omitted, saves in-place.
 *
 * Returns:
 *   - path: The saved level's content path
 *
 * Command type: save_level
 */
class UEEDITORMCP_API FSaveLevelAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override { return true; }
	virtual FString GetActionName() const override { return TEXT("save_level"); }
	virtual bool RequiresSave() const override { return false; }
};


/**
 * FGetCurrentLevelAction
 *
 * Returns information about the currently loaded level (map).
 * No parameters required.
 *
 * Returns:
 *   - world_name: UWorld name
 *   - map_name: Map/Package name
 *   - path: Content path
 *   - is_dirty: Whether the level has unsaved changes
 *   - has_level_blueprint: Whether a Level Blueprint exists
 *   - level_blueprint_name: Name of the Level Blueprint (if exists)
 *
 * Command type: get_current_level
 */
class UEEDITORMCP_API FGetCurrentLevelAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override { return true; }
	virtual FString GetActionName() const override { return TEXT("get_current_level"); }
	virtual bool RequiresSave() const override { return false; }
};


// ============================================================================
// Level Blueprint Actions
// ============================================================================

/**
 * FGetLevelBlueprintAction
 *
 * Gets a reference to the current level's Level Blueprint.
 * After calling this, you can use all existing Blueprint graph commands
 * (add_blueprint_event_node, connect_blueprint_nodes, apply_graph_patch, etc.)
 * by passing the returned blueprint_name.
 *
 * The Level Blueprint is also set as the "current blueprint" in the MCP context,
 * so subsequent commands can omit blueprint_name.
 *
 * Parameters:
 *   - none (operates on the currently loaded level)
 *
 * Returns:
 *   - blueprint_name: Name of the Level Blueprint (for use in other commands)
 *   - blueprint_class: Class name (LevelScriptBlueprint)
 *   - graphs: Array of graph names in the Level Blueprint
 *   - has_event_graph: Whether an EventGraph exists
 *
 * Command type: get_level_blueprint
 *
 * Example workflow:
 *   1. get_level_blueprint → returns {"blueprint_name": "MyLevel_ScriptBlueprint"}
 *   2. add_blueprint_event_node → params: {blueprint_name: "MyLevel_ScriptBlueprint", event_name: "ReceiveBeginPlay"}
 *   3. add_blueprint_function_node → params: {blueprint_name: "MyLevel_ScriptBlueprint", ...}
 *   4. connect_blueprint_nodes → ...
 *   5. compile_blueprint → params: {blueprint_name: "MyLevel_ScriptBlueprint"}
 */
class UEEDITORMCP_API FGetLevelBlueprintAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override { return true; }
	virtual FString GetActionName() const override { return TEXT("get_level_blueprint"); }
	virtual bool RequiresSave() const override { return false; }
};


/**
 * FOpenLevelBlueprintEditorAction
 *
 * Opens the Level Blueprint editor UI for the current level.
 * This is equivalent to the user clicking "Open Level Blueprint" in the toolbar.
 *
 * Parameters:
 *   - none
 *
 * Returns:
 *   - blueprint_name: Name of the Level Blueprint
 *   - opened: true if the editor was successfully opened
 *
 * Command type: open_level_blueprint_editor
 */
class UEEDITORMCP_API FOpenLevelBlueprintEditorAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override { return true; }
	virtual FString GetActionName() const override { return TEXT("open_level_blueprint_editor"); }
	virtual bool RequiresSave() const override { return false; }
};
