/* ************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//#define DEBUG // prool

#define __INTERPRETER_C__

#include <unistd.h> // prool
#include <crypt.h> // prool

#include "prool.h" // prool

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "constants.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "house.h"
#include "mail.h"
#include "screen.h"
#include "olc.h"
#include "dg_scripts.h"

extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;
extern room_rnum r_helled_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;
extern const char *class_menu;
extern const char *race_menu;
extern const char *religion_menu;
extern char *motd;
extern char *imotd;
extern char *background;
extern char *MENU;
extern char *WELC_MESSG;
extern char *START_MESSG;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int circle_restrict;
extern int no_specials;
extern int max_bad_pws;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern const char *pc_class_types[];
extern const char *race_types[];
extern long lastnews;
/* external functions */
void echo_on(struct descriptor_data *d);
void echo_off(struct descriptor_data *d);
void do_start(struct char_data *ch, int newbie);
int parse_class(char arg);
int parse_race(char arg);
int special(struct char_data *ch, int cmd, char *arg);
int isbanned(char *hostname);
int Valid_Name(char *newname);
int Is_Valid_Name(char *newname);
int Is_Valid_Dc(char *newname);
void read_aliases(struct char_data *ch);
int load_pkills(struct char_data *ch);
void read_saved_vars(struct char_data *ch);
void oedit_parse(struct descriptor_data *d, char *arg);
void redit_parse(struct descriptor_data *d, char *arg);
void zedit_parse(struct descriptor_data *d, char *arg);
void medit_parse(struct descriptor_data *d, char *arg);
void sedit_parse(struct descriptor_data *d, char *arg);
void trigedit_parse(struct descriptor_data *d, char *arg);
void delete_unique(struct char_data *ch);
void Crash_timer_obj(int index, long timer_dec);
int  find_social(char *name);
int  calc_loadroom(struct char_data *ch);
int  delete_char(char *name);
void do_aggressive_mob(struct char_data *ch, int check_sneak);

/* local functions */
int perform_dupe_check(struct descriptor_data *d);
struct alias_data *find_alias(struct alias_data *alias_list, char *str);
void free_alias(struct alias_data *a);
void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
int perform_alias(struct descriptor_data *d, char *orig);
int reserved_word(char *argument);
int find_name(char *name);
int _parse_name(char *arg, char *name);

/* prototypes for all do_x functions. */
int  find_action(char *cmd);
int  do_social(struct char_data *ch, char *argument);
void skip_wizard(struct char_data *ch, char *string);

ACMD(do_advance);
ACMD(do_alias);
ACMD(do_assist);
ACMD(do_at);
ACMD(do_affects);
ACMD(do_backstab);
ACMD(do_ban);
ACMD(do_bash);
ACMD(do_cast);
ACMD(do_cheat);
ACMD(do_create);
ACMD(do_mixture);
ACMD(do_color);
ACMD(do_courage);
ACMD(do_commands);
ACMD(do_consider);
ACMD(do_credits);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_diagnose);
ACMD(do_display);
ACMD(do_drink);
ACMD(do_drunkoff);
ACMD(do_findhelpee);
ACMD(do_firstaid);
ACMD(do_fire);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_revenge);
ACMD(do_remort);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_flee);
ACMD(do_follow);
ACMD(do_horseon);
ACMD(do_horseoff);
ACMD(do_horseput);
ACMD(do_horseget);
ACMD(do_horsetake);
ACMD(do_hidetrack);
ACMD(do_hidemove);
ACMD(do_force);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_mobshout);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_givehorse);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_hcontrol);
ACMD(do_help);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_hchannel);
ACMD(do_info);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_last);
ACMD(do_mode);
ACMD(do_mark);
ACMD(do_makefood);
ACMD(do_disarm);
ACMD(do_chopoff);
ACMD(do_deviate);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_listclan);
ACMD(do_load);
ACMD(do_look);
ACMD(do_sides);
ACMD(do_not_here);
ACMD(do_offer);
ACMD(do_olc);
ACMD(do_order);
ACMD(do_page);
ACMD(do_pray);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_skills);
ACMD(do_spells);
ACMD(do_remember);
ACMD(do_learn);
ACMD(do_forget);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quit);
ACMD(do_reboot);
ACMD(do_remove);
ACMD(do_rent);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_stopfight);
ACMD(do_stophorse);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_save);
ACMD(do_say);
ACMD(do_score);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_sleep);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_split);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_throw);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_sense);
ACMD(do_trans);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_whohouse);
ACMD(do_whoclan);
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zreset);
ACMD(do_parry);
ACMD(do_multyparry);
ACMD(do_style);
ACMD(do_poisoned);
ACMD(do_repair);
ACMD(do_camouflage);
ACMD(do_stupor);
ACMD(do_mighthit);
ACMD(do_block);
ACMD(do_touch);
ACMD(do_transform_weapon);
ACMD(do_protect);
/* DG Script ACMD's */
ACMD(do_attach);
ACMD(do_detach);
ACMD(do_tlist);
ACMD(do_tstat);
ACMD(do_masound);
ACMD(do_mkill);
ACMD(do_mjunk);
ACMD(do_mdoor);
ACMD(do_mechoaround);
ACMD(do_msend);
ACMD(do_mecho);
ACMD(do_mload);
ACMD(do_mpurge);
ACMD(do_mgoto);
ACMD(do_mat);
ACMD(do_mteleport);
ACMD(do_mforce);
ACMD(do_mexp);
ACMD(do_mgold);
ACMD(do_mhunt);
ACMD(do_mremember);
ACMD(do_mforget);
ACMD(do_mtransform);
ACMD(do_mskillturn);
ACMD(do_mskilladd);
ACMD(do_mspellturn);
ACMD(do_mspelladd);
ACMD(do_mspellitem);
ACMD(do_vdelete);
ACMD(do_hearing);
ACMD(do_looking);
ACMD(do_ident);
ACMD(do_upgrade);
ACMD(do_armored);


/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

#define MAGIC_NUM 419
#define MAGIC_LEN 8

const char *create_name_rules =
"�� ������� ����� ��������.\r\n"
"\r\n"
/* "�������, ��� ������ ��� ��� ��������� � ��� �� ��� ����� ����,\r\n"
"������, �������� ����������� ��������� ������������ ���������� ���� �����.\r\n"
"1) �� - �������� �������� ����, ������� ����� ���� ��������, ������ � �.�.\r\n"
"   ����� ���� ������������ ���������(�� �� ���������)\r\n"
"2) �������� ����� ����� (�����, ����� � �.�.) ��� ���������� - ��� ����� ����\r\n"
"   �� �������� :)\r\n"
"3) ��� ������� - � �������� �� ������������, �� ���� ���������, ������ ���\r\n"
"   �������� � �������� �����, ��������� � ������� ?\r\n"
"4) ������� ��������� ������������ ��������� ��������� � ��� � ���������������\r\n"
"   ������, �� � ��������� � ���(��������� � �������) ������������.\r\n"
"5) ���� � ��������� � ������ ����������� �������� ���� ������ ����� 3 ���\r\n"
"   �� �� ������ �� �� ���� �� ���� ������� - �� ����������� - ���� ���� �����\r\n"
"   ������� �����.\r\n"
"6) �� ����������� �������� ����� �� 2 ���� � ����� �� ��������������, ��� ����\r\n"
"   ����� ������ �����(������� ��� ��� ��������� ����������)\r\n"
"7) � ��������� - ��� ���������� ������� ���� ������������� ��� ������ ��������\r\n"
"   �� ����� ��� �� 5 ��������, ������� �� ������ ��������� � ������ �������.\r\n"
"\r\n"
"   ����, ��������� ��������� � ������ �����, ��� ������ �������� ���� ��������\r\n"
"�� ����.\r\n"*/;

