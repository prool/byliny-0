/* ************************************************************************
*   File: spell_parser.c                                Part of CircleMUD *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"

struct spell_info_type   spell_info[TOP_SPELL_DEFINE + 1];
struct spell_create_type spell_create[TOP_SPELL_DEFINE + 1];
struct skill_info_type   skill_info[TOP_SKILL_DEFINE + 1];
char   cast_argument[MAX_STRING_LENGTH];
extern const  char *cast_phrase[LAST_USED_SPELL+1][2];

#define SpINFO spell_info[spellnum]
#define SkINFO skill_info[skillnum]

extern struct char_data *character_list;
extern struct room_data *world;
extern int what_sky;
int check_recipe_items(struct char_data * ch, int spellnum, int spelltype, int extract);
int check_recipe_values(struct char_data * ch, int spellnum, int spelltype, int showrecipe);
int may_pkill(struct char_data *revenger, struct char_data *killer);
int equip_in_metall(struct char_data *ch);
int attack_best(struct char_data *ch, struct char_data *victim);
int check_pkill(struct char_data *ch, struct char_data *victim, char *arg);
/* local functions */
void say_spell(struct char_data * ch, int spellnum, struct char_data * tch, struct obj_data * tobj);
void spello(int spl, const char *name, const char *syn,
            int max_mana, int min_mana, int mana_change,
            int minpos, int targets, int violent, int routines, int danger);
int mag_manacost(struct char_data * ch, int spellnum);
ACMD(do_cast);
ACMD(do_ident);
ACMD(do_create);
ACMD(do_forget);
ACMD(do_remember);
ACMD(do_mixture);

void unused_spell(int spl);
void unused_skill(int spl);
void mag_assign_spells(void);

/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, 200 should provide
 * ample slots for skills.
 */

struct syllable
{ const char *org;
  const char *news;
};


