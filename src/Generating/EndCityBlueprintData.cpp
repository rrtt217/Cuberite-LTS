// EndCityBlueprintData.cpp

// Implements the raw End City blueprint data.

/*
The block layout of each piece comes from the Minecraft Wiki's layered blueprints
(https://minecraft.wiki/w/End_City/Structure). The "Base Room" is kept as the single combined
blueprint the wiki publishes, because its spiral ladder and three floor openings are only consistent
as a whole. TowerFloor is derived by solidifying the lowest layer of TowerPiece, following the piece
names and roles listed at https://minecraft.wiki/w/End_City#Structure_details (tower_floor: "similar
to tower_base, but the ladder entrance is replaced with a solid floor"). The bridge-end loot rooms
use the wiki's one, two and three storey room blueprints. The wiki does not publish block layouts for
the individual named pieces, so these are the closest allowed-source geometry.
*/

#include "Globals.h"
#include "EndCityBlueprintData.h"





static const sEndCityBlueprintLayer Layers_SecondFloor1[] =
{
	{0, "B                 |                  |                  |   D          D   |    UEEEEEEEEU    |    E     LPLE    |    E        E    |    E        E    |    E        E    |    E        E    |    E        E    |    E        E    |    E        E    |    UEEEEEEEEU    |   D          D   |                  |                  |                  "},
	{1, "B                 |                  |                  |                  |    UEEEEEEGEU    |    E   LPL  E    |    G        G    |    E        E    |    E        E    |    E        E    |    E        E    |    G        G    |    E        E    |    UEGEEEEGEU    |                  |                  |                  |                  "},
	{2, "B                 |                  |                  |                  |    UEEEEEEGEU    |    E LPL    E    |    G        G    |    E        E    |    E        E    |    E        E    |    E        E    |    G        G    |    E        E    |    UEGEEEEGEU    |                  |                  |                  |                  "},
};





static const sEndCityBlueprintLayer Layers_SecondRoof[] =
{
	{0, "B                 |   YNNYYNNYYNNY   | YSSSSSSSSSSSSSSY | YSPPPPPPPPPPPPSY | NSPPPPPPPPPPPPSN | NSPPPL      PPSN | YSPPPPPPPPPPPPSY | YSPPPPPPPPPPPPSY | NSPPPPPPPPPPPPSN | NSPPPPPPPPPPPPSN | YSPPPPPPPPPPPPSY | YSPPPPPPPPPPPPSY | NSPPPPPPPPPPPPSN | NSPPPPPPPPPPPPSN | YSPPPPPPPPPPPPSY | YSSSSSSSSSSSSSSY |   YNNYYNNYYNNY   |                  "},
};





static const sEndCityBlueprintLayer Layers_ThirdFloor1[] =
{
	{0, "B                 |                  |  D            D  |   UEEEEEEEEEEU   |   E          E   |   E          E   |   E          E   |   E    SS    E   |   E    SSU   E   |   E    UU    E   |   E    SS    E   |   E    SS    E   |   E    SS    E   |   E    SS    E   |   UEEEEEEEEEEU   |  D            D  |                  |                  "},
	{1, "B                 |                  |                  |   UEGEGEEGEGEU   |   E          E   |   G          G   |   E          E   |   G          G   |   E      U   E   |   E    UU    E   |   G          G   |   E          E   |   G          G   |   E          E   |   UEGEGEEGEGEU   |                  |                  |                  "},
	{2, "B                 |                  |                  |   UEGEGEEGEGEU   |   E          E   |   G          G   |   E          E   |   G          G   |   E      U   E   |   E    UU    E   |   G          G   |   E          E   |   G          G   |   E          E   |   UEGEGEEGEGEU   |                  |                  |                  "},
};





static const sEndCityBlueprintLayer Layers_ThirdRoof[] =
{
	{0, "B YNNYYNYYNYYNNY  |YSSSSSSSSSSSSSSSSY|YSPPPPPPPPPPPPPPSY|NSPPPPPPPPPPPPPPSN|NSPPPPPPPPPPPPPPSN|YSPPPPPPPPPPPPPPSY|YSPPPPPPPPPPPPPPSY|NSPPPPPPPPPPPPPPSN|YSPPPPPPPPPPPPPPSY|YSPPPPPPPPPPPPPPSY|NSPPPPPPPPPPPPPPSN|YSPPPPPPPPPPPPPPSY|YSPPPPPPPPPPPPPPSY|NSPPPPPPPPPPPPPPSN|NSPPPPPPPPPPPPPPSN|YSPPPPPPPPPPPPPPSY|YSSSSSSSSSSSSSSSSY|  YNNYYNYYNYYNNY  "},
	{1, "B                 |                  |  D            D  |                  |                  |                  |                  |                  |                  |                  |                  |                  |                  |                  |                  |  D            D  |                  |                  "},
};




