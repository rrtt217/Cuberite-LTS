# Vanilla 1.12.2 — 末影之眼（Eye of Ender）

分支：`feature/items-eye-of-ender`，**堆叠在 `feature/generating-stronghold` 之上**（依赖要塞生成器与
`Server/Prefabs/PieceStructures/Stronghold.cubeset`；合并顺序必须是先要塞、后眼睛）。
上游缺口：[#3445 Eye of Ender issues](https://github.com/cuberite/cuberite/issues/3445)
（"can't be thrown"），是末地流程清单 [#4907 The End MEGA issue](https://github.com/cuberite/cuberite/issues/4907) 的第二项。

## 1. 来源（cleanroom 白名单）

- [Minecraft Wiki — Eye of Ender](https://minecraft.wiki/w/Eye_of_Ender)：飞行行为、上下指示、2–3 秒后掉落/碎裂、
  20% 碎裂 / 80% 存活、仅主世界有效、音效标识与音量音高
- [Minecraft Wiki — Java Edition pre-flattening data values/Entity IDs](https://minecraft.wiki/w/Java_Edition_pre-flattening_data_values/Entity_IDs)：
  `eye_of_ender_signal` 的 **Network ID = 72**（同表 `ender_pearl`=65、`fireball`=63、`small_fireball`=64、
  `wither_skull`=66、`potion`=73、`xp_bottle`=75、`fireworks_rocket`=76、`dragon_fireball`=93 与本仓库既有表一致）
- [Minecraft Wiki — End portal frame](https://minecraft.wiki/w/End_portal_frame)：填框音效 `block.end_portal_frame.fill`
- 本仓库既有实现锚点：`cItemEyeOfEnderHandler::FindAndSetPortal`（12 框环判定）、`cProjectileEntity`、
  `cItemThrowableHandler`、`cChunkGeneratorThread::GetBiomeAt`（既有的跨线程只读查询先例）

## 2. 行为规格

- 手持末影之眼按下使用：沿**最近要塞**方向飞出，可穿过方块；飞行约 **2–3 秒**（≈80 tick）。
- **水平距离 > 12 格**时向上飞（便于玩家看清方向）；**≤ 12 格**时向下飞（表示要塞在下方，需要向下挖）。
- 飞行结束后：**20% 碎裂**（`entity.ender_eye.death` + 末影之眼破碎粒子）、**80% 掉落为可拾取物品**；
  两种情况都播放 `entity.ender_eye.death`（wiki 音效表："when an eye of ender drops or breaks"）。
- 投掷音效 `entity.ender_eye.launch`（音量 0.5，音高 1⁄3–1⁄2）。
- **只在主世界有效**；下界/末地/自定义维度不生效（1.12.2 的要塞只生成在主世界）。
- 放在末地传送门框架上：填眼（已有实现），并播放 `block.end_portal_frame.fill`。
- 投射物网络类型：Spawn Object type = **72**（见 1 节来源）。

## 3. 实现

### 3.1 只读的「最近结构」查询

新增一条从世界到结构生成器的只读查询链，供物品使用（tick 线程）调用：

| 层 | 方法 |
|---|---|
| `cChunkGeneratorThread` | `GetNearestStructureTarget(name, pos, target)` → 转发给 `m_Generator` |
| `cChunkGenerator` | 虚函数，默认返回 false（如 `Noise3DGenerator` 无结构） |
| `cComposableGenerator` | 遍历 finisher 链，命中即返回 |
| `cFinishGen` | 虚函数，默认 false |
| `cPieceStructuresGen` | 按 prefab set 名字匹配 `cGen`，再转发 |
| `cGridStructGen` | 按格点噪声算出**最近格**的结构原点（XZ），Y 置 0 |
| `cPieceStructuresGen::cGen` | 用 `cPieceGeneratorBFSTree::GetStartingPieceHeight()` 补上起始件的 Y |

关键点：

- **纯只读**：只用格点参数（`GridSizeX/Z`、`MaxOffsetX/Z`）与格点噪声 `m_Noise` 计算，不创建结构、
  不碰结构缓存 `m_Cache`。`cPrefabPiecePool::Reset()` 是空实现，参数与噪声都在加载期设定，
  因此可以从 tick 线程调用（这也是本仓库既有的 `GetBiomeAt` 跨线程只读先例的同类做法）。
- **候选格窗口**：`(Noise / 7) % (2 * MaxOffset)` 在 C++ 里向零截断，噪声为负时偏移可低至 `-3 * MaxOffset`；
  因此以「最近格点 ≤ GridSize/2，原点偏移 ≤ 3 * MaxOffset」为界，扫描
  `pos ± (GridSize/2 + 2 * 3 * MaxOffset)` 覆盖的格即可保证取到全局最近的**原点**。
  距离按 **XZ 平面**比较（要塞在地表以下，Y 参与比较会引入无关偏置；这是实现选择，非 wiki 明文）。
- 起始件的选择与真正放置时一致：同样的 `Noise(seed)` 与权重（`cPieceGeneratorBFSTree::ChooseStartingPiece`，
  由 `PlaceStartingPiece` 与新查询共用）。

### 3.2 实体

`cThrownEnderEyeEntity`（`pkEnderEye`，size 0.25×0.25）：

- 不受重力、不与方块/实体碰撞（覆写 `OnHitSolidBlock`/`OnHitEntity` 为空，`HandlePhysics` 自算飞行）。
- 速度单位与 `cEntity::SetSpeed`/`cProjectileEntity::HandlePhysics` 一致，为**格/秒**（位移 = 速度 × 秒数）：
  - **飞行阶段**（前 `TRAVEL_TICKS = 48` tick ≈ 2.4 秒，对应 wiki 的"2–3 秒"）：朝目标水平移动
    `HORIZONTAL_SPEED = 5.0` 格/秒 → 约 **12 格**，对应 wiki 的"约 12 格"；Y 方向 `+CLIMB_SPEED = 4.0`
    （水平距离 > `HIKE_DISTANCE = 12` 格）或 `-DIVE_SPEED = 4.0`（≤ 12 格）。
  - **悬停阶段**（`TRAVEL_TICKS` 之后到 `LIFETIME_TICKS = 80`）：速度归零，停在原地
    （wiki："the eye floats in the air briefly"）。
  - 水平方向不会越过目标点。
- `m_TicksAlive >= 80` 时结束：`RandInt(99) < 20` 则碎裂（粒子），否则 `SpawnItemPickups` 掉出末影之眼；
  两者都播放 `entity.ender_eye.death`。
- 不保存到存档（寿命 4 秒；`NBTChunkSerializer` 里显式跳过，Debug 下不会触发
  "Unsaved projectile entity" 断言）。

### 3.3 物品

`cItemEyeOfEnderHandler::OnItemUse`：先尝试填末地传送门框架（原有逻辑 + 填框音效）；否则
**仅主世界** + 查询到 `"Stronghold"` 时才投掷（查不到就不消耗物品），投掷时播放
`entity.ender_eye.launch`，非创造模式消耗一枚。

## 4. 已知偏差

1. **目标是本仓库的网格要塞**（见 `vanilla-1.12.2-stronghold.md` 第 4 节；该规格开头的阶段决定已明确
   **要塞有意不向 vanilla 严格对齐、本阶段不计划改动**），不是 vanilla 的环状 128 要塞；
   眼睛指向的是"结构原点 + 起始件 Y"，而 vanilla 指向起始楼梯所在区块的西北角。因为本仓库的要塞起始件
   就是传送门房，实际效果是**直接指向传送门房**。
2. **飞行剖面是近似**：vanilla 有加速/转向曲线，这里用恒定水平速度 + 固定爬升/俯冲速度；
   常量取值以 wiki 的"约 12 格 / 2–3 秒 / 12 格阈值"为准推导，未逐帧对齐（**推测/待实机微调**）。
   注意速度单位必须是**格/秒**（与 `SetSpeed` 一致）；早期版本把"格/tick"的常量直接乘了秒数，
   导致实际速度只有约 0.0075 格/tick、整个飞行只挪动不到 1 格——已修正。
3. **粒子拖尾未在服务端广播**：wiki 只说"留下紫色粒子拖尾"；vanilla 疑似由客户端随实体渲染
   （**推测/待确认**），故本分支只在碎裂时广播 `PARTICLE_EYE_OF_ENDER`（2003）。
4. **音效名是 1.9+ 的扁平名**（`entity.ender_eye.*`）：与本仓库其它所有音效一致；
   1.8 客户端不认这些名字（既有全局偏差，非本分支引入）。
5. **不更新 yaw/pitch**：眼睛实体的模型朝向不跟随飞行方向（视觉细节，先不做）。
6. **不保存**：服务器重启后飞行中的眼睛消失（寿命仅 4 秒，影响可忽略）。
7. 框架预填眼 10%、战利品表等仍属要塞分支的偏差，本分支不处理。

## 5. 验证

### 5.1 验收门

- `cd src && lua CheckBasicStyle.lua`：0 违规。
- `cmake --build build`：exit 0。
- `cd build && ctest --output-on-failure -E "UrlClient-test|Google-test"`：全绿（新增 `StructureLocatorTest`）。
- 改动未触及导出 API（`World`/`ClientHandle` 等未被导出新方法）→ 无需绑定/APIDump 检查。

### 5.2 自动化

`tests/Generating/StructureLocatorTest.cpp`（工作目录 = 服务器目录）：

1. `cPieceStructuresGen` 以 `Stronghold` 初始化成功；
2. 对若干查询点（原点、若干要塞附近点、远处点），`GetNearestStructureTarget("Stronghold", pos, target)`
   返回 true，且 **(x, z) 等于测试内用同一格点公式暴力枚举较大窗口得到的最小 XZ 距离原点**
   （验证候选窗口与选择逻辑，不会漏掉更近的格）；
3. 目标 Y 落在起始件的 `Range|20|40` 区间；
4. 未知名字（如 `"NetherFort"`）返回 false；
5. 把返回的目标点再作为查询点，得到同一目标（最近邻的不动点性质）。

### 5.3 实机验证（维护者）

- **投掷行为已实机确认正确**（维护者）：横向飞行距离与速度、悬停，均符合预期。
  早期版本因速度单位错误（格/tick 的常量乘了秒数）只挪动不到 1 格，已修正（见 §4.2 / 提交记录）。
- 仍需维护者按需确认的项：下界/末地右键不投掷（只可能填框）、碎裂与掉落的实际比例接近 1:4、
  以及指向的要塞与 `vanilla-1.12.2-stronghold.md` 5.4 表一致。

## 6. 变更清单

| 文件 | 说明 |
|---|---|
| `src/Generating/ComposableGenerator.h/.cpp` | `cFinishGen` 与 `cComposableGenerator` 的查询接口与实现 |
| `src/Generating/ChunkGenerator.h/.cpp` | 生成器基类的查询接口（默认 false） |
| `src/ChunkGeneratorThread.h/.cpp` | 世界的查询入口 |
| `src/Generating/GridStructGen.h/.cpp` | 最近格原点计算（只读） |
| `src/Generating/PieceGeneratorBFSTree.h/.cpp` | 起始件选择的只读复用（`ChooseStartingPiece` / `GetStartingPieceHeight`） |
| `src/Generating/PieceStructuresGen.h/.cpp` | 按名字匹配 prefab set + 补 Y |
| `src/Generating/EndCityLoot.h` | 补 `#include "../Defines.h"`（unity 批次变化暴露的既有缺 include） |
| `src/Entities/ThrownEnderEyeEntity.h/.cpp`、`src/Entities/CMakeLists.txt` | 新实体 |
| `src/Entities/ProjectileEntity.h/.cpp` | `pkEnderEye`、创建分支、MCA 类名 `EyeOfEnderSignal` |
| `src/Protocol/Protocol_1_8.cpp` | Spawn Object type 72 |
| `src/WorldStorage/NBTChunkSerializer.cpp` | 显式跳过保存 |
| `src/Items/ItemEyeOfEnder.h` | 投掷逻辑、填框音效 |
| `tests/Generating/StructureLocatorTest.cpp`、`tests/Generating/CMakeLists.txt` | 新增测试 |
| `specs/vanilla-1.12.2-eye-of-ender.md` | 本文件 |

## 7. 未决

1. 粒子拖尾（3 节偏差 3）需要实机确认 vanilla 是否由客户端渲染；若需要服务端广播，另开分支。
2. 若将来有**可变状态**的 piece pool（`Reset()` 非空实现）或运行期重载生成器配置，
   则只读查询需改为走生成器线程的请求队列；当前 `cPrefabPiecePool::Reset()` 是空实现，故直接调用安全。
3. 眼睛的飞行剖面（偏差 2）可按实机观察微调；本分支不引入 vanilla 的加速曲线。
