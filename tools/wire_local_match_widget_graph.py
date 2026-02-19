#!/usr/bin/env python3
"""
Rebuild WBP_LocalMatchDebug with a clean, readable event graph wiring.

Notes:
- Uses UnrealMCP TCP command endpoint (127.0.0.1:55557).
- Preserves existing graph by default (including Construct/custom callback chains).
- Use --clear for deterministic rebuild from scratch.
- Rebinds 6 button OnClicked events and wires functional chains.
- BtnRedMove is wired to a real SubmitMove test command using struct default text.
- Default behavior does NOT touch existing Construct chain unless --wire-construct is specified.
"""

from __future__ import annotations

import argparse
import json
import socket
import sys
from dataclasses import dataclass
from typing import Any, Dict


UE_HOST = "127.0.0.1"
UE_PORT = 55557
BLUEPRINT_NAME = "/Game/WBP_LocalMatchDebug"
SUBSYSTEM_CLASS = "/Script/StupidChessCoreBridge.StupidChessLocalMatchSubsystem"

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


def add_make_struct(struct_type: str, node_x: int, node_y: int, values: Dict[str, Any]) -> tuple[str, str]:
    result = require_success(
        send_command(
            "add_blueprint_make_struct_node",
            {
                "blueprint_name": BLUEPRINT_NAME,
                "struct_type": struct_type,
                "node_position": [node_x, node_y],
                "values": values,
            },
        ),
        f"add_blueprint_make_struct_node({struct_type})",
    )
    return result["result"]["node_id"], result["result"]["output_pin"]


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
    result = require_success(
        send_command(
            "add_blueprint_subsystem_getter_node",
            {
                "blueprint_name": BLUEPRINT_NAME,
                "subsystem_class": SUBSYSTEM_CLASS,
                "node_position": [node_x, node_y],
            },
        ),
        "add_blueprint_subsystem_getter_node",
    )
    return result["result"]["node_id"]


def add_button_click_log(button_name: str, node_x: int, node_y: int) -> str:
    return add_call(
        target="UKismetSystemLibrary",
        function_name="PrintString",
        node_x=node_x,
        node_y=node_y,
        params={
            "InString": f"[Click][{button_name}]",
            "Duration": 2.0,
        },
    )


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
        bound_node_id = result["result"]["node_id"]
        dedupe_result = require_success(
            send_command(
                "dedupe_blueprint_component_bound_events",
                {
                    "blueprint_name": BLUEPRINT_NAME,
                    "widget_name": button_name,
                    "event_name": "OnClicked",
                    "event_output_pin": "Then",
                    "keep_node_id": bound_node_id,
                },
            ),
            f"dedupe_blueprint_component_bound_events({button_name})",
        )
        ids[button_name] = dedupe_result["result"].get("kept_node_id", bound_node_id)
        print(
            "[OK] Bound+deduped "
            f"{button_name}: kept={ids[button_name]} "
            f"removed_event_nodes={dedupe_result['result'].get('removed_event_nodes', 0)} "
            f"removed_chain_nodes={dedupe_result['result'].get('removed_chain_nodes', 0)}"
        )
    return EventIds(**ids)  # type: ignore[arg-type]


def clear_event_exec_chain(event_node_id: str) -> None:
    require_success(
        send_command(
            "clear_blueprint_event_exec_chain",
            {
                "blueprint_name": BLUEPRINT_NAME,
                "event_node_id": event_node_id,
                "event_output_pin": "Then",
            },
        ),
        f"clear_blueprint_event_exec_chain({event_node_id}.Then)",
    )


def save_blueprint_asset() -> None:
    # Re-bind one existing button event to trigger plugin-side SaveAsset().
    require_success(
        send_command(
            "bind_widget_event",
            {
                "blueprint_name": BLUEPRINT_NAME,
                "widget_name": "BtnJoin",
                "event_name": "OnClicked",
                "function_name": "OnBtnJoinClicked",
                "node_position": [-2800, -900],
            },
        ),
        "save_blueprint_asset(bind_widget_event)",
    )


def add_event(event_name: str, node_x: int, node_y: int) -> str:
    result = require_success(
        send_command(
            "add_blueprint_event_node",
            {
                "blueprint_name": BLUEPRINT_NAME,
                "event_name": event_name,
                "node_position": [node_x, node_y],
            },
        ),
        f"add_blueprint_event_node({event_name})",
    )
    return result["result"]["node_id"]


