#!/usr/bin/env python3
"""
Reset WBP_LocalMatchDebug event graph to a clean baseline.

This script talks directly to UnrealMCP plugin TCP endpoint (127.0.0.1:55557).
It clears the widget blueprint event graph, recreates button OnClicked bindings,
and compiles the blueprint.
"""

from __future__ import annotations

import json
import socket
import sys
from typing import Any, Dict


UE_HOST = "127.0.0.1"
UE_PORT = 55557
BLUEPRINT_NAME = "/Game/WBP_LocalMatchDebug"

BUTTONS = {
    "BtnJoin": (-2800, -900),
    "BtnCommitReveal": (-2800, -300),
    "BtnRedMove": (-2800, 300),
    "BtnBlackResign": (-2800, 900),
    "BtnPullRed": (-2800, 1500),
    "BtnPullBlack": (-2800, 1900),
}


def send_command(command: str, params: Dict[str, Any]) -> Dict[str, Any]:
    payload = json.dumps({"type": command, "params": params}).encode("utf-8")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(10)
        sock.connect((UE_HOST, UE_PORT))
        sock.sendall(payload)

        buffer = b""
        while True:
            chunk = sock.recv(65536)
            if not chunk:
                break
            buffer += chunk
            try:
                return json.loads(buffer.decode("utf-8"))
            except json.JSONDecodeError:
                continue

    return {
        "status": "error",
        "error": "incomplete_response",
        "raw": buffer.decode("utf-8", errors="ignore"),
    }


def require_success(result: Dict[str, Any], context: str) -> Dict[str, Any]:
    if result.get("status") != "success":
        raise RuntimeError(f"{context} failed: {json.dumps(result, ensure_ascii=False)}")
    return result


def main() -> int:
    try:
        clear_result = require_success(
            send_command(
                "clear_blueprint_event_graph",
                {"blueprint_name": BLUEPRINT_NAME, "keep_bound_events": False},
            ),
            "clear_blueprint_event_graph",
        )
        print(f"[OK] Cleared graph: {json.dumps(clear_result.get('result', {}), ensure_ascii=False)}")

        for button_name, node_position in BUTTONS.items():
            bind_result = require_success(
                send_command(
                    "bind_widget_event",
                    {
                        "blueprint_name": BLUEPRINT_NAME,
                        "widget_name": button_name,
                        "event_name": "OnClicked",
                        "function_name": f"On{button_name}Clicked",
                        "node_position": [node_position[0], node_position[1]],
                    },
                ),
                f"bind_widget_event({button_name})",
            )
            node_id = bind_result.get("result", {}).get("node_id", "")
            print(f"[OK] Bound {button_name} -> node_id={node_id}")

        compile_result = require_success(
            send_command("compile_blueprint", {"blueprint_name": BLUEPRINT_NAME}),
            "compile_blueprint",
        )
        print(f"[OK] Compiled: {json.dumps(compile_result.get('result', {}), ensure_ascii=False)}")
        return 0
    except Exception as exc:
        print(f"[ERROR] {exc}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
