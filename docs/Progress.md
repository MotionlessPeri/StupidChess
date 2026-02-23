# Progress

## Last Updated

1. 2026-02-19
2. 2026-02-23

## Current Milestone

1. 稳定化 UE MCP 蓝图自动接线流程（可清图、可重建、可编译校验）。

## Completed

1. `core` 规则骨架与关键回归测试：`Commit/Reveal`、合法行棋、`Pass` 裁定、首次吃子切换与冻结。
2. `server` 会话层：`FInMemoryMatchSession`（玩家加入、命令提交、视角投影、事件日志）。
3. `server` 房间层：`FInMemoryMatchService`（多房间、玩家绑定、`Ack` 游标、断线续拉/全量重拉）。
4. `protocol` DTO：`JoinAck/CommandAck/Snapshot/EventDelta`。
5. `server` 映射层：`FProtocolMapper`（内部模型到协议 DTO）。
6. 文档与 ADR 已同步：`RuleSpec`、`Architecture`、`InterfaceSpec`、`ADR-0002`。
7. `server` transport 骨架：`FServerTransportAdapter + FInMemoryServerMessageSink`（Join/Command/PullSync/Ack 下发路径）。
8. `protocol` 编解码：`ProtocolCodec`（Envelope/Join/Command/PullSync/Ack/Snapshot/EventDelta/Error）。
9. `server` 网关层：`FServerGateway`（`C2S` 消息解码与路由）。
10. UE 版本控制基线：补充 UE 生成物 `.gitignore` 与内容资产 `.gitattributes`（二进制）规则。
11. UE CoreBridge 模块接入：
    - 新增 `StupidChessCoreBridge` 模块并挂入 `.uproject` 与 Targets。
    - 在 UE 模块内编译共享 `core/protocol/server` 源（桥接编译单元）。
    - 新增 `UStupidChessLocalMatchSubsystem`，暴露 `Join/PullSync/Ack` 与 outbox 拉取接口。
12. UE 编译兼容修复：`EStupidChessProtocolMessageType` 增加 `Unknown=0`，满足 UHT 对 `UENUM` 默认值的要求。
13. UE 编译修复：
    - `protocol/src/ProtocolCodec.cpp` 的内部 JSON 节点类型重命名为 `FProtocolJsonValue`，避免与 UE 全局 `FJsonValue` 冲突。
    - `UStupidChessLocalMatchSubsystem` 改为显式管理 `FStupidChessServerRuntime*` 生命周期，规避 UHT 生成单元中的不完整类型删除错误。
14. UE 命令提交封装：
    - `UStupidChessLocalMatchSubsystem` 新增 `SubmitCommitSetup/SubmitRevealSetup/SubmitPass`。
    - `RevealSetup` 支持蓝图摆子结构 `FStupidChessSetupPlacement` 输入，并统一编码到 `C2S_Command`。
15. UE 命令提交封装扩展：
    - `UStupidChessLocalMatchSubsystem` 新增 `SubmitMove/SubmitResign`。
    - 新增蓝图结构 `FStupidChessMoveCommand`，用于 `Move` 命令参数传递。
16. gateway 回归增强：
    - `ServerGatewayTests` 新增 battle phase 下 `Move` 与 `Resign` 路由用例。
    - 断言 `S2C_CommandAck + S2C_Snapshot + S2C_EventDelta` 广播序列与终局状态字段。
17. UE payload 解析层：
    - `UStupidChessLocalMatchSubsystem` 新增 `DecodeSnapshotPayloadJson/DecodeEventDeltaPayloadJson`。
    - 新增 `TryParseSnapshotMessage/TryParseEventDeltaMessage`，便于按消息类型安全解码。
    - 新增结构化视图模型：`FStupidChessSnapshotView`、`FStupidChessEventDeltaView` 等。
18. UE payload 解析层扩展：
    - 新增 `DecodeJoinAckPayloadJson/DecodeCommandAckPayloadJson/DecodeErrorPayloadJson`。
    - 新增 `TryParseJoinAckMessage/TryParseCommandAckMessage/TryParseErrorMessage`。
    - 新增结构化视图模型：`FStupidChessJoinAckView`、`FStupidChessCommandAckView`、`FStupidChessErrorView`。
