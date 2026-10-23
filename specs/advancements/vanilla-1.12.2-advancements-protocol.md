# 1.12.2 进度协议（M0）spec

> **状态**：实现中（M0 协议降险分支 feature/advancements-protocol）。
> **基线**：master 317733a；本分支从 master 切出，不改动其他子系统。
> **上游缺口**：[issue #3770](https://github.com/cuberite/cuberite/issues/3770)（进度系统）；规划见 feature/advancements-roadmap 分支的 roadmap（未合并，本文件不依赖它）。

## 1. 本阶段范围

M0 只做「让 1.12–1.12.2 客户端能收到并画出服务端声明的进度树」所需的最小协议层：

1. 新增 clientbound 包类型 pktAdvancements（本阶段不发送 pktSelectAdvancementTab，只登记类型与 ID）；
2. 1.12 / 1.12.1 / 1.12.2 的包 ID 映射，1.13 / 1.14 只登记 ID 不下发；
3. cProtocol_1_12::SendAdvancements 的序列化；
4. 入服时（SendStatistics 之后）下发一棵硬编码的两节点测试树；
5. 删除页签热修里那句 INFO 文案；
6. 1.8–1.11 的 SendAdvancements 是 no-op（这些客户端没有进度系统）。

不在本阶段：JSON 数据加载、进度持久化、条件/触发器、/advancement 命令、聊天公告、成就桥。

## 2. 协议事实与来源

### 2.1 包 ID

| 客户端 | Advancements | Select Advancement Tab | 来源 |
|---|---|---|---|
| 1.12（335） | 0x4C | 0x36 | [1.12 协议页，oldid 2772313](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772313) |
| 1.12.1（338） | 0x4D | 0x37 | [1.12.1 协议页，oldid 2772340](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772340) |
| 1.12.2（340） | 0x4D | 0x37 | [1.12.2 协议页，oldid 2772350](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772350) |
| 1.13 / 1.13.1（393 / 401） | 0x4D | 0x37 | [1.13 协议页，oldid 2772385](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772385) |
| 1.14.x（477–498） | 0x51 | 0x3A | [1.14 协议页，oldid 2772458](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772458) |

继承关系（[Protocol_1_12.h](../../src/Protocol/Protocol_1_12.h)、[Protocol_1_13.h](../../src/Protocol/Protocol_1_13.h)、[Protocol_1_14.h](../../src/Protocol/Protocol_1_14.h)）：1.12.2 继承 1.12.1，1.13 继承 1.12.2，1.14 继承 1.13.2。1.12.1 表里的 0x4D/0x37 会自动被 1.12.2 与 1.13 继承；1.14 的 GetPacketID 必须显式覆盖，否则会回退到 0x4D。

### 2.2 包结构（1.12.2）

来源：[1.12.2 协议页](https://minecraft.wiki/w/Java_Edition_protocol/Packets?oldid=2772350) 的 Advancements 段，字段顺序：

| 段 | 字段 | 类型 | 备注 |
|---|---|---|---|
| 1 | Reset/Clear | Bool | 先清空客户端已有的进度 |
| 2 | Mapping size | VarInt | |
| 2 | 每条 | Identifier；Bool has parent → Identifier；Bool has display → Display；VarInt 条件数 → String[]；VarInt 需求组数 → String[][] | |
| 2 | Display | Chat title；Chat description；Slot icon；VarInt frame；Int flags；[Identifier background]；Float x；Float y | frame 线上值 0=task、1=challenge、2=goal |
| 3 | Removals | VarInt + Identifier[] | |
| 4 | Progress | VarInt + [Identifier + Progress] | |
| 4 | Progress | VarInt + [String criterion, Bool achieved, (Long date)] | date 仅 achieved 为真时出现，毫秒（Date.getTime()） |

flags：0x1 有 background、0x2 show_toast、0x4 hidden。announce_to_chat 不在包里（公告由服务端发）。

### 2.3 实测证据（ProtoProxy）

本机经 ProtoProxy（远端端口 25567）连到一台 1.12.2 原版服务器，2026-10-16 的一次会话抓到了入服时的 Advancements 包：

- 日志：build/bin/Logs/Log_1792127802_5.log:34461
  `[0.954] S->C Advancements (0x4d, 1374 bytes, compressed)`
- 将 `Advancements = 1374 bytes` 的十六进制载荷逐字节解码，结果完全吃掉 1374 字节：
  - Reset = 1；
  - Mapping = 7：minecraft:recipes/root（无 parent、无 display、criterion impossible）与 6 个 minecraft:recipes/transportation/*_boat（parent = minecraft:recipes/root、无 display、criteria = entered_water + has_the_recipe、requirements = [[entered_water, has_the_recipe]]）；
  - Removals = 0；
  - Progress = 7：每个船条目包含 entered_water=1 + date（毫秒，例如 1791966590000）与 has_the_recipe=0；root 的 impossible=0。
- 结论：wiki 的字段顺序、frame/flags 前的元素顺序、date 的「仅 achieved 时出现、单位毫秒」都在实测里成立；本分支的序列化按同一顺序实现。
- 未覆盖：这次会话没有打开进度界面（没有 serverbound 0x19），所以只看到原版入服时下发的 recipe 进度，没有看到五棵主树的全量包；「原版是否/何时下发主树」仍是〔待核〕（后续用同一 Proxy 让客户端按 L 抓包）。

## 3. 实现映射

- [src/Advancements/Advancement.h](../../src/Advancements/Advancement.h)：cAdvancement / cAdvancementDisplay / cAdvancementProgress / cAdvancementUpdate。
- [src/Advancements/AdvancementTestData.cpp](../../src/Advancements/AdvancementTestData.cpp)：临时的两节点测试树（cuberite:m0/root、cuberite:m0/child）。
- [src/Protocol/Protocol_1_8.cpp](../../src/Protocol/Protocol_1_8.cpp)：SendAdvancements 的 no-op（覆盖 1.8–1.11.2）。
- [src/Protocol/Protocol_1_12.cpp](../../src/Protocol/Protocol_1_12.cpp)：1.12 / 1.12.1 的 ID 映射、HandlePacketAdvancementTab 去掉 INFO、SendAdvancements 序列化。
- [src/Protocol/Protocol_1_14.cpp](../../src/Protocol/Protocol_1_14.cpp)：1.14 的 0x51 / 0x3A。
- [src/ClientHandle.cpp](../../src/ClientHandle.cpp)：SendAdvancements 转发；FinishAuthenticate 在 SendStatistics 之后、仅对 335 ≤ 协议 < 393 下发测试树。

## 4. 入服同步与测试树

- 测试树 reset=true；root 有 display（title/description 是字面 JSON 文本、icon 钻石、frame task、show_toast 默认 true、无 background）；child 有 display、parent=root、icon 木棍。
- root 的 criterion 在入服时标记为已达成（带当前毫秒时间戳），child 不达成：root 用来让客户端建立页签，child 用来确认未达成节点的渲染。
- background 故意留空：客户端对无 background 的根显示缺失纹理（[Advancement definition](https://minecraft.wiki/w/Advancement_definition)），本阶段先验证「页签能出现」，准确背景字符串留到拿到原版主树抓包后再定。

## 5. 验证

| 项 | 结果 |
|---|---|
| 风格 | cd src && lua CheckBasicStyle.lua → 0 violations |
| 构建 | cmake --build build -j 8 → 通过 |
| ctest | ctest -E 'UrlClient-test|Google-test' → 49/49 通过 |
| 协议 sanity | mineflayer 1.12.2 bot 连 25565，收到包后保持在线、可查询坐标/血量 |
| 客户端界面 | 已验：1.12.2 客户端进服后按 L，屏幕为 GuiScreenAdvancements；root（钻石图标、完成边框）与 child（木棍图标、普通边框）两个节点都正常渲染，父链连线可见，背景是缺失纹理（本阶段故意不写 background），客户端未断开。截图：build/tmp/adv-screen-1.png（root 悬停 tooltip："M0 Root" / "Protocol smoke test"）、build/tmp/adv-screen-3.png（两个节点同屏，root 完成帧 / child 未完成帧） |
| 授予时 toast | 未单独验证：root 在入服包里就已达成，首次同步是否弹 toast 没有截到；留给 M1 用 /advancement grant 验证 |

## 6. 已知偏差与后续

- 数据是硬编码的两节点树；M1 用 JSON 加载的 cAdvancementManager 替换。
- 只对 1.12–1.12.2 下发；1.13/1.14 只登记 ID、不下发（AGENTS §9 冻结）。
- progress 只写已达成条件（原版会同时写 achieved=false 的条件）；1.12.2 客户端对缺失的 criterion progress 是否按未达成处理〔待核〕，M1 改成写全条件。
- 不发送 Select Advancement Tab；不处理页签包（只丢弃）。
- 无公告、无命令、无持久化：M1/M2 处理。