struct syllable syls[] = {
  {" ", " "},
  {"ar", "abra"},
  {"ate", "i"},
  {"cau", "kada"},
  {"blind", "nose"},
  {"bur", "mosa"},
  {"cu", "judi"},
  {"de", "oculo"},
  {"dis", "mar"},
  {"ect", "kamina"},
  {"en", "uns"},
  {"gro", "cra"},
  {"light", "dies"},
  {"lo", "hi"},
  {"magi", "kari"},
  {"mon", "bar"},
  {"mor", "zak"},
  {"move", "sido"},
  {"ness", "lacri"},
  {"ning", "illa"},
  {"per", "duda"},
  {"ra", "gru"},
  {"re", "candus"},
  {"son", "sabru"},
  {"tect", "infra"},
  {"tri", "cula"},
  {"ven", "nofo"},
  {"word of", "inset"},
  {"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"}, {"f", "y"}, {"g", "t"},
  {"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"},
  {"o", "a"}, {"p", "s"}, {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"}, {"u", "e"},
  {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"}, {"z", "k"}, {"", ""}
};


const char *unused_spellname = "!UNUSED!"; /* So we can get &unused_spellname */

int MAGIC_SLOT_VALUES[LVL_IMPL+1][MAX_SLOT] =
{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  // 0
 {2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {4, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {4, 1, 0, 0, 0, 0, 0, 0, 0, 0},
 {4, 2, 0, 0, 0, 0, 0, 0, 0, 0},
 {4, 3, 0, 0, 0, 0, 0, 0, 0, 0},
 {4, 3, 1, 0, 0, 0, 0, 0, 0, 0},
 {4, 3, 2, 0, 0, 0, 0, 0, 0, 0},
 {4, 4, 3, 0, 0, 0, 0, 0, 0, 0},
 {4, 4, 3, 1, 0, 0, 0, 0, 0, 0},  // 10
 {4, 4, 3, 2, 0, 0, 0, 0, 0, 0},
 {4, 4, 4, 3, 0, 0, 0, 0, 0, 0},
 {4, 4, 4, 3, 1, 0, 0, 0, 0, 0},
 {4, 4, 4, 4, 2, 0, 0, 0, 0, 0},
 {4, 5, 4, 4, 3, 0, 0, 0, 0, 0},
 {5, 5, 4, 4, 3, 1, 0, 0, 0, 0},
 {5, 5, 5, 4, 3, 2, 0, 0, 0, 0},
 {5, 5, 5, 5, 3, 3, 0, 0, 0, 0},
 {5, 5, 5, 5, 4, 3, 1, 0, 0, 0},
 {6, 5, 6, 5, 4, 3, 2, 0, 0, 0},  // 20
 {6, 6, 6, 5, 5, 4, 3, 0, 0, 0},
 {6, 6, 6, 6, 5, 5, 4, 0, 0, 0},
 {6, 6, 6, 6, 5, 5, 4, 1, 0, 0},
 {6, 6, 6, 6, 6, 5, 4, 2, 0, 0},
 {7, 6, 6, 6, 6, 6, 4, 3, 0, 0},
 {7, 7, 6, 6, 6, 6, 5, 4, 0, 0},
 {7, 7, 7, 6, 6, 6, 5, 4, 1, 0},
 {7, 7, 7, 7, 7, 6, 5, 4, 2, 0},
 {7, 7, 7, 7, 7, 6, 5, 5, 3, 0},
 {7, 7, 7, 7, 7, 6, 6, 5, 4, 0}, // 30
 {9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
 {9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
 {9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
 {9, 9, 9, 9, 9, 9, 9, 9, 9, 9}  // 34
};

int CLERIC_SLOT_VALUES[LVL_IMPL+1][MAX_SLOT] =
{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 0
 {2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {4, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {4, 1, 0, 0, 0, 0, 0, 0, 0, 0},
 {4, 2, 0, 0, 0, 0, 0, 0, 0, 0},
 {5, 2, 0, 0, 0, 0, 0, 0, 0, 0},
 {5, 3, 0, 0, 0, 0, 0, 0, 0, 0},
 {5, 3, 1, 0, 0, 0, 0, 0, 0, 0},
 {5, 3, 2, 0, 0, 0, 0, 0, 0, 0}, // 10
 {6, 4, 2, 0, 0, 0, 0, 0, 0, 0},
 {6, 4, 3, 0, 0, 0, 0, 0, 0, 0},
 {6, 4, 3, 1, 0, 0, 0, 0, 0, 0},
 {6, 4, 3, 2, 0, 0, 0, 0, 0, 0},
 {6, 5, 4, 2, 0, 0, 0, 0, 0, 0},
 {7, 5, 4, 3, 0, 0, 0, 0, 0, 0},
 {7, 5, 4, 3, 1, 0, 0, 0, 0, 0},
 {7, 5, 4, 4, 2, 0, 0, 0, 0, 0},
 {7, 5, 5, 4, 2, 0, 0, 0, 0, 0},
 {7, 6, 5, 4, 3, 0, 0, 0, 0, 0}, // 20
 {7, 6, 5, 4, 3, 1, 0, 0, 0, 0},
 {8, 6, 5, 5, 3, 2, 0, 0, 0, 0},
 {8, 6, 5, 5, 4, 2, 0, 0, 0, 0},
 {8, 6, 6, 5, 4, 3, 0, 0, 0, 0},
 {8, 6, 6, 5, 4, 3, 1, 0, 0, 0},
 {8, 7, 6, 5, 4, 3, 2, 0, 0, 0},
 {8, 7, 6, 6, 5, 4, 2, 0, 0, 0},
 {8, 7, 6, 6, 5, 4, 3, 0, 0, 0},
 {9, 7, 6, 6, 5, 4, 3, 0, 0, 0},
 {9, 7, 7, 6, 5, 4, 3, 0, 0, 0}, // 30
 {9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
 {9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
 {9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
 {9, 9, 9, 9, 9, 9, 9, 9, 9, 9}  // 34
};

int RANGER_SLOT_VALUES[][MAX_SLOT] =
{ {0,0,0,0,0,0,0,0,0,0}, // 0
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {1,0,0,0,0,0,0,0,0,0},
  {1,0,0,0,0,0,0,0,0,0},
  {1,0,0,0,0,0,0,0,0,0},
  {2,0,0,0,0,0,0,0,0,0}, // 10
  {2,0,0,0,0,0,0,0,0,0},
  {2,0,0,0,0,0,0,0,0,0},
  {2,1,0,0,0,0,0,0,0,0},
  {2,1,0,0,0,0,0,0,0,0},
  {2,1,0,0,0,0,0,0,0,0},
  {2,2,0,0,0,0,0,0,0,0},
  {2,2,0,0,0,0,0,0,0,0},
  {2,2,0,0,0,0,0,0,0,0},
  {2,2,1,0,0,0,0,0,0,0},
  {2,2,1,0,0,0,0,0,0,0}, // 20
  {3,2,1,0,0,0,0,0,0,0},
  {3,2,1,0,0,0,0,0,0,0},
  {3,2,1,0,0,0,0,0,0,0},
  {3,2,2,0,0,0,0,0,0,0},
  {3,2,2,0,0,0,0,0,0,0},
  {3,2,2,0,0,0,0,0,0,0},
  {3,3,2,0,0,0,0,0,0,0},
  {3,3,2,0,0,0,0,0,0,0},
  {3,3,3,0,0,0,0,0,0,0}, // 30
  {9,9,9,9,9,9,9,9,9,9},
  {9,9,9,9,9,9,9,9,9,9},
  {9,9,9,9,9,9,9,9,9,9},
  {9,9,9,9,9,9,9,9,9,9}
};

int PALADINE_SLOT_VALUES[][MAX_SLOT] =
{ {0,0,0,0,0,0,0,0,0,0}, // 0
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {1,0,0,0,0,0,0,0,0,0},
  {1,0,0,0,0,0,0,0,0,0},
  {1,0,0,0,0,0,0,0,0,0},
  {1,0,0,0,0,0,0,0,0,0},
  {2,0,0,0,0,0,0,0,0,0},
  {2,0,0,0,0,0,0,0,0,0},
  {2,1,0,0,0,0,0,0,0,0}, // 10
  {2,1,0,0,0,0,0,0,0,0},
  {2,1,0,0,0,0,0,0,0,0},
  {2,2,0,0,0,0,0,0,0,0},
  {2,2,0,0,0,0,0,0,0,0},
  {2,2,1,0,0,0,0,0,0,0},
  {2,2,1,0,0,0,0,0,0,0},
  {2,2,1,0,0,0,0,0,0,0},
  {2,2,2,0,0,0,0,0,0,0},
  {2,2,2,0,0,0,0,0,0,0},
  {3,2,2,0,0,0,0,0,0,0}, // 20
  {3,2,2,0,0,0,0,0,0,0},
  {3,2,2,1,0,0,0,0,0,0},
  {3,2,2,1,0,0,0,0,0,0},
  {3,3,2,1,0,0,0,0,0,0},
  {3,3,3,1,0,0,0,0,0,0},
  {3,3,3,2,0,0,0,0,0,0},
  {3,3,3,2,0,0,0,0,0,0},
  {3,3,3,3,0,0,0,0,0,0},
  {3,3,3,3,0,0,0,0,0,0},
  {4,3,3,3,0,0,0,0,0,0}, // 30
  {9,9,9,9,9,9,9,9,9,9},
  {9,9,9,9,9,9,9,9,9,9},
  {9,9,9,9,9,9,9,9,9,9},
  {9,9,9,9,9,9,9,9,9,9} // 34
};

int BARD_SLOT_VALUES[][6] =
{ // Level 3 start
  {2,0,0,0,0,0},
  {2,1,0,0,0,0},
  {3,1,0,0,0,0},
  {3,2,0,0,0,0},
  {3,2,1,0,0,0},
  {3,3,1,0,0,0},
  {3,3,2,0,0,0},
  {3,3,2,1,0,0},
  {3,3,3,1,0,0},
  {3,3,3,2,0,0},
  {3,3,3,2,1,0},
  {3,3,3,3,1,0},
  {3,3,3,3,2,0},
  {4,3,3,3,2,1},
  {4,4,3,3,3,1},
  {4,4,4,3,3,2},
  {4,4,4,4,3,2},
  {4,4,4,4,4,3}
};

#define SLOT_LEVELS  30

const int MAG_SLOTS[][MAX_SLOT] =
{{2,0,0,0,0,0,0,0,0,0},// lvl 1
 {2,0,0,0,0,0,0,0,0,0},// lvl 2
 {3,0,0,0,0,0,0,0,0,0},// lvl 3
 {3,1,0,0,0,0,0,0,0,0},// lvl 4
 {3,2,0,0,0,0,0,0,0,0},// lvl 5
 {4,2,0,0,0,0,0,0,0,0},// lvl 6
 {4,2,1,0,0,0,0,0,0,0},// lvl 7
 {4,3,1,0,0,0,0,0,0,0},// lvl 8
 {4,3,2,0,0,0,0,0,0,0},// lvl 9
 {4,3,2,1,0,0,0,0,0,0},// lvl 10
 {5,3,2,1,0,0,0,0,0,0},// lvl 11
 {5,4,2,2,0,0,0,0,0,0},// lvl 12
 {5,4,3,2,0,0,0,0,0,0},// lvl 13
 {5,4,3,2,1,0,0,0,0,0},// lvl 14
 {5,4,3,3,1,0,0,0,0,0},// lvl 15
 {5,4,3,3,2,0,0,0,0,0},// lvl 16
 {5,5,4,3,2,0,0,0,0,0},// lvl 17
 {6,5,4,3,2,1,0,0,0,0},// lvl 18
 {6,5,4,4,3,1,0,0,0,0},// lvl 19
 {6,5,4,4,3,2,0,0,0,0},// lvl 20
 {6,5,5,4,3,2,0,0,0,0},// lvl 21
 {6,6,5,4,3,2,1,0,0,0},// lvl 22
 {6,6,5,4,4,3,1,0,0,0},// lvl 23
 {7,6,5,5,4,3,2,0,0,0},// lvl 24
 {7,6,6,5,4,3,2,0,0,0},// lvl 25
 {7,7,6,5,5,3,2,1,0,0},// lvl 26
 {7,7,6,5,5,4,3,1,0,0},// lvl 27
 {7,7,6,6,5,4,3,2,0,0},// lvl 28
 {8,7,6,6,5,4,3,2,1,0},// lvl 29
 {8,8,7,6,6,5,4,2,1,0},// lvl 30
};

#define MIN_CL_LEVEL 1
#define MAX_CL_LEVEL 30
#define MAX_CL_SLOT  8
#define MIN_CL_WIS   10
#define MAX_CL_WIS   35
#define CL_WIS_DIV   1

const int CLERIC_SLOTS[][MAX_CL_SLOT] =
{{1,0,0,0,0,0,0,0},// level 1 wisdom 10
 {1,0,0,0,0,0,0,0},// level 2 wisdom 10
 {1,0,0,0,0,0,0,0},// level 3 wisdom 10
 {2,1,0,0,0,0,0,0},// level 4 wisdom 10
 {2,1,0,0,0,0,0,0},// level 5 wisdom 10
 {2,1,0,0,0,0,0,0},// level 6 wisdom 10
 {2,1,1,0,0,0,0,0},// level 7 wisdom 10
 {2,2,1,0,0,0,0,0},// level 8 wisdom 10
 {2,2,1,0,0,0,0,0},// level 9 wisdom 10
 {3,2,1,1,0,0,0,0},// level 10 wisdom 10
 {3,2,2,1,0,0,0,0},// level 11 wisdom 10
 {3,2,2,1,0,0,0,0},// level 12 wisdom 10
 {3,2,2,1,0,0,0,0},// level 13 wisdom 10
 {3,3,2,2,1,0,0,0},// level 14 wisdom 10
 {3,3,2,2,1,0,0,0},// level 15 wisdom 10
 {4,3,2,2,1,0,0,0},// level 16 wisdom 10
 {4,3,2,2,1,0,0,0},// level 17 wisdom 10
 {4,3,2,2,1,1,0,0},// level 18 wisdom 10
 {4,3,3,2,2,1,0,0},// level 19 wisdom 10
 {4,3,3,2,2,1,0,0},// level 20 wisdom 10
 {4,4,3,3,2,1,0,0},// level 21 wisdom 10
 {5,4,3,3,2,1,0,0},// level 22 wisdom 10
 {5,4,3,3,2,1,1,0},// level 23 wisdom 10
 {5,4,3,3,2,1,1,0},// level 24 wisdom 10
 {5,4,3,3,2,2,1,0},// level 25 wisdom 10
 {5,4,3,3,2,2,1,0},// level 26 wisdom 10
 {5,5,4,4,3,2,1,0},// level 27 wisdom 10
 {6,5,4,4,3,2,1,1},// level 28 wisdom 10
 {6,5,4,4,3,2,1,1},// level 29 wisdom 10
 {6,5,4,4,3,2,1,1},// level 30 wisdom 10
 {1,0,0,0,0,0,0,0},// level 1 wisdom 11
 {1,0,0,0,0,0,0,0},// level 2 wisdom 11
 {2,0,0,0,0,0,0,0},// level 3 wisdom 11
 {2,1,0,0,0,0,0,0},// level 4 wisdom 11
 {2,1,0,0,0,0,0,0},// level 5 wisdom 11
 {2,1,0,0,0,0,0,0},// level 6 wisdom 11
 {2,2,1,0,0,0,0,0},// level 7 wisdom 11
 {2,2,1,0,0,0,0,0},// level 8 wisdom 11
 {3,2,1,0,0,0,0,0},// level 9 wisdom 11
 {3,2,2,1,0,0,0,0},// level 10 wisdom 11
 {3,2,2,1,0,0,0,0},// level 11 wisdom 11
 {3,2,2,1,0,0,0,0},// level 12 wisdom 11
 {3,3,2,2,0,0,0,0},// level 13 wisdom 11
 {3,3,2,2,1,0,0,0},// level 14 wisdom 11
 {4,3,2,2,1,0,0,0},// level 15 wisdom 11
 {4,3,2,2,1,0,0,0},// level 16 wisdom 11
 {4,3,3,2,2,0,0,0},// level 17 wisdom 11
 {4,3,3,2,2,1,0,0},// level 18 wisdom 11
 {4,4,3,3,2,1,0,0},// level 19 wisdom 11
 {5,4,3,3,2,1,0,0},// level 20 wisdom 11
 {5,4,3,3,2,1,0,0},// level 21 wisdom 11
 {5,4,3,3,2,1,0,0},// level 22 wisdom 11
 {5,4,3,3,2,2,1,0},// level 23 wisdom 11
 {5,4,4,3,2,2,1,0},// level 24 wisdom 11
 {5,5,4,4,3,2,1,0},// level 25 wisdom 11
 {6,5,4,4,3,2,1,0},// level 26 wisdom 11
 {6,5,4,4,3,2,1,0},// level 27 wisdom 11
 {6,5,4,4,3,2,1,1},// level 28 wisdom 11
 {6,5,4,4,3,2,1,1},// level 29 wisdom 11
 {6,5,4,4,3,2,1,1},// level 30 wisdom 11
 {1,0,0,0,0,0,0,0},// level 1 wisdom 12
 {2,0,0,0,0,0,0,0},// level 2 wisdom 12
 {2,0,0,0,0,0,0,0},// level 3 wisdom 12
 {2,1,0,0,0,0,0,0},// level 4 wisdom 12
 {2,1,0,0,0,0,0,0},// level 5 wisdom 12
 {2,2,0,0,0,0,0,0},// level 6 wisdom 12
 {2,2,1,0,0,0,0,0},// level 7 wisdom 12
 {3,2,1,0,0,0,0,0},// level 8 wisdom 12
 {3,2,2,0,0,0,0,0},// level 9 wisdom 12
 {3,2,2,1,0,0,0,0},// level 10 wisdom 12
 {3,2,2,1,0,0,0,0},// level 11 wisdom 12
 {3,3,2,2,0,0,0,0},// level 12 wisdom 12
 {4,3,2,2,0,0,0,0},// level 13 wisdom 12
 {4,3,2,2,1,0,0,0},// level 14 wisdom 12
 {4,3,3,2,1,0,0,0},// level 15 wisdom 12
 {4,3,3,2,2,0,0,0},// level 16 wisdom 12
 {4,3,3,2,2,0,0,0},// level 17 wisdom 12
 {4,4,3,3,2,1,0,0},// level 18 wisdom 12
 {5,4,3,3,2,1,0,0},// level 19 wisdom 12
 {5,4,3,3,2,1,0,0},// level 20 wisdom 12
 {5,4,3,3,2,2,0,0},// level 21 wisdom 12
 {5,4,4,3,2,2,0,0},// level 22 wisdom 12
 {5,4,4,3,3,2,1,0},// level 23 wisdom 12
 {6,5,4,4,3,2,1,0},// level 24 wisdom 12
 {6,5,4,4,3,2,1,0},// level 25 wisdom 12
 {6,5,4,4,3,2,1,0},// level 26 wisdom 12
 {6,5,4,4,3,2,1,0},// level 27 wisdom 12
 {6,5,4,4,3,2,1,1},// level 28 wisdom 12
 {6,5,5,4,3,2,1,1},// level 29 wisdom 12
 {7,6,5,5,4,3,1,1},// level 30 wisdom 12
 {2,0,0,0,0,0,0,0},// level 1 wisdom 13
 {2,0,0,0,0,0,0,0},// level 2 wisdom 13
 {2,0,0,0,0,0,0,0},// level 3 wisdom 13
 {2,1,0,0,0,0,0,0},// level 4 wisdom 13
 {2,2,0,0,0,0,0,0},// level 5 wisdom 13
 {3,2,0,0,0,0,0,0},// level 6 wisdom 13
 {3,2,1,0,0,0,0,0},// level 7 wisdom 13
 {3,2,2,0,0,0,0,0},// level 8 wisdom 13
 {3,2,2,0,0,0,0,0},// level 9 wisdom 13
 {3,3,2,1,0,0,0,0},// level 10 wisdom 13
 {3,3,2,2,0,0,0,0},// level 11 wisdom 13
 {4,3,2,2,0,0,0,0},// level 12 wisdom 13
 {4,3,2,2,0,0,0,0},// level 13 wisdom 13
 {4,3,3,2,1,0,0,0},// level 14 wisdom 13
 {4,3,3,2,2,0,0,0},// level 15 wisdom 13
 {4,4,3,2,2,0,0,0},// level 16 wisdom 13
 {5,4,3,3,2,0,0,0},// level 17 wisdom 13
 {5,4,3,3,2,1,0,0},// level 18 wisdom 13
 {5,4,3,3,2,1,0,0},// level 19 wisdom 13
 {5,4,4,3,2,2,0,0},// level 20 wisdom 13
 {5,4,4,3,2,2,0,0},// level 21 wisdom 13
 {5,5,4,4,3,2,0,0},// level 22 wisdom 13
 {6,5,4,4,3,2,1,0},// level 23 wisdom 13
 {6,5,4,4,3,2,1,0},// level 24 wisdom 13
 {6,5,4,4,3,2,1,0},// level 25 wisdom 13
 {6,5,4,4,3,2,1,0},// level 26 wisdom 13
 {6,5,5,4,3,2,2,0},// level 27 wisdom 13
 {7,6,5,5,4,3,2,1},// level 28 wisdom 13
 {7,6,5,5,4,3,2,1},// level 29 wisdom 13
 {7,6,5,5,4,3,2,1},// level 30 wisdom 13
 {2,0,0,0,0,0,0,0},// level 1 wisdom 14
 {2,0,0,0,0,0,0,0},// level 2 wisdom 14
 {2,0,0,0,0,0,0,0},// level 3 wisdom 14
 {2,2,0,0,0,0,0,0},// level 4 wisdom 14
 {3,2,0,0,0,0,0,0},// level 5 wisdom 14
 {3,2,0,0,0,0,0,0},// level 6 wisdom 14
 {3,2,2,0,0,0,0,0},// level 7 wisdom 14
 {3,2,2,0,0,0,0,0},// level 8 wisdom 14
 {3,3,2,0,0,0,0,0},// level 9 wisdom 14
 {4,3,2,1,0,0,0,0},// level 10 wisdom 14
 {4,3,2,2,0,0,0,0},// level 11 wisdom 14
 {4,3,2,2,0,0,0,0},// level 12 wisdom 14
 {4,3,3,2,0,0,0,0},// level 13 wisdom 14
 {4,3,3,2,1,0,0,0},// level 14 wisdom 14
 {4,4,3,2,2,0,0,0},// level 15 wisdom 14
 {5,4,3,3,2,0,0,0},// level 16 wisdom 14
 {5,4,3,3,2,0,0,0},// level 17 wisdom 14
 {5,4,3,3,2,1,0,0},// level 18 wisdom 14
 {5,4,4,3,2,1,0,0},// level 19 wisdom 14
 {5,4,4,3,3,2,0,0},// level 20 wisdom 14
 {6,5,4,4,3,2,0,0},// level 21 wisdom 14
 {6,5,4,4,3,2,0,0},// level 22 wisdom 14
 {6,5,4,4,3,2,1,0},// level 23 wisdom 14
 {6,5,4,4,3,2,1,0},// level 24 wisdom 14
 {6,5,5,4,3,2,2,0},// level 25 wisdom 14
 {7,6,5,5,4,3,2,0},// level 26 wisdom 14
 {7,6,5,5,4,3,2,0},// level 27 wisdom 14
 {7,6,5,5,4,3,2,1},// level 28 wisdom 14
 {7,6,5,5,4,3,2,1},// level 29 wisdom 14
 {7,6,5,5,4,3,2,2},// level 30 wisdom 14
 {2,0,0,0,0,0,0,0},// level 1 wisdom 15
 {2,0,0,0,0,0,0,0},// level 2 wisdom 15
 {2,0,0,0,0,0,0,0},// level 3 wisdom 15
 {3,2,0,0,0,0,0,0},// level 4 wisdom 15
 {3,2,0,0,0,0,0,0},// level 5 wisdom 15
 {3,2,0,0,0,0,0,0},// level 6 wisdom 15
 {3,2,2,0,0,0,0,0},// level 7 wisdom 15
 {3,3,2,0,0,0,0,0},// level 8 wisdom 15
 {4,3,2,0,0,0,0,0},// level 9 wisdom 15
 {4,3,2,2,0,0,0,0},// level 10 wisdom 15
 {4,3,2,2,0,0,0,0},// level 11 wisdom 15
 {4,3,3,2,0,0,0,0},// level 12 wisdom 15
 {4,3,3,2,0,0,0,0},// level 13 wisdom 15
 {5,4,3,2,2,0,0,0},// level 14 wisdom 15
 {5,4,3,3,2,0,0,0},// level 15 wisdom 15
 {5,4,3,3,2,0,0,0},// level 16 wisdom 15
 {5,4,4,3,2,0,0,0},// level 17 wisdom 15
 {5,4,4,3,2,1,0,0},// level 18 wisdom 15
 {5,5,4,3,3,2,0,0},// level 19 wisdom 15
 {6,5,4,4,3,2,0,0},// level 20 wisdom 15
 {6,5,4,4,3,2,0,0},// level 21 wisdom 15
 {6,5,4,4,3,2,0,0},// level 22 wisdom 15
 {6,5,5,4,3,2,1,0},// level 23 wisdom 15
 {6,5,5,4,3,2,2,0},// level 24 wisdom 15
 {7,6,5,5,4,3,2,0},// level 25 wisdom 15
 {7,6,5,5,4,3,2,0},// level 26 wisdom 15
 {7,6,5,5,4,3,2,0},// level 27 wisdom 15
 {7,6,5,5,4,3,2,1},// level 28 wisdom 15
 {7,6,6,5,4,3,2,1},// level 29 wisdom 15
 {8,7,6,6,5,3,2,2},// level 30 wisdom 15
 {2,0,0,0,0,0,0,0},// level 1 wisdom 16
 {2,0,0,0,0,0,0,0},// level 2 wisdom 16
 {3,0,0,0,0,0,0,0},// level 3 wisdom 16
 {3,2,0,0,0,0,0,0},// level 4 wisdom 16
 {3,2,0,0,0,0,0,0},// level 5 wisdom 16
 {3,2,0,0,0,0,0,0},// level 6 wisdom 16
 {3,3,2,0,0,0,0,0},// level 7 wisdom 16
 {4,3,2,0,0,0,0,0},// level 8 wisdom 16
 {4,3,2,0,0,0,0,0},// level 9 wisdom 16
 {4,3,3,2,0,0,0,0},// level 10 wisdom 16
 {4,3,3,2,0,0,0,0},// level 11 wisdom 16
 {4,3,3,2,0,0,0,0},// level 12 wisdom 16
 {5,4,3,2,0,0,0,0},// level 13 wisdom 16
 {5,4,3,3,2,0,0,0},// level 14 wisdom 16
 {5,4,3,3,2,0,0,0},// level 15 wisdom 16
 {5,4,4,3,2,0,0,0},// level 16 wisdom 16
 {5,4,4,3,2,0,0,0},// level 17 wisdom 16
 {6,5,4,3,3,1,0,0},// level 18 wisdom 16
 {6,5,4,4,3,2,0,0},// level 19 wisdom 16
 {6,5,4,4,3,2,0,0},// level 20 wisdom 16
 {6,5,5,4,3,2,0,0},// level 21 wisdom 16
 {6,5,5,4,3,2,0,0},// level 22 wisdom 16
 {7,6,5,4,4,2,1,0},// level 23 wisdom 16
 {7,6,5,5,4,3,2,0},// level 24 wisdom 16
 {7,6,5,5,4,3,2,0},// level 25 wisdom 16
 {7,6,5,5,4,3,2,0},// level 26 wisdom 16
 {7,6,6,5,4,3,2,0},// level 27 wisdom 16
 {8,7,6,5,5,3,2,1},// level 28 wisdom 16
 {8,7,6,6,5,3,2,2},// level 29 wisdom 16
 {8,7,6,6,5,4,2,2},// level 30 wisdom 16
 {2,0,0,0,0,0,0,0},// level 1 wisdom 17
 {3,0,0,0,0,0,0,0},// level 2 wisdom 17
 {3,0,0,0,0,0,0,0},// level 3 wisdom 17
 {3,2,0,0,0,0,0,0},// level 4 wisdom 17
 {3,2,0,0,0,0,0,0},// level 5 wisdom 17
 {3,3,0,0,0,0,0,0},// level 6 wisdom 17
 {4,3,2,0,0,0,0,0},// level 7 wisdom 17
 {4,3,2,0,0,0,0,0},// level 8 wisdom 17
 {4,3,3,0,0,0,0,0},// level 9 wisdom 17
 {4,3,3,2,0,0,0,0},// level 10 wisdom 17
 {4,3,3,2,0,0,0,0},// level 11 wisdom 17
 {5,4,3,2,0,0,0,0},// level 12 wisdom 17
 {5,4,3,2,0,0,0,0},// level 13 wisdom 17
 {5,4,3,3,2,0,0,0},// level 14 wisdom 17
 {5,4,4,3,2,0,0,0},// level 15 wisdom 17
 {5,4,4,3,2,0,0,0},// level 16 wisdom 17
 {6,5,4,3,2,0,0,0},// level 17 wisdom 17
 {6,5,4,4,3,2,0,0},// level 18 wisdom 17
 {6,5,4,4,3,2,0,0},// level 19 wisdom 17
 {6,5,5,4,3,2,0,0},// level 20 wisdom 17
 {6,5,5,4,3,2,0,0},// level 21 wisdom 17
 {7,6,5,4,4,2,0,0},// level 22 wisdom 17
 {7,6,5,5,4,3,2,0},// level 23 wisdom 17
 {7,6,5,5,4,3,2,0},// level 24 wisdom 17
 {7,6,6,5,4,3,2,0},// level 25 wisdom 17
 {7,6,6,5,4,3,2,0},// level 26 wisdom 17
 {8,7,6,6,5,3,2,0},// level 27 wisdom 17
 {8,7,6,6,5,4,2,1},// level 28 wisdom 17
 {8,7,6,6,5,4,3,2},// level 29 wisdom 17
 {8,7,7,6,5,4,3,2},// level 30 wisdom 17
 {3,0,0,0,0,0,0,0},// level 1 wisdom 18
 {3,0,0,0,0,0,0,0},// level 2 wisdom 18
 {3,0,0,0,0,0,0,0},// level 3 wisdom 18
 {3,2,0,0,0,0,0,0},// level 4 wisdom 18
 {3,2,0,0,0,0,0,0},// level 5 wisdom 18
 {4,3,0,0,0,0,0,0},// level 6 wisdom 18
 {4,3,2,0,0,0,0,0},// level 7 wisdom 18
 {4,3,2,0,0,0,0,0},// level 8 wisdom 18
 {4,3,3,0,0,0,0,0},// level 9 wisdom 18
 {4,3,3,2,0,0,0,0},// level 10 wisdom 18
 {5,4,3,2,0,0,0,0},// level 11 wisdom 18
 {5,4,3,2,0,0,0,0},// level 12 wisdom 18
 {5,4,3,3,0,0,0,0},// level 13 wisdom 18
 {5,4,4,3,2,0,0,0},// level 14 wisdom 18
 {5,5,4,3,2,0,0,0},// level 15 wisdom 18
 {6,5,4,3,2,0,0,0},// level 16 wisdom 18
 {6,5,4,4,3,0,0,0},// level 17 wisdom 18
 {6,5,4,4,3,2,0,0},// level 18 wisdom 18
 {6,5,5,4,3,2,0,0},// level 19 wisdom 18
 {7,6,5,4,3,2,0,0},// level 20 wisdom 18
 {7,6,5,4,4,2,0,0},// level 21 wisdom 18
 {7,6,5,5,4,3,0,0},// level 22 wisdom 18
 {7,6,5,5,4,3,2,0},// level 23 wisdom 18
 {7,6,6,5,4,3,2,0},// level 24 wisdom 18
 {8,7,6,5,4,3,2,0},// level 25 wisdom 18
 {8,7,6,6,5,3,2,0},// level 26 wisdom 18
 {8,7,6,6,5,4,2,0},// level 27 wisdom 18
 {8,7,6,6,5,4,3,1},// level 28 wisdom 18
 {8,7,7,6,5,4,3,2},// level 29 wisdom 18
 {9,8,7,7,6,4,3,2},// level 30 wisdom 18
 {3,0,0,0,0,0,0,0},// level 1 wisdom 19
 {3,0,0,0,0,0,0,0},// level 2 wisdom 19
 {3,0,0,0,0,0,0,0},// level 3 wisdom 19
 {3,2,0,0,0,0,0,0},// level 4 wisdom 19
 {4,3,0,0,0,0,0,0},// level 5 wisdom 19
 {4,3,0,0,0,0,0,0},// level 6 wisdom 19
 {4,3,2,0,0,0,0,0},// level 7 wisdom 19
 {4,3,3,0,0,0,0,0},// level 8 wisdom 19
 {4,3,3,0,0,0,0,0},// level 9 wisdom 19
 {5,4,3,2,0,0,0,0},// level 10 wisdom 19
 {5,4,3,2,0,0,0,0},// level 11 wisdom 19
 {5,4,3,3,0,0,0,0},// level 12 wisdom 19
 {5,4,4,3,0,0,0,0},// level 13 wisdom 19
 {6,5,4,3,2,0,0,0},// level 14 wisdom 19
 {6,5,4,3,2,0,0,0},// level 15 wisdom 19
 {6,5,4,4,3,0,0,0},// level 16 wisdom 19
 {6,5,5,4,3,0,0,0},// level 17 wisdom 19
 {6,5,5,4,3,2,0,0},// level 18 wisdom 19
 {7,6,5,4,3,2,0,0},// level 19 wisdom 19
 {7,6,5,4,4,2,0,0},// level 20 wisdom 19
 {7,6,5,5,4,2,0,0},// level 21 wisdom 19
 {7,6,6,5,4,3,0,0},// level 22 wisdom 19
 {7,6,6,5,4,3,2,0},// level 23 wisdom 19
 {8,7,6,5,4,3,2,0},// level 24 wisdom 19
 {8,7,6,6,5,3,2,0},// level 25 wisdom 19
 {8,7,6,6,5,4,2,0},// level 26 wisdom 19
 {8,7,7,6,5,4,3,0},// level 27 wisdom 19
 {8,7,7,6,5,4,3,1},// level 28 wisdom 19
 {9,8,7,7,6,4,3,2},// level 29 wisdom 19
 {9,8,7,7,6,5,3,2},// level 30 wisdom 19
 {3,0,0,0,0,0,0,0},// level 1 wisdom 20
 {3,0,0,0,0,0,0,0},// level 2 wisdom 20
 {3,0,0,0,0,0,0,0},// level 3 wisdom 20
 {4,3,0,0,0,0,0,0},// level 4 wisdom 20
 {4,3,0,0,0,0,0,0},// level 5 wisdom 20
 {4,3,0,0,0,0,0,0},// level 6 wisdom 20
 {4,3,3,0,0,0,0,0},// level 7 wisdom 20
 {4,3,3,0,0,0,0,0},// level 8 wisdom 20
 {5,4,3,0,0,0,0,0},// level 9 wisdom 20
 {5,4,3,2,0,0,0,0},// level 10 wisdom 20
 {5,4,3,2,0,0,0,0},// level 11 wisdom 20
 {5,4,4,3,0,0,0,0},// level 12 wisdom 20
 {6,5,4,3,0,0,0,0},// level 13 wisdom 20
 {6,5,4,3,2,0,0,0},// level 14 wisdom 20
 {6,5,4,3,2,0,0,0},// level 15 wisdom 20
 {6,5,5,4,3,0,0,0},// level 16 wisdom 20
 {6,5,5,4,3,0,0,0},// level 17 wisdom 20
 {7,6,5,4,3,2,0,0},// level 18 wisdom 20
 {7,6,5,4,3,2,0,0},// level 19 wisdom 20
 {7,6,5,5,4,2,0,0},// level 20 wisdom 20
 {7,6,6,5,4,3,0,0},// level 21 wisdom 20
 {7,6,6,5,4,3,0,0},// level 22 wisdom 20
 {8,7,6,5,4,3,2,0},// level 23 wisdom 20
 {8,7,6,6,5,3,2,0},// level 24 wisdom 20
 {8,7,7,6,5,4,2,0},// level 25 wisdom 20
 {8,7,7,6,5,4,2,0},// level 26 wisdom 20
 {9,8,7,6,5,4,3,0},// level 27 wisdom 20
 {9,8,7,7,6,4,3,1},// level 28 wisdom 20
 {9,8,7,7,6,5,3,2},// level 29 wisdom 20
 {9,8,8,7,6,5,3,3},// level 30 wisdom 20
 {3,0,0,0,0,0,0,0},// level 1 wisdom 21
 {3,0,0,0,0,0,0,0},// level 2 wisdom 21
 {4,0,0,0,0,0,0,0},// level 3 wisdom 21
 {4,3,0,0,0,0,0,0},// level 4 wisdom 21
 {4,3,0,0,0,0,0,0},// level 5 wisdom 21
 {4,3,0,0,0,0,0,0},// level 6 wisdom 21
 {5,3,3,0,0,0,0,0},// level 7 wisdom 21
 {5,4,3,0,0,0,0,0},// level 8 wisdom 21
 {5,4,3,0,0,0,0,0},// level 9 wisdom 21
 {5,4,3,2,0,0,0,0},// level 10 wisdom 21
 {5,4,4,3,0,0,0,0},// level 11 wisdom 21
 {6,5,4,3,0,0,0,0},// level 12 wisdom 21
 {6,5,4,3,0,0,0,0},// level 13 wisdom 21
 {6,5,4,3,2,0,0,0},// level 14 wisdom 21
 {6,5,5,4,3,0,0,0},// level 15 wisdom 21
 {6,5,5,4,3,0,0,0},// level 16 wisdom 21
 {7,6,5,4,3,0,0,0},// level 17 wisdom 21
 {7,6,5,4,3,2,0,0},// level 18 wisdom 21
 {7,6,5,5,4,2,0,0},// level 19 wisdom 21
 {7,6,6,5,4,2,0,0},// level 20 wisdom 21
 {8,7,6,5,4,3,0,0},// level 21 wisdom 21
 {8,7,6,5,4,3,0,0},// level 22 wisdom 21
 {8,7,6,6,5,3,2,0},// level 23 wisdom 21
 {8,7,7,6,5,3,2,0},// level 24 wisdom 21
 {8,7,7,6,5,4,2,0},// level 25 wisdom 21
 {9,8,7,6,5,4,3,0},// level 26 wisdom 21
 {9,8,7,7,6,4,3,0},// level 27 wisdom 21
 {9,8,8,7,6,5,3,1},// level 28 wisdom 21
 {9,8,8,7,6,5,3,2},// level 29 wisdom 21
 {10,9,8,8,7,5,4,3},// level 30 wisdom 21
 {3,0,0,0,0,0,0,0},// level 1 wisdom 22
 {4,0,0,0,0,0,0,0},// level 2 wisdom 22
 {4,0,0,0,0,0,0,0},// level 3 wisdom 22
 {4,3,0,0,0,0,0,0},// level 4 wisdom 22
 {4,3,0,0,0,0,0,0},// level 5 wisdom 22
 {5,3,0,0,0,0,0,0},// level 6 wisdom 22
 {5,4,3,0,0,0,0,0},// level 7 wisdom 22
 {5,4,3,0,0,0,0,0},// level 8 wisdom 22
 {5,4,3,0,0,0,0,0},// level 9 wisdom 22
 {5,4,4,2,0,0,0,0},// level 10 wisdom 22
 {6,5,4,3,0,0,0,0},// level 11 wisdom 22
 {6,5,4,3,0,0,0,0},// level 12 wisdom 22
 {6,5,4,3,0,0,0,0},// level 13 wisdom 22
 {6,5,5,4,2,0,0,0},// level 14 wisdom 22
 {7,5,5,4,3,0,0,0},// level 15 wisdom 22
 {7,6,5,4,3,0,0,0},// level 16 wisdom 22
 {7,6,5,4,3,0,0,0},// level 17 wisdom 22
 {7,6,6,5,4,2,0,0},// level 18 wisdom 22
 {7,6,6,5,4,2,0,0},// level 19 wisdom 22
 {8,7,6,5,4,3,0,0},// level 20 wisdom 22
 {8,7,6,5,4,3,0,0},// level 21 wisdom 22
 {8,7,6,6,5,3,0,0},// level 22 wisdom 22
 {8,7,7,6,5,3,2,0},// level 23 wisdom 22
 {9,7,7,6,5,4,2,0},// level 24 wisdom 22
 {9,8,7,6,5,4,3,0},// level 25 wisdom 22
 {9,8,7,7,6,4,3,0},// level 26 wisdom 22
 {9,8,8,7,6,5,3,0},// level 27 wisdom 22
 {9,8,8,7,6,5,3,1},// level 28 wisdom 22
 {10,9,8,8,7,5,4,2},// level 29 wisdom 22
 {10,9,8,8,7,5,4,3},// level 30 wisdom 22
 {4,0,0,0,0,0,0,0},// level 1 wisdom 23
 {4,0,0,0,0,0,0,0},// level 2 wisdom 23
 {4,0,0,0,0,0,0,0},// level 3 wisdom 23
 {4,3,0,0,0,0,0,0},// level 4 wisdom 23
 {5,3,0,0,0,0,0,0},// level 5 wisdom 23
 {5,4,0,0,0,0,0,0},// level 6 wisdom 23
 {5,4,3,0,0,0,0,0},// level 7 wisdom 23
 {5,4,3,0,0,0,0,0},// level 8 wisdom 23
 {5,4,4,0,0,0,0,0},// level 9 wisdom 23
 {6,4,4,3,0,0,0,0},// level 10 wisdom 23
 {6,5,4,3,0,0,0,0},// level 11 wisdom 23
 {6,5,4,3,0,0,0,0},// level 12 wisdom 23
 {6,5,5,3,0,0,0,0},// level 13 wisdom 23
 {7,5,5,4,3,0,0,0},// level 14 wisdom 23
 {7,6,5,4,3,0,0,0},// level 15 wisdom 23
 {7,6,5,4,3,0,0,0},// level 16 wisdom 23
 {7,6,6,5,3,0,0,0},// level 17 wisdom 23
 {7,6,6,5,4,2,0,0},// level 18 wisdom 23
 {8,7,6,5,4,2,0,0},// level 19 wisdom 23
 {8,7,6,5,4,3,0,0},// level 20 wisdom 23
 {8,7,6,6,5,3,0,0},// level 21 wisdom 23
 {8,7,7,6,5,3,0,0},// level 22 wisdom 23
 {9,8,7,6,5,4,2,0},// level 23 wisdom 23
 {9,8,7,6,5,4,2,0},// level 24 wisdom 23
 {9,8,7,7,6,4,3,0},// level 25 wisdom 23
 {9,8,8,7,6,4,3,0},// level 26 wisdom 23
 {9,8,8,7,6,5,3,0},// level 27 wisdom 23
 {10,9,8,8,7,5,4,2},// level 28 wisdom 23
 {10,9,8,8,7,5,4,2},// level 29 wisdom 23
 {10,9,9,8,7,6,4,3},// level 30 wisdom 23
 {4,0,0,0,0,0,0,0},// level 1 wisdom 24
 {4,0,0,0,0,0,0,0},// level 2 wisdom 24
 {4,0,0,0,0,0,0,0},// level 3 wisdom 24
 {4,3,0,0,0,0,0,0},// level 4 wisdom 24
 {5,3,0,0,0,0,0,0},// level 5 wisdom 24
 {5,4,0,0,0,0,0,0},// level 6 wisdom 24
 {5,4,3,0,0,0,0,0},// level 7 wisdom 24
 {5,4,3,0,0,0,0,0},// level 8 wisdom 24
 {6,4,4,0,0,0,0,0},// level 9 wisdom 24
 {6,5,4,3,0,0,0,0},// level 10 wisdom 24
 {6,5,4,3,0,0,0,0},// level 11 wisdom 24
 {6,5,5,3,0,0,0,0},// level 12 wisdom 24
 {7,5,5,4,0,0,0,0},// level 13 wisdom 24
 {7,6,5,4,3,0,0,0},// level 14 wisdom 24
 {7,6,5,4,3,0,0,0},// level 15 wisdom 24
 {7,6,6,4,3,0,0,0},// level 16 wisdom 24
 {7,6,6,5,4,0,0,0},// level 17 wisdom 24
 {8,7,6,5,4,2,0,0},// level 18 wisdom 24
 {8,7,6,5,4,2,0,0},// level 19 wisdom 24
 {8,7,7,6,4,3,0,0},// level 20 wisdom 24
 {8,7,7,6,5,3,0,0},// level 21 wisdom 24
 {9,8,7,6,5,3,0,0},// level 22 wisdom 24
 {9,8,7,6,5,4,2,0},// level 23 wisdom 24
 {9,8,8,7,6,4,2,0},// level 24 wisdom 24
 {9,8,8,7,6,4,3,0},// level 25 wisdom 24
 {10,9,8,7,6,5,3,0},// level 26 wisdom 24
 {10,9,8,8,7,5,3,0},// level 27 wisdom 24
 {10,9,9,8,7,5,4,2},// level 28 wisdom 24
 {10,9,9,8,7,6,4,2},// level 29 wisdom 24
 {10,9,9,8,7,6,4,3},// level 30 wisdom 24
 {4,0,0,0,0,0,0,0} ,//level 1 wisdom 25
 {4,0,0,0,0,0,0,0} ,//level 2 wisdom 25
 {4,0,0,0,0,0,0,0} ,//level 3 wisdom 25
 {5,3,0,0,0,0,0,0} ,//level 4 wisdom 25
 {5,4,0,0,0,0,0,0} ,//level 5 wisdom 25
 {5,4,0,0,0,0,0,0} ,//level 6 wisdom 25
 {5,4,3,0,0,0,0,0} ,//level 7 wisdom 25
 {6,4,4,0,0,0,0,0} ,//level 8 wisdom 25
 {6,5,4,0,0,0,0,0} ,//level 9 wisdom 25
 {6,5,4,3,0,0,0,0} ,//level 10 wisdom 25
 {6,5,4,3,0,0,0,0} ,//level 11 wisdom 25
 {7,5,5,3,0,0,0,0} ,//level 12 wisdom 25
 {7,6,5,4,0,0,0,0} ,//level 13 wisdom 25
 {7,6,5,4,3,0,0,0} ,//level 14 wisdom 25
 {7,6,5,4,3,0,0,0} ,//level 15 wisdom 25
 {8,6,6,5,3,0,0,0},// level 16 wisdom 25
 {8,7,6,5,4,0,0,0},// level 17 wisdom 25
 {8,7,6,5,4,2,0,0},// level 18 wisdom 25
 {8,7,7,5,4,3,0,0},// level 19 wisdom 25
 {8,7,7,6,5,3,0,0},// level 20 wisdom 25
 {9,8,7,6,5,3,0,0},// level 21 wisdom 25
 {9,8,7,6,5,4,0,0},// level 22 wisdom 25
 {9,8,8,7,6,4,2,0},// level 23 wisdom 25
 {9,8,8,7,6,4,3,0},// level 24 wisdom 25
 {10,9,8,7,6,5,3,0},// level 25 wisdom 25
 {10,9,8,8,7,5,3,0},// level 26 wisdom 25
 {10,9,9,8,7,5,4,0},// level 27 wisdom 25
 {10,9,9,8,7,6,4,2},// level 28 wisdom 25
 {11,10,9,8,7,6,4,2},// level 29 wisdom 25
 {11,10,9,9,8,6,5,3},// level 30 wisdom 25
 {4,0,0,0,0,0,0,0},// level 1 wisdom 26
 {4,0,0,0,0,0,0,0},//level 2 wisdom 26
 {5,0,0,0,0,0,0,0},//level 3 wisdom 26
 {5,4,0,0,0,0,0,0},//level 4 wisdom 26
 {5,4,0,0,0,0,0,0},//level 5 wisdom 26
 {5,4,0,0,0,0,0,0},//level 6 wisdom 26
 {6,4,4,0,0,0,0,0},//level 7 wisdom 26
 {6,5,4,0,0,0,0,0},//level 8 wisdom 26
 {6,5,4,0,0,0,0,0} ,//level 9 wisdom 26
 {6,5,4,3,0,0,0,0} ,//level 10 wisdom 26
 {7,5,5,3,0,0,0,0} ,//level 11 wisdom 26
 {7,6,5,4,0,0,0,0} ,//level 12 wisdom 26
 {7,6,5,4,0,0,0,0} ,//level 13 wisdom 26
 {7,6,5,4,3,0,0,0} ,//level 14 wisdom 26
 {8,6,6,4,3,0,0,0},// level 15 wisdom 26
 {8,7,6,5,4,0,0,0},// level 16 wisdom 26
 {8,7,6,5,4,0,0,0},// level 17 wisdom 26
 {8,7,7,5,4,2,0,0},// level 18 wisdom 26
 {8,7,7,6,5,3,0,0},// level 19 wisdom 26
 {9,8,7,6,5,3,0,0},// level 20 wisdom 26
 {9,8,7,6,5,3,0,0},// level 21 wisdom 26
 {9,8,8,7,6,4,0,0},// level 22 wisdom 26
 {9,8,8,7,6,4,2,0},// level 23 wisdom 26
 {10,9,8,7,6,4,3,0},// level 24 wisdom 26
 {10,9,8,8,6,5,3,0},// level 25 wisdom 26
 {10,9,9,8,7,5,3,0},// level 26 wisdom 26
 {10,9,9,8,7,5,4,0},// level 27 wisdom 26
 {11,10,9,8,7,6,4,2},// level 28 wisdom 26
 {11,10,9,9,8,6,4,3},// level 29 wisdom 26
 {11,10,10,9,8,6,5,4},// level 30 wisdom 26
 {4,0,0,0,0,0,0,0} ,//level 1 wisdom 27
 {5,0,0,0,0,0,0,0} ,//level 2 wisdom 27
 {5,0,0,0,0,0,0,0} ,//level 3 wisdom 27
 {5,4,0,0,0,0,0,0} ,//level 4 wisdom 27
 {5,4,0,0,0,0,0,0} ,//level 5 wisdom 27
 {6,4,0,0,0,0,0,0} ,//level 6 wisdom 27
 {6,4,4,0,0,0,0,0} ,//level 7 wisdom 27
 {6,5,4,0,0,0,0,0} ,//level 8 wisdom 27
 {6,5,4,0,0,0,0,0} ,//level 9 wisdom 27
 {7,5,5,3,0,0,0,0} ,//level 10 wisdom 27
 {7,6,5,3,0,0,0,0} ,//level 11 wisdom 27
 {7,6,5,4,0,0,0,0} ,//level 12 wisdom 27
 {7,6,5,4,0,0,0,0} ,//level 13 wisdom 27
 {8,6,6,4,3,0,0,0} ,//level 14 wisdom 27
 {8,7,6,5,3,0,0,0} ,//level 15 wisdom 27
 {8,7,6,5,4,0,0,0} ,//level 16 wisdom 27
 {8,7,7,5,4,0,0,0} ,//level 17 wisdom 27
 {9,7,7,6,4,2,0,0} ,//level 18 wisdom 27
 {9,8,7,6,5,3,0,0} ,//level 19 wisdom 27
 {9,8,7,6,5,3,0,0} ,//level 20 wisdom 27
 {9,8,8,7,5,3,0,0} ,//level 21 wisdom 27
 {9,8,8,7,6,4,0,0} ,//level 22 wisdom 27
 {10,9,8,7,6,4,2,0},// level 23 wisdom 27
 {10,9,8,8,6,5,3,0},// level 24 wisdom 27
 {10,9,9,8,7,5,3,0},// level 25 wisdom 27
 {10,9,9,8,7,5,4,0},// level 26 wisdom 27
 {11,10,9,8,7,6,4,0},// level 27 wisdom 27
 {11,10,10,9,8,6,4,2},// level 28 wisdom 27
 {11,10,10,9,8,6,5,3},// level 29 wisdom 27
 {11,10,10,9,8,7,5,4},// level 30 wisdom 27
 {5,0,0,0,0,0,0,0} ,//level 1 wisdom 28
 {5,0,0,0,0,0,0,0} ,//level 2 wisdom 28
 {5,0,0,0,0,0,0,0} ,//level 3 wisdom 28
 {5,4,0,0,0,0,0,0} ,//level 4 wisdom 28
 {6,4,0,0,0,0,0,0} ,//level 5 wisdom 28
 {6,4,0,0,0,0,0,0} ,//level 6 wisdom 28
 {6,5,4,0,0,0,0,0} ,//level 7 wisdom 28
 {6,5,4,0,0,0,0,0} ,//level 8 wisdom 28
 {7,5,4,0,0,0,0,0} ,//level 9 wisdom 28
 {7,5,5,3,0,0,0,0} ,//level 10 wisdom 28
 {7,6,5,3,0,0,0,0} ,//level 11 wisdom 28
 {7,6,5,4,0,0,0,0} ,//level 12 wisdom 28
 {8,6,6,4,0,0,0,0} ,//level 13 wisdom 28
 {8,7,6,4,3,0,0,0} ,//level 14 wisdom 28
 {8,7,6,5,4,0,0,0} ,//level 15 wisdom 28
 {8,7,6,5,4,0,0,0} ,//level 16 wisdom 28
 {9,7,7,5,4,0,0,0} ,//level 17 wisdom 28
 {9,8,7,6,5,2,0,0} ,//level 18 wisdom 28
 {9,8,7,6,5,3,0,0} ,//level 19 wisdom 28
 {9,8,8,6,5,3,0,0} ,//level 20 wisdom 28
 {10,8,8,7,6,4,0,0},// level 21 wisdom 28
 {10,9,8,7,6,4,0,0},// level 22 wisdom 28
 {10,9,8,7,6,4,2,0},// level 23 wisdom 28
 {10,9,9,8,7,5,3,0},// level 24 wisdom 28
 {11,9,9,8,7,5,3,0},// level 25 wisdom 28
 {11,10,9,8,7,6,4,0},// level 26 wisdom 28
 {11,10,10,9,8,6,4,0},// level 27 wisdom 28
 {11,10,10,9,8,6,4,2},// level 28 wisdom 28
 {12,10,10,9,8,7,5,3},// level 29 wisdom 28
 {12,11,10,10,9,7,5,4},// level 30 wisdom 28
 {5,0,0,0,0,0,0,0} ,//level 1 wisdom 29
 {5,0,0,0,0,0,0,0} ,//level 2 wisdom 29
 {5,0,0,0,0,0,0,0} ,//level 3 wisdom 29
 {6,4,0,0,0,0,0,0} ,//level 4 wisdom 29
 {6,4,0,0,0,0,0,0} ,//level 5 wisdom 29
 {6,5,0,0,0,0,0,0} ,//level 6 wisdom 29
 {6,5,4,0,0,0,0,0} ,//level 7 wisdom 29
 {7,5,4,0,0,0,0,0} ,//level 8 wisdom 29
 {7,5,5,0,0,0,0,0} ,//level 9 wisdom 29
 {7,6,5,3,0,0,0,0} ,//level 10 wisdom 29
 {7,6,5,4,0,0,0,0} ,//level 11 wisdom 29
 {8,6,6,4,0,0,0,0} ,//level 12 wisdom 29
 {8,6,6,4,0,0,0,0} ,//level 13 wisdom 29
 {8,7,6,5,3,0,0,0} ,//level 14 wisdom 29
 {8,7,6,5,4,0,0,0} ,//level 15 wisdom 29
 {9,7,7,5,4,0,0,0} ,//level 16 wisdom 29
 {9,8,7,6,4,0,0,0} ,//level 17 wisdom 29
 {9,8,7,6,5,3,0,0} ,//level 18 wisdom 29
 {9,8,8,6,5,3,0,0} ,//level 19 wisdom 29
 {10,8,8,7,5,3,0,0},// level 20 wisdom 29
 {10,9,8,7,6,4,0,0},// level 21 wisdom 29
 {10,9,8,7,6,4,0,0},// level 22 wisdom 29
 {10,9,9,8,7,5,3,0},// level 23 wisdom 29
 {11,9,9,8,7,5,3,0},// level 24 wisdom 29
 {11,10,9,8,7,5,3,0},// level 25 wisdom 29
 {11,10,10,9,8,6,4,0},// level 26 wisdom 29
 {11,10,10,9,8,6,4,0},// level 27 wisdom 29
 {12,11,10,9,8,7,5,2},// level 28 wisdom 29
 {12,11,11,10,9,7,5,3},// level 29 wisdom 29
 {12,11,11,10,9,7,6,4},// level 30 wisdom 29
 {5,0,0,0,0,0,0,0} ,//level 1 wisdom 30
 {5,0,0,0,0,0,0,0} ,//level 2 wisdom 30
 {6,0,0,0,0,0,0,0} ,//level 3 wisdom 30
 {6,4,0,0,0,0,0,0} ,//level 4 wisdom 30
 {6,4,0,0,0,0,0,0} ,//level 5 wisdom 30
 {6,5,0,0,0,0,0,0} ,//level 6 wisdom 30
 {7,5,4,0,0,0,0,0} ,//level 7 wisdom 30
 {7,5,5,0,0,0,0,0} ,//level 8 wisdom 30
 {7,6,5,0,0,0,0,0} ,//level 9 wisdom 30
 {7,6,5,3,0,0,0,0} ,//level 10 wisdom 30
 {8,6,5,4,0,0,0,0} ,//level 11 wisdom 30
 {8,6,6,4,0,0,0,0} ,//level 12 wisdom 30
 {8,7,6,4,0,0,0,0} ,//level 13 wisdom 30
 {8,7,6,5,3,0,0,0} ,//level 14 wisdom 30
 {9,7,7,5,4,0,0,0} ,//level 15 wisdom 30
 {9,8,7,5,4,0,0,0} ,//level 16 wisdom 30
 {9,8,7,6,5,0,0,0} ,//level 17 wisdom 30
 {9,8,8,6,5,3,0,0} ,//level 18 wisdom 30
 {10,8,8,7,5,3,0,0},// level 19 wisdom 30
 {10,9,8,7,6,3,0,0},// level 20 wisdom 30
 {10,9,8,7,6,4,0,0},// level 21 wisdom 30
 {10,9,9,8,6,4,0,0},// level 22 wisdom 30
 {11,9,9,8,7,5,3,0},// level 23 wisdom 30
 {11,10,9,8,7,5,3,0},// level 24 wisdom 30
 {11,10,10,9,8,6,4,0},// level 25 wisdom 30
 {11,10,10,9,8,6,4,0},// level 26 wisdom 30
 {12,11,10,9,8,6,4,0},// level 27 wisdom 30
 {12,11,11,10,9,7,5,2},// level 28 wisdom 30
 {12,11,11,10,9,7,5,3},// level 29 wisdom 30
 {12,11,11,10,9,8,6,4},// level 30 wisdom 30
 {5,0,0,0,0,0,0,0} ,//level 1 wisdom 31
 {5,0,0,0,0,0,0,0} ,//level 2 wisdom 31
 {6,0,0,0,0,0,0,0} ,//level 3 wisdom 31
 {6,4,0,0,0,0,0,0} ,//level 4 wisdom 31
 {6,5,0,0,0,0,0,0} ,//level 5 wisdom 31
 {6,5,0,0,0,0,0,0} ,//level 6 wisdom 31
 {7,5,4,0,0,0,0,0} ,//level 7 wisdom 31
 {7,5,5,0,0,0,0,0} ,//level 8 wisdom 31
 {7,6,5,0,0,0,0,0} ,//level 9 wisdom 31
 {8,6,5,4,0,0,0,0} ,//level 10 wisdom 31
 {8,6,6,4,0,0,0,0} ,//level 11 wisdom 31
 {8,7,6,4,0,0,0,0} ,//level 12 wisdom 31
 {8,7,6,5,0,0,0,0} ,//level 13 wisdom 31
 {9,7,7,5,4,0,0,0} ,//level 14 wisdom 31
 {9,7,7,5,4,0,0,0} ,//level 15 wisdom 31
 {9,8,7,6,4,0,0,0} ,//level 16 wisdom 31
 {9,8,7,6,5,0,0,0} ,//level 17 wisdom 31
 {10,8,8,6,5,3,0,0},// level 18 wisdom 31
 {10,9,8,7,5,3,0,0},// level 19 wisdom 31
 {10,9,8,7,6,4,0,0},// level 20 wisdom 31
 {10,9,9,7,6,4,0,0},// level 21 wisdom 31
 {11,9,9,8,7,4,0,0},// level 22 wisdom 31
 {11,10,9,8,7,5,3,0},// level 23 wisdom 31
 {11,10,10,9,7,5,3,0},// level 24 wisdom 31
 {11,10,10,9,8,6,4,0},// level 25 wisdom 31
 {12,11,10,9,8,6,4,0},// level 26 wisdom 31
 {12,11,11,10,9,7,5,0},// level 27 wisdom 31
 {12,11,11,10,9,7,5,2},// level 28 wisdom 31
 {12,11,11,10,9,7,6,3},// level 29 wisdom 31
 {13,12,12,11,10,8,6,4},// level 30 wisdom 31
 {5,0,0,0,0,0,0,0} ,//level 1 wisdom 32
 {6,0,0,0,0,0,0,0} ,//level 2 wisdom 32
 {6,0,0,0,0,0,0,0} ,//level 3 wisdom 32
 {6,5,0,0,0,0,0,0} ,//level 4 wisdom 32
 {6,5,0,0,0,0,0,0} ,//level 5 wisdom 32
 {7,5,0,0,0,0,0,0} ,//level 6 wisdom 32
 {7,5,5,0,0,0,0,0} ,//level 7 wisdom 32
 {7,6,5,0,0,0,0,0} ,//level 8 wisdom 32
 {8,6,5,0,0,0,0,0} ,//level 9 wisdom 32
 {8,6,5,4,0,0,0,0} ,//level 10 wisdom 32
 {8,7,6,4,0,0,0,0} ,//level 11 wisdom 32
 {8,7,6,4,0,0,0,0} ,//level 12 wisdom 32
 {9,7,6,5,0,0,0,0} ,//level 13 wisdom 32
 {9,7,7,5,4,0,0,0} ,//level 14 wisdom 32
 {9,8,7,5,4,0,0,0} ,//level 15 wisdom 32
 {9,8,7,6,4,0,0,0} ,//level 16 wisdom 32
 {10,8,8,6,5,0,0,0},// level 17 wisdom 32
 {10,9,8,7,5,3,0,0},// level 18 wisdom 32
 {10,9,8,7,6,3,0,0},// level 19 wisdom 32
 {10,9,9,7,6,4,0,0},// level 20 wisdom 32
 {11,9,9,8,6,4,0,0},// level 21 wisdom 32
 {11,10,9,8,7,5,0,0},// level 22 wisdom 32
 {11,10,10,8,7,5,3,0},// level 23 wisdom 32
 {11,10,10,9,8,5,3,0},// level 24 wisdom 32
 {12,11,10,9,8,6,4,0},// level 25 wisdom 32
 {12,11,11,10,8,6,4,0},// level 26 wisdom 32
 {12,11,11,10,9,7,5,0},// level 27 wisdom 32
 {13,11,11,10,9,7,5,2},// level 28 wisdom 32
 {13,12,12,11,10,8,6,3},// level 29 wisdom 32
 {13,12,12,11,10,8,6,5},// level 30 wisdom 32
 {6,0,0,0,0,0,0,0} ,//level 1 wisdom 33
 {6,0,0,0,0,0,0,0} ,//level 2 wisdom 33
 {6,0,0,0,0,0,0,0} ,//level 3 wisdom 33
 {6,5,0,0,0,0,0,0} ,//level 4 wisdom 33
 {7,5,0,0,0,0,0,0} ,//level 5 wisdom 33
 {7,5,0,0,0,0,0,0} ,//level 6 wisdom 33
 {7,6,5,0,0,0,0,0} ,//level 7 wisdom 33
 {7,6,5,0,0,0,0,0} ,//level 8 wisdom 33
 {8,6,5,0,0,0,0,0} ,//level 9 wisdom 33
 {8,6,6,4,0,0,0,0} ,//level 10 wisdom 33
 {8,7,6,4,0,0,0,0} ,//level 11 wisdom 33
 {9,7,6,5,0,0,0,0} ,//level 12 wisdom 33
 {9,7,7,5,0,0,0,0} ,//level 13 wisdom 33
 {9,8,7,5,4,0,0,0} ,//level 14 wisdom 33
 {9,8,7,6,4,0,0,0} ,//level 15 wisdom 33
 {10,8,8,6,5,0,0,0},// level 16 wisdom 33
 {10,9,8,6,5,0,0,0},// level 17 wisdom 33
 {10,9,8,7,5,3,0,0},// level 18 wisdom 33
 {10,9,9,7,6,3,0,0},// level 19 wisdom 33
 {11,9,9,8,6,4,0,0},// level 20 wisdom 33
 {11,10,9,8,7,4,0,0},// level 21 wisdom 33
 {11,10,10,8,7,5,0,0},// level 22 wisdom 33
 {11,10,10,9,7,5,3,0},// level 23 wisdom 33
 {12,11,10,9,8,6,3,0},// level 24 wisdom 33
 {12,11,11,9,8,6,4,0},// level 25 wisdom 33
 {12,11,11,10,9,7,4,0},// level 26 wisdom 33
 {13,11,11,10,9,7,5,0},// level 27 wisdom 33
 {13,12,12,11,10,8,5,2},// level 28 wisdom 33
 {13,12,12,11,10,8,6,3},// level 29 wisdom 33
 {13,12,12,11,10,8,7,5},// level 30 wisdom 33
 {6,0,0,0,0,0,0,0} ,//level 1 wisdom 34
 {6,0,0,0,0,0,0,0} ,//level 2 wisdom 34
 {6,0,0,0,0,0,0,0} ,//level 3 wisdom 34
 {7,5,0,0,0,0,0,0} ,//level 4 wisdom 34
 {7,5,0,0,0,0,0,0} ,//level 5 wisdom 34
 {7,5,0,0,0,0,0,0} ,//level 6 wisdom 34
 {7,6,5,0,0,0,0,0} ,//level 7 wisdom 34
 {8,6,5,0,0,0,0,0} ,//level 8 wisdom 34
 {8,6,6,0,0,0,0,0} ,//level 9 wisdom 34
 {8,7,6,4,0,0,0,0} ,//level 10 wisdom 34
 {9,7,6,4,0,0,0,0} ,//level 11 wisdom 34
 {9,7,7,5,0,0,0,0} ,//level 12 wisdom 34
 {9,8,7,5,0,0,0,0} ,//level 13 wisdom 34
 {9,8,7,5,4,0,0,0} ,//level 14 wisdom 34
 {10,8,8,6,4,0,0,0},// level 15 wisdom 34
 {10,8,8,6,5,0,0,0},// level 16 wisdom 34
 {10,9,8,7,5,0,0,0},// level 17 wisdom 34
 {10,9,9,7,6,3,0,0},// level 18 wisdom 34
 {11,9,9,7,6,3,0,0},// level 19 wisdom 34
 {11,10,9,8,6,4,0,0},// level 20 wisdom 34
 {11,10,10,8,7,4,0,0},// level 21 wisdom 34
 {12,10,10,9,7,5,0,0},// level 22 wisdom 34
 {12,11,10,9,8,5,3,0},// level 23 wisdom 34
 {12,11,11,9,8,6,3,0},// level 24 wisdom 34
 {12,11,11,10,9,6,4,0},// level 25 wisdom 34
 {13,11,11,10,9,7,5,0},// level 26 wisdom 34
 {13,12,12,11,9,7,5,0},// level 27 wisdom 34
 {13,12,12,11,10,8,6,2},// level 28 wisdom 34
 {13,12,12,11,10,8,6,3},// level 29 wisdom 34
 {14,13,13,12,11,9,7,5},// level 30 wisdom 34
 {6,0,0,0,0,0,0,0} ,//level 1 wisdom 35
 {6,0,0,0,0,0,0,0} ,//level 2 wisdom 35
 {7,0,0,0,0,0,0,0} ,//level 3 wisdom 35
 {7,5,0,0,0,0,0,0} ,//level 4 wisdom 35
 {7,5,0,0,0,0,0,0} ,//level 5 wisdom 35
 {7,6,0,0,0,0,0,0} ,//level 6 wisdom 35
 {8,6,5,0,0,0,0,0} ,//level 7 wisdom 35
 {8,6,5,0,0,0,0,0} ,//level 8 wisdom 35
 {8,7,6,0,0,0,0,0} ,//level 9 wisdom 35
 {8,7,6,4,0,0,0,0} ,//level 10 wisdom 35
 {9,7,6,4,0,0,0,0} ,//level 11 wisdom 35
 {9,7,7,5,0,0,0,0} ,//level 12 wisdom 35
 {9,8,7,5,0,0,0,0} ,//level 13 wisdom 35
 {10,8,7,6,4,0,0,0},// level 14 wisdom 35
 {10,8,8,6,4,0,0,0},// level 15 wisdom 35
 {10,9,8,6,5,0,0,0},// level 16 wisdom 35
 {10,9,8,7,5,0,0,0},// level 17 wisdom 35
 {11,9,9,7,6,3,0,0},// level 18 wisdom 35
 {11,10,9,8,6,3,0,0} ,//level 19 wisdom 35
 {11,10,10,8,7,4,0,0},// level 20 wisdom 35
 {12,10,10,8,7,4,0,0},// level 21 wisdom 35
 {12,11,10,9,7,5,0,0},// level 22 wisdom 35
 {12,11,11,9,8,5,3,0} ,//level 23 wisdom 35
 {12,11,11,10,8,6,4,0},// level 24 wisdom 35
 {13,11,11,10,9,6,4,0},// level 25 wisdom 35
 {13,12,12,10,9,7,5,0},// level 26 wisdom 35
 {13,12,12,11,10,7,5,0},// level 27 wisdom 35
 {13,12,12,11,10,8,6,2},// level 28 wisdom 35
 {14,13,13,12,11,8,6,3},// level 29 wisdom 35
 {14,13,13,12,11,9,7,5},// level 30 wisdom 35
};

#define MAX_ME_SLOT 4
const int MERCHANT_SLOTS[][MAX_ME_SLOT] =
{{0,0,0,0},// lvl 1
 {0,0,0,0},// lvl 2
 {0,0,0,0},// lvl 3
 {1,0,0,0},// lvl 4
 {2,0,0,0},// lvl 5
 {2,0,0,0},// lvl 6
 {3,0,0,0},// lvl 7
 {3,0,0,0},// lvl 8
 {3,0,0,0},// lvl 9
 {4,0,0,0},// lvl 10
 {4,0,0,0},// lvl 11
 {4,1,0,0},// lvl 12
 {4,2,0,0},// lvl 13
 {5,2,0,0},// lvl 14
 {5,3,0,0},// lvl 15
 {5,3,0,0},// lvl 16
 {5,3,0,0},// lvl 17
 {5,4,1,0},// lvl 18
 {6,4,2,0},// lvl 19
 {6,4,2,0},// lvl 20
 {6,4,3,0},// lvl 21
 {6,5,3,0},// lvl 22
 {6,5,3,1},// lvl 23
 {6,5,4,2},// lvl 24
 {7,5,4,2},// lvl 25
 {7,5,4,3},// lvl 26
 {7,6,4,3},// lvl 27
 {7,6,5,3},// lvl 28
 {7,6,5,4},// lvl 29
 {7,6,5,4},// lvl 30
};

#define MIN_PA_LEVEL 8
#define MAX_PA_LEVEL 30
#define MAX_PA_SLOT  4
#define MIN_PA_WIS   10
#define MAX_PA_WIS   30
#define PA_WIS_DIV   2

const int PALADINE_SLOTS[][MAX_PA_SLOT] =
{{1,0,0,0},// lvl 8 wis 10,11
 {1,0,0,0},// lvl 9
 {1,0,0,0},// lvl 10
 {1,0,0,0},// lvl 11
 {1,0,0,0},// lvl 12
 {1,0,0,0},// lvl 13
 {1,1,0,0},// lvl 14
 {2,1,0,0},// lvl 15
 {2,1,0,0},// lvl 16
 {2,1,0,0},// lvl 17
 {2,1,1,0},// lvl 18
 {2,1,1,0},// lvl 19
 {2,1,1,0},// lvl 20
 {2,2,1,0},// lvl 21
 {3,2,1,0},// lvl 22
 {3,2,1,1},// lvl 23
 {3,2,1,1},// lvl 24
 {3,2,2,1},// lvl 25
 {3,2,2,1},// lvl 26
 {3,2,2,1},// lvl 27
 {3,3,2,1},// lvl 28
 {4,3,2,1},// lvl 29
 {4,3,2,1},// lvl 30
 {1,0,0,0},// lvl 8 wis 12,13
 {1,0,0,0},// lvl 9
 {1,0,0,0},// lvl 10
 {1,0,0,0},// lvl 11
 {1,0,0,0},// lvl 12
 {2,0,0,0},// lvl 13
 {2,1,0,0},// lvl 14
 {2,1,0,0},// lvl 15
 {2,1,0,0},// lvl 16
 {2,1,0,0},// lvl 17
 {3,1,1,0},// lvl 18
 {3,2,1,0},// lvl 19
 {3,2,1,0},// lvl 20
 {3,2,1,0},// lvl 21
 {3,2,1,0},// lvl 22
 {3,2,2,1},// lvl 23
 {4,2,2,1},// lvl 24
 {4,3,2,1},// lvl 25
 {4,3,2,1},// lvl 26
 {4,3,2,1},// lvl 27
 {4,3,2,2},// lvl 28
 {4,3,3,2},// lvl 29
 {4,3,3,2},// lvl 30
 {1,0,0,0},// lvl 8 wis 14,15
 {1,0,0,0},// lvl 9
 {1,0,0,0},// lvl 10
 {2,0,0,0},// lvl 11
 {2,0,0,0},// lvl 12
 {2,0,0,0},// lvl 13
 {2,1,0,0},// lvl 14
 {3,1,0,0},// lvl 15
 {3,1,0,0},// lvl 16
 {3,2,0,0},// lvl 17
 {3,2,1,0},// lvl 18
 {3,2,1,0},// lvl 19
 {4,2,1,0},// lvl 20
 {4,3,2,0},// lvl 21
 {4,3,2,0},// lvl 22
 {4,3,2,1},// lvl 23
 {4,3,2,1},// lvl 24
 {4,3,3,1},// lvl 25
 {4,3,3,1},// lvl 26
 {4,4,3,2},// lvl 27
 {4,4,3,2},// lvl 28
 {4,4,3,2},// lvl 29
 {4,4,3,2},// lvl 30
 {1,0,0,0},// lvl 8 wis 16,17
 {1,0,0,0},// lvl 9
 {2,0,0,0},// lvl 10
 {2,0,0,0},// lvl 11
 {2,0,0,0},// lvl 12
 {3,0,0,0},// lvl 13
 {3,1,0,0},// lvl 14
 {3,1,0,0},// lvl 15
 {3,1,0,0},// lvl 16
 {4,2,0,0},// lvl 17
 {4,2,1,0},// lvl 18
 {4,2,1,0},// lvl 19
 {4,2,1,0},// lvl 20
 {4,3,2,0},// lvl 21
 {5,3,2,0},// lvl 22
 {5,3,2,1},// lvl 23
 {5,3,2,1},// lvl 24
 {5,3,3,1},// lvl 25
 {5,4,3,1},// lvl 26
 {5,4,3,2},// lvl 27
 {5,4,3,2},// lvl 28
 {5,4,3,2},// lvl 29
 {5,4,3,2},// lvl 30
 {1,0,0,0},// lvl 8 wis 18,19
 {1,0,0,0},// lvl 9
 {2,0,0,0},// lvl 10
 {2,0,0,0},// lvl 11
 {2,0,0,0},// lvl 12
 {3,0,0,0},// lvl 13
 {3,1,0,0},// lvl 14
 {3,1,0,0},// lvl 15
 {3,2,0,0},// lvl 16
 {4,2,0,0},// lvl 17
 {4,2,1,0},// lvl 18
 {4,3,1,0},// lvl 19
 {4,3,1,0},// lvl 20
 {4,3,2,0},// lvl 21
 {5,3,2,0},// lvl 22
 {5,4,2,1},// lvl 23
 {5,4,2,1},// lvl 24
 {5,4,3,1},// lvl 25
 {5,4,3,1},// lvl 26
 {5,4,3,2},// lvl 27
 {5,4,3,2},// lvl 28
 {5,4,3,2},// lvl 29
 {5,4,3,2},// lvl 30
 {1,0,0,0},// lvl 8 wis 20,21
 {1,0,0,0},// lvl 9
 {2,0,0,0},// lvl 10
 {2,0,0,0},// lvl 11
 {2,0,0,0},// lvl 12
 {3,0,0,0},// lvl 13
 {3,1,0,0},// lvl 14
 {3,1,0,0},// lvl 15
 {3,2,0,0},// lvl 16
 {4,2,0,0},// lvl 17
 {4,2,1,0},// lvl 18
 {4,3,1,0},// lvl 19
 {4,3,1,0},// lvl 20
 {4,3,2,0},// lvl 21
 {5,3,2,0},// lvl 22
 {5,4,2,1},// lvl 23
 {5,4,2,1},// lvl 24
 {5,4,3,1},// lvl 25
 {5,4,3,1},// lvl 26
 {5,4,3,2},// lvl 27
 {6,4,3,2},// lvl 28
 {6,5,3,2},// lvl 29
 {6,5,4,2},// lvl 30
 {1,0,0,0},// lvl 8 wis 22,23
 {2,0,0,0},// lvl 9
 {2,0,0,0},// lvl 10
 {3,0,0,0},// lvl 11
 {3,0,0,0},// lvl 12
 {3,0,0,0},// lvl 13
 {4,1,0,0},// lvl 14
 {4,2,0,0},// lvl 15
 {4,2,0,0},// lvl 16
 {4,3,0,0},// lvl 17
 {4,3,1,0},// lvl 18
 {5,3,1,0},// lvl 19
 {5,3,2,0},// lvl 20
 {5,4,2,0},// lvl 21
 {5,4,2,0},// lvl 22
 {5,4,3,1},// lvl 23
 {5,4,3,1},// lvl 24
 {5,4,3,1},// lvl 25
 {6,4,3,2},// lvl 26
 {6,5,3,2},// lvl 27
 {6,5,4,2},// lvl 28
 {6,5,4,2},// lvl 29
 {6,5,4,3},// lvl 30
 {1,0,0,0},// lvl 8 wis 24,25
 {2,0,0,0},// lvl 9
 {2,0,0,0},// lvl 10
 {3,0,0,0},// lvl 11
 {3,0,0,0},// lvl 12
 {3,0,0,0},// lvl 13
 {4,1,0,0},// lvl 14
 {4,2,0,0},// lvl 15
 {4,2,0,0},// lvl 16
 {4,3,0,0},// lvl 17
 {5,3,1,0},// lvl 18
 {5,3,1,0},// lvl 19
 {5,4,2,0},// lvl 20
 {5,4,2,0},// lvl 21
 {5,4,2,0},// lvl 22
 {5,4,3,1},// lvl 23
 {6,4,3,1},// lvl 24
 {6,4,3,1},// lvl 25
 {6,5,3,2},// lvl 26
 {6,5,4,2},// lvl 27
 {6,5,4,2},// lvl 28
 {6,5,4,2},// lvl 29
 {6,5,4,3},// lvl 30
 {2,0,0,0},// lvl 8 wis 26,27
 {2,0,0,0},// lvl 9
 {3,0,0,0},// lvl 10
 {3,0,0,0},// lvl 11
 {3,0,0,0},// lvl 12
 {4,0,0,0},// lvl 13
 {4,2,0,0},// lvl 14
 {4,2,0,0},// lvl 15
 {4,3,0,0},// lvl 16
 {4,3,0,0},// lvl 17
 {5,3,1,0},// lvl 18
 {5,4,2,0},// lvl 19
 {5,4,2,0},// lvl 20
 {5,4,2,0},// lvl 21
 {5,4,3,0},// lvl 22
 {5,4,3,1},// lvl 23
 {6,4,3,1},// lvl 24
 {6,5,3,2},// lvl 25
 {6,5,4,2},// lvl 26
 {6,5,4,2},// lvl 27
 {6,5,4,2},// lvl 28
 {6,5,4,3},// lvl 29
 {6,6,4,3},// lvl 30
 {2,0,0,0},// lvl 8 wis 28,29
 {2,0,0,0},// lvl 9
 {3,0,0,0},// lvl 10
 {3,0,0,0},// lvl 11
 {3,0,0,0},// lvl 12
 {4,0,0,0},// lvl 13
 {4,2,0,0},// lvl 14
 {4,2,0,0},// lvl 15
 {4,3,0,0},// lvl 16
 {5,3,0,0},// lvl 17
 {5,3,2,0},// lvl 18
 {5,4,2,0},// lvl 19
 {5,4,2,0},// lvl 20
 {5,4,3,0},// lvl 21
 {5,4,3,0},// lvl 22
 {6,4,3,1},// lvl 23
 {6,5,3,2},// lvl 24
 {6,5,3,2},// lvl 25
 {6,5,4,2},// lvl 26
 {6,5,4,2},// lvl 27
 {6,5,4,3},// lvl 28
 {6,5,4,3},// lvl 29
 {7,6,5,3},// lvl 30
 {2,0,0,0},// lvl 8 wis 30
 {3,0,0,0},// lvl 9
 {3,0,0,0},// lvl 10
 {3,0,0,0},// lvl 11
 {4,0,0,0},// lvl 12
 {4,0,0,0},// lvl 13
 {4,2,0,0},// lvl 14
 {4,2,0,0},// lvl 15
 {5,3,0,0},// lvl 16
 {5,3,0,0},// lvl 17
 {5,3,2,0},// lvl 18
 {5,4,2,0},// lvl 19
 {6,4,3,0},// lvl 20
 {6,4,3,0},// lvl 21
 {6,4,3,0},// lvl 22
 {6,5,3,2},// lvl 23
 {6,5,3,2},// lvl 24
 {6,5,4,2},// lvl 25
 {6,5,4,2},// lvl 26
 {7,5,4,3},// lvl 27
 {7,6,4,3},// lvl 28
 {7,6,5,3},// lvl 29
 {7,6,5,3},// lvl 30
};


int slot_for_char(struct char_data * ch, int slot_num)
{int wis_is = 0, /*i,*/ wis_line, wis_block;

 if (slot_num < 1        ||
     slot_num > MAX_SLOT ||
     GET_LEVEL(ch) < 1   ||
     IS_NPC(ch)
    )
    return -1;

 if (IS_IMMORTAL(ch))
    return 10;
 slot_num--;

 switch (GET_CLASS(ch))
  { case CLASS_BATTLEMAGE :
    case CLASS_DEFENDERMAGE :
    case CLASS_CHARMMAGE :
    case CLASS_NECROMANCER :
         /*
          wis_is = MAGIC_SLOT_VALUES[(int) GET_LEVEL(ch)][slot_num];
          */
         wis_is = MAG_SLOTS[(int) GET_LEVEL(ch)-1][slot_num];
         break;
    case CLASS_CLERIC :
         /*
         if ((wis_is = CLERIC_SLOT_VALUES[(int) GET_LEVEL(ch)][slot_num]))
            {for (i = 1; i <= GET_REAL_INT(ch); i++)
                 wis_is += IS_SET(wis_app[i].spell_additional, 1 << slot_num);
            }
          */
         if (GET_LEVEL(ch)    >= MIN_CL_LEVEL &&
             slot_num         <  MAX_CL_SLOT  &&
             GET_REAL_WIS(ch) >= MIN_CL_WIS
            )
            {wis_block = MIN(MAX_CL_WIS, GET_REAL_WIS(ch)) - MIN_CL_WIS;
             wis_block = wis_block / CL_WIS_DIV;
             wis_block = wis_block * (MAX_CL_LEVEL - MIN_CL_LEVEL + 1);
             wis_line  = GET_LEVEL(ch) - MIN_CL_LEVEL;
             wis_is    = CLERIC_SLOTS[wis_block+wis_line][slot_num];
            }
         break;
    case CLASS_PALADINE :
         /*
         if ((wis_is = PALADINE_SLOT_VALUES[(int) GET_LEVEL(ch)][slot_num]))
            {for (i = 1; i <= GET_REAL_INT(ch); i++)
                 wis_is += IS_SET(wis_app[i].spell_additional, 1 << slot_num);
            }
          */
         if (GET_LEVEL(ch)    >= MIN_PA_LEVEL &&
             slot_num         <  MAX_PA_SLOT  &&
             GET_REAL_WIS(ch) >= MIN_PA_WIS
            )
            {wis_block = MIN(MAX_PA_WIS, GET_REAL_WIS(ch)) - MIN_PA_WIS;
             wis_block = wis_block / PA_WIS_DIV;
             wis_block = wis_block * (MAX_PA_LEVEL - MIN_PA_LEVEL + 1);
             wis_line  = GET_LEVEL(ch) - MIN_PA_LEVEL;
             wis_is    = PALADINE_SLOTS[wis_block+wis_line][slot_num];
            }
         break;
    /*
    case CLASS_RANGER :
         if ((wis_is = RANGER_SLOT_VALUES[(int) GET_LEVEL(ch)][slot_num]))
            {for (i = 1; i <= GET_REAL_INT(ch); i++)
                 wis_is += IS_SET(wis_app[i].spell_additional, 1 << slot_num);
            }
         break;
     */
    case CLASS_MERCHANT :
         if (slot_num < MAX_ME_SLOT)
            wis_is = MERCHANT_SLOTS[(int) GET_LEVEL(ch)-1][slot_num];
         break;
  }
 return (wis_is ? MIN(25, wis_is + GET_SLOT(ch,slot_num) + GET_REMORT(ch)) : 0);
}


int mag_manacost(struct char_data * ch, int spellnum)
{
  if (GET_LEVEL(ch) <= SpINFO.min_level[(int) GET_CLASS(ch)])
     return SpINFO.mana_max;

  return MAX(SpINFO.mana_max - (SpINFO.mana_change *
                     (GET_LEVEL(ch) - SpINFO.min_level[(int) GET_CLASS(ch)])),
                 SpINFO.mana_min);
}


/* say_spell erodes buf, buf1, buf2 */
void say_spell(struct char_data * ch, int spellnum, struct char_data * tch,
               struct obj_data * tobj)
{
  char   lbuf[256];
  char   *say_to_self, *say_to_other, *say_to_obj_vis, *say_to_something,
         *helpee_vict, *damagee_vict;
  const  char *format;
  struct char_data *i;
  int    j = 0, ofs = 0, religion;

  *buf  = '\0';
  strcpy(lbuf, SpINFO.syn);

  /* Say phrase ? */
  switch (GET_CLASS(ch))
  {case CLASS_CLERIC:
   case CLASS_BATTLEMAGE:
   case CLASS_THIEF:
   case CLASS_WARRIOR:
   case CLASS_ASSASINE:
   case CLASS_GUARD:
   case CLASS_DEFENDERMAGE:
   case CLASS_CHARMMAGE:
   case CLASS_NECROMANCER:
   case CLASS_PALADINE:
   case CLASS_RANGER:
   case CLASS_SMITH:
   case CLASS_MERCHANT:
   case CLASS_DRUID:
        if (*cast_phrase[spellnum][GET_RELIGION(ch)] != '\n')
           strcpy(buf,cast_phrase[spellnum][GET_RELIGION(ch)]);
	say_to_self      = "$n прикрыл$g глаза и прошептал$g : '%s'.";
        say_to_other     = "$n взглянул$g на $N3 и произнес$q : '%s'.";
        say_to_obj_vis   = "$n посмотрел$g на $o3 и произнес$q : '%s'.";
     	say_to_something = "$n произнес$q : '%s'.";
        damagee_vict     = "$n зыркнул$g на Вас и произнес$q : '%s'.";
        helpee_vict      = "$n подмигнул$g Вам и произнес$q : '%s'.";
        break;
   case CLASS_UNDEAD:
   case CLASS_HUMAN:
   case CLASS_HERO_WARRIOR:
   case CLASS_HERO_MAGIC:
        say_to_self      = "$n пробормотал$g : '%s'.";
        say_to_other     = "$n взглянул$g на $N3 и бросил$g : '%s'.";
        say_to_obj_vis   = "$n глянул$g на $o3 и произнес$q : '%s'.";
     	say_to_something = "$n произнес$q : '%s'.";
        damagee_vict     = "$n зыркнул$g на Вас и проревел$g : '%s'.";
        helpee_vict      = "$n улыбнул$u Вам и произнес$q : '%s'.";
        religion = number(RELIGION_POLY,RELIGION_MONO);
        if (*cast_phrase[spellnum][religion] != '\n')
            strcpy(buf,cast_phrase[spellnum][religion]);
        break;

   default:
        say_to_self      = "$n издал$g непонятный звук.";
        say_to_other     = "$n издал$g непонятный звук.";	
        say_to_obj_vis   = "$n издал$g непонятный звук.";	
        say_to_something = "$n издал$g непонятный звук.";	
        damagee_vict     = "$n издал$g непонятный звук.";	
        helpee_vict      = "$n издал$g непонятный звук.";	

        // say_to_self      = "$n рявкнул$g : '%s'.";
        // say_to_other     = "$n взглянул$g на $N3 и глухо просипел$q : '%s'.";
        // say_to_obj_vis   = "$n посмотрел$g на $o3 и произнес$q : '%s'.";
     	// say_to_something = "$n молвил$g : '%s'.";
        // damagee_vict     = "$n злобно рыкнул$g на Вас : '%s'.";
        // helpee_vict      = "$n тихо пробормотал$g Вам : '%s'.";
  }
  if (!*buf)
     {while (lbuf[ofs])
            {for (j = 0; *(syls[j].org); j++)
                 {if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org)))
                     {strcat(buf, syls[j].news);
                      ofs += strlen(syls[j].org);
                      break;
                     }
                 }
            }
      /* i.e., we didn't find a match in syls[] */
      if (!*syls[j].org)
         {log("No entry in syllable table for substring of '%s'", lbuf);
          ofs++;
         }
    }

  if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch))
     {if (tch == ch)
          format = say_to_self;
      else
          format = say_to_other;
     }
  else
  if (tobj != NULL &&
      (IN_ROOM(tobj) == IN_ROOM(ch) ||
       tobj->carried_by == ch
      )
     )
     format = say_to_obj_vis;
  else
     format = say_to_something;

  sprintf(buf1, format, spell_name(spellnum));
  sprintf(buf2, format, buf);

  for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
      {if (i == ch || i == tch || !i->desc || !AWAKE(i))
          continue;
       if (IS_SET(GET_SPELL_TYPE(i, spellnum), SPELL_KNOW))
          perform_act(buf1, ch, tobj, tch, i);
       else
          perform_act(buf2, ch, tobj, tch, i);
      }

  if (tch != NULL && tch != ch && IN_ROOM(tch) == IN_ROOM(ch))
     {if (SpINFO.violent)
         sprintf(buf1, damagee_vict,
                 IS_SET(GET_SPELL_TYPE(tch,spellnum),SPELL_KNOW) ? spell_name(spellnum) : buf);
      else
         sprintf(buf1, helpee_vict,
                 IS_SET(GET_SPELL_TYPE(tch,spellnum),SPELL_KNOW) ? spell_name(spellnum) : buf);
      act(buf1, FALSE, ch, NULL, tch, TO_VICT);
     }
}

/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= TOP_SPELL_DEFINE.
 */
const char *skill_name(int num)
{
  if (num > 0 && num <= MAX_SPELLS)
      return (skill_info[num].name);
  else
  if (num == -1)
    return "UNUSED";
  else
    return "UNDEFINED";
}

const char *spell_name(int num)
{
  if (num > 0 && num <= MAX_SKILLS)
      return (spell_info[num].name);
  else
  if (num == -1)
    return "UNUSED";
  else
    return "UNDEFINED";
}


int find_skill_num(char *name)
{
  int index, ok;
  char *temp, *temp2;
  char first[256], first2[256];
  for (index = 1; index <= TOP_SKILL_DEFINE; index++)
      {if (is_abbrev(name, skill_info[index].name))
          return (index);

       ok = TRUE;
       /* It won't be changed, but other uses of this function elsewhere may. */
       temp = any_one_arg((char *)skill_info[index].name, first);
       temp2 = any_one_arg(name, first2);
       while (*first && *first2 && ok)
             {if (!is_abbrev(first2, first))
                     ok = FALSE;
              temp = any_one_arg(temp, first);
              temp2 = any_one_arg(temp2, first2);
             }

       if (ok && !*first2)
          return (index);
       }
  return (-1);
}

int find_spell_num(char *name)
{
  int index, ok;
  char *temp, *temp2;
  char first[256], first2[256], *realname;


  for (index = 1; index <= TOP_SPELL_DEFINE; index++)
      {
       realname = ((ubyte)*name <= (ubyte)'z') ? (char *) spell_info[index].syn : (char *) spell_info[index].name;

       if (!realname || !*realname)
          continue;
       if (is_abbrev(name, realname))
          return (index);
       ok = TRUE;
       /* It won't be changed, but other uses of this function elsewhere may. */
       temp = any_one_arg((char *)realname, first);
       temp2 = any_one_arg(name, first2);
       while (*first && *first2 && ok)
             {if (!is_abbrev(first2, first))
                     ok = FALSE;
              temp = any_one_arg(temp, first);
              temp2 = any_one_arg(temp2, first2);
             }
       if (ok && !*first2)
          return (index);

       }
  return (-1);
}


int may_kill_here(struct char_data *ch, struct char_data *victim)
{
 if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOFIGHT))
    return (FALSE);

 if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_NOFIGHT))
    {act("Боги предотвратили Ваше нападение на $N3.",FALSE,ch,0,victim,TO_CHAR);
     return (FALSE);
    }

 if ((FIGHTING(ch)     && FIGHTING(ch) == victim) ||
     (FIGHTING(victim) && FIGHTING(victim) == ch))
    {return (TRUE);
    }
 else
 if (ch != victim && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
    {if (IS_IMMORTAL(ch)                        ||
         may_pkill(ch,victim) == PC_REVENGE_PC  ||
         (IS_NPC(ch) && ch->nr == real_mobile(DG_CASTER_PROXY)) ||
         IS_KILLER(victim)
        )
        return (TRUE);
     else
        {send_to_char("Здесь слишком мирно, чтобы начинать драку...\r\n", ch);
         return (FALSE);
        }
    }
 return (TRUE);
}

int may_cast_in_nomagic(struct char_data *caster, struct char_data *victim,
                        int spellnum)
{
  // More than 33 level - may cast always
  if (IS_GRGOD(caster))
     return (TRUE);

  // In nomagic cast only to victim
  if (!victim)
     return (FALSE);

  // In nomagic cann't cast groups, masses, areas spells
  if (IS_SET(SpINFO.routines, MAG_GROUPS | MAG_MASSES | MAG_AREAS))
     return (FALSE);

  // May cast themself if FIGHTING
  if (caster == victim && FIGHTING(caster))
     return (TRUE);

  // May cast good affects to VICTIM, if it REVENGE
  if (IS_SET(SpINFO.routines, NPC_AFFECT_NPC | NPC_UNAFFECT_NPC | NPC_DUMMY) &&
      FIGHTING(victim) &&
      may_pkill(victim, FIGHTING(victim)) == PC_REVENGE_PC
     )
     return (TRUE);

  // May cast bad affects to VICTIM, as REVENGE
  if (IS_SET(SpINFO.routines, NPC_AFFECT_PC | NPC_DAMAGE_PC) &&
      may_pkill(caster, victim) == PC_REVENGE_PC
     )
     return (TRUE);

  return (FALSE);
}



int may_cast_in_peacefull(struct char_data *caster, struct char_data *victim,
                          int spellnum)
{
  /*  More than 33 level - may cast always */
  if (IS_GRGOD(caster))
     return (TRUE);

  if (SpINFO.violent || IS_SET(SpINFO.routines, MAG_DAMAGE))
     {if (IS_SET(SpINFO.routines, MAG_MASSES | MAG_AREAS | MAG_MANUAL))
         return (FALSE);
      else
      if (victim &&
          (FIGHTING(caster) == victim ||
           may_pkill(caster, victim) == PC_REVENGE_PC
          )
         )
         {return (TRUE);
         }
      else
         return (FALSE);
     }
  return (TRUE);
}

int check_mobile_list(struct char_data *ch)
{struct char_data *vict;

 for (vict = character_list; vict; vict = vict->next)
     if (vict == ch)
        return (TRUE);

 return (FALSE);
}

void cast_reaction(struct char_data *victim, struct char_data *caster, int spellnum)
{
 if (!check_mobile_list(victim) || !SpINFO.violent)
    return;

 if (AFF_FLAGGED(victim,AFF_CHARM)     ||
     AFF_FLAGGED(victim,AFF_SLEEP)     ||
     AFF_FLAGGED(victim,AFF_BLIND)     ||
     AFF_FLAGGED(victim,AFF_STOPFIGHT) ||
     AFF_FLAGGED(victim,AFF_HOLD)      ||
     IS_HORSE(victim)
    )
    return;

 if (IS_NPC(caster) && GET_MOB_RNUM(caster) == real_mobile(DG_CASTER_PROXY))
    return;

 if (CAN_SEE(victim,caster)              &&
     MAY_ATTACK(victim)                  &&
     IN_ROOM(victim) == IN_ROOM(caster)
    )
    {if (IS_NPC(victim))
        attack_best(victim, caster);
     else
        hit(victim, caster, TYPE_UNDEFINED, 1);
    }
 else
 if (CAN_SEE(victim,caster) &&
     !IS_NPC(caster)        &&
     IS_NPC(victim)         &&
     MOB_FLAGGED(victim, MOB_MEMORY)
    )
    remember(victim, caster);
}


/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 */
int call_magic(struct char_data * caster,
               struct char_data * cvict,
               struct obj_data  * ovict,
               int spellnum,
               int level,
               int casttype)
{
  int savetype;

  if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
     return (0);

  if (!IS_NPC(caster))
     {if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC) &&
          !may_cast_in_nomagic(caster, cvict, spellnum)
         )
         {send_to_char("Ваша магия потерпела неудачу и развеялась по воздуху.\r\n", caster);
          act("Магия $n1 потерпела неудачу и развеялась по воздуху.", FALSE, caster, 0, 0, TO_ROOM);
          return (0);
         }
      if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL) &&
          !may_cast_in_peacefull(caster, cvict, spellnum))
         {send_to_char("Ваша магия обратилась всего лишь в яркую вспышку !\r\n",
                       caster);
          act("Яркая вспышка на миг осветила комнату, и тут же погасла.",
              FALSE, caster, 0, 0, TO_ROOM);
          return (0);
         }
     }

