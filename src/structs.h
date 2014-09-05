/* ************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  For instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */
#define _CIRCLEMUD	0x030010 /* Major/Minor/Patchlevel - MMmmPP */

/*
 * If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1.  Please note
 * that this will require erasing or converting all of your rent files.
 * And of course, you have to recompile everything.  We need this feature
 * for CircleMUD 3.0 to be complete but we refuse to break binary file
 * compatibility.
 */
#define USE_AUTOEQ	1	/* TRUE/FALSE aren't defined yet. */

#define PK_OK            0
#define PK_ONE_OVERFLOW -1
#define PK_ALL_OVERFLOW -2

#define MOB_KILL_VICTIM  0
#define PC_KILL_MOB      1
#define PC_KILL_PC       2
#define PC_REVENGE_PC    3

#define MAX_PKILLER_MEM  100
#define MAX_PK_ONE_ALLOW 10
#define MAX_PK_ALL_ALLOW 100
#define MAX_PKILL_LAG    2*60
#define MAX_REVENGE_LAG  2*60
#define MAX_PTHIEF_LAG   10*60
#define MAX_REVENGE      2
#define EXPERT_WEAPON    80
#define MAX_DEST         10

/* preamble *************************************************************/

#define NOWHERE    -1    /* nil reference for room-database	*/
#define NOTHING	   -1    /* nil reference for objects		*/
#define NOBODY	   -1    /* nil reference for mobiles		*/

#define SPECIAL(name) \
   int (name)(struct char_data *ch, void *me, int cmd, char *argument)

/* misc editor defines **************************************************/

/* format modes for format_text */
#define FORMAT_INDENT		(1 << 0)

#define KT_ALT        1
#define KT_WIN        2
#define KT_WINZ       3
#define KT_WINZ6      4
#define KT_LAST       5
#define KT_SELECTMENU 255

/* room-related defines *************************************************/


/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5


/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK		(1 << 0)   /* Dark			*/
#define ROOM_DEATH		(1 << 1)   /* Death trap		*/
#define ROOM_NOMOB		(1 << 2)   /* MOBs not allowed		*/
#define ROOM_INDOORS		(1 << 3)   /* Indoors			*/
#define ROOM_PEACEFUL		(1 << 4)   /* Violence not allowed	*/
#define ROOM_SOUNDPROOF		(1 << 5)   /* Shouts, gossip blocked	*/
#define ROOM_NOTRACK		(1 << 6)   /* Track won't go through	*/
#define ROOM_NOMAGIC		(1 << 7)   /* Magic not allowed		*/
#define ROOM_TUNNEL		    (1 << 8)   /* room for only 1 pers	*/
#define ROOM_PRIVATE		(1 << 9)   /* Can't teleport in		*/
#define ROOM_GODROOM		(1 << 10)  /* LVL_GOD+ only allowed	*/
#define ROOM_HOUSE		    (1 << 11)  /* (R) Room is a house	*/
#define ROOM_HOUSE_CRASH	(1 << 12)  /* (R) House needs saving	*/
#define ROOM_ATRIUM		    (1 << 13)  /* (R) The door to a house	*/
#define ROOM_OLC		    (1 << 14)  /* (R) Modifyable/!compress	*/
#define ROOM_BFS_MARK		(1 << 15)  /* (R) breath-first srch mrk	*/
#define ROOM_MAGE           (1 << 16)
#define ROOM_CLERIC         (1 << 17)
#define ROOM_THIEF          (1 << 18)
#define ROOM_WARRIOR        (1 << 19)
#define ROOM_ASSASINE       (1 << 20)
#define ROOM_GUARD          (1 << 21)
#define ROOM_PALADINE       (1 << 22)
#define ROOM_RANGER         (1 << 23)
#define ROOM_POLY           (1 << 24)
#define ROOM_MONO           (1 << 25)
#define ROOM_SMITH          (1 << 26)
#define ROOM_MERCHANT       (1 << 27)
#define ROOM_DRUID          (1 << 28)
#define ROOM_ARENA          (1 << 29)

#define ROOM_NOSUMMON       (INT_ONE | (1 << 0))
#define ROOM_NOTELEPORT     (INT_ONE | (1 << 1))
#define ROOM_NOHORSE        (INT_ONE | (1 << 2))
#define ROOM_NOWEATHER      (INT_ONE | (1 << 3))
#define ROOM_SLOWDEATH      (INT_ONE | (1 << 4))
#define ROOM_ICEDEATH       (INT_ONE | (1 << 5))

/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR		(1 << 0)   /* Exit is a door		*/
#define EX_CLOSED		(1 << 1)   /* The door is closed	*/
#define EX_LOCKED		(1 << 2)   /* The door is locked	*/
#define EX_PICKPROOF    (1 << 3)   /* Lock can't be picked	*/
#define EX_HIDDEN       (1 << 4)

#define AF_BATTLEDEC    (1 << 0)
#define AF_DEADKEEP     (1 << 1)

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0		   /* Indoors			*/
#define SECT_CITY            1		   /* In a city			*/
#define SECT_FIELD           2		   /* In a field		*/
#define SECT_FOREST          3		   /* In a forest		*/
#define SECT_HILLS           4		   /* In the hills		*/
#define SECT_MOUNTAIN        5		   /* On a mountain		*/
#define SECT_WATER_SWIM      6		   /* Swimmable water		*/
#define SECT_WATER_NOSWIM    7		   /* Water - need a boat	*/
#define SECT_FLYING	         8		   /* Wheee!			*/
#define SECT_UNDERWATER	     9		   /* Underwater		*/
#define SECT_SECRET          10

/* Added values for weather changes */
#define SECT_FIELD_SNOW      20
#define SECT_FIELD_RAIN      21
#define SECT_FOREST_SNOW     22
#define SECT_FOREST_RAIN     23
#define SECT_HILLS_SNOW      24
#define SECT_HILLS_RAIN      25
#define SECT_MOUNTAIN_SNOW   26
#define SECT_THIN_ICE        27
#define SECT_NORMAL_ICE      28
#define SECT_THICK_ICE       29


#define WEATHER_QUICKCOOL		(1 << 0)
#define WEATHER_QUICKHOT		(1 << 1)
#define WEATHER_LIGHTRAIN		(1 << 2)
#define WEATHER_MEDIUMRAIN		(1 << 3)
#define WEATHER_BIGRAIN			(1 << 4)
#define WEATHER_GRAD			(1 << 5)
#define WEATHER_LIGHTSNOW		(1 << 6)
#define WEATHER_MEDIUMSNOW		(1 << 7)
#define WEATHER_BIGSNOW			(1 << 8)
#define WEATHER_LIGHTWIND		(1 << 9)
#define WEATHER_MEDIUMWIND		(1 << 10)
#define WEATHER_BIGWIND			(1 << 11)


/* char and mob-related defines *****************************************/

#define NUM_CLASSES	  14  /* This must be the number of classes!! */
/* PC classes */
#define CLASS_UNDEFINED	    -1
#define CLASS_CLERIC         0
#define CLASS_MAGIC_USER     1
#define CLASS_BATTLEMAGE     1
#define CLASS_THIEF          2
#define CLASS_WARRIOR        3
#define CLASS_ASSASINE       4
#define CLASS_GUARD          5
#define CLASS_DEFENDERMAGE   6
#define CLASS_CHARMMAGE      7
#define CLASS_NECROMANCER    8
#define CLASS_PALADINE       9
#define CLASS_RANGER         10
#define CLASS_SMITH			 11
#define CLASS_MERCHANT		 12
#define CLASS_DRUID			 13

#define MASK_BATTLEMAGE   (1 << CLASS_MAGIC_USER)
#define MASK_CLERIC       (1 << CLASS_CLERIC)
#define MASK_THIEF        (1 << CLASS_THIEF)
#define MASK_WARRIOR      (1 << CLASS_WARRIOR)
#define MASK_ASSASINE     (1 << CLASS_ASSASINE)
#define MASK_GUARD        (1 << CLASS_GUARD)
#define MASK_DEFENDERMAGE (1 << CLASS_DEFENDERMAGE)
#define MASK_CHARMMAGE    (1 << CLASS_CHARMMAGE)
#define MASK_NECROMANCER  (1 << CLASS_NECROMANCER)
#define MASK_PALADINE     (1 << CLASS_PALADINE)
#define MASK_RANGER       (1 << CLASS_RANGER)
#define MASK_SMITH        (1 << CLASS_SMITH)
#define MASK_MERCHANT     (1 << CLASS_MERCHANT)
#define MASK_DRUID        (1 << CLASS_DRUID)

#define MASK_MAGES        (MASK_BATTLEMAGE | MASK_DEFENDERMAGE | MASK_CHARMMAGE | MASK_NECROMANCER)
#define MASK_CASTER       (MASK_BATTLEMAGE | MASK_DEFENDERMAGE | MASK_CHARMMAGE | MASK_NECROMANCER | MASK_CLERIC | MASK_DRUID)

