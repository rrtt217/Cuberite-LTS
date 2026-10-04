# AGENTS.md

> 本文件是本仓库对 AI 编码代理（以及人类贡献者）的操作约定。**动手前先读完整份**。
>
> 适用范围：`/home/david/Cuberite-LTS`（fork：`rrtt217/Cuberite-LTS`，上游：`cuberite/cuberite`）。

---

## 1. 项目与使命

- Cuberite 是用 **C++ 从零实现**的 Minecraft Java 版兼容服务器，内嵌 Lua 插件 API，支持协议 **1.8 – 1.12.2**。
- 本 fork 的目标：**以 vanilla Minecraft Java 1.12.2 为基准，近似 1:1 重现其特性**，按「已知功能缺失」逐条推进。
- 缺口来源：上游追踪 issue [#4888](https://github.com/cuberite/cuberite/issues/4888) 及其子 issue（用 `gh` 拉取）。**不在仓库内维护 parity ledger**——缺口由维护者逐条下达。
- 当前处于起步阶段：不追求一次性对齐，追求**每条改动可独立验证、可回滚、可评审**。

---

## 2. Cleanroom 规范（最高优先级，不可协商）

本仓库继承上游立场：**服务器从零编写，代码库不含任何 Mojang 代码，对此相当严格**。
对 AI 代理进一步收紧为以下四条。

### 2.1 绝对禁止

- 阅读、反编译、反汇编、反混淆 Minecraft 客户端/服务端，或引用其源码片段。
- 使用或参考 MCP、Mojang 官方 mappings 派生出的**代码**。
- 复制 CraftBukkit / Spigot / Paper 等**由 Minecraft 源码反编译派生**的项目的代码，或把它们当作实现蓝本。
- 从模型记忆中原样复现 Minecraft 源码（类名组合、方法体、常量表逐字照搬）。
- 引入 Mojang 的代码、资源、数据文件原文（第 2.2 条允许的公开数据报告除外）。

### 2.2 允许的行为来源（spec 白名单）

| 来源 | 用途 |
|---|---|
| [Minecraft Wiki](https://minecraft.wiki/) | 机制、数值、掉落、生成规则 |
| [wiki.vg / 协议文档](https://wiki.vg/) | 封包格式、登录流程、协议状态机 |
| 官方更新日志 / 补丁说明 | 版本行为变更 |
| 合法客户端 + 合法 vanilla 服务端的**实测观察** | 精确数值、边界行为 |
| 其他**独立从零实现**的服务端的行为观察（如 Glowstone） | 行为对照，**只看行为、不抄代码** |
| 公开数据报告（如 wiki.vg Data Generators 的 `blocks.json`） | 注册表 / 方块状态数据（本仓库既有做法） |

### 2.3 AI 特有的污染处置

- 若上下文中出现 Minecraft 反编译源码或 mappings 内容，**立即停止该任务并声明**，不要基于它继续推进。
- 每条实现都要能回答：「这个行为/数值出自哪个允许来源？」答不出就先去查，**不要凭记忆写**。
- 实现的**算法与数据结构必须自己推导**；允许来源只提供「行为规格」，不提供实现。

### 2.4 交付时的来源声明

每个功能分支的交付说明必须包含：**行为规格 + 来源链接 + 验证方式**。无法用允许来源佐证的行为，必须显式标注为「推测」。

---

## 3. 协作流程：一功能一分支

- 从 `master` 切分支：`feature/<subsystem>-<short-desc>`
  （如 `feature/protocol-shield-blocking`、`feature/mobs-mule`）。
- **绝不直接向 `master` 提交或推送**；**绝不自动合并**。
- 一个分支只做一件事，且**必须能独立编译**。
- 标准节奏：
  1. 拿到缺口编号 → 用 `gh` 拉取 issue 全文；
  2. **先复核该缺口在当前 HEAD 是否仍然存在**（清单来自 2020，基线 `23bca00`，本检出新 5 年）；
  3. 从白名单来源写出 vanilla 1.12.2 行为规格；
  4. 实现 + 测试，遵守第 5 节规范；
  5. 跑完第 4 节全部验收门；
  6. 交付汇报（改了什么 / 规格与来源 / 证据 / 已知偏差 / 复核结论）→ **停下等维护者评审合并**。
- **不创建持久 Goal，不做自动续跑循环**；合并点始终由人把关。
- 提交信息聚焦单一改动；修复 issue 用 PR 描述里的 `Fixes #NNNN`，**不要写进 commit message**。

---

## 4. 验收门（Definition of Done）

一个功能只有在下列各项全绿时才算「经验证可合并」：

1. **风格**：`cd src && lua CheckBasicStyle.lua`
2. **编译**：
   ```sh
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=DEBUG -DSELF_TEST=Yes -DBUILD_TOOLS=Yes
   cmake --build build
   ```
3. **测试**：`cd build && ctest --output-on-failure`（当前 28 个测试）
4. **绑定依赖**（改了 `src/Bindings/` 时）：`cd src/Bindings && lua CheckBindingsDependencies.lua`
5. **行为**：按写好的 vanilla 1.12.2 规格逐条核对，记录证据。
6. **API 文档**（改了导出的 C++ API 时）：同步 [Server/Plugins/APIDump/](Server/Plugins/APIDump/)，并跑 APIDump 自检——
   `NewlyUndocumented.lua` / `DuplicateDocs.txt` / `apiCheckFailed.flag` 必须均无输出。

> **本环境不跑实机**：没有 vanilla 1.12.2 服务端/客户端可作 oracle。行为验收以「规格 + 单元测试」为准，实机对照后续补。

---

## 5. 代码规范（CI 强制）

完整版见 [CONTRIBUTING.md](CONTRIBUTING.md)，速记：

- **C++17**；行首用 **Tab** 缩进，行内用 **空格** 对齐。
- 命名：类 `cXxx`、私有成员 `m_Xxx`、参数 `a_Xxx`、全局 `g_Xxx`；CamelCase。
- 控制语句必须带大括号；运算符两侧空格；`if (`/`for (`/`while (` 后有空格；`case` 多缩进一级。
- 函数之间空 **5 行**；`////` 分隔线恰好 **80** 个斜杠。
- 行尾注释前留**两个空格**再加 `// `。
- `#include` 相对源码根目录；文件末尾留空行。
- 坐标用具名类型（`Vector3i` / `Vector3d` / `cChunkCoords` / `cCuboid` / `cBoundingBox`），禁止三个裸 int。
- 禁止魔法数字（用 `E_BLOCK_*` / `E_ITEM_*` / `E_META_*` / `gmXxx` / `dimXxx`）；优先 `IsXxx()` 判定而非比较取值。
- 新增/导出 C++ 成员 → 同步 `src/Bindings/AllToLua.pkg` + 手写绑定 + APIDump 文档三处。

---

## 6. 仓库地图与边界

| 路径 | 说明 | 能否修改 |
|---|---|---|
| `src/` | C++ 源码，主战场 | ✅ |
| `src/Bindings/` | tolua++ 绑定：生成物 + 手写 | ⚠️ 只改手写部分与 `.pkg` / `LuaFunctions.h` |
| `src/Registries/BlockStates.*` | 生成物（风格检查已豁免） | ❌ |
| `lib/` | 14 个第三方 submodule | ❌ 绝不 |
| `Server/` | 运行环境：默认插件、配置、协议数据 | ⚠️ 区分源码与运行生成物 |
| `Server/Plugins/APIDump/` | 插件 API 权威文档源（`Hooks/` 70 个） | ✅ 改 API 时同步 |
| `Server/Plugins/{Core,ChatLog,ProtectionAreas}` | submodule | ⚠️ 到各自仓库改 |
| `tests/` | CTest 单元测试 | ✅ |
| `Tools/` | 工具（ProtoProxy、方块调色板生成器等） | ⚠️ 按需 |

**永不手改的生成文件**：`src/Bindings/Bindings.cpp|h`、`LuaState_Declaration.inc`、`LuaState_Typedefs.inc`、`LuaState_Implementation.cpp`、`src/Registries/BlockStates.cpp|h`。
绑定由 [CMake/GenerateBindings.cmake](CMake/GenerateBindings.cmake) 调用 `BindingsProcessor.lua` 生成。

**submodule 纪律**：`lib/*` 与默认插件的改动去各自上游提交，再更新父仓库指针；不在父仓库直接提交它们的文件。

---

## 7. 构建、运行与本地环境

- 构建目录 `build/` 已被 `.gitignore` 覆盖；CMake 会把 `Server/` 内容**符号链接**到 `build/Server/`。
- **服务器从 `build/Server` 运行**；**绝不在源码树根直接跑服务器**（否则会生成 `settings.ini`、`world/`、`logs/`）。
- 配置采用 **`.example.ini` 回退**：缺 `<name>.ini` 时读 `<name>.example.ini`（[src/IniFile.cpp:70](src/IniFile.cpp#L70)）。要改默认值就改模板。
- 运行期产物（`settings.ini`、`world*`、`logs`、`players`、`*.sqlite`、`cuberite_api.lua` 等）已被 gitignore，**不要提交**。
- CMake 选项：`SELF_TEST`（开测试）、`BUILD_TOOLS`、`BUILD_UNSTABLE_TOOLS`、`PRECOMPILE_HEADERS`、`UNITY_BUILDS`、`WHOLE_PROGRAM_OPTIMISATION`。
- 本地环境（写此文件时）：GCC/G++ **16.2.1**（CI 矩阵还含 clang，但本地**无 clang**）、cmake 4.3.0、ninja 1.13.2、
  `lua` 5.4.8（仓库内嵌 Lua 5.1）、luajit 2.1、luacheck 1.2.0、16 核、磁盘仅剩 **14GB（92% 已用）**。
  → 只保留一个 build 目录，避免并行全量构建。

---

## 8. 可用工具

- **`gh` CLI**（已认证 `rrtt217`）：读上游 issue / PR。本机 `web_fetch` 对 `github.com`、`raw.githubusercontent.com` 解析失败，**GitHub 内容一律走 `gh`**。
- **`cuberite-plugin` skill**：写/改 Lua 插件或做 tolua 内省前**必须先加载**。
  注意它面向 `/home/david/Cuberite` 安装实例，与本检出的文档结构有差异（本仓库 `APIDump/Classes/` 是 9 个聚合文件，非一类一文件），**以本仓库实际为准**。
- **`ctest`**：单元测试。
- **`Tools/ProtoProxy`**：协议包级抓取与对比，适合网络/协议类改动。
- 插件 API 优先查 [Server/Plugins/APIDump/](Server/Plugins/APIDump/) 与在线 api.cuberite.org。

---

## 9. 高风险区

- **线程模型**：`cWorld` 继承 `cIsThread`，每个世界在自己的线程 tick；跨线程共享状态用 `cCriticalSection` / `cEvent`，**不要阻塞 tick 线程**。OS 操作走 `cFile` / `cIsThread` / `cStopwatch` 封装。
- **tolua 崩服原语**：`tolua.cast` 到未注册类型、`tolua.inherit` 传 userdata 都会**整服 SIGSEGV**；细节见 `cuberite-plugin` skill。
- **协议边界**：基线是 1.8 – 1.12.2；不要顺手引入更新版本的行为。
- **文件边界**：`lib/`、生成文件、submodule、运行期产物一律不碰。

---

## 10. 禁止清单（TL;DR）

1. 不读、不抄 Minecraft 源码或反编译产物、MCP/mappings、Spigot/Paper 类反编译派生代码。
2. 不凭模型记忆复现 vanilla 源码。
3. 不直接提交 `master`、不自动合并、不自动续跑。
4. 不手改生成文件、不碰 `lib/`、不提交运行期产物。
5. 不在源码树里运行服务器。
6. 不为单个数值/行为「猜」，必须查允许来源。
7. 不在一个分支里混做多个不相关改动。
