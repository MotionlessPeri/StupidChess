# tools

构建、测试、导出等工具脚本目录。

## Scripts

1. `sync_unreal_mcp.ps1`
   - 从本地 fork 仓库同步 `UnrealMCP` UE 插件到当前项目。
   - 默认源路径：`D:\git_projects\unreal-mcp`
   - 用法：
     - `powershell -ExecutionPolicy Bypass -File tools\sync_unreal_mcp.ps1`
     - `powershell -ExecutionPolicy Bypass -File tools\sync_unreal_mcp.ps1 -ForkRepoRoot D:\git_projects\unreal-mcp`
2. `reset_local_match_widget_graph.py`
   - 清理 `WBP_LocalMatchDebug` 的 EventGraph，并重新绑定 6 个按钮 `OnClicked` 事件。
   - 会按预设坐标放置红色事件入口节点，避免堆到图右侧。
   - 用法：
     - `python tools\reset_local_match_widget_graph.py`
3. `wire_local_match_widget_graph.py`
   - 对 `WBP_LocalMatchDebug` 执行“绑定事件 + 自动接线 + 编译”一键接线。
   - 默认不会改动现有 `Event Construct` 链路；如需重建委托绑定链路，显式传 `--wire-construct`。
   - `--wire-construct` 会自动补齐 `Event Construct` 下的 `On*Parsed` 委托绑定链路（`Assign Delegate` + 自动生成回调事件 + `PrintString`）。
   - 会在 6 个按钮入口自动插入点击日志（`[Click][Btn...]`），便于确认 `OnClicked` 是否触发。
   - 默认 `Preserve` 模式：不清图，保留 `Construct` 与自定义回调链路。
   - `Preserve` 模式会先做按钮事件去重（`dedupe_blueprint_component_bound_events`）并清理旧执行链（`clear_blueprint_event_exec_chain`），避免重复执行后堆积残留节点。
   - 脚本尾部会触发一次插件侧 `SaveAsset`，确保自动接线结果写入磁盘。
   - 会按预设坐标放置红色事件入口节点，保持图可读性。
   - Subsystem 获取节点改为 MCP 新命令 `add_blueprint_subsystem_getter_node`（对应 UE 蓝图里的 `GetStupidChessLocalMatchSubsystem` 样式）。
   - `BtnRedMove` 会接真实 `SubmitMove` 测试链路（通过 `MakeStruct(StupidChessMoveCommand)` 节点接入 `Move` by-ref 引脚）。
   - 脚本会先清理 6 个按钮事件节点的旧执行链，再重接新链路，避免 Preserve 模式下历史链路（如旧 TODO 分支）继续触发。
   - 用法：
     - `python tools\wire_local_match_widget_graph.py`
     - `python tools\wire_local_match_widget_graph.py --wire-construct`
     - `python tools\wire_local_match_widget_graph.py --clear`
