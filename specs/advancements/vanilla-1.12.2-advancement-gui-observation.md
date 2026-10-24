# vanilla 1.12.2 advancement screen — observation record (read-only)

Task: capture the vanilla advancement GUI (all five tabs, every advancement granted) so the
parent can verify tab set + node layout of its own implementation. No repository file was read,
modified or created; no client/server was installed, launched or managed; no input-class tool
was used (control mode is a known crash on this instance).

## 1. Tool calls used (provenance)

Read-only only:
- mcp__mcmm__ping -> {"result":"pong"}
- mcp__mcmm__get_minecraft_status -> {"connected":true,"port":9876,"processAlive":false}
- mcp__mcmm__get_player_info -> {"name":"rrtt217","health":20,"pos":"15.6 67.0 -37.1","rotation":"-2.6 32.7","dimension":"overworld","gamemode":"survival","food":16}
- mcp__mcmm__get_world_info -> {"world_name":"MpServer","difficulty":"normal","gametype":"survival","time":0,"weather":"clear"}
- mcp__mcmm__get_screen_buttons -> {"error":"no screen"}   (also "no screen" while the advancement GUI was demonstrably open, see 4)
- mcp__mcmm__enumerate_widgets -> polled ~250x; the only relevant value is
  {"screen":"GuiScreenAdvancements","widgets":[],"total":0}
- mcp__mcmm__screenshot_to_file(path=<abs>) -> {"file":"<abs>","size":<bytes>}   x43
- read_image(<abs png>) -> image bytes + 854x480 for every frame
- bash: ls / mkdir / ImageMagick availability probes only (no repo path written)

End-of-observation state sweep (same tools):
- get_player_info -> {"name":"rrtt217","health":20,"pos":"240.1 64.0 -118.8","rotation":"-179.3 -4.1","dimension":"overworld","gamemode":"survival","food":20}
- get_world_info -> {"world_name":"null","difficulty":"normal","gametype":"survival","time":0,"weather":"clear"}
- enumerate_widgets -> {"screen":"GuiScreenAdvancements","widgets":[],"total":0}

## 2. Client state (only what a tool returned)

- Bridge: 127.0.0.1:9876, `get_minecraft_status` = connected:true, processAlive:false
  (the mod answers while reporting processAlive:false - treat that field as unreliable).
- No read-only tool exposes the game version, the loader, the GUI scale or the window size.
  `get_minecraft_status` returned only {connected, port, processAlive}; `get_world_info` has no
  version/scaling field. => "1.12.2 + Cleanroom" is the environment doc's claim, NOT tool-verified
  here: see 11.
- Framebuffer of every captured frame: 854x480 px (read_image width/height on each png).
  That is the screenshot size; it is NOT necessarily the OS window size, and the GUI scale
  factor is not exposed => 〔待核〕.
- Locale: the on-screen strings are English ("Advancements", "Sweet dreams",
  "Change your respawn point", "Upgrade your pickaxe", "Breed", "New Recipes Unlocked!").
  Note this differs from the zh_CN Forge instance recorded in AGENTS.local.md.
- Player is in the overworld; `gamemode` reports "survival" but AGENTS.local.md documents that
  field as unreliable, so the actual game mode is 〔待核〕.
- `get_world_info.world_name` reads "MpServer" early and "null" late (AGENTS.local.md documents
  this field as not identifying the world) - so the two capture sessions cannot be tied to a
  world name by tool output.

## 3. Grant evidence — the count is 455

Frame adv-tab-03.png is the key frame: the chat block in the lower-left is legible and contains

    Granted every advancement (455 total granted) to rrtt217

That is the only place the total count is visible. Frames adv-tab-12.png / adv-tab-13.png show
further chat lines in the same block, partially clipped by the window frame; the legible parts
include "[INFO] ..." and "Granted all advancements to rrtt217" (wording 〔待核〕: the left edge of
those lines is cut off, so the exact command-echo wording must not be quoted as authoritative).

Player name corroborates: get_player_info.name = "rrtt217".

## 4. What the bridge can and cannot tell about this GUI

`enumerate_widgets` returns {"screen":"GuiScreenAdvancements","widgets":[],"total":0}: the
advancement screen exposes NO widget tree, i.e. tab identity, node positions and labels are not
available as data - they can only be read off the pixels. `get_screen_buttons` answers
"no screen" both when no screen is open and while this GUI is open, so it is not a usable probe
for this GUI; `enumerate_widgets` is.

Tab names are rendered only as hover labels: in adv-tab-40.png the label "Husbandry" is drawn
above the tab strip while the displayed tree is a different tab (see 5), so a visible label
identifies the tab under the cursor, not necessarily the open tab.