static const sEndCityBlueprintLayer Layers_TowerBase[] =
{
	{0, "       |       |       |       |   l   |   U   |       "},
	{1, "       |       |       |       |   l   |   U   |       "},
	{2, "       |       |       |       |   l   |   U   |       "},
	{3, "PPPPPPP|PPPPPPP|PP   PP|PPL  PP|PP   PP|PPPPPPP|PPPPPPP"},
	{4, "       |  UUU  | U L U | U   U | U   U |  UUU  |       "},
	{5, "   S   |  UpU  | U   U |Su  LuS| U   U |  UpU  |   S   "},
	{6, "       |  UUU  | U   U | U   U | U L U |  UUU  |       "},
};





static const sEndCityBlueprintLayer Layers_TowerPiece[] =
{
	{0, "       |  PPP  | P   P | PL  P | P   P |  PPP  |       "},
	{1, "       |  UUU  | U L U | U   U | U   U |  UUU  |       "},
	{2, "   S   |  UpU  | U   U |Su  LuS| U   U |  UpU  |   S   "},
	{3, "       |  UUU  | U   U | U   U | U L U |  UUU  |       "},
};





static const sEndCityBlueprintLayer Layers_TowerFloor[] =
{
	{0, "       |  PPP  | PPPPP | PLPPP | PPPPP |  PPP  |       "},
	{1, "       |  UUU  | U L U | U   U | U   U |  UUU  |       "},
	{2, "   S   |  UpU  | U   U |Su  LuS| U   U |  UpU  |   S   "},
	{3, "       |  UUU  | U   U | U   U | U L U |  UUU  |       "},
};





static const sEndCityBlueprintLayer Layers_TowerTop[] =
{
	{0, "  B   B  | SSsSsSS |BSPPPPPSB| sPLLPPs | SP   PS | sP   Ps |BSPPPPPSB| SSsSsSS |  B   B  "},
	{1, "         | D     D |  UEEEU  |  E      |  E      |  E   E  |  UEEEU  | D     D |         "},
	{2, "         |         |  UEGEU  |  E      |  G      |  E   E  |  UEGEU  |         |         "},
	{3, "         |         |  UEGEU  |  E   E  |  G H E  |  E   E  |  UEGEU  |         |         "},
	{4, "SSssSssSS|SPPPPPPPS|sPPPPPPPs|sPPPPPPPs|SPPPPPPPS|sPPPPPPPs|sPPPPPPPs|SPPPPPPPS|SSssSssSS"},
};





static const sEndCityBlueprintLayer Layers_FatTower[] =
{
	{0, "              |      PPP     |    PPPPPPP   |   PPPPPPPPP  |   PPPPPPPPP  |  PPPP   PPPP |  PPPP   PPPP |  PPPP L PPPP |   PPPPPPPPP  |   PPPPPPPPP  |    PPPPPPP   |      PPP     |              "},
	{1, "              |      UUU     |    UU   UU   |   U       U  |   U       U  |  UL        U |  U         U |  U        LU |   U       U  |   U       U  |    UU   UU   |      UUU     |              "},
	{2, "       S      |      UuU     |    UU   UU   |   U       U  |   UE      U  |  U         U | Sp         pS|  U         U |   U      eU  |   U       U  |    UU   UU   |      UuU     |       S      "},
	{3, "              |      UUU     |    UU   UU   |   U d     U  |   U       U  |  U         U |  U         U |  U         U |   U       U  |   U     D U  |    UU   UU   |      UUU     |              "},
	{4, "              |      PPP     |    PPL  PP   |   P       P  |   P       P  |  P         P |  P         P |  P         P |   P       P  |   P       P  |    PP  LPP   |      PPP     |              "},
	{5, "              |      UUU     |    UU  LUU   |   U       U  |   U       U  |  U         U |  U         U |  U         U |   U       U  |   U       U  |    UUL  UU   |      UUU     |              "},
	{6, "       S      |      UuU     |    UU   UU   |   U     d U  |   U       U  |  U         U | SpK       kpS|  U         U |   U       U  |   U D     U  |    UU   UU   |      UuU     |       S      "},
	{7, "              |      UUU     |    UU   UU   |   U       U  |   U      eU  |  U         U |  U         U |  U         U |   UE      U  |   U       U  |    UU   UU   |      UUU     |              "},
	{8, "              |      PPP     |    PP   PP   |   P       P  |   P       P  |  P        LP |  P         P |  PL        P |   P       P  |   P       P  |    PP   PP   |      PPP     |              "},
	{9, "              |      UUU     |    UU   UU   |   U       U  |   U       U  |  UL        U |  U         U |  U        LU |   U       U  |   U       U  |    UU   UU   |      UUU     |              "},
	{10, "       S      |      UuU     |    UU h UU   |   U       U  |   UE      U  |  U         U | Sp         pS|  U         U |   U      eU  |   U       U  |    UU H UU   |      UuU     |       S      "},
	{11, "       U      |      UUU     |    UU   UU   |   U d     U  |   U       U  |  U         U | UU         UU|  U         U |   U       U  |   U     D U  |    UU   UU   |      UUU     |       U      "},
};





