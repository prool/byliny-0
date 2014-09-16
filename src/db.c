/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __DB_C__

#include "prool.h" // prool

#include "conf.h"
#include "sysdep.h"
#include "dirent.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "sys/stat.h"

/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/
#if defined(CIRCLE_MACINTOSH)
  long beginning_of_time = -1561789232;
#else
  long beginning_of_time = 650336715;
#endif



struct room_data *world = NULL; /* array of rooms                */
room_rnum top_of_world = 0;     /* ref to top element of world   */

struct char_data *character_list = NULL;        /* global linked list of
                                                                 * chars         */
struct index_data **trig_index; /* index table for triggers      */
int top_of_trigt = 0;           /* top of trigger index table    */
long max_id = MOBOBJ_ID_BASE;   /* for unique mob/obj id's       */
                                                                
struct index_data *mob_index;   /* index table for mobile file   */
struct char_data *mob_proto;    /* prototypes for mobs           */
mob_rnum top_of_mobt = 0;       /* top of mobile index table     */

struct obj_data *object_list = NULL;    /* global linked list of objs    */
struct index_data *obj_index;   /* index table for object file   */
struct obj_data *obj_proto;     /* prototypes for objs           */
obj_rnum top_of_objt = 0;       /* top of object index table     */

struct zone_data *zone_table;   /* zone table                    */
zone_rnum top_of_zone_table = 0;/* top element of zone tab       */
struct message_list fight_messages[MAX_MESSAGES];       /* fighting messages     */

struct player_index_element *player_table = NULL;       /* index to plr file     */
FILE *player_fl = NULL;         /* file desc of player file      */
FILE *pkiller_fl = NULL;
int top_of_p_table = 0;         /* ref to top of table           */
int top_of_p_file = 0;          /* ref of size of p file         */
long top_idnum = 0;             /* highest idnum in use          */

int no_mail = 0;                /* mail disabled?                */
int mini_mud = 0;               /* mini-mud mode?                */
int no_rent_check = 0;          /* skip rent check on boot?      */
time_t boot_time = 0;           /* time of mud boot              */
int circle_restrict = 0;        /* level of game restriction     */
room_rnum r_mortal_start_room;  /* rnum of mortal start room     */
room_rnum r_immort_start_room;  /* rnum of immort start room     */
room_rnum r_frozen_start_room;  /* rnum of frozen start room     */
room_rnum r_helled_start_room;
room_rnum r_named_start_room;
room_rnum r_unreg_start_room;

char *credits = NULL;           /* game credits                  */
char *news = NULL;              /* mud news                      */
long lastnews = 0;
char *motd = NULL;              /* message of the day - mortals */
char *imotd = NULL;             /* message of the day - immorts */
char *GREETINGS = NULL;         /* opening credits screen       */
char *help = NULL;              /* help screen                   */
char *info = NULL;              /* info page                     */
char *wizlist = NULL;           /* list of higher gods           */
char *immlist = NULL;           /* list of peon gods             */
char *background = NULL;        /* background story              */
char *handbook = NULL;          /* handbook for new immortals    */
char *policies = NULL;          /* policies page                 */

struct help_index_element *help_table = 0;      /* the help table        */
int top_of_helpt = 0;           /* top of help index table       */

struct time_info_data time_info;/* the infomation about the time    */
struct weather_data weather_info;       /* the infomation about the weather */
struct player_special_data dummy_mob;   /* dummy spec area for mobs     */
struct reset_q_type reset_q;    /* queue of zones to be reset    */
int supress_godsapply = FALSE;
extern const struct new_flag clear_flags = { {0,0,0,0} }; // prool
// Добавлено Дажьбогом
struct time_info_data *real_time_passed(time_t t2, time_t t1);
/* local functions */
int check_object_spell_number(struct obj_data *obj, int val);
int check_object_level(struct obj_data *obj, int val);
void setup_dir(FILE * fl, int room, int dir);
void index_boot(int mode);
void discrete_load(FILE * fl, int mode, char *filename);
int check_object(struct obj_data *);
void parse_trigger(FILE *fl, int Virtual_nr);
void parse_room(FILE * fl, int Virtual_nr, int Virtual);
void parse_mobile(FILE * mob_f, int nr);
char *parse_object(FILE * obj_f, int nr);
void load_zones(FILE * fl, char *zonename);
void load_help(FILE *fl);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void init_spec_procs(void);
void build_player_index(void);
int is_empty(zone_rnum zone_nr);
void reset_zone(zone_rnum zone);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
void reboot_wizlists(void);
ACMD(do_reboot);
void boot_world(void);
int count_alias_records(FILE *fl);
int count_hash_records(FILE * fl);
int count_social_records(FILE *fl, int *messages, int *keywords);
void asciiflag_conv(char *flag, void *value);
void parse_simple_mob(FILE *mob_f, int i, int nr);
void interpret_espec(const char *keyword, const char *value, int i, int nr);
void parse_espec(char *buf, int i, int nr);
void parse_enhanced_mob(FILE *mob_f, int i, int nr);
void get_one_line(FILE *fl, char *buf);
void save_etext(struct char_data * ch);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(zone_rnum zone, int cmd_no, const char *message);
void reset_time(void);
long get_ptable_by_name(char *name);
int  mobs_in_room(int m_num, int r_num);
void new_build_player_index(void);

int  new_load_char(char *name, struct char_file_u * char_element);
void new_load_pkills(struct char_data * ch);
void new_load_quests(struct char_data * ch);
void new_load_mkill(struct char_data * ch);

void new_save_char(struct char_data * ch, room_rnum load_room);
void new_save_mkill(struct char_data * ch);
void new_save_quests(struct char_data * ch);
void new_save_pkills(struct char_data * ch);

void init_guilds(void);
void init_basic_values(void);

/* external functions */
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void free_alias(struct alias_data *a);
void load_messages(void);
void weather_and_time(int mode);
void mag_assign_spells(void);
void boot_social_messages(void);
void update_obj_file(void);     /* In objsave.c */
void sort_commands(void);
void sort_spells(void);
void load_banned(void);
void Read_Invalid_List(void);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count);
int  find_name(char *name);
int  hsort(const void *a, const void *b);
int  csort(const void *a, const void *b);
void prune_crlf(char *txt);
void save_char_vars(struct char_data *ch);
void Crash_calc_objects(int index, int calcstore);
void Crash_timer_obj(int index, long timer_dec);
void extract_mob(struct char_data *ch);
void add_follower(struct char_data *ch, struct char_data *leader);
void load_socials(FILE *fl);
void create_rainsnow(int *wtype, int startvalue, int chance1, int chance2, int chance3);
void name_from_drinkcon(struct obj_data * obj);
void name_to_drinkcon(struct obj_data * obj, int type);
void calc_easter(void);
void calc_god_celebrate(void/*struct char_data *ch*/); // prool
void do_start(struct char_data *ch, int newbie);
int  calc_loadroom(struct char_data *ch);
void die_follower(struct char_data *ch);

/* external vars */
extern int no_specials;
extern int scheck;
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern room_vnum helled_start_room;
extern room_vnum named_start_room;
extern room_vnum unreg_start_room;
extern struct descriptor_data *descriptor_list;
extern const char *unused_spellname;
extern int top_of_socialm;
extern int top_of_socialk;
extern struct month_temperature_type year_temp[];
extern const int sunrise[][2];

void   roll_real_abils(struct char_data * ch);
#define READ_SIZE 256

/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
}


/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reboot)
{ struct stat sb;
  int    i;

  one_argument(argument, arg);

  if (!str_cmp(arg, "all") || *arg == '*')
     {if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
         prune_crlf(GREETINGS);
      file_to_string_alloc(WIZLIST_FILE, &wizlist);
      file_to_string_alloc(IMMLIST_FILE, &immlist);
      file_to_string_alloc(NEWS_FILE, &news);
      if (stat(LIB""NEWS_FILE,&sb) >= 0)
         lastnews = sb.st_mtime;
      file_to_string_alloc(CREDITS_FILE, &credits);
      file_to_string_alloc(MOTD_FILE, &motd);
      file_to_string_alloc(IMOTD_FILE, &imotd);
      file_to_string_alloc(HELP_PAGE_FILE, &help);
      file_to_string_alloc(INFO_FILE, &info);
      file_to_string_alloc(POLICIES_FILE, &policies);
      file_to_string_alloc(HANDBOOK_FILE, &handbook);
      file_to_string_alloc(BACKGROUND_FILE, &background);
     }
  else
  if (!str_cmp(arg, "wizlist"))
     file_to_string_alloc(WIZLIST_FILE, &wizlist);
  else
  if (!str_cmp(arg, "immlist"))
     file_to_string_alloc(IMMLIST_FILE, &immlist);
  else
  if (!str_cmp(arg, "news"))
     {file_to_string_alloc(NEWS_FILE, &news);
      if (stat(NEWS_FILE,&sb) >= 0)
         lastnews = sb.st_mtime;
     }
  else
  if (!str_cmp(arg, "credits"))
     file_to_string_alloc(CREDITS_FILE, &credits);
  else
  if (!str_cmp(arg, "motd"))
     file_to_string_alloc(MOTD_FILE, &motd);
  else
  if (!str_cmp(arg, "imotd"))
     file_to_string_alloc(IMOTD_FILE, &imotd);
  else
  if (!str_cmp(arg, "help"))
     file_to_string_alloc(HELP_PAGE_FILE, &help);
  else
  if (!str_cmp(arg, "info"))
     file_to_string_alloc(INFO_FILE, &info);
  else
  if (!str_cmp(arg, "policy"))
     file_to_string_alloc(POLICIES_FILE, &policies);
  else
  if (!str_cmp(arg, "handbook"))
     file_to_string_alloc(HANDBOOK_FILE, &handbook);
  else
  if (!str_cmp(arg, "background"))
     file_to_string_alloc(BACKGROUND_FILE, &background);
  else
  if (!str_cmp(arg, "greetings"))
     {if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
         prune_crlf(GREETINGS);
     }
  else
  if (!str_cmp(arg, "xhelp"))
     {if (help_table)
         {for (i = 0; i <= top_of_helpt; i++)
              {if (help_table[i].keyword)
                      free(help_table[i].keyword);
               if (help_table[i].entry && !help_table[i].duplicate)
                      free(help_table[i].entry);
              }
          free(help_table);
         }
      top_of_helpt = 0;
      index_boot(DB_BOOT_HLP);
     }
  else
  if (!str_cmp(arg, "socials"))
     {if (soc_mess_list)
         {for (i = 0; i <= top_of_socialm; i++)
              {if (soc_mess_list[i].char_no_arg)
                  free(soc_mess_list[i].char_no_arg);
               if (soc_mess_list[i].others_no_arg)
                  free(soc_mess_list[i].others_no_arg);
               if (soc_mess_list[i].char_found)
                  free(soc_mess_list[i].char_found);
               if (soc_mess_list[i].others_found)
                  free(soc_mess_list[i].others_found);
               if (soc_mess_list[i].vict_found)
                  free(soc_mess_list[i].vict_found);
               if (soc_mess_list[i].not_found)
                  free(soc_mess_list[i].not_found);
               }
          free (soc_mess_list);
         }
      if (soc_keys_list)
         {for (i = 0; i <= top_of_socialk; i++)
              if (soc_keys_list[i].keyword)
                  free(soc_keys_list[i].keyword);
          free (soc_keys_list);
         }
      top_of_socialm = -1;
      top_of_socialk = -1;
      index_boot(DB_BOOT_SOCIAL);
     }
  else
     {send_to_char("Unknown reload option.\r\n", ch);
      return;
     }

  send_to_char(OK, ch);
}


void boot_world(void)
{
  log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  log("Loading triggers and generating index.");
  index_boot(DB_BOOT_TRG);

  log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  log("Renumbering rooms.");
  renum_world();

  log("Checking start rooms.");
  check_start_rooms();

  log("Loading mobs and generating index.");
  index_boot(DB_BOOT_MOB);

  log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);

  log("Renumbering zone table.");
  renum_zone_table();

  if (!no_specials)
     {log("Loading shops.");
      index_boot(DB_BOOT_SHP);
     }
}



/* body of the booting system */
void boot_db(void)
{ struct stat sb;
  zone_rnum i;

  log("Boot db -- BEGIN.");

  log("Resetting the game time:");
  reset_time();

  log("Reading news, credits, help, bground, info & motds.");
  file_to_string_alloc(NEWS_FILE, &news);
  if (stat(NEWS_FILE,&sb) >= 0)
     lastnews = sb.st_mtime;
  file_to_string_alloc(CREDITS_FILE, &credits);
  file_to_string_alloc(MOTD_FILE, &motd);
  file_to_string_alloc(IMOTD_FILE, &imotd);
  file_to_string_alloc(HELP_PAGE_FILE, &help);
  file_to_string_alloc(INFO_FILE, &info);
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
  file_to_string_alloc(POLICIES_FILE, &policies);
  file_to_string_alloc(HANDBOOK_FILE, &handbook);
  file_to_string_alloc(BACKGROUND_FILE, &background);
  if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
     prune_crlf(GREETINGS);

  log("Loading spell definitions.");
  mag_assign_spells();

  boot_world();

  log("Loading help entries.");
  index_boot(DB_BOOT_HLP);

  log("Loading social entries.");
  index_boot(DB_BOOT_SOCIAL);

  log("Generating player index.");
  build_player_index();

  log("Loading fight messages.");
  load_messages();

  log("Assigning function pointers:");

  if (!no_specials)
     {log("   Mobiles.");
         assign_mobiles();
      log("   Shopkeepers.");
         assign_the_shopkeepers();
      log("   Objects.");
         assign_objects();
      log("   Rooms.");
         assign_rooms();
     }

  log("Assigning spell and skill levels.");
  init_spell_levels();

  log("Sorting command list and spells.");
  sort_commands();
  sort_spells();

  log("Booting mail system.");
  if (!scan_file())
     {log("    Mail boot failed -- Mail system disabled");
      no_mail = 1;
     }
  log("Reading banned site and invalid-name list.");
  load_banned();
  Read_Invalid_List();

  if (!no_rent_check)
     {log("Deleting timed-out crash and rent files:");
      update_obj_file();
      log("   Done.");
     }

  /* Moved here so the object limit code works. -gg 6/24/98 */
  if (!mini_mud)
     {log("Booting houses.");
      House_boot();
     }

  for (i = 0; i <= top_of_zone_table; i++)
      {log("Resetting %s (rooms %d-%d).", zone_table[i].name,
               (i ? (zone_table[i - 1].top + 1) : 0), zone_table[i].top);
       reset_zone(i);
      }

  reset_q.head = reset_q.tail = NULL;

  boot_time = time(0);

  log("Booting objects stored");
  for (i = 0; i <= top_of_p_table; i++)
      {Crash_calc_objects(i,TRUE);
       log ("Player %s", (player_table + i)->name);
      }

  log("Booting basic values");
  init_basic_values();

  log("Booting specials assigment");
  init_spec_procs();

  log("Booting guilds");
  init_guilds();

  log("Boot db -- DONE.");

//  printf("prool: test\n"); // prool

}