  /* determine the type of saving throw */
  switch (casttype)
  {
  case CAST_STAFF:
  case CAST_SCROLL:
  case CAST_POTION:
  case CAST_WAND:
    savetype = SAVING_ROD;
    break;
  case CAST_ITEMS:
  case CAST_RUNES:
  case CAST_SPELL:
    savetype = SAVING_SPELL;
    break;
  default:
    savetype = SAVING_BREATH;
    break;
  }

  if (IS_SET(SpINFO.routines, MAG_DAMAGE))
     if (mag_damage(level, caster, cvict, spellnum, savetype) == -1)
        return (-1);    /* Successful and target died, don't cast again. */

  if (IS_SET(SpINFO.routines, MAG_AFFECTS))
     mag_affects(level, caster, cvict, spellnum, savetype);

  if (IS_SET(SpINFO.routines, MAG_UNAFFECTS))
     mag_unaffects(level, caster, cvict, spellnum, savetype);

  if (IS_SET(SpINFO.routines, MAG_POINTS))
      mag_points(level, caster, cvict, spellnum, savetype);

  if (IS_SET(SpINFO.routines, MAG_ALTER_OBJS))
     mag_alter_objs(level, caster, ovict, spellnum, savetype);

  if (IS_SET(SpINFO.routines, MAG_GROUPS))
     mag_groups(level, caster, spellnum, savetype);