/* PC religions */
#define RELIGION_POLY    0
#define RELIGION_MONO    1

#define MASK_RELIGION_POLY        (1 << RELIGION_POLY)
#define MASK_RELIGION_MONO        (1 << RELIGION_MONO)
/* PC reces */
#define RACE_UNDEFINED      -1
#define NUM_RACES            6
#define CLASS_SEVERANE       0
#define CLASS_POLANE         1
#define CLASS_KRIVICHI       2
#define CLASS_VATICHI        3
#define CLASS_VELANE         4
#define CLASS_DREVLANE       5

#define MASK_SEVERANE        (1 << CLASS_SEVERANE)
#define MASK_POLANE          (1 << CLASS_POLANE)
#define MASK_KRIVICHI        (1 << CLASS_CRIVICHI)
#define MASK_VATICHI         (1 << CLASS_VATICHI)
#define MASK_VELANE          (1 << CLASS_VELANE)
#define MASK_DREVLANE        (1 << CLASS_DREVLANE)




/* NPC classes (currently unused - feel free to implement!) */
#define CLASS_BASIC_NPC    100
#define CLASS_UNDEAD       101
#define CLASS_HUMAN        102
#define CLASS_ANIMAL       103
#define CLASS_HERO_WARRIOR 104
#define CLASS_HERO_MAGIC   105
#define CLASS_LAST_NPC     106

/* Sex */
#define SEX_NEUTRAL         0
#define SEX_MALE            1
#define SEX_FEMALE          2
#define SEX_POLY            3

#define NUM_SEXES           4

#define MASK_SEX_NEUTRAL  (1 << SEX_NEUTRAL)
#define MASK_SEX_MALE     (1 << SEX_MALE)
#define MASK_SEX_FEMALE   (1 << SEX_FEMALE)
#define MASK_SEX_POLY     (1 << SEX_POLY)

/* GODs FLAGS */
#define GF_GODSLIKE   (1 << 0)
#define GF_GODSCURSE  (1 << 1)
#define GF_HIGHGOD    (1 << 2)
#define GF_REMORT     (1 << 3)


/* Positions */
#define POS_DEAD       0	/* dead			*/
#define POS_MORTALLYW  1	/* mortally wounded	*/
#define POS_INCAP      2	/* incapacitated	*/
#define POS_STUNNED    3	/* stunned		*/
#define POS_SLEEPING   4	/* sleeping		*/
#define POS_RESTING    5	/* resting		*/
#define POS_SITTING    6	/* sitting		*/
#define POS_FIGHTING   7	/* fighting		*/
#define POS_STANDING   8	/* standing		*/


/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER	    (1 << 0)   /* Player is a player-killer		*/
#define PLR_THIEF	    (1 << 1)   /* Player is a player-thief		*/
#define PLR_FROZEN	    (1 << 2)   /* Player is frozen			*/
#define PLR_DONTSET     (1 << 3)   /* Don't EVER set (ISNPC bit)	*/
#define PLR_WRITING	    (1 << 4)   /* Player writing (board/mail/olc)	*/
#define PLR_MAILING	    (1 << 5)   /* Player is writing mail		*/
#define PLR_CRASH	    (1 << 6)   /* Player needs to be crash-saved	*/
#define PLR_SITEOK	    (1 << 7)   /* Player has been site-cleared	*/
#define PLR_NOSHOUT	    (1 << 8)   /* Player not allowed to shout/goss	*/
#define PLR_NOTITLE	    (1 << 9)   /* Player not allowed to set title	*/
#define PLR_DELETED	    (1 << 10)  /* Player deleted - space reusable	*/
#define PLR_LOADROOM	(1 << 11)  /* Player uses nonstandard loadroom	*/
#define PLR_NOWIZLIST	(1 << 12)  /* Player shouldn't be on wizlist	*/
#define PLR_NODELETE	(1 << 13)  /* Player shouldn't be deleted	*/
#define PLR_INVSTART	(1 << 14)  /* Player should enter game wizinvis	*/
#define PLR_CRYO	    (1 << 15)  /* Player is cryo-saved (purge prog)	*/
#define PLR_HELLED	    (1 << 16)  /* Player is in Hell	*/
#define PLR_NAMED	    (1 << 17)  /* Player is in Names Room */
#define PLR_REGISTERED      (1 << 18)
#define PLR_DELETE          (1 << 28) /* RESERVED - ONLY INTERNALLY */
#define PLR_FREE            (1 << 29) /* RESERVED - ONLY INTERBALLY */


/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC         (1 << 0)  /* Mob has a callable spec-proc	*/
#define MOB_SENTINEL     (1 << 1)  /* Mob should not move		*/
#define MOB_SCAVENGER    (1 << 2)  /* Mob picks up stuff on the ground	*/
#define MOB_ISNPC        (1 << 3)  /* (R) Automatically set on all Mobs	*/
#define MOB_AWARE	     (1 << 4)  /* Mob can't be backstabbed		*/
#define MOB_AGGRESSIVE   (1 << 5)  /* Mob hits players in the room	*/
#define MOB_STAY_ZONE    (1 << 6)  /* Mob shouldn't wander out of zone	*/
#define MOB_WIMPY        (1 << 7)  /* Mob flees if severely injured	*/
#define MOB_AGGR_DAY	 (1 << 8)  /* */
#define MOB_AGGR_NIGHT	 (1 << 9)  /* */
#define MOB_AGGR_FULLMOON (1 << 10) /* */
#define MOB_MEMORY	     (1 << 11) /* remember attackers if attacked	*/
#define MOB_HELPER	     (1 << 12) /* attack PCs fighting other NPCs	*/
#define MOB_NOCHARM	     (1 << 13) /* Mob can't be charmed		*/
#define MOB_NOSUMMON	 (1 << 14) /* Mob can't be summoned		*/
#define MOB_NOSLEEP	     (1 << 15) /* Mob can't be slept		*/
#define MOB_NOBASH	     (1 << 16) /* Mob can't be bashed (e.g. trees)	*/
#define MOB_NOBLIND	     (1 << 17) /* Mob can't be blinded		*/
#define MOB_MOUNTING     (1 << 18)
#define MOB_NOHOLD       (1 << 19)
#define MOB_NOSIELENCE   (1 << 20)
#define MOB_AGGRMONO     (1 << 21)
#define MOB_AGGRPOLY     (1 << 22)
#define MOB_NOFEAR       (1 << 23)
#define MOB_NOGROUP      (1 << 24)
#define MOB_CORPSE       (1 << 25)
#define MOB_LOOTER       (1 << 26)
#define MOB_PROTECT      (1 << 27)
#define MOB_DELETE       (1 << 28) /* RESERVED - ONLY INTERNALLY */
#define MOB_FREE         (1 << 29) /* RESERVED - ONLY INTERBALLY */

#define MOB_SWIMMING      (INT_ONE | (1 << 0))
#define MOB_FLYING        (INT_ONE | (1 << 1))
#define MOB_ONLYSWIMMING  (INT_ONE | (1 << 2))
#define MOB_AGGR_WINTER   (INT_ONE | (1 << 3))
#define MOB_AGGR_SPRING   (INT_ONE | (1 << 4))
#define MOB_AGGR_SUMMER   (INT_ONE | (1 << 5))
#define MOB_AGGR_AUTUMN   (INT_ONE | (1 << 6))
#define MOB_LIKE_DAY      (INT_ONE | (1 << 7))
#define MOB_LIKE_NIGHT    (INT_ONE | (1 << 8))
#define MOB_LIKE_FULLMOON (INT_ONE | (1 << 9))
#define MOB_LIKE_WINTER   (INT_ONE | (1 << 10))
#define MOB_LIKE_SPRING   (INT_ONE | (1 << 11))
#define MOB_LIKE_SUMMER   (INT_ONE | (1 << 12))
#define MOB_LIKE_AUTUMN   (INT_ONE | (1 << 13))
#define MOB_NOFIGHT       (INT_ONE | (1 << 14))
#define MOB_EADECREASE    (INT_ONE | (1 << 15))
#define MOB_HORDE         (INT_ONE | (1 << 16))

#define MOB_FIREBREATH    (INT_TWO | (1 << 0))
#define MOB_GASBREATH     (INT_TWO | (1 << 1))
#define MOB_FROSTBREATH   (INT_TWO | (1 << 2))
#define MOB_ACIDBREATH    (INT_TWO | (1 << 3))
#define MOB_LIGHTBREATH   (INT_TWO | (1 << 4))