19. UE bridge 自动化冒烟测试：
    - 新增 `StupidChess.UE.CoreBridge.LocalFlow` 自动化用例。
    - 覆盖 `Join -> Commit/Reveal -> Move -> Resign -> AckError` 全链路与 `TryParse*` 解析断言。
20. UE bridge 错误路径自动化测试：
    - 新增 `StupidChess.UE.CoreBridge.ErrorPaths` 自动化用例。
    - 覆盖本地参数校验拒绝、服务端拒绝命令、以及 JSON 解码失败路径。
21. UE bridge 解析缓存能力：
    - `UStupidChessLocalMatchSubsystem` 新增 `ResetParsedCache/ParseOutboundMessagesToCache` 与 `GetCached*` 接口。
    - `LocalFlow` 与 `ErrorPaths` 自动化用例新增缓存读取断言（确认/快照/增量/错误消息）。
22. UE bridge 消息派发入口：
    - 新增 `PullParseAndDispatchOutboundMessages`，支持一次调用完成拉取、缓存更新与事件分发。
    - 新增 `BlueprintAssignable` 事件：`OnJoinAckParsed/OnCommandAckParsed/OnErrorParsed/OnSnapshotParsed/OnEventDeltaParsed`。
    - 新增 `GetLastPulledMessages/GetLastPulledMessageCount`，便于表现层在事件回调后读取原始批次。
    - 自动化用例迁移到新入口，覆盖 `Join/Move/InvalidAck` 路径。
23. 终局协议与 UE 接入补齐：
    - `protocol/server` 新增 `FProtocolGameOverPayload` 与 `S2C_GameOver` 编解码/下发链路。
    - `UStupidChessLocalMatchSubsystem` 新增 `GameOver` 结构化解码、缓存读取与 `OnGameOverParsed` 派发事件。
    - `ServerGateway/UE` 自动化测试新增 `S2C_GameOver` 断言，覆盖认输终局路径。
24. UE 终局视图易用性增强：
    - `FStupidChessGameOverView` 新增 `bIsDraw` 与 `WinnerSide`，减少蓝图层重复判定逻辑。
    - `DecodeGameOverPayloadJson` 自动映射赢家阵营与和局标志，缓存读取与自动化断言同步覆盖。
25. UE 蓝图接线预制能力：
    - `UStupidChessLocalMatchSubsystem` 新增 `BuildStandardSetupPlacements`，直接产出标准 16 子摆法（含红黑镜像）。
    - 新增 `clients/ue/BlueprintQuickStart.md`，提供最小本地联调蓝图模板（Join/CommitReveal/Move/Resign/Pull）。
26. UE MCP 蓝图探测流程沉淀：
    - 新增 `clients/ue/McpBlueprintWorkflow.md`，固化蓝图可读信息、不可达边界、直连诊断与故障排查。
    - `AGENTS.md` 增加约定：涉及 UE 蓝图自动化探测时先复用该文档流程。
27. UE MCP 工具链切换到 `chongdashu/unreal-mcp`：
    - Codex MCP 配置切换到本地 Python server。
    - UE 项目新增 `Plugins/UnrealMCP`，并在 `StupidChessUE.uproject` 启用。
    - 修复 UE5.7 编译兼容：`ANY_PACKAGE` 兼容宏、`MCPServerRunnable.cpp` 缓冲常量重命名。
    - `StupidChessUEEditor` 在 UE 5.7 下编译通过。
28. UnrealMCP fork 工作流落地：
    - 新增 fork 仓库：`git@github.com:MotionlessPeri/unreal-mcp.git`（本地路径 `D:\git_projects\unreal-mcp`，配置 upstream）。
    - 在 fork 中完成蓝图查找逻辑增强：支持完整路径（如 `/Game/WBP_LocalMatchDebug`）与名称自动搜索（不再限定 `/Game/Blueprints`、`/Game/Widgets`）。
    - 在 fork 中合入 `Python/unreal_mcp_server.py` 的 `FastMCP` 0.x/2.x 初始化兼容补丁。
    - 新增 `tools/sync_unreal_mcp.ps1`，用于将 fork 插件同步到 `clients/ue/StupidChessUE/Plugins/UnrealMCP`。