  if (IS_SET(SpINFO.routines, MAG_MASSES))
     mag_masses(level, caster, spellnum, savetype);

  if (IS_SET(SpINFO.routines, MAG_AREAS))
     mag_areas(level, caster, spellnum, savetype);

  if (IS_SET(SpINFO.routines, MAG_SUMMONS))
     mag_summons(level, caster, ovict, spellnum, savetype);

  if (IS_SET(SpINFO.routines, MAG_CREATIONS))
     mag_creations(level, caster, spellnum);

  if (IS_SET(SpINFO.routines, MAG_MANUAL))
     switch (spellnum)
     {case SPELL_CHARM:             MANUAL_SPELL(spell_charm); break;
      case SPELL_CREATE_WATER:      MANUAL_SPELL(spell_create_water); break;
      case SPELL_DETECT_POISON:     MANUAL_SPELL(spell_detect_poison); break;
      case SPELL_ENCHANT_WEAPON:    MANUAL_SPELL(spell_enchant_weapon); break;
      case SPELL_IDENTIFY:          MANUAL_SPELL(spell_identify); break;
      case SPELL_LOCATE_OBJECT:     MANUAL_SPELL(spell_locate_object); break;
      case SPELL_SUMMON:            MANUAL_SPELL(spell_summon); break;
      case SPELL_WORD_OF_RECALL:    MANUAL_SPELL(spell_recall); break;
      case SPELL_TELEPORT:          MANUAL_SPELL(spell_teleport); break;
      case SPELL_PORTAL:            MANUAL_SPELL(spell_portal); break;
      case SPELL_RELOCATE:          MANUAL_SPELL(spell_relocate); break;
      case SPELL_CHAIN_LIGHTNING:   MANUAL_SPELL(spell_chain_lightning); break;
      case SPELL_CONTROL_WEATHER:   MANUAL_SPELL(spell_control_weather); break;
      case SPELL_CREATE_WEAPON:     MANUAL_SPELL(spell_create_weapon); break;
      case SPELL_EVILESS:           MANUAL_SPELL(spell_eviless); break;
     }
  cast_reaction(cvict,caster,spellnum);
  return (1);
}