#define NPC_NORTH         (1 << 0)
#define NPC_EAST          (1 << 1)
#define NPC_SOUTH         (1 << 2)
#define NPC_WEST          (1 << 3)
#define NPC_UP            (1 << 4)
#define NPC_DOWN          (1 << 5)
#define NPC_POISON        (1 << 6)
#define NPC_INVIS         (1 << 7)
#define NPC_SNEAK         (1 << 8)
#define NPC_CAMOUFLAGE    (1 << 9)
#define NPC_MOVEFLY       (1 << 11)
#define NPC_MOVECREEP     (1 << 12)
#define NPC_MOVEJUMP      (1 << 13)
#define NPC_MOVESWIM      (1 << 14)
#define NPC_MOVERUN       (1 << 15)
#define NPC_AIRCREATURE   (1 << 20)
#define NPC_WATERCREATURE (1 << 21)
#define NPC_EARTHCREATURE (1 << 22)
#define NPC_FIRECREATURE  (1 << 23)
#define NPC_HELPED        (1 << 24)

#define NPC_STEALING      (INT_ONE | (1 << 0))
#define NPC_WIELDING      (INT_ONE | (1 << 1))
#define NPC_ARMORING      (INT_ONE | (1 << 2))
#define NPC_USELIGHT      (INT_ONE | (1 << 3))


/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF       (1 << 0)  /* Room descs won't normally be shown	*/
#define PRF_COMPACT     (1 << 1)  /* No extra CRLF pair before prompts	*/
#define PRF_DEAF	    (1 << 2)  /* Can't hear shouts			*/
#define PRF_NOTELL	    (1 << 3)  /* Can't receive tells		*/
#define PRF_DISPHP	    (1 << 4)  /* Display hit points in prompt	*/
#define PRF_DISPMANA	(1 << 5)  /* Display mana points in prompt	*/
#define PRF_DISPMOVE	(1 << 6)  /* Display move points in prompt	*/
#define PRF_AUTOEXIT	(1 << 7)  /* Display exits in a room		*/
#define PRF_NOHASSLE	(1 << 8)  /* Aggr mobs won't attack		*/
#define PRF_QUEST	    (1 << 9)  /* On quest				*/
#define PRF_SUMMONABLE	(1 << 10) /* Can be summoned			*/
#define PRF_NOREPEAT	(1 << 11) /* No repetition of comm commands	*/
#define PRF_HOLYLIGHT	(1 << 12) /* Can see in dark			*/
#define PRF_COLOR_1	    (1 << 13) /* Color (low bit)			*/
#define PRF_COLOR_2	    (1 << 14) /* Color (high bit)			*/
#define PRF_NOWIZ	    (1 << 15) /* Can't hear wizline			*/
#define PRF_LOG1	    (1 << 16) /* On-line System Log (low bit)	*/
#define PRF_LOG2	    (1 << 17) /* On-line System Log (high bit)	*/
#define PRF_NOAUCT	    (1 << 18) /* Can't hear auction channel		*/
#define PRF_NOGOSS	    (1 << 19) /* Can't hear gossip channel		*/
#define PRF_NOGRATZ	    (1 << 20) /* Can't hear grats channel		*/
#define PRF_ROOMFLAGS	(1 << 21) /* Can see room flags (ROOM_x)	*/
#define PRF_DISPEXP     (1 << 22)
#define PRF_DISPEXITS   (1 << 23)
#define PRF_DISPLEVEL   (1 << 24)
#define PRF_DISPGOLD    (1 << 25)
#define PRF_DISPTICK    (1 << 26)
#define PRF_PUNCTUAL    (1 << 27)
#define PRF_AWAKE       (1 << 28)
#define PRF_CODERINFO   (1 << 29)

#define PRF_AUTOMEM     (INT_ONE | 1 << 0)

/* Affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_BLIND             (1 << 0)	   /* (R) Char is blind		*/
#define AFF_INVISIBLE         (1 << 1)	   /* Char is invisible		*/
#define AFF_DETECT_ALIGN      (1 << 2)	   /* Char is sensitive to align*/
#define AFF_DETECT_INVIS      (1 << 3)	   /* Char can see invis chars  */
#define AFF_DETECT_MAGIC      (1 << 4)	   /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE        (1 << 5)	   /* Char can sense hidden life*/
#define AFF_WATERWALK	      (1 << 6)	   /* Char can walk on water	*/
#define AFF_SANCTUARY         (1 << 7)	   /* Char protected by sanct.	*/
#define AFF_GROUP             (1 << 8)	   /* (R) Char is grouped	*/
#define AFF_CURSE             (1 << 9)	   /* Char is cursed		*/
#define AFF_INFRAVISION       (1 << 10)	   /* Char can see in dark	*/
#define AFF_POISON            (1 << 11)	   /* (R) Char is poisoned	*/
#define AFF_PROTECT_EVIL      (1 << 12)	   /* Char protected from evil  */
#define AFF_PROTECT_GOOD      (1 << 13)	   /* Char protected from good  */
#define AFF_SLEEP             (1 << 14)	   /* (R) Char magically asleep	*/
#define AFF_NOTRACK	      (1 << 15)	   /* Char can't be tracked	*/
#define AFF_TETHERED	      (1 << 16)	   /* Room for future expansion	*/
#define AFF_BLESS	      (1 << 17)	   /* Room for future expansion	*/
#define AFF_SNEAK             (1 << 18)	   /* Char can move quietly	*/
#define AFF_HIDE              (1 << 19)	   /* Char is hidden		*/
#define AFF_COURAGE  	      (1 << 20)	   /* Room for future expansion	*/
#define AFF_CHARM             (1 << 21)	   /* Char is charmed		*/
#define AFF_HOLD              (1 << 22)
#define AFF_FLY               (1 << 23)
#define AFF_SIELENCE          (1 << 24)
#define AFF_AWARNESS          (1 << 25)
#define AFF_BLINK             (1 << 26)
#define AFF_HORSE             (1 << 27)    /* NPC - is horse, PC - is horsed */
#define AFF_NOFLEE            (1 << 28)
#define AFF_SINGLELIGHT       (1 << 29)

#define AFF_HOLYLIGHT         	(INT_ONE | (1 << 0))
#define AFF_HOLYDARK          	(INT_ONE | (1 << 1))
#define AFF_DETECT_POISON     	(INT_ONE | (1 << 2))
#define AFF_DRUNKED           	(INT_ONE | (1 << 3))
#define AFF_ABSTINENT         	(INT_ONE | (1 << 4))
#define AFF_STOPRIGHT         	(INT_ONE | (1 << 5))
#define AFF_STOPLEFT          	(INT_ONE | (1 << 6))
#define AFF_STOPFIGHT         	(INT_ONE | (1 << 7))
#define AFF_HAEMORRAGIA       	(INT_ONE | (1 << 8))
#define AFF_CAMOUFLAGE        	(INT_ONE | (1 << 9))
#define AFF_WATERBREATH       	(INT_ONE | (1 << 10))
#define AFF_SLOW              	(INT_ONE | (1 << 11))
#define AFF_HASTE             	(INT_ONE | (1 << 12))
#define AFF_SHIELD		(INT_ONE | (1 << 13))
#define AFF_AIRSHIELD       	(INT_ONE | (1 << 14))
#define AFF_FIRESHIELD        	(INT_ONE | (1 << 15))
#define AFF_ICESHIELD		(INT_ONE | (1 << 16))
#define AFF_MAGICGLASS		(INT_ONE | (1 << 17))
#define AFF_STAIRS		(INT_ONE | (1 << 18))
#define AFF_STONEHAND		(INT_ONE | (1 << 19))
#define AFF_PRISMATICAURA     	(INT_ONE | (1 << 20))
#define AFF_HELPER		(INT_ONE | (1 << 21))
#define AFF_EVILESS             (INT_ONE | (1 << 22))
#define AFF_AIRAURA     	(INT_ONE | (1 << 23))
#define AFF_FIREAURA		(INT_ONE | (1 << 24))
#define AFF_ICEAURA             (INT_ONE | (1 << 25))



/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING	     0		/* Playing - Nominal state	*/
#define CON_CLOSE	     1		/* Disconnecting		*/
#define CON_GET_NAME	 2		/* By what name ..?		*/
#define CON_NAME_CNFRM	 3		/* Did I get that right, x?	*/
#define CON_PASSWORD	 4		/* Password:			*/
#define CON_NEWPASSWD	 5		/* Give me a password for x	*/
#define CON_CNFPASSWD	 6		/* Please retype password:	*/
#define CON_QSEX	     7		/* Sex?				*/
#define CON_QCLASS	     8		/* Class?			*/
#define CON_RMOTD	     9		/* PRESS RETURN after MOTD	*/
#define CON_MENU	     10		/* Your choice: (main menu)	*/
#define CON_EXDESC	     11		/* Enter a new description:	*/
#define CON_CHPWD_GETOLD 12		/* Changing passwd: get old	*/
#define CON_CHPWD_GETNEW 13		/* Changing passwd: get new	*/
#define CON_CHPWD_VRFY   14		/* Verify new password		*/
#define CON_DELCNF1	     15		/* Delete confirmation 1	*/
#define CON_DELCNF2	     16		/* Delete confirmation 2	*/
#define CON_DISCONNECT	 17		/* In-game disconnection	*/
#define CON_OEDIT        18		/*. OLC mode - object edit     .*/
#define CON_REDIT        19		/*. OLC mode - room edit       .*/
#define CON_ZEDIT        20		/*. OLC mode - zone info edit  .*/
#define CON_MEDIT        21		/*. OLC mode - mobile edit     .*/
#define CON_SEDIT        22		/*. OLC mode - shop edit       .*/
#define CON_TRIGEDIT     23		/*. OLC mode - trigger edit    .*/