29. UE MCP 操作文档切换完成：
    - `clients/ue/McpBlueprintWorkflow.md` 明确旧 MCP 流程弃用，统一到 UnrealMCP/fork 方案。
    - 补齐 Codex 日常操作闭环（连通性探测 -> 蓝图操作 -> Compile+Save -> 回读校验）。
    - 补齐能力边界与排障口径（端口监听、依赖版本、DLL 占用）。
30. UnrealMCP 事件绑定修复已入 fork 并同步：
    - 修复 `bind_widget_event`：从 `CreateNewBoundEventForClass` 切换到 `CreateNewBoundEventForComponent`，按组件变量生成 `UK2Node_ComponentBoundEvent`。
    - 增加参数兼容：支持 `blueprint_name + widget_name` 与 legacy `widget_name + widget_component_name` 两种调用形态。
    - Python `umg_tools.bind_widget_event` 已改为优先发送新参数。
    - 通过 `tools/sync_unreal_mcp.ps1` 同步到 `clients/ue/StupidChessUE/Plugins/UnrealMCP`。
31. `WBP_LocalMatchDebug` 自动接线推进：
    - 6 个按钮的 `OnClicked` 事件节点已通过 MCP 自动创建并绑定。
    - 已自动接线并可编译通过的链路：`BtnJoin`、`BtnCommitReveal`、`BtnBlackResign`、`BtnPullRed`、`BtnPullBlack`。
    - `BtnRedMove` 当前接为占位日志链路（`PrintString`），等待 `FStupidChessMoveCommand` 结构体入参自动化能力补齐后再切回真实 `SubmitMove`。
32. UnrealMCP 蓝图命令增强（本地 fork + 项目插件同步）：
    - 新增 `clear_blueprint_event_graph`，支持一键清理 EventGraph。
    - 新增 `add_blueprint_dynamic_cast_node`，支持在自动接线中做类型转换。
    - `connect_blueprint_nodes` 改为走 `Schema->TryCreateConnection`，拒绝非法连线，避免“连上但编译报错”。
33. 蓝图自动化脚本落地：
    - 新增 `tools/reset_local_match_widget_graph.py`（清图 + 仅重绑按钮事件）。
    - 新增 `tools/wire_local_match_widget_graph.py`（清图 + 重绑 + 完整链路重建）。
    - 脚本执行后 `WBP_LocalMatchDebug` 当前可完成无 `AssetLog` 编译错误的重建流程。
34. 蓝图事件节点布局控制：
    - `bind_widget_event` 新增 `node_position` 支持，可稳定控制红色 `OnClicked` 事件节点位置。
    - `WBP_LocalMatchDebug` 自动重建脚本已接入该参数，避免事件入口堆叠在图右侧。
35. 仓库 LFS 受限兜底完成：
    - 由于 GitHub 仓库侧返回 `Git LFS is disabled for this repository`，UE 资产跟踪策略临时切换为普通 Git 二进制对象。
    - `.gitattributes`、`docs/DependencyPolicy.md`、`clients/ue/README.md` 已同步更新为非 LFS 口径。
36. 蓝图接线脚本防误清图改造：
    - `tools/wire_local_match_widget_graph.py` 默认切换为 Preserve 模式（不执行清图），避免误删 `Construct` 与自定义回调链路。
    - 增加 `--clear` 显式开关用于全量重建，并在脚本与文档中标注该模式是 destructive。
    - `tools/README.md` 与 `clients/ue/McpBlueprintWorkflow.md` 已同步更新使用说明。
37. 委托绑定自动化已接入：
    - UnrealMCP 新增 `bind_blueprint_multicast_delegate` 命令，可自动创建 `Assign Delegate` 与匹配 `CustomEvent`。
    - `tools/wire_local_match_widget_graph.py` 已接入该命令，自动重建 `Construct -> Subsystem -> On*Parsed` 绑定链路并附带回调日志节点。
    - `Btn*` 按钮链路与 `Construct` 委托链路现在可由同一脚本统一重建。
    - 自动化脚本默认改用蓝图全路径 `/Game/WBP_LocalMatchDebug`，避免短名在 AssetRegistry 未命中时偶发 `Blueprint not found`。