def bind_multicast_delegate(
    delegate_name: str,
    target_node_id: str,
    exec_source_node_id: str,
    node_x: int,
    node_y: int,
) -> tuple[str, str]:
    result = require_success(
        send_command(
            "bind_blueprint_multicast_delegate",
            {
                "blueprint_name": BLUEPRINT_NAME,
                "target_class": SUBSYSTEM_CLASS,
                "delegate_name": delegate_name,
                "target_node_id": target_node_id,
                "target_output_pin": "ReturnValue",
                "assign_target_pin": "self",
                "exec_source_node_id": exec_source_node_id,
                "exec_source_pin": "Then",
                "assign_exec_pin": "Execute",
                "node_position": [node_x, node_y],
                "custom_event_position": [node_x + 300, node_y + 120],
            },
        ),
        f"bind_blueprint_multicast_delegate({delegate_name})",
    )
    assign_node_id = result["result"]["assign_node_id"]
    custom_event_node_id = result["result"]["custom_event_node_id"]
    return assign_node_id, custom_event_node_id


def wire_construct_delegate_bindings() -> None:
    construct_event = add_event("Construct", -3600, -2200)
    getter_subsystem = add_get_subsystem(-3200, -2200)

    delegate_logs = [
        ("OnJoinAckParsed", "[Callback][JoinAck]"),
        ("OnCommandAckParsed", "[Callback][CommandAck]"),
        ("OnErrorParsed", "[Callback][Error]"),
        ("OnSnapshotParsed", "[Callback][Snapshot]"),
        ("OnEventDeltaParsed", "[Callback][EventDelta]"),
        ("OnGameOverParsed", "[Callback][GameOver]"),
    ]

    previous_exec_node = construct_event
    for index, (delegate_name, log_text) in enumerate(delegate_logs):
        base_y = -2200 + index * 280
        assign_node, custom_event_node = bind_multicast_delegate(
            delegate_name=delegate_name,
            target_node_id=getter_subsystem,
            exec_source_node_id=previous_exec_node,
            node_x=-2700,
            node_y=base_y,
        )
        print_node = add_call(
            target="UKismetSystemLibrary",
            function_name="PrintString",
            node_x=-1800,
            node_y=base_y + 120,
            params={
                "InString": log_text,
                "Duration": 2.0,
            },
        )
        connect_exec(custom_event_node, print_node)
        previous_exec_node = assign_node


def wire_join_chain(events: EventIds) -> None:
    click_log = add_button_click_log("BtnJoin", -2550, -900)
    getter_subsystem = add_get_subsystem(-2300, -900)
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
        connect_data(getter_subsystem, "ReturnValue", node_id, "self")

    connect_exec(events.BtnJoin, click_log)
    connect_exec(click_log, reset_local_server)
    connect_exec(reset_local_server, join_red)
    connect_exec(join_red, join_black)
    connect_exec(join_black, pull_red)
    connect_exec(pull_red, pull_black)


def wire_commit_reveal_chain(events: EventIds) -> None:
    click_log = add_button_click_log("BtnCommitReveal", -2550, -300)
    getter_subsystem = add_get_subsystem(-2300, -300)
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
        connect_data(getter_subsystem, "ReturnValue", node_id, "self")

    connect_data(placements_red, "ReturnValue", reveal_red, "Placements")
    connect_data(placements_black, "ReturnValue", reveal_black, "Placements")

    connect_exec(events.BtnCommitReveal, click_log)
    connect_exec(click_log, commit_red)
    connect_exec(commit_red, commit_black)
    connect_exec(commit_black, reveal_red)
    connect_exec(reveal_red, reveal_black)
    connect_exec(reveal_black, pull_red)
    connect_exec(pull_red, pull_black)


def wire_red_move_chain(events: EventIds) -> None:
    click_log = add_button_click_log("BtnRedMove", -1450, 300)
    getter_subsystem = add_get_subsystem(-1200, 300)
    move_struct_node, move_struct_output_pin = add_make_struct(
        "/Script/StupidChessCoreBridge.StupidChessMoveCommand",
        -980,
        80,
        {
            "PieceId": 11,
            "FromX": 0,
            "FromY": 3,
            "ToX": 0,
            "ToY": 4,
            "bHasCapturedPieceId": False,
            "CapturedPieceId": 0,
        },
    )
    submit_move = add_call(
        "UStupidChessLocalMatchSubsystem",
        "SubmitMove",
        -900,
        300,
        {
            "MatchId": MATCH_ID,
            "PlayerId": RED_PLAYER_ID,
            "Side": "Red",
        },
    )
    pull_red = add_call(
        "UStupidChessLocalMatchSubsystem",
        "PullParseAndDispatchOutboundMessages",
        -500,
        300,
        {"PlayerId": RED_PLAYER_ID, "AfterServerSequence": AFTER_SERVER_SEQUENCE},
    )
    pull_black = add_call(
        "UStupidChessLocalMatchSubsystem",
        "PullParseAndDispatchOutboundMessages",
        -100,
        300,
        {"PlayerId": BLACK_PLAYER_ID, "AfterServerSequence": AFTER_SERVER_SEQUENCE},
    )

    for node_id in [submit_move, pull_red, pull_black]:
        connect_data(getter_subsystem, "ReturnValue", node_id, "self")
    connect_data(move_struct_node, move_struct_output_pin, submit_move, "Move")

    connect_exec(events.BtnRedMove, click_log)
    connect_exec(click_log, submit_move)
    connect_exec(submit_move, pull_red)
    connect_exec(pull_red, pull_black)