ACMD(do_ident)
{ struct char_data *cvict = NULL, *caster = ch;
  struct obj_data  *ovict = NULL;
  struct timed_type timed;
  int    k, level = 0;

  if (IS_NPC(ch) || GET_SKILL(ch, SKILL_IDENTIFY) <= 0)
     {send_to_char("Вам стоит сначала этому научиться.\r\n", ch);
      return;
     }

  one_argument(argument, arg);

  if (timed_by_skill(ch, SKILL_IDENTIFY))
     {send_to_char("Вы же недавно опознавали - подождите чуток.\r\n", ch);
      return;
     }

  k = generic_find(arg, FIND_CHAR_ROOM |
                        FIND_OBJ_INV   |
                        FIND_OBJ_ROOM  |
                                FIND_OBJ_EQUIP, caster, &cvict, &ovict);
  if (!k)
     {send_to_char("Похоже, здесь этого нет.\r\n", ch);
      return;
     }
  if (!IS_IMMORTAL(ch))
     {timed.skill    = SKILL_IDENTIFY;
      timed.time     = 12;
      timed_to_char(ch, &timed);
     }
  MANUAL_SPELL(skill_identify)
}

/*
 * mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 *
 * For reference, object values 0-3:
 * staff  - [0] level   [1] max charges [2] num charges [3] spell num
 * wand   - [0] level   [1] max charges [2] num charges [3] spell num
 * scroll - [0] level   [1] spell num   [2] spell num   [3] spell num
 * potion - [0] level   [1] spell num   [2] spell num   [3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified;
 * the DikuMUD format did not specify staff and wand levels in the world
 * files (this is a CircleMUD enhancement).
 */

 const char * what_sky_type[] =
 { "пасмурно",
   "cloudless",
   "облачно",
   "cloudy",
   "дождь",
   "raining",
   "ясно",
   "lightning",
   "\n"
};

const char *what_weapon[] =
{ "плеть",
  "whip",
  "дубина",
  "club",
  "\n"
};

int find_cast_target(int spellnum, char *t, struct char_data *ch, struct char_data **tch, struct obj_data **tobj)
{*tch  = NULL;
 *tobj = NULL;

  if (spellnum == SPELL_CONTROL_WEATHER)
     {if (!t || (what_sky = search_block(t, what_sky_type, FALSE)) < 0)
         {send_to_char("Не указан тип погоды.\r\n",ch);
          return FALSE;
         }
      else
         what_sky >>= 1;
     }	

  if (spellnum == SPELL_CREATE_WEAPON)
     {if (!t || (what_sky = search_block(t, what_weapon, FALSE)) < 0)
         {send_to_char("Не указан тип оружия.\r\n",ch);
          return FALSE;
         }
      else
         what_sky = 5 + (what_sky >> 1);
     }	

  *cast_argument = '\0';

  if (t)
     strcat(cast_argument,t);


 if (IS_SET(SpINFO.targets, TAR_IGNORE))
    return  TRUE;
 else
 if (t != NULL && *t)
    {if (IS_SET(SpINFO.targets, TAR_CHAR_ROOM))
        {if ((*tch = get_char_vis(ch, t, FIND_CHAR_ROOM)) != NULL)
            {if (SpINFO.violent && check_pkill(ch,*tch,t))
	        return FALSE;
             return TRUE;
            }
         }

      if (IS_SET(SpINFO.targets, TAR_CHAR_WORLD))
         {if ((*tch = get_char_vis(ch, t, FIND_CHAR_WORLD)) != NULL)
	     {if (SpINFO.violent && check_pkill(ch,*tch,t))
	        return FALSE;
              return TRUE;
	     }
         }	

      if (IS_SET(SpINFO.targets, TAR_OBJ_INV))
         if ((*tobj = get_obj_in_list_vis(ch, t, ch->carrying)) != NULL)
            return TRUE;

      if (IS_SET(SpINFO.targets, TAR_OBJ_EQUIP))
         {int i;
	  for (i = 0; i < NUM_WEARS; i++)
              if (GET_EQ(ch, i) && isname(t, GET_EQ(ch, i)->name))
                 {*tobj = GET_EQ(ch, i);
                  return TRUE;
                 }
         }

      if (IS_SET(SpINFO.targets, TAR_OBJ_ROOM))
         if ((*tobj = get_obj_in_list_vis(ch, t, world[IN_ROOM(ch)].contents)) != NULL)
            return TRUE;

      if (IS_SET(SpINFO.targets, TAR_OBJ_WORLD))
         if ((*tobj = get_obj_vis(ch, t)) != NULL)
            return TRUE;
     }
  else
     {if (IS_SET(SpINFO.targets, TAR_FIGHT_SELF))
         if (FIGHTING(ch) != NULL)
            {*tch    = ch;
             return  TRUE;
            }
      if (IS_SET(SpINFO.targets, TAR_FIGHT_VICT))
         if (FIGHTING(ch) != NULL)
            {*tch    = FIGHTING(ch);
             return TRUE;
            }
      if (IS_SET(SpINFO.targets, TAR_CHAR_ROOM) && !SpINFO.violent)
         {*tch    = ch;
          return TRUE;
         }
     }
  sprintf(buf, "На %s Вы хотите ЭТО колдовать ?\r\n",
          IS_SET(SpINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP) ? "ЧТО" : "КОГО");
  send_to_char(buf, ch);
  return FALSE;
}