38. 回调日志可读性增强：
    - `tools/wire_local_match_widget_graph.py` 将 6 个 `On*Parsed` 回调的 `PrintString` 文案改为可区分前缀（`[Callback][JoinAck]` 等），便于运行时快速定位事件来源。
39. 蓝图自动接线落盘保障：
    - `tools/wire_local_match_widget_graph.py` 在编译后补一次 `bind_widget_event` 触发插件侧 `SaveAsset`，避免“脚本执行成功但编辑器显示/磁盘落盘不同步”。
40. 按钮点击日志自动化补齐：
    - `tools/wire_local_match_widget_graph.py` 为 6 个 `OnClicked` 入口统一插入 `PrintString`（`[Click][Btn...]`）再进入原业务链路。
    - 便于在 PIE 中快速确认“点击事件是否触发”与“触发的是哪个按钮”。
41. UnrealMCP 蓝图查找扩展到 Level Blueprint：
    - `FUnrealMCPCommonUtils::FindBlueprintByName` 现支持将 `UWorld/ULevel/ALevelScriptActor` 解析为 `ULevelScriptBlueprint`。
    - 非路径名查找新增 `/Game/Maps/<Name>` 候选路径，并补充 AssetRegistry 的 `UWorld` 回退检索。
    - 支持使用 `DebugLevel` 或 `/Game/DebugLevel` 作为 `find_blueprint_nodes` 的 `blueprint_name` 来读取 Level BP 事件节点。
42. UnrealMCP 新增 Subsystem Getter 节点创建能力：
    - `FUnrealMCPBlueprintNodeCommands` 新增 `add_blueprint_subsystem_getter_node` 命令。
    - 命令通过 `UK2Node_GetSubsystem` 直接创建 `Get <Subsystem>` 节点，可指定 `subsystem_class` 与 `node_position`。
    - `UnrealMCPBridge` 已注册该命令路由；当前待 UE 重编译加载新插件二进制后生效。
43. 蓝图自动接线脚本切换到专用 Subsystem Getter：
    - `tools/wire_local_match_widget_graph.py` 改为使用 `add_blueprint_subsystem_getter_node`，不再拼接 `GetGameInstanceSubsystem + DynamicCast`。
    - 脚本新增 `--wire-construct` 开关；默认跳过 `Construct` 重建，避免覆盖手工维护的 `Construct` 链。
    - `tools/README.md` 已同步新参数与行为说明。
44. `BtnRedMove` 自动接线恢复为真实命令链：
    - `tools/wire_local_match_widget_graph.py` 改为 `SubmitMove(Move)` + 红黑双侧 `PullParseAndDispatchOutboundMessages`。
    - `Move` 参数以结构体默认值文本写入：`PieceId=11, From(0,3)->To(0,4)`。
    - 可用于验证“红先走一步后，黑认输触发 `GameOver`”的端到端流程。
45. UnrealMCP 新增结构体与清旧链路命令：
    - 新增 `add_blueprint_make_struct_node`，用于创建 `MakeStruct` 节点并写入字段默认值（支持 `StupidChessMoveCommand`）。
    - 新增 `break_blueprint_node_pin_links`，用于断开按钮事件 `Then` 旧连线，避免 Preserve 模式下历史 TODO/旧链路继续触发。
    - `tools/wire_local_match_widget_graph.py` 已接入以上命令：`BtnRedMove` 走 `MakeStruct -> SubmitMove`，并在重接前清理 6 个按钮入口旧连线。
46. UnrealMCP 新增按钮事件去重清理能力：
    - 新增 `dedupe_blueprint_component_bound_events`，按 `widget_name + event_name` 去重 `UK2Node_ComponentBoundEvent`。
    - 去重时会连同重复事件节点下游执行链一并清理，减少 `WBP_LocalMatchDebug` 的历史残留节点。
    - `tools/wire_local_match_widget_graph.py` 绑定按钮后会自动调用去重命令，默认 Preserve 模式下也能保持按钮入口可重复执行且不堆积脏节点。