/* reset the time in the game from file */
void reset_time(void)
{ time_info = *mud_time_passed(time(0), beginning_of_time);
  // Calculate moon day
  weather_info.moon_day      = ((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % MOON_CYCLE;
  weather_info.week_day_mono = ((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % WEEK_CYCLE;
  weather_info.week_day_poly = ((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % POLY_WEEK_CYCLE;
  // Calculate Easter
  calc_easter();
  calc_god_celebrate(/*NULL*/);

  if (time_info.hours <  sunrise[time_info.month][0])
     weather_info.sunlight = SUN_DARK;
  else
  if (time_info.hours == sunrise[time_info.month][0])
     weather_info.sunlight = SUN_RISE;
  else
  if (time_info.hours <  sunrise[time_info.month][1])
     weather_info.sunlight = SUN_LIGHT;
  else
  if (time_info.hours == sunrise[time_info.month][1])
     weather_info.sunlight = SUN_SET;
  else
     weather_info.sunlight = SUN_DARK;

  log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
          time_info.day, time_info.month, time_info.year);
        
  weather_info.temperature = (year_temp[time_info.month].med * 4 +
                              number(year_temp[time_info.month].min,year_temp[time_info.month].max)) / 5;
  weather_info.pressure = 960;
  if ((time_info.month >= MONTH_MAY) && (time_info.month <= MONTH_NOVEMBER))
     weather_info.pressure += dice(1, 50);
  else
     weather_info.pressure += dice(1, 80);

  weather_info.change       = 0;
  weather_info.weather_type = 0;

  if (time_info.month >= MONTH_APRIL && time_info.month <= MONTH_MAY)
     weather_info.season = SEASON_SPRING;
  else
  if (time_info.month == MONTH_MART && weather_info.temperature >= 3)
     weather_info.season = SEASON_SPRING;
  else
  if (time_info.month >= MONTH_JUNE  && time_info.month <= MONTH_AUGUST)
     weather_info.season = SEASON_SUMMER;
  else
  if (time_info.month >= MONTH_SEPTEMBER && time_info.month <= MONTH_OCTOBER)
     weather_info.season = SEASON_AUTUMN;
  else
  if (time_info.month == MONTH_NOVEMBER && weather_info.temperature >= 3)
     weather_info.season = SEASON_AUTUMN;
  else
     weather_info.season = SEASON_WINTER;

  if (weather_info.pressure <= 980)
     weather_info.sky = SKY_LIGHTNING;
  else
  if (weather_info.pressure <= 1000)
     {weather_info.sky = SKY_RAINING;
      if (time_info.month >= MONTH_APRIL && time_info.month <= MONTH_OCTOBER)
         create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTRAIN, 40, 40, 20);
      else
      if (time_info.month >= MONTH_DECEMBER || time_info.month <= MONTH_FEBRUARY)
         create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTSNOW, 50, 40, 10);
      else
      if (time_info.month == MONTH_NOVEMBER || time_info.month == MONTH_MART)
         {if (weather_info.temperature >= 3)
             create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTRAIN, 70, 30, 0);
          else
             create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTSNOW, 80, 20, 0);
         }
     }
  else
  if (weather_info.pressure <= 1020)
     weather_info.sky = SKY_CLOUDY;
  else
     weather_info.sky = SKY_CLOUDLESS;
}



/* generate index table for the player file */
void build_player_index(void)
{
  int nr = -1, i;
  long size, recs;
  struct char_file_u dummy;

  if (USE_SINGLE_PLAYER)
     {new_build_player_index();
      return;
     }

  if (!(pkiller_fl = fopen(PKILLER_FILE, "r+b")))
     {if (errno != ENOENT)
         {perror("SYSERR: fatal error opening pkillerfile");
          exit(1);
         }
      else
         {log("No pkillerfile.  Creating a new one.");
          touch(PKILLER_FILE);
          if (!(pkiller_fl = fopen(PKILLER_FILE, "r+b")))
             {perror("SYSERR: fatal error opening pkillerfile");
                  exit(1);
             }
         }
     }

  fseek(pkiller_fl, 0L, SEEK_END);
  size = ftell(pkiller_fl);
  rewind(pkiller_fl);


  if (!(player_fl = fopen(PLAYER_FILE, "r+b")))
     {if (errno != ENOENT)
         {perror("SYSERR: fatal error opening playerfile");
          exit(1);
         }
      else
         {log("No playerfile.  Creating a new one.");
          touch(PLAYER_FILE);
          if (!(player_fl = fopen(PLAYER_FILE, "r+b")))
             {perror("SYSERR: fatal error opening playerfile");
                  exit(1);
             }
         }
     }

  fseek(player_fl, 0L, SEEK_END);
  size = ftell(player_fl);
  rewind(player_fl);
  if (size % sizeof(struct char_file_u))
     log("\aWARNING:  PLAYERFILE IS PROBABLY CORRUPT!");
  recs = size / sizeof(struct char_file_u);
  if (recs)
     {log("   %ld players in database.", recs);
      CREATE(player_table, struct player_index_element, recs);
     }
  else
     {player_table  = NULL;
      top_of_p_file = top_of_p_table = -1;
      return;
     }

  for (; !feof(player_fl);)
      {fread(&dummy, sizeof(struct char_file_u), 1, player_fl);
       if (!feof(player_fl))
          {/* new record */
           nr++;
           log("OLD PLAYER INDEX <%s=%d >", dummy.name, dummy.level);
           CREATE(player_table[nr].name, char, strlen(dummy.name) + 1);
           for (i = 0, player_table[nr].name[i] = '\0';
                (player_table[nr].name[i] = LOWER(dummy.name[i])); i++);
           player_table[nr].id     = dummy.char_specials_saved.idnum;
           player_table[nr].unique = dummy.player_specials_saved.unique;
           player_table[nr].level  = (dummy.player_specials_saved.Remorts ?
                                      30 : dummy.level);
           player_table[nr].timer  = NULL;
           if (IS_SET(GET_FLAG(dummy.char_specials_saved.act, PLR_DELETED), PLR_DELETED))
             {player_table[nr].last_logon = -1;
              player_table[nr].level      = -1;
              player_table[nr].activity   = -1;

             }
          else
             {player_table[nr].last_logon = dummy.last_logon;
              player_table[nr].activity   = number(0, OBJECT_SAVE_ACTIVITY - 1);
             }
           top_idnum = MAX(top_idnum, dummy.char_specials_saved.idnum);
          }
      }

  top_of_p_file = top_of_p_table = nr;
}

/*
 * Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I
 * did add the 'goto' and changed some "while()" into "do { } while()".
 *      -gg 6/24/98 (technically 6/25/98, but I care not.)
 */
int count_alias_records(FILE *fl)
{
  char key[READ_SIZE], next_key[READ_SIZE];
  char line[READ_SIZE], *scan;
  int total_keywords = 0;

  /* get the first keyword line */
  get_one_line(fl, key);

  while (*key != '$')
        {/* skip the text */
         do {get_one_line(fl, line);
             if (feof(fl))
                    goto ackeof;
            } while (*line != '#');

         /* now count keywords */
         scan = key;
         do {scan = one_word(scan, next_key);
             if (*next_key)
                ++total_keywords;
            } while (*next_key);

         /* get next keyword line (or $) */
         get_one_line(fl, key);

         if (feof(fl))
            goto ackeof;
        }

  return (total_keywords);

  /* No, they are not evil. -gg 6/24/98 */
ackeof: 
  log("SYSERR: Unexpected end of help file.");
  exit(1);      /* Some day we hope to handle these things better... */
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
        if (*buf == '#')
           count++;

  return (count);
}

int count_social_records(FILE *fl, int *messages, int *keywords)
{
  char key[READ_SIZE], next_key[READ_SIZE];
  char line[READ_SIZE], *scan;

  /* get the first keyword line */
  get_one_line(fl, key);

  while (*key != '$')
        {/* skip the text */
         do {get_one_line(fl, line);
             if (feof(fl))
                    goto ackeof;
            } while (*line != '#');

         /* now count keywords */
         scan = key;
         ++(*messages);
         do {scan = one_word(scan, next_key);
             if (*next_key)
                ++(*keywords);
            } while (*next_key);

         /* get next keyword line (or $) */
         get_one_line(fl, key);

         if (feof(fl))
            goto ackeof;
        }

  return (TRUE);

  /* No, they are not evil. -gg 6/24/98 */
ackeof: 
  log("SYSERR: Unexpected end of help file.");
  exit(1);      /* Some day we hope to handle these things better... */
}




void index_boot(int mode)
{
  const char *index_filename, *prefix = NULL;   /* NULL or egcs 1.1 complains */
  FILE *index, *db_file;
  int rec_count = 0, size[2], count;

  switch (mode)
  {
  case DB_BOOT_TRG:
    prefix = TRG_PREFIX;
    break;
  case DB_BOOT_WLD:
    prefix = WLD_PREFIX;
    break;
  case DB_BOOT_MOB:
    prefix = MOB_PREFIX;
    break;
  case DB_BOOT_OBJ:
    prefix = OBJ_PREFIX;
    break;
  case DB_BOOT_ZON:
    prefix = ZON_PREFIX;
    break;
  case DB_BOOT_SHP:
    prefix = SHP_PREFIX;
    break;
  case DB_BOOT_HLP:
    prefix = HLP_PREFIX;
    break;
  case DB_BOOT_SOCIAL:
    prefix = SOC_PREFIX;
    break;
  default:
    log("SYSERR: Unknown subcommand %d to index_boot!", mode);
    exit(1);
  }

  if (mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  sprintf(buf2, "%s%s", prefix, index_filename);

  if (!(index = fopen(buf2, "r")))
     { log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
       exit(1);
     }

  /* first, count the number of records in the file so we can malloc */
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$')
   {sprintf(buf2, "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r")))
       {log("SYSERR: File '%s' listed in '%s/%s': %s", buf2, prefix,
                 index_filename, strerror(errno));
        fscanf(index, "%s\n", buf1);
        continue;
       }
    else
       {if (mode == DB_BOOT_ZON)
               rec_count++;
        else
        if (mode == DB_BOOT_SOCIAL)
           rec_count += count_social_records(db_file, &top_of_socialm, &top_of_socialk);
        else
        if (mode == DB_BOOT_HLP)
               rec_count += count_alias_records(db_file);
        else
        if (mode == DB_BOOT_WLD)
           {count = count_hash_records(db_file);
            if (count >= 99)
               {log("SYSERR: File '%s' list more than 98 room",buf2);
                exit(1);
               }
            rec_count += (count+1);
           }
        else
           rec_count += count_hash_records(db_file);
       }
    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }

  /* Exit if 0 records, unless this is shops */
  if (!rec_count)
  {if (mode == DB_BOOT_SHP)
      return;
    log("SYSERR: boot error - 0 records counted in %s/%s.", prefix,
        index_filename);
    exit(1);
  }

  /* Any idea why you put this here Jeremy? */
  rec_count++;

  /*
   * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
   */
  switch (mode)
  {
  case DB_BOOT_TRG:
    CREATE(trig_index, struct index_data *, rec_count);
    break;
  case DB_BOOT_WLD:
    CREATE(world, struct room_data, rec_count);
    size[0] = sizeof(struct room_data) * rec_count;
    log("   %d rooms, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_MOB:
    CREATE(mob_proto, struct char_data, rec_count);
    CREATE(mob_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct char_data) * rec_count;
    log("   %d mobs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    break;
  case DB_BOOT_OBJ:
    CREATE(obj_proto, struct obj_data, rec_count);
    CREATE(obj_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct obj_data) * rec_count;
    log("   %d objs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    break;
  case DB_BOOT_ZON:
    CREATE(zone_table, struct zone_data, rec_count);
    size[0] = sizeof(struct zone_data) * rec_count;
    log("   %d zones, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_HLP:
    CREATE(help_table, struct help_index_element, rec_count);
    size[0] = sizeof(struct help_index_element) * rec_count;
    log("   %d entries, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_SOCIAL:
    CREATE(soc_mess_list, struct social_messg, top_of_socialm+1);
    CREATE(soc_keys_list, struct social_keyword, top_of_socialk+1);
    size[0] = sizeof(struct social_messg) * (top_of_socialm+1);
    size[1] = sizeof(struct social_keyword) * (top_of_socialk+1);
    log("   %d entries(%d keywords), %d(%d) bytes.", top_of_socialm+1,
        top_of_socialk+1, size[0], size[1]);
  }

  rewind(index);
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$')
    {sprintf(buf2, "%s%s", prefix, buf1);
     if (!(db_file = fopen(buf2, "r")))
        {log("SYSERR: %s: %s", buf2, strerror(errno));
         exit(1);
        }
    switch (mode)
    {
    case DB_BOOT_TRG:
    case DB_BOOT_WLD:
    case DB_BOOT_OBJ:
    case DB_BOOT_MOB:
      discrete_load(db_file, mode, buf2);
      break;
    case DB_BOOT_ZON:
      load_zones(db_file, buf2);
      break;
    case DB_BOOT_HLP:
      /*
       * If you think about it, we have a race here.  Although, this is the
       * "point-the-gun-at-your-own-foot" type of race.
       */
      load_help(db_file);
      break;
    case DB_BOOT_SHP:
      boot_the_shops(db_file, buf2, rec_count);
      break;
    case DB_BOOT_SOCIAL:
      load_socials(db_file);
      break;
    }
    if (mode == DB_BOOT_WLD)
       parse_room(db_file,0,TRUE);
    fclose(db_file);
    fscanf(index, "%s\n", buf1);
   }
  fclose(index);
  // Create Virtual room for zone

  // sort the help index
  if (mode == DB_BOOT_HLP)
     {qsort(help_table, top_of_helpt, sizeof(struct help_index_element), hsort);
      top_of_helpt--;
     }
  // sort the social index
  if (mode == DB_BOOT_SOCIAL)
     {qsort(soc_keys_list, top_of_socialk+1, sizeof(struct social_keyword), csort);
     }
}


void discrete_load(FILE * fl, int mode, char *filename)
{
  int nr = -1, last;
  char line[256];

  const char *modes[] = {"world", "mob", "obj", "ZON", "SHP", "HLP", "trg"};
  /* modes positions correspond to DB_BOOT_xxx in db.h */


  for (;;)
    {/*
      * we have to do special processing with the obj files because they have
      * no end-of-record marker :(
      */
     if (mode != DB_BOOT_OBJ || nr < 0)
        if (!get_line(fl, line))
           {if (nr == -1)
                   {log("SYSERR: %s file %s is empty!", modes[mode], filename);
                   }
                else {log("SYSERR: Format error in %s after %s #%d\n"
                          "...expecting a new %s, but file ended!\n"
                          "(maybe the file is not terminated with '$'?)", filename,
                          modes[mode], nr, modes[mode]);
                     }
                exit(1);
           }

     if (*line == '$')
        return;
    /* This file create ADAMANT MUD ETITOR ? */
     if (strcmp(line,"#ADAMANT") == 0)
        continue;

     if (*line == '#')
        {last = nr;
         if (sscanf(line, "#%d", &nr) != 1)
            {log("SYSERR: Format error after %s #%d", modes[mode], last);
                 exit(1);
            }
         if (nr >= 99999)
                return;
         else
                switch (mode)
                {
    case DB_BOOT_TRG:
      parse_trigger(fl, nr);
      break;
        case DB_BOOT_WLD:
          parse_room(fl, nr, FALSE);
          break;
        case DB_BOOT_MOB:
          parse_mobile(fl, nr);
          break;
        case DB_BOOT_OBJ:
          strcpy(line, parse_object(fl, nr));
          break;
                }
        }
     else
        {log("SYSERR: Format error in %s file %s near %s #%d", modes[mode],
                 filename, modes[mode], nr);
         log("SYSERR: ... offending line: '%s'", line);
         exit(1);
        }
    }
}

void asciiflag_conv(char *flag, void *value)
{
  int       *flags  = (int *) value;
  int is_number     = 1, block = 0, i;
  register char *p;

  for (p = flag; *p; p += i+1)
      {i = 0;
       if (islower(*p))
          {if (*(p+1) >= '0' && *(p+1) <= '9')
              {block = (int) *(p+1) - '0';
               i     = 1;
              }
           else
              block = 0;
           *(flags+block) |= (0x3FFFFFFF & (1 << (*p - 'a')));
          }
       else
       if (isupper(*p))
          {if (*(p+1) >= '0' && *(p+1) <= '9')
              {block = (int) *(p+1) - '0';
               i     = 1;
              }
           else
              block  = 0;
           *(flags+block) |=  (0x3FFFFFFF & (1 << (26 + (*p - 'A'))));
          }
       if (!isdigit(*p))
          is_number = 0;
      }

  if (is_number)
     {is_number = atol(flag);
      block     = is_number < INT_ONE ? 0 : is_number < INT_TWO ? 1 : is_number < INT_THREE ? 2 : 3;
      *(flags+block) = is_number & 0x3FFFFFFF;
     }
}

char fread_letter(FILE *fp)
{
  char c;
  do {c = getc(fp);
     } while (isspace(c));
  return c;
}

/* load the rooms */
void parse_room(FILE * fl, int Virtual_nr, int Virtual)
{
  static int room_nr = 0, zone = 0;
  int t[10], i;
  char line[256], flags[128];
  struct extra_descr_data *new_descr;
  char letter;

  if (Virtual)
     {Virtual_nr = zone_table[zone].top;
     }

  sprintf(buf2, "room #%d%s", Virtual_nr,Virtual ? "(Virtual)" : "");

  if (Virtual_nr <= (zone ? zone_table[zone - 1].top : -1))
     {log("SYSERR: Room #%d is below zone %d.", Virtual_nr, zone);
      exit(1);
     }
  while (Virtual_nr > zone_table[zone].top)
        if (++zone > top_of_zone_table)
           {log("SYSERR: Room %d is outside of any zone.", Virtual_nr);
            exit(1);
           }
  world[room_nr].zone   = zone;
  world[room_nr].number = Virtual_nr;
  if (Virtual)
     {world[room_nr].name   = str_dup("Виртуальная комната");
      world[room_nr].description = str_dup("Похоже, здесь Вам делать нечего.");
      world[room_nr].room_flags.flags[0]  = 0;
      world[room_nr].room_flags.flags[1]  = 0;
      world[room_nr].room_flags.flags[2]  = 0;
      world[room_nr].room_flags.flags[3]  = 0;
      world[room_nr].sector_type    = SECT_SECRET;
     }
  else
     {world[room_nr].name   = fread_string(fl, buf2);
      world[room_nr].description = fread_string(fl, buf2);

      if (!get_line(fl, line))
         {log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!",
                  Virtual_nr);
          exit(1);
         }

      if (sscanf(line, " %d %s %d ", t, flags, t + 2) != 3)
         {log("SYSERR: Format error in roomflags/sector type of room #%d",
                  Virtual_nr);
          exit(1);
         }
      /* t[0] is the zone number; ignored with the zone-file system */
      asciiflag_conv(flags, &world[room_nr].room_flags);
      world[room_nr].sector_type = t[2];
     }

  world[room_nr].func      = NULL;
  world[room_nr].contents  = NULL;
  world[room_nr].people    = NULL;
  world[room_nr].track     = NULL;
  world[room_nr].light     = 0; /* Zero light sources */
  world[room_nr].fires     = 0;
  world[room_nr].gdark     = 0;
  world[room_nr].glight    = 0;
  world[room_nr].forbidden = 0;

  for (i = 0; i < NUM_OF_DIRS; i++)
      world[room_nr].dir_option[i] = NULL;

  world[room_nr].ex_description = NULL;
  if (Virtual)
     {top_of_world = room_nr++;
      return;
     }

  sprintf(buf,"SYSERR: Format error in room #%d (expecting D/E/S)",Virtual_nr);

  for (;;)
      {if (!get_line(fl, line))
          {log(buf);
           exit(1);
          }
       switch (*line)
       {
    case 'D':
      setup_dir(fl, room_nr, atoi(line + 1));
      break;
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(fl, buf2);
      new_descr->description = fread_string(fl, buf2);
      new_descr->next = world[room_nr].ex_description;
      world[room_nr].ex_description = new_descr;
      break;
    case 'S':                   /* end of room */
      /* DG triggers -- script is defined after the end of the room */
      letter = fread_letter(fl);
      ungetc(letter, fl);
      while (letter=='T')
            {dg_read_trigger(fl, &world[room_nr], WLD_TRIGGER);
             letter = fread_letter(fl);
             ungetc(letter, fl);
            }
      top_of_world = room_nr++;
      return;
    default:
      log(buf);
      exit(1);
       }
      }
}



/* read direction data */
void setup_dir(FILE * fl, int room, int dir)
{ int t[5];
  char line[256];

  sprintf(buf2, "room #%d, direction D%d", GET_ROOM_VNUM(room), dir);

  CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
  world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
  world[room].dir_option[dir]->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line))
     {log("SYSERR: Format error, %s", buf2);
      exit(1);
     }
  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3)
     {log("SYSERR: Format error, %s", buf2);
      exit(1);
     }

  if (t[0] & 1)
     world[room].dir_option[dir]->exit_info = EX_ISDOOR;
  else
  if (t[0] & 2)
     world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
  else
     world[room].dir_option[dir]->exit_info = 0;
  if (t[0] & 4)
     world[room].dir_option[dir]->exit_info |= EX_HIDDEN;

  world[room].dir_option[dir]->key     = t[1];
  world[room].dir_option[dir]->to_room = t[2];
}


/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{ if ((r_mortal_start_room = real_room(mortal_start_room)) < 0)
     {log("SYSERR:  Mortal start room does not exist.  Change in config.c. %d",mortal_start_room);
      exit(1);
     }
  if ((r_immort_start_room = real_room(immort_start_room)) < 0)
     {if (!mini_mud)
         log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
      r_immort_start_room = r_mortal_start_room;
     }
  if ((r_frozen_start_room = real_room(frozen_start_room)) < 0)
     {if (!mini_mud)
         log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
      r_frozen_start_room = r_mortal_start_room;
     }
  if ((r_helled_start_room = real_room(helled_start_room)) < 0)
     {if (!mini_mud)
         log("SYSERR:  Warning: Hell start room does not exist.  Change in config.c.");
      r_helled_start_room = r_mortal_start_room;
     }
  if ((r_named_start_room = real_room(named_start_room)) < 0)
     {if (!mini_mud)
         log("SYSERR:  Warning: NAME start room does not exist.  Change in config.c.");
      r_named_start_room = r_mortal_start_room;
     }
  if ((r_unreg_start_room = real_room(unreg_start_room)) < 0)
     {if (!mini_mud)
         log("SYSERR:  Warning: UNREG start room does not exist.  Change in config.c.");
      r_unreg_start_room = r_mortal_start_room;
     }
}


/* resolve all vnums into rnums in the world */
void renum_world(void)
{ register int room, door;

  for (room = 0; room <= top_of_world; room++)
      for (door = 0; door < NUM_OF_DIRS; door++)
          if (world[room].dir_option[door])
                 if (world[room].dir_option[door]->to_room != NOWHERE)
                    world[room].dir_option[door]->to_room =
                    real_room(world[room].dir_option[door]->to_room);
}


#define ZCMD zone_table[zone].cmd[cmd_no]

/* resulve vnums into rnums in the zone reset tables */
void renum_zone_table(void)
{
  int cmd_no, a, b, c, olda, oldb, oldc;
  zone_rnum zone;
  char buf[128];

  for (zone = 0; zone <= top_of_zone_table; zone++)
      for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
          {a = b = c = 0;
           olda = ZCMD.arg1;
           oldb = ZCMD.arg2;
           oldc = ZCMD.arg3;
           switch (ZCMD.command)
           {
      case 'M':
        a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
        c = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
          case 'Q':
        a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
        break;
          case 'F':
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
        b = ZCMD.arg2 = real_mobile(ZCMD.arg2);
        c = ZCMD.arg3 = real_mobile(ZCMD.arg3);
        break;
      case 'O':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        if (ZCMD.arg3 != NOWHERE)
           c = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      case 'G':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        break;
      case 'E':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        break;
      case 'P':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        c = ZCMD.arg3 = real_object(ZCMD.arg3);
        break;
      case 'D':
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
        break;
      case 'R': /* rem obj from room */
    a = ZCMD.arg1 = real_room(ZCMD.arg1);
        b = ZCMD.arg2 = real_object(ZCMD.arg2);
        break;
      case 'T': /* a trigger */
       /* designer's choice: convert this later */
       /* b = ZCMD.arg2 = real_trigger(ZCMD.arg2); */
    b = real_trigger(ZCMD.arg2); /* leave this in for validation */
    break;
      case 'V': /* trigger variable assignment */
    if (ZCMD.arg1 == WLD_TRIGGER)
       b = ZCMD.arg2 = real_room(ZCMD.arg2);
       break;
           }
      if (a < 0 || b < 0 || c < 0)
         {if (!mini_mud)
             {sprintf(buf,  "Invalid vnum %d, cmd disabled",
                                  (a < 0) ? olda : ((b < 0) ? oldb : oldc));
                  log_zone_error(zone, cmd_no, buf);
                 }
              ZCMD.command = '*';
         }
          }
}


void parse_simple_mob(FILE *mob_f, int i, int nr)
{
  int j, t[10];
  char line[256];

  mob_proto[i].real_abils.str   = 11;
  mob_proto[i].real_abils.intel = 11;
  mob_proto[i].real_abils.wis   = 11;
  mob_proto[i].real_abils.dex   = 11;
  mob_proto[i].real_abils.con   = 11;
  mob_proto[i].real_abils.cha   = 11;

  if (!get_line(mob_f, line))
  {log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
    exit(1);
  }
  if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
          t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9)
     {log("SYSERR: Format error in mob #%d, 1th line\n"
              "...expecting line of form '# # # #d#+# #d#+#'", nr);
      exit(1);
     }

  GET_LEVEL(mob_proto + i)        = t[0];
  mob_proto[i].real_abils.hitroll = 20 - t[1];
  mob_proto[i].real_abils.armor   = 10 * t[2];

  /* max hit = 0 is a flag that H, M, V is xdy+z */
  mob_proto[i].points.max_hit = 0;
  mob_proto[i].ManaMemNeeded  = t[3];
  mob_proto[i].ManaMemStored  = t[4];
  mob_proto[i].points.hit     = t[5];

  mob_proto[i].points.move     = 100;
  mob_proto[i].points.max_move = 100;

  mob_proto[i].mob_specials.damnodice   = t[6];
  mob_proto[i].mob_specials.damsizedice = t[7];
  mob_proto[i].real_abils.damroll       = t[8];

  if (!get_line(mob_f, line))
  { log("SYSERR: Format error in mob #%d, 2th line\n"
        "...expecting line of form '#d#+# #', but file ended!", nr);
      exit(1);
  }
  if (sscanf(line, " %dd%d+%d %d", t, t + 1, t+2, t+3) != 4)
  { log("SYSERR: Format error in mob #%d, 2th line\n"
        "...expecting line of form '#d#+# #'", nr);
    exit(1);
  }

  GET_GOLD(mob_proto + i)    = t[2];
  GET_GOLD_NoDs(mob_proto+i) = t[0];
  GET_GOLD_SiDs(mob_proto+i) = t[1];
  GET_EXP(mob_proto + i)     = t[3];

  if (!get_line(mob_f, line))
     {log("SYSERR: Format error in 3th line of mob #%d\n"
              "...expecting line of form '# # #', but file ended!", nr);
      exit(1);
     }

  if (sscanf(line, " %d %d %d", t, t + 1, t + 2) != 3)
     {log("SYSERR: Format error in 3th line of mob #%d\n"
              "...expecting line of form '# # #'", nr);
      exit(1);
     }

  mob_proto[i].char_specials.position   = t[0];
  mob_proto[i].mob_specials.default_pos = t[1];
  mob_proto[i].player.sex               = t[2];

  mob_proto[i].player.chclass = 0;
  mob_proto[i].player.weight = 200;
  mob_proto[i].player.height = 198;

  /*
   * these are now save applies; base save numbers for MOBs are now from
   * the warrior save table.
   */
  for (j = 0; j < 6; j++)
      GET_SAVE(mob_proto + i, j) = 0;
}



