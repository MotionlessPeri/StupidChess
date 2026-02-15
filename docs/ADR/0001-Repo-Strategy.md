# ADR-0001: Repo Strategy

- Status: Accepted
- Date: 2026-02-15

## Context

项目包含平台无关规则核心、联网服务端、UE 客户端、微信小程序客户端。当前规则仍在快速演进，且需要高频联调。

## Decision

1. 采用单仓（Monorepo）作为当前阶段默认策略。
2. 核心规则库保持平台无关（纯 C++20），通过清晰接口暴露给服务端与客户端适配层。
3. 暂不使用 Submodule 拆仓。

## Rationale

1. 规则变更时可在一次提交中同步修改 `core/server/clients/protocol/docs`。
2. 降低多仓版本漂移、Submodule 指针错位和联调复杂度。
3. 对首发联网 PvP 的迭代速度最优。

## Consequences

1. 需要更严格的目录边界与文档治理，防止耦合扩散。
2. CI 需要按模块分层执行测试，保持反馈速度。

## Revisit Conditions

满足以下条件后评估拆仓：

1. 核心接口与规则版本连续稳定多个里程碑。
2. 有明确外部复用场景（第三方项目独立依赖 Core）。
3. 已建立包管理与版本发布流程（优先包分发，次选 Submodule）。

## Alternatives Considered

1. 多仓 + Submodule：早期联调成本高，规则快速变化阶段收益不足。
2. 多仓 + 二进制包：前期发布链路成本高，不利于快速试错。
