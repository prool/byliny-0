/* ************************************************************************
*   File: db.h                                          Part of CircleMUD *
*  Usage: header file for database handling                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define USE_SINGLE_PLAYER 1

/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD	0
#define DB_BOOT_MOB	1
#define DB_BOOT_OBJ	2
#define DB_BOOT_ZON	3
#define DB_BOOT_SHP	4
#define DB_BOOT_HLP	5
#define DB_BOOT_TRG	6
#define DB_BOOT_SOCIAL 7

#define LIB_A       "A-E"
#define LIB_F       "F-J"
#define LIB_K       "K-O"
#define LIB_P       "P-T"
#define LIB_U       "U-Z"
#define LIB_Z       "ZZZ"

#if defined(CIRCLE_MACINTOSH)
#define LIB_WORLD	":world:"
#define LIB_TEXT	":text:"
#define LIB_TEXT_HELP	":text:help:"
#define LIB_MISC	":misc:"
#define LIB_ETC		":etc:"
#define LIB_PLRTEXT	":plrtext:"
#define LIB_PLROBJS	":plrobjs:"
#define LIB_PLRS    ":plrs:"
#define LIB_PLRALIAS	":plralias:"
#define LIB_HOUSE	":house:"
#define LIB_PLRVARS	":plrvars:"
#define LIB             ":lib:"
#define SLASH		":"
#elif defined(CIRCLE_AMIGA) || defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS) || defined(CIRCLE_ACORN) || defined(CIRCLE_VMS)
#define LIB_WORLD	"world/"
#define LIB_TEXT	"text/"
#define LIB_TEXT_HELP	"text/help/"
#define LIB_MISC	"misc/"
#define LIB_ETC		"etc/"
#define LIB_PLRTEXT	"plrtext/"
#define LIB_PLROBJS	"plrobjs/"
#define LIB_PLRS    "plrs/"
#define LIB_PLRALIAS	"plralias/"
#define LIB_HOUSE	"house/"
#define LIB_PLRVARS	"plrvars/"
#define LIB             "lib/"
#define SLASH		"/"
#else
#error "Unknown path components."
#endif

#define SUF_OBJS	"objs"
#define TEXT_SUF_OBJS	"textobjs"
#define TIME_SUF_OBJS	"timeobjs"
#define SUF_TEXT	"text"
#define SUF_ALIAS	"alias"
#define SUF_MEM		"mem"
#define SUF_PLAYER  "player"
#define SUF_PKILLER "pkiller"
#define SUF_QUESTS  "quests"
#define SUF_PMKILL	"mobkill"
#if defined(CIRCLE_AMIGA)
#define FASTBOOT_FILE   "/.fastboot"    /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "/.killscript"  /* autorun: shut mud down       */
#define PAUSE_FILE      "/pause"        /* autorun: don't restart mud   */
#elif defined(CIRCLE_MACINTOSH)
#define FASTBOOT_FILE	"::.fastboot"	/* autorun: boot without sleep	*/
#define KILLSCRIPT_FILE	"::.killscript"	/* autorun: shut mud down	*/
#define PAUSE_FILE	"::pause"	/* autorun: don't restart mud	*/
#else
#define FASTBOOT_FILE   "../.fastboot"  /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "../.killscript"/* autorun: shut mud down       */
#define PAUSE_FILE      "../pause"      /* autorun: don't restart mud   */
#endif

/* names of various files and directories */
#define INDEX_FILE	"index"		/* index of world files		*/
#define MINDEX_FILE	"index.mini"	/* ... and for mini-mud-mode	*/
#define WLD_PREFIX	LIB_WORLD"wld"SLASH	/* room definitions	*/
#define MOB_PREFIX	LIB_WORLD"mob"SLASH	/* monster prototypes	*/
#define OBJ_PREFIX	LIB_WORLD"obj"SLASH	/* object prototypes	*/
#define ZON_PREFIX	LIB_WORLD"zon"SLASH	/* zon defs & command tables */
#define SHP_PREFIX	LIB_WORLD"shp"SLASH	/* shop definitions	*/
#define TRG_PREFIX	LIB_WORLD"trg"SLASH	/* shop definitions	*/
#define HLP_PREFIX	LIB_TEXT"help"SLASH	/* for HELP <keyword>	*/
#define SOC_PREFIX	LIB_MISC
#define PLAYER_F_PREFIX LIB_PLRS""LIB_F
#define PLAYER_K_PREFIX LIB_PLRS""LIB_K
#define PLAYER_P_PREFIX LIB_PLRS""LIB_P
#define PLAYER_U_PREFIX LIB_PLRS""LIB_U
#define PLAYER_Z_PREFIX LIB_PLRS""LIB_Z