cpp_extern const struct command_info cmd_info[] = {
  { "RESERVED", 0, 0, 0, 0 },	/* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "�����"    , POS_STANDING, do_move     , 0, SCMD_NORTH, -2 },
  { "������"   , POS_STANDING, do_move     , 0, SCMD_EAST, -2 },
  { "��"       , POS_STANDING, do_move     , 0, SCMD_SOUTH, -2 },
  { "�����"    , POS_STANDING, do_move     , 0, SCMD_WEST, -2 },
  { "�����"    , POS_STANDING, do_move     , 0, SCMD_UP, -2 },
  { "����"     , POS_STANDING, do_move     , 0, SCMD_DOWN, -2 },
  { "north"    , POS_STANDING, do_move     , 0, SCMD_NORTH, -2 },
  { "east"     , POS_STANDING, do_move     , 0, SCMD_EAST, -2 },
  { "south"    , POS_STANDING, do_move     , 0, SCMD_SOUTH, -2 },
  { "west"     , POS_STANDING, do_move     , 0, SCMD_WEST, -2 },
  { "up"       , POS_STANDING, do_move     , 0, SCMD_UP, -2 },
  { "down"     , POS_STANDING, do_move     , 0, SCMD_DOWN, -2 },

  { "�������",		POS_DEAD,     do_affects  , 0, SCMD_AUCTION, 0 },
  { "����������",	POS_DEAD,     do_gen_tog  , 0, SCMD_AUTOEXIT, 0 },
  { "������",		POS_DEAD    , do_gen_ps   , 0, SCMD_CREDITS, 0 },
  { "���������",	POS_FIGHTING, do_hit      , 0, SCMD_MURDER, -1 },
  { "�������",		POS_RESTING,  do_gen_comm , 0, SCMD_AUCTION, 100 },

  { "������",		POS_STANDING, do_not_here , 1, 0, 0 },
  { "������",		POS_FIGHTING, do_flee     , 1, 0, -1 },
  { "����",		POS_FIGHTING, do_block    , 0, 0, -1 },
  { "����",		POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST, 0 },
  { "�������",		POS_RESTING , do_gen_comm , 0, SCMD_GOSSIP, -1 },
  { "�������",		POS_RESTING , do_drop     , 0, SCMD_DROP, -1 },

  { "������",		POS_DEAD    , do_gen_ps   , 0, SCMD_VERSION, 0 },
  { "�����",		POS_RESTING , do_get      , 0, 0, 200 },
  { "���������",	POS_RESTING , do_diagnose , 0, 0, 100 },
  { "��������", 	POS_STANDING, do_gen_door , 1, SCMD_PICK, -1 },
  { "�������",		POS_STANDING, do_not_here , 1, 0, -1 },
  { "�������",		POS_DEAD    , do_return   , 0, 0, -1 },
  { "�����",		POS_STANDING, do_enter    , 0, 0, -2 },
  { "�����������",	POS_RESTING , do_wield , 0, 0, 200 },
  { "�����",		POS_DEAD    , do_time     , 0, 0, 0 },
  { "��������",		POS_FIGHTING, do_horseon,   0, 0, 500 },
  { "������",		POS_RESTING , do_stand    , 0, 0, 500 },
  { "���������",	POS_RESTING , do_drop     , 0, 0 /*SCMD_DONATE*/, 300 },
  { "���������",	POS_STANDING, do_track    , 0, 0, 500 },
  { "������", 		POS_STANDING, do_pour     , 0, SCMD_POUR, 500 },
  { "������",		POS_RESTING , do_exits    , 0, 0, 0 },

  { "��������",		POS_RESTING , do_say      , 0, 0, -1 },
  { "�������",		POS_SLEEPING, do_gsay     , 0, 0, 500 },
  { "���������",	POS_SLEEPING, do_gsay     , 0, 0, 500 },
  { "�������",		POS_SLEEPING ,do_hchannel , 0, 0, 0 },
  { "���",		POS_RESTING , do_where    , 1, 0, 0 },
  { "������",		POS_RESTING , do_drink    , 0, SCMD_SIP, 200 },
  { "������",		POS_SLEEPING , do_group    , 1, 0, -1 },
  { "����",		POS_DEAD    , do_gecho    , LVL_GOD, 0, 0 },


  { "����",		POS_RESTING , do_give     , 0, 0, 500 },
  { "����",		POS_DEAD    , do_date     , LVL_IMMORT, SCMD_DATE, 0 },
  { "������",		POS_RESTING , do_split    , 1, 0, 200 },
  { "�������",		POS_RESTING , do_grab     , 0, 0, 300 },
  { "��������",		POS_RESTING , do_report   , 0, 0, 500 },
  { "�������",		POS_RESTING , do_listclan , 0, 0, 0 },

  { "����",		POS_RESTING , do_eat      , 0, SCMD_EAT, 500 },

  { "����������",	POS_STANDING, do_pray     , 1, SCMD_DONATE, -1 },

  { "��������",		POS_STANDING, do_backstab, 1, 0, 500 },
  { "������",		POS_RESTING,  do_forget,    0, 0, 0 },
  { "���������",	POS_STANDING, do_not_here,  1, 0, -1 },
  { "����������",	POS_RESTING , do_spells , 0, 0, 0 },
  { "�������",		POS_SITTING , do_gen_door, 0, SCMD_CLOSE, 500 },
  { "��������",		POS_DEAD,     do_cheat, LVL_IMPL, 0, 0},
  { "�������",	        POS_STANDING, do_hidetrack, 1, 0, -1 },
  { "���������",	POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_SQUELCH, 0 },
  { "����������",	POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_FREEZE, 0 },
  { "���������",	POS_RESTING,  do_remember,  0, 0, 0 },
  { "��������",		POS_SITTING , do_gen_door , 0, SCMD_LOCK, 500 },
  { "������",		POS_DEAD    , do_ban      , LVL_GRGOD, 0, 0 },
  { "�������",		POS_SLEEPING, do_sleep    , 0, 0, -1 },
  { "��������",		POS_DEAD    , do_gen_ps   , 0, SCMD_MOTD, 0 },
  { "��������",		POS_RESTING , do_upgrade  , 0, 0, 500 },
  { "�������",		POS_RESTING,  do_remember,  0, 0, 0 },
  { "��������",		POS_RESTING , do_use      , 0, SCMD_RECITE, 500 },
  { "������",		POS_RESTING , do_gold     , 0, 0, 0 },

  { "���������",	POS_DEAD    , do_inventory, 0, 0, 0 },
  { "����",	        POS_DEAD    , do_gen_write, 0, SCMD_IDEA, 0 },
  { "�������",		POS_SITTING,  do_learn,     0, 0, 0 },
  { "�����������",	POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_IMOTD, 0 },
  { "����������",	POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO, 0 },
  { "������",		POS_RESTING , do_use      , 0, SCMD_QUAFF, 500 },

  { "���������",	POS_SITTING , do_cast     , 1, 0, -1 },
  { "�������",		POS_DEAD    , do_commands , 0, SCMD_COMMANDS, 0 },
  { "����",		POS_SLEEPING, do_quit     , 0, 0, 0 },
  { "�����",		POS_SLEEPING, do_quit     , 0, SCMD_QUIT, 0 },
  { "��������",	        POS_STANDING, do_hidemove , 1, 0, -2},
  { "�������",		POS_RESTING , do_gen_comm , 0, SCMD_SHOUT, -1 },
  { "�������",		POS_DEAD    , do_gen_tog  , 0, SCMD_BRIEF, 0 },
  { "���",		POS_RESTING , do_who      , 0, 0, 0 },
  { "����������",	POS_RESTING , do_whohouse , 0, 0, 0 },
  { "�������",		POS_RESTING , do_whoclan  , 0, 0, 0 },
  { "����",		POS_DEAD    , do_gen_ps   , 0, SCMD_WHOAMI, 0 },
  { "������",		POS_STANDING, do_not_here , 0, 0, -1 },

  { "���������",	POS_RESTING , do_grab     , 1, 0, 300 },
  { "������",		POS_STANDING, do_firstaid , 0, 0, -1 },
  { "����", 		POS_STANDING, do_pour     , 0, SCMD_POUR, 500 },
  { "������", 		POS_STANDING, do_not_here , 1, 0, -1 },

  { "����������", 	POS_RESTING , do_camouflage, 0, 0, 500 },
  { "�������",    	POS_FIGHTING, do_throw,      0, 0, -1 },
  { "������",    	POS_STANDING, do_not_here,   0, 0, -1 },
  { "�����", 		POS_RESTING , do_revenge,    0, 0, 0 },
  { "�����",        POS_FIGHTING, do_mighthit,   0, 0, -1 },
  { "��������",     POS_STANDING, do_pray,       1, SCMD_PRAY, -1},

  { "������", 		POS_STANDING, do_pour     ,  0, SCMD_FILL, 500 },
  { "���������", 	POS_STANDING, do_pour     ,  0, SCMD_FILL, 500 },
  { "�����",		POS_STANDING, do_sense    ,  0, 0, 500 },
  { "������",		POS_STANDING, do_findhelpee, 0, SCMD_BUYHELPEE, -1 },
  { "�������",		POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO, 0 },
  { "�������",		POS_SLEEPING, do_gen_ps   ,  0, SCMD_NEWS, 0 },

  { "�����������", 	POS_FIGHTING, do_disarm , 0, 0, -1 },
  { "��������", 	POS_RESTING , do_wear     , 0, 0, 500 },
  { "�����", 		POS_STANDING, do_not_here , 0, 0, 0 },
  { "����������",	POS_RESTING , do_sides    , 0, 0, 500 },
  { "��������", 	POS_FIGHTING, do_stupor,  0, 0, -1 },
  { "�����", 		POS_RESTING , do_wear     , 0, 0, 500 },
  { "��������",		POS_DEAD    , do_gen_write, 0, SCMD_TYPO, 0 },
  { "��������", 	POS_RESTING , do_ident    , 0, 0, 500 },
  { "������������",	POS_RESTING , do_drunkoff , 0, 0, -1 },
  { "��������", 	POS_RESTING , do_put      , 0, 0, 500 },
  { "�����", 		POS_RESTING , do_gen_comm , 1, SCMD_HOLLER, -1 },
  { "���������", 	POS_RESTING , do_examine  , 0, 0, 300 },
  { "��������", 	POS_STANDING, do_horsetake, 1, 0, -1 },
  { "���������", 	POS_RESTING , do_insult   , 0, 0, -1 },
  { "�������",		POS_RESTING , do_use      , 0, SCMD_QUAFF, 300 },
  { "����������",	POS_STANDING, do_makefood ,  0, 0, -1 },
  { "��������",		POS_RESTING,  do_reply    , 0, 0, -1 },
  { "��������",	    POS_FIGHTING, do_multyparry, 0, 0, -1 },
  { "��������",		POS_DEAD, do_horseget,  0, 0, -1 },
  { "���������",	POS_RESTING, do_rest, 0, 0, -1 },
  { "�������",		POS_SITTING , do_gen_door, 0, SCMD_OPEN, 500 },
  { "��������",		POS_SITTING , do_gen_door, 0, SCMD_UNLOCK, 500 },
  { "���������",	POS_SITTING , do_stophorse, 0, 0, -1 },
  { "��������",		POS_FIGHTING, do_poisoned,0, 0, -1 },
  { "���������",	POS_FIGHTING, do_stopfight, 1, 0, -1 },
  { "�������",		POS_STANDING, do_not_here , 0, 0, 500 },
  { "����",		POS_DEAD    , do_score    , 0, 0, 0 },
  { "��������",		POS_DEAD    , do_not_here,  0, SCMD_CLEAR, -1 },
  { "������",		POS_DEAD    , do_gen_write, 0, SCMD_BUG, 0 },

  { "����������",	POS_FIGHTING, do_parry ,  0, 0, -1 },
  { "�����������",      POS_FIGHTING, do_touch ,  0, 0, -1 },
  { "����������",	POS_STANDING, do_transform_weapon, 0, SKILL_TRANSFORMWEAPON, -1 },
  { "��������",		POS_STANDING, do_givehorse, 0, 0, -1 },
  { "��������������",	POS_STANDING, do_remort,  0, 0, -1 },
  { "���������������",	POS_STANDING, do_remort,  0, 1, -1 },
  { "��������",		POS_STANDING, do_pour     , 0, SCMD_POUR, 500 },
  { "����",		POS_RESTING , do_drink,   0, SCMD_DRINK, 400 },
  { "������",		POS_STANDING, do_write,   1, 0, -1 },
  { "�����",		POS_FIGHTING, do_kick,    1, 0, -1 },
  { "������",		POS_RESTING , do_weather  , 0, 0, 0 },
  { "�����������",	POS_STANDING, do_sneak    , 1, 0, 500 },
  { "��������",		POS_FIGHTING, do_chopoff, 0, 0, 500 },
  { "���������",	POS_RESTING , do_stand    , 0, 0, -1 },
  { "�����������",	POS_RESTING , do_look     , 0, SCMD_LOOK_HIDE, 200 },
  // { "����������",	POS_RESTING , do_gen_comm , 0, SCMD_GRATZ, -1 },
  { "��������",		POS_STANDING, do_leave    , 0, 0, -2 },
  { "��������",		POS_RESTING , do_put      , 0, 0, 400 },
  { "��������",		POS_STANDING, do_not_here , 1, 0, -1 },
  { "������",		POS_FIGHTING, do_assist   , 1, 0, -1 },
  { "������",		POS_DEAD    , do_help     , 0, 0, 0 },
  { "��������",		POS_DEAD    , do_mark     , LVL_IMPL, 0, 0 },
  { "������",		POS_STANDING, do_not_here , 1, 0, -1 },
  { "�����",		POS_STANDING, do_not_here , 1, 0, -1 },
  { "���������",	POS_RESTING , do_visible  , 1, 0, -1 },
  { "�������",		POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES, 0 },
  { "�����������",	POS_STANDING, do_not_here , 1, 0, 500 },
  { "������",		POS_RESTING , do_order    , 1, 0, -1 },
  { "���������",	POS_RESTING , do_horseput,  0, 0, 500 },
  { "������������",	POS_RESTING , do_looking  , 0, 0, 250 },
  { "��������",		POS_FIGHTING, do_protect, 0, 0, -1 },
  { "���������",	POS_SITTING , do_use      , 1, SCMD_USE, 400 },
  { "��������",		POS_RESTING , do_sit      , 0, 0, -1 },
  { "������������",	POS_RESTING , do_hearing  , 0, 0, 300 },
  { "�������������",POS_RESTING , do_looking  , 0, 0, 250 },
  { "����������",	POS_SLEEPING, do_wake     , 0, 0, -1 },
  { "���������",	POS_RESTING , do_eat      , 0, SCMD_TASTE, 300 },
  { "�������",		POS_STANDING, do_not_here , 0, 0, -1 },
  { "������",		POS_SLEEPING, do_goto     , LVL_GOD, 0, 0 },

  { "���������������", 	POS_DEAD    , do_ungroup,   0, 0, 500 },
  { "���������", 	POS_RESTING , do_split,     1, 0, 500 },
  { "�������", 		POS_STANDING, do_fire,      0, 0, -1 },
  { "����������", 	POS_DEAD    , do_ungroup,   0, 0, 500 },
  { "�����������",      POS_STANDING, do_not_here , 0, 0, -1 },
  { "���������",        POS_RESTING,  do_findhelpee,0, SCMD_FREEHELPEE,-1 },
  { "�����",          	POS_DEAD,     do_mode,      0, 0, 0 },
  { "������", 		POS_RESTING , do_repair,    0, 0, -1 },
  { "����",           	POS_FIGHTING, do_mixture,   0, SCMD_RUNES, -1 },

  { "�����",      		POS_FIGHTING, do_bash     , 1, 0, -1 },
  { "��������", 		POS_STANDING, do_not_here , 0, 0, -1 },
  { "�������", 			POS_SLEEPING, do_gsay     , 0, 0, -1 },
  { "�����", 			POS_RESTING , do_sit      , 0, 0, -1 },
  { "������",			POS_DEAD    , do_gen_tog  , 0, SCMD_COMPACT, 0 },
  { "�������",			POS_DEAD    , do_alias    , 0, 0, 0 },
  { "�������", 			POS_RESTING , do_tell     , 0, 0, -1 },
  { "���������", 		POS_RESTING , do_follow   , 0, 0, 500 },
  { "�������",			POS_FIGHTING, do_mixture,   0, SCMD_RUNES, -1 },
  { "��������", 		POS_RESTING , do_look     , 0, SCMD_LOOK, 200},
  { "�������",			POS_STANDING, do_mixture,   0, SCMD_ITEMS, -1 },
  { "����������",		POS_STANDING, do_transform_weapon, 0, SKILL_CREATEBOW, -1 },
  { "�����", 			POS_RESTING , do_remove   , 0, 0, 500 },
  { "�������",			POS_SITTING,  do_create,    0, 0, -1 },
  { "���", 				POS_SLEEPING, do_sleep    , 0, 0, -1 },
  { "���������", 		POS_FIGHTING, do_horseoff,  0, 0, -1 },
  { "������",			POS_RESTING,  do_create,    0, SCMD_RECIPE, 0 },
  { "���������",		POS_SLEEPING, do_save     , LVL_IMPL, 0, 0 },
  { "�������",			POS_DEAD    , do_commands , 0, SCMD_SOCIALS, 0 },
  { "�����",			POS_SLEEPING, do_sleep    , 0, 0, -1 },
  { "������",			POS_FIGHTING, do_rescue   , 1, 0, -1 },
  { "������",			POS_STANDING, do_not_here , 0, 0, -1 },
  { "�������",			POS_DEAD    , do_help     , 0, 0, 0 },
  { "��������",			POS_RESTING , do_spec_comm, 0, SCMD_ASK, -1 },
  { "����������",		POS_STANDING , do_hide     , 1, 0, 500 },
  { "��������",			POS_RESTING , do_consider , 0, 0, 500 },
  { "������",			POS_DEAD    , do_display  , 0, 0, 0 },
  { "�������",			POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR, 0 },
  { "�����",			POS_RESTING , do_style    , 0, 0, 0 },
  { "������",			POS_DEAD    , do_display  , 0, 0, 0 },
  { "����",			POS_DEAD    , do_score    , 0, 0, 0 },

  { "�����"     , POS_DEAD      , do_title    , LVL_IMMORT, 0, 0 },
  { "��������"    , POS_DEAD    , do_wimpy    , 0, 0, 0 },

  { "�����"             , POS_FIGHTING, do_kill     , 0, 0, -1 },
  { "������"   , POS_RESTING , do_remove   , 0, 0, 400 },
  { "�������"   , POS_FIGHTING, do_hit      , 0, SCMD_HIT, -1 },
  { "����������"    , POS_FIGHTING, do_deviate, 1, 0, -1 },
  { "�������"    , POS_STANDING, do_steal    , 1, 0, 300 },
  { "��������"      , POS_RESTING , do_armored      , 0, 0, -1 },
  { "������"            , POS_SLEEPING , do_skills , 0, 0, 0 },
  { "�������"   , POS_DEAD    , do_score    , 0, 0, 0 },
  { "������"     , POS_DEAD    , do_levels   , 0, 0, 0 },
  { "�������"       , POS_SLEEPING, do_force    , LVL_GRGOD, 0, 0 },
  { "�����"      , POS_STANDING, do_not_here , 0, 0, -1 },

  { "����",           POS_DEAD    , do_color    , 0, 0, 0 },

  { "������"    , POS_STANDING, do_not_here , 0, 0, -1 },
  { "������"   , POS_RESTING , do_look     , 0, SCMD_READ, 200 },

  { "�������"     , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER, -1 },

  { "����������", POS_SLEEPING, do_equipment, 0, 0, 0 },
  { "�����"    , POS_DEAD    , do_toggle   , 0, 0, 0 },
  { "������"    , POS_RESTING , do_echo     , 1, SCMD_EMOTE, -1 },
  { "���"       , POS_SLEEPING, do_echo     , LVL_IMMORT, SCMD_ECHO, -1 },

  { "������",	POS_RESTING , do_courage  , 0, 0, -1 },


  { "'"        , POS_RESTING , do_say      , 0, 0, -1 },
  { ":"        , POS_RESTING, do_echo      , 1, SCMD_EMOTE, -1 },
  { ";"        , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0, -1 },
  { "advance"  , POS_DEAD    , do_advance  , LVL_IMPL, 0, 0 },
  { "alias"    , POS_DEAD    , do_alias    , 0, 0, 0 },
  { "ask"      , POS_RESTING , do_spec_comm, 0, SCMD_ASK, -1 },
  { "assist"   , POS_FIGHTING, do_assist   , 1, 0, -1 },
  { "attack"   , POS_FIGHTING, do_hit      , 0, SCMD_MURDER, -1 },
  { "at"       , POS_DEAD    , do_at       , LVL_GRGOD, 0, 0 },
  { "auction"  , POS_RESTING , do_gen_comm , 0, SCMD_AUCTION, -1 },
  { "autoexit" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOEXIT, 0 },
  { "backstab" , POS_STANDING, do_backstab , 1, 0, 500 },
  { "balance"  , POS_STANDING, do_not_here , 1, 0, -1 },
  { "ban"      , POS_DEAD    , do_ban      , LVL_GRGOD, 0, 0 },
  { "bash"     , POS_FIGHTING, do_bash     , 1, 0, -1 },
  { "block"             , POS_FIGHTING, do_block ,  0, 0, -1 },
  { "brief"    , POS_DEAD    , do_gen_tog  , 0, SCMD_BRIEF, 0 },
  { "bug"      , POS_DEAD    , do_gen_write, 0, SCMD_BUG, 0 },
  { "buy"      , POS_STANDING, do_not_here , 0, 0, -1 },
  { "cast"     , POS_SITTING , do_cast     , 1, 0, -1 },
  { "check"    , POS_STANDING, do_not_here , 1, 0, -1 },
  { "chopoff"       , POS_FIGHTING, do_chopoff, 0, 0, 500 },
  { "clear"    , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR, 0 },
  { "close"    , POS_SITTING , do_gen_door , 0, SCMD_CLOSE, 500 },
  { "cls"      , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR, 0 },
  { "color"    , POS_DEAD    , do_color    , 0, 0, 0 },
  { "commands" , POS_DEAD    , do_commands , 0, SCMD_COMMANDS, 0 },
  { "compact"  , POS_DEAD    , do_gen_tog  , 0, SCMD_COMPACT, 0 },
  { "consider" , POS_RESTING , do_consider , 0, 0, 500 },
  { "credits"  , POS_DEAD    , do_gen_ps   , 0, SCMD_CREDITS, 0 },
  { "date"     , POS_DEAD    , do_date     , LVL_IMMORT, SCMD_DATE, 0 },
  { "dc"       , POS_DEAD    , do_dc       , LVL_GRGOD, 0, 0 },
  { "deposit"  , POS_STANDING, do_not_here , 1, 0, 500 },
  { "deviate"       , POS_FIGHTING, do_deviate, 0, 0, -1 },
  { "diagnose" , POS_RESTING , do_diagnose , 0, 0, 500 },
  { "disarm"        , POS_FIGHTING, do_disarm , 0, 0, -1 },
  { "display"  , POS_DEAD    , do_display  , 0, 0, 0 },
  { "donate"   , POS_RESTING , do_drop     , 0, 0 /*SCMD_DONATE*/, 500 },
  { "drink"    , POS_RESTING , do_drink    , 0, SCMD_DRINK, 500 },
  { "drop"     , POS_RESTING , do_drop     , 0, SCMD_DROP, 500 },
  { "eat"      , POS_RESTING , do_eat      , 0, SCMD_EAT, 500 },
  { "echo"     , POS_SLEEPING, do_echo     , LVL_IMMORT, SCMD_ECHO, 0 },
  { "emote"    , POS_RESTING , do_echo     , 1, SCMD_EMOTE, -1 },
  { "enter"    , POS_STANDING, do_enter    , 0, 0, -2 },
  { "equipment", POS_SLEEPING, do_equipment, 0, 0, 0 },
  { "examine"  , POS_RESTING, do_examine  , 0, 0, 500 },
  { "exits"    , POS_RESTING , do_exits    , 0, 0, 500 },
  { "fill"     , POS_STANDING, do_pour     , 0, SCMD_FILL, 500 },
  { "flee"     , POS_FIGHTING, do_flee     , 1, 0, -1 },
  { "follow"   , POS_RESTING , do_follow   , 0, 0, -1 },
  { "force"    , POS_SLEEPING, do_force    , LVL_GRGOD, 0, 0 },
  { "freeze"   , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_FREEZE, 0 },
  { "gecho"    , POS_DEAD    , do_gecho    , LVL_GOD, 0, 0 },
  { "get"      , POS_RESTING , do_get      , 0, 0, 500 },
  { "give"     , POS_RESTING , do_give     , 0, 0, 500 },
  { "gold"     , POS_RESTING , do_gold     , 0, 0, 0 },
  { "gossip"   , POS_RESTING , do_gen_comm , 0, SCMD_GOSSIP, -1 },
  { "goto"     , POS_SLEEPING, do_goto     , LVL_GOD, 0, 0 },
  { "grab"     , POS_RESTING , do_grab     , 0, 0, 500 },
  //{ "grats"    , POS_RESTING , do_gen_comm , 0, SCMD_GRATZ, 0 },
  { "group"    , POS_RESTING , do_group    , 1, 0, 500 },
  { "gsay"     , POS_SLEEPING, do_gsay     , 0, 0, -1 },
  { "gtell"    , POS_SLEEPING, do_gsay     , 0, 0, -1 },
  { "handbook" , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_HANDBOOK, 0 },
  { "hcontrol" , POS_DEAD    , do_hcontrol , LVL_GRGOD, 0, 0 },
  { "help"     , POS_DEAD    , do_help     , 0, 0, 0 },
  { "hell"   , POS_DEAD      , do_wizutil  , LVL_IMMORT, SCMD_HELL, 0 },
  { "hide"     , POS_STANDING, do_hide     , 1, 0, 0 },
  { "hit"      , POS_FIGHTING, do_hit      , 0, SCMD_HIT, -1 },
  { "hold"     , POS_RESTING , do_grab     , 1, 0, 500 },
  { "holler"   , POS_RESTING , do_gen_comm , 1, SCMD_HOLLER, -1 },
  { "holylight", POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_HOLYLIGHT, 0 },
  { "horse"    , POS_STANDING, do_not_here , 0, 0, -1 },
  { "house"     , POS_RESTING , do_house    , 0, 0, 0 },
  { "huk"           , POS_FIGHTING, do_mighthit,  0, 0, -1 },
  { "idea"     , POS_DEAD    , do_gen_write, 0, SCMD_IDEA, 0 },
  { "immlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST, 0 },
  { "imotd"    , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_IMOTD, 0 },
  { "info"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO, 0 },
  { "insult"   , POS_RESTING , do_insult   , 0, 0, -1 },
  { "inventory", POS_DEAD    , do_inventory, 0, 0, 0 },
  { "invis"    , POS_DEAD    , do_invis    , LVL_IMMORT, 0, -1 },
  { "junk"     , POS_RESTING , do_drop     , 0, 0 /*SCMD_JUNK*/, 500 },
  { "kick"     , POS_FIGHTING, do_kick     , 1, 0, -1 },
  { "kill"     , POS_FIGHTING, do_kill     , 0, 0, -1 },
  { "last"     , POS_DEAD    , do_last     , LVL_GRGOD, 0, 0 },
  { "leave"    , POS_STANDING, do_leave    , 0, 0, -2 },
  { "levels"   , POS_DEAD    , do_levels   , 0, 0, 0 },
  { "list"     , POS_STANDING, do_not_here , 0, 0, -1 },
  { "load"     , POS_DEAD    , do_load     , LVL_GRGOD, 0, 0 },
  { "look"     , POS_RESTING , do_look     , 0, SCMD_LOOK, 200 },
  { "lock"     , POS_SITTING , do_gen_door , 0, SCMD_LOCK, 500 },
  { "mail"     , POS_STANDING, do_not_here , 1, 0, -1 },
  { "mode",      POS_DEAD,     do_mode,      0, 0, 0 },
  { "mshout"   , POS_RESTING , do_mobshout,  0, 0, -1 },
  { "motd"     , POS_DEAD    , do_gen_ps   , 0, SCMD_MOTD, 0 },
  { "murder"   , POS_FIGHTING, do_hit      , 0, SCMD_MURDER, -1 },
  { "mute"     , POS_DEAD    , do_wizutil  , LVL_IMMORT, SCMD_SQUELCH, 0 },
  { "medit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_MEDIT},
  { "name"     , POS_DEAD      , do_wizutil, LVL_IMMORT, SCMD_NAME, 0 },
  { "news"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_NEWS, 0 },
  { "noauction", POS_DEAD    , do_gen_tog  , 0, SCMD_NOAUCTION, 0 },
  { "nogossip" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGOSSIP, 0 },
  { "nograts"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGRATZ, 0 },
  { "nohassle" , POS_DEAD    , do_gen_tog  , LVL_GOD, SCMD_NOHASSLE, 0 },
  { "norepeat" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOREPEAT, 0 },
  { "noshout"  , POS_SLEEPING, do_gen_tog  , 1, SCMD_DEAF, 0 },
  { "nosummon" , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOSUMMON, 0 },
  { "notell"   , POS_DEAD    , do_gen_tog  , 1, SCMD_NOTELL, 0 },
  { "notitle"  , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_NOTITLE, 0 },
  { "nowiz"    , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOWIZ, 0 },
  { "oedit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_OEDIT},
  { "offer"    , POS_STANDING, do_not_here , 1, 0, 0 },
//  { "olc"      , POS_DEAD    , do_olc      , LVL_GOD, SCMD_OLC_SAVEINFO },
  { "open"     , POS_SITTING , do_gen_door , 0, SCMD_OPEN, 500 },
  { "order"    , POS_RESTING , do_order    , 1, 0, -1 },
  { "page"     , POS_DEAD    , do_page     , LVL_GOD, 0, 0 },
  { "pardon"   , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_PARDON, 0 },
  { "parry"         , POS_FIGHTING, do_parry ,  0, 0, -1 },
  { "pick"     , POS_STANDING, do_gen_door , 1, SCMD_PICK, -1 },
  { "poisoned"      , POS_FIGHTING, do_poisoned,0, 0, -1 },
  { "policy"   , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES, 0 },
  { "poofin"   , POS_DEAD    , do_poofset  , LVL_GOD, SCMD_POOFIN, 0 },
  { "poofout"  , POS_DEAD    , do_poofset  , LVL_GOD, SCMD_POOFOUT, 0 },
  { "pour"     , POS_STANDING, do_pour     , 0, SCMD_POUR, -1 },
  { "practice" , POS_STANDING, do_not_here , 0, 0, -1 },
  { "prompt"   , POS_DEAD    , do_display  , 0, 0, 0 },
  { "purge"    , POS_DEAD    , do_purge    , LVL_GOD, 0, 0 },
  { "put"      , POS_RESTING , do_put      , 0, 0, 500 },
  { "qecho"    , POS_DEAD    , do_qcomm    , LVL_IMMORT, SCMD_QECHO, 0 },
  { "qsay"     , POS_RESTING , do_qcomm    , 0, SCMD_QSAY, -1 },
  { "quaff"    , POS_RESTING , do_use      , 0, SCMD_QUAFF, 500 },
  { "quest"    , POS_DEAD    , do_gen_tog  , 0, SCMD_QUEST, -1 },
  { "qui"      , POS_SLEEPING, do_quit     , 0, 0, 0 },
  { "quit"     , POS_SLEEPING, do_quit     , 0, SCMD_QUIT, -1 },
  { "read"     , POS_RESTING , do_look     , 0, SCMD_READ, 200 },
  { "receive"  , POS_STANDING, do_not_here , 1, 0, -1 },
  { "recite"   , POS_RESTING , do_use      , 0, SCMD_RECITE, 500 },
  { "redit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_REDIT},
  { "register" , POS_DEAD      , do_wizutil, LVL_IMMORT, SCMD_REGISTER, 0 },
  { "reload"   , POS_DEAD    , do_reboot   , LVL_IMPL, 0, 0 },
  { "remove"   , POS_RESTING , do_remove   , 0, 0, 500 },
  { "rent"     , POS_STANDING, do_not_here , 1, 0, -1 },
  { "reply"    , POS_RESTING , do_reply    , 0, 0, -1 },
  { "report"   , POS_RESTING , do_report   , 0, 0, -1 },
  { "reroll"   , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_REROLL },
  { "rescue"   , POS_FIGHTING, do_rescue   , 1, 0, -1 },
  { "rest"     , POS_RESTING , do_rest     , 0, 0, -1 },
  { "restore"  , POS_DEAD    , do_restore  , LVL_GRGOD, 0, 0 },
  { "return"   , POS_DEAD    , do_return   , 0, 0, -1 },
  { "roomflags", POS_DEAD    , do_gen_tog  , LVL_GRGOD, SCMD_ROOMFLAGS, 0 },
  { "save"     , POS_SLEEPING, do_save     , 0, LVL_IMPL, 0 },
  { "say"      , POS_RESTING , do_say      , 0, 0, -1 },
  { "scan"     , POS_RESTING , do_sides    , 0, 0, 500 },
  { "score"    , POS_DEAD    , do_score    , 0, 0, 0 },
  { "sedit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_SEDIT},
  { "sell"     , POS_STANDING, do_not_here , 0, 0, -1 },
  { "send"     , POS_SLEEPING, do_send     , LVL_GRGOD, 0, 0 },
  { "sense"    , POS_STANDING, do_sense    , 0, 0, 500 },
  { "set"      , POS_DEAD    , do_set      , LVL_IMMORT, 0, 0 },
  { "shout"    , POS_RESTING , do_gen_comm , 0, SCMD_SHOUT, -1 },
  { "show"     , POS_DEAD    , do_show     , LVL_GOD, 0, 0 },
  { "shutdow"  , POS_DEAD    , do_shutdown , LVL_IMPL, 0, 0 },
  { "shutdown" , POS_DEAD    , do_shutdown , LVL_IMPL, SCMD_SHUTDOWN, 0 },
  { "sip"      , POS_RESTING , do_drink    , 0, SCMD_SIP, 500 },
  { "sit"      , POS_RESTING , do_sit      , 0, 0, -1 },
  { "skills"   , POS_RESTING , do_skills , 0, 0, 0 },
  { "skillset" , POS_SLEEPING, do_skillset , LVL_GRGOD, 0, 0 },
  { "sleep"    , POS_SLEEPING, do_sleep    , 0, 0, -1 },
  { "slowns"   , POS_DEAD    , do_gen_tog  , LVL_IMPL, SCMD_SLOWNS, 0 },
  { "sneak"    , POS_STANDING, do_sneak    , 1, 0, -2 },
  { "snoop"    , POS_DEAD    , do_snoop    , LVL_GOD, 0, 0 },
  { "socials"  , POS_DEAD    , do_commands , 0, SCMD_SOCIALS, 0 },
  { "spells"   , POS_RESTING , do_spells , 0, 0, 0 },
  { "split"    , POS_RESTING, do_split    , 1, 0, 0 },
  { "stand"    , POS_RESTING , do_stand    , 0, 0, -1 },
  { "stat"     , POS_DEAD    , do_stat     , LVL_GOD, 0, 0 },
  { "steal"    , POS_STANDING, do_steal    , 1, 0, 300 },
  { "stupor"        , POS_FIGHTING, do_stupor,  0, 0, -1 },
  { "switch"   , POS_DEAD    , do_switch   , LVL_GRGOD, 0, 0 },
  { "syslog"   , POS_DEAD    , do_syslog   , LVL_GOD, 0, 0 },
  { "take"     , POS_RESTING , do_get      , 0, 0, 500 },
  { "taste"    , POS_RESTING , do_eat      , 0, SCMD_TASTE, 500 },
  { "teleport" , POS_DEAD    , do_teleport , LVL_GRGOD, 0, -1 },
  { "tell"     , POS_RESTING , do_tell     , 0, 0, -1 },
  { "thaw"     , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_THAW, 0 },
  { "time"     , POS_DEAD    , do_time     , 0, 0, 0 },
  { "title"    , POS_DEAD    , do_title    , LVL_IMMORT, 0, 0 },
  { "toggle"   , POS_DEAD    , do_toggle   , 0, 0, -1 },
  { "touch"         , POS_FIGHTING, do_touch ,  0, 0, -1 },
  { "track"    , POS_STANDING, do_track    , 0, 0, -1 },
  { "trackthru", POS_DEAD    , do_gen_tog  , LVL_IMPL, SCMD_TRACK, 0 },
  { "transfer" , POS_SLEEPING, do_trans    , LVL_GRGOD, 0, 0 },
//  { "trigedit" , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_TRIGEDIT},
  { "typo"     , POS_DEAD    , do_gen_write, 0, SCMD_TYPO, 0 },
  { "unaffect" , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_UNAFFECT, 0 },
  { "unban"    , POS_DEAD    , do_unban    , LVL_GRGOD, 0, 0 },
  { "ungroup"  , POS_DEAD    , do_ungroup  , 0, 0, -1 },
  { "unlock"   , POS_SITTING , do_gen_door , 0, SCMD_UNLOCK, 500 },
  { "uptime"   , POS_DEAD    , do_date     , LVL_IMMORT, SCMD_UPTIME, 0 },
  { "use"      , POS_SITTING , do_use      , 1, SCMD_USE, 500 },
  { "users"    , POS_DEAD    , do_users    , LVL_IMMORT, 0, 0 },
  { "value"    , POS_STANDING, do_not_here , 0, 0, -1 },
  { "version"  , POS_DEAD    , do_gen_ps   , 0, SCMD_VERSION, 0 },
  { "visible"  , POS_RESTING , do_visible  , 1, 0, -1 },
  { "vnum"     , POS_DEAD    , do_vnum     , LVL_GRGOD, 0, 0 },
  { "vstat"    , POS_DEAD    , do_vstat    , LVL_GRGOD, 0, 0 },
  { "wake"     , POS_SLEEPING, do_wake     , 0, 0, -1 },
  { "wear"     , POS_RESTING , do_wear     , 0, 0, 500 },
  { "weather"  , POS_RESTING , do_weather  , 0, 0, 0 },
  { "where"    , POS_RESTING , do_where    , 1, 0, 0 },
  { "whisper"  , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER, -1 },
  { "who"      , POS_RESTING , do_who      , 0, 0, 0 },
  { "whoami"   , POS_DEAD    , do_gen_ps   , 0, SCMD_WHOAMI, 0 },
  { "wield"    , POS_RESTING , do_wield    , 0, 0, 500 },
  { "wimpy"    , POS_DEAD    , do_wimpy    , 0, 0, 0 },
  { "withdraw" , POS_STANDING, do_not_here , 1, 0, -1 },
  { "wizhelp"  , POS_SLEEPING, do_commands , LVL_IMMORT, SCMD_WIZHELP, 0 },
  { "wizlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_WIZLIST, 0 },
  { "wizlock"  , POS_DEAD    , do_wizlock  , LVL_IMPL, 0, 0 },
  { "wiznet"   , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0, 0 },
  { "write"    , POS_STANDING, do_write    , 1, 0, -1 },
  { "zedit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_ZEDIT},
  { "zreset"   , POS_DEAD    , do_zreset   , LVL_GRGOD, 0, 0 },


  /* DG trigger commands */
  { "attach"   , POS_DEAD    , do_attach   , LVL_IMPL, 0, 0 },
  { "detach"   , POS_DEAD    , do_detach   , LVL_IMPL, 0, 0 },
  { "tlist"    , POS_DEAD    , do_tlist    , LVL_GRGOD, 0, 0 },
  { "tstat"    , POS_DEAD    , do_tstat    , LVL_GRGOD, 0, 0 },
  { "masound"  , POS_DEAD    , do_masound  , -1, 0, -1 },
  { "mkill"    , POS_STANDING, do_mkill    , -1, 0, -1 },
  { "mjunk"    , POS_SITTING , do_mjunk    , -1, 0, -1 },
  { "mdoor"    , POS_DEAD    , do_mdoor    , -1, 0, -1 },
  { "mecho"    , POS_DEAD    , do_mecho    , -1, 0, -1 },
  { "mechoaround" , POS_DEAD , do_mechoaround, -1, 0, -1 },
  { "msend"    , POS_DEAD    , do_msend    , -1, 0, -1 },
  { "mload"    , POS_DEAD    , do_mload    , -1, 0, -1 },
  { "mpurge"   , POS_DEAD    , do_mpurge   , -1, 0, -1 },
  { "mgoto"    , POS_DEAD    , do_mgoto    , -1, 0, -1 },
  { "mat"      , POS_DEAD    , do_mat      , -1, 0, -1 },
  { "mteleport", POS_DEAD    , do_mteleport, -1, 0, -1 },
  { "mforce"   , POS_DEAD    , do_mforce   , -1, 0, -1 },
  { "mexp"     , POS_DEAD    , do_mexp     , -1, 0, -1 },
  { "mgold"    , POS_DEAD    , do_mgold    , -1, 0, -1 },
  { "mhunt"    , POS_DEAD    , do_mhunt    , -1, 0, -1 },
  { "mremember", POS_DEAD    , do_mremember, -1, 0, -1 },
  { "mforget"  , POS_DEAD    , do_mforget  , -1, 0, -1 },
  { "mtransform",POS_DEAD    , do_mtransform,-1, 0, -1 },
  { "mskillturn",POS_DEAD    , do_mskillturn,-1, 0, -1 },
  { "mskilladd",  POS_DEAD    , do_mskilladd, -1, 0, -1 },
  { "mspellturn",POS_DEAD    , do_mspellturn,-1, 0, -1 },
  { "mspelladd", POS_DEAD    , do_mspelladd, -1, 0, -1 },
  { "mspellitem", POS_DEAD   , do_mspellitem,-1, 0, -1 },
  { "vdelete"  , POS_DEAD    , do_vdelete  , LVL_IMPL, 0, 0 },

  { "\n", 0, 0, 0, 0 } };	/* this must be last */


const char *fill[] =
{ "in",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

const char *reserved[] =
{ "a",
  "an",
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};


void check_hiding_cmd(struct char_data *ch, int percent)
{ int remove_hide = FALSE;
  if (affected_by_spell(ch, SPELL_HIDE))
     {if (percent == -2)
         {if (AFF_FLAGGED(ch, AFF_SNEAK))
             remove_hide = number(1,skill_info[SKILL_SNEAK].max_percent) >
                           calculate_skill(ch,SKILL_SNEAK,skill_info[SKILL_SNEAK].max_percent,0);
          else
             percent = 500;
         }

      if (percent == -1)
         remove_hide = TRUE;
      else
      if (percent > 0)
         remove_hide = number(1,percent) > calculate_skill(ch,SKILL_HIDE,percent,0);

      if (remove_hide)
         {affect_from_char(ch, SPELL_HIDE);
          if (!AFF_FLAGGED(ch, AFF_HIDE))
             {send_to_char("�� ���������� ���������.\r\n",ch);
              act("$n ���������$g ���������.",FALSE,ch,0,0,TO_ROOM);
             }
         }
     }
}

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data *ch, char *argument)
{
  int  cmd, length, social = FALSE, hardcopy = FALSE;
  char *line;

  /* just drop to next line for hitting CR */
  CHECK_AGRO(ch) = 0;
  skip_wizard(ch, argument);
  skip_spaces(&argument);

  if (!*argument)
     return;
  log("<%s> [%s]",GET_NAME(ch), argument);
  /*
   * special case to handle one-character, non-alphanumeric commands;
   * requested by many people so "'hi" or ";godnet test" is possible.
   * Patch sent by Eric Green and Stefan Wasilewski.
   */
  if (!a_isalpha(*argument))
     {arg[0] = argument[0];
      arg[1] = '\0';
      line   = argument + 1;
     }
  else
     line = any_one_arg(argument, arg);

  if (// GET_LEVEL(ch) < LVL_IMMORT &&
      !GET_MOB_HOLD(ch) || GET_COMMSTATE(ch)
     )
     {int cont;            /* continue the command checks */
      cont = command_wtrigger(ch, arg, line);
      if (!cont) cont  += command_mtrigger(ch, arg, line);
      if (!cont) cont   = command_otrigger(ch, arg, line);
      if (cont)
         {check_hiding_cmd(ch,-1);
          return;    // command trigger took over
         }
     }

  /* otherwise, find the command */
  if ((length = strlen(arg)) &&
      length > 1 && *(arg+length-1) == '!')
     {hardcopy = TRUE;
      *(arg+(--length))   = '\0';
      *(argument+length)  = ' ';
     }

  for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
      if (hardcopy)
         {if (!strcmp(cmd_info[cmd].command, arg))
             {if (GET_LEVEL(ch) >= cmd_info[cmd].minimum_level || GET_COMMSTATE(ch))
	             break;
	         }
         }
      else
         {if (!strncmp(cmd_info[cmd].command, arg, length))
             {if (GET_LEVEL(ch) >= cmd_info[cmd].minimum_level || GET_COMMSTATE(ch))
                 break;
             }
         }

  if (*cmd_info[cmd].command == '\n')
     {if (find_action(arg) >= 0)
         social = TRUE;
      else
         {send_to_char("���� ?\r\n", ch);
          return;
         }
     }

  if (!IS_NPC(ch) && !IS_GOD(ch) &&
      (PLR_FLAGGED(ch, PLR_FROZEN) || GET_MOB_HOLD(ch) > 0)
     )
     {send_to_char("�� ����������, �� �� ������ ���������� � �����...\r\n", ch);
      return;
     }

  if (!social && cmd_info[cmd].command_pointer == NULL)
     {send_to_char("��������, �� ���� ��������� �������.\r\n", ch);
      return;
     }

  if (!social && IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IMMORT)
     {send_to_char("�� ��� �� ���, ����� ������ ���.\r\n", ch);
      return;
     }

  if (!social && GET_POS(ch) < cmd_info[cmd].minimum_position)
     {switch (GET_POS(ch))
         {case POS_DEAD:
               send_to_char("����� ���� - �� ������ !!! :-(\r\n", ch);
               break;
          case POS_INCAP:
          case POS_MORTALLYW:
               send_to_char("�� � ����������� ��������� � �� ������ ������ ������!\r\n", ch);
               break;
          case POS_STUNNED:
               send_to_char("�� ������� �����, ����� ������� ���!\r\n", ch);
               break;
          case POS_SLEEPING:
               send_to_char("������� ��� � ����� ���� ?\r\n", ch);
               break;
          case POS_RESTING:
               send_to_char("���... �� ������� �����������..\r\n", ch);
               break;
          case POS_SITTING:
               send_to_char("�������, ��� ����� ������ �� ����.\r\n", ch);
               break;
          case POS_FIGHTING:
               send_to_char("�� �� ��� !  �� ���������� �� ���� �����!\r\n", ch);
               break;
         }
      return;
     }

  if (social)
     {check_hiding_cmd(ch,-1);
      do_social(ch, argument);
     }
  else
  if (no_specials || !special(ch, cmd, line))
     {check_hiding_cmd(ch,cmd_info[cmd].unhide_percent);
      (*cmd_info[cmd].command_pointer) (ch, line, cmd, cmd_info[cmd].subcmd);
      if (!IS_NPC(ch) && IN_ROOM(ch) != NOWHERE && CHECK_AGRO(ch))
         {do_aggressive_mob(ch,FALSE);
          CHECK_AGRO(ch) = FALSE;
         }
     }
  skip_wizard(ch,NULL);
}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
  while (alias_list != NULL)
        {if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
            if (!strcmp(str, alias_list->alias))
	           return (alias_list);

         alias_list = alias_list->next;
        }

  return (NULL);
}


void free_alias(struct alias_data *a)
{
  if (a->alias)
     free(a->alias);
  if (a->replacement)
     free(a->replacement);
  free(a);
}


/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  char *repl;
  struct alias_data *a, *temp;

  if (IS_NPC(ch))
     return;

  repl = any_one_arg(argument, arg);

  if (!*arg)
     {/* no argument specified -- list currently defined aliases */
      send_to_char("���������� ��������� ������:\r\n", ch);
      if ((a = GET_ALIASES(ch)) == NULL)
         send_to_char(" ��� �������.\r\n", ch);
      else
         {while (a != NULL)
                {sprintf(buf, "%-15s %s\r\n", a->alias, a->replacement);
	             send_to_char(buf, ch);
	             a = a->next;
                }
         }
     }
  else
     {/* otherwise, add or remove aliases */
      /* is this an alias we've already defined? */
      if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL)
         {REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
          free_alias(a);
         }
      /* if no replacement string is specified, assume we want to delete */
      if (!*repl)
         {if (a == NULL)
	         send_to_char("����� ����� �� ���������.\r\n", ch);
          else
	         send_to_char("����� ������� ������.\r\n", ch);
         }
      else
         {/* otherwise, either add or redefine an alias */
          if (!str_cmp(arg, "alias"))
             {send_to_char("�� �� ������ ���������� ����� 'alias'.\r\n", ch);
	          return;
             }
          CREATE(a, struct alias_data, 1);
          a->alias = str_dup(arg);
          delete_doubledollar(repl);
          a->replacement = str_dup(repl);
          if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
	         a->type = ALIAS_COMPLEX;
          else
	         a->type = ALIAS_SIMPLE;
          a->next = GET_ALIASES(ch);
          GET_ALIASES(ch) = a;
          send_to_char("����� ������� ��������.\r\n", ch);
         }
     }
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  temp = strtok(strcpy(buf2, orig), " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS)
        {tokens[num_of_tokens++] = temp;
         temp = strtok(NULL, " ");
        }

  /* initialize */
  write_point     = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++)
      {if (*temp == ALIAS_SEP_CHAR)
          {*write_point = '\0';
           buf[MAX_INPUT_LENGTH - 1] = '\0';
           write_to_q(buf, &temp_queue, 1);
           write_point = buf;
          }
       else
       if (*temp == ALIAS_VAR_CHAR)
          {temp++;
           if ((num = *temp - '1') < num_of_tokens && num >= 0)
              {strcpy(write_point, tokens[num]);
	           write_point += strlen(tokens[num]);
              }
           else
           if (*temp == ALIAS_GLOB_CHAR)
              {strcpy(write_point, orig);
	           write_point += strlen(orig);
              }
           else
           if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
	          *(write_point++) = '$';
          }
       else
          *(write_point++) = *temp;
      }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
     *input_q = temp_queue;
  else
     {temp_queue.tail->next = input_q->head;
      input_q->head = temp_queue.head;
     }
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data *d, char *orig)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias_data *a, *tmp;

  /* Mobs don't have alaises. */
  if (IS_NPC(d->character))
     return (0);

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return (0);

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
     return (0);

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
     return (0);

  if (a->type == ALIAS_SIMPLE)
     {strcpy(orig, a->replacement);
      return (0);
     }
  else
     {perform_complex_alias(&d->input, ptr, a);
      return (1);
     }
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, const char **list, int exact)
{
  register int i, l;

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
      *(arg + l) = LOWER(*(arg + l));

  if (exact)
     { for (i = 0; **(list + i) != '\n'; i++)
           if (!str_cmp(arg, *(list + i)))
	          return (i);
     }
  else
     {if (!l)
         l = 1;	/* Avoid "" to match the first available
				 * string */
      for (i = 0; **(list + i) != '\n'; i++)
          if (!strn_cmp(arg, *(list + i), l))
	         return (i);
     }

  return (-1);
}


int is_number(const char *str)
{
  while (*str)
        if (!isdigit(*(str++)))
           return (0);

  return (1);
}

/*
 * Function to skip over the leading spaces of a string.
 */
void skip_spaces(char **string)
{ for (; **string && a_isspace(**string); (*string)++);
}

void skip_wizard(struct char_data *ch, char *string)
{
  SET_COMMSTATE(ch,0);
  return;
}


/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
char *delete_doubledollar(char *string)
{
  char *read, *write;

  /* If the string has no dollar signs, return immediately */
  if ((write = strchr(string, '$')) == NULL)
     return (string);

  /* Start from the location of the first dollar sign */
  read = write;


  while (*read)   /* Until we reach the end of the string... */
        if ((*(write++) = *(read++)) == '$') /* copy one char */
           if (*read == '$')
	          read++; /* skip if we saw 2 $'s in a row */

  *write = '\0';

  return (string);
}


int fill_word(char *argument)
{ return (search_block(argument, fill, TRUE) >= 0);
}


int reserved_word(char *argument)
{ return (search_block(argument, reserved, TRUE) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument)
     {log("SYSERR: one_argument received a NULL pointer!");
      *first_arg = '\0';
      return (NULL);
     }

  do {skip_spaces(&argument);

      first_arg = begin;
      while (*argument && !a_isspace(*argument))
            {*(first_arg++) = LOWER(*argument);
             argument++;
            }

      *first_arg = '\0';
     } while (fill_word(begin));

  return (argument);
}


/*
 * one_word is like one_argument, except that words in quotes ("") are
 * considered one word.
 */
char *one_word(char *argument, char *first_arg)
{
  char *begin = first_arg;

  do {skip_spaces(&argument);
      first_arg = begin;

      if (*argument == '\"')
         {argument++;
          while (*argument && *argument != '\"')
                {*(first_arg++) = LOWER(*argument);
                 argument++;
                }
          argument++;
         }
      else
         {while (*argument && !a_isspace(*argument))
                {*(first_arg++) = LOWER(*argument);
                 argument++;
                }
         }
      *first_arg = '\0';
     } while (fill_word(begin));

  return (argument);
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !a_isspace(*argument))
        {*(first_arg++) = LOWER(*argument);
         argument++;
        }

  *first_arg = '\0';

  return (argument);
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{ return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}



/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 *
 * returnss 1 if arg1 is an abbreviation of arg2
 */
int is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
     return (0);

  for (; *arg1 && *arg2; arg1++, arg2++)
      if (LOWER(*arg1) != LOWER(*arg2))
         return (0);

  if (!*arg1)
     return (1);
  else
     return (0);
}



/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  strcpy(arg2, temp);
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
  int cmd;

  for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
      if (!strcmp(cmd_info[cmd].command, command))
         return (cmd);

  return (-1);
}