#define CON_NAME2        24
#define CON_NAME3        25
#define CON_NAME4        26
#define CON_NAME5        27
#define CON_NAME6        28
#define CON_RELIGION     29
#define CON_RACE         30
#define CON_LOWS         31
#define CON_GET_KEYTABLE 32
#define CON_GET_EMAIL    33


/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_LIGHT      0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_SHIELD    11
#define WEAR_ABOUT     12
#define WEAR_WAIST     13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WEAR_WIELD     16
#define WEAR_HOLD      17
#define WEAR_BOTHS     18
#define NUM_WEARS      19	/* This must be the # of eq positions!! */


/* object-related defines ********************************************/


/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT      1		/* Item is a light source	*/
#define ITEM_SCROLL     2		/* Item is a scroll		*/
#define ITEM_WAND       3		/* Item is a wand		*/
#define ITEM_STAFF      4		/* Item is a staff		*/
#define ITEM_WEAPON     5		/* Item is a weapon		*/
#define ITEM_FIREWEAPON 6		/* Unimplemented		*/
#define ITEM_MISSILE    7		/* Unimplemented		*/
#define ITEM_TREASURE   8		/* Item is a treasure, not gold	*/
#define ITEM_ARMOR      9		/* Item is armor		*/
#define ITEM_POTION    10 		/* Item is a potion		*/
#define ITEM_WORN      11		/* Unimplemented		*/
#define ITEM_OTHER     12		/* Misc object			*/
#define ITEM_TRASH     13		/* Trash - shopkeeps won't buy	*/
#define ITEM_TRAP      14		/* Unimplemented		*/
#define ITEM_CONTAINER 15		/* Item is a container		*/
#define ITEM_NOTE      16		/* Item is note 		*/
#define ITEM_DRINKCON  17		/* Item is a drink container	*/
#define ITEM_KEY       18		/* Item is a key		*/
#define ITEM_FOOD      19		/* Item is food			*/
#define ITEM_MONEY     20		/* Item is money (gold)		*/
#define ITEM_PEN       21		/* Item is a pen		*/
#define ITEM_BOAT      22		/* Item is a boat		*/
#define ITEM_FOUNTAIN  23		/* Item is a fountain		*/
#define ITEM_BOOK      24       /**** Item is book */
#define ITEM_INGRADIENT 25      /**** Item is magical ingradient */


/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE		(1 << 0)  /* Item can be takes		*/
#define ITEM_WEAR_FINGER	(1 << 1)  /* Can be worn on finger	*/
#define ITEM_WEAR_NECK		(1 << 2)  /* Can be worn around neck 	*/
#define ITEM_WEAR_BODY		(1 << 3)  /* Can be worn on body 	*/
#define ITEM_WEAR_HEAD		(1 << 4)  /* Can be worn on head 	*/
#define ITEM_WEAR_LEGS		(1 << 5)  /* Can be worn on legs	*/
#define ITEM_WEAR_FEET		(1 << 6)  /* Can be worn on feet	*/
#define ITEM_WEAR_HANDS		(1 << 7)  /* Can be worn on hands	*/
#define ITEM_WEAR_ARMS		(1 << 8)  /* Can be worn on arms	*/
#define ITEM_WEAR_SHIELD	(1 << 9)  /* Can be used as a shield	*/
#define ITEM_WEAR_ABOUT		(1 << 10) /* Can be worn about body 	*/
#define ITEM_WEAR_WAIST 	(1 << 11) /* Can be worn around waist 	*/
#define ITEM_WEAR_WRIST		(1 << 12) /* Can be worn on wrist 	*/
#define ITEM_WEAR_WIELD		(1 << 13) /* Can be wielded		*/
#define ITEM_WEAR_HOLD		(1 << 14) /* Can be held		*/
#define ITEM_WEAR_BOTHS     (1 << 15)


/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW          (1 << 0)	/* Item is glowing		*/
#define ITEM_HUM           (1 << 1)	/* Item is humming		*/
#define ITEM_NORENT        (1 << 2)	/* Item cannot be rented	*/
#define ITEM_NODONATE      (1 << 3)	/* Item cannot be donated	*/
#define ITEM_NOINVIS	   (1 << 4)	/* Item cannot be made invis	*/
#define ITEM_INVISIBLE     (1 << 5)	/* Item is invisible		*/
#define ITEM_MAGIC         (1 << 6)	/* Item is magical		*/
#define ITEM_NODROP        (1 << 7)	/* Item is cursed: can't drop	*/
#define ITEM_BLESS         (1 << 8)	/* Item is blessed		*/
#define ITEM_NOSELL        (1 << 9)	/* Not usable by good people	*/
#define ITEM_DECAY         (1 << 10)	/* Not usable by evil people	*/
#define ITEM_ZONEDECAY     (1 << 11)	/* Not usable by neutral people	*/
#define ITEM_NODISARM      (1 << 12)	/* Not usable by mages		*/
#define ITEM_NODECAY       (1 << 13)
#define ITEM_POISONED      (1 << 14)
#define ITEM_SHARPEN       (1 << 15)
#define ITEM_ARMORED       (1 << 16)
#define ITEM_DAY		   (1 << 17)
#define ITEM_NIGHT         (1 << 18)
#define ITEM_FULLMOON      (1 << 19)
#define ITEM_WINTER        (1 << 20)
#define ITEM_SPRING        (1 << 21)
#define ITEM_SUMMER        (1 << 22)
#define ITEM_AUTUMN        (1 << 23)
#define ITEM_SWIMMING      (1 << 24)
#define ITEM_FLYING        (1 << 25)
#define ITEM_THROWING      (1 << 26)
#define ITEM_TICKTIMER     (1 << 27)

#define ITEM_NO_MONO       (1 << 0)
#define ITEM_NO_POLY       (1 << 1)
#define ITEM_NO_NEUTRAL    (1 << 2)
#define ITEM_NO_MAGIC      (1 << 3)
#define ITEM_NO_CLERIC     (1 << 4)
#define ITEM_NO_THIEF      (1 << 5)
#define ITEM_NO_WARRIOR    (1 << 6)
#define ITEM_NO_ASSASINE   (1 << 7)
#define ITEM_NO_GUARD      (1 << 8)
#define ITEM_NO_PALADINE   (1 << 9)
#define ITEM_NO_RANGER     (1 << 10)
#define ITEM_NO_SMITH      (1 << 11)
#define ITEM_NO_MERCHANT   (1 << 12)
#define ITEM_NO_DRUID      (1 << 13)

#define ITEM_AN_MONO       (1 << 0)
#define ITEM_AN_POLY       (1 << 1)
#define ITEM_AN_NEUTRAL    (1 << 2)
#define ITEM_AN_MAGIC      (1 << 3)
#define ITEM_AN_CLERIC     (1 << 4)
#define ITEM_AN_THIEF      (1 << 5)
#define ITEM_AN_WARRIOR    (1 << 6)
#define ITEM_AN_ASSASINE   (1 << 7)
#define ITEM_AN_GUARD      (1 << 8)
#define ITEM_AN_PALADINE   (1 << 9)
#define ITEM_AN_RANGER     (1 << 10)
#define ITEM_AN_SMITH      (1 << 11)
#define ITEM_AN_MERCHANT   (1 << 12)
#define ITEM_AN_DRUID      (1 << 13)



