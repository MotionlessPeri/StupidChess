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
   - 对 `WBP_LocalMatchDebug` 执行“清图 + 绑定事件 + 自动接线 + 编译”一键重建。
   - 会按预设坐标放置红色事件入口节点，保持图可读性。
   - 当前会将 `BtnRedMove` 接为占位日志（等待结构体引脚自动化能力补齐）。
   - 用法：
     - `python tools\wire_local_match_widget_graph.py`