int special(struct char_data *ch, int cmd, char *arg)
{
  register struct obj_data *i;
  register struct char_data *k;
  int j;

  /* special in room? */
  if (GET_ROOM_SPEC(ch->in_room) != NULL)
      if (GET_ROOM_SPEC(ch->in_room) (ch, world + ch->in_room, cmd, arg))
         {check_hiding_cmd(ch,-1);
          return (1);
         }

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
         if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
	         {check_hiding_cmd(ch,-1);
	          return (1);
	         }

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
      if (GET_OBJ_SPEC(i) != NULL)
         if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	        {check_hiding_cmd(ch,-1);
	         return (1);
	        }

  /* special in mobile present? */
  for (k = world[ch->in_room].people; k; k = k->next_in_room)
      if (GET_MOB_SPEC(k) != NULL)
         if (GET_MOB_SPEC(k) (ch, k, cmd, arg))
	        {check_hiding_cmd(ch,-1);
	         return (1);
	        }

  /* special in object present? */
  for (i = world[ch->in_room].contents; i; i = i->next_content)
      if (GET_OBJ_SPEC(i) != NULL)
         if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	        {check_hiding_cmd(ch,-1);
	         return (1);
            }

  return (0);
}



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


/* locate entry in p_table with entry->name == name. -1 mrks failed search */
int find_name(char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
      {if (!str_cmp((player_table + i)->name, name))
          return (i);
      }

  return (-1);
}