void mag_objectmagic(struct char_data * ch, struct obj_data * obj,
                     char *argument)
{
  int    i, k, spellnum;
  struct char_data *tch = NULL, *next_tch;
  struct obj_data *tobj = NULL;

  one_argument(argument, arg);
  *cast_argument = '\0';
  strcat(cast_argument,arg);

  switch (GET_OBJ_TYPE(obj))
         {
  case ITEM_STAFF:
    if (obj->action_description)
       act(obj->action_description, FALSE, ch, obj, 0, TO_CHAR);
    else
       act("Вы ударили $o4 о землю.", FALSE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
    else
      act("$n ударил$g $o4 о землю.", FALSE, ch, obj, 0, TO_ROOM);

    if (GET_OBJ_VAL(obj, 2) <= 0)
       {send_to_char("Похоже, кончились заряды :)\r\n", ch);
        act("И ничего не случилось.", FALSE, ch, obj, 0, TO_ROOM);
       }
    else
       {GET_OBJ_VAL(obj, 2)--;
        WAIT_STATE(ch, PULSE_VIOLENCE);
        /* Level to cast spell at. */
        k = GET_OBJ_VAL(obj, 0) ? GET_OBJ_VAL(obj, 0) : DEFAULT_STAFF_LVL;

       /*
        * Problem : Area/mass spells on staves can cause crashes.
        * Solution: Remove the special nature of area/mass spells on staves.
        * Problem : People like that behavior.
        * Solution: We special case the area/mass spells here.
        */
        if (HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, 3), MAG_MASSES | MAG_AREAS))
           {for (i = 0, tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
                i++;
            while (i-- > 0)
                  call_magic(ch, NULL, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
           }
        else
           {for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
                {next_tch = tch->next_in_room;
                 if (ch != tch)
                    call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
                }
           }
       }
    break;
  case ITEM_WAND:
    spellnum = GET_OBJ_VAL(obj, 3);

    if (!*argument)
       tch = ch;
    else
    if (!find_cast_target(spellnum,argument,ch,&tch,&tobj))
       return;

    if (tch)
       {if (tch == ch)
           {if (obj->action_description)
               act(obj->action_description, FALSE, ch, obj, tch, TO_CHAR);
            else
               act("Вы указали $o4 на себя.", FALSE, ch, obj, 0, TO_CHAR);
            if (obj->action_description)
               act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
            else
               act("$n указал$g $o4 на себя.", FALSE, ch, obj, 0, TO_ROOM);
           }
        else
           {if (obj->action_description)
               act(obj->action_description, FALSE, ch, obj, tch, TO_CHAR);
            else
               act("Вы ткнули $o4 в $N1.", FALSE, ch, obj, tch, TO_CHAR);
            if (obj->action_description)
               act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
            else
               act("$n ткнул$g $o4 в $N1.", TRUE, ch, obj, tch, TO_ROOM);
           }
       }
    else
    if (tobj)
       {if (obj->action_description)
           act(obj->action_description, FALSE, ch, obj, tobj, TO_CHAR);
        else
           act("Вы прикоснулись $o4 к $P2.", FALSE, ch, obj, tobj, TO_CHAR);
        if (obj->action_description)
           act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
        else
           act("$n прикоснул$u $o4 к $P2.", TRUE, ch, obj, tobj, TO_ROOM);
       }
    else
    if (IS_SET(spell_info[GET_OBJ_VAL(obj, 3)].routines, MAG_AREAS | MAG_MASSES))
       {/* Wands with area spells don't need to be pointed. */
        act("Вы обвели $o4 вокруг комнаты.", FALSE, ch, obj, NULL, TO_CHAR);
        act("$n обвел$g $o4 вокруг комнаты.", TRUE, ch, obj, NULL, TO_ROOM);
       }

    if (GET_OBJ_VAL(obj, 2) <= 0)
       {send_to_char("Похоже, магия кончилась.\r\n", ch);
        act("И ничего не произошло.", FALSE, ch, obj, 0, TO_ROOM);
        return;
       }
    GET_OBJ_VAL(obj, 2)--;
    WAIT_STATE(ch, PULSE_VIOLENCE);
    if (GET_OBJ_VAL(obj, 0))
       call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
                  GET_OBJ_VAL(obj, 0), CAST_WAND);
    else
       call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
                  DEFAULT_WAND_LVL, CAST_WAND);
    break;
  case ITEM_SCROLL:
    if (AFF_FLAGGED(ch, AFF_SIELENCE))
       {send_to_char("Вы немы, как рыба.\r\n", ch);
        return;
       }
    if (AFF_FLAGGED(ch, AFF_BLIND))
       {send_to_char("Вы ослеплены.\r\n", ch);
        return;
       }

    spellnum = GET_OBJ_VAL(obj, 1);
    if (!*argument)
       tch = ch;
    else
    if (!find_cast_target(spellnum,argument,ch,&tch,&tobj))
       return;

    if (obj->action_description)
       act(obj->action_description, FALSE, ch, obj, NULL, TO_CHAR);
    else
       act("Вы зачитали $o3, котор$W рассыпался в прах.", TRUE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
       act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
    else
       act("$n зачитал$g $o3.", FALSE, ch, obj, NULL, TO_ROOM);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    for (i = 1; i <= 3; i++)
        if (call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, i),
                       GET_OBJ_VAL(obj, 0), CAST_SCROLL) <= 0)
           break;

    if (obj != NULL)
       extract_obj(obj);
    break;

  case ITEM_POTION:
    tch = ch;
    act("Вы осушили $o3.", FALSE, ch, obj, NULL, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
    else
      act("$n осушил$g $o3.", TRUE, ch, obj, NULL, TO_ROOM);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    for (i = 1; i <= 3; i++)
        if (call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i),
                       GET_OBJ_VAL(obj, 0), CAST_POTION) <= 0)
        break;

    if (obj != NULL)
       extract_obj(obj);
    break;
  default:
    log("SYSERR: Unknown object_type %d in mag_objectmagic.",
        GET_OBJ_TYPE(obj));
    break;
  }
}


/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 */
#define REMEMBER_SPELL(ch,spellnum) ({(ch)->Memory[spellnum]++; \
                                      (ch)->ManaMemNeeded += mag_manacost(ch,spellnum);})

int cast_spell(struct char_data * ch, struct char_data * tch,
                   struct obj_data * tobj, int spellnum)
{ if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE)
     {log("SYSERR: cast_spell trying to call spellnum %d/%d.\n", spellnum,
          TOP_SPELL_DEFINE);
      return (0);
     }
  if (GET_POS(ch) < SpINFO.min_position)
     { switch (GET_POS(ch))
       {
    case POS_SLEEPING:
      send_to_char("Вы спите и не могете думать больше ни о чем.\r\n", ch);
      break;
    case POS_RESTING:
      send_to_char("Вы расслаблены и отдыхаете. И далась Вам эта магия ?\r\n", ch);
      break;
    case POS_SITTING:
      send_to_char("Похоже, в этой позе Вы много не наколдуете.\r\n", ch);
      break;
    case POS_FIGHTING:
      send_to_char("Невозможно!  Вы сражаетесь ! Это Вам не шухры-мухры.\r\n", ch);
      break;
    default:
      send_to_char("Вам вряд ли это удасться.\r\n", ch);
      break;
    }
    log("illegal position");
    return (0);
  }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch))
     {send_to_char("Вы не посмеете поднять руку на Вашего повелителя !\r\n", ch);
      return (0);
     }
  if (tch != ch && !IS_IMMORTAL(ch) && IS_SET(SpINFO.targets, TAR_SELF_ONLY))
     {send_to_char("Вы можете колдовать это только на себя !\r\n", ch);
      return (0);
     }
  if (tch == ch && IS_SET(SpINFO.targets, TAR_NOT_SELF))
     {send_to_char("Колдовать ? ЭТО ? На себя ?! Да Вы с ума сошли !\r\n", ch);
      return (0);
     }
  if ((!tch || IN_ROOM(tch) == NOWHERE) && !tobj &&
      IS_SET(SpINFO.targets,
             TAR_CHAR_ROOM | TAR_CHAR_WORLD | TAR_FIGHT_SELF | TAR_FIGHT_VICT |
             TAR_OBJ_INV   | TAR_OBJ_ROOM   | TAR_OBJ_WORLD  | TAR_OBJ_EQUIP)
     )
     {send_to_char("Цель заклинания не доступна.\r\n",ch);
      return (0);
     }
  if (IS_SET(SpINFO.routines, MAG_GROUPS) &&
      !IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_GROUP)
     )
     {send_to_char("Но Вы же не член группы !\r\n",ch);
      return (0);
     }
  if (!FIGHTING(ch) && !IS_NPC(ch))
     {if (PRF_FLAGGED(ch,PRF_NOREPEAT))
         send_to_char(OK, ch);
      else
         {sprintf(buf,"Вы произнесли заклинание \"%s%s%s\".\r\n",
                  CCICYN(ch,C_NRM),SpINFO.name,CCNRM(ch,C_NRM));
          send_to_char(buf,ch);
         }
     }	 	  		
	
  say_spell(ch, spellnum, tch, tobj);
  if (GET_SPELL_MEM(ch,spellnum) > 0)
     GET_SPELL_MEM(ch,spellnum)--;
  else
     GET_SPELL_MEM(ch,spellnum) = 0;
  if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, PRF_AUTOMEM))
     REMEMBER_SPELL(ch,spellnum);
  if (!IS_NPC(ch))
     {if (!GET_SPELL_MEM(ch,spellnum))
         REMOVE_BIT(GET_SPELL_TYPE(ch,spellnum),SPELL_TEMP);
      /*log("[CAST_SPELL->AFFECT_TOTAL] Start <%s(%d)> <%s(%d)> <%s> <%d>",
            GET_NAME(ch),
            IN_ROOM(ch),
            tch  ? GET_NAME(tch)   : "-",
            tch  ? IN_ROOM(tch)    : -2,
            tobj ? tobj->PNames[0] : "-",
            spellnum); */
      affect_total(ch);
      //log("[CAST_SPELL->AFFECT_TOTAL] Stop");
     }
  else
     {GET_CASTER(ch)  -= (IS_SET(spell_info[spellnum].routines,NPC_CALCULATE) ? 1 : 0);
     }
  return (call_magic(ch, tch, tobj, spellnum, GET_LEVEL(ch), CAST_SPELL));
}

int spell_use_success(struct char_data *ch, struct char_data *victim, int casting_type, int spellnum)
{int prob = 100;

 if  (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE))
     return (TRUE);

 switch (casting_type)
 {case SAVING_SPELL:
  case SAVING_ROD:
       prob = int_app[GET_REAL_WIS(ch)].spell_success +
              GET_CAST_SUCCESS(ch);

       if ((IS_MAGE(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch),ROOM_MAGE)) ||
           (IS_CLERIC(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch),ROOM_CLERIC)) ||
           (IS_PALADINE(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch),ROOM_PALADINE)) ||
           (IS_MERCHANT(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch),ROOM_MERCHANT)))
          prob += 10;
       if (IS_MAGE(ch) && equip_in_metall(ch))
          prob -= 50;
       break;
 }

 prob = complex_spell_modifier(ch,spellnum,GAPPLY_SPELL_SUCCESS,prob);
 if (GET_GOD_FLAG(ch, GF_GODSCURSE) ||
     (SpINFO.violent  && victim && GET_GOD_FLAG(victim, GF_GODSLIKE)) ||
     (!SpINFO.violent && victim && GET_GOD_FLAG(victim, GF_GODSCURSE))
    )
    prob -= 50;
 if ((SpINFO.violent  && victim && GET_GOD_FLAG(victim, GF_GODSCURSE)) ||
     (!SpINFO.violent && victim && GET_GOD_FLAG(victim, GF_GODSLIKE))
    )
    prob += 50;
 return (prob > number(0,100));
}


/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to cast_spell().
 */
ACMD(do_cast)
{
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;
  char  *s, *t;
  int    spellnum, target = 0;

  if (IS_NPC(ch))
     return;

  if (AFF_FLAGGED(ch, AFF_SIELENCE))
     {send_to_char("Вы не смогли вымолвить и слова.\r\n",ch);
      return;
     }

  /* get: blank, spell name, target name */
  s = strtok(argument, "'*!");
  if (s == NULL)
     {send_to_char("ЧТО Вы хотите колдовать ?\r\n", ch);
      return;
     }
  s = strtok(NULL, "'*!");
  if (s == NULL)
     {send_to_char("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
      return;
     }
  t = strtok(NULL, "\0");

  spellnum = find_spell_num(s);

  /* Unknown spell */
  if (spellnum < 1 || spellnum > MAX_SPELLS)
     {send_to_char("И откуда Вы набрались таких выражений ?\r\n", ch);
      return;
     }

  /* Caster is lower than spell level */
  if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_TEMP | SPELL_KNOW) &&
      !IS_IMMORTAL(ch))
     {if (GET_LEVEL(ch) < SpINFO.min_level[(int) GET_CLASS(ch)] ||
          slot_for_char(ch,SpINFO.slot_forc[(int) GET_CLASS(ch)]) <= 0
         )
         {send_to_char("Рано еще Вам бросаться такими словами !\r\n", ch);
          return;
         }
      else
         {send_to_char("Было бы неплохо изучить, для начала, это заклинание...\r\n", ch);
          return;
         }
     }

  /* Caster havn't slot  */
  if (!GET_SPELL_MEM(ch,spellnum) && !IS_IMMORTAL(ch))
     {send_to_char("Вы совершенно не помните, как произносится это заклинание...\r\n", ch);
      return;
     }

  /* Find the target */
  if (t != NULL)
     {one_argument(strcpy(arg, t), t);
      skip_spaces(&t);
     }

  target = find_cast_target(spellnum,t,ch,&tch,&tobj);

  if (target && (tch == ch) && SpINFO.violent)
     {send_to_char("Лекари не рекомендуют использовать ЭТО на себя !\r\n", ch);
      return;
     }

  if (!target)
     {send_to_char("Тяжеловато найти цель Вашего заклинания !\r\n", ch);
      return;
     }

  /* You throws the dice and you takes your chances.. 101% is total failure */

  if (!spell_use_success(ch, tch, SAVING_SPELL, spellnum))
     { if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
          WAIT_STATE(ch, PULSE_VIOLENCE);
       if (GET_SPELL_MEM(ch, spellnum))
          GET_SPELL_MEM(ch,spellnum)--;
       if (!GET_SPELL_MEM(ch,spellnum))
          REMOVE_BIT(GET_SPELL_TYPE(ch,spellnum), SPELL_TEMP);
       if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, PRF_AUTOMEM))
          REMEMBER_SPELL(ch,spellnum);
       //log("[DO_CAST->AFFECT_TOTAL] Start");
       affect_total(ch);
       //log("[DO_CAST->AFFECT_TOTAL] Stop");
       if (!tch || !skill_message(0, ch, tch, spellnum))
          send_to_char("Вы не смогли сосредоточиться !\r\n", ch);
     }
  else
     { /* cast spell returns 1 on success; subtract mana & set waitstate */
       if (FIGHTING(ch) && !IS_IMPL(ch))
          {SET_CAST(ch,spellnum,tch,tobj);
           sprintf(buf,"Вы приготовились применить заклинание %s'%s'%s%s.\r\n",
                   CCCYN(ch,C_NRM),SpINFO.name,CCNRM(ch,C_NRM),
                   tch == ch ? " на себя" :
                   tch       ? " на $N3"  :
                   tobj      ? " на $O3"  : "" );
           act(buf,FALSE,ch,tobj,tch,TO_CHAR);
          }
       else
       if (cast_spell(ch, tch, tobj, spellnum) >= 0)
          {if (!(WAITLESS(ch) || CHECK_WAIT(ch)))
              WAIT_STATE(ch, PULSE_VIOLENCE);
          }
     }
}

ACMD(do_mixture)
{
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;
  char  *s, *t;
  int    spellnum, target = 0;

  if (IS_NPC(ch))
     return;

  /* get: blank, spell name, target name */
  s = strtok(argument, "'*!");
  if (!s)
     {if (subcmd == SCMD_RUNES)
         send_to_char("Что Вы хотите сложить ?\r\n", ch);
      else
      if (subcmd == SCMD_ITEMS)
         send_to_char("Что Вы хотите смешать ?\r\n", ch);
      return;
     }
  s = strtok(NULL, "'*!");
  if (!s)
     {send_to_char("Название вызываемой магии смеси должно быть заключено в символы : ' или * или !\r\n", ch);
      return;
     }
  t = strtok(NULL, "\0");

  spellnum = find_spell_num(s);

  /* Unknown spell */
  if (spellnum < 1 || spellnum > MAX_SPELLS)
     {send_to_char("И откуда Вы набрались рецептов ?\r\n", ch);
      return;
     }

  /* Caster is don't know this recipe */
  if (((!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_ITEMS) && subcmd == SCMD_ITEMS) ||
       (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_RUNES) && subcmd == SCMD_RUNES)) &&
      !IS_GOD(ch))
     {send_to_char("Это блюдо Вам явно не понравится.\r\n"
                   "Научитесь его правильно готовить.\r\n", ch);
      return;
     }

  if (!check_recipe_values(ch, spellnum, subcmd == SCMD_ITEMS ? SPELL_ITEMS : SPELL_RUNES, FALSE))
     return;

  if (!check_recipe_items(ch, spellnum, subcmd == SCMD_ITEMS ? SPELL_ITEMS : SPELL_RUNES, FALSE))
     {if (subcmd == SCMD_ITEMS)
         send_to_char("У Вас нет нужных инградиентов !", ch);
      else
      if (subcmd == SCMD_RUNES)
         send_to_char("У Вас нет нужных рун !", ch);
      return;
     }

  /* Find the target */
  if (t != NULL)
     {one_argument(strcpy(arg, t), t);
      skip_spaces(&t);
     }

  target = find_cast_target(spellnum,t,ch,&tch,&tobj);

  if (target && (tch == ch) && SpINFO.violent)
     {send_to_char("Лекари не рекомендуют использовать ЭТО на себя !\r\n", ch);
      return;
     }

  if (!target)
     {send_to_char("Тяжеловато найти цель для Вашей магии !\r\n", ch);
      return;
     }

  /* You throws the dice and you takes your chances.. 101% is total failure */

  if (check_recipe_items(ch,spellnum,subcmd==SCMD_ITEMS ? SPELL_ITEMS : SPELL_RUNES,TRUE))
     {if (!spell_use_success(ch, tch, SAVING_ROD, spellnum))
         { WAIT_STATE(ch, PULSE_VIOLENCE);
           if (!tch || !skill_message(0, ch, tch, spellnum))
              {if (subcmd == SCMD_ITEMS)
                  send_to_char("Вы неправильно смешали инградиенты !\r\n", ch);
               else
               if (subcmd == SCMD_RUNES)
                  send_to_char("Вы не смогли правильно истолковать значение рун !\r\n", ch);
              }
	
         }
      else
         { /* call magic returns 1 on success; set waitstate */
          if (call_magic(ch, tch, tobj, spellnum, GET_LEVEL(ch),
               subcmd == SCMD_ITEMS ? CAST_ITEMS : CAST_RUNES) >= 0)
              {if (!(WAITLESS(ch) || CHECK_WAIT(ch)))
                  WAIT_STATE(ch, PULSE_VIOLENCE);
              }
         }
     }
}


ACMD(do_create)
{ char   *s;
  int   spellnum, itemnum = 0, i;

  if (IS_NPC(ch))
     return;

  /* get: blank, spell name, target name */
  one_argument(argument, arg);

  if (!arg || !*arg)
     {if (subcmd == SCMD_RECIPE)
         send_to_char("Состав ЧЕГО Вы хотите узнать ?\r\n", ch);
      else
         send_to_char("ЧТО Вы хотите создать ?", ch);
      return;
     }

  i = strlen(arg);
  if (!strn_cmp(arg,"potion",i) || !strn_cmp(arg,"напиток",i))
     itemnum = SPELL_POTION;
  else
  if (!strn_cmp(arg,"wand",i) || !strn_cmp(arg,"палочка",i))
     itemnum = SPELL_WAND;
  else
  if (!strn_cmp(arg,"scroll",i) || !strn_cmp(arg,"свиток",i))
     itemnum = SPELL_SCROLL;
  else
  if (!strn_cmp(arg,"items",i) || !strn_cmp(arg,"смесь",i))
     {if (subcmd != SCMD_RECIPE)
         {send_to_char("Магическую смесь необходимо СМЕШАТЬ.\r\n",ch);
          return;
         }
      itemnum = SPELL_ITEMS;
     }
  else
  if (!strn_cmp(arg,"runes",i) || !strn_cmp(arg,"руны",i))
     {if (subcmd != SCMD_RECIPE)
         {send_to_char("Руны требуется сложить.\r\n",ch);
          return;
         }
      itemnum = SPELL_RUNES;
     }
  else
     {if (subcmd == SCMD_RECIPE)
         sprintf(buf, "Состав '%s' уже давно утерян.\r\n",arg);
      else
         sprintf(buf, "Создание '%s' доступно только Великим Богам.\r\n",arg);
      send_to_char(buf,ch);
      return;
     }

  s = strtok(argument, "'*!");
  if (s == NULL)
     {sprintf(buf,"Уточните тип состава !\r\n");
      send_to_char(buf, ch);
      return;
     }
  s = strtok(NULL, "'*!");
  if (s == NULL)
     {send_to_char("Название состава должно быть заключено в символы : ' или * или !\r\n", ch);
      return;
     }

  spellnum = find_spell_num(s);

  /* Unknown spell */
  if (spellnum < 1 || spellnum > MAX_SPELLS)
     {send_to_char("И откуда Вы набрались рецептов ?\r\n", ch);
      return;
     }

  /* Caster is don't know this recipe */
  if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), itemnum) &&
      !IS_IMMORTAL(ch))
     {send_to_char("Было бы неплохо прежде всего выучить этот состав.\r\n",ch);
      return;
     }

  if (subcmd == SCMD_RECIPE)
     {check_recipe_values(ch, spellnum, itemnum, TRUE);
      return;
     }

  if (!check_recipe_values(ch, spellnum, itemnum, FALSE))
     {send_to_char("Боги хранят в тайне этот состав.\r\n", ch);
      return;
     }

  if (!check_recipe_items(ch, spellnum, itemnum, TRUE))
     {send_to_char("У Вас нет нужных инградиентов !", ch);
      return;
     }
}


