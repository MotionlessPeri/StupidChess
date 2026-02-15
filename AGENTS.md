# AGENTS.md

## 开工前必读

每次开始实现、重构或修复前，按顺序阅读：

1. `docs/Requirements.md`
2. `docs/RuleSpec.md`
3. `docs/Architecture.md`
4. `docs/InterfaceSpec.md`
5. `docs/DependencyPolicy.md`
6. `docs/CommitConvention.md`
7. `docs/ADR/0001-Repo-Strategy.md`

冲突处理：

1. 若文档与临时聊天描述冲突，以文档为准。
2. 若文档彼此冲突，以编号靠前者为准，并在同次提交修正文档不一致。

## 项目目标

1. 核心规则层平台无关，基于纯 `C++20`。
2. 当前优先级为联网 PvP。
3. 设计必须兼容后续 AI（含强化学习）接入。
4. 客户端目标平台为 UE 与微信小程序。

## 强约束

1. 命名风格采用 UE 习惯（大驼峰，布尔使用 `b` 前缀）。
2. 规则引擎不得依赖 UE 类型或运行时。
3. 服务端是权威判定源，客户端不做最终规则裁决。
4. 未经确认不得修改 `RuleSpec` 既定规则口径。
5. 第三方依赖引入必须遵循 `docs/DependencyPolicy.md`。

## 实现协作约定

1. 新增规则或协议字段时，必须同步更新 `docs/RuleSpec.md` 与 `docs/InterfaceSpec.md`。
2. 任何跨端行为变化都要补充 ADR，或在现有 ADR 追加 `Decision Update`。
3. 提交前至少执行核心规则单测与一个回放一致性用例。
4. 每次代码改动都要同步更新相关文档（至少包括受影响模块的设计/规范/依赖说明）。