static const sEndCityBlueprintLayer Layers_FatTowerTop[] =
{
	{0, "                    |   YYNNYYNNNYYNNYY  |   SSSSSSSSSSSSSSS  |  YSPPPPPPPPPPPPPSY |  NSPPPPPPPPPPPPPSN |  NSPPPPPL PPPPPPSN |  YSPPP    PPPPPPSY |  YSPPP    PPPPPPSY |  NSPP     PPPPPPSN |  NSPP         PPSN |  NSPPPPPP     PPSN |  YSPPPPPP    PPPSY |  YSPPPPPP    PPPSY |  NSPPPPPP LPPPPPSN |  NSPPPPPPPPPPPPPSN |  YSPPPPPPPPPPPPPSY |   SSSSSSSSSSSSSSS  |   YYNNYYNNNYYNNYY  |                    "},
	{1, "                    |                    |   D             D  |    UEEEEEEEEEEEU   |    E    LPL    E   |    E           E   |    E           E   |    E           E   |    E      U    E   |    E           E   |    E    U      E   |    E           E   |    EC          E   |    E           E   |    E  C        E   |    UEEEEEEEEEEEU   |   D             D  |                    |                    "},
	{2, "                    |                    |                    |    UEGEEGEEEEEEU   |    E      LPL  E   |    G           E   |    E           E   |    E           E   |    G      U    G   |    E           E   |    G    U      G   |    E           E   |    E           E   |    G           G   |    E           E   |    UEGEEGEGEEGEU   |                    |                    |                    "},
	{3, "                    |                    |                    |    UEGEEGEEEEEEU   |    E        LPLE   |    G           E   |    E           E   |    E           E   |    G      U    G   |    E           E   |    G    U      G   |    E           E   |    E           E   |    G           G   |    E           E   |    UEGEEGEGEEGEU   |                    |                    |                    "},
	{4, "  YYNNYYNNYNNYYNNYY |  SSSSSSSSSSSSSSSSS | YSPPPPPPPPPPPPPPPSY| NSPPPPPPPPPPPPPPPSN| NSPPPPPPP     LPPSN| YSPPPPPPPPPPPPPPPSY| YSPPPPPPPPPPPPPPPSY| NSPPPPPPPPPPPPPPPSN| NSPPPPPPPPPPPPPPPSN| YSPPPPPPPPPPPPPPPSY| NSPPPPPPPPPPPPPPPSN| NSPPPPPPPPPPPPPPPSN| YSPPPPPPPPPPPPPPPSY| YSPPPPPPPPPPPPPPPSY| NSPPPPPPPPPPPPPPPSN| NSPPPPPPPPPPPPPPPSN| YSPPPPPPPPPPPPPPPSY|  SSSSSSSSSSSSSSSSS |  YYNNYYNNYNNYYNNYY "},
	{5, "                    |                    |                    |         SSSSS      |         S          |         SSSSS      |                    |                    |                    |                    |                    |                    |                    |                    |                    |                    |                    |                    |                    "},
};





static const sEndCityBlueprintLayer Layers_BaseFloor[] =
{
	{0, "            |            |  PPPPPPPP  | SPPPPPPPP  | PPPPPPPPP  | PPPPPPPPP  | PPPPPPPPP  | PPPPPPPPP  | SPPPPPPPP  |  PPPPPPPP  |            |            "},
	{1, "            |            |  UEEEEEEU  |  E      E  |  E      E  |         E  |         E  |  E      E  |  E      E  |  UEEEEEEU  |            |            "},
	{2, "            |            |  UEGEEGEU  |  E      E  | KE      G  |         E  |         E  | KE      G  |  E      E  |  UEGEEGEU  |            |            "},
	{3, "            |            |  UEGEEGEU  |  E      E  |  E      G  |         E  |         E  |  E      G  |  E      E  |  UEGEEGEU  |            |            "},
};