## 5. The five tabs

Every frame shows exactly five tabs in one horizontal strip above the window, plus the window
title "Advancements". Each tab has its own tree background texture and its own node tree.
Mapping below is what the frames support; the tab strip ordinal (left-to-right) is deliberately
NOT asserted - measure it from the pngs.

| tab | tree background | direct evidence | example frames |
|---|---|---|---|
| Minecraft | pale, smooth parchment/sandstone-like | label "Minecraft" above the strip in adv-tab-26.png while this tree is displayed; content is craft/trade/sleep themed (sheet of paper, sword, pickaxe, villager, emerald, chest, bed) | 03, 14, 25, 26, 28, 29, 30 |
| Nether | red-brown netherrack-like mottle | label "Nether" above the strip in adv-tab-21.png and adv-tab-33.png while this tree is displayed | 04, 18, 21, 22, 32, 33, 42 |
| The End | pale yellow mottled end-stone-like | label "The End" in adv-tab-05.png while this tree is displayed (adv-tab-04.png also shows the label, there over a different tab); content is pearl/obsidian/purple themed | 05, 06, 15, 16, 23, 24, 31 |
| Husbandry | brown planks/wood-like | content is farming/breeding themed and a node tooltip reads "Breed" (adv-tab-17.png); the strip label "Husbandry" exists in adv-tab-40.png | 07, 17, 19, 20, 34, 41 |
| Adventure | gray stone-like | by elimination (the other four are identified) + content is bow/pickaxe/ingot/armour themed and a node tooltip reads "Isn't It Iron Pick / Upgrade your pickaxe" (adv-tab-11.png) | 08, 09, 10, 11, 12, 13, 35, 36, 37, 38, 39, 40 |

Confidence: the first three are label-verified; "Husbandry" is content+label supported;
"Adventure" for the gray tree is elimination + content => 〔推测, high confidence〕.

Recipe note (parent item 5): the screen has exactly five tabs and none of them is a recipe tab,
and no tree in any frame contains a recipe-style node. Recipe unlocks appear instead as their own
toast, "New Recipes Unlocked! / Check your recipe book" (legible in adv-tab-03.png, partially in
adv-tab-02.png), which is a different toast from the three advancement toasts. That is consistent
with recipe entries being display-less (no tab, no node), but the screen alone cannot prove that
every recipe entry is absent from the trees, because the trees are viewport-clipped (see 9) =>
〔推测: display-less recipes are consistent with every frame; not provable from the GUI〕.

## 6. Per-screenshot index

All paths absolute; 854x480 px each. "session A" = fresh toast backlog from the grant;
"session B" = after the player left and re-entered the world, toasts cleared, tree scrolled to
a different position.