/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE              0	/* No effect			*/
#define APPLY_STR               1	/* Apply to strength		*/
#define APPLY_DEX               2	/* Apply to dexterity		*/
#define APPLY_INT               3	/* Apply to constitution	*/
#define APPLY_WIS               4	/* Apply to wisdom		*/
#define APPLY_CON               5	/* Apply to constitution	*/
#define APPLY_CHA		        6	/* Apply to charisma		*/
#define APPLY_CLASS             7	/* Reserved			*/
#define APPLY_LEVEL             8	/* Reserved			*/
#define APPLY_AGE               9	/* Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/* Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/* Apply to height		*/
#define APPLY_MANAREG          12	/* Apply to max mana		*/
#define APPLY_HIT              13	/* Apply to max hit points	*/
#define APPLY_MOVE             14	/* Apply to max move points	*/
#define APPLY_GOLD             15	/* Reserved			*/
#define APPLY_EXP              16	/* Reserved			*/
#define APPLY_AC               17	/* Apply to Armor Class		*/
#define APPLY_HITROLL          18	/* Apply to hitroll		*/
#define APPLY_DAMROLL          19	/* Apply to damage roll		*/
#define APPLY_SAVING_PARA      20	/* Apply to save throw: paralz	*/
#define APPLY_SAVING_ROD       21	/* Apply to save throw: rods	*/
#define APPLY_SAVING_PETRI     22	/* Apply to save throw: petrif	*/
#define APPLY_SAVING_BREATH    23	/* Apply to save throw: breath	*/
#define APPLY_SAVING_SPELL     24	/* Apply to save throw: spells	*/
#define APPLY_HITREG           25
#define APPLY_MOVEREG          26
#define APPLY_C1               27
#define APPLY_C2               28
#define APPLY_C3               29
#define APPLY_C4               30
#define APPLY_C5               31
#define APPLY_C6               32
#define APPLY_C7               33
#define APPLY_C8               34
#define APPLY_C9               35
#define APPLY_SIZE             36
#define APPLY_ARMOUR           37
#define APPLY_POISON           38
#define APPLY_SAVING_BASIC     39
#define APPLY_CAST_SUCCESS     40
#define APPLY_MORALE           41
#define APPLY_INITIATIVE       42
#define APPLY_RELIGION		   43
#define APPLY_ABSORBE          44

#define MAT_NONE               0
#define MAT_BULAT              1
#define MAT_BRONZE             2
#define MAT_IRON               3
#define MAT_STEEL              4
#define MAT_SWORDSSTEEL        5
#define MAT_COLOR              6
#define MAT_CRYSTALL           7
#define MAT_WOOD               8
#define MAT_SUPERWOOD          9
#define MAT_FARFOR             10
#define MAT_GLASS              11
#define MAT_ROCK               12
#define MAT_BONE               13
#define MAT_MATERIA            14
#define MAT_SKIN               15
#define MAT_ORGANIC            16
#define MAT_PAPER              17
#define MAT_DIAMOND            18


#define TRACK_NPC              (1 << 0)
#define TRACK_HIDE             (1 << 1)

/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/* Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/* Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/* Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/* Container is locked		*/


/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0
#define LIQ_BEER       1
#define LIQ_WINE       2
#define LIQ_ALE        3
#define LIQ_QUAS       4
#define LIQ_BRANDY     5
#define LIQ_MORSE      6
#define LIQ_VODKA      7
#define LIQ_BRAGA      8
#define LIQ_MED        9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFE      12
#define LIQ_BLOOD      13
#define LIQ_SALTWATER  14
#define LIQ_CLEARWATER 15


/* other miscellaneous defines *******************************************/


/* Player conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2


/* Sun state for weather_data */
#define SUN_DARK	0
#define SUN_RISE	1
#define SUN_LIGHT	2
#define SUN_SET		3

/* Moon change type */
#define MOON_INCREASE 0
#define MOON_DECREASE 1

/* Sky conditions for weather_data */
#define SKY_CLOUDLESS	0
#define SKY_CLOUDY	    1
#define SKY_RAINING	    2
#define SKY_LIGHTNING	3


/* Rent codes */
#define RENT_UNDEF      0
#define RENT_CRASH      1
#define RENT_RENTED     2
#define RENT_CRYO       3
#define RENT_FORCED     4
#define RENT_TIMEDOUT   5

#define EXTRA_FAILHIDE       (1 << 0)
#define EXTRA_FAILSNEAK      (1 << 1)
#define EXTRA_FAILCAMOUFLAGE (1 << 2)


/* other #defined constants **********************************************/

/*
 * **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for
 * details.
 *
 * LVL_IMPL should always be the HIGHEST possible immortal level, and
 * LVL_IMMORT should always be the LOWEST immortal level.  The number of
 * mortal levels will always be LVL_IMMORT - 1.
 */
#define LVL_IMPL	34
#define LVL_GRGOD	33
#define LVL_BUILDER	33
#define LVL_GOD		32
#define LVL_IMMORT	31

/* Level of the 'freeze' command */
#define LVL_FREEZE	LVL_GRGOD

#define NUM_OF_DIRS	6	/* number of directions in a room (nsewud) */
#define MAGIC_NUMBER	(0x06)	/* Arbitrary number that won't be in a string */

#define OPT_USEC	100000	/* 10 passes per second */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
#define RL_SEC		* PASSES_PER_SEC

#define PULSE_ZONE      (10 RL_SEC)
#define PULSE_MOBILE    (10 RL_SEC)
#define PULSE_VIOLENCE  (2 RL_SEC)

/* Variables for the output buffering system */
#define MAX_SOCK_BUF            (12 * 1024) /* Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH       256          /* Max length of prompt        */
#define GARBAGE_SPACE			32          /* Space for **OVERFLOW** etc  */
#define SMALL_BUFSIZE			1024        /* Static output buffer size   */
/* Max amount of output that can be buffered */
#define LARGE_BUFSIZE	   		(MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

#define HISTORY_SIZE			5	/* Keep last 5 commands. */
#define MAX_STRING_LENGTH		8192
#define MAX_EXTEND_LENGTH		0xFFFF
#define MAX_INPUT_LENGTH		256	/* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH	        512     /* Max size of *raw* input */
#define MAX_MESSAGES			600
#define MAX_NAME_LENGTH			20      /* Used in char_file_u *DO*NOT*CHANGE* */
#define MIN_NAME_LENGTH                 4
#define MAX_PWD_LENGTH			10      /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TITLE_LENGTH		80      /* Used in char_file_u *DO*NOT*CHANGE* */
#define HOST_LENGTH			30      /* Used in char_file_u *DO*NOT*CHANGE* */
#define EXDSCR_LENGTH			240     /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TONGUE			3       /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_SKILLS			200     /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_SPELLS       		350     /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFFECT		 	32      /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_OBJ_AFFECT	 		6 	/* Used in obj_file_elem *DO*NOT*CHANGE* */
#define MAX_TIMED_SKILLS 		16 /* Used in obj_file_elem *DO*NOT*CHANGE* */

/*
 * A MAX_PWD_LENGTH of 10 will cause BSD-derived systems with MD5 passwords
 * and GNU libc 2 passwords to be truncated.  On BSD this will enable anyone
 * with a name longer than 5 character to log in with any password.  If you
 * have such a system, it is suggested you change the limit to 20.
 *
 * Please note that this will erase your player files.  If you are not
 * prepared to do so, simply erase these lines but heed the above warning.
 */
#if defined(HAVE_UNSAFE_CRYPT) && MAX_PWD_LENGTH == 10
#error You need to increase MAX_PWD_LENGTH to at least 20.
#error See the comment near these errors for more explanation.
#endif

/**********************************************************************
* Structures                                                          *
**********************************************************************/


typedef signed char		sbyte;
typedef unsigned char	ubyte;
typedef short int		sh_int;
typedef short int		ush_int;
#if !defined(__cplusplus)	/* Anyone know a portable method? */
typedef char			bool;
#endif

#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)	/* Hm, sysdep.h? */
typedef char			byte;
#endif

typedef int	room_vnum;	/* A room's vnum type */
typedef int	obj_vnum;	/* An object's vnum type */
typedef int	mob_vnum;	/* A mob's vnum type */
typedef int	zone_vnum;	/* A virtual zone number.	*/

typedef int	room_rnum;	/* A room's real (internal) number type */
typedef int	obj_rnum;	/* An object's real (internal) num type */
typedef int	mob_rnum;	/* A mobile's real (internal) num type */
typedef int	zone_rnum;	/* A zone's real (array index) number.	*/


/************* WARNING **********************************************/
/* This structure describe new bitvector structure                  */
typedef long int bitvector_t;
struct new_flag {
   int flags[4];
};

#define INT_ZERRO (0 << 30)
#define INT_ONE   (1 << 30)
#define INT_TWO   (2 << 30)
#define INT_THREE (3 << 30)
#define GET_FLAG(value,flag)  ((unsigned long)flag < (unsigned long)INT_ONE   ? value.flags[0] : \
                               (unsigned long)flag < (unsigned long)INT_TWO   ? value.flags[1] : \
                               (unsigned long)flag < (unsigned long)INT_THREE ? value.flags[2] : value.flags[3])


/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
   char	*keyword;                 /* Keyword in look/examine          */
   char	*description;             /* What to see                      */
   struct extra_descr_data *next; /* Next in list                     */
};


/* object-related structures ******************************************/


