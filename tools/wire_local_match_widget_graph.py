#!/usr/bin/env python3
"""
Rebuild WBP_LocalMatchDebug with a clean, readable event graph wiring.

Notes:
- Uses UnrealMCP TCP command endpoint (127.0.0.1:55557).
- Clears the current EventGraph first.
- Rebinds 6 button OnClicked events and wires functional chains.
- BtnRedMove is temporarily wired to a log message because SubmitMove needs
  a by-ref struct input that current MCP commands cannot auto-build.
"""

from __future__ import annotations

import json
import socket
import sys
from dataclasses import dataclass
from typing import Any, Dict


UE_HOST = "127.0.0.1"
UE_PORT = 55557
BLUEPRINT_NAME = "WBP_LocalMatchDebug"

MATCH_ID = "900"
RED_PLAYER_ID = "10001"
BLACK_PLAYER_ID = "10002"
AFTER_SERVER_SEQUENCE = "0"


@dataclass(frozen=True)
class EventIds:
    BtnJoin: str
    BtnCommitReveal: str
    BtnRedMove: str
    BtnBlackResign: str
    BtnPullRed: str
    BtnPullBlack: str


def send_command(command: str, params: Dict[str, Any]) -> Dict[str, Any]:
    payload = json.dumps({"type": command, "params": params}).encode("utf-8")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(20)
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


def add_call(target: str, function_name: str, node_x: int, node_y: int, params: Dict[str, Any] | None = None) -> str:
    result = require_success(
        send_command(
            "add_blueprint_function_node",
            {
                "blueprint_name": BLUEPRINT_NAME,
                "target": target,
                "function_name": function_name,
                "node_position": [node_x, node_y],
                "params": params or {},
            },
        ),
        f"add_blueprint_function_node({function_name})",
    )
    return result["result"]["node_id"]


def connect_exec(source_node_id: str, target_node_id: str) -> None:
    require_success(
        send_command(
            "connect_blueprint_nodes",
            {
                "blueprint_name": BLUEPRINT_NAME,
                "source_node_id": source_node_id,
                "source_pin": "Then",
                "target_node_id": target_node_id,
                "target_pin": "Execute",
            },
        ),
        f"connect_exec({source_node_id}->{target_node_id})",
    )


def connect_data(source_node_id: str, source_pin: str, target_node_id: str, target_pin: str) -> None:
    require_success(
        send_command(
            "connect_blueprint_nodes",
            {
                "blueprint_name": BLUEPRINT_NAME,
                "source_node_id": source_node_id,
                "source_pin": source_pin,
                "target_node_id": target_node_id,
                "target_pin": target_pin,
            },
        ),
        f"connect_data({source_pin}->{target_pin})",
    )


def add_get_subsystem(node_x: int, node_y: int) -> str:
    get_subsystem = add_call(
        target="USubsystemBlueprintLibrary",
        function_name="GetGameInstanceSubsystem",
        node_x=node_x,
        node_y=node_y,
        params={"Class": "/Script/StupidChessCoreBridge.StupidChessLocalMatchSubsystem"},
    )
    cast_result = require_success(
        send_command(
            "add_blueprint_dynamic_cast_node",
            {
                "blueprint_name": BLUEPRINT_NAME,
                "target_class": "/Script/StupidChessCoreBridge.StupidChessLocalMatchSubsystem",
                "node_position": [node_x + 280, node_y],
            },
        ),
        "add_blueprint_dynamic_cast_node",
    )
    cast_node = cast_result["result"]["node_id"]
    connect_data(get_subsystem, "ReturnValue", cast_node, "Object")
    return cast_node