- adv-probe-1.png — NOT the GUI. Test write of the path; shows plain world view. Ignore.
  01 — NOT the GUI: shows the world; enumerate_widgets reported GuiScreenAdvancements immediately
       before the shot, so the capture raced the screen closing. Not usable.
  02 — Unusable frame: strongly dithered/dark, a fragment of a "New Recipes Unlocked!" toast at
       top-left, tree unreadable (render/transition artifact).
  03 — Minecraft tree (session A). 5 toasts. Chat shows the 455 grant line. KEY FRAME for §3.
  04 — Nether tree (session A). Strip label "The End". 5 toasts: Suit Up / Return to Sender /
       Sweet dreams / Hot Stuff / You Need a Mint.
  05 — The End tree (session A). Strip label "The End". Toasts: Free the End / Monsters Hunted /
       Enchanter / A Furious Cocktail / Eye Spy.
  06 — The End tree (session A). Strip label "Nether".
  07 — Husbandry tree (session A). Toasts: Best Friends Forever / Bring Home the Beacon /
       Not Today, Thank You / Isn't It Iron Pick / Postmortal.
  08 — Adventure tree (session A). Toasts: Monster Hunter / Adventuring Time / Local Brewery /
       A Seedy Place / The Next Generation.
  09 — Adventure tree (session A), same toasts (one tooltip frame edge visible at bottom).
  10 — Adventure tree (session A). Toasts: Sniper duel / Great View From Up Here / Beaconator /
       Withering Heights / A Balanced Diet.
  11 — Adventure tree (session A). Node tooltip "Isn't It Iron Pick / Upgrade your pickaxe".
       ANOMALY: no tab strip rendered in this frame (window + title only).
  12 — Adventure tree (session A). Toasts + partially clipped chat lines.
  13 — Adventure tree (session A). Toasts incl. What a Deal!, You Need a Mint.
  14 — Minecraft tree (session A). Toasts.
  15 — The End tree (session A). Toasts: The Next Generation / Sky's the Limit / Remote Getaway /
       Free the End.
  16 — The End tree (session A). Adds toast "The City at the End of the Game".
  17 — Husbandry tree (session A). Node tooltip "Breed". Toasts incl. Great View From Up Here /
       The End... Again... / A Balanced Diet / Serious Dedication / Two by Two.
  18 — Nether tree (session A), same toast set as 17.
  19 — Husbandry tree (session A). Toasts: A Seedy Place / Best Friends Forever /
       How Did We Get Here? / A Furious Cocktail (+1 scrolled off).
  20 — Husbandry tree (session A). Adds toast "The Parrots and the Bats".
  21 — Nether tree (session A). Strip label "Nether". Toasts: Local Brewery /
       Bring Home the Beacon / Beaconator / Subspace Bubble / A Terrible Fortress.
  22 — Nether tree (session A), same toasts, no label.
  23 — The End tree (session A). Toasts: Spooky Scary Skeleton / Into Fire / Return to Sender /
       Withering Heights / Uneasy Alliance.
  24 — The End tree (session A), identical to 23.
  25 — Minecraft tree (session A). Toasts: Zombie Doctor / Not Today, Thank You / Enchanter /
       The End? / We Need to Go Deeper.
  26 — Minecraft tree (session A). Strip label "Minecraft". Same toasts as 25.
  27 — NOT the GUI: world view after re-entering the world (session B start, player elsewhere).
  28 — Minecraft tree (session B, clean). Node tooltip "Sweet dreams / Change your respawn point".
  29 — Minecraft tree (session B, clean, no tooltip).
  30 — Minecraft tree (session B), slightly different scroll (bow + rod visible at the top row).
  31 — The End tree (session B, clean).
  32 — Nether tree (session B, clean).
  33 — Nether tree (session B). Strip label "Nether".
  34 — Husbandry tree (session B, clean).
  35 — Adventure tree (session B, clean); node column clipped at the frame's right edge.
  36 — Adventure tree (session B), identical to 35.
  37 — Adventure tree (session B); right-edge clipping clearly visible (node fragment outside frame).
  38 — Adventure tree (session B), same as 37.
  39 — Adventure tree (session B), scrolled one column further right than 37/38.
  40 — Adventure tree (session B), same scroll as 39. Strip label "Husbandry" (hover on a tab that
       is NOT the displayed tree - see §4).
  41 — Husbandry tree (session B, clean).
  42 — Nether tree (session B), widest view of this tree (2 extra columns vs 32).

## 7. Nodes/icons visible per tab (1x reading of the pngs; icon identity is 〔推测〕 where noted)

No numeric coordinates are given on purpose - measure them from the pngs. Every visible node in
every frame is drawn with the same gold/amber decorative frame; no plainly-framed node was seen,
which is consistent with every advancement being granted.

- Minecraft: a pale sheet-of-paper node on the left; a sword node; a diamond-blue pickaxe node;
  a bow node and a fishing-rod node above (clearer in adv-tab-30.png); a villager node;
  an emerald node and a chest node; a bed node (tooltip "Sweet dreams"); two further nodes at the
  bottom (paper-like, and a cyan/pink item). Node labels are not drawn in the tree - only hover
  tooltips are.
- Nether: a pale/white blob node at the top; a golden rod-like node, a pink potion node, a bucket
  node and a grey node in a row; a dark-red block node and a black cube node on the left; a dark
  green/black node, a white sparkle node and two teal/cyan nodes; a dark round node and a pale
  node at the bottom (widest view: adv-tab-42.png).
- The End: a pale end-stone block node on the left; a dark round (eye-like) node under it; a black
  cube node; a teal/green pearl node; a pink sparkle node; a pink potion node; a purple/magenta
  cube node and two purple/grey nodes to the right (widest: adv-tab-31.png).
- Husbandry: a golden hay-like block node on the left; wheat/sprig nodes; a red apple node; a
  bright yellow/gold item node; seed-like nodes; one node carries the tooltip "Breed".
- Adventure: a green-topped dirt/grass-like block node on the left; a bow node; two pickaxe nodes;
  a silver ingot node; a white shirt/armour node and a brown stick/plank node; a bucket node; a
  black cube node; a shears-like node; a gold apple-like node; a teal/green gem node; a
  blue armour node and a magenta/purple node (widest: adv-tab-39.png / adv-tab-40.png).

## 8. Toast stack (useful for frame-type parity)

- Three advancement toast headers were observed, matching three frame strengths:
  "Advancement Made!", "Goal Reached!", "Challenge Complete!".
  "Challenge Complete!" is rendered in a magenta/pink title colour; the other two in yellow tones
  (exact hues: measure from the pngs).
