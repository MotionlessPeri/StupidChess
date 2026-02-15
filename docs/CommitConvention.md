# CommitConvention

## 目标

统一提交信息结构，提高历史可读性与回滚效率。

## 提交格式

```text
<type>(<scope>): <subject>

Why:
- ...

What:
- ...

Validation:
- ...
```

## Type 列表

1. `feat`：新增功能
2. `fix`：缺陷修复
3. `refactor`：重构（不改变外部行为）
4. `test`：测试新增或调整
5. `docs`：文档变更
6. `build`：构建系统或依赖配置
7. `ci`：CI 流程变更
8. `perf`：性能优化
9. `chore`：杂项维护
10. `revert`：回滚提交

## Scope 约定

1. `repo`
2. `core`
3. `protocol`
4. `server`
5. `ue`
6. `miniapp`
7. `tests`
8. `docs`

## Subject 规则

1. 使用祈使句，简洁描述动作。
2. 不以句号结尾。
3. 建议不超过 72 个字符。
4. 避免空泛描述（例如 `update`、`fix stuff`）。

## Body 规则

1. 必须包含 `Why`、`What`、`Validation` 三段。
2. `Why` 说明变更动机与问题背景。
3. `What` 列出关键改动点，不写实现细节噪声。
4. `Validation` 列出实际执行过的验证命令或测试项。
5. 若改动影响规则、架构、依赖或流程，`What` 中必须包含对应文档更新项。

## Footer 规则

按需追加：

1. `BREAKING CHANGE: ...`
2. `Refs: #<issue-id>`
3. `Co-authored-by: ...`

## 示例

```text
feat(core): implement legal move generation and check filtering

Why:
- Move validation was placeholder-only and blocked server authority checks.

What:
- Implement pseudo move generation for all Xiangqi roles.
- Add self-check filtering and king-facing validation.
- Wire move execution with reveal/freeze transitions.

Validation:
- cmake --build build --config Debug
- ctest --test-dir build -C Debug --output-on-failure
```
