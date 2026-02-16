# Architecture

## 1. 仓库策略

当前采用单仓（Monorepo）：

1. `core/`：平台无关规则库（纯 C++20）。
2. `server/`：联网对战权威服务。
3. `clients/ue/`：UE 客户端实现。
4. `clients/miniapp/`：微信小程序客户端实现。
5. `protocol/`：跨端共享协议定义与版本策略。
6. `docs/`：需求、规则、接口、ADR 与协作约束。

## 2. 组件职责

### 2.1 CoreRules

1. 维护 `GameState`。
2. 接收命令并执行规则判定。
3. 生成合法步、局面事件、终局判定。
4. 提供可见性投影接口，避免信息泄露。

### 2.2 MatchServer

1. 房间管理与玩家会话。
2. `Commit-Reveal` 验证。
3. 权威命令判定与事件广播。
4. 事件日志与回放持久化。

### 2.3 Clients

1. 仅处理输入、表现、网络同步。
2. 不做最终规则裁决。
3. 使用 `Snapshot + EventDelta` 维护前端状态。

## 3. 构建策略

1. 统一语言标准：`C++20`。
2. 核心库与服务端使用 CMake。
3. UE 通过 Adapter 调用 CoreRules，不直接改写规则语义。
4. MiniApp 分阶段实现：
   - 阶段 1：薄客户端，服务端权威。
   - 阶段 2：WASM 本地预演与提示。
5. 第三方依赖通过 `vcpkg` manifest（`vcpkg.json`）统一管理。
6. 默认构建入口使用 `CMakePresets.json`，避免 IDE 与命令行的 toolchain 参数漂移。

## 4. 版本策略

1. 协议包含 `RuleVersion`。
2. 协议包含 `ConfigHash`。
3. 回放元数据包含：`EngineVersion + RuleVersion + ConfigHash + Seed`。

## 5. AI/RL 接入点

1. Core 提供 `Observation / ActionMask / Step / Reset`。
2. 训练环境复用服务端逻辑或纯 Core 仿真。
3. 训练与实战使用同一规则引擎，避免语义偏差。

## 6. 依赖治理

1. 默认不引入第三方库，优先标准库与自实现。
2. 所有新增依赖必须遵循 `docs/DependencyPolicy.md` 的评估流程。
3. 测试框架统一使用 `gtest`，由 `CTest` 进行测试发现和执行。
