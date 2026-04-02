"""
Automated integration tests for the 6 LevelActions commands.

Requires UE 5.2 editor running with UEEditorMCP plugin active on port 55558.
The /Game/ Content directory should be empty before running.
"""

import json
import socket
import sys
import time

HOST = "127.0.0.1"
PORT = 55558
TIMEOUT = 30


def _recv_exact(sock: socket.socket, n: int) -> bytes:
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("Connection closed while receiving")
        buf += chunk
    return buf


def send_command(command_type: str, params: dict | None = None) -> dict:
    command = {"type": command_type}
    if params:
        command["params"] = params
    payload = json.dumps(command).encode("utf-8")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(TIMEOUT)
        sock.connect((HOST, PORT))
        sock.sendall(len(payload).to_bytes(4, byteorder="big"))
        sock.sendall(payload)
        length_bytes = _recv_exact(sock, 4)
        length = int.from_bytes(length_bytes, byteorder="big")
        body = _recv_exact(sock, length)
        return json.loads(body.decode("utf-8"))


def wait_for_connection(max_retries: int = 5, delay: float = 2.0) -> bool:
    for i in range(max_retries):
        try:
            result = send_command("ping")
            if result.get("status") == "success" or result.get("result", {}).get("pong"):
                return True
        except Exception:
            pass
        time.sleep(delay)
    return False


def is_success(r: dict) -> bool:
    return r.get("success") is True or r.get("status") == "success"


# ─── Test definitions ────────────────────────────────────────────────

passed = 0
failed = 0
errors = []


def check(test_name: str, condition: bool, detail: str = ""):
    global passed, failed
    if condition:
        passed += 1
        print(f"  [PASS] {test_name}")
    else:
        failed += 1
        msg = f"  [FAIL] {test_name}" + (f" — {detail}" if detail else "")
        print(msg)
        errors.append(msg)


def test_get_current_level():
    print("\n=== Test 1: get_current_level ===")
    r = send_command("get_current_level")
    check("returns success", is_success(r), json.dumps(r, indent=2))

    data = r.get("result", r)
    check("has world_name", "world_name" in data, str(data.keys()))
    check("has map_name", "map_name" in data)
    check("has is_dirty", "is_dirty" in data)
    check("has has_level_blueprint", "has_level_blueprint" in data)
    print(f"  Current level: {data.get('world_name', '?')} at {data.get('path', '?')}")
    return data


def test_get_level_blueprint():
    print("\n=== Test 2: get_level_blueprint ===")
    r = send_command("get_level_blueprint")
    check("returns success", is_success(r), json.dumps(r, indent=2))

    data = r.get("result", r)
    check("has blueprint_name", "blueprint_name" in data)
    check("has blueprint_class", "blueprint_class" in data)
    check("is LevelScriptBlueprint", data.get("blueprint_class") == "LevelScriptBlueprint",
          data.get("blueprint_class", "?"))
    check("has graphs", "graphs" in data and isinstance(data.get("graphs"), list))
    check("has EventGraph", data.get("has_event_graph") is True)

    bp_name = data.get("blueprint_name", "")
    print(f"  Level BP: {bp_name}, graphs: {data.get('graphs', [])}")
    return bp_name


def test_open_level_blueprint_editor():
    print("\n=== Test 3: open_level_blueprint_editor ===")
    r = send_command("open_level_blueprint_editor")
    check("returns success", is_success(r), json.dumps(r, indent=2))

    data = r.get("result", r)
    check("has blueprint_name", "blueprint_name" in data)
    check("opened is true", data.get("opened") is True)


def test_save_level_temp():
    """save_level on a temporary (/Temp/) level without path should return error."""
    print("\n=== Test 4: save_level on temp level (no path) ===")
    r = send_command("save_level")
    check("returns error for temp level",
          r.get("success") is False or r.get("status") == "error",
          json.dumps(r, indent=2))
    data = r.get("result", r)
    check("error mentions 'path' parameter",
          "path" in r.get("error", "").lower(),
          r.get("error", ""))


def test_save_level_with_path():
    """save_level with explicit path (save-as) should work."""
    print("\n=== Test 5: save_level with path (save-as) ===")
    r = send_command("save_level", {"path": "/Game/Maps/SaveAsTest"})
    check("returns success", is_success(r), json.dumps(r, indent=2))
    data = r.get("result", r)
    check("has path", "path" in data)
    check("has filename", "filename" in data)