static const sEndCityBlueprintLayer Layers_BaseRoof[] =
{
	{0, "SSSSSSSSSSSS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SSSSSSSSSSSS"},
	{1, "            | D        D |            |            |            |            |            |            |            |            | D        D |            "},
};





static const sEndCityBlueprintLayer Layers_EmptyRoom[] =
{
	{0, "            |            |  PPPPPPPP  | SPPPPPPPP  | PPPPPPPPP  | PPPPPPPPP  | PPPPPPPPP  | PPPPPPPPP  | SPPPPPPPP  |  PPPPPPPP  |            |            "},
	{1, "            |            |  UEEEEEEU  |  E      E  |  E      E  |         E  |         E  |  E      E  |  E      E  |  UEEEEEEU  |            |            "},
	{2, "            |            |  UEGEEGEU  |  E      E  | KE      G  |         E  |         E  | KE      G  |  E      E  |  UEGEEGEU  |            |            "},
	{3, "            |            |  UEGEEGEU  |  E      E  |  E      G  |         E  |         E  |  E      G  |  E      E  |  UEGEEGEU  |            |            "},
	{4, "SSSSSSSSSSSS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SPPPPPPPPPPS|SSSSSSSSSSSS"},
	{5, "            | D        D |            |            |            |            |            |            |            |            | D        D |            "},
};





static const sEndCityBlueprintLayer Layers_LootRoom2[] =
{
	{0, "||    SPPPPS|   PPPPPPPP|   PPPPPPPP|   PPPPPPPP|   PPPPPPPP|   PPPPPPPP|   PPPPPPPP|   PPPPPPPP|   PPPPPPPP|||"},
	{1, "|||   UEE  EEU|   E      E|   E  L   E|   E  SP  E|   E  PP  E|   E      E|   E      E|   UEEEEEEU|||"},
	{2, "|||   UEE  EEU|   E      E|   G      G|   E   P  E|   E LSP  E|   G      G|   E      E|   UEGEEGEU|||"},
	{3, "|||   UEE  EEU|   E      E|   G      G|   E   PL E|   E   S  E|   G   L  G|   E      E|   UEGEEGEU|||"},
	{4, "| SSSS     SSS| SPPPPPPPPPPS| SPPPPPPPPPPS| SPP      PPS| SPP      PPS| SPPLLLS  PPS| SPP      PPS| SPP      PPS| SPP      PPS| SPPPPPPPPPPS| SPPPPPPPPPPS| SSSSSSSSSSSS|"},
	{5, "| D          D|  UEEEEEEEEU|  E        E|  ES   U   E|  ES   A   E|  E        E|  E        E|  E        E|  E        E|  E        E|  UEEEEEEEEU| D          D|"},
	{6, "||  UEEEEEEEEU|  E        E|  ES   U   G|  ES   A   E|  E        E|  E        E|  E        E|  G        G|  E        E|  UEGEEEEGEU||"},
	{7, "||  UEEEEEEEEU|  E        E|  ES   U   G|  ES   A   E|  E        E|  E        E|  E        E|  G        G|  E        E|  UEGEEEEGEU||"},
	{8, "SSSSSSSSSSSSSS|SPPPPPPPPPPPPS|SPPPPPPPPPPPPS|SPPPPPPPPPPPPS|SPPPPPPPPPPPPS|SPPPPPPPPPPPPS|SPPPPPPPPPPPPS|SPPPPPPPPPPPPS|SPPPPPPPPPPPPS|SPPPPPPPPPPPPS|SPPPPPPPPPPPPS|SPPPPPPPPPPPPS|SPPPPPPPPPPPPS|SSSSSSSSSSSSSS"},
	{9, "| D          D||||||||||| D          D|"},
};





