# UE MCP Blueprint Workflow

## 状态说明

1. 当前唯一推荐方案是 `UnrealMCP`（`chongdashu/unreal-mcp` 的 fork）。
2. 旧的 MCP/RemoteExecution 探测流程不再作为主流程使用。
3. 本文档是 UE 蓝图 MCP 操作的唯一权威入口。
4. 源码真源规则：`UnrealMCP` 功能修改必须先在 fork 仓库实现，再同步到本项目插件副本。

## 当前基线

1. UE: `5.7.x`
2. UE 插件路径: `clients/ue/StupidChessUE/Plugins/UnrealMCP`
3. 插件来源: `D:\git_projects\unreal-mcp`（fork: `MotionlessPeri/unreal-mcp`）
4. Python server: `Python/unreal_mcp_server.py`
5. Python 依赖固定:
   - `fastmcp==0.4.1`
   - `mcp[cli]==1.5.0`

## 一次性准备

1. 同步插件到 UE 工程:
```powershell
powershell -ExecutionPolicy Bypass -File tools\sync_unreal_mcp.ps1 -ForkRepoRoot D:\git_projects\unreal-mcp
```
2. 在 `StupidChessUE.uproject` 中启用 `UnrealMCP` 插件。
3. 打开 UE Editor 并加载目标工程（不要只打开空编辑器）。
4. 确认 `127.0.0.1:55557` 正在监听（UnrealMCP 默认端口）。
5. 若要改 MCP 功能，先在 fork 仓库改动并提交，再执行：
```powershell
powershell -ExecutionPolicy Bypass -File tools\sync_unreal_mcp.ps1 -ForkRepoRoot D:\git_projects\unreal-mcp
```

## 日常使用流程（Codex）

1. 启动 UE Editor 并保持项目处于打开状态。
2. 使用 Codex MCP 连接本地 `unreal_mcp_server.py`。
3. 先做连通性探测，再做蓝图操作：
   - 读场景演员（例如 `get_actors_in_level`）。
   - 读蓝图节点（例如 `find_blueprint_nodes`，支持 `/Game/WBP_LocalMatchDebug` 这种完整路径）。
4. 蓝图改动后在 UE 内执行 `Compile + Save`，再做一次读取校验。
5. 需要冷编译插件前，优先使用 MCP 编辑器生命周期命令而不是手动强关：
   - `save_dirty_assets`
   - `request_editor_exit`
   - `save_and_exit_editor`（推荐，先保存再延迟退出，降低 MCP 回包被截断概率）

## 已验证能力

1. 可读取工程资产并按路径/名称定位蓝图。
2. 可读取蓝图图表与节点列表，用于校验接线是否存在。
3. 已修复蓝图查找逻辑，不再限定 `/Game/Blueprints` 或 `/Game/Widgets`。
4. 已支持 `clear_blueprint_event_graph`（清空 EventGraph）与 `add_blueprint_dynamic_cast_node`（自动插入类型转换）。
5. `connect_blueprint_nodes` 已改为 schema 校验连线，不再静默写入非法连接。
6. `bind_widget_event` 支持可选 `node_position`，可将红色事件入口节点按预期布局到链路左侧。
7. `tools/wire_local_match_widget_graph.py` 默认进入 Preserve 模式（不清空 EventGraph），避免误删 `Construct`/自定义回调链路；需要全量重建时显式传 `--clear`。
8. 已接入 `bind_blueprint_multicast_delegate`，可在自动化脚本中直接生成 `Assign Delegate + CustomEvent` 委托绑定链路。
9. `find_blueprint_nodes` 现支持通过地图资产名/路径解析 Level Blueprint（`ULevelScriptBlueprint`），例如 `DebugLevel` 或 `/Game/DebugLevel`。
10. 新增 `add_blueprint_subsystem_getter_node`：可直接创建 `Get <YourSubsystem>` 节点（基于 `UK2Node_GetSubsystem`），避免“通用 `Get Game Instance Subsystem + Cast`”手工拼接。
11. `tools/wire_local_match_widget_graph.py` 默认不重建 `Construct`，避免覆盖手工链路；需要自动重建委托绑定时显式使用 `--wire-construct`。
12. 安全限制：当前脚本在 `Preserve` 模式下会跳过 `--wire-construct`（仅给出警告），避免重复执行导致 `Bind Event + Custom Event` 成对堆积；需要重建 `Construct` 时使用 `--clear --wire-construct`。
13. 新增 `add_blueprint_make_struct_node`：可创建 `UK2Node_MakeStruct` 并设置字段默认值，适配 `SubmitMove` 这类 by-ref 结构体入参。
14. 新增 `break_blueprint_node_pin_links`：可按节点+引脚断开旧连线，用于 Preserve 模式下重接按钮链路时清理历史分支。
15. 新增 `dedupe_blueprint_component_bound_events`：可按 `widget_name + event_name` 清理重复 `ComponentBoundEvent` 及其旧执行链，防止按钮入口重复残留。
16. `add_blueprint_event_node` 已修复 override/lifecycle 事件创建路径：
   - 使用 `FKismetEditorUtilities::AddDefaultEventNode` 创建事件节点（而非直接构造 `UK2Node_Event`）。
   - `WidgetBlueprint` 的 `Event Construct` 等生命周期事件现在创建后可正常在运行时触发。

