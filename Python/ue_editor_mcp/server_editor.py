"""
UE Editor MCP Server — level, viewport, and asset management.

Covers: level management, Level Blueprint, actor CRUD, transforms, viewport control, save, assets, blueprint summary, auto-layout.
"""

from .server_factory import create_mcp_server
from .tools import editor, level

server, main = create_mcp_server("ue-editor-mcp", [editor, level])

if __name__ == "__main__":
    main()