#define CREDITS_FILE	LIB_TEXT"credits"/* for the 'credits' command	*/
#define NEWS_FILE	    LIB_TEXT"news"	/* for the 'news' command	*/
#define MOTD_FILE	    LIB_TEXT"motd"	/* messages of the day / mortal	*/
#define IMOTD_FILE	    LIB_TEXT"imotd"	/* messages of the day / immort	*/
#define GREETINGS_FILE	LIB_TEXT"greetings"	/* The opening screen.	*/
#define HELP_PAGE_FILE	LIB_TEXT_HELP"screen" /* for HELP <CR>		*/
#define INFO_FILE	    LIB_TEXT"info"		/* for INFO		*/
#define WIZLIST_FILE	LIB_TEXT"wizlist"	/* for WIZLIST		*/
#define IMMLIST_FILE	LIB_TEXT"immlist"	/* for IMMLIST		*/
#define BACKGROUND_FILE	LIB_TEXT"background"/* for the background story	*/
#define POLICIES_FILE	LIB_TEXT"policies" /* player policies/rules	*/
#define HANDBOOK_FILE	LIB_TEXT"handbook" /* handbook for new immorts	*/

#define IDEA_FILE	    LIB_MISC"ideas"	/* for the 'idea'-command	*/
#define TYPO_FILE	    LIB_MISC"typos"	/*         'typo'		*/
#define BUG_FILE	    LIB_MISC"bugs"	/*         'bug'		*/
#define MESS_FILE      	LIB_MISC"messages" /* damage messages		*/
#define SOCMESS_FILE	LIB_MISC"socials" /* messgs for social acts	*/
#define XNAME_FILE	    LIB_MISC"xnames" /* invalid name substrings	*/

#define PLAYER_FILE	    LIB_ETC"players" /* the player database		*/
#define PKILLER_FILE	LIB_ETC"pkillers" /* the player database		*/
#define MAIL_FILE	    LIB_ETC"plrmail" /* for the mudmail system	*/
#define BAN_FILE	    LIB_ETC"badsites" /* for the siteban system	*/
#define HCONTROL_FILE	LIB_ETC"hcontrol"  /* for the house system	*/

/* public procedures in db.c */
void	boot_db(void);
int	    create_entry(char *name);
void	zone_update(void);
room_rnum real_room(room_vnum vnum);
char	*fread_string(FILE *fl, char *error);
long	get_id_by_name(char *name);
long	cmp_ptable_by_name(char *name, int len);
char	*get_name_by_id(long id);

void	char_to_store(struct char_data *ch, struct char_file_u *st);
void	store_to_char(struct char_file_u *st, struct char_data *ch);
int	    load_char(char *name, struct char_file_u *char_element);
void	save_char(struct char_data *ch, room_rnum load_room);
void	init_char(struct char_data *ch);
struct  char_data* create_char(void);
struct  char_data *read_mobile(mob_vnum nr, int type);
mob_rnum real_mobile(mob_vnum vnum);
int	     vnum_mobile(char *searchname, struct char_data *ch);
void	 clear_char(struct char_data *ch);
void	 reset_char(struct char_data *ch);
void	 free_char(struct char_data *ch);

struct  obj_data *create_obj(void);
void	clear_object(struct obj_data *obj);
void	free_obj(struct obj_data *obj);
obj_rnum real_object(obj_vnum vnum);
struct  obj_data *read_object(obj_vnum nr, int type);
int	    vnum_object(char *searchname, struct char_data *ch);

#define REAL          0
#define VIRTUAL       (1 << 0)
#define OBJ_NO_CALC   (1 << 1)

