/* ************************************************************************
*   File: spells.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define DEFAULT_STAFF_LVL	12
#define DEFAULT_WAND_LVL	12
#define CAST_UNDEFINED	-1
#define CAST_SPELL	0
#define CAST_POTION	1
#define CAST_WAND	2
#define CAST_STAFF	3
#define CAST_SCROLL	4
#define CAST_ITEMS  5
#define CAST_RUNES  6

#define MTYPE_NEUTRAL (1 << 0)
#define MTYPE_AIR     (1 << 1)
#define MTYPE_FIRE    (1 << 2)
#define MTYPE_WATER   (1 << 3)
#define MTYPE_EARTH   (1 << 4)

#define MAG_DAMAGE	    (1 << 0)
#define MAG_AFFECTS	    (1 << 1)
#define MAG_UNAFFECTS	(1 << 2)
#define MAG_POINTS	    (1 << 3)
#define MAG_ALTER_OBJS	(1 << 4)
#define MAG_GROUPS	    (1 << 5)
#define MAG_MASSES	    (1 << 6)
#define MAG_AREAS	    (1 << 7)
#define MAG_SUMMONS	    (1 << 8)
#define MAG_CREATIONS	(1 << 9)
#define MAG_MANUAL	    (1 << 10)
#define NPC_DAMAGE_PC           (1 << 16)
#define NPC_DAMAGE_PC_MINHP     (1 << 17)
#define NPC_AFFECT_PC           (1 << 18)
#define NPC_AFFECT_PC_CASTER    (1 << 19)
#define NPC_AFFECT_NPC          (1 << 20)
#define NPC_UNAFFECT_NPC        (1 << 21)
#define NPC_UNAFFECT_NPC_CASTER (1 << 22)
#define NPC_DUMMY               (1 << 23)
#define NPC_CALCULATE           (0xff << 16)
/***** Extra attack bit flags */
#define EAF_PARRY       (1 << 0)
#define EAF_BLOCK       (1 << 1)
#define EAF_TOUCH       (1 << 2)
#define EAF_PROTECT     (1 << 3)
#define EAF_DEVIATE     (1 << 4)
#define EAF_MIGHTHIT    (1 << 5)
#define EAF_STUPOR      (1 << 6)
#define EAF_SLOW        (1 << 7)
#define EAF_PUNCTUAL    (1 << 8)
#define EAF_AWAKE       (1 << 9)
#define EAF_FIRST       (1 << 10)
#define EAF_SECOND      (1 << 11)
#define EAF_STAND       (1 << 13)
#define EAF_USEDRIGHT   (1 << 14)
#define EAF_USEDLEFT    (1 << 15)
#define EAF_MULTYPARRY  (1 << 16)
#define EAF_SLEEP       (1 << 17)

#define TYPE_NOPARRY		    -2
#define TYPE_UNDEFINED              -1
#define SPELL_RESERVED_DBC          0  /* SKILL NUMBER ZERO -- RESERVED */

/* PLAYER SPELLS TYPES */
#define   SPELL_KNOW   (1 << 0)
#define   SPELL_TEMP   (1 << 1)
#define   SPELL_POTION (1 << 2)
#define   SPELL_WAND   (1 << 3)
#define   SPELL_SCROLL (1 << 4)
#define   SPELL_ITEMS  (1 << 5)
#define   SPELL_RUNES  (1 << 6)

#define   ITEM_RUNES       (1 << 0)
#define   ITEM_CHECK_USES  (1 << 1)
#define   ITEM_CHECK_LAG   (1 << 2)
#define   ITEM_CHECK_LEVEL (1 << 3)
#define   ITEM_DECAY_EMPTY (1 << 4)

#define   MI_LAG1s       (1 << 0)
#define   MI_LAG2s       (1 << 1)
#define   MI_LAG4s       (1 << 2)
#define   MI_LAG8s       (1 << 3)
#define   MI_LAG16s      (1 << 4)
#define   MI_LAG32s      (1 << 5)
#define   MI_LAG64s      (1 << 6)
#define   MI_LAG128s     (1 << 7)
#define   MI_LEVEL1      (1 << 8)
#define   MI_LEVEL2      (1 << 9)
#define   MI_LEVEL4      (1 << 10)
#define   MI_LEVEL8      (1 << 11)
#define   MI_LEVEL16     (1 << 12)

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */

#define SPELL_ARMOR                   1 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_TELEPORT                2 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLESS                   3 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLINDNESS               4 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BURNING_HANDS           5 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CALL_LIGHTNING          6 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHARM                   7 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHILL_TOUCH             8 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CLONE                   9 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_COLOR_SPRAY            10 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CONTROL_WEATHER        11 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_FOOD            12 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_WATER           13 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_BLIND             14 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_CRITIC            15 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_LIGHT             16 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURSE                  17 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_ALIGN           18 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_INVIS           19 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_MAGIC           20 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_POISON          21 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_EVIL            22 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_EARTHQUAKE             23 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENCHANT_WEAPON         24 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENERGY_DRAIN           25 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_FIREBALL               26 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HARM                   27 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HEAL                   28 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INVISIBLE              29 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LIGHTNING_BOLT         30 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LOCATE_OBJECT          31 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_MAGIC_MISSILE          32 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_POISON                 33 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_PROT_FROM_EVIL         34 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_CURSE           35 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SANCTUARY              36 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SHOCKING_GRASP         37 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SLEEP                  38 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_STRENGTH               39 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SUMMON                 40 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VENTRILOQUATE          41 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WORD_OF_RECALL         42 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_POISON          43 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SENSE_LIFE             44 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ANIMATE_DEAD	     45 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_GOOD	     46 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_ARMOR	     47 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_HEAL	     48 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_RECALL	     49 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INFRAVISION	     50 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WATERWALK		     51 /* Reserved Skill[] DO NOT CHANGE */

/* Insert new spells here, up to MAX_SPELLS */
#define SPELL_CURE_SERIOUS       52
#define SPELL_GROUP_STRENGTH     53
#define SPELL_HOLD               54
#define SPELL_POWER_HOLD         55
#define SPELL_MASS_HOLD          56
#define SPELL_FLY                57

#define SPELL_NOFLEE             59
#define SPELL_CREATE_LIGHT       60
#define SPELL_DARKNESS           61
#define SPELL_STONESKIN          62
#define SPELL_CLOUDLY            63
#define SPELL_SIELENCE           64
#define SPELL_LIGHT              65
#define SPELL_CHAIN_LIGHTNING    66
#define SPELL_FIREBLAST          67
#define SPELL_IMPLOSION          68
#define SPELL_WEAKNESS           69
#define SPELL_GROUP_INVISIBLE    70
#define SPELL_PROTECT_MAGIC      71
#define SPELL_ACID               72
#define SPELL_REPAIR             73
#define SPELL_ENLARGE            74
#define SPELL_FEAR               75
#define SPELL_SACRIFICE          76
#define SPELL_WEB                77
#define SPELL_BLINK              78
#define SPELL_REMOVE_HOLD        79
#define SPELL_CAMOUFLAGE         80
#define SPELL_POWER_BLINDNESS    81
#define SPELL_MASS_BLINDNESS     82
#define SPELL_POWER_SIELENCE     83
#define SPELL_EXTRA_HITS         84
#define SPELL_RESSURECTION       85
#define SPELL_PROLONGATION       86
#define SPELL_FORBIDDEN          87
#define SPELL_MASS_SIELENCE      88
#define SPELL_REMOVE_SIELENCE    89
#define SPELL_DAMAGE_LIGHT       90
#define SPELL_DAMAGE_SERIOUS     91
#define SPELL_DAMAGE_CRITIC      92
#define SPELL_MASS_CURSE         93
#define SPELL_ARMAGEDDON         94
#define SPELL_GROUP_FLY          95
#define SPELL_GROUP_BLESS        96
#define SPELL_REFRESH            97
#define SPELL_STUNNING           98
#define SPELL_HIDE               99
#define SPELL_SNEAK              100
#define SPELL_DRUNKED            101
#define SPELL_ABSTINENT          102
#define SPELL_FULL               103
#define SPELL_CONE_OF_COLD       104
#define SPELL_BATTLE             105
#define SPELL_HAEMORRAGIA        106
#define SPELL_COURAGE            107
#define SPELL_WATERBREATH        108
#define SPELL_SLOW               109
#define SPELL_HASTE              110
#define SPELL_MASS_SLOW          111
#define SPELL_GROUP_HASTE        112
#define SPELL_SHIELD		 113
#define SPELL_PLAQUE		 114
#define SPELL_CURE_PLAQUE	 115
#define SPELL_AWARNESS		 116
#define SPELL_RELIGION           117
#define SPELL_AIR_SHIELD         118
#define SPELL_PORTAL             119
#define SPELL_DISPELL_MAGIC      120
#define SPELL_SUMMON_KEEPER      121
#define SPELL_FAST_REGENERATION  122
#define SPELL_CREATE_WEAPON      123
#define SPELL_FIRE_SHIELD        124
#define SPELL_RELOCATE           125
#define SPELL_SUMMON_FIREKEEPER  126
#define SPELL_ICE_SHIELD         127