## 能力边界（当前）

1. MCP 工具集合是否包含“创建/连线节点”取决于当前插件版本和 server 注册结果。
2. 即使支持编辑命令，也建议按“改动后立即读取校验”的闭环执行，避免静默失败。
3. `WidgetBlueprint` 不提供稳定文本导出，不建议走文本 diff 作为主验证手段。
4. `compile_blueprint` 返回 `compiled=true` 不等价于“绝对无编译告警/错误”，仍需回读 UE 日志中的 `LogBlueprint: Error: [AssetLog]`。
5. `find_blueprint_nodes` 当前只支持 `node_type=Event`（返回 `node_guids`），不支持直接按 `K2Node_CallFunction` 枚举详细节点信息。

## 常见问题排查

1. `Transport closed`:
   - 先看 `unreal_mcp.log`。
   - 再确认 UE 插件端口 `127.0.0.1:55557` 是否监听。
2. `WinError 10061`:
   - UE 未打开目标工程，或插件未加载。
3. `FastMCP.__init__() got an unexpected keyword argument 'description'`:
   - 依赖漂移到 `fastmcp` 2.x，需回退到固定版本。
4. 构建时报 `UnrealEditor-UnrealMCP.dll` 被占用:
   - 关闭正在运行的 UE Editor 后重试链接。
5. 已执行同步但命令行为仍是旧逻辑:
   - UE 正在使用旧插件二进制（常见于 Live Coding 开启）。
   - 先按 `Ctrl+Alt+F11` 停止 Live Coding，或直接关闭 UE，再重建并重启编辑器。
6. `bind_widget_event` 报组件不存在或不可绑定:
   - 确认目标控件已勾选 `Is Variable`，并已 `Compile + Save`。
   - 新参数推荐：`blueprint_name + widget_name(component)`；旧参数 `widget_name + widget_component_name` 仍兼容。
7. 日志出现 `LoadAsset failed: /Game/Blueprints/...` 与 `/Game/Widgets/...`:
   - 当前插件会先探测这些默认路径，再回退到 `/Game/<AssetName>` 或 AssetRegistry 搜索。
   - 若最终能加载并执行命令，这类日志可视为探测噪声，不是致命错误。
8. 新增命令报 `Unknown command: add_blueprint_subsystem_getter_node`:
   - 说明 UE 正在运行旧版插件二进制。
   - 关闭 UE 或停止 Live Coding（`Ctrl+Alt+F11`）后重编译 `StupidChessUEEditor`，再重启编辑器。
9. 运行 `wire_local_match_widget_graph.py` 后按钮仍出现多条重复分支:
   - 先确认插件二进制已包含 `dedupe_blueprint_component_bound_events`。
   - 若命令未知，按上一条流程重启 UE 并重编译插件。
10. 需要频繁重编插件导致反复手动关闭 UE:
   - 使用 `save_and_exit_editor` 让编辑器在 MCP 回包后延迟退出，再执行 `Build.bat` 冷编译。
   - 若仍无法退出，检查是否有调试器附加或弹窗阻塞（保存提示/确认框）。

## 本地直连自检（可选）

```powershell
$VenvPy = "D:\git_projects\unreal-mcp\Python\.venv\Scripts\python.exe"
& $VenvPy "D:\git_projects\unreal-mcp\Python\unreal_mcp_server.py"
```