static const sEndCityBlueprintLayer Layers_LootRoom3[] =
{
	{0, "|||     SPPPPS|    PPPPPPPP|    PPPPPPPP|    PPPPPPPP|    PPPPPPPP|    PPPPPPPP|    PPPPPPPP|    PPPPPPPP|    PPPPPPPP||||"},
	{1, "||||    UEE  EEU|    E      E|    E  L   E|    E  SP  E|    E  PP  E|    E      E|    E      E|    UEEEEEEU||||"},
	{2, "||||    UEE  EEU|    E      E|    G      G|    E   P  E|    E LSP  E|    G      G|    E      E|    UEGEEGEU||||"},
	{3, "||||    UEE  EEU|    E      E|    G      G|    E   PL E|    E   S  E|    G   L  G|    E      E|    UEGEEGEU||||"},
	{4, "||  SSSS     SSS|  SPPPPPPPPPPS|  SPPPPPPPPPPS|  SPP   PPPPPS|  SPP      PPS|  SPPLLLS  PPS|  SPP      PPS|  SPP      PPS|  SPP      PPS|  SPPPPPPPPPPS|  SPPPPPPPPPPS|  SSSSSSSSSSSS||"},
	{5, "||  D          D|   UEEEEEEEEU|   E PPPP   E|   ES   PPS E|   ES       E|   E        E|   E        E|   E        E|   E        E|   E        E|   UEEEEEEEEU|  D          D||"},
	{6, "|||   UEEEEEEGEU|   E PPPP   E|   ES   PS  G|   ES       E|   E        E|   E        E|   E        E|   G        G|   E        E|   UEGEEEEGEU|||"},
	{7, "|||   UEEEEEEGEU|   E PS     E|   ESL      G|   ESL      E|   E        E|   E        E|   E        E|   G        G|   E        E|   UEGEEEEGEU|||"},
	{8, "| SSSSSSSSSSSSSS| SPPPPPPPPPPPPS| SPPPPPPPPPPPPS| SPPPS      PPS| SPP        PPS| SPP  PPPP  PPS| SPP        PPS| SPP        PPS| SPP        PPS| SPP   DD   PPS| SPP   PP   PPS| SPPPPPPPPPPPPS| SPPPPPPPPPPPPS| SSSSSSSSSSSSSS|"},
	{9, "| D            D|  UEEEEEEEEEEU|  E          E|  EP        PE|  E          E|  EP         E|  E      U   E|  EP     A   E|  E          E|  E          E|  E          E|  E    eC    E|  UEEEEEEEEEEU| D            D|"},
	{10, "||  UEGEGEEGEGEU|  E          E|  EP        PE|  E          E|  EPPP       E|  E      U   E|  EP     A   E|  G          G|  E          E|  G          G|  E          E|  UEGEGEEGEGEU||"},
	{11, "||  UEGEGEEGEGEU|  E          E|  EP        PE|  E          E|  EP         E|  E      U   E|  EP     A   E|  G          G|  E          E|  G          G|  E          E|  UEGEGEEGEGEU||"},
	{12, "SSSSSSSSSSSSSSSS|SPPPPPPPPPPPPPPS|SPPPPPPPPPPPPPPS|SPPPPPPPPPPPPPPS|SPPPPPPPPPPPPPPS|SPPPPPPPPPPPPPPS|SPPPPPPPPPPPPPPS|SPPPPPPPPPPPPPPS|SPPPPPPPPPPPPPPS|SPPPPPPPPPPPPPPS|SPPPPPPPPPPPPPPS|SPPPPPPPPPPPPPPS|SPPPPPPPPPPPPPPS|SPPPPPPPPPPPPPPS|SPPPPPPPPPPPPPPS|SSSSSSSSSSSSSSSS"},
	{13, "| D            D||||||||||||| D            D|"},
};





static const sEndCityBlueprintLayer Layers_BridgePiece[] =
{
	{0, "  P  |  P  |  P  | SPS "},
	{1, "BBBBB|BBBBB|BBBBB|SBBBS"},
	{2, "S   S|S   S|S   S|B   B"},
};





static const sEndCityBlueprintLayer Layers_BridgeGentleStairs[] =
{
	{0, "  P  |     |     |     |     |     |     |     "},
	{1, "SBBBS|     |     |     |     |     |     |     "},
	{2, "BHHHB|BBBBB|SBBBS|     |     |     |     |     "},
	{3, "S   S|S   S|BHHHB|BBBBB|SBBBS|     |     |     "},
	{4, "     |     |S   S|S   S|BHHHB|BBBBB|SBBBS|  P  "},
	{5, "     |     |     |     |S   S|S   S|BHHHB|BBBBB"},
	{6, "     |     |     |     |     |     |S   S|B   B"},
};





static const sEndCityBlueprintLayer Layers_BridgeSteepStairs[] =
{
	{0, "  P  |     |     |     "},
	{1, " BBB |     |     |     "},
	{2, "BSSSB| SBS |     |     "},
	{3, "S   S|BSSSB| SBS |     "},
	{4, "     |S   S|BSSSB| SBS "},
	{5, "     |     |S   S|BSSSB"},
	{6, "     |     |     |B   B"},
};