#define SPELL_ICESTORM			 128
#define SPELL_ENLESS			 129
#define	SPELL_SHINEFLASH		 130
#define SPELL_MADNESS			 131
#define SPELL_MAGICGLASS		 132
#define SPELL_STAIRS			 133
#define SPELL_VACUUM			 134
#define SPELL_METEORSTORM		 135
#define SPELL_STONEHAND			 136
#define SPELL_MINDLESS			 137
#define SPELL_PRISMATICAURA              138
#define SPELL_EVILESS                    139
#define SPELL_AIR_AURA                   140
#define SPELL_FIRE_AURA                  141
#define SPELL_ICE_AURA                   142
#define LAST_USED_SPELL			 142
/* PLAYER SKILLS - Numbered from 1 to MAX_SKILLS */
#define SKILL_THAC0                 0  /* Internal */
#define SKILL_PROTECT               1 /**** Protect grouppers    */
#define SKILL_TOUCH                 2 /**** Touch attacker       */
#define SKILL_SHIT                  3
#define SKILL_MIGHTHIT              4
#define SKILL_STUPOR                5
#define SKILL_POISONED              6
#define SKILL_SENSE                 7
#define SKILL_HORSE                 8
#define SKILL_HIDETRACK             9
#define SKILL_RELIGION              10
#define SKILL_MAKEFOOD              11
#define SKILL_MULTYPARRY            12
#define SKILL_TRANSFORMWEAPON       13
#define SKILL_LEADERSHIP            20
#define SKILL_PUNCTUAL              21
#define SKILL_AWAKE                 22
#define SKILL_IDENTIFY              23
#define SKILL_HEARING               24
#define SKILL_CREATE_POTION         25
#define SKILL_CREATE_SCROLL         26
#define SKILL_CREATE_WAND           27
#define SKILL_LOOK_HIDE             28
#define SKILL_ARMORED               29
#define SKILL_DRUNKOFF              30
#define SKILL_AID                   31
#define SKILL_FIRE                  32
#define SKILL_CREATEBOW             33

#define SKILL_THROW                 130
#define SKILL_BACKSTAB              131 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_BASH                  132 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_HIDE                  133 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_KICK                  134 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PICK_LOCK             135 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PUNCH                 136 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_RESCUE                137 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_SNEAK                 138 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_STEAL                 139 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_TRACK                 140 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_CLUBS                 141 /**** Weapon is club, etc    */
#define SKILL_AXES                  142 /**** Weapon is axe, etc     */
#define SKILL_LONGS                 143 /**** Weapon is long blades  */
#define SKILL_SHORTS                144 /**** Weapon is short blades */
#define SKILL_NONSTANDART           145 /**** Weapon is non-standart */
#define SKILL_BOTHHANDS             146 /**** Weapon in both hands   */
#define SKILL_PICK                  147 /**** Weapon is pick         */
#define SKILL_SPADES                148 /**** Weapon is spades       */
#define SKILL_SATTACK               149
#define SKILL_DISARM                150
#define SKILL_PARRY                 151
#define SKILL_HEAL                  152
#define SKILL_TURN                  153
#define SKILL_BOWS                  154
#define SKILL_ADDSHOT               155
#define SKILL_CAMOUFLAGE            156
#define SKILL_DEVIATE               157
#define SKILL_BLOCK                 158
#define SKILL_LOOKING               159
#define SKILL_CHOPOFF               160
#define SKILL_REPAIR                161
#define SKILL_UPGRADE               164
#define SKILL_COURAGE               165


/* New skills may be added here up to MAX_SKILLS (200) */


/*
 *  NON-PLAYER AND OBJECT SPELLS AND SKILLS
 *  The practice levels for the spells and skills below are _not_ recorded
 *  in the playerfile; therefore, the intended use is for spells and skills
 *  associated with objects (such as SPELL_IDENTIFY used with scrolls of
 *  identify) or non-players (such as NPC-only spells).
 */

#define SPELL_IDENTIFY               351
#define SPELL_FIRE_BREATH            352
#define SPELL_GAS_BREATH             353
#define SPELL_FROST_BREATH           354
#define SPELL_ACID_BREATH            355
#define SPELL_LIGHTNING_BREATH       356

#define TOP_SPELL_DEFINE	         399
#define TOP_SKILL_DEFINE             200
/* NEW NPC/OBJECT SPELLS can be inserted here up to 299 */


/* WEAPON ATTACK TYPES */

#define TYPE_HIT                     400
#define TYPE_STING                   401
#define TYPE_WHIP                    402
#define TYPE_SLASH                   403
#define TYPE_BITE                    404
#define TYPE_BLUDGEON                405
#define TYPE_CRUSH                   406
#define TYPE_POUND                   407
#define TYPE_CLAW                    408
#define TYPE_MAUL                    409
#define TYPE_THRASH                  410
#define TYPE_PIERCE                  411
#define TYPE_BLAST		     412
#define TYPE_PUNCH		     413
#define TYPE_STAB		     414
#define TYPE_PICK                    415
#define TYPE_MAGIC                   420
/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_ROOMDEATH				 498
#define TYPE_SUFFERING		         499