int is_lat (char c) // prool
{
if ((c>='a') && (c<='z')) return 1;
if ((c>='A') && (c<='Z')) return 1;
return 0;
}

int _parse_name(char *arg, char *name)
{
  int i;

printf("prooldebug arg=%s\n", arg);

#if 0 // prool: 0 - proolfool mode, 1 - original mode
  /* skip whitespaces */
// prool:
//  for (i = 0; (*name = (i ? LOWER(*arg) : UPPER(*arg))); arg++, i++, name++)
  for (i = 0; (*name = (is_lat(*arg) ? (*arg) : (i ? LOWER(*arg) : UPPER(*arg)))); arg++, i++, name++)
      if (!a_isalpha(*arg) || *arg /* > 0 */ ) // prool
	{
	printf("prooldebug name=%s\n", name);
         return (1);
	}

if (!i) return 1;
#else
strcpy(name,arg);
#endif

printf("prooldebug name=%s\n", name);

return (0);
}

#define RECON		1
#define USURP		2
#define UNSWITCH	3

/*
 * XXX: Make immortals 'return' instead of being disconnected when switched
 *      into person returns.  This function seems a bit over-extended too.
 */
int perform_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  struct char_data *target = NULL, *ch, *next_ch;
  int mode = 0;

  int id = GET_IDNUM(d->character);

  /*
   * Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number.
   */

  for (k = descriptor_list; k; k = next_k)
      {next_k = k->next;
       if (k == d)
          continue;

       if (k->original && (GET_IDNUM(k->original) == id))
          {/* switched char */
	   if (str_cmp(d->host, k->host))
	      {sprintf(buf,"��������� ���� !!! ID = %ld �������� = %s ���� = %s(��� %s)",
                       GET_IDNUM(d->character),
                       GET_NAME(d->character),
                       d->host, k->host
                      );
	       mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
               //send_to_gods(buf);
	      }
           SEND_TO_Q("\r\n������� ������� ����� - �����������.\r\n", k);
           STATE(k) = CON_CLOSE;
           if (!target)
              {target = k->original;
	       mode   = UNSWITCH;
              }
           if (k->character)
	      k->character->desc = NULL;
           k->character          = NULL;
           k->original           = NULL;
          }
       else
       if (k->character && (GET_IDNUM(k->character) == id))
          {if (str_cmp(d->host, k->host))
	      {sprintf(buf,"��������� ���� !!! ID = %ld Name = %s Host = %s(was %s)",
                       GET_IDNUM(d->character),
                       GET_NAME(d->character),
                       d->host, k->host
                      );
	       mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
               //send_to_gods(buf);
	      }

           if (!target && STATE(k) == CON_PLAYING)
              {SEND_TO_Q("\r\n���� ���� ��� ���-�� ������ !\r\n", k);
	       target = k->character;
	       mode   = USURP;
              }
           k->character->desc = NULL;
           k->character       = NULL;
           k->original        = NULL;
           SEND_TO_Q("\r\n������� ������� ����� - �����������.\r\n", k);
           STATE(k) = CON_CLOSE;
          }
      }

 /*
  * now, go through the character list, deleting all characters that
  * are not already marked for deletion from the above step (i.e., in the
  * CON_HANGUP state), and have not already been selected as a target for
  * switching into.  In addition, if we haven't already found a target,
  * choose one if one is available (while still deleting the other
  * duplicates, though theoretically none should be able to exist).
  */

  for (ch = character_list; ch; ch = next_ch)
      {next_ch = ch->next;

       if (IS_NPC(ch))
          continue;
       if (GET_IDNUM(ch) != id)
          continue;

       /* ignore chars with descriptors (already handled by above step) */
       if (ch->desc)
          continue;

       /* don't extract the target char we've found one already */
       if (ch == target)
          continue;

       /* we don't already have a target and found a candidate for switching */
       if (!target)
          {target = ch;
           mode   = RECON;
           continue;
          }

       /* we've found a duplicate - blow him away, dumping his eq in limbo. */
       if (ch->in_room != NOWHERE)
          char_from_room(ch);
       char_to_room(ch, STRANGE_ROOM);
       extract_char(ch,FALSE);
      }

  /* no target for swicthing into was found - allow login to continue */
  if (!target)
     return (0);

  /* Okay, we've found a target.  Connect d to target. */

  free_char(d->character); /* get rid of the old char */
  d->character       = target;
  d->character->desc = d;
  d->original        = NULL;
  d->character->char_specials.timer = 0;
  REMOVE_BIT(PLR_FLAGS(d->character, PLR_MAILING), PLR_MAILING);
  REMOVE_BIT(PLR_FLAGS(d->character, PLR_WRITING), PLR_WRITING);
  STATE(d) = CON_PLAYING;

  switch (mode)
  {
  case RECON:
    SEND_TO_Q("���������������.\r\n", d);
    act("$n �����������$g �����.", TRUE, d->character, 0, 0, TO_ROOM);
    sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
    break;
  case USURP:
    SEND_TO_Q("���� ���� ����� ��������� � ����, ������� ��� ����� �� ����������� !\r\n", d);
    act("$n ��������$u �� ����, ��������$w ����� �����...\r\n"
        "���� $s ���� ��������� ����� ����� !",
	TRUE, d->character, 0, 0, TO_ROOM);
    sprintf(buf, "%s has re-logged in ... disconnecting old socket.",
	        GET_NAME(d->character));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
    break;
  case UNSWITCH:
    SEND_TO_Q("��������������� ��� ������������� ������.", d);
    sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
    break;
  }

  return (1);
}