static const sEndCityBlueprintLayer Layers_BridgeEnd[] =
{
	{0, "  P  | BBB "},
	{1, "BSSSB|BBBBB"},
	{2, "B   B|B   B"},
	{3, "E   E|B   B"},
	{4, "     |BH HB"},
	{5, "     |HBBBH"},
};





static const sEndCityBlueprintLayer Layers_Ship[] =
{
	{0, "              |              |              |              |              |              |              |              |              |              |              |      P       |      P       |      P       |      P       |      P       |      P       |      P       |      P       |      P       |      P       |      P       |      P       |      P       |              |              |              |              |              "},
	{1, "              |              |              |              |              |              |              |              |      P       |      P       |      P       |      P       |     BBB      |    sBBBs     |     BBB      |     BBB      |     BBB      |    sBBBs     |     BBB      |     BBB      |     BBB      |    sBBBs     |     BBB      |     BPB      |     BPB      |              |              |              |              "},
	{2, "              |              |              |              |              |              |      P       |      P       |     BPB      |     BPB      |    BBPBB     |    BBPBB     |    BoPoB     |   sBoPoBs    |    BoPoB     |    BoPoB     |    BoPoB     |   sBoPoBs    |    BoPoB     |    BoPoB     |    BoPoB     |   sBoPoBs    |    BoPoB     |    BoPoB     |    BoPoB     |   sBBPBBs    |              |              |              "},
	{3, "              |              |              |              |              |      P       |      P       |     BBB      |    BBBBB     |    B   B     |   B     B    |   B     B    |   B     B    |  sB     Bs   |   B  P  B    |   B    sB    |   B    BB    |  sB    BBs   |   B    BB    |   B    BB    |   B    BB    |  sB    BBs   |   B    BB    |   B    BB    |   B    BB    |  sBBBBBBBs   |              |              |              "},
	{4, "              |              |              |              |              |      B       |     BBB      |    Bc cB     |    B h B     |   B     B    |   B     B    |  B       B   |  B       B   |  B       B   |  B   P   B   |  B       B   |  B       B   |  B       B   |  B       B   |  B     ssB   |  B     BBB   |  B     BBB   |  B     BBB   |  B     BBB   |  B     BBB   |  BBBBBBBBB   |  s   s   s   |              |              "},
	{5, "              |              |              |              |      P       |      P       |     BBB      |    B i B     |    B   B     |   B     B    |   B     B    |  B       B   |  B       B   | sB       Bs  |  B   P   B   |  B       B   | sB       Bs  |  B       B   |  B       B   |  B       B   | sBBBBBBssBs  | sBBBBBBBBBs  | sBBBBBBBBBs  | sBBBBBBBBBs  | sBBBBBBBBBs  | sBBBBBBBBBs  | sBBBBBBBBBs  | sssssssssss  |              "},
	{6, "              |              |              |      P       |      P       |      B       |     BeB      |    B   B     |    B   B     |   B     B    |   B     B    |  B       B   |  B       B   |              |  B   P   B   |  B       B   |              |  B       B   |  B       B   |  BBBBBB  B   |  BBssBB  B   |  PB  BB  P   |  E       E   |  E       E   |  E       E   |  E   P   E   |  PEEEPEEEP   | e      h  e  |              "},
	{7, "              |      P       |      P       |      P       |      P       |     BBB      |    BBBBB     |    BBBBB     |   BBBBBBB    |   BBBBBBB    |  BBBBBBBBB   |  BBBBBBBBB   |  BBBBPBBBB   |  BBBBgBBBB   |  BBBBPBBBB   |  BBBBBBBBB   |  BBBBBBBBB   |  BBBBBBBBB   |  BBBBBBBBB   |  BBssBB  B   |  BB  BB  B   |  PB  BB  P   |  E       E   |  g       g   |  g       g   |  E   b   E   |  PEgEPEgEP   |              |              "},
	{8, "      d       |      s       |              |              |      B       |     B B      |    B   B     |    s   s     |   Bs   sB    |   s     s    |  Bs  P  sB   |  s   P   s   |  s   s   s   |  B       B   |  s   P   s   |  s   L   s   |  B       B   |  s       s   |  s       s   |  B     ssB   |  B     BBB   |  PB  BBssP   |  E       E   |  E       E   |  E       E   |  E       E   |  PEEEPEEEP   |              |              "},
	{9, "              |              |              |              |      e       |              |              |              |      P       |      P       |  e   s   e   |              |              |              |      P       |      L       |              |              |              |ssss       ss |sBBB  BBssBBs |sBBBBBBBBBBBs |sBBBBBBBBBBBs |sBBBBBBBBBBBs |sBBBBBBBBBBBs |sBBBBBBBBBBBs |sBBBBBBBBBBBs |sBBBBBBBBBBBs |sssssssssssss "},
	{10, "              |              |              |              |              |              |      P       |      P       |      s       |              |              |              |              |              |      P       |      L       |              |              |              |              | BsB  BB  BB  | s ssss    s  | s  P      s  | s         s  | s         s  | s         s  | s  sssss  s  | BssBsssBssB  |              "},
	{11, "              |              |              |              |      P       |      P       |      s       |              |              |              |              |              |              |              |      P       |      L       |              |              |              |              |              |              |              |              |              |              |              |    h         |              "},
	{12, "              |      S       |      P       |      P       |      s       |              |              |              |              |              |              |              |              |              |      P       |      L       |              |              |              |              |              |              |              |              |              |              |              |              |              "},
	{13, "              |              |              |              |              |              |              |              |              |              |              |              |              |              |      P       |      L       |              |              |              |              |              |              |              |              |              |              |              |              |              "},
	{14, "              |              |              |              |              |              |              |              |              |              |              |              |              |              |      P       |      L       |              |              |              |              |              |              |              |              |              |              |              |              |              "},
	{15, "              |              |              |              |              |              |              |              |              |              |              |              |              |              |      P       |      L       |              |              |              |              |              |              |              |              |              |              |              |              |              "},
	{16, "              |              |              |              |              |              |              |              |              |              |              |              |              |              |      P       |      L       |              |              |              |              |              |              |              |              |              |              |              |              |              "},
	{17, "              |              |              |              |              |              |              |              |              |              |              |              |              |              |      P       |      L       |              |              |              |              |              |              |              |              |              |              |              |              |              "},
	{18, "              |              |              |              |              |              |              |              |              |              |              |              |              |              |      P       |      L       |              |              |              |              |              |              |              |              |              |              |              |              |              "},
	{19, "              |              |              |              |              |              |              |              |              |              |              |              |              |              |      P       |      L       |              |              |              |              |              |              |              |              |              |              |              |              |              "},
	{20, "              |              |              |              |              |              |              |              |              |              |              |              |              |     S S      |     SPS      |     SLS      |              |              |              |              |              |              |              |              |              |              |              |              |              "},
	{21, "              |              |              |              |              |              |              |              |              |              |              |              |    sssss     |    s   s     |    s P s     |    s L s     |    sssss     |              |              |              |              |              |              |              |              |              |              |              |              "},
	{22, "              |              |              |              |              |              |              |              |              |              |              |              |              |              |      P       |              |              |              |              |              |              |              |              |              |              |              |              |              |              "},
	{23, "              |              |              |              |              |              |              |              |              |              |              |              |              |			           |      B       |              |              |              |              |              |              |              |              |              |              |              |              |              |			           "},
};