def test_create_level():
    print("\n=== Test 5: create_level ===")

    # 5a: Create a new level (does NOT switch editor — just writes .umap file)
    r = send_command("create_level", {"path": "/Game/Maps/TestAutoLevel"})
    check("create returns success", is_success(r), json.dumps(r, indent=2))
    data = r.get("result", r)
    check("path matches", data.get("path") == "/Game/Maps/TestAutoLevel",
          data.get("path", "?"))
    check("has filename", "filename" in data and ".umap" in data.get("filename", ""))

    # 5b: Editor should still be on the original level (not switched)
    r2 = send_command("get_current_level")
    check("get_current_level works after create", is_success(r2), json.dumps(r2, indent=2))

    # 5c: Duplicate creation should fail
    r3 = send_command("create_level", {"path": "/Game/Maps/TestAutoLevel"})
    check("duplicate create returns error",
          r3.get("success") is False or r3.get("status") == "error",
          json.dumps(r3, indent=2))

    # 5d: Create a second level for open_level test later
    r4 = send_command("create_level", {"path": "/Game/Maps/TestAutoLevel2"})
    check("second create returns success", is_success(r4), json.dumps(r4, indent=2))


def test_save_level_in_place():
    """save_level without path on a named level (not /Temp/) should work."""
    print("\n=== Test 7: save_level in-place ===")
    r = send_command("save_level")
    check("returns success", is_success(r), json.dumps(r, indent=2))
    data = r.get("result", r)
    check("has path", "path" in data)
    check("has filename", "filename" in data)


def test_create_second_level():
    """Create a second level so we can test open_level."""
    print("\n=== Test 8: create second level ===")
    r = send_command("create_level", {"path": "/Game/Maps/TestAutoLevel2"})
    check("second create returns success", is_success(r), json.dumps(r, indent=2))

    time.sleep(1)
    if not wait_for_connection(max_retries=10, delay=2.0):
        check("MCP available after second create", False, "Could not reconnect")
        return
    check("MCP available after second create", True)


def test_open_level():
    print("\n=== Test 9: open_level ===")

    # Open the first level we created
    try:
        r = send_command("open_level", {"path": "/Game/Maps/TestAutoLevel"})
        success = is_success(r)
        check("open returns success", success, json.dumps(r, indent=2))
    except ConnectionError:
        # LoadMap may sever the connection before the response arrives
        print("  (connection severed during LoadMap — expected)")

    # Wait for the editor to stabilise after LoadMap
    print("  Waiting for editor to stabilise after LoadMap...")
    time.sleep(2)
    if not wait_for_connection(max_retries=15, delay=2.0):
        check("MCP reconnects after open_level", False, "Could not reconnect")
        return

    check("MCP reconnects after open_level", True)

    # Verify we're on the right level
    r2 = send_command("get_current_level")
    data = r2.get("result", r2)
    current = data.get("world_name", data.get("map_name", ""))
    check("opened level is TestAutoLevel",
          "TestAutoLevel" in current and "2" not in current,
          f"current: {current}")
    print(f"  Now on level: {current}")


def test_level_blueprint_on_new_level():
    print("\n=== Test 10: get_level_blueprint on opened level ===")
    r = send_command("get_level_blueprint")
    check("returns success", is_success(r), json.dumps(r, indent=2))
    data = r.get("result", r)
    check("has blueprint_name", "blueprint_name" in data)
    check("is LevelScriptBlueprint", data.get("blueprint_class") == "LevelScriptBlueprint")

    # Verify the level BP works with existing blueprint commands
    bp_name = data.get("blueprint_name", "")
    if bp_name:
        print(f"  Testing add_blueprint_event_node with Level BP '{bp_name}'...")
        r2 = send_command("add_blueprint_event_node", {
            "blueprint_name": bp_name,
            "event_name": "ReceiveBeginPlay"
        })
        check("add_blueprint_event_node on Level BP", is_success(r2),
              json.dumps(r2, indent=2))


# ─── Main ────────────────────────────────────────────────────────────

def main():
    global passed, failed

    print("=" * 60)
    print("UEEditorMCP — Level Actions Integration Tests")
    print("=" * 60)

    # Pre-flight
    print("\nConnecting to MCP server...")
    if not wait_for_connection():
        print("ERROR: Cannot connect to MCP server. Is UE editor running?")
        sys.exit(1)
    print("Connected!")

    # Run tests in order (some depend on prior state)
    try:
        test_get_current_level()           # 1: query current level
        test_get_level_blueprint()         # 2: get Level BP reference
        test_open_level_blueprint_editor() # 3: open Level BP editor
        test_save_level_temp()             # 4: save on temp → error
        test_create_level()                # 5: create levels (file-only, no switch)
        test_open_level()                  # 6: open the first created level
        test_save_level_in_place()         # 7: save in-place (now on named level)
        test_level_blueprint_on_new_level()  # 8: Level BP on opened level
    except ConnectionError as e:
        print(f"\n[CONNECTION ERROR] {e}")
        print("Attempting to reconnect...")
        if wait_for_connection(max_retries=10, delay=2.0):
            print("Reconnected — some tests may have been skipped.")
        else:
            print("Could not reconnect. Editor may have crashed.")
            failed += 1

    # Summary
    total = passed + failed
    print("\n" + "=" * 60)
    print(f"Results: {passed}/{total} passed, {failed} failed")
    if errors:
        print("\nFailures:")
        for e in errors:
            print(e)
    print("=" * 60)

    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