/* object flags; used in obj_data */
#define NUM_OBJ_VAL_POSITIONS 4

struct obj_flag_data
{  int  value[NUM_OBJ_VAL_POSITIONS];
   byte   type_flag;	        /* Type of item			        */
   int	  wear_flags;	        /* Where you can wear it	    */
   struct new_flag extra_flags; /* If it hums, glows, etc.	    */
   int	  weight;		        /* Weigt what else              */
   int	  cost;		            /* Value when sold (gp.)        */
   int	  cost_per_day_on;      /* Rent to keep pr. real day if wear       */
   int    cost_per_day_off;     /* Rent to keep pr. real day if in inv     */
   int	  Obj_timer;	        /* Timer for object             */
   struct new_flag bitvector;	/* To set chars bits            */

   struct new_flag affects;
   struct new_flag anti_flag;
   struct new_flag no_flag;
   int    Obj_sex;
   int    Obj_spell;
   int    Obj_level;
   int    Obj_skill;
   int    Obj_max;
   int    Obj_cur;
   int    Obj_mater;
   int    Obj_owner;
   int    Obj_destroyer;
   int    Obj_zone;
};


/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_affected_type
{
   byte location;      /* Which ability to change (APPLY_XXX) */
   sbyte modifier;     /* How much it changes by              */
};


/* ================== Memory Structure for Objects ================== */
struct obj_data {
   obj_vnum item_number;	/* Where in data-base			   */
   room_rnum in_room;		/* In what room -1 when conta/carr */

   struct obj_flag_data     obj_flags;/* Object information       */
   struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects */

   char	*name;                    /* Title of object :get etc.        */
   char	*description;		      /* When in room                     */
   char	*short_description;       /* when worn/carry/in cont.         */
   char	*action_description;      /* What to write when used          */
   struct extra_descr_data *ex_description; /* extra descriptions     */
   struct char_data *carried_by;  /* Carried by :NULL in room/conta   */
   struct char_data *worn_by;	  /* Worn by?			      */
   short  int worn_on;		      /* Worn where?		      */

   struct obj_data *in_obj;       /* In what object NULL when none    */
   struct obj_data *contains;     /* Contains objects                 */

   long id;                       /* used by DG triggers              */
   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;    /* script info for the object       */

   struct obj_data *next_content; /* For 'contains' lists             */
   struct obj_data *next;         /* For the object list              */
   int    room_was_in;
   char   *PNames[6];
};
/* ======================================================================= */


/* ====================== File Element for Objects ======================= */
/*                 BEWARE: Changing it will ruin rent files		   */
struct obj_file_elem
{  obj_vnum item_number;

#if USE_AUTOEQ
   short  int location;
#endif
   int    value[NUM_OBJ_VAL_POSITIONS];
   struct new_flag extra_flags;
   int	  weight;
   int	  timer;
   int    maxstate;
   int    curstate;
   int    owner;
   int    wear_flags;
   int    mater;
   struct new_flag bitvector;
   struct obj_affected_type affected[MAX_OBJ_AFFECT];
};


/* header block for rent files.  BEWARE: Changing it will ruin rent files  */
struct save_rent_info {
   int	time;
   int	rentcode;
   int	net_cost_per_diem;
   int	gold;
   int	account;
   int	nitems;
   int	oitems;
   int	spare1;
   int	spare2;
   int	spare3;
   int	spare4;
   int	spare5;
   int	spare6;
   int	spare7;
};

struct save_time_info {
   int vnum;
   int timer;
};

struct save_info {
   struct save_rent_info rent;
   struct save_time_info time[2];
};

/* ======================================================================= */


/* room-related structures ************************************************/


struct room_direction_data
{  char	*general_description;       /* When look DIR.			*/

   char	*keyword;		            /* for open/close			*/

   sh_int  exit_info;	            /* Exit info			    */
   obj_vnum key;	  	            /* Key's number (-1 for no key)*/
   room_rnum to_room;		        /* Where direction leads (NOWHERE)*/
};

struct   track_data
{ int    track_info;                 /* bitvector */
  int    who;                        /* real_number for NPC, IDNUM for PC */
  int    time_income[6];             /* time bitvector */
  int    time_outgone[6];
  struct track_data *next;
};

struct weather_control
{ int  rainlevel;
  int  snowlevel;
  int  icelevel;
  int  sky;
  int  weather_type;
  int  duration;
};


/* ================== Memory Structure for room ======================= */
struct room_data
{  room_vnum number;		    /* Rooms number	(vnum)		          */
   zone_rnum zone;              /* Room zone (for resetting)          */
   int	sector_type;            /* sector type (move/hide)            */
   int  sector_state;           /**** External, change by weather     */
   char	*name;                  /* Rooms name 'You are ...'           */
   char	*description;           /* Shown when entered                 */
   struct extra_descr_data *ex_description; /* for examine/look       */
   struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
   struct new_flag room_flags;	/* DEATH,DARK ... etc */

   byte    light;                     /* Number of lightsources in room */
   byte    glight;                    /* Number of lightness person     */
   byte    gdark;                     /* Number of darkness  person     */
   struct  weather_control weather;   /* Weather state for room */
   SPECIAL(*func);

   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data     *script;       /* script info for the object*/
   struct track_data      *track;

   struct obj_data *contents;   /* List of items in room              */
   struct char_data *people;    /* List of NPC / PC in room           */

   ubyte  fires;                /* Time when fires                    */
   ubyte  forbidden;            /* Time when room forbidden for mobs  */
   ubyte  ices;                 /* Time when ices restore */
   int    portal_room;
   ubyte  portal_time;
};
/* ====================================================================== */


/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct
{  long	  id;
   long   time;
   struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data
{  int hours, day, month;
   sh_int year;
};


/* These data contain information about a players time data */
struct time_data
{  time_t birth;    /* This represents the characters age                */
   time_t logon;    /* Time of the last logon (used to calculate played) */
   int	played;     /* This is the total accumulated time played in secs */
};


/* general player-related info, usually PC's and NPC's */
struct char_player_data
{  char	passwd[MAX_PWD_LENGTH+1]; /* character's password      */
   char	*name;	       /* PC / NPC s name (kill ...  )         */
   char	*short_descr;  /* for NPC 'actions'                    */
   char	*long_descr;   /* for 'look'			       */
   char	*description;  /* Extra descriptions                   */
   char	*title;        /* PC / NPC's title                     */
   byte sex;           /* PC / NPC's sex                       */
   byte chclass;       /* PC / NPC's class		       */
   byte level;         /* PC / NPC's level                     */
   int	hometown;      /* PC s Hometown (zone)                 */
   struct time_data time;  /* PC's AGE in days                 */
   ubyte weight;       /* PC / NPC's weight                    */
   ubyte height;       /* PC / NPC's height                    */

   char* PNames[6];
   ubyte Religion;
   ubyte Race;
   ubyte Side;
   ubyte Lows;
};


/* Char's abilities.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_ability_data
{  ubyte Skills[MAX_SKILLS+1];	/* array of skills plus skill 0		*/
   ubyte SplKnw[MAX_SPELLS+1];  /* array of SPELL_KNOW_TYPE         */
   ubyte SplMem[MAX_SPELLS+1];  /* array of MEMed SPELLS            */
   sbyte str;
   sbyte intel;
   sbyte wis;
   sbyte dex;
   sbyte con;
   sbyte cha;
   sbyte size;
   sbyte hitroll;
   sbyte damroll;
   sbyte armor;
};

/* Char's additional abilities. Used only while work */
struct char_played_ability_data
{  int  str_add;
   int  intel_add;
   int  wis_add;
   int  dex_add;
   int  con_add;
   int  cha_add;
   int  weight_add;
   int  height_add;
   int  size_add;
   int  age_add;
   int  hit_add;
   int  move_add;
   int  hitreg;
   int  movereg;
   int  manareg;
   sbyte  slot_add[10];
   int  armour;
   int  ac_add;
   int  hr_add;
   int  dr_add;
   int  absorb;
   int  morale_add;
   int  cast_success;
   int  initiative_add;
   int  poison_add;
   int  pray_add;
   sh_int apply_saving_throw[6]; /* Saving throw (Bonuses)	*/
};


/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_point_data
{  sh_int hit;
   sh_int max_hit;      /* Max hit for PC/NPC                      */
   sh_int move;
   sh_int max_move;     /* Max move for PC/NPC                     */
   int	gold;           /* Money carried                           */
   long	bank_gold;  	/* Gold the char has in a bank account	   */
   long	exp;            /* The experience of the player            */
   long pk_counter;     /* pk counter list */
};


/*
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved
{  int	alignment;		/* +-1000 for alignments                */
   long	idnum;			/* player's idnum; -1 for mobiles	*/
   struct new_flag act;	/* act flag for NPC's; player flag for PC's */

   struct new_flag affected_by;
	     		        /* Bitvector for spells/skills affected by */
};


