# Progress - UnrealMCP

## Last Updated

1. 2026-02-26

## Scope

1. `D:\git_projects\unreal-mcp` fork 能力建设。
2. 同步到 `clients/ue/StupidChessUE/Plugins/UnrealMCP` 的消费端验证。
3. UMG Route B 自动化能力与 smoke 稳定性。

## Status

1. Route B 阶段 0-5 已完成并在消费者侧多轮 smoke 通过。
2. 已补齐批量命令与清理命令，支持自动构建 9x10 DebugBoard 骨架。
3. 已落地模板 helper：`create_debugboard_skeleton_widget`（参数化持续增强）。

## Completed Highlights

1. UMG 基础读写：`get_widget_tree`、`ensure_widget_root`、`add_widget_child`。
2. 布局能力：
   - `set_canvas_slot_layout` / `set_canvas_slot_layout_batch`
   - `set_uniform_grid_slot` / `set_uniform_grid_slot_batch`
3. 属性能力：
   - `set_widget_common_properties` / `set_widget_common_properties_batch`
   - `set_text_block_properties` / `set_text_block_properties_batch`
4. 清理能力：
   - `clear_widget_children`
   - `remove_widget_from_blueprint`
   - `delete_widget_blueprints_by_prefix`
5. 编辑器工作流能力：
   - `save_dirty_assets`
   - `request_editor_exit`
   - `save_and_exit_editor`
6. 稳定性与可回归：
   - full debugboard skeleton smoke
   - stress smoke harness（D3D12 模式重复执行）

## Open Items

1. 模板 helper 进一步参数化（面板项定义、命名策略、可选构件）。
2. 需要时评估 deferred compile/save 事务模式，降低大批量构建时 editor 抖动。
3. 将常用 smoke 汇总为单入口回归脚本（按场景开关执行）。

## Source

1. 历史明细见 `docs/progress/LegacyProgressLog.md`（条目 26-67 为主）。