ACMD(do_learn)
{ struct obj_data *obj;
  int   spellnum, addchance;

  if (IS_NPC(ch))
     return;

  /* get: blank, spell name, target name */
  one_argument(argument, arg);

  if (!arg || !*arg)
     {send_to_char("Вы принялись внимательно изучать свои ногти. Да, пора бы и подстричь.\r\n", ch);
      act("$n удивленно уставил$u на свои ногти. Подстриг бы их кто-нибудь $m.", FALSE, ch, 0, 0, TO_ROOM);
      return;
     }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {send_to_char("А у Вас этого нет.\r\n", ch);
      return;
     }

  if (GET_OBJ_TYPE(obj) != ITEM_BOOK)
     {act("Вы уставились на $o3, как баран на новые ворота.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n начал$g внимательно изучать устройство $o1.", FALSE, ch, obj, 0, TO_ROOM);
      return;
     }

  if (slot_for_char(ch,1) <= 0)
     {send_to_char("Далась Вам эта магия ! Пошли-бы, водочки выпили...\r\n", ch);
      return;
     }

  if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) <= LAST_USED_SPELL)
     spellnum = GET_OBJ_VAL(obj,1);
  else
     {send_to_char("МАГИЯ НЕ ОПРЕДЕЛЕНА - сообщите Богам !\r\n",ch);
      return;
     }

  if (SpINFO.min_level[(int) GET_CLASS(ch)] > GET_LEVEL(ch)    ||
      slot_for_char(ch, SpINFO.slot_forc[(int) GET_CLASS(ch)]) <= 0
     )
     {sprintf(buf,"- \"Какие интересные буковки ! Особенно %s, похожая на %s\".\r\n"
              "Полюбовавшись еще несколько минут на сию красоту, Вы с чувством выполненного\r\n"
              "долга закрыли %s. До %s Вы еще не доросли.\r\n",
              number(0,1) ? "вон та" : number(0,1) ? "вот эта" : "пятая справа",
              number(0,1) ? "жука"   : number(0,1) ? "бабочку" : "русалку",
              obj->PNames[3],
              obj->obj_flags.Obj_sex == SEX_FEMALE ? "нее"  :
              obj->obj_flags.Obj_sex == SEX_POLY   ? "них"  : "него");
      send_to_char(buf,ch);
      act("$n с интересом осмотрел$g $o3, крякнул$g от досады и положил$g обратно.",
          FALSE, ch, obj, 0, TO_ROOM);
      return;
     }

  if (GET_SPELL_TYPE(ch,spellnum) & SPELL_KNOW)
     {sprintf(buf,"Вы открыли %s и принялись с интересом\r\n"
                  "изучать. Каким же было разочарование, когда прочитав %s,\r\n"
                  "Вы поняли, что это заклинание \"%s\".\r\n",
                  obj->PNames[3],
                  number(0,1) ? "несколько абзацев" :
                  number(0,1) ? "пару строк" : "почти до конца",
                  SpINFO.name);
      send_to_char(buf,ch);
      act("$n с интересом принял$g читать $o3.\r\n"
          "Постепенно $s интерес начал угасать, и $e, плюясь, сунул$g $o3 обратно.",
          FALSE, ch, obj, 0, TO_ROOM);
      return;
     }

  addchance = (IS_CLERIC(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_CLERIC)) ||
              (IS_MAGE(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_MAGE))     ||
              (IS_PALADINE(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PALADINE)) ||
              (IS_MERCHANT(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_MERCHANT))
              ? 10 : 0;

  if (number(1,100) <= int_app[POSI(GET_REAL_INT(ch))].spell_aknowlege + addchance ||
      GET_GOD_FLAG(ch, GF_GODSLIKE))
     {sprintf(buf,"Вы взяли в руки %s и начали изучать. Постепенно,\r\n"
                  "незнакомые доселе, буквы стали складываться в понятные слова и фразы.\r\n"
                  "Буквально через несколько минут Вы узнали секрет заклинания \"%s\".\r\n",
                  obj->PNames[3], SpINFO.name);
      send_to_char(buf,ch);
      GET_SPELL_TYPE(ch,spellnum) |= SPELL_KNOW;
     }
  else
     {sprintf(buf,"Вы взяли в руки %s и начали изучать. Непослушные\r\n"
                  "буквы никак не хотели выстраиваться в понятные и доступные фразы.\r\n"
                  "Промучившись несколько минут, Вы бросили это унылое занятие,\r\n"
                  "с удивлением отметив исчезновение %s.\r\n",
                  obj->PNames[3], obj->PNames[1]);
      send_to_char(buf,ch);
     }
  obj_from_char(obj);
}



void show_wizdom(struct char_data * ch, int bitset)
{char names[MAX_SLOT][MAX_STRING_LENGTH];
 int  slots[MAX_SLOT], i, max_slot, count, slot_num, is_full, gcount=0,
      imax_slot = 0;
 for (i = 1; i <= MAX_SLOT; i++)
     {*names[i-1] = '\0';
       slots[i-1] = 0;
       if (slot_for_char(ch,i))
          imax_slot = i;
     }
 if (bitset & 0x01)
   {is_full = 0;
    for (i = 1, max_slot = 0; i <= MAX_SPELLS; i++)
        {if (!GET_SPELL_TYPE(ch,i))
            continue;
         if (!spell_info[i].name || *spell_info[i].name == '!')
            continue;
         count    = GET_SPELL_MEM(ch,i);
         if (IS_IMMORTAL(ch))
             count = 10;
         if (!count)
            continue;
         slot_num = spell_info[i].slot_forc[(int) GET_CLASS(ch)] - 1;
         max_slot = MAX(slot_num, max_slot);
         slots[slot_num] += sprintf(names[slot_num]+slots[slot_num],
                                    "%2s|[%2d] %-31s|",
                                    slots[slot_num] % 80 < 10 ? "\r\n" : "  ",
                                    count,
                                    spell_info[i].name);
         is_full++;
        };

    gcount += sprintf(buf2+gcount,"  %sВы знаете следующие заклинания :%s",
                                  CCCYN(ch, C_NRM), CCNRM(ch, C_NRM)  );
    if (is_full)
       {for (i = 0; i < max_slot+1; i++)
            {if (slots[i])
                {gcount += sprintf(buf2+gcount,"\r\nКруг %d",i+1);
                 gcount += sprintf(buf2+gcount,"%s",names[i]);
                }
            }
       }
    else
      {gcount += sprintf(buf2+gcount,"\r\nСейчас у Вас нет заученных заклинаний.");
      }
    gcount += sprintf(buf2+gcount,"\r\n");
   }
 if (bitset & 0x02)
   {is_full = 0;
    for (i = 0; i < MAX_SLOT; i++)
        {*names[i]='\0'; slots[i] = 0;}
    for (i = 0; i <= MAX_SPELLS; i++)
        {if ((count = ch->Memory[i]) == 0)
            continue;
         slot_num         = spell_info[i].slot_forc[(int) GET_CLASS(ch)] - 1;
         slots[slot_num] += sprintf(names[slot_num]+slots[slot_num],
                                    "%2s|[%2d] %-31s|",
                                    slots[slot_num] % 80 < 10 ? "\r\n" : "  ",
                                    count,spell_info[i].name);
         is_full++;
        };
    gcount += sprintf(buf2+gcount,"  %sВы запоминаете следующие заклинания :%s",
                                  CCCYN(ch, C_NRM), CCNRM(ch,C_NRM) );
    if (is_full)
       {for (i = 0; i < imax_slot; i++)
            {if (slots[i])
                {gcount += sprintf(buf2+gcount,"\r\nКруг %d",i+1);
                 gcount += sprintf(buf2+gcount,"%s",names[i]);
                }
            }
       }
    else
      gcount += sprintf(buf2+gcount,"\r\nВы ничего не запоминаете.");
    gcount += sprintf(buf2+gcount,"\r\n");
   }
 if (bitset & 0x04)
   {is_full = 0;
    for (i = 0; i < MAX_SLOT; i++)
        slots[i] = 0;
    for (i = 0; i < MAX_SPELLS; i++)
        {if (!IS_SET(GET_SPELL_TYPE(ch,i),SPELL_KNOW))
            continue;
         slot_num         = spell_info[i].slot_forc[(int) GET_CLASS(ch)] - 1;
         slots[slot_num] += ch->Memory[i] + GET_SPELL_MEM(ch,i);
        };
    if (imax_slot)
       {gcount += sprintf(buf2+gcount,"  %sСвободно :%s\r\n",
                          CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
        for (i = 0; i < imax_slot; i++)
            {slot_num = MAX(0,slot_for_char(ch,i+1) - slots[i]);
             gcount += sprintf(buf2+gcount,"%s%2d-%2d%s  ",
                               slot_num ? CCICYN(ch,C_NRM) : "",
			       i+1,slot_num,
			       slot_num ? CCNRM(ch,C_NRM) : "");
            }
       }
   }
 page_string(ch->desc, buf2, 1);
}


ACMD(do_remember)
{ char *s;
  int spellnum, i, slotnum, slotcnt;

  /* get: blank, spell name, target name */
  if (!argument || !(*argument))
     {show_wizdom(ch,0x07);
      return;
     }
  if (IS_IMMORTAL(ch))
     {send_to_char("Господи, хоть ты не подкалывай !", ch);
      return;
     }
  s = strtok(argument, "'*!");
  if (s == NULL)
     {send_to_char("Какое заклинание Вы хотите заучить ?\r\n", ch);
      return;
     }
  s = strtok(NULL, "'*!");
  if (s == NULL)
     {send_to_char("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
      return;
     }
  spellnum = find_spell_num(s);

  if (spellnum < 1 || spellnum > MAX_SPELLS)
     {send_to_char("И откуда Вы набрались таких выражений ?\r\n", ch);
      return;
     }
  /* Caster is lower than spell level */
  if (GET_LEVEL(ch) < SpINFO.min_level[(int) GET_CLASS(ch)] ||
      slot_for_char(ch,SpINFO.slot_forc[(int) GET_CLASS(ch)]) <= 0)
     {send_to_char("Рано еще Вам бросаться такими словами !\r\n", ch);
      return;
     };
  if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW))
     {send_to_char("Было бы неплохо изучить, для начала, это заклинание...\r\n", ch);
      return;
     }
  slotnum = SpINFO.slot_forc[(int) GET_CLASS(ch)];
  slotcnt = slot_for_char(ch,slotnum);
  for (i = 1; i <= MAX_SPELLS; i++)
      {if (slotnum != spell_info[i].slot_forc[(int) GET_CLASS(ch)])
          continue;
       if ((slotcnt -= (GET_SPELL_MEM(ch,i) + ch->Memory[i])) <= 0)
          {send_to_char("У Вас нет свободных ячеек этого круга.", ch);
           return;
          }
      };
  REMEMBER_SPELL(ch,spellnum);
  if (GET_RELIGION(ch) == RELIGION_MONO)
     sprintf(buf,"Вы дописали заклинание \"%s%s%s\" в свой часослов.\r\n",
             CCIMAG(ch,C_NRM),SpINFO.name,CCNRM(ch,C_NRM));
  else
     sprintf(buf,"Вы занесли заклинание \"%s%s%s\" в свои резы.\r\n",
             CCIMAG(ch,C_NRM),SpINFO.name,CCNRM(ch,C_NRM));
  send_to_char(buf,ch);
  return;
}

ACMD(do_forget)
{ char *s, *t;
  int spellnum, in_mem;

  /* get: blank, spell name, target name */
  if (IS_IMMORTAL(ch))
     {send_to_char("Господи, тебе лень набрать skillset ?\r\n", ch);
      return;
     }
  s = strtok(argument, "'*!");
  if (s == NULL)
     {send_to_char("Какое заклинание Вы хотите забыть ?\r\n", ch);
      return;
     }
  s = strtok(NULL, "'*!");
  if (s == NULL)
     {send_to_char("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
      return;
     }
  spellnum = find_spell_num(s);
  /* Unknown spell */
  if (spellnum < 1 || spellnum > MAX_SPELLS)
     {send_to_char("И откуда Вы набрались таких выражений ?\r\n", ch);
      return;
     }
  if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW | SPELL_TEMP))
     {send_to_char("Трудно забыть то, чего не знаешь...\r\n", ch);
      return;
     }
  t      = strtok(NULL, "\0");
  in_mem = 0;
  if (t != NULL)
     {one_argument(strcpy(arg, t), t);
      skip_spaces(&t);
      in_mem = (!strn_cmp("часослов",t,strlen(t)) ||
                !strn_cmp("резы",t,strlen(t)) ||
                !strn_cmp("book",t,strlen(t)));
     }
  if (!in_mem)
     if (!GET_SPELL_MEM(ch,spellnum))
        {send_to_char("Прежде чем забыть что-то ненужное, следует заучить что-то ненужное...\r\n",ch);
         return;
        }
     else
        {GET_SPELL_MEM(ch,spellnum)--;
         if (!GET_SPELL_MEM(ch,spellnum))
            REMOVE_BIT(GET_SPELL_TYPE(ch,spellnum), SPELL_TEMP);
         //log("[DO_FORGET->AFFECT_TOTAL] Start");
         affect_total(ch);
         //log("[DO_FORGET->AFFECT_TOTAL] Stop");
         sprintf(buf,"Вы удалили заклинание '%s%s%s' из %s.\r\n",
                 CCICYN(ch,C_NRM),
                 SpINFO.name,
                 CCNRM(ch,C_NRM),
                 GET_RELIGION(ch) == RELIGION_MONO ? "своего часослова" :
                                                     "своих рез");
         send_to_char(buf,ch);
        }
  else
     if (ch->Memory[spellnum])
        {ch->Memory[spellnum]--;
         GET_MANA_NEED(ch) -= MIN(GET_MANA_NEED(ch), mag_manacost(ch,spellnum));
         if (!GET_MANA_NEED(ch))
            GET_MANA_STORED(ch) = 0;
         sprintf(buf,"Вы вычеркнули заклинание '%s' из списка для запоминания.\r\n",
                 SpINFO.name);
         send_to_char(buf,ch);
        }
     else
        send_to_char("Вы и ни собирались заучить это заклинание.\r\n",ch);
}

void mspell_level(int spell, int chclass, int level)
{
  int bad = 0, i;

  if (spell < 0 || spell > TOP_SPELL_DEFINE)
  { log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
    return;
  }

  if (chclass <= 0 || chclass >= 1 << NUM_CLASSES)
  { log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell),
                chclass, NUM_CLASSES - 1);
    bad = 1;
  }

  if (level < 1 || level > LVL_IMPL)
     { log("SYSERR: assigning '%s' to illegal level %d/%d.", skill_name(spell),
           level, LVL_IMPL);
       bad = 1;
     }

  if (!bad)
     for (i = 0; i < NUM_CLASSES; i++)
         if (chclass & (1 << i))
            spell_info[spell].min_level[i] = level;
}


void mspell_slot(int spell, int chclass, int slot)
{
  int bad = 0, i;

  if (spell < 0 || spell > TOP_SPELL_DEFINE)
  { log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
    return;
  }

  if (chclass <= 0 || chclass >= 1 << NUM_CLASSES)
  { log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell),
                chclass, NUM_CLASSES - 1);
    bad = 1;
  }

  if (slot < 1 || slot > MAX_SLOT)
     { log("SYSERR: assigning '%s' to illegal slot %d/%d.", skill_name(spell),
           slot, LVL_IMPL);
       bad = 1;
     }

  if (!bad)
     for (i = 0; i < NUM_CLASSES; i++)
         if (chclass & (1 << i))
            spell_info[spell].slot_forc[i] = slot;
}


/* Assign the spells on boot up */
void spello(int spl, const char *name, const char *syn,
            int max_mana, int min_mana, int mana_change,
            int minpos, int targets, int violent, int routines, int danger)
{
  int i;
  for (i = 0; i < NUM_CLASSES; i++)
    {spell_info[spl].min_level[i] = LVL_IMPL;
     spell_info[spl].slot_forc[i] = MAX_SLOT;
    }


  spell_info[spl].mana_max      = max_mana;
  spell_info[spl].mana_min      = min_mana;
  spell_info[spl].mana_change   = mana_change;
  spell_info[spl].min_position  = minpos;
  spell_info[spl].danger        = danger;
  spell_info[spl].targets       = targets;
  spell_info[spl].violent       = violent;
  spell_info[spl].routines      = routines;
  spell_info[spl].name          = name;
  spell_info[spl].syn           = syn;
}

void unused_spell(int spl)
{
  int i;
  for (i = 0; i < NUM_CLASSES; i++)
       {spell_info[spl].min_level[i] = LVL_IMPL + 1;
        spell_info[spl].slot_forc[i] = MAX_SLOT;
       }

  for (i = 0; i < 3; i++)
      {spell_create[spl].wand.items[i]   = -1;
       spell_create[spl].scroll.items[i] = -1;
       spell_create[spl].potion.items[i] = -1;
       spell_create[spl].items.items[i]  = -1;
       spell_create[spl].runes.items[i]  = -1;
      }

  spell_create[spl].wand.rnumber    = -1;
  spell_create[spl].scroll.rnumber  = -1;
  spell_create[spl].potion.rnumber  = -1;
  spell_create[spl].items.rnumber   = -1;
  spell_create[spl].runes.rnumber   = -1;

  spell_info[spl].mana_max     = 0;
  spell_info[spl].mana_min     = 0;
  spell_info[spl].mana_change  = 0;
  spell_info[spl].min_position = 0;
  spell_info[spl].danger       = 0;
  spell_info[spl].targets      = 0;
  spell_info[spl].violent      = 0;
  spell_info[spl].routines     = 0;
  spell_info[spl].name         = unused_spellname;
  spell_info[spl].syn          = unused_spellname;
}

void skillo(int spl, const char *name, int max_percent)
{
  int i;
  for (i = 0; i < NUM_CLASSES; i++)
    skill_info[spl].k_improove[i] = 1;
  skill_info[spl].min_position    = 0;
  skill_info[spl].name            = name;
  skill_info[spl].max_percent     = max_percent;
}

void unused_skill(int spl)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
      skill_info[spl].k_improove[i] = 0;
  skill_info[spl].min_position      = 0;
  skill_info[spl].max_percent       = 200;
  skill_info[spl].name              = unused_spellname;
}

/*
 * Arguments for spello calls:
 *
 * spellnum, maxmana, minmana, manachng, minpos, targets, violent?, routines.
 *
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 * spells.h (such as SPELL_HEAL).
 *
 * spellname: The name of the spell.
 *
 * maxmana :  The maximum mana this spell will take (i.e., the mana it
 * will take when the player first gets the spell).
 *
 * minmana :  The minimum mana this spell will take, no matter how high
 * level the caster is.
 *
 * manachng:  The change in mana for the spell from level to level.  This
 * number should be positive, but represents the reduction in mana cost as
 * the caster's level increases.
 *
 * minpos  :  Minimum position the caster must be in for the spell to work
 * (usually fighting or standing). targets :  A "list" of the valid targets
 * for the spell, joined with bitwise OR ('|').
 *
 * violent :  TRUE or FALSE, depending on if this is considered a violent
 * spell and should not be cast in PEACEFUL rooms or on yourself.  Should be
 * set on any spell that inflicts damage, is considered aggressive (i.e.
 * charm, curse), or is otherwise nasty.
 *
 * routines:  A list of magic routines which are associated with this spell
 * if the spell uses spell templates.  Also joined with bitwise OR ('|').
 *
 * See the CircleMUD documentation for a more detailed description of these
 * fields.
 */

/*
 * NOTE: SPELL LEVELS ARE NO LONGER ASSIGNED HERE AS OF Circle 3.0 bpl9.
 * In order to make this cleaner, as well as to make adding new classes
 * much easier, spell levels are now assigned in class.c.  You only need
 * a spello() call to define a new spell; to decide who gets to use a spell
 * or skill, look in class.c.  -JE 5 Feb 1996
 */