def wire_black_resign_chain(events: EventIds) -> None:
    click_log = add_button_click_log("BtnBlackResign", -2550, 900)
    getter_subsystem = add_get_subsystem(-2300, 900)
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
        connect_data(getter_subsystem, "ReturnValue", node_id, "self")

    connect_exec(events.BtnBlackResign, click_log)
    connect_exec(click_log, resign_black)
    connect_exec(resign_black, pull_red)
    connect_exec(pull_red, pull_black)


def wire_pull_buttons(events: EventIds) -> None:
    click_log_red = add_button_click_log("BtnPullRed", -2550, 1500)
    getter_subsystem_red = add_get_subsystem(-2300, 1500)
    pull_red = add_call(
        "UStupidChessLocalMatchSubsystem",
        "PullParseAndDispatchOutboundMessages",
        -1850,
        1500,
        {"PlayerId": RED_PLAYER_ID, "AfterServerSequence": AFTER_SERVER_SEQUENCE},
    )
    connect_data(getter_subsystem_red, "ReturnValue", pull_red, "self")
    connect_exec(events.BtnPullRed, click_log_red)
    connect_exec(click_log_red, pull_red)

    click_log_black = add_button_click_log("BtnPullBlack", -2550, 1900)
    getter_subsystem_black = add_get_subsystem(-2300, 1900)
    pull_black = add_call(
        "UStupidChessLocalMatchSubsystem",
        "PullParseAndDispatchOutboundMessages",
        -1850,
        1900,
        {"PlayerId": BLACK_PLAYER_ID, "AfterServerSequence": AFTER_SERVER_SEQUENCE},
    )
    connect_data(getter_subsystem_black, "ReturnValue", pull_black, "self")
    connect_exec(events.BtnPullBlack, click_log_black)
    connect_exec(click_log_black, pull_black)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Wire WBP_LocalMatchDebug event graph.")
    parser.add_argument(
        "--clear",
        action="store_true",
        help="Clear EventGraph before wiring. This is destructive and removes custom Construct/callback chains.",
    )
    parser.add_argument(
        "--wire-construct",
        action="store_true",
        help="Also (re)build Construct->delegate bindings. Disabled by default to preserve manual Construct chains.",
    )
    return parser.parse_args()


def main() -> int:
    try:
        args = parse_args()
        if args.clear:
            clear_result = require_success(
                send_command(
                    "clear_blueprint_event_graph",
                    {"blueprint_name": BLUEPRINT_NAME, "keep_bound_events": False},
                ),
                "clear_blueprint_event_graph",
            )
            print(f"[OK] Cleared graph: {json.dumps(clear_result.get('result', {}), ensure_ascii=False)}")
        else:
            print("[INFO] Preserve mode enabled: skip clear step and keep existing custom nodes/chains.")
            print("[INFO] Preserve mode still performs event-chain cleanup + bound-event dedupe for button paths.")

        events = bind_buttons()
        for event_node_id in [events.BtnJoin, events.BtnCommitReveal, events.BtnRedMove, events.BtnBlackResign, events.BtnPullRed, events.BtnPullBlack]:
            clear_event_exec_chain(event_node_id)
        if args.wire_construct:
            wire_construct_delegate_bindings()
        else:
            print("[INFO] Skip Construct wiring (--wire-construct not set).")
        wire_join_chain(events)
        wire_commit_reveal_chain(events)
        wire_red_move_chain(events)
        wire_black_resign_chain(events)
        wire_pull_buttons(events)

        compile_result = require_success(
            send_command("compile_blueprint", {"blueprint_name": BLUEPRINT_NAME}),
            "compile_blueprint",
        )
        print(f"[OK] Compiled: {json.dumps(compile_result.get('result', {}), ensure_ascii=False)}")
        save_blueprint_asset()
        print("[OK] Saved: /Game/WBP_LocalMatchDebug")
        print("[DONE] WBP_LocalMatchDebug graph rebuilt.")
        return 0
    except Exception as exc:
        print(f"[ERROR] {exc}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