/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data
{  struct char_data *fighting;	/* Opponent */
   struct char_data *hunting;	/* Char hunted by this char */

   byte   position;		        /* Standing, fighting, sleeping, etc. */

   int	  carry_weight;		    /* Carried weight */
   byte   carry_items;		    /* Number of items carried	*/
   int	  timer;	            /* Timer for update	*/

   struct char_special_data_saved saved; /* constants saved in plrfile	*/
};


/*
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
struct player_special_data_saved
{  byte PADDING0;		    /* used to be spells_to_learn		*/
   bool talks[MAX_TONGUE];	/* PC s Tongues 0 for NPC		*/
   int	wimp_level;		    /* Below this # of hit points, flee!	*/
   int  freeze_level;		/* Level of god who froze char, if any	*/
   int  invis_level;		/* level of invisibility		*/
   room_vnum load_room;		/* Which room to place char in		*/
   struct    new_flag	pref;	/* preference flags for PC's.		*/
   int   bad_pws;		/* number of bad password attemps	*/
   int   conditions[3];         /* Drunk, full, thirsty			*/

   /* spares below for future expansion.  You can change the names from
      'sparen' to something meaningful, but don't change the order.  */

   int Side;              /****/
   int Religion;          /****/
   int Race;              /****/
   int Lows;              /****/
   int DrunkState;
   int Prelimit;
   int glory;
   int olc_zone;
   int unique;
   int HouseRank;
   int Remorts;
   int spare11;
   int spare12;
   int spare13;
   int spare14;
   int spare15;

   long NameDuration;
   long	GodsLike;
   long	GodsDuration;
   long	MuteDuration;
   long	FreezeDuration;
   long	HellDuration;
   long HouseUID;
   long LastLogon;
   long spare08;
   long spare09;
   long spare0A;
   long spare0B;
   long spare0C;
   long spare0D;
   long spare0E;
   long spare0F;

   char EMail[128];
   char spare001[128];
   char spare002[128];
   char spare003[128];
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the playerfile.  This structure can
 * be changed freely; beware, though, that changing the contents of
 * player_special_data_saved will corrupt the playerfile.
 */
struct player_special_data
{  struct player_special_data_saved saved;

   char	 *poofin;		    /* Description on arrival of a god. */
   char	 *poofout;		    /* Description upon a god's exit.   */
   struct alias_data *aliases;	    /* Character's aliases		*/
   long   last_tell;		    /* idnum of last tell from		*/
   void  *last_olc_targ;	    /* olc control			*/
   int    last_olc_mode;	    /* olc control			*/
   int    may_rent;                 /* PK control                       */
};


/* Specials used by NPCs, not PCs */
struct mob_special_data
{  byte last_direction;     /* The last direction the monster went     */
   int	attack_type;        /* The Attack Type Bitvector for NPC's     */
   byte default_pos;        /* Default position for NPC                */
   memory_rec *memory;	    /* List of attackers to remember	       */
   byte damnodice;          /* The number of damage dice's	           */
   byte damsizedice;        /* The size of the damage dice's           */
   int  dest[MAX_DEST];
   int  dest_dir;
   int  dest_pos;
   int  dest_count;
   int  activity;
   struct new_flag npc_flags;
   byte ExtraAttack;
   byte LikeWork;
   byte MaxFactor;
   int  GoldNoDs;
   int  GoldSiDs;
   int  HorseState;
   int  LastRoom;
   char *Questor;
};


/* An affect structure.  Used in char_file_u *DO*NOT*CHANGE* */
struct affected_type
{  sh_int type;            /* The type of spell that caused this      */
   sh_int duration;        /* For how long its effects will last      */
   sbyte  modifier;        /* This is added to apropriate ability     */
   byte   location;        /* Tells which ability to change(APPLY_XXX)*/
   long   battleflag;      /**** SUCH AS HOLD,SIELENCE etc **/
   long   bitvector;       /* Tells which bits to set (AFF_XXX) */

   struct affected_type *next;
};

struct timed_type
{ ubyte      skill;               /* Number of used skill/spell */
  ubyte      time;                /* Time for next using        */
  struct     timed_type *next;
};


/* Structure used for chars following other chars */
struct follow_type
{  struct char_data *follower;
   struct follow_type *next;
};

/* Structure used for char PK-memory */
struct PK_Memory_type
{  int      unique;     /* unique for pkill_player */
   int      pkills;     /* how much kills IDNUM   */
   long     pkilllast;
// Добавдяем два поля времени последнего пк....
   long	    pkilllast1;
   long	    pkilllast2;   
   long	    pkilllast3;   
   long	    pkilllast4;      
//
   int      revenge;    /* how much steal IDNUM   */
   long     revengelast;
   int      pthief;
   long     thieflast;
   struct   PK_Memory_type *next;
};

/* Structure used for extra_attack - bash, kick, diasrm, chopoff, etc */
struct extra_attack_type
{ int    used_skill;
  struct char_data *victim;
};

struct cast_attack_type
{ int    spellnum;
  struct char_data *tch;
  struct obj_data  *tobj;
};

/* Structure used for tracking a mob */
struct track_info
{ int  track_info;
  int  who;
  int  dirs;
};

/* Structure used for helpers */
struct helper_data_type
{ int mob_vnum;
   struct helper_data_type *next_helper;
};

/* Structure used for questing */
struct quest_data
{ int count;
  int *quests;
};

struct mob_kill_data
{ int count;
  int *howmany;
  int *vnum;
};


/* ================== Structure for player/non-player ===================== */
struct char_data
{  int pfilepos;			 /* playerfile pos		          */
   mob_rnum nr;              /* Mob's rnum			          */
   room_rnum in_room;        /* Location (real room number)	  */
   room_rnum was_in_room;	 /* location for linkdead people  */
   int wait;				 /* wait for how many loops	      */

   struct char_player_data         player;          /* Normal data                   */
   struct char_played_ability_data add_abils;       /* Abilities that add to main */
   struct char_ability_data        real_abils;	    /* Abilities without modifiers   */
   struct char_point_data          points;          /* Points                       */
   struct char_special_data        char_specials;	/* PC/NPC specials	  */
   struct player_special_data      *player_specials;/* PC specials		  */
   struct mob_special_data         mob_specials;	/* NPC specials		  */

   struct affected_type *affected;       /* affected by what spells       */
   struct timed_type    *timed;          /* use which timed skill/spells  */
   struct obj_data *equipment[NUM_WEARS];/* Equipment array               */

   struct obj_data *carrying;            /* Head of list                  */
   struct descriptor_data *desc;         /* NULL for mobiles              */

   long id;                            /* used by DG triggers             */
   struct trig_proto_list *proto_script; /* list of default triggers      */
   struct script_data *script;         /* script info for the object      */
   struct script_memory *memory;       /* for mob memory triggers         */

   struct char_data *next_in_room;     /* For room->people - list         */
   struct char_data *next;             /* For either monster or ppl-list  */
   struct char_data *next_fighting;    /* For fighting list               */

   struct follow_type *followers;        /* List of chars followers       */
   struct char_data *master;             /* Who is char following?        */

   ubyte                           Memory[MAX_SPELLS+1]; /* какие заклинания запоминаем        */
   int                             ManaMemNeeded;        /* сколько маны нужно для запоминания */
   int	                           ManaMemStored;        /* сколько маны уже накопили          */
   struct quest_data               Questing;
   struct mob_kill_data            MobKill;
   int                             CasterLevel;
   int                             DamageLevel;
   struct PK_Memory_type          *pk_list;
   struct helper_data_type        *helpers;
   int                             track_dirs;
   int                             CheckAggressive;
   struct new_flag                 Temporary;
   int                             ExtractTimer;

   int                             Initiative;
   int                             BattleCounter;
   struct new_flag                 BattleAffects;
   struct char_data*               Protecting;
   struct char_data*               Touching;
   int                             Poisoner;
   struct extra_attack_type        extra_attack;
   struct cast_attack_type         cast_attack;
};
/* ====================================================================== */


/* ==================== File Structure for Player ======================= */
/*             BEWARE: Changing it will ruin the playerfile		  */
struct char_file_u {
   /* char_player_data */
   char	  name[MAX_NAME_LENGTH+1];
   char   PNames[6][MAX_NAME_LENGTH+1];             /****/
   char	  description[EXDSCR_LENGTH];
   char	  title[MAX_TITLE_LENGTH+1];
   byte   sex;
   byte   chclass;
   byte   level;
   sh_int hometown;
   time_t birth;     /* Time of birth of character     */
   long   played;    /* Number of secs played in total */
   ubyte  weight;
   ubyte  height;

   char	pwd[MAX_PWD_LENGTH+1];    /* character's password */

