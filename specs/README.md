# specs/ — 任务级行为规格

本目录下的每个 `.md` 是**一次下达任务**的交付物：vanilla 1.12.2 的行为规格 + 来源 + 验证方式
（[AGENTS.md §2.4](../AGENTS.md)）。规格**按子系统分子目录**；平铺在 `specs/` 根下是错的。

## 为什么分目录

- 一次任务 = 一个子系统（[AGENTS.md §3](../AGENTS.md) 的 `feature/<subsystem>-*`）。
  规格跟着子系统走，引用点（测试头注释、`.cubeset` 注释、其他规格）就能一眼看出它属于哪条线。
- 平铺时 `specs/` 下无法按前缀区分「命令层」「状态效果」「末地结构」三类内容，
  补全与检索都退化成记文件名。
- 目录是**引用面的一部分**：移动规格 = 全仓改引用（本次 42 个文件、约 140 个引用点）。
  因此只在**整批整理**时移动，新规格直接落进对应子目录，不逐条挪。

## 子目录

| 子目录 | 覆盖的子系统 | 典型内容 |
|---|---|---|
| `commands/` | 命令层：解析与 origin、目标选择器、命令方块、`/execute`、表现类命令、记分板队伍 | C1–C6 系列规格 + 命令层 roadmap |
| `status-effects/` | 状态效果：施加与合并、免疫、数值、快照同步、持久化；区域效果云、怪物近战上效果 | S1–S18 系列规格 + 状态效果 roadmap |
| `worldgen/` | 世界生成与结构：要塞、末地城、末地到达平台、末地折跃门、结构箱子战利品、`.cubeset` 方块实体 | 结构蓝图与生成规则、连接器旋转不变量 |
| `mobs/` | 生物行为：末影人、末影龙、自然生成 | 生成表、AI 阈值 |
| `blocks/` | 单方块行为：紫颂植物 | 生长与支撑规则 |
| `lighting/` | 光照：三维度光值表与天光曲线 | 光值表 |
| `items/` | 物品行为：末影之眼 | 投掷与指向 |

归属判不准时的顺序：① 这条规格主要约束哪个 `src/` 子系统？
② 历史分支前缀是哪个（`feature/effects-*` → `status-effects/`）？
③ 都不贴 → **问维护者**，不要为一条规格新开目录（`blocks/`、`items/`、`lighting/`
是历史形成的单文件目录，不作为先例）。

## 命名

- vanilla 行为规格：`vanilla-<版本>-<主题>.md`，如 `vanilla-1.12.2-command-blocks.md`。
  **版本前缀保留**——引用点靠它标明规格针对的版本。
- 一个任务族共用主题前缀（如 `status-effect-*`）；族级编排文件用 `<主题>-roadmap.md`。
  roadmap 是**任务级**交付物，它列出的条目**不自动构成新任务**（[AGENTS.md §1](../AGENTS.md)）。
- 唯一的例外是 `worldgen/piece-connector-rotation.md`：它是**仓库内部一致性规格**
  （不是 vanilla 行为），因此不带版本前缀。

## 新增一条规格

1. 按上表定子目录；
2. 落盘到 `specs/<子目录>/<文件名>`；
3. 文件内部的相对链接：同子目录写文件名本身，跨子目录写 `../<子目录>/<文件名>`，
   指向仓库其他文件写 `../../<路径>`；
4. 引用点（[tests/](../tests/)、[Server/Prefabs/](../Server/Prefabs/)、其他规格、[AGENTS.md](../AGENTS.md)）
   写**全路径** `specs/<子目录>/<文件名>`；
5. 跑下方校验，断链数必须为 0。

## 校验

在仓库根运行；它检查两件事：① `specs/**/*.md` 里的相对链接能解析；
② 全仓（`git ls-files`）出现的每个 `specs/<路径>` 都真实存在。
标注〔待建〕的行（尚未创建的前向引用）按约定跳过。

```sh
python3 - <<'PY'
import re, os, glob, subprocess, urllib.parse
bad = []
for f in glob.glob('specs/**/*.md', recursive=True):                    # ① 规格内部相对链接
    for i, line in enumerate(open(f, encoding='utf-8'), 1):
        if '〔待建〕' in line:
            continue
        for m in re.finditer(r'\]\(([^)\s]+)', line):
            t = m.group(1)
            if t.startswith(('http://', 'https://', 'mailto:', '#')):
                continue
            p = urllib.parse.unquote(t.split('#')[0])
            if p and not os.path.exists(os.path.normpath(os.path.join(os.path.dirname(f), p))):
                bad.append(f'{f}:{i}: 断链链接 -> {t}')
for f in subprocess.run(['git', 'ls-files'], capture_output=True, text=True,
                        check=True).stdout.split():                     # ② 全仓 specs/ 引用
    if f.startswith(('lib/', 'build/')):
        continue
    try:
        lines = open(f, encoding='utf-8').read().splitlines()
    except (OSError, UnicodeDecodeError):
        continue
    for i, line in enumerate(lines, 1):
        if '〔待建〕' in line:
            continue
        for m in re.finditer(r'specs/([A-Za-z0-9][A-Za-z0-9._/-]*)', line):
            if not os.path.exists('specs/' + m.group(1).rstrip('.,;:）)')):
                bad.append(f'{f}:{i}: 指向不存在的 specs/{m.group(1)}')
print(f'断链数: {len(bad)}')
print('\n'.join(bad))
PY
```
