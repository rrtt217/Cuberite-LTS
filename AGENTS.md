# AGENTS.md

> 本文件是本仓库对 AI 编码代理（以及人类贡献者）的操作约定。**动手前先读完整份**。
>
> 本文件只收录**与具体机器、检出无关**的约定。环境特定事实（本机工具链、路径、可用工具的限制）
> 放在同目录的 `AGENTS.local.md`——它**不入库**，需各自维护；两者冲突时以本文件为准。
>
> 上游：`cuberite/cuberite`。

---

## 1. 项目与使命

- Cuberite 是用 **C++ 从零实现**的 Minecraft Java 版兼容服务器，内嵌 Lua 插件 API。
  协议层继承上游，实际接受 **1.8.x – 1.14.4** 客户端
  （[src/Protocol/ProtocolRecognizer.h:10](src/Protocol/ProtocolRecognizer.h#L10) 的 `MCS_CLIENT_VERSIONS`）；
  **parity 目标只针对 1.12.2 行为**，1.13 / 1.14 的地位见第 9 节。
- 本 fork 的目标：**以 vanilla Minecraft Java 1.12.2 为基准，近似 1:1 重现其特性**，按「已知功能缺失」逐条推进。
- 缺口来源：上游追踪 issue [#4888](https://github.com/cuberite/cuberite/issues/4888) 及其子 issue（用 `gh` 拉取）。**不在仓库内维护全局 parity ledger**——缺口由维护者逐条下达。允许的唯一例外是**任务级 roadmap**（如 `specs/vanilla-1.12.2-status-effects-roadmap.md`）：它是某次下达任务的交付物，只覆盖该任务划定的子系统，写完即冻结；roadmap 列出的条目**不自动构成新任务**，开工仍需维护者逐条下达（与第 3 节「不做自动续跑」一致）。
- 当前处于起步阶段：不追求一次性对齐，追求**每条改动可独立验证、可回滚、可评审**。

---

## 2. Cleanroom 规范（最高优先级，不可协商）

本仓库继承上游立场：**服务器从零编写，代码库不含任何 Mojang 专有代码，对此相当严格**。
对 AI 代理进一步收紧为以下四条。

### 2.1 绝对禁止

- 阅读、反编译、反汇编、反混淆 Minecraft 客户端/服务端，或引用其源码片段。
- 使用或参考 MCP、Mojang 官方 mappings 派生出的**代码**。
- 复制 CraftBukkit / Spigot / Paper 等**由 Minecraft 源码反编译派生**的项目的代码，或把它们当作实现蓝本。
- 从模型记忆中原样复现 Minecraft 源码（类名组合、方法体、常量表逐字照搬）。
- 引入 Mojang 的代码、资源、数据文件原文（第 2.2 条允许的公开数据报告与下述许可证兼容例外除外）。

**许可证兼容例外**：来源代码若使用与本仓库（Apache-2.0）兼容的宽松许可证（MIT / BSD / Apache-2.0 / Zlib / 公有领域 / CC0），允许**参照、借鉴乃至改写入库**——Mojang 官方 GitHub 组织下即有大量此类库（brigadier、DataFixerUpper、slicer 等均为 MIT），与其他满足条件的来源同等对待。条件：

1. 以**该仓库自带的 LICENSE 文件**为准逐一核实，不凭组织归属推定（Mojang 组织下也有未附许可证的仓库）；
2. 入库时保留其版权声明与许可证文本，并在交付说明中注明出处；
3. copyleft 许可证（GPL / AGPL 等）不适用本例外；LGPL / MPL 等拿不准的，先问维护者。

### 2.2 允许的行为来源（spec 白名单）

| 来源 | 用途 |
|---|---|
| [Minecraft Wiki](https://minecraft.wiki/) | 机制、数值、掉落、生成规则 |
| wiki.vg 协议文档（原站 2024-11-30 关闭：[minecraft.wiki 迁移页](https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge) / [wikivg.booky.dev 镜像](https://wikivg.booky.dev/Main_Page)） | 封包格式、登录流程、协议状态机 |
| 官方更新日志 / 补丁说明 | 版本行为变更 |
| 合法客户端 + 合法 vanilla 服务端的**实测观察** | 精确数值、边界行为 |
| 其他**独立从零实现**的服务端的行为观察（如 Glowstone） | 行为对照，**只看行为、不抄代码** |
| 公开数据报告（如原 wiki.vg Data Generators 的 `blocks.json`） | 注册表 / 方块状态数据（本仓库既有做法） |

> **数据报告边界**：白名单中的「公开数据报告」只覆盖**注册表 / 互操作数据**（方块状态、注册表 ID 映射等——协议兼容的硬事实，无法从文字规格推导），按既有做法经生成器工具转成生成物入库（`src/Registries/BlockStates.*` 模式）。**行为 / 逻辑类数据**（战利品表、配方、进度、结构 NBT 等）**不在此列**——必须从 Wiki / 实测等来源写成规格后自行实现（`Server/crafting.txt`、结构战利品 spec 模式），不得直接引入 Mojang 数据生成器的导出文件。

### 2.3 AI 特有的污染处置

- 若上下文中出现 Minecraft 反编译源码或 mappings 内容，**立即停止该任务并声明**，不要基于它继续推进。
- 每条实现都要能回答：「这个行为/数值出自哪个允许来源？」答不出就先去查，**不要凭记忆写**。
- 实现的**算法与数据结构必须自己推导**；允许来源只提供「行为规格」，不提供实现——第 2.1 条许可证兼容例外允许借鉴的除外。

### 2.4 交付时的来源声明

每个功能分支的交付说明必须包含：**行为规格 + 来源链接 + 验证方式**。
存疑内容按两级标注，不许含糊带过：

- **〔待核〕**：原则上可核实、但本次没有核实的断言（如未实际跑过的命令、未复现的观察）——
  标注时写明**怎么核**；
- **〔推测〕**：无法用允许来源佐证的行为 / 数值推断——标注时写明**依据与置信度**。

规格、交付说明、文档与代码注释里的断言同样适用。

---

## 3. 协作流程：一功能一分支

- 从 `master` 切分支：`feature/<subsystem>-<short-desc>`
  （如 `feature/protocol-shield-blocking`、`feature/mobs-mule`）。
- **绝不直接向 `master` 提交或推送**。合并是维护者的动作，判据只有一条：
  **当且仅当分支功能完善、通过全部适用验收门（第 4 节）、且维护者明确同意后，才合并进 `master`。**
  Agent **绝不自行合并**，也绝不推送 `master`。
  - **CI 与合并判据**：GitHub Actions 在本 fork 尚未激活（迁移与激活是独立任务）。
    激活前，「通过全部适用验收门」= 本地逐项跑绿，并在交付说明里给出证据；
    激活后，分支还须推送远端、**CI 全绿**才算满足判据，本地结果不再替代 CI。
- 一个分支只做一件事，且**必须能独立编译**。
- **提交粒度**：分支在请求合并前整理为**一个提交**（squash）。
  - 若功能确实横跨多个可独立评审的单元，允许保留**多个原子提交**：每个都独立可编译、可理解、
    不含半成品，并在交付说明里说明为什么不能合成一个。
  - 「原子」= 一个自洽的改动单元；禁止 `WIP` / 「再修一下」式堆叠。
  - 本规则**只约束新分支**，不改写已有历史；合并方式（FF / merge commit）由维护者定。
- 标准节奏：
  1. 拿到缺口 → 有上游 issue 编号的，用 `gh` 拉取 issue 全文；
     无编号的（维护者直接描述、或任务级 roadmap 指派的条目），
     以维护者下达的描述为任务定义，不补造 issue；
  2. **先复核该缺口在当前 HEAD 是否仍然存在**（清单来自 2020，基线 `23bca00`，本检出新 5 年）；
  3. 从白名单来源写出 vanilla 1.12.2 行为规格；
  4. 实现 + 测试，遵守第 5 节规范；
  5. 跑完第 4 节**适用的**验收门；
  6. 交付汇报（改了什么 / 规格与来源 / 证据 / 已知偏差 / 复核结论）→ **停下等维护者评审**。
- **实现途中发现别的缺口**，按它与本功能的关系分三类处理：
  1. **前置依赖**（不补上就编译不过、或无法验证本功能）：允许在同一分支内一并实现，
     但要作为**独立的原子提交**，规格与来源单独写，并在交付说明里显式列为前置依赖。
  2. **可绕过的相关缺口**：**不在本分支实现**；用最小近似或显式 TODO 绕过，记入「已知偏差」，
     并给出后续分支 / issue 建议。
  3. **无关缺口**：一律不在本分支实现，只在交付说明或规格里记录，等维护者下达。
  - 若前置依赖的规模 ≥ 本功能本身，应把它**拆成独立分支先行合并**（从 `master` 切）
    ——这是唯一需要「回头从 `master` 补」的情形。术语与叠层规则：
    - 「**落地**」= 维护者把该前置分支合入 `master`（合并是维护者的动作，见上）；
      前置分支未落地前，本功能分支**不得请求合并**。
    - 等待期间二选一：**停下等**（在交付说明里写明阻塞点，推荐）；
      或**叠层继续**——本功能分支直接从**前置分支的尖端**继续开发，
      并在交付说明首部声明「本分支叠在 `<前置分支>` 之上」。
    - 前置分支落地后，叠层分支须 `git rebase --onto master <前置分支>` 归位到新 `master`，
      确认其 diff 不再包含前置分支的提交，重跑适用的验收门，然后才交付评审。
  - 判据始终是**依赖关系**（不做它就完不成 / 验不了），而不是「顺手做掉更快」。
- **不做自动续跑**：这不是「每个分支」的附加限制，而是「一功能一分支」的推论——
  Agent 不得自行挑选与当前任务无关的缺口并一路实现到底，也不得设立「把某个维度的缺失全做完」
  这类大 Goal（例如「实现末地的所有缺失功能」）。**帮助发现与整理缺口是允许的**
  （查 issue、写规格、列候选清单），**但未经维护者下达不得自动开工实现**。
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
3. **测试**：`cd build && ctest --output-on-failure`（测试总数以 `ctest -N` 为准，**不要写死在文档里**）
   - **联网测试完整列表**（需要外部网络；无外网环境须排除，排除方式因环境而异，
     写在该环境的 `AGENTS.local.md`，不写进本文档）：
     - `UrlClient-test`（[tests/HTTP/CMakeLists.txt:87](tests/HTTP/CMakeLists.txt#L87)）
       访问 `github.com` / `cuberite.org` / `api.mojang.com`；**Debug 下超时被设为 `0xffffffff`（约 49 天）**
       （[tests/HTTP/UrlClientTest.cpp:18-22](tests/HTTP/UrlClientTest.cpp#L18-L22)），无外网时会永久挂起。
     - `Google-test`（[tests/Network/CMakeLists.txt:72](tests/Network/CMakeLists.txt#L72)）连接 `google.com:80`。
   - **新增需要外部网络的测试，必须在同一分支加进上面的列表**，漏加 = 验收门不过。
   - CI 用 Release 构建（`NDEBUG`，UrlClient 超时 10 秒），联网测试不排除；
     这些测试自身的老化（如域名失效）随 CI 迁移到 GitHub Actions 之后由独立分支修复。
4. **绑定依赖**（改了 `src/Bindings/` 时）：`cd src/Bindings && lua CheckBindingsDependencies.lua`
5. **行为**：按写好的 vanilla 1.12.2 规格逐条核对，记录证据。
6. **API 文档**（改了导出的 C++ API 时）：同步 [Server/Plugins/APIDump/](Server/Plugins/APIDump/)，并跑 APIDump 自检（流程取自 [.github/workflows/Build.yml](.github/workflows/Build.yml) 的 Docs 步骤；[cibuild.sh:48](cibuild.sh#L48) 是旧 CI 的同款步骤，随「CI 迁移到 GitHub Actions 并激活」任务一并处置）：
   ```sh
   cd build/Server
   touch apiCheckFailed.flag
   ./Cuberite <<EOF
   load APIDump
   api
   apicheck
   restart
   stop
   EOF
   ```
   通过判据：`NewlyUndocumented.lua`、`DuplicateDocs.txt`、`apiCheckFailed.flag` 三个文件**均不存在**——
   flag 由首条命令创建、`apicheck` 成功时删除；前两个文件只在发现问题时生成。

### 4.1 验收门按改动面选

改动**没有触及**的领域，其门记为「不适用」，而不是「跳过不查」：

| 分支改动面 | 1 风格 | 2 编译 | 3 ctest | 4 绑定依赖 | 5 行为规格 | 6 APIDump |
|---|---|---|---|---|---|---|
| `src/`、`tests/`、CMake 构建系统 | ✅ | ✅ | ✅ | 改 `src/Bindings/` 时 | ✅ | 导出 C++ API 时 |
| 仅新增 / 修改测试 | ✅ | ✅ | ✅（新测试必须真的被 `ctest` 跑到） | ❌ | ✅ | ❌ |
| 仅数据（`Server/Prefabs/`、`*.example.ini`、协议数据） | ❌ 无 `.cpp`/`.h` | 需要时 | 必须有能加载该数据的测试，或写明可复现的手工验证 | ❌ | ✅ | ❌ |
| 仅文档（`specs/`、`dev-docs/`、`*.md`） | ❌ | ❌ | ❌ | ❌ | 文档自身即交付物，见下 | ❌ |
| 仅 Agent 规范（`AGENTS*.md`、`.gitignore`） | ❌ | ❌ | ❌ | ❌ | 写清约定变更即可 | ❌ |

- **纯文档 / 数据分支只需一道实质门：内容正确性。** 但它是硬要求——
  文档里引用的**每个文件路径、行号、命令、issue / PR 编号都必须真实存在且可复核**；
  核实不了、或本次暂未核实的，按 §2.4 的两级约定分别标注〔待核〕/〔推测〕。
- `CheckBasicStyle.lua` 只处理 `.cpp` / `.h`（[src/CheckBasicStyle.lua:34-40](src/CheckBasicStyle.lua#L34-L40)），
  对纯文档 / 数据分支**无事可做**——没跑风格检查不算缺门。
- 反过来：**只要动了 `src/`，六道门就全部进入评估**——门 1 / 2 / 3 / 5 恒适用，门 4 / 6 按上表条件触发；凡适用项**不许因为「改动很小」省略**。

### 4.2 测试组织（与仓库既有风格一致）

- 一个测试 = **一个独立可执行 + 一个 `add_test`**，源码放 `tests/<子系统>/`，数据文件与源码同目录。
- 用 [tests/TestHelpers.h](tests/TestHelpers.h) 的 `IMPLEMENT_TEST_MAIN` + `TEST_*` 宏；
  不引入新框架（`TEST_GLOBALS` 已在 [tests/CMakeLists.txt](tests/CMakeLists.txt) 定义）。
- CMake 块照既有写法：`add_executable(...)` → `target_link_libraries(... GeneratorTestingSupport)` →
  `add_test(...)`；需要数据文件时加 `WORKING_DIRECTORY`，指向数据所在目录
  （如 `WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/Server`）。
- **FOLDER 分组全仓统一**：测试目标按 `FOLDER Tests/<子系统>`、辅助库目标按 `FOLDER Tests/Libraries`
  分组；**同一 FOLDER 在同一文件里有且仅有一个** `set_target_properties` 调用，覆盖该文件全部同类目标。
  新增目标 = 把目标名加进对应列表（`add_executable` 须写在该调用之前）；**不另开新调用**。
  文件原本缺分组调用的**补上**，不留例外。
- 命名：**新测试**的目标名与 `add_test` 的 `NAME` 统一用 `<Feature>Test`（两者同名）。
  仓库并存上游遗留的 `<name>-exe` / `<name>-test` 旧风格（如 `Google-exe` / `Google-test`、
  `BoundingBox-exe` / `BoundingBox-test`）——**不仿效，也不批量改名**（避免无意义 churn）。
- 每个测试文件顶部写清三件事：**它验证哪条规格**（指向 `specs/*.md`）、**不变量**、**为什么这样验证**；
  常量用具名 `TEST_*` 而不是裸魔术数字。

> **没有可用 oracle 时**：行为验收以「规格 + 单元测试」为准，实机对照后续补。本机是否具备实机环境见 `AGENTS.local.md`。

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
| `lib/` | 14 个第三方 submodule | ❌ 只读（仅依赖更新任务可动指针） |
| `Server/` | 运行环境：默认插件、配置、协议数据 | ⚠️ 区分源码与运行生成物 |
| `Server/Plugins/APIDump/` | 插件 API 权威文档源（`Hooks/` 70 个） | ✅ 改 API 时同步 |
| `Server/Plugins/{Core,ChatLog,ProtectionAreas}` | 默认插件 submodule | ⚠️ 只在 fork 中改（见下方纪律） |
| `tests/` | CTest 单元测试 | ✅ |
| `Tools/` | 工具（ProtoProxy、方块调色板生成器等） | ⚠️ 按需 |
| `Tools/BlockTypePaletteGenerator/lib/lunajson` | 第三方 submodule（纯 Lua JSON 库） | ❌ 只读（仅依赖更新任务可动指针） |

**永不手改的生成文件**：`src/Bindings/Bindings.cpp|h`、`LuaState_Declaration.inc`、`LuaState_Typedefs.inc`、`LuaState_Implementation.cpp`、`src/Registries/BlockStates.cpp|h`。
绑定由 [CMake/GenerateBindings.cmake](CMake/GenerateBindings.cmake) 调用 `BindingsProcessor.lua` 生成。

**submodule 纪律**：submodule 的文件**永远不在父仓库直接提交**——父仓库只记录指针。按用途分两档。

**（一）默认插件 `Server/Plugins/*`（`Core` / `ProtectionAreas` / `ChatLog`）——按功能需要，可以改。**
某些 vanilla parity 缺口只能在插件侧落地（如 Core 缺少的命令 / 行为）。
本仓库预期**长期维持接近硬分叉状态**：`.gitmodules` 指向贡献者 fork 是常态而非过渡，
**不要求切回上游**。此时：

1. **先在贡献者自己的账号下 fork** 对应上游仓库（`cuberite/Core` → `<你>/Core`）。
   **绝不以 `cuberite` 组织名义提交**，也**绝不在父仓库直接写这些文件**。
2. 在 fork 上按同样的「一功能一分支」提交；分支命名、cleanroom、验收门要求与父仓库一致。
3. 在父仓库的同一功能分支里**成对**完成两件事：
   - 把 `.gitmodules` 中该 submodule 的 `url` 指向 fork（否则新 clone / CI 取不到该提交）；
   - 把 gitlink 指针更新为 fork 上的那个提交。
4. 交付说明必须同时写明：父仓库分支、fork 仓库与分支/提交号。

**（二）第三方依赖（`lib/*`、`Tools/BlockTypePaletteGenerator/lib/lunajson`）——默认只读，当且仅当依赖更新类任务可动。**
- 默认**绝不修改**其文件；需要不同行为时，改父仓库侧代码，或先在依赖上游落地。
- 仅当任务本身就是**依赖更新**（升版本、取上游修复、安全补丁）时，才允许改 gitlink 指针与必要的
  `.gitmodules` 条目；更新需单独成分支，并给出上游来源与验证方式。

**目录级 `AGENTS.md`（子项目约定）：默认不建。** 只在**同时**满足两条时才建：

1. 该目录有**与根约定不同、且容易出错**的规则（例：`src/Bindings/` 的生成物边界与 tolua 崩服原语）；
2. 该目录**经常被触及**。

- 粒度：只写该目录特有的边界、陷阱与验证方式，**不复述根文件**；建议 ≤ 30 行，
  每条都要能回答「违反了会出什么错」。
- **限制**：目录级文件不在会话基准里，只有走到该目录（或议程触及）时才加载，
  因此**不能承载必须始终生效的强制规则**——那类规则属于根 `AGENTS.md` 或 `AGENTS.local.md`。
- 按需候选：`src/Bindings/AGENTS.md`、`tests/AGENTS.md`、`Server/Prefabs/AGENTS.md`。
  没有这种需求就不要建：多一个文件就多一处会过期、会互相矛盾的约定。

---

## 7. 构建与运行

- 构建目录 `build/` 已被 `.gitignore` 覆盖；CMake 会把 `Server/` 内容**符号链接**到 `build/Server/`。
- **服务器从 `build/Server` 运行**；**任何会写运行期产物的可执行（服务器、工具、测试）都不得在被 git 跟踪的源码目录里运行**——
  不止「不在源码树根跑服务器」（那会生成 `settings.ini`、`world/`、`logs/`），
  在 `src/`、`Server/`、`tests/` 等任何入库目录里跑同类程序同样禁止；
  一律以 `build/` 下或临时目录为 CWD。
  只读地使用源码目录做 CWD（如 `WORKING_DIRECTORY` 指向数据目录的测试）不受此限。
- 配置采用 **`.example.ini` 回退**：缺 `<name>.ini` 时读 `<name>.example.ini`（[src/IniFile.cpp:70](src/IniFile.cpp#L70)）。要改默认值就改模板。
- 运行期产物（`settings.ini`、`world*`、`logs`、`players`、`*.sqlite`、`cuberite_api.lua` 等）已被 gitignore，**不要提交**。
- CMake 选项：`SELF_TEST`（开测试）、`BUILD_TOOLS`、`BUILD_UNSTABLE_TOOLS`、`PRECOMPILE_HEADERS`、`UNITY_BUILDS`、`WHOLE_PROGRAM_OPTIMISATION`。
- 本机工具链版本、磁盘余量与构建并行度约束是环境特定的，见 `AGENTS.local.md`。

---

## 8. 可用工具

- **`gh` CLI**：读上游 issue / PR。若环境无法直接抓取 GitHub 网页，**GitHub 内容一律走 `gh`**（本机限制见 `AGENTS.local.md`）。
- **`cuberite-plugin` skill**：写/改 Lua 插件或做 tolua 内省前**必须先加载**；它面向安装实例，文档结构可能与本检出不同，**以本仓库实际为准**。
- **`ctest`**：单元测试。
- **`Tools/ProtoProxy`**：协议包级抓取与对比，适合网络/协议类改动。
- 插件 API 优先查 [Server/Plugins/APIDump/](Server/Plugins/APIDump/) 与在线 api.cuberite.org。

---

## 9. 高风险区

- **线程模型**：`cWorld` 继承 `cIsThread`，每个世界在自己的线程 tick；跨线程共享状态用 `cCriticalSection` / `cEvent`，**不要阻塞 tick 线程**。OS 操作走 `cFile` / `cIsThread` / `cStopwatch` 封装。
- **tolua 崩服原语**：`tolua.cast` 到未注册类型、`tolua.inherit` 传 userdata 都会**整服 SIGSEGV**；细节见 `cuberite-plugin` skill。
- **协议边界**：parity 与行为实现只以 **1.12.2** 为准，不要顺手引入 1.13+ 的行为。
  - **1.8 – 1.11 客户端**：协议层只做**封包格式翻译**，服务端行为统一按 1.12.2 实现；
    **不实现历史版本行为**（如 1.8 战斗、旧版移动 / 碰撞规则）——
    「按客户端版本切换行为」类需求是独立任务，未经下达不得做。
  - 上游遗留的 **1.13 / 1.14 协议实现**（`src/Protocol/Protocol_1_13.*`、`src/Protocol/Protocol_1_14.*`、
    `Server/Protocol/{1.13,1.14.4}`）**处于冻结状态**：不扩展、不做 parity、不主动修；
    parity 改动**无需保持 1.13 / 1.14 客户端行为**，但也不得无谓破坏其编译。
    整体移除是独立任务，未经下达不得顺手做。
- **文件边界**：`lib/` 与 `lunajson` 等第三方依赖默认只读；`Server/Plugins/*` submodule 只在 fork 中改；生成文件、运行期产物一律不碰（细则见第 6 节）。

---

## 10. 禁止清单（TL;DR）

1. 不读、不抄 Minecraft 源码或反编译产物、MCP/mappings、Spigot/Paper 类反编译派生代码（§2.1 的许可证兼容例外除外）。
2. 不凭模型记忆复现 vanilla 源码。
3. 不直接提交 / 推送 `master`、不自行合并（合并需维护者同意）、不自动续跑实现无关缺口。
4. 不手改生成文件、不在父仓库提交 `lib/` 与第三方依赖、不以 `cuberite` 名义提交、不提交运行期产物。
5. 不在入库目录里运行会写运行期产物的可执行（服务器只是其一）；服务器从 build/Server 跑。
6. 不为单个数值/行为「猜」，必须查允许来源。
7. 不在一个分支里混做多个不相关改动。