47. MCP 源码真源流程已纠偏并固化：
    - `UnrealMCP` 相关改动已迁移并提交到 fork：`MotionlessPeri/unreal-mcp@4c60eac`。
    - 本仓通过 `tools/sync_unreal_mcp.ps1 -ForkRepoRoot D:\git_projects\unreal-mcp` 同步插件副本，不再以本仓插件副本作为真源。
    - `AGENTS.md` 与 `clients/ue/McpBlueprintWorkflow.md` 已补充“先改 fork、后同步本仓”的强约束说明。
48. UE 编译故障修复（去重命令排序比较器签名）：
    - `dedupe_blueprint_component_bound_events` 的 `TArray<UK2Node_ComponentBoundEvent*>.Sort` 比较器参数由指针签名修正为引用签名。
    - 修复已先提交到 fork：`MotionlessPeri/unreal-mcp@4a5abea`，再通过 `tools/sync_unreal_mcp.ps1` 同步回本仓插件副本。
    - 命令行编译验证通过：`Build.bat StupidChessUEEditor Win64 Development ...`（Result: Succeeded）。
49. `WBP_LocalMatchDebug` 重叠节点清理与全量重建：
    - 使用 `python tools/wire_local_match_widget_graph.py --clear --wire-construct` 执行 destructive 清图重建，移除历史重复/孤立节点（本次清理 `removed_count=125`）。
    - 按钮事件入口与主链路已重建为单份可读图，不再依赖 Preserve 模式下残留节点。
    - 重建后蓝图编译通过并落盘：`compiled=true`，`/Game/WBP_LocalMatchDebug` 已保存。
50. UE 增量拉取游标接入：
    - `UStupidChessLocalMatchSubsystem` 新增 `PullParseAndDispatchOutboundMessagesIncremental`，按玩家维护 `ServerSequence` 游标，默认只拉取新消息。
    - 新增 `ResetPullCursor` 与 `GetPullCursor`，支持按玩家/全量重置与调试观测。
    - `tools/wire_local_match_widget_graph.py` 与 `clients/ue/BlueprintQuickStart.md` 已切换到增量拉取接口，减少重复回调噪声。
    - 新增 UE 自动化用例 `StupidChess.UE.CoreBridge.IncrementalPull`，覆盖“首拉->空拉->新消息->重置游标重拉”路径。
    - 修复 UE 5.7 编译兼容：`StupidChessLocalMatchSubsystemTests` 中 `TestEqual(int64, 0)` 改为显式 `int64{0}`，消除重载歧义。
51. `WBP_LocalMatchDebug` 游标可视化日志接线：
    - `tools/wire_local_match_widget_graph.py` 在每个增量拉取节点后自动追加 `GetPullCursor(PlayerId)` 调试日志（标签 + 数值）。
    - 便于在 PIE 中确认“空拉不增长 / 新消息后增长 / 重置后回退”的游标行为。
52. `WBP_LocalMatchDebug` 重置游标拉取调试按钮接线：
    - `tools/wire_local_match_widget_graph.py` 支持可选按钮 `BtnResetPullRed / BtnResetPullBlack`。
    - 若按钮存在，脚本自动接入 `PullParseAndDispatchOutboundMessagesIncremental(..., bResetCursorBeforePull=true)` 链路并打印游标。
    - 用于在 PIE 中验证“强制重放拉取”行为，不影响原有普通 Pull 按钮。
53. 回调关键字段调试摘要（脚本/Subsystem）：
    - `UStupidChessLocalMatchSubsystem` 新增 `GetCachedCommandAckDebugString` / `GetCachedGameOverDebugString`。
    - `tools/wire_local_match_widget_graph.py` 在 `--wire-construct` 模式下，`OnCommandAckParsed` / `OnGameOverParsed` 回调会额外打印缓存摘要字符串。
    - 目标是降低仅靠事件名排查问题时的信息不足。
54. 仓库忽略规则补充（UE 调试资产清理）：
    - `.gitignore` 新增 `**/__pycache__/`，避免本地 Python 缓存目录进入工作区噪声。
    - `.gitignore` 新增 `clients/ue/**/*_HLOD*_Instancing.uasset`，屏蔽 UE HLOD 生成实例化资产（如 `TestLevel_HLOD0_Instancing.uasset`）。
    - 便于将 `DebugLevel.umap` 等手工资产与可再生生成物区分管理。