/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 */

#define CASE(test) if (!matched && !str_cmp(keyword, test) && (matched = 1))
#define RANGE(low, high) (num_arg = MAX((low), MIN((high), (num_arg))))

void interpret_espec(const char *keyword, const char *value, int i, int nr)
{ struct helper_data_type *helper;
  int num_arg, matched = 0, t[5];

  num_arg = atoi(value);

  CASE("BareHandAttack")
  { RANGE(0, 99);
    mob_proto[i].mob_specials.attack_type = num_arg;
  }

  CASE("Destination")
  { if (mob_proto[i].mob_specials.dest_count < MAX_DEST)
       {mob_proto[i].mob_specials.dest[mob_proto[i].mob_specials.dest_count] = num_arg;
        mob_proto[i].mob_specials.dest_count++;
       }
  }

  CASE("Str")
  { RANGE(3, 50);
    mob_proto[i].real_abils.str = num_arg;
  }

  CASE("StrAdd")
  { RANGE(0, 100);
    mob_proto[i].add_abils.str_add = num_arg;
  }

  CASE("Int")
  { RANGE(3, 50);
    mob_proto[i].real_abils.intel = num_arg;
  }

  CASE("Wis")
  { RANGE(3, 50);
    mob_proto[i].real_abils.wis = num_arg;
  }

  CASE("Dex")
  { RANGE(3, 50);
    mob_proto[i].real_abils.dex = num_arg;
  }

  CASE("Con")
  { RANGE(3, 50);
    mob_proto[i].real_abils.con = num_arg;
  }

  CASE("Cha")
  { RANGE(3, 50);
    mob_proto[i].real_abils.cha = num_arg;
  }

  CASE("Size")
  {   RANGE(0,100);
      mob_proto[i].real_abils.size    = num_arg;
  }
  /**** Extended for Adamant */
  CASE("LikeWork")
  {  RANGE(0,200);
     mob_proto[i].mob_specials.LikeWork = num_arg;
  }

  CASE("MaxFactor")
  {  RANGE(0,255);
     mob_proto[i].mob_specials.MaxFactor = num_arg;
  }

  CASE("ExtraAttack")
  {  RANGE(0,255);
     mob_proto[i].mob_specials.ExtraAttack = num_arg;
  }

  CASE("Class")
  {   RANGE(100,116);
      mob_proto[i].player.chclass = num_arg;
  }


  CASE("Height")
  {  RANGE(0,200);
     mob_proto[i].player.height = num_arg;
  }

  CASE("Weight")
  {  RANGE(0,200);
     mob_proto[i].player.weight = num_arg;
  }

  CASE("Race")
  {  RANGE(0,20);
     mob_proto[i].player.Race    = num_arg;
  }

  CASE("Special_Bitvector")
  {
     asciiflag_conv((char *)value, &mob_proto[i].mob_specials.npc_flags);
  /**** Empty now */
  }

  CASE("Skill")
  {if (sscanf(value, "%d %d", t, t + 1) != 2)
      {log("SYSERROR : Excepted format <# #> for SKILL in MOB #%d",i);
       return;
      }
   if (t[0] > MAX_SKILLS && t[0] < 1)
      {log("SYSERROR : Unknown skill No %d for MOB #%d",t[0],i);
       return;
      }
   t[1] = MIN(200, MAX(0, t[1]));
   SET_SKILL(mob_proto+i,t[0],t[1]);
  }

  CASE("Spell")
  {if (sscanf(value, "%d", t+0) != 1)
      {log("SYSERROR : Excepted format <#> for SPELL in MOB #%d",i);
       return;
      }
   if (t[0] > MAX_SPELLS && t[0] < 1)
      {log("SYSERROR : Unknown spell No %d for MOB #%d",t[0],i);
       return;
      }
   GET_SPELL_MEM(mob_proto+i,t[0]) += 1;
   GET_CASTER(mob_proto+i) += (IS_SET(spell_info[t[0]].routines,NPC_CALCULATE) ? 1 : 0);
   log("Set spell %d to %d(%s)",  t[0], GET_SPELL_MEM(mob_proto+i,t[0]),GET_NAME(mob_proto+i));
  }

  CASE("Helper")
  { CREATE(helper, struct helper_data_type, 1);
    helper->mob_vnum = num_arg;
    helper->next_helper = GET_HELPER(mob_proto+i);
    GET_HELPER(mob_proto+i) = helper;
  }

  if (!matched)
     {log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d",
              keyword, nr);
     }
}

#undef CASE
#undef RANGE

void parse_espec(char *buf, int i, int nr)
{
  char *ptr;

  if ((ptr = strchr(buf, ':')) != NULL)
     { *(ptr++) = '\0';
       while (a_isspace(*ptr))
             ptr++;
      #if 0     /* Need to evaluate interpret_espec()'s NULL handling. */
     }
      #else
     } else
       ptr = "";
      #endif
  interpret_espec(buf, ptr, i, nr);
}


void parse_enhanced_mob(FILE *mob_f, int i, int nr)
{
  char line[256];

  parse_simple_mob(mob_f,i,nr);

  while (get_line(mob_f, line))
  { if (!strcmp(line, "E"))     /* end of the enhanced section */
       return;
    else
      if (*line == '#')
         {      /* we've hit the next mob, maybe? */
            log("SYSERR: Unterminated E section in mob #%d", nr);
            exit(1);
         }
      else
         parse_espec(line, i, nr);
  }

  log("SYSERR: Unexpected end of file reached after mob #%d", nr);
  exit(1);
}


void parse_mobile(FILE * mob_f, int nr)
{
  static int i = 0;
  int j, t[10];
  char line[256], *tmpptr, letter;
  char f1[128], f2[128];

  mob_index[i].vnum = nr;
  mob_index[i].number = 0;
  mob_index[i].func = NULL;

  clear_char(mob_proto + i);
  /*
   * Mobiles should NEVER use anything in the 'player_specials' structure.
   * The only reason we have every mob in the game share this copy of the
   * structure is to save newbie coders from themselves. -gg 2/25/98
   */
  mob_proto[i].player_specials = &dummy_mob;
  sprintf(buf2, "mob vnum %d", nr);
  /***** String data *****/
  mob_proto[i].player.name = fread_string(mob_f, buf2);  /* aliases*/
  tmpptr = mob_proto[i].player.short_descr = fread_string(mob_f, buf2);
                                                         /* real name */
  CREATE(GET_PAD(mob_proto+i,0), char, strlen(mob_proto[i].player.short_descr)+1);
  strcpy(GET_PAD(mob_proto+i,0), mob_proto[i].player.short_descr);
  for(j=1;j < NUM_PADS;j++)
     GET_PAD(mob_proto+i,j) = fread_string(mob_f, buf2);
  if (tmpptr && *tmpptr)
     if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
             !str_cmp(fname(tmpptr), "the"))
        *tmpptr = LOWER(*tmpptr);

  mob_proto[i].player.long_descr    = fread_string(mob_f, buf2);
  mob_proto[i].player.description   = fread_string(mob_f, buf2);
  mob_proto[i].mob_specials.Questor = NULL;
  mob_proto[i].player.title         = NULL;

  // mob_proto[i].mob_specials.Questor = fread_string(mob_f, buf2);

  /* *** Numeric data *** */
  if (!get_line(mob_f, line))
     {log("SYSERR: Format error after string section of mob #%d\n"
          "...expecting line of form '# # # {S | E}', but file ended!\n%s", nr,line);
      exit(1);
     }

#ifdef CIRCLE_ACORN     /* Ugh. */
  if (sscanf(line, "%s %s %d %s", f1, f2, t + 2, &letter) != 4)
     {
#else
  if (sscanf(line, "%s %s %d %c", f1, f2, t + 2, &letter) != 4)
     {
#endif
     log("SYSERR: Format error after string section of mob #%d\n"
             "...expecting line of form '# # # {S | E}'\n%s", nr,line);
     exit(1);
    }
  asciiflag_conv(f1, &MOB_FLAGS(mob_proto+i, 0));
  SET_BIT(MOB_FLAGS(mob_proto + i, MOB_ISNPC), MOB_ISNPC);
  asciiflag_conv(f2, &AFF_FLAGS(mob_proto + i, 0));
  GET_ALIGNMENT(mob_proto + i) = t[2];
  switch (UPPER(letter))
  {
  case 'S':     /* Simple monsters */
    parse_simple_mob(mob_f, i, nr);
    break;
  case 'E':     /* Circle3 Enhanced monsters */
    parse_enhanced_mob(mob_f, i, nr);
    break;
  /* add new mob types here.. */
  default:
    log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
    exit(1);
  }

  /* DG triggers -- script info follows mob S/E section */
  letter = fread_letter(mob_f);
  ungetc(letter, mob_f);
  while (letter=='T')
        {dg_read_trigger(mob_f, &mob_proto[i], MOB_TRIGGER);
         letter = fread_letter(mob_f);
         ungetc(letter, mob_f);
        }

  for (j = 0; j < NUM_WEARS; j++)
      mob_proto[i].equipment[j] = NULL;

  mob_proto[i].nr   = i;
  mob_proto[i].desc = NULL;

  /**** See reading mob
  log("%s/%s/%s/%s/%s/%s",GET_PAD(mob_proto+i,0),GET_PAD(mob_proto+i,1),
                          GET_PAD(mob_proto+i,2),GET_PAD(mob_proto+i,3),
                          GET_PAD(mob_proto+i,4),GET_PAD(mob_proto+i,5));

  log("Str: %d, Dex: %d, Con: %d, Int: %d, Wis: %d, Cha: %d, Size: %d",
      GET_STR(mob_proto+i),GET_DEX(mob_proto+i),GET_CON(mob_proto+i),
      GET_INT(mob_proto+i),GET_WIS(mob_proto+i),GET_CHA(mob_proto+i),
      GET_SIZE(mob_proto+i));

  log("Weight: %d, Height: %d, Damage: %dD%d",
       GET_WEIGHT(mob_proto+i),
       GET_HEIGHT(mob_proto+i),
       mob_proto[i].mob_specials.damnodice,
       mob_proto[i].mob_specials.damsizedice);

  log("Level: %d, HR: %d, DR: %d, AC: %d",
      GET_LEVEL(mob_proto + i),
      mob_proto[i].real_abils.damroll,
      mob_proto[i].real_abils.hitroll,
      mob_proto[i].real_abils.armor);

  log("MaxHit: %d, Hit: %dD%d+%d, Mv: %d",
      mob_proto[i].points.max_hit,
      mob_proto[i].ManaMemNeeded,
      mob_proto[i].ManaMemStored,
      mob_proto[i].points.hit,
      mob_proto[i].points.move);
  for(j=1, count=0; j<MAX_SPELLS; j++)
     {if (GET_SPELL_MEM(mob_proto+i,j))
         count += sprintf(buf+count," %d ",j);
     }
  if (count) log("SPELLS : %s", buf);
  for(j=1, count=0; j<MAX_SKILLS; j++)
     {if (GET_SKILL(mob_proto+i,j))
         count += sprintf(buf+count," %d ",j);
     }
  if (count) log("SKILLS : %s", buf);
  */

  top_of_mobt = i++;
}

#define SEVEN_DAYS 60*24*30

/* read all objects from obj file; generate index and prototypes */
char *parse_object(FILE * obj_f, int nr)
{
  static int i = 0;
  static char line[256];
  int t[10], j, retval;
  char *tmpptr;
  char f0[256], f1[256], f2[256];
  struct extra_descr_data *new_descr;

  obj_index[i].vnum        = nr;
  obj_index[i].number      = 0;
  obj_index[i].stored      = 0;
  obj_index[i].func        = NULL;

  clear_object(obj_proto + i);
  obj_proto[i].item_number = i;

  /**** Add some initialization fields */
  obj_proto[i].obj_flags.Obj_skill = 0;
  obj_proto[i].obj_flags.Obj_max   = 100;
  obj_proto[i].obj_flags.Obj_cur   = 100;
  obj_proto[i].obj_flags.Obj_mater = 0;
  obj_proto[i].obj_flags.Obj_sex   = 1;
  obj_proto[i].obj_flags.Obj_timer = SEVEN_DAYS;
  obj_proto[i].obj_flags.Obj_spell = 0;
  obj_proto[i].obj_flags.Obj_level = 1;
  obj_proto[i].obj_flags.affects   = clear_flags;
  obj_proto[i].obj_flags.no_flag   = clear_flags;
  obj_proto[i].obj_flags.anti_flag = clear_flags;
  obj_proto[i].obj_flags.Obj_destroyer = 60;

  sprintf(buf2, "object #%d", nr);

  /* *** string data *** */
  if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL)
     {log("SYSERR: Null obj name or format error at or near %s", buf2);
      exit(1);
     }
  tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);
  *obj_proto[i].short_description = LOWER(*obj_proto[i].short_description);
  CREATE(obj_proto[i].PNames[0], char, strlen(obj_proto[i].short_description)+1);
  strcpy(obj_proto[i].PNames[0], obj_proto[i].short_description);

  for(j=1;j < NUM_PADS;j++)
     {obj_proto[i].PNames[j]  = fread_string(obj_f, buf2);
      *obj_proto[i].PNames[j] = LOWER(*obj_proto[i].PNames[j]);
     }

  if (tmpptr && *tmpptr)
     if (!str_cmp(fname(tmpptr), "a")  ||
         !str_cmp(fname(tmpptr), "an") ||
         !str_cmp(fname(tmpptr), "the")
        )
       *tmpptr = LOWER(*tmpptr);

  tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
     CAP(tmpptr);
  obj_proto[i].action_description = fread_string(obj_f, buf2);

  if (!get_line(obj_f, line))
     {log("SYSERR: Expecting *1th* numeric line of %s, but file ended!", buf2);
      exit(1);
     }
  if ((retval = sscanf(line, " %s %d %d %d", f0, t+1, t+2, t+3)) != 4)
     {log("SYSERR: Format error in *1th* numeric line (expecting 4 args, got %d), %s", retval, buf2);
      exit(1);
     }
  asciiflag_conv(f0, &obj_proto[i].obj_flags.Obj_skill);
  obj_proto[i].obj_flags.Obj_max   = t[1];
  obj_proto[i].obj_flags.Obj_cur   = t[2];
  obj_proto[i].obj_flags.Obj_mater = t[3];

  if (!get_line(obj_f, line))
     {log("SYSERR: Expecting *2th* numeric line of %s, but file ended!", buf2);
      exit(1);
     }
  if ((retval = sscanf(line, " %d %d %d %d", t, t+1, t+2, t+3)) != 4)
     {log("SYSERR: Format error in *2th* numeric line (expecting 4 args, got %d), %s", retval, buf2);
      exit(1);
     }
  obj_proto[i].obj_flags.Obj_sex   = t[0];
  obj_proto[i].obj_flags.Obj_timer = t[1] > 0 ? t[1] : SEVEN_DAYS;
  obj_proto[i].obj_flags.Obj_spell = t[2];
  obj_proto[i].obj_flags.Obj_level = t[3];

  if (!get_line(obj_f, line))
     {log("SYSERR: Expecting *3th* numeric line of %s, but file ended!", buf2);
      exit(1);
     }
  if ((retval = sscanf(line, " %s %s %s", f0, f1, f2)) != 3)
     {log("SYSERR: Format error in *3th* numeric line (expecting 3 args, got %d), %s", retval, buf2);
      exit(1);
     }
  asciiflag_conv(f0, &obj_proto[i].obj_flags.affects);   /*** Affects */
  asciiflag_conv(f1, &obj_proto[i].obj_flags.anti_flag); /*** Miss for ...    */
  asciiflag_conv(f2, &obj_proto[i].obj_flags.no_flag);   /*** Deny for ...    */

  if (!get_line(obj_f, line))
     {log("SYSERR: Expecting *3th* numeric line of %s, but file ended!", buf2);
      exit(1);
     }
  if ((retval = sscanf(line, " %d %s %s", t, f1, f2)) != 3)
     {log("SYSERR: Format error in *3th* misc line (expecting 3 args, got %d), %s", retval, buf2);
      exit(1);
     }
  obj_proto[i].obj_flags.type_flag   = t[0];                /*** What's a object */
  asciiflag_conv(f1, &obj_proto[i].obj_flags.extra_flags);  /*** Its effects     */
  asciiflag_conv(f2, &obj_proto[i].obj_flags.wear_flags);  /*** Wear on ...     */

  if (!get_line(obj_f, line))
     {log("SYSERR: Expecting *5th* numeric line of %s, but file ended!", buf2);
      exit(1);
     }
  if ((retval = sscanf(line, "%s %d %d %d", f0, t + 1, t + 2, t + 3)) != 4)
     {log("SYSERR: Format error in *5th* numeric line (expecting 4 args, got %d), %s", retval, buf2);
      exit(1);
     }
  asciiflag_conv(f0,&obj_proto[i].obj_flags.value); /****/
  obj_proto[i].obj_flags.value[1] = t[1]; /****/
  obj_proto[i].obj_flags.value[2] = t[2]; /****/
  obj_proto[i].obj_flags.value[3] = t[3]; /****/

  if (!get_line(obj_f, line))
     {log("SYSERR: Expecting *6th* numeric line of %s, but file ended!", buf2);
      exit(1);
     }
  if ((retval = sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3)) != 4)
     {log("SYSERR: Format error in *6th* numeric line (expecting 4 args, got %d), %s", retval, buf2);
      exit(1);
     }
  obj_proto[i].obj_flags.weight          = t[0];
  obj_proto[i].obj_flags.cost            = t[1];
  obj_proto[i].obj_flags.cost_per_day_off= t[2];
  obj_proto[i].obj_flags.cost_per_day_on = t[3];

  /* check to make sure that weight of containers exceeds curr. quantity */
  if (obj_proto[i].obj_flags.type_flag == ITEM_DRINKCON ||
      obj_proto[i].obj_flags.type_flag == ITEM_FOUNTAIN)
  {if (obj_proto[i].obj_flags.weight < obj_proto[i].obj_flags.value[1])
      obj_proto[i].obj_flags.weight = obj_proto[i].obj_flags.value[1] + 5;
  }

  /* *** extra descriptions and affect fields *** */
  for (j = 0; j < MAX_OBJ_AFFECT; j++)
      {obj_proto[i].affected[j].location = APPLY_NONE;
       obj_proto[i].affected[j].modifier = 0;
      }

  strcat(buf2, ", after numeric constants\n"
         "...expecting 'E', 'A', '$', or next object number");
  j = 0;

  for (;;)
     {if (!get_line(obj_f, line))
         {log("SYSERR: Format error in %s", buf2);
          exit(1);
         }
      switch (*line)
         { case 'E':
                    CREATE(new_descr, struct extra_descr_data, 1);
                    new_descr->keyword = fread_string(obj_f, buf2);
                    new_descr->description = fread_string(obj_f, buf2);
                    new_descr->next = obj_proto[i].ex_description;
                    obj_proto[i].ex_description = new_descr;
                    break;
            case 'A':
                    if (j >= MAX_OBJ_AFFECT)
                    {log("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFFECT, buf2);
                         exit(1);
                    }
                    if (!get_line(obj_f, line))
                    {log("SYSERR: Format error in 'A' field, %s\n"
                             "...expecting 2 numeric constants but file ended!", buf2);
                         exit(1);
                    }
                    if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2)
                    {log("SYSERR: Format error in 'A' field, %s\n"
                             "...expecting 2 numeric arguments, got %d\n"
                             "...offending line: '%s'", buf2, retval, line);
                         exit(1);
                    }
                    obj_proto[i].affected[j].location = t[0];
                    obj_proto[i].affected[j].modifier = t[1];
                    j++;
                    break;
            case 'T':  /* DG triggers */
                    dg_obj_trigger(line, &obj_proto[i]);
                    break;
	    case 'V':
	    case 'R':
	    case 'M': break; // prool
            case '$':
            case '#':
                    check_object(&obj_proto[i]);
                    top_of_objt = i++;
                    return (line);
             default:
                    log("SYSERR: Format error in %s", buf2);
                    exit(1);
         }
     }
}