- A fourth, non-advancement toast: "New Recipes Unlocked! / Check your recipe book".
- The stack is drawn right-aligned, newest at the top in these frames, and never more than FIVE
  toasts are visible at once - every frame with a backlog shows exactly 5, both when the backlog
  was larger (session A) and later.
- Advancement toasts show a small item icon + header + the advancement title.
- Frames adv-tab-28.png ... adv-tab-42.png still have all five tabs and all trees, but no toasts.

## 9. Viewport / clipping (parent item 4)

The tree is drawn inside a windowed viewport and is scrollable; several tabs do not fit at this
framebuffer size:
- Adventure: adv-tab-35/36/37/38 clip nodes at the frame's right edge; adv-tab-37.png even shows a
  node fragment protruding outside the frame; adv-tab-39/40 are scrolled one column further right.
- Husbandry: adv-tab-19/20 clip the right part of the tree.
- Minecraft: adv-tab-03/25/26 clip at the right edge; adv-tab-30 is scrolled slightly versus 29.
- Nether: adv-tab-42 shows two more columns than adv-tab-32.
=> A single frame per tab does NOT show the whole tab. Scrolling was NOT possible for me:
scroll/mouse tools are input-class and control mode is documented to crash this Cleanroom
instance, so I never entered it. If the parent needs the off-screen nodes, the human must scroll
each tab while a capture loop runs.

## 10. Window chrome (what the frames show, no coordinates)

Window title "Advancements" at the top-left inside the frame; the tab strip sits above the window
frame and the tab whose tree is displayed is drawn merged into the frame (it sits lower and its
bottom edge joins the frame's top border); nodes are connected by thin lines; node tooltips appear
as a two-line dark popup (title + description, e.g. "Sweet dreams" / "Change your respawn point").
A mod HUD overlay is burned into the lower-left corner of every frame ("MCP Fort at 9876") - it is
NOT part of the vanilla GUI; do not measure it.

## 11. Unverified / 〔待核〕 / 〔推测〕

〔待核〕 Client version and loader: no read-only tool exposes them (get_minecraft_status returned
        only connected/port/processAlive). The "1.12.2 + Cleanroom" claim is from AGENTS.local.md,
        not from this observation. How to verify: a tool that reports the version, or the
        launcher's instance metadata.
〔待核〕 GUI scale and OS window size: not exposed by any read-only tool. The png framebuffer is
        854x480 (read_image on each file). Derive the scale from the pngs if needed.
〔待核〕 Player game mode: get_player_info.gamemode says "survival", a field AGENTS.local.md
        documents as wrong for creative; not independently verified here.
〔待核〕 The exact wording of the "[INFO] ..." chat lines in adv-tab-12/13.png (left edge clipped).
〔待核〕 Whether the advancement state is identical in session B (frames 27-42). The player left
        and re-entered the world between session A and B (position changed from
        "15.6 67.0 -37.1" to "240.1 64.0 -118.8", toasts cleared down to zero, trees scrolled
        to a different position). All nodes still render in the same granted style, but no
        read-only tool exposes advancement state, so "still 455 granted" is NOT tool-verified.
〔待核〕 Exact item identity for several node icons (listed with "like"/"such as" in §7).
〔推测〕 Tab ordering left-to-right: not asserted. The parent should measure the five tab icons.
〔推测〕 "Adventure" as the name of the gray-stone tree: by elimination + content.
〔推测〕 Recipes are display-less: consistent with the screen (5 tabs, no recipe nodes, separate
        "New Recipes Unlocked!" toast), but the trees are viewport-clipped so absence cannot be
        proven from these pngs.
〔推测〕 adv-tab-11.png renders no tab strip (real state or capture artifact) - unexplained.

## 12. Deviations from the brief

- The spec could not be written to /home/david/adv-oracle/full-tree.md: my sandbox is
  workspace-write over the repository only, so outside paths are read-only for me
  (write tool: "[sandbox: file access denied under workspace-write mode]"; bash mkdir: "read-only
  filesystem"), and approvals are disabled for this session so I cannot escalate. Only the
  Minecraft client process can write into /home/david/adv-oracle/ (that is how the pngs got there).
  The spec is therefore at /tmp/adv-oracle/full-tree.md (parent approved this fallback) and a copy
  at the fs-backend's /tmp; if neither path is readable for the parent, the content of this file is
  also summarised in the final report message.
- adv-probe-1.png is a leftover path-writability probe, not evidence.
- No crops/upscales were produced: the only writable scratch area for my shell is a private /tmp
  that the image reader cannot see, and writing scratch files into the repository was forbidden,
  so all reading was done on the original 1x frames.