int pre_help(struct char_data *ch, char *arg)
{ char command[MAX_INPUT_LENGTH], topic[MAX_INPUT_LENGTH];

  half_chop(arg, command, topic);

  if (!command || !*command || strlen(command) < 2 ||
      !topic   || !*topic   || strlen(topic) < 2)
     return (0);
  if (isname(command, "������") ||
      isname(command, "help")   ||
      isname(command, "�������"))
     {do_help(ch,topic,0,0);
      return (1);
     }
  return (0);
}

int  check_dupes_host(struct descriptor_data *d)
{struct descriptor_data *i;
 if (!d->character || IS_IMMORTAL(d->character) || PLR_FLAGGED(d->character, PLR_REGISTERED))
    return (1);

 for (i = descriptor_list; i; i = i->next)
     {if (i == d)
         continue;
      // if (i->character)
      //     {sprintf(buf,"%s<%s> and %s<%s>\r\n",GET_NAME(d->character),d->host,
      //	          GET_NAME(i->character),i->host);
      //      send_to_gods(buf);
      //     }
      if (i->character               &&
          !IS_IMMORTAL(i->character) &&
	  STATE(i) != CON_PLAYING    &&
	  !str_cmp(i->host, d->host))
	 {sprintf(buf,"�� ����� � ������� %s � ������ IP(%s) !\r\n"
	              "��� ���������� ���������� � ����� ��� �����������.\r\n"
		      "���� �� ������ �������� � ������� ��� �������������������� �������.\r\n",
		      GET_PAD(i->character,4),i->host);
          send_to_char(buf,d->character);
	  sprintf(buf,"! ���� � ������ IP ! ��������������������� ������.\r\n"
	              "����� - %s, � ���� - %s, IP - %s.\r\n"
		      "����� ������� � ������� �������������������� �������.\r\n",
		      GET_NAME(d->character), GET_NAME(i->character), d->host);
          mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
	  //send_to_gods(buf);
	  return (0);
	 }
     }
 return (1);
}