#define Z       zone_table[zone]

/* load the zone table and command tables */
void load_zones(FILE * fl, char *zonename)
{
  static zone_rnum zone = 0;
  int cmd_no, num_of_cmds = 0, line_num = 0, tmp, error;
  char *ptr, buf[256], zname[256];
  char t1[80], t2[80];

  strcpy(zname, zonename);

  while (get_line(fl, buf))
        num_of_cmds++;          /* this should be correct within 3 or so */
  rewind(fl);

  if (num_of_cmds == 0)
     {log("SYSERR: %s is empty!", zname);
      exit(1);
     }
  else
     CREATE(Z.cmd, struct reset_com, num_of_cmds);

  line_num += get_line(fl, buf);

  if (sscanf(buf, "#%d", &Z.number) != 1)
     {log("SYSERR: Format error in %s, line %d", zname, line_num);
      exit(1);
     }
  sprintf(buf2, "beginning of zone #%d", Z.number);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL) /* take off the '~' if it's there */
     *ptr = '\0';
  Z.name = str_dup(buf);

  line_num += get_line(fl, buf);
  if (sscanf(buf, " %d %d %d ", &Z.top, &Z.lifespan, &Z.reset_mode) != 3)
     {log("SYSERR: Format error in 3-constant line of %s", zname);
      exit(1);
     }
  cmd_no = 0;

  for (;;)
     {if ((tmp = get_line(fl, buf)) == 0)
         {log("SYSERR: Format error in %s - premature end of file", zname);
          exit(1);
         }
      line_num += tmp;
      ptr = buf;
      skip_spaces(&ptr);

      if ((ZCMD.command = *ptr) == '*')
         continue;
      ptr++;

      if (ZCMD.command == 'S' || ZCMD.command == '$')
         {ZCMD.command = 'S';
          break;
         }
      error     = 0;
      ZCMD.arg4 = -1;
      if (strchr("MOEGPDTVQF", ZCMD.command) == NULL)
         {/* a 3-arg command */
          if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
                 error = 1;
         }
      else
      if (ZCMD.command=='V')
         {/* a string-arg command */
          if (sscanf(ptr, " %d %d %d %d %s %s", &tmp, &ZCMD.arg1, &ZCMD.arg2,
                             &ZCMD.arg3, t1, t2) != 6)
                 error = 1;
          else
            {ZCMD.sarg1 = str_dup(t1);
             ZCMD.sarg2 = str_dup(t2);
            }
         }
      else
      if (ZCMD.command=='Q')
         {/* a number command */
          if (sscanf(ptr, " %d %d", &tmp, &ZCMD.arg1) != 2)
             error = 1;
          else
             tmp   = 0;
         }
      else
         {if (sscanf(ptr, " %d %d %d %d %d", &tmp, &ZCMD.arg1, &ZCMD.arg2,
                             &ZCMD.arg3, &ZCMD.arg4) < 4)
              error = 1;
         }

      ZCMD.if_flag = tmp;

      if (error)
         {log("SYSERR: :-( Format error in %s, line %d: '%s'", zname, line_num, buf); // prool
          exit(1);
         }
      ZCMD.line = line_num;
      cmd_no++;
     }
  top_of_zone_table = zone++;
}

#undef Z


void get_one_line(FILE *fl, char *buf)
{if (fgets(buf, READ_SIZE, fl) == NULL)
    {log("SYSERR: error reading help file: not terminated with $?");
     exit(1);
    }
  buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}


void load_help(FILE *fl)
{
#if defined(CIRCLE_MACINTOSH)
  static char key[READ_SIZE+1], next_key[READ_SIZE+1], entry[32384]; /* ? */
#else
  char key[READ_SIZE+1], next_key[READ_SIZE+1], entry[32384];
#endif
  char line[READ_SIZE+1], *scan;
  struct help_index_element el;

  /* get the first keyword line */
  get_one_line(fl, key);
  while (*key != '$')
     {/* read in the corresponding help entry */
      strcpy(entry, strcat(key, "\r\n"));
      get_one_line(fl, line);
      while (*line != '#')
            {strcat(entry, strcat(line, "\r\n"));
             get_one_line(fl, line);
            }

      /* now, add the entry to the index with each keyword on the keyword line */
      el.duplicate = 0;
      el.entry     = str_dup(entry);
      scan         = one_word(key, next_key);
      while (*next_key)
            {el.keyword = str_dup(next_key);
             help_table[top_of_helpt++] = el;
             el.duplicate++;
             scan = one_word(scan, next_key);
            }

      /* get next keyword line (or $) */
      get_one_line(fl, key);
     }
}


int hsort(const void *a, const void *b)
{
  const struct help_index_element *a1, *b1;

  a1 = (const struct help_index_element *) a;
  b1 = (const struct help_index_element *) b;

  return (str_cmp(a1->keyword, b1->keyword));
}


int csort(const void *a, const void *b)
{
  const struct social_keyword *a1, *b1;

  a1 = (const struct social_keyword *) a;
  b1 = (const struct social_keyword *) b;

  return (str_cmp(a1->keyword, b1->keyword));
}


/*************************************************************************
*  procedures for resetting, both play-time and boot-time                *
*************************************************************************/



int vnum_mobile(char *searchname, struct char_data * ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_mobt; nr++)
      {if (isname(searchname, mob_proto[nr].player.name))
          {sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
                       mob_index[nr].vnum,
                       mob_proto[nr].player.short_descr);
           send_to_char(buf, ch);
          }
      }
  return (found);
}



int vnum_object(char *searchname, struct char_data * ch)
{ int nr, found = 0;

  for (nr = 0; nr <= top_of_objt; nr++)
      {if (isname(searchname, obj_proto[nr].name))
          {sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
                       obj_index[nr].vnum,
                       obj_proto[nr].short_description);
           send_to_char(buf, ch);
          }
      }
  return (found);
}


/* create a character, and add it to the char list */
struct char_data *create_char(void)
{ struct char_data *ch;

  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  ch->next = character_list;
  character_list = ch;
  GET_ID(ch) = max_id++;

  return (ch);
}


/* create a new mobile from a prototype */
struct char_data *read_mobile(mob_vnum nr, int type) /* and mob_rnum */
{
  mob_rnum i;
  struct char_data *mob;

  if (type == VIRTUAL)
     {if ((i = real_mobile(nr)) < 0)
         {log("WARNING: Mobile vnum %d does not exist in database.", nr);
          return (NULL);
         }
     }
  else
    i = nr;

  CREATE(mob, struct char_data, 1);
  clear_char(mob);
  *mob           = mob_proto[i];
  mob->next      = character_list;
  character_list = mob;

  if (!mob->points.max_hit)
    { mob->points.max_hit = dice(GET_MANA_NEED(mob),GET_MANA_STORED(mob)) +
                                 mob->points.hit;
    }
  else
    mob->points.max_hit = number(mob->points.hit, GET_MANA_NEED(mob));

  mob->points.hit    = mob->points.max_hit;
  GET_MANA_NEED(mob) = GET_MANA_STORED(mob) = 0;
  GET_HORSESTATE(mob)= 200;
  GET_LASTROOM(mob)  = NOWHERE;
  GET_PFILEPOS(mob)  = -1;
  GET_ACTIVITY(mob)  = number(0,PULSE_MOBILE-1);
  EXTRACT_TIMER(mob) = 0;
  mob->points.move   = mob->points.max_move;
  GET_GOLD(mob)     += dice(GET_GOLD_NoDs(mob), GET_GOLD_SiDs(mob));

  mob->player.time.birth  = time(0);
  mob->player.time.played = 0;
  mob->player.time.logon  = time(0);

  mob_index[i].number++;
  GET_ID(mob) = max_id++;
  assign_triggers(mob, MOB_TRIGGER);
  return (mob);
}


/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  obj->next             = object_list;
  object_list           = obj;
  GET_ID(obj)           = max_id++;
  GET_OBJ_ZONE(obj)     = NOWHERE;
  OBJ_GET_LASTROOM(obj) = NOWHERE;
  assign_triggers(obj, OBJ_TRIGGER);
  return (obj);
}


/* create a new object from a prototype */
struct obj_data *read_object(obj_vnum nr, int type) /* and obj_rnum */
{
  struct obj_data *obj;
  obj_rnum i;

  if (nr < 0)
     { log("SYSERR: Trying to create obj with negative (%d) num!", nr);
       return (NULL);
     }
  if (type == VIRTUAL)
    {if ((i = real_object(nr)) < 0)
        {log("Object (V) %d does not exist in database.", nr);
         return (NULL);
        }
    }
 else
    i = nr;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  *obj        = obj_proto[i];
  obj->next   = object_list;
  object_list = obj;
  obj_index[i].number++;
  GET_ID(obj)          = max_id++;
  GET_OBJ_ZONE(obj)    = NOWHERE;
  OBJ_GET_LASTROOM(obj)= NOWHERE;
  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
     {name_from_drinkcon(obj);
      if (GET_OBJ_VAL(obj,1) && GET_OBJ_VAL(obj,2))
         name_to_drinkcon(obj, GET_OBJ_VAL(obj,2));
     }
  assign_triggers(obj, OBJ_TRIGGER);

  return (obj);
}



#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i;
  struct reset_q_element *update_u, *temp;
  static int timer = 0;
  char   buf[128];

  /* jelson 10/22/92 */
  if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60)
     {/* one minute has passed */
      /*
       * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
       * factor of 60
       */

      timer = 0;

      /* since one minute has passed, increment zone ages */
      for (i = 0; i <= top_of_zone_table; i++)
          {if (zone_table[i].age < zone_table[i].lifespan &&
                   zone_table[i].reset_mode)
                  (zone_table[i].age)++;

           if (zone_table[i].age >= zone_table[i].lifespan &&
                   zone_table[i].age < ZO_DEAD &&
                   zone_table[i].reset_mode)
                  {/* enqueue zone */
                   CREATE(update_u, struct reset_q_element, 1);
                   update_u->zone_to_reset = i;
                   update_u->next = 0;

                   if (!reset_q.head)
                      reset_q.head = reset_q.tail = update_u;
                   else
                      {reset_q.tail->next = update_u;
                       reset_q.tail = update_u;
                      }

                   zone_table[i].age = ZO_DEAD;
              }
          }
     } /* end - one minute has passed */


  /* dequeue zones (if possible) and reset                    */
  /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
  for (update_u = reset_q.head; update_u; update_u = update_u->next)
      if (zone_table[update_u->zone_to_reset].reset_mode == 2 ||
              is_empty(update_u->zone_to_reset))
             {reset_zone(update_u->zone_to_reset);
          sprintf(buf, "Auto zone reset: %s",
                  zone_table[update_u->zone_to_reset].name);
          mudlog(buf, CMP, LVL_GOD, FALSE);
          /* dequeue */
          if (update_u == reset_q.head)
             reset_q.head = reset_q.head->next;
          else
             {for (temp = reset_q.head; temp->next != update_u;
                       temp = temp->next);

                  if (!update_u->next)
                     reset_q.tail = temp;

                  temp->next = update_u->next;
             }

          free(update_u);
          break;
         }
}

void log_zone_error(zone_rnum zone, int cmd_no, const char *message)
{
  char buf[256];

  sprintf(buf, "SYSERR: zone file: %s", message);
  mudlog(buf, NRM, LVL_GOD, TRUE);

  sprintf(buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
              ZCMD.command, zone_table[zone].number, ZCMD.line);
  mudlog(buf, NRM, LVL_GOD, TRUE);
}

#define ZONE_ERROR(message) \
        { log_zone_error(zone, cmd_no, message); last_cmd = 0; }

/* execute the reset command table of a given zone */
void reset_zone(zone_rnum zone)
{ int cmd_no, last_cmd  = 0;
  struct char_data *mob = NULL, *leader=NULL, *ch;
  struct obj_data *obj, *obj_to;
  int room_vnum, room_rnum;
  struct char_data *tmob=NULL; /* for trigger assignment */
  struct obj_data *tobj=NULL;  /* for trigger assignment */


  log("[Reset] Start zone %s",zone_table[zone].name);
  for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
      {if (ZCMD.if_flag && !last_cmd)
          continue;

       switch (ZCMD.command)
          {
    case '*':                   /* ignore command */
      last_cmd = 0;
      break;
    case 'M':                   /* read a mobile */
      if (mob_index[ZCMD.arg1].number < ZCMD.arg2 &&
          (ZCMD.arg4 < 0 ||
           mobs_in_room(ZCMD.arg1,ZCMD.arg3) < ZCMD.arg4)
         )
         {mob      = read_mobile(ZCMD.arg1, REAL);
          char_to_room(mob, ZCMD.arg3);
          load_mtrigger(mob);
          tmob     = mob;
              last_cmd = 1;
         }
      else
              last_cmd = 0;
          tobj = NULL;
      break;

    case 'F':           /* Follow mobiles */
      last_cmd = 0;
      leader   = NULL;
      if (ZCMD.arg1 >= 0 && ZCMD.arg1 <= top_of_world)
         {for (ch = world[ZCMD.arg1].people; ch && !leader; ch = ch->next_in_room)
              if (IS_NPC(ch) && GET_MOB_RNUM(ch) == ZCMD.arg2)
                 leader = ch;
          for (ch = world[ZCMD.arg1].people; ch && leader; ch = ch->next_in_room)
              if (IS_NPC(ch)                    &&
                  GET_MOB_RNUM(ch) == ZCMD.arg3 &&
                  leader     != ch              &&
                  ch->master != leader
                 )
                 {if (ch->master)
                     stop_follower(ch, SF_EMPTY);
                  add_follower(ch,leader);
                  last_cmd = 1;
                 }
         }
      break;

    case 'Q':                   /* delete all mobiles */
      last_cmd = 0;
      for (ch = character_list; ch; ch=leader)
          {leader = ch->next;
           if (IS_NPC(ch) && GET_MOB_RNUM(ch) == ZCMD.arg1)
              {extract_mob(ch);
               last_cmd = 1;
              }
          }
          tobj = NULL;
          tmob = NULL;
      break;

    case 'O':                   /* read an object */
      if (obj_index[ZCMD.arg1].number + obj_index[ZCMD.arg1].stored < ZCMD.arg2 &&
          (ZCMD.arg4 <= 0 || number(1,100) <= ZCMD.arg4))
         {if (ZCMD.arg3 >= 0)
             {obj = read_object(ZCMD.arg1, REAL);
              GET_OBJ_ZONE(obj) = world[ZCMD.arg3].zone;
                  obj_to_room(obj, ZCMD.arg3);
              load_otrigger(obj);
              tobj = obj;
                  last_cmd = 1;
                 }
              else
                 {obj = read_object(ZCMD.arg1, REAL);
                  obj->in_room = NOWHERE;
                  tobj = obj;
                  last_cmd = 1;
                 }
         }
      else
             last_cmd = 0;
          tmob = NULL;
      break;

    case 'P':                   /* object to object */
      if (obj_index[ZCMD.arg1].number + obj_index[ZCMD.arg1].stored < ZCMD.arg2 &&
          (ZCMD.arg4 <= 0 || number(1,100) <= ZCMD.arg4))
         {if (!(obj_to = get_obj_num(ZCMD.arg3)))
             { ZONE_ERROR("target obj not found, command disabled");    
               ZCMD.command = '*';
               break;
             }
          if (GET_OBJ_TYPE(obj_to) != ITEM_CONTAINER)
             { ZONE_ERROR("attempt put obj to non container, disabled");
               ZCMD.command = '*';
               break;
             }
          obj = read_object(ZCMD.arg1, REAL);
          if (obj_to->in_room != NOWHERE)
             GET_OBJ_ZONE(obj) = world[obj_to->in_room].zone;
          else
          if (obj_to->worn_by)
             GET_OBJ_ZONE(obj) = world[IN_ROOM(obj_to->worn_by)].zone;
          else
          if (obj_to->carried_by)
             GET_OBJ_ZONE(obj) = world[IN_ROOM(obj_to->carried_by)].zone;
          obj_to_obj(obj, obj_to);
          load_otrigger(obj);
          tobj = obj;
              last_cmd = 1;
         }
      else
             last_cmd = 0;
          tmob = NULL;
      break;

    case 'G':                   /* obj_to_char */
      if (!mob)
         {ZONE_ERROR("attempt to give obj to non-existant mob, command disabled");
          ZCMD.command = '*';
          break;
         }
      if (obj_index[ZCMD.arg1].number + obj_index[ZCMD.arg1].stored < ZCMD.arg2 &&
          (ZCMD.arg4 <= 0 || number(1,100) <= ZCMD.arg4))
         {obj = read_object(ZCMD.arg1, REAL);
              obj_to_char(obj, mob);
              GET_OBJ_ZONE(obj) = world[IN_ROOM(mob)].zone;
              tobj = obj;
          load_otrigger(obj);
              last_cmd = 1;
         }
      else
          last_cmd = 0;
      tmob = NULL;
      break;

    case 'E':                   /* object to equipment list */
      if (!mob)
         {ZONE_ERROR("trying to equip non-existant mob, command disabled");
              ZCMD.command = '*';
              break;
         }
      if (obj_index[ZCMD.arg1].number + obj_index[ZCMD.arg1].stored < ZCMD.arg2 &&
          (ZCMD.arg4 <= 0 || number(1,100) <= ZCMD.arg4))
         {if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS)
             {ZONE_ERROR("invalid equipment pos number");
                 }
              else
                 {obj = read_object(ZCMD.arg1, REAL);
                  GET_OBJ_ZONE(obj) = world[IN_ROOM(mob)].zone;
              IN_ROOM(obj) = IN_ROOM(mob);
              load_otrigger(obj);
              if (wear_otrigger(obj, mob, ZCMD.arg3))
                     {IN_ROOM(obj) = NOWHERE;
                      equip_char(mob, obj, ZCMD.arg3);
                     }
              else
                 obj_to_char(obj, mob);
              if (!(obj->carried_by == mob) && !(obj->worn_by == mob))
                 {extract_obj(obj);
                  tobj     = NULL;
                  last_cmd = 0;
                 }
              else
                 {tobj = obj;
                  last_cmd = 1;
                 }
                 }
         }
      else
             last_cmd = 0;
             tmob = NULL;
      break;

    case 'R': /* rem obj from room */
      if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1].contents)) != NULL)
         {obj_from_room(obj);
          extract_obj(obj);
         }
      last_cmd = 1;
      tmob = NULL;
      tobj = NULL;
      break;

    case 'D':                   /* set state of door */
      if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
             (world[ZCMD.arg1].dir_option[ZCMD.arg2] == NULL))
             {ZONE_ERROR("door does not exist, command disabled");
              ZCMD.command = '*';
         }
      else
             switch (ZCMD.arg3)
                {
        case 0:
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_LOCKED);
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_CLOSED);
          break;
        case 1:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_CLOSED);
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_LOCKED);
          break;
        case 2:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_LOCKED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_CLOSED);
          break;
        case 3:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_HIDDEN);
          break;        
        case 4: 
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_HIDDEN);
          break;
                }
      last_cmd = 1;
      tmob = NULL;
      tobj = NULL;
       break;

    case 'T': /* trigger command; details to be filled in later */
      if (ZCMD.arg1==MOB_TRIGGER && tmob)
         {if (!SCRIPT(tmob))
             CREATE(SCRIPT(tmob), struct script_data, 1);
          add_trigger(SCRIPT(tmob), read_trigger(real_trigger(ZCMD.arg2)), -1);
          last_cmd = 1;
         }
      else
      if (ZCMD.arg1==OBJ_TRIGGER && tobj)
         {if (!SCRIPT(tobj))
             CREATE(SCRIPT(tobj), struct script_data, 1);
          add_trigger(SCRIPT(tobj), read_trigger(real_trigger(ZCMD.arg2)), -1);
          last_cmd = 1;
         }
      break;

    case 'V':
      if (ZCMD.arg1==MOB_TRIGGER && tmob)
         {if (!SCRIPT(tmob))
             {ZONE_ERROR("Attempt to give variable to scriptless mobile");
             }
          else
             add_var(&(SCRIPT(tmob)->global_vars), ZCMD.sarg1, ZCMD.sarg2,
                                                   ZCMD.arg3);
          last_cmd = 1;
         }
      else
      if (ZCMD.arg1==OBJ_TRIGGER && tobj)
         {if (!SCRIPT(tobj))
             {ZONE_ERROR("Attempt to give variable to scriptless object");
             }
          else
             add_var(&(SCRIPT(tobj)->global_vars), ZCMD.sarg1, ZCMD.sarg2,
                                                   ZCMD.arg3);
          last_cmd = 1;
         }
      else
      if (ZCMD.arg1==WLD_TRIGGER)
         {if (ZCMD.arg2<0 || ZCMD.arg2>top_of_world)
             {ZONE_ERROR("Invalid room number in variable assignment");
             }
          else
             {if (!(world[ZCMD.arg2].script))
                 {ZONE_ERROR("Attempt to give variable to scriptless object");
                 }
              else
                 add_var(&(world[ZCMD.arg2].script->global_vars),
                           ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg3);
              last_cmd = 1;
             }
         }
      break;

    default:
      ZONE_ERROR("unknown cmd in reset table; cmd disabled");
      ZCMD.command = '*';
      break;
          }
      }

  zone_table[zone].age = 0;

  /* handle reset_wtrigger's */
  log("[Reset] Triggers");
  room_vnum = zone_table[zone].number * 100;
  while (room_vnum <= zone_table[zone].top)
        {room_rnum = real_room(room_vnum);
         if (room_rnum != NOWHERE)
            reset_wtrigger(&world[room_rnum]);
         room_vnum++;
        }
  log("[Reset] Paste mobiles");
  paste_mobiles(zone);
  log("[Reset] Stop zone %s",zone_table[zone].name);
}


