# 修复：世界外高度的生物 tick 导致服务器崩溃

分支：`feature/mobs-out-of-world-guard` ｜ 级别：P0（崩溃修复）｜ 前置：无

## 1. 缺陷（合并后 master `c4e613e`）

**复现（确定性，1 秒内）**：

```
/summon pig 20 64 20
/teleport @e[type=pig] 10 4096 10      # y=4096 在命令允许范围内（±4096 含端点）
```

服务器 `SIGABRT`：

```
Assertion failed: cChunkDef::IsValidRelPos(a_RelPos), file src/ChunkData.cpp, line 24
  cChunkDataStore<...>::Get
  cChunk::GetBlockTypeMeta(Vector3i)
  cMonster::InStateIdle(...)
  cMonster::Tick → cWorld::TickMobs → cWorld::Tick
```

根因：`cMonster::InStateIdle()` 查询脚下方块时只检查了下界：

```cpp
int YBelowUs = static_cast<int>(Destination.y) - 1;
if (YBelowUs >= 0)                        // ← 缺上界
{
    Chunk->GetBlockTypeMeta(RelX, YBelowUs, RelZ, BlockType, BlockMeta);
```

`cChunk` 的方块访问器要求合法高度（0..255，`cChunkDef::IsValidHeight`），越界即触发断言。
命令侧没有问题（`/teleport` 的 y 范围 ±4096 是按 1.12.2 规格实现的，见
[movement 规格](../commands/vanilla-1.12.2-movement-commands.md) §2.1：x/z 开区间 ±30,000,000、
y 含端点 ±4096）；**引擎必须容忍被传送到世界外的实体**。

## 2. 行为规格

1. vanilla 1.12.2 允许实体位于世界高度之外：`/teleport` 的 y 范围是 −4096…4096，
   `/summon`、爆炸、活塞等也可以把实体推出世界；实体在世界外继续受重力下落，**服务器不崩溃**。
   （y 范围来源：[Commands/teleport rev 1139121](https://minecraft.wiki/w/Commands/teleport?oldid=1139121)：
   「`y` must be within the range -4096 to 4096 inclusive」。）
2. 世界外的实体不应当运行依赖方块的 AI：没有方块可查，也没有可站立的地面；
   下落的物理由 `cEntity::Tick` 负责（`Super::Tick` 已经执行）。
3. Cuberite 既有行为：非玩家实体在 `y < 0` 时由 `cEntity::Tick` 直接 `Destroy()`
   （[Entity.cpp:945-950](../../src/Entities/Entity.cpp#L945-L950)）——与「掉出世界即移除」一致；
   本题不改变该规则。
4. 本条只要求「不崩溃 + 不执行方块相关 AI」；世界外实体的其它细节（虚空伤害的确切阈值、
   玩家在 y>255 的客户端表现）不在本支范围〔待核〕。

## 3. 实现映射

| 规格点 | 落点 |
|---|---|
| 世界外不运行方块相关 AI | `cMonster::Tick()`：在死亡动画处理之后、`HandleDaylightBurning` 之前，若 `GetPosY() < 0` 或 `>= cChunkDef::Height` 直接返回（`Super::Tick()` 已处理重力） |
| 脚下方块查询的高度保护 | `cMonster::InStateIdle()`：`YBelowUs >= 0` 改为 `cChunkDef::IsValidHeight({ RelX, YBelowUs, RelZ })`（防御性，早退后理论不可达） |

其它同类调用点已核实为安全：`cChunkMap::GetBlockTypeMeta` 自带高度校验（越界返回 air + false）；
`cPath::FillCellAttributes`、`cPathFinder` 的方块查询都有 `IsValidHeight` 保护；
`cPawn`/`cPlayer` 的方块查询同样有保护（`Pawn.cpp:399/443`、`Player.cpp:2202/2589`）。

## 4. 验证

### 4.1 静态与单元

- `src/CheckBasicStyle.lua`（门 1）。
- `cmake --build build`（门 2）。
- `cd build && ctest --output-on-failure -E "UrlClient-test|Google-test"`（门 3，联网测试按
  [AGENTS.local.md](../../AGENTS.local.md) §3 排除）。
- 门 4 绑定依赖：未改 `src/Bindings/`，不适用。
- 门 6 APIDump：未新增/导出 C++ API，不适用。
- 本修复是竞态/崩溃类、难以在单元测试里构造（需要真实 chunk + 生物 tick），验收以实机为准。

### 4.2 实机（真实服务器 + bot）

修复前已记录两次崩溃（`ChunkData.cpp:24`）。修复后（新二进制 + 重启）：

| 用例 | 期望 |
|---|---|
| `/summon pig 20 64 20` → `/teleport @e[type=pig] 10 4096 10`，等待 5 s | 服务器存活；猪下落，`y < 0` 后被移除 |
| 同上但 y=`-4096` | 服务器存活（`cEntity::Tick` 直接销毁） |
| 把**玩家** `/teleport @s 0 4096 0`，等待 5 s | 服务器存活；玩家下落并正常落地 |
| 常规 mob tick（附近有动物/怪物走动） | 行为不变（TPS 稳定、无断言） |

### 4.3 验收门（按 AGENTS.md §4.1，改动 `src/`）

| 门 | 结果 |
|---|---|
| 1 风格 | `src/CheckBasicStyle.lua` |
| 2 编译 | `cmake --build build` |
| 3 ctest | 排除两条联网测试 |
| 4 绑定依赖 | 不适用（未改 `src/Bindings/`） |
| 5 行为规格 | 本文件 + §4.2 |
| 6 APIDump | 不适用（未新增/导出 C++ API） |