55. 蓝图接线脚本 `Construct` 重建安全保护：
    - `tools/wire_local_match_widget_graph.py` 在 Preserve 模式下改为跳过 `--wire-construct`（输出警告），避免重复执行导致 `Bind Event + Custom Event` 成对堆积。
    - 文档 `tools/README.md` 与 `clients/ue/McpBlueprintWorkflow.md` 已同步“需要重建 Construct 请使用 `--clear --wire-construct`”口径。
56. UnrealMCP `add_blueprint_event_node` 生命周期事件修复（fork 同步）：
    - 在 fork `MotionlessPeri/unreal-mcp` 中将事件节点创建由手工 `NewObject<UK2Node_Event>` 改为 `FKismetEditorUtilities::AddDefaultEventNode`。
    - 修复 `WidgetBlueprint` 中 `Event Construct` 等 override/lifecycle 事件“节点可见但运行时不触发”的问题。
    - 已通过 `tools/sync_unreal_mcp.ps1` 同步到本仓插件副本，并在 `StupidChessUEEditor` 冷编译下验证通过。
57. `Construct` 入口运行时探针：
    - `tools/wire_local_match_widget_graph.py` 在 `--wire-construct` 重建的 `Event Construct` 链首节点插入 `PrintString("[Construct][Enter]")`。
    - 用于快速验证 MCP 创建的 `Event Construct` 是否在 PIE 运行时实际触发。
58. UnrealMCP 编辑器生命周期命令已同步并完成消费者侧 smoke：
    - 新增 MCP 命令：`save_dirty_assets`、`request_editor_exit`、`save_and_exit_editor`（来自 fork `MotionlessPeri/unreal-mcp`）。
    - 命令通过 `UnrealMCPBridge` 路由到 Editor 命令集，并已在 `StupidChessUE` 会话中验证：
      - `save_dirty_assets` 可返回脏包保存前后计数。
      - `save_and_exit_editor` 可先回 MCP 响应，再延迟退出 UE Editor（避免回包被关机截断）。
    - `clients/ue/McpBlueprintWorkflow.md` 已补充“优先使用 MCP 保存并关闭 Editor 再冷编译插件”的工作流口径。

## In Progress

1. 在新脚本链路上做一次稳定回归（Join -> CommitReveal -> Move -> Resign）并记录期望日志断言。
2. 冷启动验证 `bind_blueprint_multicast_delegate`（关闭 UE 后重编译插件，再执行脚本回归）。
3. 整理 `WBP_ClickProbe` 与 `DefaultEngine.ini` 的入库策略（长期调试资产 vs 本地开发偏好）。
4. 回到 `unreal-mcp` Route B：恢复并继续 UMG Designer 自动化阶段 `0+1`（命令口径对齐 + `get_widget_tree`）。

## Next Steps

1. 在蓝图层为 `S2C_GameOver` 增加终局 UI 流程（弹窗/结算态/重开入口）。
2. 增加一键“仅重接按钮链路”的自动化回归脚本（不触碰 Construct）。
3. 为 `Snapshot/EventDelta` 增加轻量调试摘要（例如阶段/回合/事件数），避免日志信息过载但仍可判定状态推进。
4. 决定是否将 `DebugLevel` 设为团队默认启动地图，并据此处理 `DefaultEngine.ini` 改动。
5. 在 `unreal-mcp` 中继续补齐 UMG Designer 自动化能力，并用 `WBP_McpUmgProbe` 做阶段性 smoke。

## Test Baseline

1. `ctest --preset vcpkg-debug-test --output-on-failure` 当前为全通过（39/39）。
2. `Build.bat StupidChessUEEditor Win64 Development ...` 当前编译通过（UE 5.7）。
3. `UnrealEditor-Cmd ... -ExecCmds="Automation RunTests StupidChess.UE.CoreBridge.LocalFlow;Quit" -culture=en` 当前通过（EXIT CODE: 0）。
4. `UnrealEditor-Cmd ... -ExecCmds="Automation RunTests StupidChess.UE.CoreBridge.ErrorPaths;Quit" -culture=en` 当前通过（EXIT CODE: 0）。
5. `UnrealEditor-Cmd ... -ExecCmds="Automation RunTests StupidChess.UE.CoreBridge.IncrementalPull;Quit" -culture=en` 当前通过（EXIT CODE: 0）。
