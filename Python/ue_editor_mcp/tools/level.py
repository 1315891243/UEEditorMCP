"""
Level tools - Level management and Level Blueprint operations.
"""

import json
from typing import Any
from mcp.types import Tool, TextContent

from ..connection import get_connection, CommandResult


def _send_command(command_type: str, params: dict | None = None) -> list[TextContent]:
    """Helper to send command and format response."""
    conn = get_connection()
    if not conn.is_connected:
        conn.connect()
    result = conn.send_command(command_type, params)
    return [TextContent(type="text", text=json.dumps(result.to_dict(), indent=2))]


def get_tools() -> list[Tool]:
    """Get all level tools."""
    return [
        # Level management
        Tool(
            name="create_level",
            description="Create a new empty level (map) asset at the specified content path.",
            inputSchema={
                "type": "object",
                "properties": {
                    "path": {"type": "string", "description": "Content path for the level, e.g. /Game/Maps/MyLevel"},
                    "template": {"type": "string", "description": "Template type: 'Empty' (default) or 'Default'"}
                },
                "required": ["path"]
            }
        ),
        Tool(
            name="open_level",
            description="Open (load) an existing level in the editor. Replaces the currently loaded level.",
            inputSchema={
                "type": "object",
                "properties": {
                    "path": {"type": "string", "description": "Content path of the level, e.g. /Game/Maps/MyLevel"}
                },
                "required": ["path"]
            }
        ),
        Tool(
            name="save_level",
            description="Save the currently loaded level. Optionally save to a new path.",
            inputSchema={
                "type": "object",
                "properties": {
                    "path": {"type": "string", "description": "Content path to save as (optional, saves in-place if omitted)"}
                }
            }
        ),
        Tool(
            name="get_current_level",
            description="Get information about the currently loaded level: name, path, dirty state, and Level Blueprint info.",
            inputSchema={"type": "object", "properties": {}}
        ),

        # Level Blueprint
        Tool(
            name="get_level_blueprint",
            description=(
                "Get a reference to the current level's Level Blueprint. "
                "Returns blueprint_name for use with all existing Blueprint graph commands "
                "(add_blueprint_event_node, connect_blueprint_nodes, etc.)."
            ),
            inputSchema={"type": "object", "properties": {}}
        ),
        Tool(
            name="open_level_blueprint_editor",
            description="Open the Level Blueprint editor UI for the current level.",
            inputSchema={"type": "object", "properties": {}}
        ),
    ]


# Map tool names to command types
TOOL_HANDLERS = {
    "create_level": "create_level",
    "open_level": "open_level",
    "save_level": "save_level",
    "get_current_level": "get_current_level",
    "get_level_blueprint": "get_level_blueprint",
    "open_level_blueprint_editor": "open_level_blueprint_editor",
}


async def handle_tool(name: str, arguments: dict[str, Any]) -> list[TextContent]:
    """Handle a level tool call."""
    command_type = TOOL_HANDLERS.get(name)
    if not command_type:
        return [TextContent(type="text", text=f'{{"success": false, "error": "Unknown tool: {name}"}}')]

    return _send_command(command_type, arguments if arguments else None)