void mag_assign_spells(void)
{
  int i;

  /* Do not change the loop below. */
  for (i = 0; i <= TOP_SPELL_DEFINE; i++)
      { unused_spell(i);
      }
  for (i = 0; i <= TOP_SKILL_DEFINE; i++)
      { unused_skill(i);
      }


  /* Do not change the loop above. */

//1
  spello(SPELL_ARMOR, "защита", "armor",
         40, 30, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//2
  spello(SPELL_TELEPORT, "прыжок", "teleport",
         140, 120, 2, POS_STANDING,
         TAR_CHAR_ROOM, FALSE,
         MAG_MANUAL | NPC_DAMAGE_PC, 1);
//3
  spello(SPELL_BLESS, "доблесть", "bless",
         55, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_OBJ_INV, FALSE,
         MAG_AFFECTS | MAG_ALTER_OBJS | NPC_AFFECT_NPC, 0);
//4
  spello(SPELL_BLINDNESS, "слепота", "blind",
         70, 40, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 1);
//5
  spello(SPELL_BURNING_HANDS, "горящие руки", "burning hands",
         40, 30, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_FIRE,
         MAG_DAMAGE | NPC_DAMAGE_PC,1);
//6
  spello(SPELL_CALL_LIGHTNING, "шаровая молния", "call lightning",
         85, 70, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AIR,
         MAG_DAMAGE | NPC_DAMAGE_PC, 2);
//7
  spello(SPELL_CHARM, "подчинить разум", "mind control",
         55, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, MTYPE_NEUTRAL, MAG_MANUAL, 1);
//8
  spello(SPELL_CHILL_TOUCH, "ледяное прикосновение", "chill touch",
         55, 45, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC,1);
//9
  spello(SPELL_CLONE, "клонирование", "clone",
         150, 130, 5, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_SUMMONS, 0);
//10
  spello(SPELL_COLOR_SPRAY, "огненная стрела", "fire missle",
         90, 75, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_FIRE,
         MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 3);
//11
  spello(SPELL_CONTROL_WEATHER, "контроль погоды", "weather control",
         100, 90, 1, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_MANUAL, 0);
//12
  spello(SPELL_CREATE_FOOD, "создать пищу", "create food",
         40, 30, 1, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_CREATIONS, 0);
//13
  spello(SPELL_CREATE_WATER, "создать воду", "create water",
         40, 30, 1, POS_STANDING,
         TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_CHAR_ROOM, FALSE, MAG_MANUAL, 0);
//14
  spello(SPELL_CURE_BLIND, "вылечить слепоту", "cure blind",
         110, 90, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_UNAFFECTS | NPC_UNAFFECT_NPC, 0);
//15
  spello(SPELL_CURE_CRITIC, "критическое исцеление", "critical cure",
         100, 90, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_POINTS | NPC_DUMMY, 3);
//16
  spello(SPELL_CURE_LIGHT, "легкое исцеление", "light cure",
         40, 30, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_POINTS | NPC_DUMMY,1);
//17
  spello(SPELL_CURSE, "проклятье", "curse",
         55, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_INV,  MTYPE_NEUTRAL,
         MAG_AFFECTS | MAG_ALTER_OBJS | NPC_AFFECT_PC, 1);
//18
  spello(SPELL_DETECT_ALIGN, "определение наклонностей", "detect alignment",
         40, 30, 1, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0);
//19
  spello(SPELL_DETECT_INVIS, "видеть невидимое", "detect invisible",
         100, 55, 3, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0);
//20
  spello(SPELL_DETECT_MAGIC, "определение магии", "detect magic",
         100, 55, 3, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0);
//21
  spello(SPELL_DETECT_POISON, "определение яда", "detect poison",
         100, 55, 3, POS_STANDING,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL, 0);
//22
  spello(SPELL_DISPEL_EVIL, "изгнать зло", "dispel evil",
         100, 90, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL, MAG_DAMAGE, 1);
//23
  spello(SPELL_EARTHQUAKE, "землетрясение", "earthquake",
         110, 90, 2, POS_FIGHTING,
         TAR_IGNORE, MTYPE_EARTH,
         MAG_AREAS | NPC_DAMAGE_PC, 2);
//24
  spello(SPELL_ENCHANT_WEAPON, "заколдовать оружие", "enchant weapon",
         140, 110, 2, POS_STANDING,
         TAR_OBJ_INV, FALSE, MAG_MANUAL, 0);
//25
  spello(SPELL_ENERGY_DRAIN, "истощить энергию", "energy drain",
         150, 140, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 1);
//26
  spello(SPELL_FIREBALL, "огненный шар", "fireball",
         110, 100, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_FIRE,
         MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2);
//27
  spello(SPELL_HARM, "вред", "harm",
         110, 100, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_DAMAGE_PC, 5);
//28
  spello(SPELL_HEAL, "исцеление", "heal",
         110, 100, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_POINTS | MAG_UNAFFECTS | NPC_DUMMY, 10);
//29
  spello(SPELL_INVISIBLE, "невидимость", "invisible",
         50, 40, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM,
         FALSE, MAG_AFFECTS | MAG_ALTER_OBJS, 0);
//30
  spello(SPELL_LIGHTNING_BOLT, "молния", "lightning bolt",
         55, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_FIRE,
         MAG_DAMAGE | NPC_DAMAGE_PC, 1);
//31
  spello(SPELL_LOCATE_OBJECT, "разыскать предмет", "locate object",
         140, 110, 2, POS_STANDING,
         TAR_OBJ_WORLD, FALSE, MAG_MANUAL, 0);
//32
  spello(SPELL_MAGIC_MISSILE, "магическая стрела", "magic missle",
         40, 30, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_DAMAGE_PC, 1);
//33
  spello(SPELL_POISON, "яд", "poison",
         70, 55, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | MAG_ALTER_OBJS | NPC_AFFECT_PC, 2);
//34
  spello(SPELL_PROT_FROM_EVIL, "защита от тьмы", "protect evil",
         60, 45, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//35
  spello(SPELL_REMOVE_CURSE, "снять проклятье", "remove curse",
         50, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE,
         MAG_UNAFFECTS | MAG_ALTER_OBJS | NPC_UNAFFECT_NPC, 0);
//36
  spello(SPELL_SANCTUARY, "освящение", "sanctuary",
         85, 70, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS   | NPC_AFFECT_NPC, 1);
//37
  spello(SPELL_SHOCKING_GRASP, "обжигающая хватка", "shocking grasp",
         50, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_FIRE,
         MAG_DAMAGE | NPC_DAMAGE_PC, 1);
//38
  spello(SPELL_SLEEP, "сон", "sleep",
         70, 55, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
         MAG_AFFECTS | NPC_AFFECT_PC, 0);
//39
  spello(SPELL_STRENGTH, "сила", "strength",
         40, 30, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//40
  spello(SPELL_SUMMON, "призвать", "summon",
         140, 120, 2, POS_STANDING,
         TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL, 0);
//41-not used now
//42
  spello(SPELL_WORD_OF_RECALL, "слово возврата", "recall",
         140, 100, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_MANUAL | NPC_DAMAGE_PC, 0);
//43
  spello(SPELL_REMOVE_POISON, "удалить яд", "remove poison",
         60, 45, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE,
         MAG_UNAFFECTS | MAG_ALTER_OBJS | NPC_UNAFFECT_NPC, 0);
//44
  spello(SPELL_SENSE_LIFE, "определение жизни", "sense life",
         85, 70, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
//45
  spello(SPELL_ANIMATE_DEAD, "поднять труп", "animate dead",
         50, 35, 3, POS_STANDING,
         TAR_OBJ_ROOM, FALSE, MAG_SUMMONS, 0);
//46
  spello(SPELL_DISPEL_GOOD, "рассеять свет", "dispel good",
         100, 90, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL, MAG_DAMAGE, 1);
//47
  spello(SPELL_GROUP_ARMOR, "групповая защита", "group armor",
         110, 100, 2, POS_FIGHTING,
         TAR_IGNORE, FALSE,
         MAG_GROUPS | NPC_AFFECT_NPC, 0);
//48
  spello(SPELL_GROUP_HEAL, "групповое исцеление", "group heal",
         110, 100, 2, POS_FIGHTING,
         TAR_IGNORE, FALSE,
         MAG_GROUPS | NPC_DUMMY, 30);
//49
  spello(SPELL_GROUP_RECALL, "групповой возврат", "group recall",
         125, 120, 2, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL, 0);
//50
  spello(SPELL_INFRAVISION, "видение ночью", "infravision",
         50, 40, 2, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0);
//51
  spello(SPELL_WATERWALK, "водохождение", "waterwalk",
         70, 55, 1, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0);
//52
  spello(SPELL_CURE_SERIOUS, "серьезное исцеление", "serious cure",
         85, 70, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_POINTS | NPC_DUMMY, 2);
//53
  spello(SPELL_GROUP_STRENGTH, "групповая сила", "group strength",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE,
         MAG_GROUPS | NPC_AFFECT_NPC, 0);
//54
  spello(SPELL_HOLD, "оцепенение", "hold",
         100, 40, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 3);
//55
  spello(SPELL_POWER_HOLD, "длительное оцепенение", "power hold",
         140, 90, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 4);
//56
  spello(SPELL_MASS_HOLD, "массовое оцепенение", "mass hold",
         150, 130, 5, POS_FIGHTING,
         TAR_IGNORE,  MTYPE_NEUTRAL,
         MAG_MASSES | NPC_AFFECT_PC, 5);
//57
  spello(SPELL_FLY, "полет", "fly",
         50, 35, 1, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0);
//58-not used now

//59
  spello(SPELL_NOFLEE, "приковать противника", "noflee",
         100, 90, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 0);
//60
  spello(SPELL_CREATE_LIGHT, "создать свет", "create light",
         40, 30, 1, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_CREATIONS, 0);
//61
  spello(SPELL_DARKNESS, "тьма", "darkness",
         100, 70, 2, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,0);
//62
  spello(SPELL_STONESKIN, "каменная кожа", "stoneskin",
         55, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//63
  spello(SPELL_CLOUDLY, "затуманивание", "cloudly",
         55, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//64
  spello(SPELL_SIELENCE, "молчание", "sielence",
         100, 40, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 1);
//65
  spello(SPELL_LIGHT, "свет", "sun shine",
         100, 70, 2, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0);
//66
  spello(SPELL_CHAIN_LIGHTNING, "цепь молний", "chain lightning",
         120, 110, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AIR,
         MAG_MANUAL | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,1);
//67
  spello(SPELL_FIREBLAST, "огненный поток", "fireblast",
         110, 90, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_FIRE,
         MAG_AREAS | NPC_DAMAGE_PC, 5);
//68
  spello(SPELL_IMPLOSION, "гнев богов", "implosion",
         140, 120, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_WATER,
         MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,15);
//69
  spello(SPELL_WEAKNESS, "слабость", "weakness",
         70, 55, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 0);
//70
  spello(SPELL_GROUP_INVISIBLE, "групповая невидимость", "group invisible",
         150, 130, 5, POS_STANDING, TAR_IGNORE,
         FALSE, MAG_GROUPS, 0);
//71-not used now

//72
  spello(SPELL_ACID, "кислота", "acid",
         90, 65, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | MAG_ALTER_OBJS | NPC_DAMAGE_PC,2);
//73
  spello(SPELL_REPAIR, "починка", "repair",
         110, 100, 1, POS_STANDING,
         TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_ALTER_OBJS, 0);
//74
  spello(SPELL_ENLARGE, "увеличение", "enlarge",
         55, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//75
  spello(SPELL_FEAR, "страх", "fear",
         70, 55, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
         MAG_DAMAGE | NPC_DAMAGE_PC,1);
//76
  spello(SPELL_SACRIFICE, "высосать жизнь", "sacrifice",
         140, 125, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,10);
//77
  spello(SPELL_WEB, "сеть", "web",
         70, 55, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 1);
//78
  spello(SPELL_BLINK, "мигание", "blink",
         70, 55, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_SELF_ONLY, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//79
  spello(SPELL_REMOVE_HOLD, "снять оцепенение", "remove hold",
         110, 90, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_UNAFFECTS | NPC_UNAFFECT_NPC, 1);
//80
  spello(SPELL_CAMOUFLAGE, "!маскировка!", "!set by skill!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//81
  spello(SPELL_POWER_BLINDNESS, "полная слепота", "power blind",
         110, 100, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 2);
//82
  spello(SPELL_MASS_BLINDNESS, "массовая слепота", "mass blind",
         140, 120, 2, POS_FIGHTING,
         TAR_IGNORE,  MTYPE_NEUTRAL,
         MAG_MASSES | NPC_AFFECT_PC, 2);
//83
  spello(SPELL_POWER_SIELENCE, "длительное молчание", "power sielence",
         120, 90, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 2);
//84
  spello(SPELL_EXTRA_HITS, "увеличить жизнь", "extra hits",
         100, 85, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_POINTS | NPC_DUMMY,1);
//85
  spello(SPELL_RESSURECTION, "оживить труп", "ressurection",
         120, 100, 2, POS_STANDING,
         TAR_OBJ_ROOM, FALSE, MAG_SUMMONS, 0);
//86-not used now

//87
  spello(SPELL_FORBIDDEN, "запечатать комнату", "forbidden",
         125, 110, 2, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_AREAS, 0);
//88
  spello(SPELL_MASS_SIELENCE, "массовое молчание", "mass sielence",
         140, 120, 2, POS_FIGHTING,
         TAR_IGNORE,  MTYPE_NEUTRAL,
         MAG_MASSES | NPC_AFFECT_PC, 2);
//89
  spello(SPELL_REMOVE_SIELENCE, "снять молчание", "remove sielence",
         70, 55, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_UNAFFECTS | NPC_UNAFFECT_NPC | NPC_UNAFFECT_NPC_CASTER, 1);
//90
  spello(SPELL_DAMAGE_LIGHT, "легкий вред", "light damage",
         40, 30, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_DAMAGE_PC,1);
//91
  spello(SPELL_DAMAGE_SERIOUS, "серьезный вред", "serious damage",
         70, 55, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_DAMAGE_PC, 2);
//92
  spello(SPELL_DAMAGE_CRITIC, "критический вред", "critical damage",
         85, 70, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_DAMAGE_PC, 3);
//93
  spello(SPELL_MASS_CURSE, "массовое проклятье", "mass curse",
         140, 120, 2, POS_FIGHTING,
         TAR_IGNORE,  MTYPE_NEUTRAL,
         MAG_MASSES | NPC_AFFECT_PC, 2);
//94
  spello(SPELL_ARMAGEDDON, "суд богов", "armageddon",
         150, 130, 5, POS_FIGHTING,
         TAR_IGNORE, MTYPE_AIR,
         MAG_AREAS | NPC_DAMAGE_PC, 10);
//95
  spello(SPELL_GROUP_FLY, "групповой полет", "group fly",
         140, 120, 2, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_GROUPS, 0);
//96
  spello(SPELL_GROUP_BLESS, "групповая доблесть", "group bless",
         110, 100, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE,
         MAG_GROUPS | NPC_AFFECT_NPC, 1);
//97
  spello(SPELL_REFRESH, "восстановление", "refresh",
         80, 60, 1, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_POINTS,0);
//98
  spello(SPELL_STUNNING, "каменное проклятье", "stunning",
         150, 140, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL, MAG_DAMAGE,15);

//99
  spello(SPELL_HIDE, "!спрятался!", "!set by skill!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//100
  spello(SPELL_SNEAK, "!крадется!", "!set by skill!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//101
  spello(SPELL_DRUNKED, "!опьянение!", "!set by programm!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//102
  spello(SPELL_ABSTINENT, "!абстиненция!", "!set by programm!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//103
  spello(SPELL_FULL, "насыщение", "full",
         70, 55, 1, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_POINTS, 10);
//104
  spello(SPELL_CONE_OF_COLD, "ледяной ветер", "cold wind",
         100, 90, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_WATER,
         MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,15);
//105
  spello(SPELL_BATTLE, "!получил в бою!", "!set by programm!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//106
  spello(SPELL_HAEMORRAGIA, "!кровотечение!", "!set by programm!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//107
  spello(SPELL_COURAGE, "!ярость!", "!set by programm!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//108
  spello(SPELL_WATERBREATH, "дышать водой", "waterbreath",
         100, 90, 1, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0);
//109
  spello(SPELL_SLOW, "медлительность", "slow",
         55, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 1);
//110
  spello(SPELL_HASTE, "ускорение", "haste",
         55, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//111
  spello(SPELL_MASS_SLOW, "массовая медлительность", "mass slow",
         140, 120, 2, POS_FIGHTING,
         TAR_IGNORE,  MTYPE_NEUTRAL,
         MAG_MASSES | NPC_AFFECT_PC, 2);
//112
  spello(SPELL_GROUP_HASTE, "групповое ускорение", "group haste",
         110, 100, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE,
         MAG_GROUPS | NPC_AFFECT_NPC, 1);
//113
  spello(SPELL_SHIELD, "защита богов", "gods shield",
         150, 140, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 2);
//114
  spello(SPELL_PLAQUE, "лихорадка", "plaque",
         70, 55, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 2);
//115
  spello(SPELL_CURE_PLAQUE, "вылечить лихорадку", "cure plaque",
         110, 90, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_UNAFFECTS | NPC_UNAFFECT_NPC, 0);
//116
  spello(SPELL_AWARNESS, "внимательность", "awarness",
         100, 90, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
//117
  spello(SPELL_RELIGION, "!молитва или жертва!", "!pray or donate!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//118
  spello(SPELL_AIR_SHIELD, "воздушный щит", "air shield",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//119
  spello(SPELL_PORTAL, "переход", "portal",
         140, 120, 2, POS_STANDING,
         TAR_CHAR_WORLD, FALSE, MAG_MANUAL, 0);
//120
  spello(SPELL_DISPELL_MAGIC, "развеять магию", "dispel magic",
         85, 70, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS, 0);
//121
  spello(SPELL_SUMMON_KEEPER, "защитник", "keeper",
         100, 80, 2, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS, 0);
//122
  spello(SPELL_FAST_REGENERATION, "быстрое восстановление", "fast regeneration",
         85, 70, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//123
  spello(SPELL_CREATE_WEAPON, "создать оружие", "create weapon",
         130, 110, 2, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_MANUAL, 0);
//124
  spello(SPELL_FIRE_SHIELD, "огненный щит", "fire shield",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//125
  spello(SPELL_RELOCATE, "переместиться", "relocate",
         140, 120, 2, POS_STANDING,
         TAR_CHAR_WORLD, FALSE, MAG_MANUAL, 0);
//126
  spello(SPELL_SUMMON_FIREKEEPER, "огненный защитник", "fire keeper",
         150, 140, 1, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS, 0);
//127
  spello(SPELL_ICE_SHIELD, "ледяной щит", "ice protect",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//128
  spello(SPELL_ICESTORM, "ледяной шторм", "ice storm",
         125, 110, 2, POS_FIGHTING,
         TAR_IGNORE, MTYPE_WATER,
         MAG_AREAS | NPC_DAMAGE_PC, 5);
//129
  spello(SPELL_ENLESS, "уменьшение", "enless",
         55, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS, 0);
//130
  spello(SPELL_SHINEFLASH, "яркий блик", "shine flash",
         60, 45, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_FIRE,
         MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2);
//131
  spello(SPELL_MADNESS, "безумие", "madness",
         130, 110, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 1);
//132
  spello(SPELL_MAGICGLASS, "магическое зеркало", "magic glass",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//133-not used now

//134
  spello(SPELL_VACUUM, "круг пустоты", "vacuum sphere",
         150, 140, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,15);
//135
  spello(SPELL_METEORSTORM, "метеоритный дождь", "meteor storm",
         125, 110, 2, POS_FIGHTING,
         TAR_IGNORE, MTYPE_EARTH,
         MAG_AREAS | NPC_DAMAGE_PC, 5);
//136
  spello(SPELL_STONEHAND, "каменные руки", "stonehand",
         40, 30, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//137
  spello(SPELL_MINDLESS, "повреждение разума", "mindness",
         120, 110, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 0);
//138
  spello(SPELL_PRISMATICAURA, "призматическая аура", "prismatic aura",
         85, 70, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 1);
//139
  spello(SPELL_EVILESS, "силы зла", "eviless",
         150, 130, 5, POS_STANDING, TAR_IGNORE, FALSE,
	 MAG_MANUAL, 0);
//140
  spello(SPELL_AIR_AURA, "воздушная аура", "air aura",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//141
  spello(SPELL_FIRE_AURA, "огненная аура", "fire aura",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//142
  spello(SPELL_ICE_AURA, "ледяная аура", "ice aura",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);

  /* NON-castable spells should appear below here. */

  spello(SPELL_IDENTIFY, "идентификация", "identify",
         0, 0, 0, 0,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL, 0);

  /*
   * These spells are currently not used, not implemented, and not castable.
   * Values for the 'breath' spells are filled in assuming a dragon's breath.
   */

  spello(SPELL_FIRE_BREATH, "огненное дыхание", "fire breath",
         0, 0, 0, POS_SITTING,
             TAR_IGNORE, TRUE, 0, 0);

  spello(SPELL_GAS_BREATH, "зловонное дыхание", "gas breath",
         0, 0, 0, POS_SITTING,
             TAR_IGNORE, TRUE, 0, 0);

  spello(SPELL_FROST_BREATH, "ледяное дыхание", "frost breath",
         0, 0, 0, POS_SITTING,
             TAR_IGNORE, TRUE, 0, 0);

  spello(SPELL_ACID_BREATH, "кислотное дыхание", "acid breath",
         0, 0, 0, POS_SITTING,
             TAR_IGNORE, TRUE, 0, 0);

  spello(SPELL_LIGHTNING_BREATH, "опаляющее дыхание", "lightning breath",
         0, 0, 0, POS_SITTING,
             TAR_IGNORE, TRUE, 0, 0);



  /*
   * Declaration of skills - this actually doesn't do anything except
   * set it up so that immortals can use these skills by default.  The
   * min level to use the skill for other classes is set up in class.c.
   */

  skillo(SKILL_BACKSTAB,    "заколоть", 140);
  skillo(SKILL_BASH,        "сбить", 140);
  skillo(SKILL_HIDE,        "спрятаться", 100);
  skillo(SKILL_KICK,        "пнуть", 100);
  skillo(SKILL_PICK_LOCK,   "взломать", 120);
  skillo(SKILL_PUNCH,       "кулачный бой", 100);
  skillo(SKILL_RESCUE,      "спасти", 100);
  skillo(SKILL_SNEAK,       "подкрасться", 100);
  skillo(SKILL_STEAL,       "украсть", 100);
  skillo(SKILL_TRACK,       "выследить", 100);
  skillo(SKILL_PARRY,       "парировать", 120);
  skillo(SKILL_BLOCK,       "блокировать щитом", 120);
  skillo(SKILL_TOUCH,       "перехватить атаку", 100);
  skillo(SKILL_PROTECT,     "прикрыть", 120);
  skillo(SKILL_BOTHHANDS,   "двуручники", 120);
  skillo(SKILL_LONGS,       "длинные лезвия", 120);
  skillo(SKILL_SPADES,      "копья и рогатины", 120);
  skillo(SKILL_SHORTS,      "короткие лезвия", 120);
  skillo(SKILL_BOWS,        "луки", 120);
  skillo(SKILL_CLUBS,       "палицы и дубины", 120);
  skillo(SKILL_PICK,        "проникающее оружие", 120);
  skillo(SKILL_NONSTANDART, "иное оружие", 120);
  skillo(SKILL_AXES,        "секиры", 120);
  skillo(SKILL_SATTACK,     "атака левой рукой", 100);
  skillo(SKILL_LOOKING,     "приглядеться", 100);
  skillo(SKILL_HEARING,     "прислушаться", 100);

  skillo(SKILL_DISARM,          "обезоружить", 100);
  skillo(SKILL_HEAL,            "!heal!", 100);
  skillo(SKILL_TURN,            "turn", 100);
  skillo(SKILL_ADDSHOT,         "дополнительный выстрел", 100);
  skillo(SKILL_CAMOUFLAGE,      "маскировка", 100);
  skillo(SKILL_DEVIATE,         "уклониться", 100);
  skillo(SKILL_CHOPOFF,     "подножка", 100);
  skillo(SKILL_REPAIR,          "ремонт", 100);
  skillo(SKILL_COURAGE,         "ярость", 100);
  skillo(SKILL_IDENTIFY,        "опознание", 100);
  skillo(SKILL_LOOK_HIDE,       "подсмотреть", 100);
  skillo(SKILL_UPGRADE,         "заточить", 100);
  skillo(SKILL_ARMORED,         "укрепить", 100);
  skillo(SKILL_DRUNKOFF,    "опохмелиться", 100);
  skillo(SKILL_AID,         "лечить", 100);
  skillo(SKILL_FIRE,        "разжечь костер", 100);
  skillo(SKILL_SHIT,        "удар левой рукой", 100);
  skillo(SKILL_MIGHTHIT,    "богатырский молот", 100);
  skillo(SKILL_STUPOR,      "оглушить", 100);
  skillo(SKILL_POISONED,    "отравить", 100);
  skillo(SKILL_LEADERSHIP,  "лидерство", 100);
  skillo(SKILL_PUNCTUAL,    "точный стиль", 100);
  skillo(SKILL_AWAKE,       "осторожный стиль", 100);
  skillo(SKILL_SENSE,       "найти", 100);
  skillo(SKILL_HORSE,       "сражение верхом", 100);
  skillo(SKILL_HIDETRACK,   "замести следы", 120);
  skillo(SKILL_RELIGION,    "!молитва или жертва!", 100);
  skillo(SKILL_MAKEFOOD,    "освежевать", 120);
  skillo(SKILL_MULTYPARRY,         "веерная защита", 140);
  skillo(SKILL_TRANSFORMWEAPON,    "перековать", 140);
  skillo(SKILL_THROW,              "метнуть", 150);
  skillo(SKILL_CREATEBOW,    "смастерить лук", 140);
}