void do_entergame(struct descriptor_data *d)
{ int    load_room, load_result, neednews=0;
  struct char_data *ch;

  reset_char(d->character);
  read_aliases(d->character);
  if (GET_HOUSE_UID(d->character) && !House_check_exist(GET_HOUSE_UID(d->character)))
     GET_HOUSE_UID(d->character) = GET_HOUSE_RANK(d->character) = 0;
  if (PLR_FLAGGED(d->character, PLR_INVSTART))
     GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);

  /*
   * We have to place the character in a room before equipping them
   * or equip_char() will gripe about the person in NOWHERE.
   */
  if (PLR_FLAGGED(d->character, PLR_HELLED))
     load_room = r_helled_start_room;
  else
  if (PLR_FLAGGED(d->character, PLR_NAMED))
     load_room = r_named_start_room;
  else
  if (PLR_FLAGGED(d->character, PLR_FROZEN))
     load_room = r_frozen_start_room;
  else
  if (!check_dupes_host(d))
     load_room = r_unreg_start_room;
  else
     {if ((load_room = GET_LOADROOM(d->character)) == NOWHERE)
         load_room = calc_loadroom(d->character);
      load_room = real_room(load_room);
     }

  log("Player %s enter at room %d", GET_NAME(d->character), load_room);
  /* If char was saved with NOWHERE, or real_room above failed... */
  if (load_room == NOWHERE)
     {if (GET_LEVEL(d->character) >= LVL_IMMORT)
         load_room = r_immort_start_room;
      else
         load_room = r_mortal_start_room;
     }
  send_to_char(WELC_MESSG, d->character);

  for (ch = character_list; ch; ch = ch->next)
      if (ch == d->character)
         break;
	
  if (!ch)
     {d->character->next = character_list;
      character_list     = d->character;
     }
  else
     {REMOVE_BIT(MOB_FLAGS(ch,MOB_DELETE), MOB_DELETE);
      REMOVE_BIT(MOB_FLAGS(ch,MOB_FREE),   MOB_FREE);
     }

  load_result        = 0;
  char_to_room(d->character, load_room);
  if (GET_LEVEL(d->character) != 0)
     load_result        = Crash_load(d->character);

  if (lastnews && LAST_LOGON(d->character) < lastnews)
     neednews = TRUE;

   /* with the copyover patch, this next line goes in enter_player_game() */
   GET_ID(d->character) = GET_IDNUM(d->character);
   GET_ACTIVITY(d->character) = number(0, PLAYER_SAVE_ACTIVITY-1);
   save_char(d->character, NOWHERE);
   act("$n �������$g � ����.", TRUE, d->character, 0, 0, TO_ROOM);
   /* with the copyover patch, this next line goes in enter_player_game() */
   read_saved_vars(d->character);
   greet_mtrigger(d->character, -1);
   greet_memory_mtrigger(d->character);
   STATE(d) = CON_PLAYING;
   if (GET_LEVEL(d->character) == 0)
      {do_start(d->character, TRUE);
       send_to_char(START_MESSG, d->character);
       send_to_char("�������������� �������� ������� ��� ��������� ������� ���������� ������.\r\n",d->character);
      }
   sprintf(buf, "%s [%s] - ����� � ���� - %s %d, %s.\r\n",
           GET_NAME(d->character), d->host,
           pc_class_types[(int) GET_CLASS(d->character)], GET_LEVEL(d->character),
           race_types[GET_RACE(d->character)]);
   mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
   //send_to_gods(buf);
   if (neednews)
      {sprintf(buf,"%s��� ������� ������ ������� (������� �������)%s\r\n",
                   CCWHT(d->character,C_NRM), CCNRM(d->character,C_NRM));
       send_to_char(buf,d->character);
      }
   look_at_room(d->character, 0);
   if (has_mail(GET_IDNUM(d->character)))
      send_to_char("��� ������� ������.\r\n", d->character);
   if (load_result == 2)
      {/* rented items lost */
       send_to_char("\r\n\007�� �� ������ �������� ���� ������ !\r\n"
                    "��� ���� ���� �������� � ������ ���������� �� ����� ������� ����� !\r\n",
                    d->character);
      }
   // sprintf(buf,"NEWS %ld %ld(%s)",lastnews,LAST_LOGON(d->character),GET_NAME(d->character));
   // mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
   // sprintf(buf,"� ��� %s.\r\n",d->character->desc ? "���� ����������" : "��� �����������");
   // send_to_char(buf,d->character);
   d->has_prompt = 0;
}

void DoAfterPassword(struct descriptor_data *d)
{int load_result;
	
 /* Password was correct. */
 load_result = GET_BAD_PWS(d->character);
 GET_BAD_PWS(d->character) = 0;
 d->bad_pws                = 0;

 if (isbanned(d->host) == BAN_SELECT &&
     !PLR_FLAGGED(d->character, PLR_SITEOK))
    {SEND_TO_Q("��������, �� �� ������ ������� ����� ������ � ������� IP !\r\n", d);
     STATE(d) = CON_CLOSE;
     sprintf(buf, "Connection attempt for %s denied from %s",
	     GET_NAME(d->character), d->host);
     mudlog(buf, NRM, LVL_GOD, TRUE);
     return;
    }
 if (GET_LEVEL(d->character) < circle_restrict)
    {SEND_TO_Q("���� �������� ��������������.. ���� ��� ������� �����.\r\n", d);
     STATE(d) = CON_CLOSE;
     sprintf(buf, "Request for login denied for %s [%s] (wizlock)",
	     GET_NAME(d->character), d->host);
     mudlog(buf, NRM, LVL_GOD, TRUE);
     return;
    }

  /* check and make sure no other copies of this player are logged in */
  if (perform_dupe_check(d))
     return;

  if (GET_LEVEL(d->character) >= LVL_IMMORT)
     SEND_TO_Q(imotd, d);
  else
     SEND_TO_Q(motd, d);

  sprintf(buf, "%s [%s] has connected.", GET_NAME(d->character), d->host);
  mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);

  if (load_result)
     {sprintf(buf, "\r\n\r\n\007\007\007"
	      "%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
	      CCRED(d->character, C_SPR), load_result,
	      (load_result > 1) ? "S" : "", CCNRM(d->character, C_SPR));
      SEND_TO_Q(buf, d);
      GET_BAD_PWS(d->character) = 0;
     }
   SEND_TO_Q("\r\n* � ����� � ���������� �������� ����� ANYKEY ������� ENTER *", d);
   STATE(d) = CON_RMOTD;
}