   struct char_special_data_saved   char_specials_saved;
   struct player_special_data_saved player_specials_saved;
   struct char_ability_data         abilities;
   struct char_point_data           points;
   struct affected_type             affected[MAX_AFFECT];
   struct timed_type                timed[MAX_TIMED_SKILLS];

   time_t last_logon;	        	/* Time (in secs) of last logon */
   char host[HOST_LENGTH+1];	    /* host of last logon */
};
/* ====================================================================== */


/* descriptor-related structures ******************************************/


struct txt_block {
   char	*text;
   int aliased;
   struct txt_block *next;
};


struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};


struct descriptor_data {
   socket_t	descriptor;	/* file descriptor for socket		*/
   char	host[HOST_LENGTH+1];	/* hostname				*/
   byte	bad_pws;		/* number of bad pw attemps this login	*/
   byte idle_tics;		/* tics idle at password prompt		*/
   int	connected;		/* mode of 'connectedness'		*/
   int	desc_num;		/* unique num assigned to desc		*/
   time_t input_time;
   time_t login_time;		/* when the person connected		*/
   char *showstr_head;		/* for keeping track of an internal str	*/
   char **showstr_vector;	/* for paging through texts		*/
   int  showstr_count;		/* number of pages to page through	*/
   int  showstr_page;		/* which page are we currently showing?	*/
   char	**str;			/* for the modify-str system		*/
   size_t max_str;	    /*		-			*/
   char *backstr;		/* added for handling abort buffers	*/
   long	mail_to;		/* name for mail system			*/
   int	has_prompt;		/* is the user at a prompt?             */
   char	inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input		*/
   char	last_input[MAX_INPUT_LENGTH]; /* the last input			*/
   char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer		*/
   char *output;		/* ptr to the current output buffer	*/
   char **history;		/* History of commands, for ! mostly.	*/
   int	history_pos;		/* Circular array position.		*/
   int  bufptr;			/* ptr to end of current output		*/
   int	bufspace;		/* space left in the output buffer	*/
   struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
   struct txt_q input;		/* q of unprocessed input		*/
   struct char_data *character;	/* linked to char			*/
   struct char_data *original;	/* original char if switched		*/
   struct descriptor_data *snooping; /* Who is this char snooping	*/
   struct descriptor_data *snoop_by; /* And who is snooping this char	*/
   struct descriptor_data *next; /* link to next descriptor		*/
   struct olc_data *olc;	     /*. OLC info - defined in olc.h   .*/
   ubyte  keytable;
};


/* other miscellaneous structures ***************************************/


struct msg_type {
   char	*attacker_msg;  /* message to attacker */
   char	*victim_msg;    /* message to victim   */
   char	*room_msg;      /* message to room     */
};


struct message_type {
   struct msg_type die_msg;	/* messages when death			*/
   struct msg_type miss_msg;	/* messages when miss			*/
   struct msg_type hit_msg;	/* messages when hit			*/
   struct msg_type god_msg;	/* messages when hit on god		*/
   struct message_type *next;	/* to next messages of this kind.	*/
};


struct message_list {
   int	a_type;			/* Attack type				*/
   int	number_of_attacks;	/* How many attack messages to chose from. */
   struct message_type *msg;	/* List of messages.			*/
};


struct dex_skill_type
{  int p_pocket;
   int p_locks;
   int traps;
   int sneak;
   int hide;
};


struct dex_app_type
{  int reaction;
   int miss_att;
   int defensive;
};


struct str_app_type
{  int tohit;    /* To Hit (THAC0) Bonus/Penalty        */
   int todam;    /* Damage Bonus/Penalty                */
   int carry_w;  /* Maximum weight that can be carrried */
   int wield_w;  /* Maximum weight that can be wielded  */
   int hold_w;   /* MAXIMUM WEIGHT THAT CAN BE HELDED   */
};


struct wis_app_type
{  int   spell_additional;     /* bitvector */
   int   max_learn_l20;        /* MAX SKILL on LEVEL 20        */
   int   spell_success;        /* spell using success          */
   int   char_savings;         /* saving spells (damage)       */
   int   max_skills;
};


struct int_app_type
{  int spell_aknowlege;    /* chance to know spell               */
   int to_skilluse;        /* ADD CHANSE FOR USING SKILL         */
   int mana_per_tic;
   int spell_success;      /*  max count of spell on 1s level    */
   int improove;           /* chance to improove skill           */
   int observation;        /* chance to use SKILL_AWAKE/CRITICAL */
};

struct con_app_type
{  int hitp;
   int ressurection;
   int affect_saving;      /* saving spells (affects)  */
   int poison_saving;
   int critic_saving;
};

struct cha_app_type
{  int leadership;
   int charms;
   int morale;
   int illusive;
};

struct size_app_type
{  int ac;           /* ADD VALUE FOR AC           */
   int interpolate;  /* ADD VALUE FOR SOME SKILLS  */
   int initiative;
   int shocking;
};

struct weapon_app_type
{  int shocking;
   int bashing;
   int parrying;
};

struct extra_affects_type
{ int affect;
  int set_or_clear;
};

struct class_app_type
{ int    unknown_weapon_fault;
  int    koef_con;
  int    base_con;
  int    min_con;
  int    max_con;

  struct extra_affects_type *extra_affects;
  struct obj_affected_type  *extra_modifiers;
};

struct race_app_type
{ struct extra_affects_type *extra_affects;
  struct obj_affected_type  *extra_modifiers;
};

struct weather_data
{  int  hours_go;              /* Time life from reboot */

   int	pressure;	           /* How is the pressure ( Mb )            */
   int  press_last_day;        /* Average pressure last day             */
   int  press_last_week;       /* Average pressure last week            */

   int  temperature;           /* How is the temperature (C)            */
   int  temp_last_day;         /* Average temperature last day          */
   int  temp_last_week;        /* Average temperature last week         */

   int  rainlevel;             /* Level of water from rains             */
   int  snowlevel;             /* Level of snow                         */
   int  icelevel;              /* Level of ice                          */

   int  weather_type;          /* bitvector - some values for month     */

   int	change;  	/* How fast and what way does it change. */
   int	sky;	    /* How is the sky.   */
   int	sunlight;	/* And how much sun. */
   int  moon_day;   /* And how much moon */
   int  season;
   int  week_day_mono;
   int  week_day_poly;
};

struct weapon_affect_types
{      int    aff_pos;
       int    aff_bitvector;
       int    aff_spell;
};

struct title_type
{  char	*title_m;
   char	*title_f;
   int	exp;
};


/* element in monster and object index-tables   */
struct index_data
{  int	vnum;	/* virtual number of this mob/obj		    */
   int	number;		/* number of existing units of this mob/obj	*/
   int  stored;  /* number of things in rent file            */
   SPECIAL(*func);

   char   *farg;         /* string argument for special function     */
   struct trig_data *proto;     /* for triggers... the trigger     */
};

/* linked list for mob/object prototype trigger lists */
struct trig_proto_list
{ int vnum;                             /* vnum of the trigger   */
  struct trig_proto_list *next;         /* next trigger          */
};

struct social_messg
{ /* No argument was supplied */
  int  ch_min_pos, ch_max_pos, vict_min_pos, vict_max_pos;
  char *char_no_arg;
  char *others_no_arg;

  /* An argument was there, and a victim was found */
  char *char_found;		/* if NULL, read no further, ignore args */
  char *others_found;
  char *vict_found;

  /* An argument was there, but no victim was found */
  char *not_found;
};



struct social_keyword
{ char *keyword;
  int  social_message;
};

extern struct social_messg   *soc_mess_list;
extern struct social_keyword *soc_keys_list;

struct auction_lot_type
{int item_id;
 struct obj_data *item;
 int seller_unique;
 struct char_data *seller;
 int buyer_unique;
 struct char_data *buyer;
 int prefect_unique;
 struct char_data *prefect;
 int cost;
 int tact;
};

struct pray_affect_type
{ int  metter;
  int  location;
  int  modifier;
  long bitvector;
  int  battleflag;
};

#define  DAY_EASTER    	-1

struct gods_celebrate_type
{ int  unique;         // Uniqum ID
  char *name;          // Celebrate
  int  from_day;       // Day of month, -1 and less if in range
  int  from_month;     // Month, -1 and less if relative
  int  duration;
  int  religion;       // Religion type
};

#define 	GAPPLY_NONE                 0
#define 	GAPPLY_SKILL_SUCCESS        1
#define 	GAPPLY_SPELL_SUCCESS        2
#define 	GAPPLY_SPELL_EFFECT         3
#define         GAPPLY_MODIFIER             4
#define         GAPPLY_AFFECT               5

struct gods_celebrate_apply_type
{ int    unique;
  int    gapply_type;
  int    what;
  int    modi;
  struct gods_celebrate_apply_type *next;
};
