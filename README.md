# StupidChess

中国象棋变体项目，包含盲摆、表面职业/实际职业切换、冻结与 `Pass` 扩展规则。

## 仓库结构

```text
docs/              # 需求、规则、架构、ADR、接口入口
core/              # 平台无关规则核心（C++20）
server/            # 联网 PvP 权威服务端
clients/ue/        # UE 客户端
clients/miniapp/   # 微信小程序客户端
protocol/          # 跨端协议定义
tools/             # 构建/测试工具脚本
```

## 开发前阅读

按顺序阅读：

1. `docs/Requirements.md`
2. `docs/RuleSpec.md`
3. `docs/Architecture.md`
4. `docs/InterfaceSpec.md`
5. `docs/DependencyPolicy.md`
6. `docs/CommitConvention.md`
7. `docs/ADR/0001-Repo-Strategy.md`

## 当前技术基线

1. 语言标准：`C++20`
2. 命名风格：UE 风格（大驼峰，布尔 `b` 前缀）
3. 服务端权威判定，客户端做展示与输入
4. 依赖管理：`vcpkg` manifest（`vcpkg.json`）

## 构建与测试

推荐方式（`CMakePresets.json`）：

```powershell
cmake --preset vcpkg-debug
cmake --build --preset vcpkg-debug-build
ctest --preset vcpkg-debug-test
```

说明：

1. 预设默认通过 `$env{VCPKG_ROOT}` 解析 vcpkg toolchain。
2. 如果只想编译不跑测试，可用：
   - `cmake --preset vcpkg-debug-no-tests`
   - `cmake --build --preset vcpkg-debug-no-tests-build`

手动方式（Windows PowerShell）：

```powershell
$VcpkgRoot = "D:\\path\\to\\vcpkg"
cmake -S . -B build `
  -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot\\scripts\\buildsystems\\vcpkg.cmake" `
  -DVCPKG_MANIFEST_FEATURES=tests
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```