void paste_mobiles(int zone)
{struct char_data *ch,  *ch_next;
 struct obj_data  *obj, *obj_next;
 int    time_ok, month_ok, need_move, no_month, no_time, room = -1;

 for (ch = character_list; ch; ch = ch_next)
     {ch_next = ch->next;
      if (!IS_NPC(ch))
         continue;
      if (FIGHTING(ch))
         continue;
      if (GET_POS(ch) < POS_STUNNED)
         continue;
      if (AFF_FLAGGED(ch,AFF_CHARM) ||
          AFF_FLAGGED(ch,AFF_HORSE) ||
          AFF_FLAGGED(ch,AFF_HOLD))
         continue;
      if (MOB_FLAGGED(ch, MOB_CORPSE))
         continue;
      if ((room = IN_ROOM(ch)) == NOWHERE)
         continue;
      if (zone >= 0 && world[room].zone != zone)
         continue;
        
      time_ok  = FALSE;
      month_ok = FALSE;
      need_move= FALSE;
      no_month = TRUE;
      no_time  = TRUE;

      if (MOB_FLAGGED(ch,MOB_LIKE_DAY))
         {if (weather_info.sunlight == SUN_RISE ||
              weather_info.sunlight == SUN_LIGHT
             )
             time_ok = TRUE;
          need_move = TRUE;
          no_time   = FALSE;
         }
      if (MOB_FLAGGED(ch,MOB_LIKE_NIGHT))
         {if (weather_info.sunlight == SUN_SET ||
              weather_info.sunlight == SUN_DARK
             )
             time_ok = TRUE;
          need_move = TRUE;
          no_time   = FALSE;
         }
      if (MOB_FLAGGED(ch,MOB_LIKE_FULLMOON))
         {if ((weather_info.sunlight == SUN_SET ||
               weather_info.sunlight == SUN_DARK) &&
              (weather_info.moon_day >= 12 &&
               weather_info.moon_day <= 15)
             )
             time_ok = TRUE;
          need_move = TRUE;
          no_time   = FALSE;
         }
      if (MOB_FLAGGED(ch,MOB_LIKE_WINTER))
         {if (weather_info.season == SEASON_WINTER)
             month_ok = TRUE;
          need_move = TRUE;
          no_month  = FALSE;
         }
      if (MOB_FLAGGED(ch,MOB_LIKE_SPRING))
         {if (weather_info.season == SEASON_SPRING)
             month_ok = TRUE;
          need_move = TRUE;
          no_month  = FALSE;
         }
      if (MOB_FLAGGED(ch,MOB_LIKE_SUMMER))
         {if (weather_info.season == SEASON_SUMMER)
             month_ok = TRUE;
          need_move = TRUE;
          no_month  = FALSE;
         }
      if (MOB_FLAGGED(ch,MOB_LIKE_AUTUMN))
         {if (weather_info.season == SEASON_AUTUMN)
             month_ok = TRUE;
          need_move = TRUE;
          no_month  = FALSE;
         }
      if (need_move)
         {month_ok |= no_month;
          time_ok  |= no_time;
          if (month_ok && time_ok)
             {if (world[room].number != zone_table[world[room].zone].top)
                 continue;
              if (GET_LASTROOM(ch) == NOWHERE)
                 {extract_mob(ch);
                  continue;
                 }
              char_from_room(ch);
              char_to_room(ch,GET_LASTROOM(ch));
              //log("Put %s at room %d",GET_NAME(ch),world[IN_ROOM(ch)].number);
             }
          else
             {if (world[room].number == zone_table[world[room].zone].top)
                 continue;
              GET_LASTROOM(ch) = room;
              char_from_room(ch);
              room = real_room(zone_table[world[room].zone].top);
              if (room == NOWHERE)
                 room = GET_LASTROOM(ch);
              char_to_room(ch,room);
              //log("Remove %s at room %d",GET_NAME(ch),world[IN_ROOM(ch)].number);
             }
         }
     }

 for (obj = object_list; obj; obj = obj_next)
     {obj_next = obj->next;
      if (obj->carried_by || obj->worn_by ||
          (room = obj->in_room) == NOWHERE)
         continue;
      if (zone >= 0 && world[room].zone != zone)
         continue;
      time_ok  = FALSE;
      month_ok = FALSE;
      need_move= FALSE;
      no_time  = TRUE;
      no_month = TRUE;
      if (OBJ_FLAGGED(obj,ITEM_DAY))
         {if (weather_info.sunlight == SUN_RISE ||
              weather_info.sunlight == SUN_LIGHT
             )
             time_ok = TRUE;
          need_move = TRUE;
          no_time   = FALSE;
         }
      if (OBJ_FLAGGED(obj,ITEM_NIGHT))
         {if (weather_info.sunlight == SUN_SET ||
              weather_info.sunlight == SUN_DARK
             )
             time_ok = TRUE;
          need_move = TRUE;
          no_time   = FALSE;
         }
      if (OBJ_FLAGGED(obj,ITEM_FULLMOON))
         {if ((weather_info.sunlight == SUN_SET ||
               weather_info.sunlight == SUN_DARK) &&
              (weather_info.moon_day >= 12 &&
               weather_info.moon_day <= 15)
             )
             time_ok = TRUE;
          need_move = TRUE;
          no_time   = FALSE;
         }
      if (OBJ_FLAGGED(obj,ITEM_WINTER))
         {if (weather_info.season == SEASON_WINTER)
             month_ok = TRUE;
          need_move = TRUE;
          no_month  = FALSE;
         }
      if (OBJ_FLAGGED(obj,ITEM_SPRING))
         {if (weather_info.season == SEASON_SPRING)
             month_ok = TRUE;
          need_move = TRUE;
          no_month  = FALSE;
         }
      if (OBJ_FLAGGED(obj,ITEM_SUMMER))
         {if (weather_info.season == SEASON_SUMMER)
             month_ok = TRUE;
          need_move = TRUE;
          no_month  = FALSE;
         }
      if (OBJ_FLAGGED(obj,ITEM_AUTUMN))
         {if (weather_info.season == SEASON_AUTUMN)
             month_ok = TRUE;
          need_move = TRUE;
          no_month  = FALSE;
         }
      if (need_move)
         {month_ok |= no_month;
          time_ok  |= no_time;
          if (month_ok && time_ok)
             {if (world[room].number != zone_table[world[room].zone].top)
                 continue;
              if (OBJ_GET_LASTROOM(obj) == NOWHERE)
                 {extract_obj(obj);
                  continue;
                 }
              obj_from_room(obj);
              obj_to_room(obj,OBJ_GET_LASTROOM(obj));
              //log("Put %s at room %d",obj->PNames[0],world[OBJ_GET_LASTROOM(obj)].number);
             }
          else
             {if (world[room].number == zone_table[world[room].zone].top)
                 continue;
              OBJ_GET_LASTROOM(obj) = room;
              obj_from_room(obj);
              room = real_room(zone_table[world[room].zone].top);
              if (room == NOWHERE)
                 room = OBJ_GET_LASTROOM(obj);
              obj_to_room(obj,room);
              //log("Remove %s at room %d",GET_NAME(ch),world[room].number);
             }
         }
     }
}

/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(zone_rnum zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next)
      {if (STATE(i) != CON_PLAYING)
          continue;
       if (IN_ROOM(i->character) == NOWHERE)
          continue;
       if (GET_LEVEL(i->character) >= LVL_IMMORT)
          continue;
       if (world[i->character->in_room].zone != zone_nr)
          continue;

       return (0);
      }
        

  return (1);
}

int mobs_in_room(int m_num, int r_num)
{struct char_data *ch;
 int    count = 0;

 for (ch = world[r_num].people; ch; ch = ch->next_in_room)
     if (m_num == GET_MOB_RNUM(ch))
        count++;

 return count;
}


/*************************************************************************
*  stuff related to the save/load player system                          *
*************************************************************************/

long cmp_ptable_by_name(char *name, int len)
{ int i;

  len = MIN(len,strlen(name));
  one_argument(name, arg);
  for (i = 0; i <= top_of_p_table; i++)
      if (!strn_cmp(player_table[i].name, arg, MIN(len,strlen(player_table[i].name))))
         return (i);
  return (-1);
}



long get_ptable_by_name(char *name)
{
  int i;

  one_argument(name, arg);
  for (i = 0; i <= top_of_p_table; i++)
      if (!str_cmp(player_table[i].name, arg))
         return (i);
  sprintf(buf,"Char %s(%s) not found !!!",name,arg);
  mudlog(buf, CMP, LVL_GOD, FALSE);
  return (-1);
}


long get_id_by_name(char *name)
{
  int i;

  one_argument(name, arg);
  for (i = 0; i <= top_of_p_table; i++)
      if (!str_cmp(player_table[i].name, arg))
         return (player_table[i].id);

  return (-1);
}


char *get_name_by_id(long id)
{ int i;

  for (i = 0; i <= top_of_p_table; i++)
      if (player_table[i].id == id)
         return (player_table[i].name);

  return (NULL);
}

void delete_unique(struct char_data *ch)
{int i;

 if ((i = get_id_by_name(GET_NAME(ch))) >= 0)
    player_table[i].unique = GET_UNIQUE(ch) = -1;

}

int correct_unique(int unique)
{int i;

 for (i = 0; i <= top_of_p_table; i++)
     if (player_table[i].unique == unique)
        return (TRUE);

 return (FALSE);
}

int create_unique(void)
{int unique;

 do {unique = (number(0,64) << 24) + (number(0,255) << 16) +
              (number(0,255) << 8) + (number(0,255));
    } while (correct_unique(unique));
 return (unique);
}

struct pkiller_file_u
{int  unique;
 int  pkills;
};

int dec_pk_values(struct char_data *ch, struct char_data *victim,
                  int pkills, int prevenge)
{struct PK_Memory_type *pk, *pk_next, *temp;

 for (pk = ch->pk_list; pk; pk = pk_next)
     {pk_next = pk->next;
      if (pk->unique == GET_UNIQUE(victim))
         {pk->pkills   -= MIN(pk->pkills, pkills);
          if (pkills > 0)
             {act("Вы использовали право мести $N2.", FALSE, victim, 0, ch, TO_CHAR);
              act("$N отомстил$G Вам.", FALSE, ch, 0, victim, TO_CHAR);
             }
          pk->revenge -= MIN(pk->revenge, prevenge);
          if (!pk->pkills)
             {act("Вы больше не можете мстить $N2.", FALSE, victim, 0, ch, TO_CHAR);
              REMOVE_FROM_LIST(pk, ch->pk_list, next);
              free(pk);
              return (1);
             }
          if (pkills > 0)
             return (1);
          else
             return (0);
         }
     }
 return (0);
}

#define KILLER_UNRENTABLE  30
#define REVENGE_UNRENTABLE 10
#define SPAM_PK_TIME       30*60
#define TIME_GODS_CURSE    96
#define MAX_PKILL_FOR_PERIOD 3

int inc_pk_values(struct char_data *ch, struct char_data *victim,
                  int pkills, int prevenge)
{struct PK_Memory_type *pk;
 int    count=0,count_pk=0,spam_pk=0;
 struct PK_Memory_type *pktmp;
 long   pk_time,pk1,pk2,pk3,pk4,pk5;
 
 if (IS_IMMORTAL(ch) || IS_NPC(victim))
    return (PK_OK);
//    
 if (House_name(ch) && 
     House_rank(ch) && 
     House_name(victim) && 
     House_rank(victim)
    ) 
    {act("Не обижайтесь на $N3 - по всем вопросам отношений дружин обращайтесь к Воеводе вашей дружины !", FALSE, victim, 0, ch, TO_CHAR);
     act("Вы уверены что Воевода вашей дружины одобрит нападение на $N3 !", FALSE, ch, 0, victim, TO_CHAR);
     return (PK_OK);
    }
//
// Добавлено Дажьбогом
     if (PLR_FLAGGED(victim,PLR_KILLER) == PLR_KILLER)
    {act("Боги помогают $N2 в охоте на вас !", FALSE, victim, 0, ch, TO_CHAR);
     act("Боги дали Вам разрешение на уничтожение $N2 !", FALSE, ch, 0, victim, TO_CHAR);
     return(PK_OK);
    }
 for (pktmp = ch->pk_list; pktmp; pktmp = pktmp->next)
    {
     pk_time=(long) (time(0)-SPAM_PK_TIME);
     if ( pk_time < pktmp->pkilllast )
        {spam_pk++;
         pk1=(long) (time(0)-pktmp->pkilllast);  
         pk2=(long) (time(0)-pktmp->pkilllast1);
         pk3=(long) (time(0)-pktmp->pkilllast2);
         pk4=(long) (time(0)-pktmp->pkilllast3);
         pk5=(long) (time(0)-pktmp->pkilllast4); 
        if ((pk1+pk2+pk3+pk4+pk5)<SPAM_PK_TIME)
           {SET_GOD_FLAG(ch,GF_GODSCURSE);
            GODS_DURATION(ch)=time(0)+TIME_GODS_CURSE*60*60;
            act("Боги прокляли тот день когда ты появился на свет !", FALSE, ch, 0, victim, TO_CHAR);
           }
        }
    }
    if (spam_pk>MAX_PKILL_FOR_PERIOD)
       {SET_GOD_FLAG(ch,GF_GODSCURSE);
        GODS_DURATION(ch)=time(0)+TIME_GODS_CURSE*60*60;
        act("Боги прокляли тот день когда ты появился на свет !", FALSE, ch, 0, victim, TO_CHAR);
       }
//    
 for (pk = ch->pk_list; pk; pk = pk->next, count++)
     {
// Добавлено Дажьбогом     
      count_pk=count_pk+pk->pkills;
      if ((count_pk >= MAX_PK_CHAR)&&(PLR_FLAGGED(ch,PLR_KILLER)!=PLR_KILLER))
         {SET_BIT(PLR_FLAGS(ch,PLR_KILLER), PLR_KILLER);
         }    
//
     if (pk->unique == GET_UNIQUE(victim))
        {if (pkills > 0 && time(NULL) - MAX_PKILL_LAG > pk->pkilllast)
            {act("Вы получили право еще раз отомстить $N2 !", FALSE, victim, 0, ch, TO_CHAR);
             act("$N получил$G право еще раз отомстить Вам !", FALSE, ch, 0, victim, TO_CHAR);
             pk->pkills     += pkills;
             pk->pkilllast4  = pk->pkilllast3;
             pk->pkilllast3  = pk->pkilllast2;
             pk->pkilllast2  = pk->pkilllast1;
             pk->pkilllast1  = pk->pkilllast;
             pk->pkilllast   = time(NULL);
             pk->revengelast = time(NULL);
             RENTABLE(ch)    = MAX(RENTABLE(ch), time(NULL) + KILLER_UNRENTABLE * 60);
            }
         if (prevenge > 0 && time(NULL) - MAX_REVENGE_LAG > pk->revengelast)
            {pk->revenge     += prevenge;
             pk->revengelast  = time(NULL);
             pk->pkilllast4   = pk->pkilllast3;
             pk->pkilllast3   = pk->pkilllast2;
             pk->pkilllast2   = pk->pkilllast1;
             pk->pkilllast1   = pk->pkilllast;
             pk->pkilllast    = time(NULL);
             RENTABLE(ch)     = MAX(RENTABLE(ch), time(NULL) + REVENGE_UNRENTABLE * 60);
             RENTABLE(victim) = MAX(RENTABLE(victim), time(NULL) + REVENGE_UNRENTABLE * 60);
            } 
         return (PK_OK);
        }
     }
 if (pkills <= 0)
    return (PK_OK);

 act("Вы получили право отомстить $N2 !", FALSE, victim, 0, ch, TO_CHAR);
 act("$N получил$G право на Ваш отстрел !", FALSE, ch, 0, victim, TO_CHAR);
 CREATE(pk, struct PK_Memory_type, 1);
 pk->unique      = GET_UNIQUE(victim);
 pk->pkills      = MAX(pkills, 0);
 pk->pkilllast4  = pk->pkilllast3;
 pk->pkilllast3  = pk->pkilllast2;
 pk->pkilllast2  = pk->pkilllast1;
 pk->pkilllast1  = pk->pkilllast;
 pk->pkilllast   = time(NULL);
 pk->revenge     = MAX(prevenge, 0);
 pk->pthief      = 0;
 pk->thieflast   = 0;
 pk->revengelast = time(NULL);
 pk->next        = ch->pk_list;
 ch->pk_list     = pk;
 RENTABLE(ch)    = MAX(RENTABLE(ch), time(NULL) + MAX_PTHIEF_LAG);
 return (PK_OK);
}