def bind_buttons() -> EventIds:
    button_positions = {
        "BtnJoin": (-2800, -900),
        "BtnCommitReveal": (-2800, -300),
        "BtnRedMove": (-2800, 300),
        "BtnBlackResign": (-2800, 900),
        "BtnPullRed": (-2800, 1500),
        "BtnPullBlack": (-2800, 1900),
    }
    ids: Dict[str, str] = {}
    for button_name, node_position in button_positions.items():
        result = require_success(
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
        ids[button_name] = result["result"]["node_id"]
        print(f"[OK] Bound {button_name}: {ids[button_name]}")
    return EventIds(**ids)  # type: ignore[arg-type]


def wire_join_chain(events: EventIds) -> None:
    cast_subsystem = add_get_subsystem(-2300, -900)
    reset_local_server = add_call("UStupidChessLocalMatchSubsystem", "ResetLocalServer", -1850, -900)
    join_red = add_call(
        "UStupidChessLocalMatchSubsystem",
        "JoinLocalMatch",
        -1450,
        -900,
        {"MatchId": MATCH_ID, "PlayerId": RED_PLAYER_ID},
    )
    join_black = add_call(
        "UStupidChessLocalMatchSubsystem",
        "JoinLocalMatch",
        -1050,
        -900,
        {"MatchId": MATCH_ID, "PlayerId": BLACK_PLAYER_ID},
    )
    pull_red = add_call(
        "UStupidChessLocalMatchSubsystem",
        "PullParseAndDispatchOutboundMessages",
        -650,
        -900,
        {"PlayerId": RED_PLAYER_ID, "AfterServerSequence": AFTER_SERVER_SEQUENCE},
    )
    pull_black = add_call(
        "UStupidChessLocalMatchSubsystem",
        "PullParseAndDispatchOutboundMessages",
        -250,
        -900,
        {"PlayerId": BLACK_PLAYER_ID, "AfterServerSequence": AFTER_SERVER_SEQUENCE},
    )

    for node_id in [reset_local_server, join_red, join_black, pull_red, pull_black]:
        connect_data(cast_subsystem, "ReturnValue", node_id, "self")

    connect_exec(events.BtnJoin, cast_subsystem)
    connect_exec(cast_subsystem, reset_local_server)
    connect_exec(reset_local_server, join_red)
    connect_exec(join_red, join_black)
    connect_exec(join_black, pull_red)
    connect_exec(pull_red, pull_black)


def wire_commit_reveal_chain(events: EventIds) -> None:
    cast_subsystem = add_get_subsystem(-2300, -300)
    commit_red = add_call(
        "UStupidChessLocalMatchSubsystem",
        "SubmitCommitSetup",
        -1850,
        -300,
        {"MatchId": MATCH_ID, "PlayerId": RED_PLAYER_ID, "Side": "Red", "HashHex": ""},
    )
    commit_black = add_call(
        "UStupidChessLocalMatchSubsystem",
        "SubmitCommitSetup",
        -1450,
        -300,
        {"MatchId": MATCH_ID, "PlayerId": BLACK_PLAYER_ID, "Side": "Black", "HashHex": ""},
    )
    placements_red = add_call(
        "UStupidChessLocalMatchSubsystem",
        "BuildStandardSetupPlacements",
        -1850,
        -120,
        {"Side": "Red"},
    )
    reveal_red = add_call(
        "UStupidChessLocalMatchSubsystem",
        "SubmitRevealSetup",
        -1050,
        -300,
        {"MatchId": MATCH_ID, "PlayerId": RED_PLAYER_ID, "Side": "Red", "Nonce": "R"},
    )
    placements_black = add_call(
        "UStupidChessLocalMatchSubsystem",
        "BuildStandardSetupPlacements",
        -1450,
        60,
        {"Side": "Black"},
    )
    reveal_black = add_call(
        "UStupidChessLocalMatchSubsystem",
        "SubmitRevealSetup",
        -650,
        -300,
        {"MatchId": MATCH_ID, "PlayerId": BLACK_PLAYER_ID, "Side": "Black", "Nonce": "B"},
    )
    pull_red = add_call(
        "UStupidChessLocalMatchSubsystem",
        "PullParseAndDispatchOutboundMessages",
        -250,
        -300,
        {"PlayerId": RED_PLAYER_ID, "AfterServerSequence": AFTER_SERVER_SEQUENCE},
    )
    pull_black = add_call(
        "UStupidChessLocalMatchSubsystem",
        "PullParseAndDispatchOutboundMessages",
        150,
        -300,
        {"PlayerId": BLACK_PLAYER_ID, "AfterServerSequence": AFTER_SERVER_SEQUENCE},
    )

    for node_id in [commit_red, commit_black, placements_red, reveal_red, placements_black, reveal_black, pull_red, pull_black]:
        connect_data(cast_subsystem, "ReturnValue", node_id, "self")

    connect_data(placements_red, "ReturnValue", reveal_red, "Placements")
    connect_data(placements_black, "ReturnValue", reveal_black, "Placements")

    connect_exec(events.BtnCommitReveal, cast_subsystem)
    connect_exec(cast_subsystem, commit_red)
    connect_exec(commit_red, commit_black)
    connect_exec(commit_black, reveal_red)
    connect_exec(reveal_red, reveal_black)
    connect_exec(reveal_black, pull_red)
    connect_exec(pull_red, pull_black)


def wire_red_move_placeholder(events: EventIds) -> None:
    print_node = add_call(
        "UKismetSystemLibrary",
        "PrintString",
        -1200,
        300,
        {
            "InString": "TODO: SubmitMove needs MCP struct-pin support for FStupidChessMoveCommand.",
            "Duration": 2.0,
        },
    )
    connect_exec(events.BtnRedMove, print_node)


def wire_black_resign_chain(events: EventIds) -> None:
    cast_subsystem = add_get_subsystem(-2300, 900)
    resign_black = add_call(
        "UStupidChessLocalMatchSubsystem",
        "SubmitResign",
        -1850,
        900,
        {"MatchId": MATCH_ID, "PlayerId": BLACK_PLAYER_ID, "Side": "Black"},
    )
    pull_red = add_call(
        "UStupidChessLocalMatchSubsystem",
        "PullParseAndDispatchOutboundMessages",
        -1450,
        900,
        {"PlayerId": RED_PLAYER_ID, "AfterServerSequence": AFTER_SERVER_SEQUENCE},
    )
    pull_black = add_call(
        "UStupidChessLocalMatchSubsystem",
        "PullParseAndDispatchOutboundMessages",
        -1050,
        900,
        {"PlayerId": BLACK_PLAYER_ID, "AfterServerSequence": AFTER_SERVER_SEQUENCE},
    )

    for node_id in [resign_black, pull_red, pull_black]:
        connect_data(cast_subsystem, "ReturnValue", node_id, "self")

    connect_exec(events.BtnBlackResign, cast_subsystem)
    connect_exec(cast_subsystem, resign_black)
    connect_exec(resign_black, pull_red)
    connect_exec(pull_red, pull_black)


def wire_pull_buttons(events: EventIds) -> None:
    cast_subsystem_red = add_get_subsystem(-2300, 1500)
    pull_red = add_call(
        "UStupidChessLocalMatchSubsystem",
        "PullParseAndDispatchOutboundMessages",
        -1850,
        1500,
        {"PlayerId": RED_PLAYER_ID, "AfterServerSequence": AFTER_SERVER_SEQUENCE},
    )
    connect_data(cast_subsystem_red, "ReturnValue", pull_red, "self")
    connect_exec(events.BtnPullRed, cast_subsystem_red)
    connect_exec(cast_subsystem_red, pull_red)

    cast_subsystem_black = add_get_subsystem(-2300, 1900)
    pull_black = add_call(
        "UStupidChessLocalMatchSubsystem",
        "PullParseAndDispatchOutboundMessages",
        -1850,
        1900,
        {"PlayerId": BLACK_PLAYER_ID, "AfterServerSequence": AFTER_SERVER_SEQUENCE},
    )
    connect_data(cast_subsystem_black, "ReturnValue", pull_black, "self")
    connect_exec(events.BtnPullBlack, cast_subsystem_black)
    connect_exec(cast_subsystem_black, pull_black)


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

        events = bind_buttons()
        wire_join_chain(events)
        wire_commit_reveal_chain(events)
        wire_red_move_placeholder(events)
        wire_black_resign_chain(events)
        wire_pull_buttons(events)

        compile_result = require_success(
            send_command("compile_blueprint", {"blueprint_name": BLUEPRINT_NAME}),
            "compile_blueprint",
        )
        print(f"[OK] Compiled: {json.dumps(compile_result.get('result', {}), ensure_ascii=False)}")
        print("[DONE] WBP_LocalMatchDebug graph rebuilt.")
        return 0
    except Exception as exc:
        print(f"[ERROR] {exc}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