/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg)
{ char   buf[128], *dog_pos;
  int    player_i, load_result;
  char   tmp_name[MAX_INPUT_LENGTH], pwd_name[MAX_INPUT_LENGTH], pwd_pwd[MAX_INPUT_LENGTH];
  struct char_file_u tmp_store;

  int i1, i2, i3; // prool

  skip_spaces(&arg);

  switch (STATE(d))
  {
  /*. OLC states .*/
  case CON_OEDIT:
    oedit_parse(d, arg);
    break;
  case CON_REDIT:
    redit_parse(d, arg);
    break;
  case CON_ZEDIT:
    zedit_parse(d, arg);
    break;
  case CON_MEDIT:
    medit_parse(d, arg);
    break;
  case CON_SEDIT:
    sedit_parse(d, arg);
    break;
  case CON_TRIGEDIT:
    trigedit_parse(d, arg);
    break;
  /*. End of OLC states .*/

  case CON_GET_KEYTABLE:
       {if (!*arg || *arg < '0' || *arg >= '0'+KT_LAST)
           {SEND_TO_Q("\r\nUnknown key table. Retry, please : ",d);
            return;
           };
        d->keytable = (ubyte)*arg - (ubyte)'0';
        SEND_TO_Q("\r\n\r\n\
\r\n������� ���� ���: ",d);
        STATE(d) = CON_GET_NAME;
       }
    break;
  case CON_GET_NAME:		/* wait for input of name */
    if (d->character == NULL)
       {CREATE(d->character, struct char_data, 1);
        memset(d->character, 0, sizeof(struct char_data));
        clear_char(d->character);
        CREATE(d->character->player_specials, struct player_special_data, 1);
	memset(d->character->player_specials, 0, sizeof(struct player_special_data));
        d->character->desc = d;
       }
    if (!*arg)
       STATE(d) = CON_CLOSE;
    else
       {if (sscanf(arg,"%s %s",pwd_name, pwd_pwd) == 2)
           {if (_parse_name(pwd_name, tmp_name) ||
	        (player_i = load_char(tmp_name, &tmp_store)) < 0
	       )
	       {SEND_TO_Q("error 1. ������������ ���. ���������, ����������.\r\n"
	                  "��� : ", d);
	        return;
	       }
	    store_to_char(&tmp_store, d->character);
	    if (PLR_FLAGGED(d->character, PLR_DELETED) ||
	        strncmp(CRYPT(pwd_pwd, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)
	       )
	       {SEND_TO_Q("error 2. ������������ ���. ���������, ����������.\r\n"
	                  "��� : ", d);
		free_char(d->character);			
		d->character = NULL;
	        return;
	       }
	    load_pkills(d->character);
	    REMOVE_BIT(PLR_FLAGS(d->character, PLR_MAILING), PLR_MAILING);
	    REMOVE_BIT(PLR_FLAGS(d->character, PLR_WRITING), PLR_WRITING);
	    REMOVE_BIT(PLR_FLAGS(d->character, PLR_CRYO), PLR_CRYO);	
	    GET_PFILEPOS(d->character) = player_i;
	    GET_ID(d->character) = GET_IDNUM(d->character);
	    DoAfterPassword(d);
	    return;
	   }
	else
        if (((i1=_parse_name(arg, tmp_name))*0) || // 0 - prool
            strlen(tmp_name) < MIN_NAME_LENGTH ||
	    strlen(tmp_name) > MAX_NAME_LENGTH ||
	    // !Is_Valid_Name(tmp_name)           || // prool
	    ((i2=fill_word(strcpy(buf, tmp_name))*0))   || // 0 - prool
	    ((i3=reserved_word(buf))*0)) // 0 - prool
	   {SEND_TO_Q("error 3. ������������ ���. ���������, ����������.\r\n"
		      "��� : ", d);
		printf("prooldebug. i1=%i i2=%i i3=%i tmp_name=%s\n", i1, i2, i3, tmp_name);
       	    return;
           }
	else
	if (!Is_Valid_Dc(tmp_name))
	   {SEND_TO_Q("����� � �������� ������ ��������� � ����.\r\n",d);
	    SEND_TO_Q("�� ��������� ������������� ������� ���� ��� ������.\r\n",d);
	    SEND_TO_Q("��� � ������ ����� ������ : ",d);
	    return;
	   }
        if ((player_i = load_char(tmp_name, &tmp_store)) > -1)
           {store_to_char(&tmp_store, d->character);
	    GET_PFILEPOS(d->character) = player_i;
	    if (PLR_FLAGGED(d->character, PLR_DELETED))
	    {/* We get a false positive from the original deleted character. */
	     free_char(d->character);
	     d->character = 0;
	     /* Check for multiple creations... */
	     if (!Valid_Name(tmp_name))
	        {SEND_TO_Q("error 4. ������������ ���. ���������, ����������.\r\n"
	                   "��� : ", d);
	         return;
	        }
	     CREATE(d->character, struct char_data, 1);
	     memset(d->character, 0, sizeof(struct char_data));
	     clear_char(d->character);
	     CREATE(d->character->player_specials, struct player_special_data, 1);
	     memset(d->character->player_specials, 0, sizeof(struct player_special_data));
	     d->character->desc = d;
	     CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	     strcpy(d->character->player.name, CAP(tmp_name));
     	     CREATE(GET_PAD(d->character,0), char, strlen(tmp_name) + 1);
	     strcpy(GET_PAD(d->character,0), CAP(tmp_name));
	     GET_PFILEPOS(d->character) = player_i;
	     sprintf(buf, "�� ������������� ������� ��� %s [ Y(�) / N(�) ] ? ", tmp_name);
	     SEND_TO_Q(buf, d);
	     STATE(d) = CON_NAME_CNFRM;
	    }
	    else
	    {/* undo it just in case they are set */
	     load_pkills(d->character);
	     REMOVE_BIT(PLR_FLAGS(d->character, PLR_MAILING), PLR_MAILING);
	     REMOVE_BIT(PLR_FLAGS(d->character, PLR_WRITING), PLR_WRITING);
	     REMOVE_BIT(PLR_FLAGS(d->character, PLR_CRYO), PLR_CRYO);	
	     SEND_TO_Q("������ : ", d);
	     echo_off(d);
	     d->idle_tics = 0;
	     STATE(d) = CON_PASSWORD;
	    }
           }
        else
           {/* player unknown -- make new character */
	    /* Check for multiple creations of a character. */
	    if (!Valid_Name(tmp_name))
	       {SEND_TO_Q("error 5. ������������ ���. ���������, ����������.\r\n"
	                  "��� : ", d);
	        return;
	       }
	    if (cmp_ptable_by_name(tmp_name,MIN_NAME_LENGTH+1) >= 0)
	       {SEND_TO_Q("������  �������� ������ ����� ��������� � ��� ������������ ����������.\r\n"
	                  "��� ���������� ������ ������������� ��� ���������� ������� ������ ���.\r\n"
	                  "���  : ", d);
	            return;
	        }
	     CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	     strcpy(d->character->player.name, CAP(tmp_name));
	     CREATE(GET_PAD(d->character,0), char, strlen(tmp_name) + 1);
	     strcpy(GET_PAD(d->character,0), CAP(tmp_name));
	     SEND_TO_Q(create_name_rules, d);
	     sprintf(buf, "�� ������������� ������� ���  %s [ Y(�) / N(�) ] ? ", tmp_name);
	     SEND_TO_Q(buf, d);
	     STATE(d) = CON_NAME_CNFRM;
            }
           }
    break;
  case CON_NAME_CNFRM:		/* wait for conf. of new name    */
    if (UPPER(*arg) == 'Y' || UPPER(*arg) == '�' )
       {if (isbanned(d->host) >= BAN_NEW)
           {sprintf(buf, "������� �������� ��������� %s ��������� ��� [%s] (siteban)",
		    GET_PC_NAME(d->character), d->host);
	    mudlog(buf, NRM, LVL_GOD, TRUE);
	    SEND_TO_Q("��������, �������� ������ ��������� ��� ������ IP !!! ��������� !!!\r\n", d);
	    STATE(d) = CON_CLOSE;
	    return;
           }
        if (circle_restrict)
           {SEND_TO_Q("��������, �� �� ������ ������� ����� �������� � ��������� ������.\r\n", d);
	    sprintf(buf, "������� �������� ������ ��������� %s ��������� ��� [%s] (wizlock)",
		    GET_PC_NAME(d->character), d->host);
	    mudlog(buf, NRM, LVL_GOD, TRUE);
	    STATE(d) = CON_CLOSE;
	    return;
	   }
        SEND_TO_Q("����� ��������.\r\n", d);
        sprintf(buf, "��� ������ ��������� � ����������� ������ [ ��� ���� ? ] ");
        SEND_TO_Q(buf, d);
        echo_on(d);
        STATE(d) = CON_NAME2;	
       }
    else
    if (UPPER(*arg) == 'N' || UPPER(*arg) == '�')
       {SEND_TO_Q("����, ���� �������� ? ������, ������� ��� :)\r\n"
                  "��� : ", d);
        free(d->character->player.name);
        d->character->player.name = NULL;
        STATE(d) = CON_GET_NAME;
       }
    else
       {SEND_TO_Q("�������� Yes(��) or No(���) : ", d);
       }
    break;

  case CON_PASSWORD:		/* get pwd for known player      */
    /*
     * To really prevent duping correctly, the player's record should
     * be reloaded from disk at this point (after the password has been
     * typed).  However I'm afraid that trying to load a character over
     * an already loaded character is going to cause some problem down the
     * road that I can't see at the moment.  So to compensate, I'm going to
     * (1) add a 15 or 20-second time limit for entering a password, and (2)
     * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
     */

    echo_on(d);    /* turn echo back on */

    /* New echo_on() eats the return on telnet. Extra space better than none. */
    SEND_TO_Q("\r\n", d);

    if (!*arg)
       STATE(d) = CON_CLOSE;
    else
       {if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH))
           {sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
	            mudlog(buf, BRF, LVL_GOD, TRUE);
	    GET_BAD_PWS(d->character)++;
	    save_char(d->character, NOWHERE);
	    if (++(d->bad_pws) >= max_bad_pws)
	       {/* 3 strikes and you're out. */
	        SEND_TO_Q("�������� ������... �������������.\r\n", d);
	        STATE(d) = CON_CLOSE;
	       }
	    else
	       {SEND_TO_Q("�������� ������.\r\n������ : ", d);
	        echo_off(d);
	       }
	    return;
           }
        DoAfterPassword(d);
       }
    break;

  case CON_NEWPASSWD:
  case CON_CHPWD_GETNEW:
    if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
	!str_cmp(arg, GET_PC_NAME(d->character)))
       {SEND_TO_Q("\r\n�������� ������.\r\n", d);
        SEND_TO_Q("������ : ", d);
        return;
       }
#ifdef DEBUG
printf("L1 %i\n",strlen(GET_PASSWD(d->character)));
//printf("L1a %i\n",strlen(CRYPT(arg, GET_PC_NAME(d->character))));
printf("L1b %i\n",CRYPT(arg, GET_PC_NAME(d->character)));
printf("L1c %i\n", MAX_PWD_LENGTH);
#endif
    strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_PC_NAME(d->character)), MAX_PWD_LENGTH);