int inc_pk_thiefs(struct char_data *ch, struct char_data *victim)
{struct PK_Memory_type *pk;
 int    count=0;

 if (IS_IMMORTAL(ch) || IS_NPC(ch) || IS_NPC(victim))
    return (PK_OK);

 for (pk = ch->pk_list; pk; pk = pk->next, count++)
     if (pk->unique == GET_UNIQUE(victim))
        break;
        

 if (!pk)
    {act("$N получил$G право на Ваш отстрел !", FALSE, ch, 0, victim, TO_CHAR);
     CREATE(pk, struct PK_Memory_type, 1);
     pk->unique      = GET_UNIQUE(victim);
     pk->pkills      = 0;
     pk->revenge     = 0;
     pk->next        = ch->pk_list;
     ch->pk_list     = pk;
    }
 else
    act("$N продлил$G право на Ваш отстрел !", FALSE, ch, 0, victim, TO_CHAR);
 pk->pthief      = 1;
 pk->pkilllast   = time(NULL);
 pk->revengelast = time(NULL);
 pk->thieflast   = time(NULL);
 RENTABLE(ch)    = MAX(RENTABLE(ch), time(NULL) + MAX_PTHIEF_LAG);
 sprintf(buf,"Попытка кражи : %s->%s",GET_NAME(ch),GET_NAME(victim));
 mudlog(buf, CMP, LVL_GOD, FALSE);
 return (PK_OK);
}


/* Load a char, TRUE if loaded, FALSE if not */
void load_pkills(struct char_data * ch)
{int i;
 struct pkiller_file_u pk_data;
 struct PK_Memory_type *pk_one = NULL;

 if (USE_SINGLE_PLAYER)
     {new_load_pkills(ch);
      return;
     }


 /**** read pkiller list for char */
 fseek(pkiller_fl, GET_PFILEPOS(ch) * sizeof(struct pkiller_file_u) * MAX_PKILLER_MEM, SEEK_SET);
 for (i=0; i < MAX_PKILLER_MEM; i++)
     {fread(&pk_data, sizeof(struct pkiller_file_u), 1, pkiller_fl);
      if (pk_data.unique == -1 || !correct_unique(pk_data.unique))
         break;
      for (pk_one = ch->pk_list; pk_one; pk_one = pk_one->next)
          if (pk_one->unique == pk_data.unique)
             break;
      if (pk_one)
         {log("SYSERROR : duplicate entry pkillers data for %s", GET_NAME(ch));
          continue;
         }
      // log("SYSINFO : load pkill one for %s", GET_NAME(ch));
      CREATE(pk_one, struct PK_Memory_type, 1);
      pk_one->unique = pk_data.unique;
      pk_one->pkills = pk_data.pkills;
      pk_one->pkilllast   = 0;
      pk_one->revenge     = 0;
      pk_one->revengelast = 0;
      pk_one->next   = ch->pk_list;
      ch->pk_list    = pk_one;
     };
 new_load_quests(ch);
 new_load_mkill(ch);
}


int load_char(char *name, struct char_file_u * char_element)
{
  int player_i;

  if (USE_SINGLE_PLAYER)
     {return (new_load_char(name,char_element));
     }

  if ((player_i = find_name(name)) >= 0)
     {fseek(player_fl, (long) (player_i * sizeof(struct char_file_u)), SEEK_SET);
      fread(char_element, sizeof(struct char_file_u), 1, player_fl);
      return (player_i);
     }
  else
     return (-1);
}

/*
 * write the vital data of a player to the player file
 *
 * NOTE: load_room should be an *RNUM* now.  It is converted to a vnum here.
 */

void save_pkills(struct char_data * ch)
{ struct PK_Memory_type *pk, *temp, *tpk;
  struct char_data *tch = NULL;
  struct pkiller_file_u pk_data;
  int    i;

  if (USE_SINGLE_PLAYER)
     {new_save_pkills(ch);
      return;
     }

  /**** store pkiller list for char */
  fseek(pkiller_fl, GET_PFILEPOS(ch) * sizeof(struct pkiller_file_u) * MAX_PKILLER_MEM, SEEK_SET);
  for (i=0, pk=ch->pk_list; i < MAX_PKILLER_MEM; i++)
      {if (pk && correct_unique(pk->unique))
          {if (pk->revenge >= MAX_REVENGE)
              {for (tch = character_list; tch; tch = tch->next)
                   if (!IS_NPC(tch) && GET_UNIQUE(tch) == pk->unique)
                      break;
               while (pk->pkills && pk->revenge >= MAX_REVENGE)
                     {if (pk->pkills > 0)
                         pk->pkills--;
                      else
                         pk->pkills = 0;
                      pk->revenge   = MAX_REVENGE;
                      if (tch)
                         act("Вы утратили право на месть $N2", FALSE, tch, 0, ch, TO_CHAR);
                     }
               if (pk->pkills <= 0)
                  {tpk = pk->next;
                   REMOVE_FROM_LIST(pk, ch->pk_list, next);
                   free(pk);
                   pk = tpk;
                   i--;
                   continue;
                  }
              }
           pk_data.unique = pk->unique;
           pk_data.pkills = pk->pkills;
           // log("SYSINFO : Save pkill one for %s", GET_NAME(ch));
           pk = pk->next;
          }
       else
          {pk_data.unique = -1;
           pk_data.pkills = -1;
          }
       fwrite(&pk_data, sizeof(struct pkiller_file_u), 1, pkiller_fl);
      }
}

void save_char(struct char_data * ch, room_rnum load_room)
{ struct char_file_u st;

  if (IS_NPC(ch) || /* !ch->desc || */ GET_PFILEPOS(ch) < 0)
     return;

  if (USE_SINGLE_PLAYER)
     {new_save_char(ch,load_room);
      return;
     }

  char_to_store(ch, &st);

  if (ch->desc)
     {strncpy(st.host, ch->desc->host, HOST_LENGTH);
      st.host[HOST_LENGTH] = '\0';
     }
  else
     strcpy(st.host,"Unknown");

  if (!PLR_FLAGGED(ch, PLR_LOADROOM))
     {if (load_room == NOWHERE)
         st.player_specials_saved.load_room = NOWHERE;
      else
         {st.player_specials_saved.load_room = GET_ROOM_VNUM(load_room);
          GET_LOADROOM(ch)                   = GET_ROOM_VNUM(load_room);
          log("Player %s save at room %d", GET_NAME(ch), GET_ROOM_VNUM(load_room));
         }
     }

  fseek(player_fl, GET_PFILEPOS(ch) * sizeof(struct char_file_u), SEEK_SET);
  fwrite(&st, sizeof(struct char_file_u), 1, player_fl);

  save_pkills(ch);
  new_save_quests(ch);
  new_save_mkill(ch);
  save_char_vars(ch);
}

/* copy data from the file structure to a char struct */
void store_to_char(struct char_file_u * st, struct char_data * ch)
{
  int i;

  /* to save memory, only PC's -- not MOB's -- have player_specials */
  if (ch->player_specials == NULL)
     CREATE(ch->player_specials, struct player_special_data, 1);

  GET_SEX(ch)   = st->sex;
  GET_CLASS(ch) = st->chclass;
  GET_LEVEL(ch) = st->level;

  ch->player.short_descr = NULL;
  ch->player.long_descr  = NULL;
  ch->player.title       = str_dup(st->title);
  ch->player.description = str_dup(st->description);

  ch->player.hometown    = st->hometown;
  ch->player.time.birth  = st->birth;
  ch->player.time.played = st->played;
  ch->player.time.logon  = time(0);

  ch->player.weight = st->weight;
  ch->player.height = st->height;

  if (ch->player.name == NULL)
     CREATE(ch->player.name, char, strlen(st->name) + 1);
  strcpy(ch->player.name,   st->name);
  strcpy(ch->player.passwd, st->pwd);
  /*********************************************************/
  for (i=0; i < NUM_PADS; i++)
      {CREATE(GET_PAD(ch,i), char, strlen(st->PNames[i]) + 1);
       strcpy(GET_PAD(ch,i), st->PNames[i]);
      }
  GET_RACE(ch)               = st->player_specials_saved.Race;
  GET_RELIGION(ch)           = st->player_specials_saved.Religion;
  GET_LOWS(ch)               = st->player_specials_saved.Lows;
  GET_SIDE(ch)               = st->player_specials_saved.Side;

  ch->real_abils             = st->abilities;
  ch->points                 = st->points;
  ch->char_specials.saved    = st->char_specials_saved;
  ch->player_specials->saved = st->player_specials_saved;

  ch->char_specials.saved.affected_by = clear_flags;
  POOFIN(ch)                          = NULL;
  POOFOUT(ch)                         = NULL;
  LAST_LOGON(ch)                      = st->last_logon;
  GET_LAST_TELL(ch)                   = NOBODY;
  GET_MANA_NEED(ch)                   = 0;
  GET_MANA_STORED(ch)                 = 0;
  ch->char_specials.carry_weight      = 0;
  ch->char_specials.carry_items       = 0;
  ch->real_abils.armor                = 100;
  CLR_MEMORY(ch);

  /* Add all spell effects */
  for (i = 0; i < MAX_AFFECT; i++)
      if (st->affected[i].type                     &&
          st->affected[i].type != SPELL_SLEEP      &&
          st->affected[i].type != SPELL_ARMAGEDDON &&
          st->affected[i].type >  0                &&
          st->affected[i].type <= LAST_USED_SPELL
         )
         affect_to_char(ch, &st->affected[i]);

  for (i = 0; i < MAX_TIMED_SKILLS; i++)
      if (st->timed[i].time)
         timed_to_char(ch, &st->timed[i]);

  /**** Set all spells to GODS */
  if (IS_GRGOD(ch))
     {for (i = 0; i <= MAX_SPELLS; i++)
           GET_SPELL_TYPE(ch,i) = GET_SPELL_TYPE(ch,i) |
                                  SPELL_ITEMS          |
                                  SPELL_KNOW           |
                                  SPELL_RUNES          |
                                  SPELL_SCROLL         |
                                  SPELL_POTION         |
                                  SPELL_WAND;
     }
  else
  if (!IS_IMMORTAL(ch))
     {for (i = 0; i <= MAX_SPELLS; i++)
          if (spell_info[i].slot_forc[(int)GET_CLASS(ch)] == MAX_SLOT)
             REMOVE_BIT(GET_SPELL_TYPE(ch,i),SPELL_KNOW);
     }

  if (IS_GRGOD(ch))
     for (i = 1; i <= MAX_SKILLS; i++)
         GET_SKILL(ch,i) = MAX(GET_SKILL(ch,i),100);

  /*
   * If you're not poisioned and you've been away for more than an hour of
   * real time, we'll set your HMV back to full
   */


  if (!AFF_FLAGGED(ch, AFF_POISON) &&
      (((long) (time(0) - st->last_logon)) >= SECS_PER_REAL_HOUR))
     { GET_HIT(ch)  = GET_REAL_MAX_HIT(ch);
       GET_MOVE(ch) = GET_REAL_MAX_MOVE(ch);
     }
  else
     GET_HIT(ch) = MIN(GET_HIT(ch), GET_REAL_MAX_HIT(ch));
}                               /* store_to_char */




/* copy vital data from a players char-structure to the file structure */
void char_to_store(struct char_data * ch, struct char_file_u * st)
{
  int    i;
  struct affected_type *af;
  struct timed_type    *timed;
  struct obj_data *char_eq[NUM_WEARS];
  /* Unaffect everything a character can be affected by */
  log("[CHAR TO STORE] Unequip char %s", GET_NAME(ch));
  for (i = 0; i < NUM_WEARS; i++)
      { if (GET_EQ(ch, i))
           {char_eq[i] = unequip_char(ch, i | 0x80 | 0x40);
            #ifndef NO_EXTRANEOUS_TRIGGERS
            remove_otrigger(char_eq[i], ch);
            #endif
           }
        else
           char_eq[i] = NULL;
      }

  log("[CHAR TO STORE] Clear affects");
  for (af = ch->affected, i=0;i < MAX_AFFECT; i++)
      {if (af)
          {if (af->type == SPELL_ARMAGEDDON ||
               af->type < 1                 ||
               af->type > LAST_USED_SPELL
              )
              i--;
           else
              {st->affected[i]      = *af;
               st->affected[i].next = 0;
              }
           af                   = af->next;
          }
       else
          {st->affected[i].type      = 0;       /* Zero signifies not used */
           st->affected[i].duration  = 0;
           st->affected[i].modifier  = 0;
           st->affected[i].location  = 0;
           st->affected[i].bitvector = 0;
           st->affected[i].next      = 0;
          }
      }

  log("[CHAR TO STORE] Clear timed");
  for (timed = ch->timed, i = 0; i < MAX_TIMED_SKILLS; i++)
      {if (timed)
          {st->timed[i]         = *timed;
           st->timed[i].next    = 0;
           timed                = timed->next;
          }
       else
          {st->timed[i].skill   = 0;
           st->timed[i].time    = 0;
           st->timed[i].next    = 0;
          }
      }

  /*
   * remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in.
   */

  log("[CHAR TO STORE] Remove affects");
  supress_godsapply = TRUE;
  while (ch->affected)
         affect_remove(ch, ch->affected);
  supress_godsapply = FALSE;

  if (ch->affected)
     {sprintf(buf,"Char %s was affected !!!",GET_NAME(ch));
      mudlog(buf, CMP, LVL_GOD, FALSE);
     }

  if ((i >= MAX_AFFECT) && af && af->next)
     log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");
  log("[CHAR TO STORE] Copy data");
  st->birth                 = ch->player.time.birth;
  st->played                = ch->player.time.played;
  st->played               += (long) (time(0) - ch->player.time.logon);
  st->last_logon = LAST_LOGON(ch) = time(0);
  ch->player.time.played    = st->played;
  ch->player.time.logon     = time(0);

  st->hometown              = ch->player.hometown;
  st->weight                = GET_WEIGHT(ch);
  st->height                = GET_HEIGHT(ch);
  st->sex                   = GET_SEX(ch);
  st->chclass               = GET_CLASS(ch);
  if (GET_TITLE(ch))
    strcpy(st->title, GET_TITLE(ch));
  else
    *st->title = '\0';
  if (ch->player.description)
    strcpy(st->description, ch->player.description);
  else
    *st->description = '\0';
  strcpy(st->name, GET_NAME(ch));
  strcpy(st->pwd, GET_PASSWD(ch));
  for (i = 0; i < 6; i++)
      strcpy(st->PNames[i],GET_PAD(ch,i));

  st->abilities                      = ch->real_abils;
  st->points                         = ch->points;
  st->level                          = GET_LEVEL(ch);
  st->char_specials_saved            = ch->char_specials.saved;
  st->player_specials_saved          = ch->player_specials->saved;

  st->player_specials_saved.Side     = GET_SIDE(ch);
  st->player_specials_saved.Religion = GET_RELIGION(ch);
  st->player_specials_saved.Race     = GET_RACE(ch);
  st->player_specials_saved.Lows     = GET_LOWS(ch);

  /* add spell and eq affections back in now */
  log("[CHAR TO STORE] Restore affects");
  for (i = 0; i < MAX_AFFECT; i++)
      if (st->affected[i].type)
         {/* sprintf(buf,"[CTS] T = %d",st->affected[i].type);
             mudlog(buf, CMP, LVL_GOD, FALSE); */
          affect_to_char(ch,&st->affected[i]);
         }

  log("[CHAR TO STORE] Reequip char");
  for (i = 0; i < NUM_WEARS; i++)
      {if (char_eq[i])
          {
           #ifndef NO_EXTRANEOUS_TRIGGERS
           if (wear_otrigger(char_eq[i], ch, i))
           #endif
           equip_char(ch, char_eq[i], i | 0x80 | 0x40);
           #ifndef NO_EXTRANEOUS_TRIGGERS
           else
           obj_to_char(char_eq[i], ch);
           #endif
          }
      }
  log("[CHAR TO STORE] Recalc affects");
  affect_total(ch);
  if ((i = get_ptable_by_name(GET_NAME(ch))) >= 0)
     {// sprintf(buf,"%s (%d) (%d) (%d)",GET_NAME(ch),i,-1,GET_LEVEL(ch));
      // mudlog(buf, CMP, LVL_GOD, FALSE);
      log("[CHAR TO STORE] Change logon time");
      player_table[i].last_logon = -1;
      player_table[i].level      = GET_LEVEL(ch);
     }
  log("[CHAR TO STORE] Stop...");
}                               /* Char to store */



void save_etext(struct char_data * ch)
{
/* this will be really cool soon */

}


/*
 * Create a new entry in the in-memory index table for the player file.
 * If the name already exists, by overwriting a deleted character, then
 * we re-use the old position.
 */
int create_entry(char *name)
{
  int i, pos, found = TRUE;

  if (top_of_p_table == -1)
     {/* no table */
      CREATE(player_table, struct player_index_element, 1);
      pos   = top_of_p_table = 0;
      found = FALSE;
     }
  else
  if ((pos = get_ptable_by_name(name)) == -1)
     {/* new name */
      i     = ++top_of_p_table + 1;
      RECREATE(player_table, struct player_index_element, i);
      pos   = top_of_p_table;
      found = FALSE;
     }

  CREATE(player_table[pos].name, char, strlen(name) + 1);

  /* copy lowercase equivalent of name to table field */
  for (i = 0, player_table[pos].name[i] = '\0';
       (player_table[pos].name[i] = LOWER(name[i])); i++);
  /* create new save activity */
  player_table[pos].activity = number(0, OBJECT_SAVE_ACTIVITY-1);
  player_table[pos].timer    = NULL;

  return (pos);
}



/************************************************************************
*  funcs of a (more or less) general utility nature                     *
************************************************************************/


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl, char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
  register char *point;
  int done = 0, length = 0, templength;

  *buf = '\0';

  do {if (!fgets(tmp, 512, fl))
         {log("SYSERR: fread_string: format error at or near %s", error);
          exit(1);
         }
      /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
      if ((point = strchr(tmp, '~')) != NULL)
         {*point = '\0';
          done = 1;
         }
      else
         {point = tmp + strlen(tmp) - 1;
          *(point++) = '\r';
          *(point++) = '\n';
          *point = '\0';
         }

      templength = strlen(tmp);

      if (length + templength >= MAX_STRING_LENGTH)
         {log("SYSERR: fread_string: string too large (db.c)");
          log(error);
          exit(1);
         }
      else
         {strcat(buf + length, tmp);
          length += templength;
         }
     } while (!done);

  /* allocate space for the new string and copy it */
  if (strlen(buf) > 0)
     {CREATE(rslt, char, length + 1);
      strcpy(rslt, buf);
     }
  else
     rslt = NULL;

  return (rslt);
}


/* release memory allocated for a char struct */
void free_char(struct char_data * ch)
{
  int    i, j, id = -1;
  struct alias_data *a;
  struct PK_Memory_type *pk, *pk_next;
  struct helper_data_type *temp;

  log("[FREE CHAR] (%s) Start",GET_NAME(ch));
  if (ch->player_specials != NULL &&
      ch->player_specials != &dummy_mob)
     {while ((a = GET_ALIASES(ch)) != NULL)
            {GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
             free_alias(a);
            }
      if (ch->player_specials->poofin)
         free(ch->player_specials->poofin);
      if (ch->player_specials->poofout)
         free(ch->player_specials->poofout);
      free(ch->player_specials);
      if (IS_NPC(ch))
         log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(ch), GET_MOB_VNUM(ch));
     };

  if (!IS_NPC(ch))
     {id = get_ptable_by_name(GET_NAME(ch));
     }

  if (!IS_NPC(ch) ||
      (IS_NPC(ch) && GET_MOB_RNUM(ch) == -1))
     {/* if this is a player, or a non-prototyped non-player, free all */
      if (GET_NAME(ch))
         free(GET_NAME(ch));

      for (j = 0; j < NUM_PADS; j++)
         if (GET_PAD(ch,j))
            free(GET_PAD(ch,j));

      if (ch->player.title)
         free(ch->player.title);

      if (ch->player.short_descr)
         free(ch->player.short_descr);

      if (ch->player.long_descr)
         free(ch->player.long_descr);

      if (ch->player.description)
         free(ch->player.description);

      if (IS_NPC(ch) && ch->mob_specials.Questor)
         free(ch->mob_specials.Questor);

      if (ch->Questing.quests)
         free(ch->Questing.quests);

      if (ch->MobKill.howmany)
         free(ch->MobKill.howmany);

      if (ch->MobKill.vnum)
         free(ch->MobKill.vnum);

      if ((pk=ch->pk_list))
         for(;pk;pk=pk_next)
            {pk_next = pk->next;
             free(pk);
            }
      while (ch->helpers)
            REMOVE_FROM_LIST(ch->helpers,ch->helpers,next_helper);
     }
  else
  if ((i = GET_MOB_RNUM(ch)) >= 0)
     {/* otherwise, free strings only if the string is not pointing at proto */
      if (ch->player.name && ch->player.name != mob_proto[i].player.name)
         free(ch->player.name);

      for (j = 0; j < NUM_PADS; j++)
         if (GET_PAD(ch,j) && (ch->player.PNames[j] != mob_proto[i].player.PNames[j]))
            free(ch->player.PNames[j]);

      if (ch->player.title && ch->player.title != mob_proto[i].player.title)
         free(ch->player.title);

      if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
         free(ch->player.short_descr);

      if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr)
         free(ch->player.long_descr);

      if (ch->player.description && ch->player.description != mob_proto[i].player.description)
         free(ch->player.description);

      if (ch->mob_specials.Questor && ch->mob_specials.Questor != mob_proto[i].mob_specials.Questor)
         free(ch->mob_specials.Questor);
     }

  supress_godsapply = TRUE;
  while (ch->affected)
        affect_remove(ch, ch->affected);
  supress_godsapply = FALSE;

  while (ch->timed)
        timed_from_char(ch, ch->timed);

  if (ch->desc)
     ch->desc->character = NULL;

  if (id >= 0)
     {player_table[id].level       = (GET_REMORT(ch) ? 30 : GET_LEVEL(ch));
      player_table[id].last_logon  = time(0);
      player_table[id].activity    = number(0, OBJECT_SAVE_ACTIVITY-1);
     }

  free(ch);
  log("FREE_CHAR] Stop");
}