#define SAVING_PARA   0
#define SAVING_ROD    1
#define SAVING_PETRI  2
#define SAVING_BREATH 3
#define SAVING_SPELL  4
#define SAVING_BASIC  5
#define SAVING_COUNT  6

#define TAR_IGNORE      (1 << 0)
#define TAR_CHAR_ROOM   (1 << 1)
#define TAR_CHAR_WORLD  (1 << 2)
#define TAR_FIGHT_SELF  (1 << 3)
#define TAR_FIGHT_VICT  (1 << 4)
#define TAR_SELF_ONLY   (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF   	(1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV     (1 << 7)
#define TAR_OBJ_ROOM    (1 << 8)
#define TAR_OBJ_WORLD   (1 << 9)
#define TAR_OBJ_EQUIP	(1 << 10)
#define MAX_SLOT        10

struct spell_info_type
{  byte min_position;	/* Position for caster	 */
   int mana_min;	    /* Min amount of mana used by a spell (highest lev) */
   int mana_max;	    /* Max amount of mana used by a spell (lowest lev) */
   int mana_change;	    /* Change in mana used by spell from lev to lev */

   int    min_level[NUM_CLASSES];
   int    slot_forc[NUM_CLASSES];
   long   danger;
   long   routines;
   byte   violent;
   int    targets;      /* See below for use with TAR_XXX  */
   const  char *name;
   const  char *syn;
};

struct skill_info_type {
   byte  min_position;	 /* Position for caster */
   long int   k_improove[NUM_CLASSES];
   int   max_percent;
   const char *name;
};

struct spell_create_item {
       int items[3];
       int rnumber;
};

struct spell_create_type {
       struct spell_create_item  wand;
       struct spell_create_item  scroll;
       struct spell_create_item  potion;
       struct spell_create_item  items;
       struct spell_create_item  runes;
};



/* Possible Targets:

   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self.

*/

#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4


/* Attacktypes with grammar */

struct attack_hit_type {
   const char	*singular;
   const char	*plural;
};

extern struct spell_info_type spell_info[];
extern struct skill_info_type skill_info[];

#define ASPELL(spellname) \
void	spellname(int level, struct char_data *ch, \
		  struct char_data *victim, struct obj_data *obj)

#define MANUAL_SPELL(spellname)	spellname(level, caster, cvict, ovict);

ASPELL(spell_create_water);
ASPELL(spell_recall);
ASPELL(spell_teleport);
ASPELL(spell_summon);
ASPELL(spell_relocate);
ASPELL(spell_portal);
ASPELL(spell_locate_object);
ASPELL(spell_charm);
ASPELL(spell_information);
ASPELL(spell_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_detect_poison);
ASPELL(skill_identify);
ASPELL(spell_chain_lightning);
ASPELL(spell_control_weather);
ASPELL(spell_create_weapon);
ASPELL(spell_eviless);

/* basic magic calling functions */

int find_skill_num(char *name);

int find_spell_num(char *name);

int mag_damage(int level, struct char_data *ch, struct char_data *victim,
               int spellnum, int savetype);

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
                 int spellnum, int savetype);

void mag_groups(int level, struct char_data *ch, int spellnum, int savetype);

void mag_masses(int level, struct char_data *ch, int spellnum, int savetype);

void mag_areas(int level, struct char_data *ch, int spellnum, int savetype);

void mag_char_areas(int level, struct char_data *ch, struct char_data *victim, int spellnum, int savetype);

void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
                 int spellnum, int savetype);

void mag_points(int level, struct char_data *ch, struct char_data *victim,
                int spellnum, int savetype);

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
                   int spellnum, int type);

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
                    int spellnum, int type);

void mag_creations(int level, struct char_data *ch, int spellnum);

int	call_magic(struct char_data *caster, struct char_data *cvict,
               struct obj_data *ovict, int spellnum, int level, int casttype);

void mag_objectmagic(struct char_data *ch, struct obj_data *obj,
	                 char *argument);

int	cast_spell(struct char_data *ch, struct char_data *tch,
               struct obj_data *tobj, int spellnum);

/* other prototypes */
void  mspell_level(int spell, int chclass, int level);
void  mspell_slot (int spell, int chclass, int slot);
int   general_savingthrow(struct char_data * ch, int type, int ext_apply);
void  init_spell_levels(void);
const char *skill_name(int num);
const char *spell_name(int num);