const sEndCityBlueprint g_EndCityBlueprints[] =
{
	{"SecondFloor1", "P=Purpur Block|S=Purpur Stairs|U=Purpur Pillar@top|E=End Stone Bricks|L=Purpur Slab|G=Magenta Stained Glass|Y=Green Wool|N=Red Wool|D=End Rod|A=Ladder|O=EntitySprite:Shulker|B=Black Wool", 18, 18, 3, Layers_SecondFloor1, true},
	{"SecondRoof", "P=Purpur Block|S=Purpur Stairs|U=Purpur Pillar@top|E=End Stone Bricks|L=Purpur Slab|G=Magenta Stained Glass|Y=Green Wool|N=Red Wool|D=End Rod|A=Ladder|O=EntitySprite:Shulker|B=Black Wool", 18, 18, 1, Layers_SecondRoof, true},
	{"ThirdFloor1", "P=Purpur Block|S=Purpur Stairs|U=Purpur Pillar@top|E=End Stone Bricks|L=Purpur Slab|G=Magenta Stained Glass|Y=Green Wool|N=Red Wool|D=End Rod|A=Ladder|O=EntitySprite:Shulker|B=Black Wool", 18, 18, 3, Layers_ThirdFloor1, true},
	{"ThirdRoof", "P=Purpur Block|S=Purpur Stairs|U=Purpur Pillar@top|E=End Stone Bricks|L=Purpur Slab|G=Magenta Stained Glass|Y=Green Wool|N=Red Wool|D=End Rod|A=Ladder|O=EntitySprite:Shulker|B=Black Wool", 18, 18, 2, Layers_ThirdRoof, true},
	{"TowerBase", "P=Purpur Block|U=Purpur Pillar@top|u=Purpur Pillar@horizontal|p=Purpur Pillar|L=Purpur Slab|S=Purpur Stairs|l=Ladder", 7, 7, 7, Layers_TowerBase},
	{"TowerPiece", "P=Purpur Block|U=Purpur Pillar@top|u=Purpur Pillar@horizontal|p=Purpur Pillar|L=Purpur Slab|S=Purpur Stairs", 7, 7, 4, Layers_TowerPiece},
	{"TowerFloor", "P=Purpur Block|U=Purpur Pillar@top|u=Purpur Pillar@horizontal|p=Purpur Pillar|L=Purpur Slab|S=Purpur Stairs", 7, 7, 4, Layers_TowerFloor},
	{"TowerTop", "P=Purpur Block|S=Purpur Stairs-rot180|s=Purpur Stairs|L=Purpur Slab|B=Magenta Wall Banner|D=End Rod|E=End Stone Bricks|U=Purpur Pillar@top|G=Magenta Stained Glass|H=EntitySprite:Shulker-rot180", 9, 9, 5, Layers_TowerTop},
	{"FatTower", "P=Purpur Block|S=Purpur Stairs|L=Purpur Slab|D=End Rod|E=End Rod-rot90|d=End Rod-rot180|e=End Rod-rot270|U=Purpur Pillar@top|u=Purpur Pillar|p=Purpur Pillar@horizontal|H=EntitySprite:Shulker|K=EntitySprite:Shulker-rot90|h=EntitySprite:Shulker-rot180|k=EntitySprite:Shulker-rot270", 14, 13, 12, Layers_FatTower},
	{"FatTowerTop", "P=Purpur Block|S=Purpur Stairs|L=Purpur Slab|Y=Green Wool|N=Red Wool|C=Chest|U=Purpur Pillar@top|G=Magenta Stained Glass|E=End Stone Bricks|D=End Rod", 20, 19, 6, Layers_FatTowerTop},
	{"BaseFloor", "P=Purpur Block|U=Purpur Pillar@top|S=Purpur Stairs|E=End Stone Bricks|D=End Rod|G=Magenta Stained Glass|K=EntitySprite:Shulker-rot270", 12, 12, 4, Layers_BaseFloor, true},
	{"BaseRoof", "P=Purpur Block|S=Purpur Stairs|D=End Rod", 12, 12, 2, Layers_BaseRoof, true},
	{"EmptyRoom", "P=Purpur Block|U=Purpur Pillar@top|S=Purpur Stairs|E=End Stone Bricks|D=End Rod|G=Magenta Stained Glass|K=EntitySprite:Shulker-rot270", 12, 12, 6, Layers_EmptyRoom, true},
	{"LootRoom2", "P=Purpur Block|S=Purpur Stairs|U=Purpur Pillar@top|E=End Stone Bricks|L=Purpur Slab|G=Magenta Stained Glass|D=End Rod|A=Ladder", 14, 14, 10, Layers_LootRoom2, true},
	{"LootRoom3", "P=Purpur Block|S=Purpur Stairs|U=Purpur Pillar@top|E=End Stone Bricks|L=Purpur Slab|G=Magenta Stained Glass|D=End Rod|A=Ladder|C=Chest|e=Ender Chest", 16, 16, 14, Layers_LootRoom3, true},
	{"BridgePiece", "B=Purpur Block|P=Purpur Pillar|S=Purpur Stairs", 5, 4, 3, Layers_BridgePiece},
	{"BridgeGentleStairs", "B=Purpur Block|H=Purpur Slab|P=Purpur Pillar|S=Purpur Stairs", 5, 8, 7, Layers_BridgeGentleStairs},
	{"BridgeSteepStairs", "B=Purpur Block|P=Purpur Pillar|S=Purpur Stairs", 5, 4, 7, Layers_BridgeSteepStairs},
	{"BridgeEnd", "B=Purpur Block|P=Purpur Pillar|S=Purpur Stairs|H=Purpur Slab|E=End Rod", 5, 2, 6, Layers_BridgeEnd},
	{"Ship", "P=Purpur Pillar|s=Purpur Stairs|B=Purpur Block|S=Purpur Slab|o=Obsidian|z=Red Wool|c=Chest|i=EntitySprite:Item Frame|E=End Stone Bricks|e=End Rod|b=Brewing Stand|g=Magenta Stained Glass|d=Dragon Head|L=Ladder|h=EntitySprite:Shulker", 14, 29, 24, Layers_Ship},
};





const int g_NumEndCityBlueprints = ARRAYCOUNT(g_EndCityBlueprints);