/* release memory allocated for an obj struct */
void free_obj(struct obj_data * obj)
{
  int nr, i;
  struct extra_descr_data *thisd, *next_one;

  if ((nr = GET_OBJ_RNUM(obj)) == -1)
     {if (obj->name)
         free(obj->name);

      for (i = 0; i < NUM_PADS; i++)
          if (obj->PNames[i])
             free(obj->PNames[i]);

      if (obj->description)
         free(obj->description);

      if (obj->short_description)
         free(obj->short_description);

      if (obj->action_description)
         free(obj->action_description);

      if (obj->ex_description)
         for (thisd = obj->ex_description; thisd; thisd = next_one)
             {next_one = thisd->next;
              if (thisd->keyword)
                 free(thisd->keyword);
              if (thisd->description)
                 free(thisd->description);
              free(thisd);
             }
     }
  else
    {if (obj->name && obj->name != obj_proto[nr].name)
        free(obj->name);

     for (i = 0; i < NUM_PADS; i++)
         if (obj->PNames[i] && obj->PNames[i] != obj_proto[nr].PNames[i])
            free(obj->PNames[i]);

     if (obj->description && obj->description != obj_proto[nr].description)
        free(obj->description);

     if (obj->short_description && obj->short_description != obj_proto[nr].short_description)
        free(obj->short_description);

     if (obj->action_description && obj->action_description != obj_proto[nr].action_description)
        free(obj->action_description);

     if (obj->ex_description && obj->ex_description != obj_proto[nr].ex_description)
        for (thisd = obj->ex_description; thisd; thisd = next_one)
            {next_one = thisd->next;
                 if (thisd->keyword)
                    free(thisd->keyword);
                 if (thisd->description)
                    free(thisd->description);
                 free(thisd);
            }
    }
  free(obj);
}


/*
 * Steps:
 *   1: Make sure no one is using the pointer in paging.
 *   2: Read contents of a text file.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * str_dup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 */
int file_to_string_alloc(const char *name, char **buf)
{
  char temp[MAX_EXTEND_LENGTH];
  struct descriptor_data *in_use;

  for (in_use = descriptor_list; in_use; in_use = in_use->next)
      if (in_use->showstr_vector && *in_use->showstr_vector == *buf)
         return (-1);

  /* Lets not free() what used to be there unless we succeeded. */
  if (file_to_string(name, temp) < 0)
      return (-1);

  if (*buf)
     free(*buf);

  *buf = str_dup(temp);
  return (0);
}


/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char *buf)
{
  FILE *fl;
  char tmp[READ_SIZE+3];

  *buf = '\0';

  if (!(fl = fopen(name, "r")))
     {log("SYSERR: reading %s: %s", name, strerror(errno));
      return (-1);
     }
  do {fgets(tmp, READ_SIZE, fl);
      tmp[strlen(tmp) - 1] = '\0'; /* take off the trailing \n */
      strcat(tmp, "\r\n");

      if (!feof(fl))
         {if (strlen(buf) + strlen(tmp) + 1 > MAX_EXTEND_LENGTH)
             {log("SYSERR: %s: string too big (%d max)", name,
                          MAX_STRING_LENGTH);
                  *buf = '\0';
                  return (-1);
             }
          strcat(buf, tmp);
         }
     } while (!feof(fl));

  fclose(fl);

  return (0);
}



/* clear some of the the working variables of a char */
void reset_char(struct char_data * ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
      GET_EQ(ch, i) = NULL;
  memset((void *) &ch->add_abils, 0, sizeof(struct char_played_ability_data));

  ch->followers     = NULL;
  ch->master        = NULL;
  ch->in_room       = NOWHERE;
  ch->carrying      = NULL;
  ch->next          = NULL;
  ch->next_fighting = NULL;
  ch->next_in_room  = NULL;
  ch->Protecting    = NULL;
  ch->Touching      = NULL;
  ch->BattleAffects = clear_flags;
  ch->Poisoner      = 0;
  FIGHTING(ch)      = NULL;
  ch->char_specials.position     = POS_STANDING;
  ch->mob_specials.default_pos   = POS_STANDING;
  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items  = 0;

  if (GET_HIT(ch) <= 0)
     GET_HIT(ch) = 1;
  if (GET_MOVE(ch) <= 0)
     GET_MOVE(ch) = 1;
  GET_MANA_NEED(ch)   = 0;
  GET_MANA_STORED(ch) = 0;
  CLR_MEMORY(ch);
  GET_CASTER(ch)    = 0;
  GET_DAMAGE(ch)    = 0;
  GET_LAST_TELL(ch) = NOBODY;
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data * ch)
{
  memset((char *) ch, 0, sizeof(struct char_data));
  ch->in_room      = NOWHERE;
  GET_PFILEPOS(ch) = -1;
  GET_MOB_RNUM(ch) = NOBODY;
  GET_WAS_IN(ch)   = NOWHERE;
  GET_POS(ch)      = POS_STANDING;
  GET_CASTER(ch)   = 0;
  GET_DAMAGE(ch)   = 0;
  CLR_MEMORY(ch);
  ch->Poisoner     = 0;
  ch->mob_specials.default_pos = POS_STANDING;
}


void clear_object(struct obj_data * obj)
{
  memset((char *) obj, 0, sizeof(struct obj_data));

  obj->item_number      = NOTHING;
  obj->in_room          = NOWHERE;
  obj->worn_on          = NOWHERE;
  GET_OBJ_ZONE(obj)     = NOWHERE;
  OBJ_GET_LASTROOM(obj) = NOWHERE;
}




/* initialize a new character only if class is set */
void init_char(struct char_data * ch)
{
  int i, start_room = NOWHERE;

  /* create a player_special structure */
  if (ch->player_specials == NULL)
     CREATE(ch->player_specials, struct player_special_data, 1);

  /* *** if this is our first player --- he be God *** */

  if (top_of_p_table == 0)
     { GET_EXP(ch)         = EXP_IMPL;
       GET_LEVEL(ch)       = LVL_IMPL;

       ch->points.max_hit  = 500;
       ch->points.max_move = 82;
       start_room          = immort_start_room;
     }
  else
     switch(GET_RACE(ch))
     { case CLASS_SEVERANE: start_room = START_ROOM; break;
       case CLASS_POLANE:   start_room = START_ROOM; break;
       case CLASS_KRIVICHI: start_room = START_ROOM; break;
       case CLASS_VATICHI:  start_room = START_ROOM; break;
       case CLASS_VELANE:   start_room = START_ROOM; break;
       case CLASS_DREVLANE: start_room = START_ROOM; break;
     }
  set_title(ch, NULL);
  ch->player.short_descr = NULL;
  ch->player.long_descr  = NULL;
  ch->player.description = NULL;
  ch->player.hometown    = 1;
  ch->player.time.birth  = time(0);
  ch->player.time.played = 0;
  ch->player.time.logon  = time(0);

  for (i = 0; i < MAX_TONGUE; i++)
      GET_TALK(ch, i) = 0;

  /* make favors for sex */
  if (ch->player.sex == SEX_MALE)
     {ch->player.weight = number(120, 180);
      ch->player.height = number(160, 200);
     }
  else
     {ch->player.weight = number(100, 160);
      ch->player.height = number(150, 180);
     }

  ch->points.hit      = GET_MAX_HIT(ch);
  ch->points.max_move = 82;
  ch->points.move     = GET_MAX_MOVE(ch);
  ch->real_abils.armor= 100;

  if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
     { player_table[i].id         = GET_IDNUM(ch)  = ++top_idnum;
       player_table[i].unique     = GET_UNIQUE(ch) = create_unique();
       player_table[i].level      = 0;
       player_table[i].last_logon = -1;
     }
  else
     {log("SYSERR: init_char: Character '%s' not found in player table.", GET_NAME(ch));
     }

  for (i = 1; i <= MAX_SKILLS; i++)
      { if (GET_LEVEL(ch) < LVL_GRGOD)
           SET_SKILL(ch, i, 0);
        else
           SET_SKILL(ch, i, 100);
      }
  for (i=1; i <= MAX_SPELLS; i++)
      { if (GET_LEVEL(ch) < LVL_GRGOD)
           GET_SPELL_TYPE(ch, i) = 0;
        else
           GET_SPELL_TYPE(ch, i) = SPELL_KNOW;
      }

  ch->char_specials.saved.affected_by = clear_flags;
  for (i = 0; i < 6; i++)
      GET_SAVE(ch, i) = 0;

  ch->real_abils.intel = 25;
  ch->real_abils.wis   = 25;
  ch->real_abils.dex   = 25;
  ch->real_abils.str   = 25;
  ch->real_abils.con   = 25;
  ch->real_abils.cha   = 25;
  ch->real_abils.size  = 50;

  if (GET_LEVEL(ch) != LVL_IMPL)
     roll_real_abils(ch);

  for (i = 0; i < 3; i++)
      {GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : i == DRUNK ? 0 : 24);}
  GET_LOADROOM(ch) = start_room;
  SET_BIT(PLR_FLAGS(ch,PLR_LOADROOM), PLR_LOADROOM);
  SET_BIT(PRF_FLAGS(ch,PRF_DISPHP), PRF_DISPHP);
  SET_BIT(PRF_FLAGS(ch,PRF_DISPMANA), PRF_DISPMANA);
  SET_BIT(PRF_FLAGS(ch,PRF_DISPEXITS), PRF_DISPEXITS);
  SET_BIT(PRF_FLAGS(ch,PRF_DISPMOVE), PRF_DISPMOVE);
  SET_BIT(PRF_FLAGS(ch,PRF_DISPEXP), PRF_DISPMOVE);
  SET_BIT(PRF_FLAGS(ch,PRF_SUMMONABLE), PRF_SUMMONABLE);
  save_char(ch, NOWHERE);
}

const char *remort_msg =
"  Если Вы так настойчивы в желании начать все заново -\r\n"
"наберите <перевоплотиться> полностью.\r\n";

ACMD(do_remort)
{ char filename[MAX_INPUT_LENGTH];
  int  i, load_room = NOWHERE;
  struct obj_data *obj, *nobj;
  struct helper_data_type *temp;

  if (IS_NPC(ch) || IS_IMMORTAL(ch))
     {send_to_char("Вам это, похоже, совсем ни к чему.\r\n",ch);
      return;
     }
  if (!GET_GOD_FLAG(ch, GF_REMORT))
     {send_to_char("ЧАВО ???\r\n",ch);
      return;
     }
  if (!subcmd)
     {send_to_char(remort_msg,ch);
      return;
     }

  log("Remort %s", GET_NAME(ch));
  get_filename(GET_NAME(ch),filename,PQUESTS_FILE);
  remove(filename);
//  get_filename(GET_NAME(ch), filename, PMKILL_FILE);
//  remove(filename);

  GET_REMORT(ch)++;
  CLR_GOD_FLAG(ch, GF_REMORT);
  GET_STR(ch) += 1;
  GET_CON(ch) += 1;
  GET_DEX(ch) += 1;
  GET_INT(ch) += 1;
  GET_WIS(ch) += 1;
  GET_CHA(ch) += 1;

  if (FIGHTING(ch))
     stop_fighting(ch,TRUE);

  die_follower(ch);

  ch->Questing.count = 0;
  ch->MobKill.count  = 0;
  if (ch->Questing.quests)
     {free(ch->Questing.quests);
      ch->Questing.quests = 0;
     }
/*  if (ch->MobKill.howmany)
     {free(ch->MobKill.howmany);
      ch->MobKill.howmany = 0;
     }
  if (ch->MobKill.vnum)
     {free(ch->MobKill.vnum);
      ch->MobKill.vnum = 0;
     }*/

  while (ch->helpers)
        REMOVE_FROM_LIST(ch->helpers,ch->helpers,next_helper);
        
  supress_godsapply = TRUE;
  while (ch->affected)
        affect_remove(ch, ch->affected);
  supress_godsapply = FALSE;

  for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch,i))
         extract_obj(unequip_char(ch,i));
  for (obj = ch->carrying; obj; obj = nobj)
      {nobj = obj->next_content;
       extract_obj(obj);
      }

  while (ch->timed)
        timed_from_char(ch, ch->timed);
  for (i = 1; i <= MAX_SKILLS; i++)
      SET_SKILL(ch, i, 0);
  for (i = 1; i <= MAX_SPELLS; i++)
      {GET_SPELL_TYPE(ch, i) = (GET_CLASS(ch) == CLASS_DRUID ? SPELL_RUNES : 0);
       GET_SPELL_MEM(ch, i)  = 0;
      }
  GET_HIT(ch)       = GET_MAX_HIT(ch)     = 10;
  GET_MOVE(ch)      = GET_MAX_MOVE(ch)    = 82;
  GET_MANA_NEED(ch) = GET_MANA_STORED(ch) = 0;
  GET_LEVEL(ch)     = 0;
  GET_WIMP_LEV(ch)  = 0;
  GET_GOLD(ch)      = GET_BANK_GOLD(ch) = 0;
  GET_AC(ch)        = 100;
  GET_LOADROOM(ch)  = calc_loadroom(ch);
  SET_BIT(PLR_FLAGS(ch,PLR_LOADROOM), PLR_LOADROOM);
  SET_BIT(PRF_FLAGS(ch,PRF_SUMMONABLE), PRF_SUMMONABLE);
  REMOVE_BIT(PRF_FLAGS(ch,PRF_AWAKE), PRF_AWAKE);
  REMOVE_BIT(PRF_FLAGS(ch,PRF_PUNCTUAL), PRF_PUNCTUAL);

  do_start(ch,FALSE);
  save_char(ch, NOWHERE);
  if (PLR_FLAGGED(ch, PLR_HELLED))
     load_room = r_helled_start_room;
  else
  if (PLR_FLAGGED(ch, PLR_NAMED))
     load_room = r_named_start_room;
  else
  if (PLR_FLAGGED(ch, PLR_FROZEN))
     load_room = r_frozen_start_room;
  else
     {if ((load_room = GET_LOADROOM(ch)) == NOWHERE)
         load_room = calc_loadroom(ch);
      load_room = real_room(load_room);
     }
  if (load_room == NOWHERE)
     {if (GET_LEVEL(ch) >= LVL_IMMORT)
         load_room = r_immort_start_room;
      else
         load_room = r_mortal_start_room;
     }
  char_from_room(ch);
  char_to_room(ch, load_room);
  look_at_room(ch, 0);
  act("$n вступил$g в игру.", TRUE, ch, 0, 0, TO_ROOM);
  act("Вы перевоплотились ! Желаем удачи !",FALSE, ch, 0, 0, TO_CHAR);
}


/* returns the real number of the room with given Virtual number */
room_rnum real_room(room_vnum vnum)
{
  room_rnum bot, top, mid;

  bot = 0;
  top = top_of_world;
  /* perform binary search on world-table */
  for (;;)
  { mid = (bot + top) / 2;

    if ((world + mid)->number == vnum)
      return (mid);
    if (bot >= top)
      return (NOWHERE);
    if ((world + mid)->number > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}



/* returns the real number of the monster with given Virtual number */
mob_rnum real_mobile(mob_vnum vnum)
{
  mob_rnum bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /* perform binary search on mob-table */
  for (;;)
  { mid = (bot + top) / 2;

    if ((mob_index + mid)->vnum == vnum)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((mob_index + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}



/* returns the real number of the object with given Virtual number */
obj_rnum real_object(obj_vnum vnum)
{
  obj_rnum bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /* perform binary search on obj-table */
  for (;;)
  { mid = (bot + top) / 2;

    if ((obj_index + mid)->vnum == vnum)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((obj_index + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */
int check_object(struct obj_data *obj)
{
  int error = FALSE;

  if (GET_OBJ_WEIGHT(obj) < 0 && (error = TRUE))
     log("SYSERR: Object #%d (%s) has negative weight (%d).",
             GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_WEIGHT(obj));

  if (GET_OBJ_RENT(obj) < 0 && (error = TRUE))
     log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
             GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_RENT(obj));

  sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf);
  if (strstr(buf, "UNDEFINED") && (error = TRUE))
      log("SYSERR: Object #%d (%s) has unknown wear flags.",
              GET_OBJ_VNUM(obj), obj->short_description);

  sprintbits(obj->obj_flags.extra_flags, extra_bits, buf, ",");
  if (strstr(buf, "UNDEFINED") && (error = TRUE))
      log("SYSERR: Object #%d (%s) has unknown extra flags.",
              GET_OBJ_VNUM(obj), obj->short_description);

  sprintbits(obj->obj_flags.affects, affected_bits, buf, ",");

  if (strstr(buf, "UNDEFINED") && (error = TRUE))
     log("SYSERR: Object #%d (%s) has unknown affection flags.",
             GET_OBJ_VNUM(obj), obj->short_description);

  switch (GET_OBJ_TYPE(obj))
  {
  case ITEM_DRINKCON:
  { char onealias[MAX_INPUT_LENGTH], *space = strchr(obj->name, ' ');

    int offset = space ? space - obj->name : strlen(obj->name);

    strncpy(onealias, obj->name, offset);
    onealias[offset] = '\0';

    if (search_block(onealias, drinknames, TRUE) < 0 && (error = TRUE))
       log("SYSERR: Object #%d (%s) doesn't have drink type as first alias. (%s)",
                   GET_OBJ_VNUM(obj), obj->short_description, obj->name);
  }
  /* Fall through. */
  case ITEM_FOUNTAIN:
    if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE))
       log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
                   GET_OBJ_VNUM(obj), obj->short_description,
                   GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 1);
    error |= check_object_spell_number(obj, 2);
    error |= check_object_spell_number(obj, 3);
    break;
  case ITEM_BOOK:
    error |= check_object_spell_number(obj, 1);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 3);
    if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1) && (error = TRUE))
       log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
                   GET_OBJ_VNUM(obj), obj->short_description,
                   GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
    break;
  }

  return (error);
}

int check_object_spell_number(struct obj_data *obj, int val)
{
  int   error = FALSE;
  const char *spellname;

  if (GET_OBJ_VAL(obj, val) == -1)      /* i.e.: no spell */
     return (error);

  /*
   * Check for negative spells, spells beyond the top define, and any
   * spell which is actually a skill.
   */
  if (GET_OBJ_VAL(obj, val) < 0)
     error = TRUE;
  if (GET_OBJ_VAL(obj, val) > MAX_SPELLS)
     error = TRUE;
  if (error)
    log("SYSERR: Object #%d (%s) has out of range spell #%d.",
            GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

  /*
   * This bug has been fixed, but if you don't like the special behavior...
   */
#if 0
  if (GET_OBJ_TYPE(obj) == ITEM_STAFF &&
      HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS | MAG_MASSES))
     log("... '%s' (#%d) uses %s spell '%s'.",
         obj->short_description,        GET_OBJ_VNUM(obj),
         HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS) ? "area" : "mass",
         spell_name(GET_OBJ_VAL(obj, val)));
#endif

  if (scheck)           /* Spell names don't exist in syntax check mode. */
     return (error);

  /* Now check for unnamed spells. */
  spellname = spell_name(GET_OBJ_VAL(obj, val));

  if ((spellname == unused_spellname || !str_cmp("UNDEFINED", spellname)) && (error = TRUE))
     log("SYSERR: Object #%d (%s) uses '%s' spell #%d.",
                 GET_OBJ_VNUM(obj), obj->short_description, spellname,
                 GET_OBJ_VAL(obj, val));
  return (error);
}

