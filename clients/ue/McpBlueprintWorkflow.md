# UE MCP Blueprint Workflow

## 状态说明

1. 当前唯一推荐方案是 `UnrealMCP`（`chongdashu/unreal-mcp` 的 fork）。
2. 旧的 MCP/RemoteExecution 探测流程不再作为主流程使用。
3. 本文档是 UE 蓝图 MCP 操作的唯一权威入口。

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

## 日常使用流程（Codex）

1. 启动 UE Editor 并保持项目处于打开状态。
2. 使用 Codex MCP 连接本地 `unreal_mcp_server.py`。
3. 先做连通性探测，再做蓝图操作：
   - 读场景演员（例如 `get_actors_in_level`）。
   - 读蓝图节点（例如 `find_blueprint_nodes`，支持 `/Game/WBP_LocalMatchDebug` 这种完整路径）。
4. 蓝图改动后在 UE 内执行 `Compile + Save`，再做一次读取校验。

## 已验证能力

1. 可读取工程资产并按路径/名称定位蓝图。
2. 可读取蓝图图表与节点列表，用于校验接线是否存在。
3. 已修复蓝图查找逻辑，不再限定 `/Game/Blueprints` 或 `/Game/Widgets`。
4. 已支持 `clear_blueprint_event_graph`（清空 EventGraph）与 `add_blueprint_dynamic_cast_node`（自动插入类型转换）。
5. `connect_blueprint_nodes` 已改为 schema 校验连线，不再静默写入非法连接。
6. `bind_widget_event` 支持可选 `node_position`，可将红色事件入口节点按预期布局到链路左侧。

## 能力边界（当前）

1. MCP 工具集合是否包含“创建/连线节点”取决于当前插件版本和 server 注册结果。
2. 即使支持编辑命令，也建议按“改动后立即读取校验”的闭环执行，避免静默失败。
3. `WidgetBlueprint` 不提供稳定文本导出，不建议走文本 diff 作为主验证手段。
4. `compile_blueprint` 返回 `compiled=true` 不等价于“绝对无编译告警/错误”，仍需回读 UE 日志中的 `LogBlueprint: Error: [AssetLog]`。

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

## 本地直连自检（可选）

```powershell
$VenvPy = "D:\git_projects\unreal-mcp\Python\.venv\Scripts\python.exe"
& $VenvPy "D:\git_projects\unreal-mcp\Python\unreal_mcp_server.py"
```