/* structure for the reset commands */
struct reset_com {
   char	command;   /* current command                      */

   bool if_flag;	/* if TRUE: exe only if preceding exe'd */
   int	arg1;		/*                                      */
   int	arg2;		/* Arguments to the command             */
   int	arg3;		/*                                      */
   int  arg4;
   int  line;		/* line number this command appears on  */
   char *sarg1;		/* string argument                      */
   char *sarg2;		/* string argument                      */

   /* 
	*  Commands:              *
	*  'M': Read a mobile     *
	*  'O': Read an object    *
	*  'G': Give obj to mob   *
	*  'P': Put obj in obj    *
	*  'G': Obj to char       *
	*  'E': Obj to char equip *
	*  'D': Set state of door *
	*  'T': Trigger command   *	
   */
};



/* zone definition structure. for the 'zone-table'   */
struct zone_data {
   char	*name;		    /* name of this zone                  */
   int	lifespan;           /* how long between resets (minutes)  */
   int	age;                /* current age of this zone (minutes) */
   room_vnum top;           /* upper limit for rooms in this zone */

   int	reset_mode;         /* conditions for reset (see below)   */
   zone_vnum number;	    /* virtual number of this zone	  */
   struct reset_com *cmd;   /* command table for reset	          */

   /*
    * Reset mode:
    *   0: Don't reset, and don't update age.
    *   1: Reset if no PC's are located in zone.
    *   2: Just reset.
    */
};



/* for queueing zones for update   */
struct reset_q_element {
   zone_rnum zone_to_reset;            /* ref to zone_data */
   struct reset_q_element *next;
};



/* structure for the update queue     */
struct reset_q_type 
{  struct reset_q_element *head;
   struct reset_q_element *tail;
};

#define OBJECT_SAVE_ACTIVITY 300
#define PLAYER_SAVE_ACTIVITY 300
#define MAX_SAVED_ITEMS      500

struct player_index_element 
{  char   *name;
   long    id;
   long    unique;
   int     level;
   int     last_logon;
   int     activity;         // When player be saved and checked
   struct  save_info *timer;
};


struct help_index_element {
   char	*keyword;
   char *entry;
   int   duplicate;
};

#define SEASON_WINTER		0
#define SEASON_SPRING		1
#define SEASON_SUMMER		2
#define SEASON_AUTUMN		3

#define MONTH_JANUARY   	0
#define MONTH_FEBRUARY  	1
#define MONTH_MART			2
#define MONTH_APRIL			3
#define MONTH_MAY			4
#define MONTH_JUNE			5
#define MONTH_JULY			6
#define MONTH_AUGUST		7
#define MONTH_SEPTEMBER		8
#define MONTH_OCTOBER		9
#define MONTH_NOVEMBER		10
#define MONTH_DECEMBER		11
#define DAYS_PER_WEEK		7

struct month_temperature_type { 
  int min;
  int max;
  int med;
};

/* don't change these */
#define BAN_NOT 	0
#define BAN_NEW 	1
#define BAN_SELECT	2
#define BAN_ALL		3

#define BANNED_SITE_LENGTH    50
#define BANNED_METTER_LENGTH  80
struct ban_list_element {
   char	site[BANNED_SITE_LENGTH+1];
   int	type;
   time_t date;
   char	name[MAX_NAME_LENGTH+1];
   char metter[BANNED_METTER_LENGTH];
   struct ban_list_element *next;
};


/* global buffering system */

#ifdef __DB_C__
char	buf[MAX_STRING_LENGTH];
char	buf1[MAX_STRING_LENGTH];
char	buf2[MAX_STRING_LENGTH];
char	arg[MAX_STRING_LENGTH];
#else
extern room_rnum top_of_world;
extern struct player_special_data dummy_mob;
extern char	buf[MAX_STRING_LENGTH];
extern char	buf1[MAX_STRING_LENGTH];
extern char	buf2[MAX_STRING_LENGTH];
extern char	arg[MAX_STRING_LENGTH];
#endif

#ifndef __CONFIG_C__
extern char	*OK;
extern char	*NOPERSON;
extern char	*NOEFFECT;
#endif