int check_object_level(struct obj_data *obj, int val)
{
  int error = FALSE;

  if ((GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > LVL_IMPL) && (error = TRUE))
     log("SYSERR: Object #%d (%s) has out of range level #%d.",
         GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));
  return (error);
}

void entrycount(char *name)
{ FILE *player_file;
  int  i;
  long size;
  struct char_file_u dummy;
  char filename[MAX_STRING_LENGTH];

  log("Entry count %s", name);
  if (get_filename(name,filename,PLAYERS_FILE) &&
      (player_file = fopen(filename,"r+b")))
     {fseek(player_file, 0L, SEEK_END);
      size = ftell(player_file);
      if (size != sizeof(struct char_file_u))
         {log("WARNING:  PLAYERFILE %s IS PROBABLY CORRUPT!",name);
          fclose(player_file);
          return;
         }

      fseek(player_file, 0L, SEEK_SET);
      fread(&dummy, sizeof(struct char_file_u), 1, player_file);
      if (!feof(player_file))
         {/* new record */
          if (player_table)
             RECREATE(player_table, struct player_index_element, top_of_p_table+2);
          else
             CREATE(player_table, struct player_index_element, 1);
          top_of_p_file++;
          top_of_p_table++;

          CREATE(player_table[top_of_p_table].name, char, strlen(dummy.name) + 1);
          for (i = 0, player_table[top_of_p_table].name[i] = '\0';
               (player_table[top_of_p_table].name[i] = LOWER(dummy.name[i])); i++);
          player_table[top_of_p_table].id     = dummy.char_specials_saved.idnum;
          player_table[top_of_p_table].unique = dummy.player_specials_saved.unique;
          player_table[top_of_p_table].level  = (dummy.player_specials_saved.Remorts ?
                                                 30 : dummy.level);
          player_table[top_of_p_table].timer  = NULL;
          if (IS_SET(GET_FLAG(dummy.char_specials_saved.act, PLR_DELETED), PLR_DELETED))
             {player_table[top_of_p_table].last_logon = -1;
              player_table[top_of_p_table].activity   = -1;
             }
          else
             {player_table[top_of_p_table].last_logon = dummy.last_logon;
              player_table[top_of_p_table].activity   = number(0, OBJECT_SAVE_ACTIVITY - 1);
             }
          top_idnum = MAX(top_idnum, dummy.char_specials_saved.idnum);
          log("Add new player %s",player_table[top_of_p_table].name);
         }
      fclose(player_file);
     }
  return;
}

void new_build_player_index(void)
{ FILE *players;
  char name[MAX_INPUT_LENGTH], playername[MAX_INPUT_LENGTH];
  int  c;

  player_table  = NULL;
  top_of_p_file = top_of_p_table = -1;
  if (!(players = fopen(LIB_PLRS"players.lst","r")))
     {log("Players list empty...");
      return;
     }

  while (get_line(players,name))
        {if (!*name || *name == ';')
            continue;
         if (sscanf(name, "%s ", playername) == 0)
            continue;
         for (c = 0; c < top_of_p_table; c++)
             if (!str_cmp(playername, player_table[c].name))
                break;
         if (c < top_of_p_table)
            continue;   
         entrycount(playername);
        }
  fclose(players);
}

void flush_player_index(void)
{ FILE *players;
  char name[MAX_STRING_LENGTH];
  int  i;

  if (!(players = fopen(LIB_PLRS"players.lst","w+")))
     {log("Cann't save players list...");
      return;
     }
  for (i = 0; i <= top_of_p_table; i++)
      {if (!player_table[i].name || !*player_table[i].name)
          continue;
        
       // check double
       // for (c = 0; c < i; c++)
       //     if (!str_cmp(player_table[c].name, player_table[i].name))
       //         break;
       // if (c < i)
       //    continue;  
        
       sprintf(name,"%s %ld %ld %d %d\n",
               player_table[i].name,
               player_table[i].id,
               player_table[i].unique,
               player_table[i].level,
               player_table[i].last_logon
              );
       fputs(name, players);
      }
  fclose(players);
  log("Сохранено индексов %d (считано при загрузке %d)",i,top_of_p_file+1);
}

void dupe_player_index(void)
{ FILE *players;
  char name[MAX_STRING_LENGTH];
  int  i, c;

  sprintf(name,LIB_PLRS"players.%ld",time(NULL));

  if (!(players = fopen(name,"w+")))
     {log("Cann't save players list...");
      return;
     }
  for (i = 0; i <= top_of_p_table; i++)
      {if (!player_table[i].name || !*player_table[i].name)
          continue;
        
       // check double
       for (c = 0; c < i; c++)
           if (!str_cmp(player_table[c].name, player_table[i].name))
              break;
       if (c < i)
          continue;
        
       sprintf(name,"%s %ld %ld %d %d\n",
               player_table[i].name,
               player_table[i].id,
               player_table[i].unique,
               player_table[i].level,
               player_table[i].last_logon
              );
        fputs(name, players);
       }
  fclose(players);
  log("Продублировано индексов %d (считано при загрузке %d)",i,top_of_p_file+1);
}


/* Load a char, TRUE if loaded, FALSE if not */
void new_load_pkills(struct char_data * ch)
{FILE *loaded;
 struct pkiller_file_u pk_data;
 struct PK_Memory_type *pk_one = NULL;
 char   filename[MAX_STRING_LENGTH];

 /**** read pkiller list for char */
 log("Load pkill %s", GET_NAME(ch));
 if (get_filename(GET_NAME(ch),filename,PKILLERS_FILE) &&
     (loaded = fopen(filename,"r+b")))
    {while (!feof(loaded))
          {fread(&pk_data, sizeof(struct pkiller_file_u), 1, loaded);
           if (!feof(loaded))
              {if (pk_data.unique < 0 || !correct_unique(pk_data.unique))
                  continue;
               for (pk_one = ch->pk_list; pk_one; pk_one = pk_one->next)
                   if (pk_one->unique == pk_data.unique)
                      break;
               if (pk_one)
                  {log("SYSERROR : duplicate entry pkillers data for %s", GET_NAME(ch));
                   continue;
                  }
               // log("SYSINFO : load pkill one for %s", GET_NAME(ch));
              CREATE(pk_one, struct PK_Memory_type, 1);
              pk_one->unique = pk_data.unique;
              pk_one->pkills = pk_data.pkills;
              pk_one->pkilllast   = 0;
              pk_one->revenge     = 0;
              pk_one->revengelast = 0;
              pk_one->next   = ch->pk_list;
              ch->pk_list    = pk_one;
             };
          }
     fclose(loaded);
    }
 new_load_quests(ch);
 new_load_mkill(ch);
}


void new_load_quests(struct char_data * ch)
{FILE *loaded;
 char filename[MAX_STRING_LENGTH];
 int  intload = 0;

 if (ch->Questing.quests)
    {log("QUESTING ERROR for %s - attempt load when not empty - call to Coder",
         GET_NAME(ch));
     free(ch->Questing.quests);
    }
 ch->Questing.count = 0;
 ch->Questing.quests= NULL;

 log("Load quests %s", GET_NAME(ch));
 if (get_filename(GET_NAME(ch),filename,PQUESTS_FILE) &&
     (loaded = fopen(filename,"r+b")))
    {intload = fread(&ch->Questing.count, sizeof(int), 1, loaded);
     if (ch->Questing.count && intload)
        {CREATE(ch->Questing.quests, int, (ch->Questing.count / 10L + 1) * 10L);
         intload = fread(ch->Questing.quests, sizeof(int), ch->Questing.count, loaded);
         if (intload < ch->Questing.count)
            ch->Questing.count = intload;
         //sprintf(buf,"SYSINFO: Questing file for %s loaded(%d)...",GET_NAME(ch),intload);
         //mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
        }
     else
        {//sprintf(buf,"SYSERR: Questing file for %s empty...",GET_NAME(ch));
         //mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
         ch->Questing.count = 0;
         CREATE(ch->Questing.quests, int, 10);
        }
     fclose(loaded);
    }
 else
    {//sprintf(buf,"SYSERR: Questing file not found for %s...",GET_NAME(ch));
     //mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
     CREATE(ch->Questing.quests, int, 10);
    }
}

void new_load_mkill(struct char_data * ch)
{FILE *loaded;
 char filename[MAX_STRING_LENGTH];
 int  intload = 0,i,c;

 if (ch->MobKill.howmany)
    {log("MOBKILL ERROR for %s - attempt load when not empty - call to Coder",
         GET_NAME(ch));
     free(ch->MobKill.howmany);
    }
 if (ch->MobKill.vnum)
    {log("MOBKILL ERROR for %s - attempt load when not empty - call to Coder",
         GET_NAME(ch));
     free(ch->MobKill.vnum);
    }
 ch->MobKill.count   = 0;
 ch->MobKill.howmany = NULL;
 ch->MobKill.vnum    = NULL;

 log("Load mkill %s", GET_NAME(ch));
 if (get_filename(GET_NAME(ch),filename,PMKILL_FILE) &&
     (loaded = fopen(filename,"r+b"))
    )
    {intload = fread(&ch->MobKill.count, sizeof(int), 1, loaded);
     if (ch->MobKill.count && intload)
        {CREATE(ch->MobKill.howmany, int, (ch->MobKill.count / 10L + 1) * 10L);
         CREATE(ch->MobKill.vnum,    int, (ch->MobKill.count / 10L + 1) * 10L);
         intload = fread(ch->MobKill.howmany, sizeof(int), ch->MobKill.count, loaded);
         intload = fread(ch->MobKill.vnum,    sizeof(int), ch->MobKill.count, loaded);
         if (intload < ch->MobKill.count)
            ch->MobKill.count = intload;
         for (i = c = 0; c < ch->MobKill.count; c++)
             if (*(ch->MobKill.vnum+c) > 0 && *(ch->MobKill.vnum+c) < 100000)
                {*(ch->MobKill.vnum+i)    = *(ch->MobKill.vnum+c);
                 *(ch->MobKill.howmany+i) = *(ch->MobKill.howmany+c);
                 i++;
                }
         ch->MobKill.count = i;
         //sprintf(buf,"SYSINFO: MobKill file for %s loaded (%d)...",GET_NAME(ch),i);
         //mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
        }
     else
        {//sprintf(buf,"SYSERR: MobKill file for %s empty...",GET_NAME(ch));
         //mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
         ch->MobKill.count = 0;
         CREATE(ch->MobKill.howmany, int, 10);
         CREATE(ch->MobKill.vnum,    int, 10);
        }
     fclose(loaded);
    }
 else
    {//sprintf(buf,"SYSERR: MobKill file not found for %s...",GET_NAME(ch));
     //mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
     CREATE(ch->MobKill.howmany, int, 10);
     CREATE(ch->MobKill.vnum,    int, 10);
    }
}

int new_load_char(char *name, struct char_file_u * char_element)
{ FILE  *loaded;
  int   player_i;
  char  filename[MAX_STRING_LENGTH];

  *filename = '\0';
  log("Load char %s", name);
  if ((player_i = find_name(name)) >= 0 &&
      get_filename(name,filename,PLAYERS_FILE) &&
      (loaded=fopen(filename,"r+b")))
     {fread(char_element, sizeof(struct char_file_u), 1, loaded);
      fclose(loaded);
      return (player_i);
     }
  else
     {log("Cann't load %d %s", player_i, filename);
      return (-1);
     }
}


void new_save_quests(struct char_data *ch)
{ FILE *saved;
  char filename[MAX_STRING_LENGTH];

  if (!ch->Questing.count || !ch->Questing.quests)
     {//if (!IS_IMMORTAL(ch))
      //   {sprintf(buf,"SYSERR: Questing list for %s empty...",GET_NAME(ch));
      //    mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      //   }
     }

  log("Save quests %s", GET_NAME(ch));
  if (get_filename(GET_NAME(ch),filename,PQUESTS_FILE) &&
      (saved = fopen(filename,"w+b")))
      {fwrite(&ch->Questing.count, sizeof(int), 1, saved);
       fwrite(ch->Questing.quests, sizeof(int), ch->Questing.count, saved);
       fclose(saved);
      }
}

void new_save_mkill(struct char_data *ch)
{ FILE *saved;
  char filename[MAX_STRING_LENGTH];

  if (!ch->MobKill.count || !ch->MobKill.vnum || !ch->MobKill.howmany)
     {//if (!IS_IMMORTAL(ch))
      //   {sprintf(buf,"SYSERR: MobKill list for %s empty...",GET_NAME(ch));
      //    mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      //   }
     }

  log("Save mkill %s", GET_NAME(ch));
  if (get_filename(GET_NAME(ch),filename,PMKILL_FILE) &&
      (saved = fopen(filename,"w+b")))
      {fwrite(&ch->MobKill.count, sizeof(int), 1, saved);
       fwrite(ch->MobKill.howmany, sizeof(int), ch->MobKill.count, saved);
       fwrite(ch->MobKill.vnum,    sizeof(int), ch->MobKill.count, saved);
       fclose(saved);
      }
}

void new_save_pkills(struct char_data * ch)
{ FILE   *saved;
  struct PK_Memory_type *pk, *temp, *tpk;
  struct char_data *tch = NULL;
  struct pkiller_file_u pk_data;
  char   filename[MAX_STRING_LENGTH];

  /**** store pkiller list for char */
  log("Save pkill %s", GET_NAME(ch));
  if (get_filename(GET_NAME(ch),filename,PKILLERS_FILE) &&
      (saved = fopen(filename,"w+b"))
     )
     {for (pk=ch->pk_list; pk && !PLR_FLAGGED(ch,PLR_DELETED);)
          {if (pk->pkills > 0 && correct_unique(pk->unique))
              {if (pk->revenge >= MAX_REVENGE)
                  {for (tch = character_list; tch; tch = tch->next)
                       if (!IS_NPC(tch) && GET_UNIQUE(tch) == pk->unique)
                          break;
                   while (pk->pkills && pk->revenge >= MAX_REVENGE)
                         {if (pk->pkills > 0)
                             pk->pkills--;
                          else
                            pk->pkills = 0;
                          pk->revenge   -= MAX_REVENGE;
                          if (tch)
                             act("Вы утратили право на месть $N2", FALSE, tch, 0, ch, TO_CHAR);
                         }
                   if (pk->pkills <= 0)
                      {tpk = pk->next;
                       REMOVE_FROM_LIST(pk, ch->pk_list, next);
                       free(pk);
                       pk = tpk;
                       continue;
                      }
                  }
               pk_data.unique = pk->unique;
               pk_data.pkills = pk->pkills;
               fwrite(&pk_data, sizeof(struct pkiller_file_u), 1, saved);
               // log("SYSINFO : Save pkill one for %s", GET_NAME(ch));
              }
           pk = pk->next;
          }
      fclose(saved);
     }
}

void new_save_char(struct char_data * ch, room_rnum load_room)
{ FILE   *saved;
  struct char_file_u st;
  char   filename[MAX_STRING_LENGTH];

  if (IS_NPC(ch) || /* !ch->desc || */ GET_PFILEPOS(ch) < 0)
     return;


  char_to_store(ch, &st);

  if (ch->desc)
     strncpy(st.host, ch->desc->host, HOST_LENGTH);
  else
     strcpy(st.host, "Unknown");
  st.host[HOST_LENGTH] = '\0';

  if (!PLR_FLAGGED(ch, PLR_LOADROOM))
     {if (load_room == NOWHERE)
         st.player_specials_saved.load_room = NOWHERE;
      else
         {st.player_specials_saved.load_room = GET_ROOM_VNUM(load_room);
          GET_LOADROOM(ch)                   = GET_ROOM_VNUM(load_room);
          log("Player %s save at room %d", GET_NAME(ch), GET_ROOM_VNUM(load_room));
         }
     }

  log("Save char %s", GET_NAME(ch));
  if (get_filename(GET_NAME(ch),filename,PLAYERS_FILE) &&
      (saved = fopen(filename,"w+b")))
     {fwrite(&st, sizeof(struct char_file_u), 1, saved);
      fclose(saved);
     }

  new_save_pkills(ch);
  new_save_quests(ch);
  new_save_mkill(ch);
  save_char_vars(ch);
}

void rename_char(struct char_data *ch, char *oname)
{ char filename[MAX_INPUT_LENGTH], ofilename[MAX_INPUT_LENGTH];
  FILE *saved;
  struct char_file_u st;

  char_to_store(ch, &st);

 // 1) Rename(if need) char and pkill file - directly
 log("Rename char %s->%s", GET_NAME(ch), oname);
 if (USE_SINGLE_PLAYER)
    {get_filename(oname,ofilename,PLAYERS_FILE);
     get_filename(GET_NAME(ch),filename,PLAYERS_FILE);
     rename(ofilename,filename);
     saved = fopen(filename,"w+b");
     fwrite(&st,sizeof(struct char_file_u), 1, saved);
     fclose(saved);
     get_filename(oname,ofilename,PKILLERS_FILE);
     get_filename(GET_NAME(ch),filename,PKILLERS_FILE);
     rename(ofilename,filename);
    }
 else
    {fseek(player_fl, GET_PFILEPOS(ch) * sizeof(struct char_file_u), SEEK_SET);
     fwrite(&st, sizeof(struct char_file_u), 1, player_fl);
    }

 // 2) Rename all other files
 get_filename(oname,ofilename,CRASH_FILE);
 get_filename(GET_NAME(ch),filename,CRASH_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,TEXT_CRASH_FILE);
 get_filename(GET_NAME(ch),filename,TEXT_CRASH_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,TIME_CRASH_FILE);
 get_filename(GET_NAME(ch),filename,TIME_CRASH_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,ETEXT_FILE);
 get_filename(GET_NAME(ch),filename,ETEXT_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,ALIAS_FILE);
 get_filename(GET_NAME(ch),filename,ALIAS_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,SCRIPT_VARS_FILE);
 get_filename(GET_NAME(ch),filename,SCRIPT_VARS_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,PQUESTS_FILE);
 get_filename(GET_NAME(ch),filename,PQUESTS_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,PMKILL_FILE);
 get_filename(GET_NAME(ch),filename,PMKILL_FILE);
 rename(ofilename,filename);
}

int delete_char(char *name)
{ char   filename[MAX_INPUT_LENGTH];
  FILE  *saved;
  struct char_file_u st;
  int    id, retval=TRUE;

  if ((id = load_char(name,&st)) >= 0)
     {// 1) Mark char as deleted
      SET_BIT(GET_FLAG(st.char_specials_saved.act, PLR_DELETED), PLR_DELETED);

      if (USE_SINGLE_PLAYER)
         {get_filename(name,filename,PLAYERS_FILE);
          saved = fopen(filename,"w+b");
          fwrite(&st,sizeof(struct char_file_u), 1, saved);
          fclose(saved);
          get_filename(name,filename,PKILLERS_FILE);
          remove(filename);
         }
      else
         {fseek(player_fl, id * sizeof(struct char_file_u), SEEK_SET);
          fwrite(&st, sizeof(struct char_file_u), 1, player_fl);
         }
      Crash_timer_obj(id, 1000000L);
      player_table[id].unique     = -1;
      player_table[id].level      = -1;
      player_table[id].last_logon = -1;
      player_table[id].activity   = -1;
     }
  else
     retval = FALSE;

  // 2) Remove all other files
  Crash_delete_file(name, CRASH_DELETE_OLD | CRASH_DELETE_NEW);

  get_filename(name,filename,ETEXT_FILE);
  remove(filename);

  get_filename(name,filename,ALIAS_FILE);
  remove(filename);

  get_filename(name,filename,SCRIPT_VARS_FILE);
  remove(filename);

  get_filename(name,filename,PQUESTS_FILE);
  remove(filename);

  get_filename(name, filename, PMKILL_FILE);
  remove(filename);
  return (retval);
}