#ifdef DEBUG
printf("L2\n");
#endif
    *(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

    SEND_TO_Q("\r\n��������� ������, ���������� : ", d);
    if (STATE(d) == CON_NEWPASSWD)
       STATE(d) = CON_CNFPASSWD;
    else
       STATE(d) = CON_CHPWD_VRFY;

    break;

  case CON_CNFPASSWD:
  case CON_CHPWD_VRFY:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character),
		MAX_PWD_LENGTH))
	   {SEND_TO_Q("\r\n������ �� �������������... ��������.\r\n", d);
        SEND_TO_Q("������: ", d);
        if (STATE(d) == CON_CNFPASSWD)
	       STATE(d) = CON_NEWPASSWD;
        else
	       STATE(d) = CON_CHPWD_GETNEW;
        return;
       }
    echo_on(d);

    if (STATE(d) == CON_CNFPASSWD)
       {SEND_TO_Q("\r\n��� ��� [ �(M)/�(F) ] ? ", d);
        STATE(d) = CON_QSEX;
       }
    else
       {save_char(d->character, NOWHERE);
        echo_on(d);
        SEND_TO_Q("\r\n������.\r\n", d);
        SEND_TO_Q(MENU, d);
        STATE(d) = CON_MENU;
       }

    break;

  case CON_QSEX:		/* query sex of new user         */
    if (pre_help(d->character,arg))
       {SEND_TO_Q("\r\n��� ��� [ �(M)/�(F) ] ? ", d);
        STATE(d) = CON_QSEX;
        return;
       }
    switch (UPPER(*arg))
    {
    case '�':
    case 'M':
      GET_SEX(d->character) = SEX_MALE;
      break;
    case '�':
    case 'F':
      GET_SEX(d->character) = SEX_FEMALE;
      break;
    default:
      SEND_TO_Q("��� ����� ���� � ���, �� ���� �� ��� :)\r\n"
		        "� ����� � ��� ��� ? ", d);
      return;
    }
    SEND_TO_Q(religion_menu,d);
    SEND_TO_Q("\n\r������� :",d);
    STATE(d) = CON_RELIGION;
    break;

  case  CON_RELIGION:  /* query religion of new user      */
    if (pre_help(d->character,arg))
       {SEND_TO_Q(religion_menu,d);
        SEND_TO_Q("\n\r������� :",d);
        STATE(d) = CON_RELIGION;
        return;
       }

    switch (UPPER(*arg))
    {
    case '�':
    case '�':
    case 'P':
      GET_RELIGION(d->character) = RELIGION_POLY;
      break;
    case '�':
    case 'C':
      GET_RELIGION(d->character) = RELIGION_MONO;
      break;
    default:
      SEND_TO_Q("������ ������ �� ����� :)\r\n"
		        "��� ����� ����� �� ������ ������� ? ", d);
      return;
    }
    SEND_TO_Q(class_menu, d);
    SEND_TO_Q("\r\n���� ��������� : ", d);
    STATE(d) = CON_QCLASS;
    break;

  case CON_QCLASS:
    if (pre_help(d->character,arg))
       {SEND_TO_Q(class_menu, d);
        SEND_TO_Q("\r\n���� ��������� : ", d);
        STATE(d) = CON_QCLASS;
        return;
       }
    load_result = parse_class(*arg);
    if (load_result == CLASS_UNDEFINED)
       {SEND_TO_Q("\r\n��� �� ���������.\r\n��������� : ", d);
        return;
       }
    else
       GET_CLASS(d->character) = load_result;
    SEND_TO_Q(race_menu, d);
    SEND_TO_Q("\r\n�� ���� �� ������ : ", d);
    STATE(d) = CON_RACE;
    break;

  case  CON_RACE:       /* query race      */
    if (pre_help(d->character,arg))
       {SEND_TO_Q(race_menu, d);
        SEND_TO_Q("\r\n��� : ", d);
        STATE(d) = CON_RACE;
        return;
       }
    load_result = parse_race(*arg);
    if (load_result == RACE_UNDEFINED)
       {SEND_TO_Q("������ �� ������� �������.\r\n"
	          "����� ��� ��� ����� ����� ? ", d);
        return;
       }
    GET_RACE(d->character) = load_result;
    SEND_TO_Q("\r\n��� E-mail : ",d);
    STATE(d) = CON_GET_EMAIL;
    break;

  case CON_GET_EMAIL:
    if (!*arg)
       {SEND_TO_Q("\r\n��� E-mail : ",d);
        return;
       }
    else
    if (!(dog_pos = strchr(arg,'@')) ||
        dog_pos == arg               ||
        !*(dog_pos+1)
       )
       {SEND_TO_Q("\r\n������������ E-mail !"
                  "\r\n��� E-mail :  ",d);
        return;
       }

    if (GET_PFILEPOS(d->character) < 0)
       GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));

    /* Now GET_NAME() will work properly. */
    init_char(d->character);
    strncpy(GET_EMAIL(d->character),arg,127);
    *(GET_EMAIL(d->character)+127) = '\0';
    save_char(d->character,NOWHERE);

    sprintf(buf, "%s [%s] - ����� ����� - %s, %s.\r\n",
            GET_NAME(d->character), d->host,
            pc_class_types[(int) GET_CLASS(d->character)],
            race_types[(int) GET_RACE(d->character)]);
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
    //send_to_gods(buf);
    SEND_TO_Q(motd, d);
    SEND_TO_Q("\r\n* � ����� � ���������� �������� ����� ANYKEY ������� ENTER *", d);
    STATE(d) = CON_RMOTD;

    sprintf(buf, "%s [%s] new player.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, LVL_IMMORT, TRUE);
    break;

  case CON_RMOTD:		/* read CR after printing motd   */
    do_entergame(d);
    // SEND_TO_Q(MENU, d);
    // STATE(d) = CON_MENU;
    break;

  case CON_MENU:		/* get selection from main menu  */
    switch (*arg)
    {
    case '0':
      SEND_TO_Q("�� ������� �� ����� ��������.\r\n", d);
      STATE(d) = CON_CLOSE;
      break;

    case '1':
      do_entergame(d);
      break;

    case '2':
      if (d->character->player.description)
         {SEND_TO_Q("���� ������� ��������:\r\n", d);	
	      SEND_TO_Q(d->character->player.description, d);
	/*
	 * Don't free this now... so that the old description gets loaded
	 * as the current buffer in the editor.  Do setup the ABORT buffer
	 * here, however.
	 *
	 * free(d->character->player.description);
	 * d->character->player.description = NULL;
	 */
	      d->backstr = str_dup(d->character->player.description);
         }
      SEND_TO_Q("������� �������� ������ �����, ������� ����� ���������� �� ������� <���������>.\r\n", d);
      SEND_TO_Q("(/s ��������� /h ������)\r\n", d);
      d->str     = &d->character->player.description;
      d->max_str = EXDSCR_LENGTH;
      STATE(d)   = CON_EXDESC;
      break;

    case '3':
      page_string(d, background, 0);
      STATE(d) = CON_RMOTD;
      break;

    case '4':
      SEND_TO_Q("\r\n������� ������ ������ : ", d);
      echo_off(d);
      STATE(d) = CON_CHPWD_GETOLD;
      break;

    case '5':
      SEND_TO_Q("\r\n��� ������������� ������� ���� ������ : ", d);
      echo_off(d);
      STATE(d) = CON_DELCNF1;
      break;

    default:
      SEND_TO_Q("\r\n��� �� ���� ���������� ����� !\r\n", d);
      SEND_TO_Q(MENU, d);
      break;
    }

    break;

  case CON_CHPWD_GETOLD:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH))
       {echo_on(d);
        SEND_TO_Q("\r\n�������� ������.\r\n", d);
        SEND_TO_Q(MENU, d);
        STATE(d) = CON_MENU;
       }
    else
       {SEND_TO_Q("\r\n������� ����� ������ : ", d);
        STATE(d) = CON_CHPWD_GETNEW;
       }
    return;

  case CON_DELCNF1:
    echo_on(d);
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH))
       {SEND_TO_Q("\r\n�������� ������.\r\n", d);
        SEND_TO_Q(MENU, d);
        STATE(d) = CON_MENU;
       }
    else
       {SEND_TO_Q("\r\n!!! ��� �������� ����� ������ !!!\r\n"
		  "�� ��������� � ���� ������� ?\r\n\r\n"
		  "�������� \"YES / ��\" ��� ������������� : ", d);
        STATE(d) = CON_DELCNF2;
       }
    break;

  case CON_DELCNF2:
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES") ||
        !strcmp(arg, "��")  || !strcmp(arg, "��"))
       {if (GET_LEVEL(d->character) < 2       ||
            PLR_FLAGGED(d->character, PLR_FROZEN)
	   )
           {SEND_TO_Q("�� �������� �� ������, �� ���� ���������� ���.\r\n", d);
	    SEND_TO_Q("�������� �� ������.\r\n", d);
	    STATE(d) = CON_CLOSE;
	    return;
           }
        if (GET_LEVEL(d->character) >= LVL_GRGOD)
           return;
        delete_char(GET_NAME(d->character));
        sprintf(buf, "�������� '%s' ������ !\r\n"
	             "�� ��������.\r\n", GET_NAME(d->character));
        SEND_TO_Q(buf, d);
        sprintf(buf, "%s (lev %d) has self-deleted.", GET_NAME(d->character),
	             GET_LEVEL(d->character));
        mudlog(buf, NRM, LVL_GOD, TRUE);
        STATE(d) = CON_CLOSE;
        return;
       }
    else
       {SEND_TO_Q("\r\n�������� �� ������.\r\n", d);
        SEND_TO_Q(MENU, d);
        STATE(d) = CON_MENU;
       }
    break;
  case CON_NAME2:
     skip_spaces(&arg);

     if (!_parse_name(arg,tmp_name) &&
         strlen(tmp_name) >= 4 &&
         strlen(tmp_name) <= MAX_NAME_LENGTH)
        {CREATE(GET_PAD(d->character,1), char, strlen(tmp_name)+1);
         strcpy(GET_PAD(d->character,1), CAP(tmp_name));
         sprintf(buf, "��� ������ ��������� � ��������� ������ [ ��������� ���� ? ] ");
         SEND_TO_Q(buf, d);
         echo_on(d);
         STATE(d) = CON_NAME3;	
        }
     else
        { SEND_TO_Q("�����������.\r\n", d);
          sprintf(buf, "��� ������ ��������� � ����������� ������ [ ��� ���� ? ] ");
          SEND_TO_Q(buf, d);
         };
     break;
  case CON_NAME3:
     skip_spaces(&arg);
     if (!_parse_name(arg,tmp_name) &&
         strlen(tmp_name) >= 4 &&
         strlen(tmp_name) <= MAX_NAME_LENGTH)
        {CREATE(GET_PAD(d->character,2), char, strlen(tmp_name)+1);
         strcpy(GET_PAD(d->character,2), CAP(tmp_name));
         sprintf(buf, "��� ������ ��������� � ����������� ������ [ ������� ���� ? ] ");
         SEND_TO_Q(buf, d);
         echo_on(d);
         STATE(d) = CON_NAME4;	
        }
     else
        { SEND_TO_Q("�����������.\r\n", d);
          sprintf(buf, "��� ������ ��������� � ��������� ������ [ ��������� ���� ? ] ");
          SEND_TO_Q(buf, d);
         };
     break;
  case CON_NAME4:
     skip_spaces(&arg);
     if (!_parse_name(arg,tmp_name) &&
         strlen(tmp_name) >= 4 &&
         strlen(tmp_name) <= MAX_NAME_LENGTH)
        {CREATE(GET_PAD(d->character,3), char, strlen(tmp_name)+1);
         strcpy(GET_PAD(d->character,3), CAP(tmp_name));
         sprintf(buf, "��� ������ ��������� � ������������ ������ [ ��������� � ��� ? ] ");
         SEND_TO_Q(buf, d);
         echo_on(d);
         STATE(d) = CON_NAME5;	
        }
     else
        { SEND_TO_Q("�����������.\n\r", d);
          sprintf(buf, "��� ������ ��������� � ����������� ������ [ ������� ���� ? ] ");
          SEND_TO_Q(buf, d);
         };
     break;
  case CON_NAME5:
     skip_spaces(&arg);
     if (!_parse_name(arg,tmp_name) &&
         strlen(tmp_name) >= 4 &&
         strlen(tmp_name) <= MAX_NAME_LENGTH)
        {CREATE(GET_PAD(d->character,4), char, strlen(tmp_name)+1);
         strcpy(GET_PAD(d->character,4), CAP(tmp_name));
         sprintf(buf, "��� ������ ��������� � ���������� ������ [ �������� � ��� ? ] ");
         SEND_TO_Q(buf, d);
         echo_on(d);
         STATE(d) = CON_NAME6;	
        }
     else
        { SEND_TO_Q("�����������.\n\r", d);
          sprintf(buf, "��� ������ ��������� � ������������ ������ [ ��������� � ��� ? ] ");
          SEND_TO_Q(buf, d);
         };
     break;
  case CON_NAME6:
     skip_spaces(&arg);
     if (!_parse_name(arg,tmp_name) &&
         strlen(tmp_name) >= 4 &&
         strlen(tmp_name) <= MAX_NAME_LENGTH)
        {CREATE(GET_PAD(d->character,5), char, strlen(tmp_name)+1);
         strcpy(GET_PAD(d->character,5), CAP(tmp_name));
         sprintf(buf, "������� ������ ��� %s : ", GET_PAD(d->character,1));
         SEND_TO_Q(buf, d);
         echo_off(d);
         STATE(d) = CON_NEWPASSWD;
        }
     else
        { SEND_TO_Q("�����������.\n\r", d);
          sprintf(buf, "��� ������ ��������� � ���������� ������ [ �������� � ��� ? ] ");
          SEND_TO_Q(buf, d);
         };
     break;

  case CON_CLOSE:
    break;

  default:
    log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
        STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
    STATE(d) = CON_DISCONNECT;	/* Safest to do. */
    break;
   }
}
