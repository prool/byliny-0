/* ************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <unistd.h> // prool
#include <crypt.h> // prool

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "screen.h"
#include "constants.h"
#include "olc.h"
#include "dg_scripts.h"

/*   external vars  */
extern FILE *player_fl;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern struct attack_hit_type attack_hit_text[];
extern char *class_abbrevs[];
extern const char *weapon_affects[];
extern time_t boot_time;
extern zone_rnum top_of_zone_table;
extern int circle_shutdown, circle_reboot;
extern int circle_restrict;
extern int load_into_inventory;
extern int buf_switches, buf_largecount, buf_overflows;
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;
extern int top_of_p_table;
extern int shutdown_time;
extern struct player_index_element *player_table;
extern struct obj_data *obj_proto;
extern room_rnum r_helled_start_room;
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;

/* for chars */
extern const char *pc_class_types[];

/* extern functions */
int    level_exp(int chclass, int level);
void   show_shops(struct char_data * ch, char *value);
void   hcontrol_list_houses(struct char_data *ch);
void   do_start(struct char_data *ch, int newbie);
void   appear(struct char_data *ch);
void   reset_zone(zone_rnum zone);
void   roll_real_abils(struct char_data *ch);
int    parse_class(char arg);
struct char_data *find_char(long);
void   load_pkills(struct char_data *ch);
void   rename_char(struct char_data *ch, char *oname);
void   save_pkills(struct char_data *ch);
void   new_save_quests(struct char_data *ch);
void   new_save_mkill(struct char_data *ch);
int    get_ptable_by_name(char *name);
int    _parse_name(char *arg, char *name);
int    Valid_Name(char *name);
int    reserved_word(char *name);
int    compute_armor_class(struct char_data *ch);
int    calc_loadroom(struct char_data *ch);
/* local functions */
int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
void perform_immort_invis(struct char_data *ch, int level);
ACMD(do_echo);
ACMD(do_send);
room_rnum find_target_room(struct char_data * ch, char *rawroomstr);
ACMD(do_at);
ACMD(do_goto);
ACMD(do_trans);
ACMD(do_teleport);
ACMD(do_vnum);
void do_stat_room(struct char_data * ch);
void do_stat_object(struct char_data * ch, struct obj_data * j);
void do_stat_character(struct char_data * ch, struct char_data * k);
ACMD(do_stat);
ACMD(do_shutdown);
void stop_snooping(struct char_data * ch);
ACMD(do_snoop);
ACMD(do_switch);
ACMD(do_return);
ACMD(do_load);
ACMD(do_vstat);
ACMD(do_purge);
ACMD(do_syslog);
ACMD(do_advance);
ACMD(do_restore);
void perform_immort_vis(struct char_data *ch);
ACMD(do_invis);
ACMD(do_gecho);
ACMD(do_poofset);
ACMD(do_dc);
ACMD(do_wizlock);
ACMD(do_date);
ACMD(do_last);
ACMD(do_force);
ACMD(do_wiznet);
ACMD(do_zreset);
ACMD(do_wizutil);
void print_zone_to_buf(char *bufptr, zone_rnum zone);
ACMD(do_show);
ACMD(do_set);

#define MAX_TIME 0x7fffffff

ACMD(do_echo)
{
  skip_spaces(&argument);

  if (!*argument)
     send_to_char("И что Вы хотите выразить столь красочно ?\r\n", ch);
  else
     {if (subcmd == SCMD_EMOTE)
         sprintf(buf, "*$n %s", argument);
      else
         strcpy(buf, argument);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(OK, ch);
      else
         act(buf, FALSE, ch, 0, 0, TO_CHAR);
     }
}


ACMD(do_send)
{
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg)
     {send_to_char("Послать что и кому (не путать с куда и кого :)\r\n", ch);
      return;
     }
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
     {send_to_char(NOPERSON, ch);
      return;
     }
  send_to_char(buf, vict);
  send_to_char("\r\n", vict);
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
     send_to_char("Послано.\r\n", ch);
  else
     {sprintf(buf2, "Вы послали '%s' %s.\r\n", buf, GET_PAD(vict,2));
      send_to_char(buf2, ch);
     }
}



/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(struct char_data * ch, char *rawroomstr)
{
  room_vnum tmp;
  room_rnum location;
  struct char_data *target_mob;
  struct obj_data *target_obj;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if (!*roomstr)
     {send_to_char("Укажите номер или название комнаты.\r\n", ch);
      return (NOWHERE);
     }
  if (isdigit(*roomstr) && !strchr(roomstr, '.'))
     {tmp = atoi(roomstr);
      if ((location = real_room(tmp)) < 0)
         {send_to_char("Нет комнаты с таким номером.\r\n", ch);
          return (NOWHERE);
         }
     }
  else
  if ((target_mob = get_char_vis(ch, roomstr, FIND_CHAR_WORLD)) != NULL)
     location = target_mob->in_room;
  else
  if ((target_obj = get_obj_vis(ch, roomstr)) != NULL)
     {if (target_obj->in_room != NOWHERE)
         location = target_obj->in_room;
      else
         {send_to_char("Этот объект Вам недоступен.\r\n", ch);
          return (NOWHERE);
         }
     }
  else
     {send_to_char("В округе нет похожего предмета или создания.\r\n", ch);
      return (NOWHERE);
     }

  /* a location has been found -- if you're < GRGOD, check restrictions. */
  if (!IS_GRGOD(ch))
     {if (ROOM_FLAGGED(location, ROOM_GODROOM) && GET_LEVEL(ch) < LVL_GRGOD)
         {send_to_char("Вы не столь божественны, чтобы получить доступ в это комнату!\r\n", ch);
          return (NOWHERE);
         }
      if (ROOM_FLAGGED(location, ROOM_PRIVATE) &&
	      world[location].people && world[location].people->next_in_room)
	     {send_to_char("Что-то интимное вершится в этой комнате - третий явно лишний.\r\n", ch);
          return (NOWHERE);
         }
      if (ROOM_FLAGGED(location, ROOM_HOUSE) &&
	      !House_can_enter(ch, GET_ROOM_VNUM(location)))
	     {send_to_char("Частная собственность - посторонним в ней делать нечего !\r\n", ch);
          return (NOWHERE);
         }
     }
  return (location);
}



ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH];
  room_rnum location, original_loc;

  half_chop(argument, buf, command);
  if (!*buf)
     {send_to_char("Необходимо указать номер или название комнаты.\r\n", ch);
      return;
     }

  if (!*command)
     {send_to_char("Что Вы собираетесь там делать ?\r\n", ch);
      return;
     }

  if ((location = find_target_room(ch, buf)) < 0)
     return;

  /* a location has been found. */
  original_loc = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the char is still there */
  if (ch->in_room == location)
     {char_from_room(ch);
      char_to_room(ch, original_loc);
     }
  check_horse(ch);
}


ACMD(do_goto)
{
  room_rnum location;

  if ((location = find_target_room(ch, argument)) < 0)
     return;

  if (!GET_COMMSTATE(ch))
     {if (POOFOUT(ch))
         sprintf(buf, "$n %s", POOFOUT(ch));
      else
         strcpy(buf, "$n растворил$u в клубах дыма.");
     }
  else
     strcpy(buf, "$n взял$g свиток возврата в левую рукую.\r\n"
                 "$n зачитал$g свиток возврата.\r\n"
                 "$n исчез$q.");

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);

  char_to_room(ch, location);
  check_horse(ch);

  if (!GET_COMMSTATE(ch) && POOFIN(ch))
     sprintf(buf, "$n %s", POOFIN(ch));
  else
     strcpy(buf, "$n возник$q посреди комнаты.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
}



ACMD(do_trans)
{
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf);
  if (!*buf)
     send_to_char("Кого Вы хотите переместить ?\r\n", ch);
  else
  if (str_cmp("all", buf) && str_cmp("все",buf))
     {if (!(victim = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
         send_to_char(NOPERSON, ch);
      else
      if (victim == ch)
         send_to_char("Не стОит, да ?\r\n", ch);
      else
         {if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim) && !GET_COMMSTATE(ch))
              {send_to_char("Он гораздо старше Вас, стыдитесь !\r\n", ch);
  	           return;
              }
           act("$n растворил$u в клубах дыма.", FALSE, victim, 0, 0, TO_ROOM);
           char_from_room(victim);
           char_to_room(victim, ch->in_room);
           check_horse(victim);
           act("$n появил$u, окутанн$w розовым туманом.", FALSE, victim, 0, 0, TO_ROOM);
           act("$n призвал$g Вас !", FALSE, ch, 0, victim, TO_VICT);
           look_at_room(victim, 0);
          }
    }
 else
    {/* Trans All */
     if (!IS_GRGOD(ch))
        {send_to_char("Не стОит, я так думаю.\r\n", ch);
         return;
        }

     for (i = descriptor_list; i; i = i->next)
         if (STATE(i) == CON_PLAYING && i->character && i->character != ch)
            {victim = i->character;
             if (GET_LEVEL(victim) >= GET_LEVEL(ch) && !GET_COMMSTATE(ch))
                continue;
             act("$n растворил$u в клубах дыма.", FALSE, victim, 0, 0, TO_ROOM);
             char_from_room(victim);
             char_to_room(victim, ch->in_room);
             check_horse(victim);
             act("$n появил$u, окутанн$w розовым туманом.", FALSE, victim, 0, 0, TO_ROOM);
             act("$n призвал$g Вас !", FALSE, ch, 0, victim, TO_VICT);
             look_at_room(victim, 0);
            }
     send_to_char(OK, ch);
    }
}



ACMD(do_teleport)
{
  struct char_data *victim;
  room_rnum target;

  two_arguments(argument, buf, buf2);

  if (!*buf)
     send_to_char("Кого Вы хотите переместить ?\r\n", ch);
  else
  if (!(victim = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
     send_to_char(NOPERSON, ch);
  else
  if (victim == ch)
     send_to_char("Используйте 'прыжок' для собственного перемещения.\r\n", ch);
  else
  if (GET_LEVEL(victim) >= GET_LEVEL(ch) && !GET_COMMSTATE(ch))
     send_to_char("Попробуйте придумать что-то другое.\r\n", ch);
  else
  if (!*buf2)
     act("Куда Вы хотите $S переместить ?", FALSE, ch, 0, victim, TO_CHAR);
  else
  if ((target = find_target_room(ch, buf2)) >= 0)
     {send_to_char(OK, ch);
      act("$n растворил$u в клубах дыма.", FALSE, victim, 0, 0, TO_ROOM);
      char_from_room(victim);
      char_to_room(victim, target);
      check_horse(victim);
      act("$n появил$u, окутанн$w розовым туманом.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n переместил$g Вас !", FALSE, ch, 0, (char *) victim, TO_VICT);
      look_at_room(victim, 0);
     }
}



ACMD(do_vnum)
{
  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj")))
     {send_to_char("Usage: vnum { obj | mob } <name>\r\n", ch);
      return;
     }
  if (is_abbrev(buf, "mob"))
    if (!vnum_mobile(buf2, ch))
       send_to_char("Нет существа с таким именем.\r\n", ch);

  if (is_abbrev(buf, "obj"))
     if (!vnum_object(buf2, ch))
        send_to_char("Нет предмета с таким названием.\r\n", ch);
}



void do_stat_room(struct char_data * ch)
{
  struct extra_descr_data *desc;
  struct room_data *rm = &world[ch->in_room];
  int i, found;
  struct obj_data *j;
  struct char_data *k;

  sprintf(buf, "Комната : %s%s%s\r\n",
          CCCYN(ch, C_NRM), rm->name, CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  sprinttype(rm->sector_type, sector_types, buf2);
  sprintf(buf, "Зона: [%3d], VNum: [%s%5d%s], RNum: [%5d], Тип  сектора: %s\r\n",
  	      zone_table[rm->zone].number,
  	      CCGRN(ch, C_NRM), rm->number,CCNRM(ch, C_NRM), ch->in_room, buf2);
  send_to_char(buf, ch);

  sprintbits(rm->room_flags, room_bits, buf2, ",");
  sprintf(buf, "СпецПроцедура: %s, Флаги: %s\r\n",
	      (rm->func == NULL) ? "None" : "Exists", buf2);
  send_to_char(buf, ch);

  send_to_char("Описание:\r\n", ch);
  if (rm->description)
     send_to_char(rm->description, ch);
  else
     send_to_char("  Нет.\r\n", ch);

  if (rm->ex_description)
     {sprintf(buf, "Доп. описание:%s", CCCYN(ch, C_NRM));
      for (desc = rm->ex_description; desc; desc = desc->next)
          {strcat(buf, " ");
           strcat(buf, desc->keyword);
          }
      strcat(buf, CCNRM(ch, C_NRM));
      send_to_char(strcat(buf, "\r\n"), ch);
     }
  sprintf(buf, "Живые существа:%s", CCYEL(ch, C_NRM));
  for (found = 0, k = rm->people; k; k = k->next_in_room)
      {if (!CAN_SEE(ch, k))
          continue;
       sprintf(buf2, "%s %s(%s)", found++ ? ",\
               " : "",GET_NAME(k),(!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
       strcat(buf, buf2);
       if (strlen(buf) >= 62)
          {if (k->next_in_room)
	          send_to_char(strcat(buf, ",\r\n"), ch);
           else
	          send_to_char(strcat(buf, "\r\n"), ch);
           *buf = found = 0;
          }
      }

  if (*buf)
     send_to_char(strcat(buf, "\r\n"), ch);
  send_to_char(CCNRM(ch, C_NRM), ch);

  if (rm->contents)
     {sprintf(buf, "Предметы:%s", CCGRN(ch, C_NRM));
      for (found = 0, j = rm->contents; j; j = j->next_content)
          {if (!CAN_SEE_OBJ(ch, j))
	          continue;
           sprintf(buf2, "%s %s", found++ ? "," : "", j->short_description);
           strcat(buf, buf2);
           if (strlen(buf) >= 62)
              {if (j->next_content)
	              send_to_char(strcat(buf, ",\r\n"), ch);
	           else
	              send_to_char(strcat(buf, "\r\n"), ch);
	           *buf = found = 0;
              }
          }

      if (*buf)
         send_to_char(strcat(buf, "\r\n"), ch);
      send_to_char(CCNRM(ch, C_NRM), ch);
     }
  for (i = 0; i < NUM_OF_DIRS; i++)
      {if (rm->dir_option[i])
          {if (rm->dir_option[i]->to_room == NOWHERE)
	          sprintf(buf1, " %sNONE%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
           else
	          sprintf(buf1, "%s%5d%s", CCCYN(ch, C_NRM),
		   GET_ROOM_VNUM(rm->dir_option[i]->to_room), CCNRM(ch, C_NRM));
           sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
           sprintf(buf, "Выход %s%-5s%s:  Ведет в : [%s], Ключ: [%5d], Название: %s, Тип: %s\r\n ",
	                    CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM),
	                    buf1, rm->dir_option[i]->key,
	                    rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "Нет(дверь)",
	                    buf2);
           send_to_char(buf, ch);
           if (rm->dir_option[i]->general_description)
	          strcpy(buf, rm->dir_option[i]->general_description);
           else
	          strcpy(buf, "  Нет описания выхода.\r\n");
           send_to_char(buf, ch);
          }
      }
 /* check the room for a script */
 do_sstat_room(ch);
}



void do_stat_object(struct char_data * ch, struct obj_data * j)
{
  int i, found;
  obj_vnum rnum,vnum;
  struct obj_data *j2;
  struct extra_descr_data *desc;

  vnum = GET_OBJ_VNUM(j);
  rnum = GET_OBJ_RNUM(j);
  sprintf(buf, "Название: '%s%s%s', Синонимы: %s\r\n",
          CCYEL(ch, C_NRM), ((j->short_description) ? j->short_description : "<None>"),
	      CCNRM(ch, C_NRM), j->name);
  send_to_char(buf, ch);
  sprinttype(GET_OBJ_TYPE(j), item_types, buf1);
  if (GET_OBJ_RNUM(j) >= 0)
     strcpy(buf2, (obj_index[GET_OBJ_RNUM(j)].func ? "Есть" : "Нет"));
  else
     strcpy(buf2, "None");
  sprintf(buf, "VNum: [%s%5d%s], RNum: [%5d], Тип: %s, СрецПроцедура: %s\r\n",
          CCGRN(ch, C_NRM), vnum, CCNRM(ch, C_NRM),
          GET_OBJ_RNUM(j), buf1, buf2);
  send_to_char(buf, ch);

  if (GET_OBJ_OWNER(j))
     {sprintf(buf,"Владелец : %d",GET_OBJ_OWNER(j));
      send_to_char(buf,ch);
     }

  sprintf(buf, "L-Des: %s\r\n", ((j->description) ? j->description : "Нет"));
  send_to_char(buf, ch);

  if (j->ex_description)
     {sprintf(buf, "Экстра описание:%s", CCCYN(ch, C_NRM));
      for (desc = j->ex_description; desc; desc = desc->next)
          {strcat(buf, " ");
           strcat(buf, desc->keyword);
          }
      strcat(buf, CCNRM(ch, C_NRM));
      send_to_char(strcat(buf, "\r\n"), ch);
     }
  send_to_char("Может быть одет : ", ch);
  sprintbit(j->obj_flags.wear_flags, wear_bits, buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  send_to_char("Неудобства : ", ch);
  sprintbits(j->obj_flags.no_flag, no_bits, buf, ",");
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  send_to_char("Запреты : ", ch);
  sprintbits(j->obj_flags.anti_flag, anti_bits, buf, ",");
  strcat(buf, "\r\n");
  send_to_char(buf, ch);



  send_to_char("Устанавливает аффекты : ", ch);
  sprintbits(j->obj_flags.affects, weapon_affects, buf, ",");
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  send_to_char("Дополнительные флаги  : ", ch);
  sprintbits(j->obj_flags.extra_flags, extra_bits, buf, ",");
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  sprintf(buf, "Вес: %d, Цена: %d, Рента(eq): %d, Рента(inv): %d, Таймер: %d\r\n",
     GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENTEQ(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j));
  send_to_char(buf, ch);

  strcpy(buf, "Находится : ");
  if (j->in_room == NOWHERE)
    strcat(buf, "нигде");
  else
     {sprintf(buf2, "%d", GET_ROOM_VNUM(IN_ROOM(j)));
      strcat(buf, buf2);
     }
  /*
   * NOTE: In order to make it this far, we must already be able to see the
   *       character holding the object. Therefore, we do not need CAN_SEE().
   */
  strcat(buf, ", В контенйнере: ");
  strcat(buf, j->in_obj ? j->in_obj->short_description : "Нет");
  strcat(buf, ", В инвенторе: ");
  strcat(buf, j->carried_by ? GET_NAME(j->carried_by) : "Нет");
  strcat(buf, ", Одет: ");
  strcat(buf, j->worn_by ? GET_NAME(j->worn_by) : "Нет");
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  switch (GET_OBJ_TYPE(j))
     {
  case ITEM_LIGHT:
    if (GET_OBJ_VAL(j, 2) < 0)
      strcpy(buf, "Вечный свет !");
    else
      sprintf(buf,"Осталось светить: [%d]", GET_OBJ_VAL(j, 2));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    sprintf(buf, "Заклинания: (Уровень %d) %s, %s, %s",
            GET_OBJ_VAL(j, 0),
	        spell_name(GET_OBJ_VAL(j, 1)),
	        spell_name(GET_OBJ_VAL(j, 2)),
	        spell_name(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    sprintf(buf, "Заклинание: %s уровень %d, %d (из %d) зарядов осталось",
	        spell_name(GET_OBJ_VAL(j, 3)),
	        GET_OBJ_VAL(j, 0),
	        GET_OBJ_VAL(j, 2),
	        GET_OBJ_VAL(j, 1));
    break;
  case ITEM_WEAPON:
    sprintf(buf, "Повреждения: %dd%d, Тип повреждения: %d",
	        GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_ARMOR:
    sprintf(buf, "AC: [%d]  Броня: [%d]",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
    break;
  case ITEM_TRAP:
    sprintf(buf, "Spell: %d, - Hitpoints: %d",
	        GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
    break;
  case ITEM_CONTAINER:
       sprintbit(GET_OBJ_VAL(j, 1), container_bits, buf2);
       sprintf(buf, "Объем: %d, Тип ключа: %s, Номер ключа: %d, Труп: %s",
	           GET_OBJ_VAL(j, 0),
	           buf2,
	           GET_OBJ_VAL(j, 2),
	           YESNO(GET_OBJ_VAL(j, 3)));
       break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
    sprintf(buf, "Обьем: %d, Содержит: %d, Отравлен: %s, Жидкость: %s",
	        GET_OBJ_VAL(j, 0),
	        GET_OBJ_VAL(j, 1),
	        YESNO(GET_OBJ_VAL(j, 3)),
	        buf2);
    break;
  case ITEM_NOTE:
    sprintf(buf, "Tongue: %d", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_KEY:
    strcpy(buf, "");
    break;
  case ITEM_FOOD:
    sprintf(buf, "Насыщает(час): %d, Отравлен: %s",
            GET_OBJ_VAL(j, 0),
	        YESNO(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_MONEY:
    sprintf(buf, "Монет: %d", GET_OBJ_VAL(j, 0));
    break;

  case ITEM_INGRADIENT:
      sprintbit(GET_OBJ_SKILL(j),ingradient_bits,buf2);
      sprintf(buf,"%s\r\n",buf2);
      send_to_char(buf,ch);

      if (IS_SET(GET_OBJ_SKILL(j),ITEM_CHECK_USES))
         {sprintf(buf,"можно применить %d раз\r\n",GET_OBJ_VAL(j,2));
          send_to_char(buf,ch);
         }

      if (IS_SET(GET_OBJ_SKILL(j),ITEM_CHECK_LAG))
         {sprintf(buf,"можно применить 1 раз в %d сек",(i = GET_OBJ_VAL(j,0) & 0xFF));
          if (GET_OBJ_VAL(j,3) == 0 ||
              GET_OBJ_VAL(j,3) + i < time(NULL))
             strcat(buf,"(можно применять).\r\n");
          else
             sprintf(buf + strlen(buf),"(осталось %ld сек).\r\n",GET_OBJ_VAL(j,3) + i - time(NULL));
          send_to_char(buf,ch);
         }

      if (IS_SET(GET_OBJ_SKILL(j),ITEM_CHECK_LEVEL))
         {sprintf(buf,"можно применить c %d уровня.\r\n",(GET_OBJ_VAL(j,0) >> 8) & 0x1F);
          send_to_char(buf,ch);
         }

      if ((i = real_object(GET_OBJ_VAL(j,1))) >= 0)
         {sprintf(buf,"прототип %s%s%s.\r\n",
                      CCICYN(ch, C_NRM),
                      (obj_proto+i)->PNames[0],
                      CCNRM(ch,C_NRM));
          send_to_char(buf,ch);
         }
      break;

  default:
    sprintf(buf, "Values 0-3: [%d] [%d] [%d] [%d]",
	        GET_OBJ_VAL(j, 0),
	        GET_OBJ_VAL(j, 1),
	        GET_OBJ_VAL(j, 2),
	        GET_OBJ_VAL(j, 3));
    break;
  }
  send_to_char(strcat(buf, "\r\n"), ch);

  /*
   * I deleted the "equipment status" code from here because it seemed
   * more or less useless and just takes up valuable screen space.
   */

  if (j->contains)
     {sprintf(buf, "\r\nСодержит:%s", CCGRN(ch, C_NRM));
      for (found = 0, j2 = j->contains; j2; j2 = j2->next_content)
          {sprintf(buf2, "%s %s", found++ ? "," : "", j2->short_description);
           strcat(buf, buf2);
           if (strlen(buf) >= 62)
              {if (j2->next_content)
	              send_to_char(strcat(buf, ",\r\n"), ch);
	           else
	              send_to_char(strcat(buf, "\r\n"), ch);
	           *buf = found = 0;
              }
          }

      if (*buf)
         send_to_char(strcat(buf, "\r\n"), ch);
      send_to_char(CCNRM(ch, C_NRM), ch);
     }
  found = 0;
  send_to_char("Аффекты:", ch);
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (j->affected[i].modifier)
         {sprinttype(j->affected[i].location, apply_types, buf2);
          sprintf(buf, "%s %+d to %s", found++ ? "," : "",
	      j->affected[i].modifier, buf2);
          send_to_char(buf, ch);
         }
  if (!found)
    send_to_char(" Нет", ch);

  send_to_char("\r\n", ch);
  sprintf(buf,"Всего в мире : %d/%d\r\n",
          rnum >= 0 ? obj_index[rnum].number : -1,
	  rnum >= 0 ? obj_index[rnum].stored : -1);
  send_to_char(buf,ch);	
  /* check the object for a script */
  do_sstat_object(ch, j);

}


void do_stat_character(struct char_data * ch, struct char_data * k)
{
  int i, i2, found = 0;
  struct obj_data *j;
  struct follow_type *fol;
  struct affected_type *aff;

  sprinttype(GET_SEX(k), genders, buf);

  sprintf(buf2, " %s '%s'  IDNum: [%5ld], В комнате [%5d]\r\n",
	      (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
	      GET_NAME(k), GET_IDNUM(k), GET_ROOM_VNUM(IN_ROOM(k)));
  send_to_char(strcat(buf, buf2), ch);
  if (IS_MOB(k))
     { sprintf(buf, "Синонимы: %s, VNum: [%5d], RNum: [%5d]\r\n",
	           k->player.name, GET_MOB_VNUM(k), GET_MOB_RNUM(k));
       send_to_char(buf, ch);
     }

  sprintf(buf, "Падежди: %s/%s/%s/%s/%s/%s\r\n",
          GET_PAD(k,0),GET_PAD(k,1),GET_PAD(k,2),GET_PAD(k,3),GET_PAD(k,4),GET_PAD(k,5));
  send_to_char(buf, ch);

  if (!IS_NPC(k))
     {sprintf(buf,"E-mail : %s\r\n"
                  "Unique : %d\r\n",
                  GET_EMAIL(k), GET_UNIQUE(k));
      send_to_char(buf,ch);
      if (GET_REMORT(k))
         {sprintf(buf,"Перевоплощений: %d\r\n",GET_REMORT(k));
          send_to_char(buf,ch);
         }
      if (PLR_FLAGGED(k, PLR_HELLED) && HELL_DURATION(k))
         {sprintf(buf, "Находиться в темнице : %ld час.\r\n",
	          (HELL_DURATION(k) - time(NULL)) / 3600);
          send_to_char(buf, ch);
	 }
      if (PLR_FLAGGED(k, PLR_NAMED) && NAME_DURATION(k))
         {sprintf(buf, "Находиться в комнате имени : %ld час.\r\n",
	          (NAME_DURATION(k) - time(NULL)) / 3600);
          send_to_char(buf, ch);
	 }
      if (PLR_FLAGGED(k, PLR_NOSHOUT) && MUTE_DURATION(k))
         {sprintf(buf, "Будет молчать : %ld час.\r\n",
	          (MUTE_DURATION(k) - time(NULL)) / 3600);
          send_to_char(buf, ch);
	 }
      if (GET_GOD_FLAG(k, GF_GODSLIKE) && GODS_DURATION(k))
         {sprintf(buf, "Под защитой Богов : %ld час.\r\n",
	          (GODS_DURATION(k) - time(NULL)) / 3600);
          send_to_char(buf, ch);
	 }
      if (GET_GOD_FLAG(k, GF_GODSCURSE) && GODS_DURATION(k))
         {sprintf(buf, "Проклят Богами : %ld час.\r\n",
	          (GODS_DURATION(k) - time(NULL)) / 3600);
          send_to_char(buf, ch);
	 }
     }

  sprintf(buf, "Титул: %s\r\n", (k->player.title ? k->player.title : "<Нет>"));
  send_to_char(buf, ch);

  sprintf(buf, "L-Des: %s", (k->player.long_descr ? k->player.long_descr : "<Нет>\r\n"));
  send_to_char(buf, ch);

  if (IS_NPC(k))
     {/* Use GET_CLASS() macro? */
      strcpy(buf, "Тип монстра: ");
      sprinttype(k->player.chclass, npc_class_types, buf2);
     }
  else
     {strcpy(buf, "Профессия: ");
      sprinttype(k->player.chclass, pc_class_types, buf2);
     }
  strcat(buf, buf2);

  sprintf(buf2, ", Уровень: [%s%2d%s], Опыт: [%s%10ld%s], Наклонности: [%4d]\r\n",
	      CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
	      CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM),
	      GET_ALIGNMENT(k));
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (!IS_NPC(k))
     {strcpy(buf1, (char *) asctime(localtime(&(k->player.time.birth))));
      strcpy(buf2, (char *) asctime(localtime(&(k->player.time.logon))));
      buf1[10] = buf2[10] = '\0';

      sprintf(buf, "Создан: [%s], Последний вход: [%s], Играл [%dh %dm], Возраст [%d]\r\n",
	          buf1, buf2, k->player.time.played / 3600,
	          ((k->player.time.played % 3600) / 60), age(k)->year);
      send_to_char(buf, ch);

      sprintf(buf, "На постое: [%d], Говорил: [%d/%d/%d]",
 	          k->player.hometown, GET_TALK(k, 0), GET_TALK(k, 1), GET_TALK(k, 2));
    /*. Display OLC zone for immorts .*/
      if(GET_LEVEL(k) >= LVL_IMMORT)
         sprintf(buf, "%s, OLC[%d]", buf, GET_OLC_ZONE(k));
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
     }
  sprintf(buf, "Сила: [%s%d/%d%s]  Инт: [%s%d/%d%s]  Мудр: [%s%d/%d%s]  "
  	           "Ловк: [%s%d/%d%s]  Тело:[%s%d/%d%s]  Обаян:[%s%d/%d%s] Размер: [%s%d/%d%s]\r\n",
	  CCCYN(ch, C_NRM), GET_STR(k), GET_REAL_STR(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_INT(k), GET_REAL_INT(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_WIS(k), GET_REAL_WIS(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_DEX(k), GET_REAL_DEX(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_CON(k), GET_REAL_CON(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_CHA(k), GET_REAL_CHA(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_SIZE(k),GET_REAL_SIZE(k),CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  sprintf(buf, "Жизни :[%s%d/%d+%d%s]  Энергии :[%s%d/%d+%d%s]\r\n",
	  CCGRN(ch, C_NRM), GET_HIT(k), GET_REAL_MAX_HIT(k), hit_gain(k), CCNRM(ch, C_NRM),
	  CCGRN(ch, C_NRM), GET_MOVE(k), GET_REAL_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  sprintf(buf, "Денег: [%9d], В банке: [%9ld] (Всего: %ld)\r\n",
	      GET_GOLD(k), GET_BANK_GOLD(k), GET_GOLD(k) + GET_BANK_GOLD(k));
  send_to_char(buf, ch);

  sprintf(buf, "AC: [%d/%d(%d)], Броня: [%d], Hitroll: [%2d/%2d/%d], Damroll: [%2d/%2d/%d], Saving throws: [%d/%d/%d/%d/%d/%d]\r\n",
	      GET_AC(k), GET_REAL_AC(k), compute_armor_class(k),
	      GET_ARMOUR(k),
	      GET_HR(k), GET_REAL_HR(k), GET_REAL_HR(k) + str_app[GET_REAL_STR(k)].tohit,
	      GET_DR(k), GET_REAL_DR(k), GET_REAL_DR(k) + str_app[GET_REAL_STR(k)].todam,
	      GET_SAVE(k, 0), GET_SAVE(k, 1), GET_SAVE(k, 2),
	      GET_SAVE(k, 3), GET_SAVE(k, 4), GET_SAVE(k, 5));
  send_to_char(buf, ch);

  sprinttype(GET_POS(k), position_types, buf2);
  sprintf(buf, "Положение: %s, Сражается: %s", buf2,
	      (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Нет"));

  if (IS_NPC(k))
     {strcat(buf, ", Тип атаки: ");
      strcat(buf, attack_hit_text[k->mob_specials.attack_type].singular);
     }
  if (k->desc)
     {sprinttype(STATE(k->desc), connected_types, buf2);
      strcat(buf, ", Соединение: ");
      strcat(buf, buf2);
     }
  send_to_char(strcat(buf, "\r\n"), ch);

  strcpy(buf, "Позиция по умолчанию: ");
  sprinttype((k->mob_specials.default_pos), position_types, buf2);
  strcat(buf, buf2);

  sprintf(buf2, ", Таймер отсоединения (тиков) [%d]\r\n", k->char_specials.timer);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (IS_NPC(k))
     {sprintbits(k->char_specials.saved.act, action_bits, buf2, ",");
      sprintf(buf, "NPC флаги: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
      send_to_char(buf, ch);
     }
  else
     {sprintbits(k->char_specials.saved.act, player_bits, buf2, ",");
      sprintf(buf, "PLR: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
      send_to_char(buf, ch);
      sprintbits(k->player_specials->saved.pref,preference_bits, buf2, ",");
      sprintf(buf, "PRF: %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
      send_to_char(buf, ch);
     }

  if (IS_MOB(k))
     {sprintf(buf, "Mob СпецПроц: %s, NPC сила удара: %dd%d\r\n",
	          (mob_index[GET_MOB_RNUM(k)].func ? "Есть" : "Нет"),
	          k->mob_specials.damnodice, k->mob_specials.damsizedice);
      send_to_char(buf, ch);
     }
  sprintf(buf, "Несет - вес %d, предметов %d; ",
	      IS_CARRYING_W(k), IS_CARRYING_N(k));

  for (i = 0, j = k->carrying; j; j = j->next_content, i++);
      sprintf(buf + strlen(buf), "(в инвентаре) : %d, ", i);

  for (i = 0, i2 = 0; i < NUM_WEARS; i++)
      if (GET_EQ(k, i))
         i2++;
  sprintf(buf2, "(одето): %d\r\n", i2);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (!IS_NPC(k))
     {sprintf(buf, "Голод: %d, Жажда: %d, Опьянение: %d\r\n",
	          GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
      send_to_char(buf, ch);
     }

  sprintf(buf, "Ведущий: %s, Ведомые:",
	      ((k->master) ? GET_NAME(k->master) : "<нет>"));

  for (fol = k->followers; fol; fol = fol->next)
      {sprintf(buf2, "%s %s", found++ ? "," : "", PERS(fol->follower, ch, 0));
       strcat(buf, buf2);
       if (strlen(buf) >= 62)
          {if (fol->next)
	          send_to_char(strcat(buf, ",\r\n"), ch);
           else
	          send_to_char(strcat(buf, "\r\n"), ch);
          *buf = found = 0;
         }
      }

  if (*buf)
     send_to_char(strcat(buf, "\r\n"), ch);

  /* Showing the bitvector */
  sprintbits(k->char_specials.saved.affected_by, affected_bits, buf2, ",");
  sprintf(buf, "Аффекты: %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  /* Routine to show what spells a char is affected by */
  if (k->affected)
     {for (aff = k->affected; aff; aff = aff->next)
          {*buf2 = '\0';
           sprintf(buf, "Заклинания: (%3dhr) %s%-21s%s ", aff->duration + 1,
	               CCCYN(ch, C_NRM), spell_name(aff->type), CCNRM(ch, C_NRM));
           if (aff->modifier)
              {sprintf(buf2, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
	           strcat(buf, buf2);
              }
           if (aff->bitvector)
              {if (*buf2)
	              strcat(buf, ", sets ");
	           else
	              strcat(buf, "sets ");
               sprintbit(aff->bitvector, affected_bits, buf2);
	           strcat(buf, buf2);
              }
           send_to_char(strcat(buf, "\r\n"), ch);
          }
     }

  /* check mobiles for a script */
  if (IS_NPC(k))
     {do_sstat_character(ch, k);
      if (MEMORY(k))
         {struct memory_rec_struct *memchar;
	  send_to_char("Помнит:\r\n",ch);
	  for (memchar = MEMORY(k); memchar; memchar = memchar->next)
	      {sprintf(buf,"%10ld - %10ld\r\n",memchar->id,memchar->time);
	       send_to_char(buf,ch);
	      }
	 }
      if (SCRIPT_MEM(k))
         {struct script_memory *mem = SCRIPT_MEM(k);
          send_to_char("Память (скрипт):\r\n  Игрок                Команда\r\n", ch);
          while (mem)
                {struct char_data *mc = find_char(mem->id);
                 if (!mc)
                    send_to_char("  ** Разрушено!\r\n", ch);
                 else
                    {if (mem->cmd)
                        sprintf(buf,"  %-20.20s%s\r\n",GET_NAME(mc),mem->cmd);
                     else
                        sprintf(buf,"  %-20.20s <default>\r\n",GET_NAME(mc));
                     send_to_char(buf, ch);
                    }
                 mem = mem->next;
                }
         }
     }
  else
     {/* this is a PC, display their global variables */
      struct char_data      *tch;
      struct PK_Memory_type *pk;
      int    found;
      
      if (k->script && k->script->global_vars)
         {struct trig_var_data *tv;
          char name[MAX_INPUT_LENGTH];
          void find_uid_name(char *uid, char *name);
           send_to_char("Глобальные переменные:\r\n", ch);
          /* currently, variable context for players is always 0, so it is */
          /* not displayed here. in the future, this might change */
          for (tv = k->script->global_vars; tv; tv = tv->next)
              {if (*(tv->value) == UID_CHAR)
                  {find_uid_name(tv->value, name);
                   sprintf(buf, "    %10s:  [UID]: %s\r\n", tv->name, name);
                  }
               else
                  sprintf(buf, "    %10s:  %s\r\n", tv->name, tv->value);
               send_to_char(buf, ch);
              }
         }
      if (k->Questing.count)
         {send_to_char("Выполнил квесты NN : \r\n",ch);
          *buf = '\0';
          for (i = 0; i < k->Questing.count && strlen(buf) + 80 < MAX_STRING_LENGTH; i++)
              sprintf(buf + strlen(buf), "%-8d", k->Questing.quests[i]);
          strcat(buf,"\r\n");
          send_to_char(buf,ch);
         }
      /*
      if (k->MobKill.count)
         {send_to_char("Убил мобов NN : \r\n",ch);
          *buf = '\0';
          for (i = 0; i < k->MobKill.count && strlen(buf) + 80 < MAX_STRING_LENGTH; i++)
              sprintf(buf + strlen(buf), "%-5d[%-5d]", k->MobKill.vnum[i], k->MobKill.howmany[i]);
          strcat(buf,"\r\n");
          send_to_char(buf,ch);
         }
       */
      for (pk = k->pk_list, found = FALSE; pk; pk = pk->next)
          {if (!found)
              send_to_char("ПК список:\r\n",ch);
           found = TRUE;              
           for (tch = character_list; tch; tch = tch->next)
               {if (IS_NPC(ch))
                   continue;
                if (pk->unique == GET_UNIQUE(tch))
                   {sprintf(buf,"%18s : ",GET_NAME(tch));
                    break;
                   }
               }
           if (!tch)               
              sprintf(buf,"Player %10d : ",pk->unique);
           sprintf(buf+strlen(buf)," Kills %3d  Thiefs %3d Revenge %3d\r\n",
                   pk->pkills, pk->pthief, pk->revenge);
           send_to_char(buf,ch);
          }
     }
}


ACMD(do_stat)
{
  struct char_data *victim;
  struct obj_data *object;
  struct char_file_u tmp_store;
  int tmp;

  half_chop(argument, buf1, buf2);

  if (!*buf1)
     {send_to_char("Состояние КОГО или ЧЕГО ?\r\n", ch);
      return;
     }
  else
  if (is_abbrev(buf1, "room"))
     {do_stat_room(ch);
     }
  else
  if (is_abbrev(buf1, "mob"))
     {if (!*buf2)
         send_to_char("Состояние какого создания ?\r\n", ch);
      else
         {if ((victim = get_char_vis(ch, buf2, FIND_CHAR_WORLD)) != NULL)
	         do_stat_character(ch, victim);
          else
	         send_to_char("Нет такого создания в этом МАДе.\r\n", ch);
         }
     }
  else
  if (is_abbrev(buf1, "player"))
     {if (!*buf2)
         {send_to_char("Состояние какого игрока ?\r\n", ch);
         }
      else
         {if ((victim = get_player_vis(ch, buf2, FIND_CHAR_WORLD)) != NULL)
	         do_stat_character(ch, victim);
          else
	         send_to_char("Этого персонажа сейчас нет в игре.\r\n", ch);
         }
     }
  else
  if (is_abbrev(buf1, "file"))
     {if (!*buf2)
         {send_to_char("Состояние какого игрока(из файла) ?\r\n", ch);
         }
      else
         {CREATE(victim, struct char_data, 1);
          clear_char(victim);
          if (load_char(buf2, &tmp_store) > -1)
             {store_to_char(&tmp_store, victim);
	      load_pkills(victim);
	      victim->player.time.logon = tmp_store.last_logon;
	      char_to_room(victim, 0);
	      if (GET_LEVEL(victim) > GET_LEVEL(ch) && !GET_COMMSTATE(ch))
	         send_to_char("Извините, Вам это еще рано.\r\n", ch);
	      else
	         do_stat_character(ch, victim);
	      extract_char(victim,FALSE);
	     }
          else
             send_to_char("Такого игрока нет ВООБЩЕ.\r\n", ch);
	  free(victim);
         }
     }
  else
  if (is_abbrev(buf1, "object"))
     {if (!*buf2)
         send_to_char("Состояние какого предмета ?\r\n", ch);
      else
         {if ((object = get_obj_vis(ch, buf2)) != NULL)
	         do_stat_object(ch, object);
          else
	         send_to_char("Нет такого предмета в игре.\r\n", ch);
         }
     }
  else
     {if ((object = get_object_in_equip_vis(ch, buf1, ch->equipment, &tmp)) != NULL)
         do_stat_object(ch, object);
      else
      if ((object = get_obj_in_list_vis(ch, buf1, ch->carrying)) != NULL)
         do_stat_object(ch, object);
      else
      if ((victim = get_char_vis(ch, buf1, FIND_CHAR_ROOM)) != NULL)
         do_stat_character(ch, victim);
      else
      if ((object = get_obj_in_list_vis(ch, buf1, world[ch->in_room].contents)) != NULL)
         do_stat_object(ch, object);
      else
      if ((victim = get_char_vis(ch, buf1, FIND_CHAR_WORLD)) != NULL)
         do_stat_character(ch, victim);
      else
      if ((object = get_obj_vis(ch, buf1)) != NULL)
         do_stat_object(ch, object);
      else
         send_to_char("Ничего похожего с этим именем нет.\r\n", ch);
     }
}


ACMD(do_shutdown)
{ int times = 0;
  if (subcmd != SCMD_SHUTDOWN)
     {send_to_char("Если Вы хотите SHUT что-то DOWN, так и скажите !\r\n", ch);
      return;
     }
  two_arguments(argument, arg, buf);
  shutdown_time = 0;
  if (buf && *buf && (times = atoi(buf)) > 0)
     {shutdown_time = time(NULL) + times;
     }

  if (!*arg)
     {log("(GC) Shutdown by %s.", GET_NAME(ch));
      if (!times)
         send_to_all("ОСТАНОВКА.\r\n");
      else
         {sprintf(buf,"[ОСТАНОВКА через %d %s]\r\n",times,desc_count(times,WHAT_SEC));
          send_to_all(buf);
         };
      circle_shutdown = 1;
     }
  else
  if (!str_cmp(arg, "reboot"))
     {log("(GC) Reboot by %s.", GET_NAME(ch));
      if (!times)
         send_to_all("ПЕРЕЗАГРУЗКА.\r\n");
      else
         {sprintf(buf,"[ПЕРЕЗАГРУЗКА через %d %s]\r\n",times,desc_count(times,WHAT_SEC));
          send_to_all(buf);
         };
      touch(FASTBOOT_FILE);
      circle_shutdown = circle_reboot = 1;
     }
  else
  if (!str_cmp(arg, "now"))
     {sprintf(buf, "(GC) Shutdown NOW by %s.", GET_NAME(ch));
      log(buf);
      send_to_all("ПЕРЕЗАГРУЗКА.. Вернетесь через пару минут.\r\n");
      shutdown_time   = 0;
      circle_shutdown = 1;
      circle_reboot   = 2;
     }
  else
  if (!str_cmp(arg, "die"))
     {log("(GC) Shutdown by %s.", GET_NAME(ch));
      send_to_all("ОСТАНОВКА.\r\n");
      if (!times)
         send_to_all("ОСТАНОВКА.\r\n");
      else
         {sprintf(buf,"[ОСТАНОВКА через %d %s]\r\n",times,desc_count(times,WHAT_SEC));
          send_to_all(buf);
         };
      touch(KILLSCRIPT_FILE);
      circle_shutdown = 1;
     }
  else
  if (!str_cmp(arg, "pause"))
     {log("(GC) Shutdown by %s.", GET_NAME(ch));
      if (!times)
         send_to_all("ОСТАНОВКА.\r\n");
      else
         {sprintf(buf,"[ОСТАНОВКА через %d %s]\r\n",times,desc_count(times,WHAT_SEC));
          send_to_all(buf);
         };
      touch(PAUSE_FILE);
      circle_shutdown = 1;
     }
  else
     send_to_char("Не знаю, чего Вы конкретно хотите.\r\n", ch);
}


void stop_snooping(struct char_data * ch)
{
  if (!ch->desc->snooping)
     send_to_char("Вы не подслушиваете.\r\n", ch);
  else
     {send_to_char("Вы прекратили подслушивать.\r\n", ch);
      ch->desc->snooping->snoop_by = NULL;
      ch->desc->snooping = NULL;
     }
}


ACMD(do_snoop)
{
  struct char_data *victim, *tch;

  if (!ch->desc)
     return;

  one_argument(argument, arg);

  if (!*arg)
     stop_snooping(ch);
  else
  if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
     send_to_char("Нет такого создания в игре.\r\n", ch);
  else
  if (!victim->desc)
     act("Вы не можете $S подслушать - он$G потерял$G связь..\r\n",FALSE,ch,0,victim,TO_CHAR);
  else
  if (victim == ch)
     stop_snooping(ch);
  else
  if (victim->desc->snooping == ch->desc)
     send_to_char("Вы уже подслушиваете.\r\n", ch);
  else
  if (victim->desc->snoop_by)
     send_to_char("Дык его уже кто-то из богов подслушивает.\r\n", ch);
  else {if (victim->desc->original)
           tch = victim->desc->original;
        else
           tch = victim;

        if (GET_LEVEL(tch) >= GET_LEVEL(ch) && !GET_COMMSTATE(ch))
           {send_to_char("Вы не можете.\r\n", ch);
            return;
           }
        send_to_char(OK, ch);

        if (ch->desc->snooping)
           ch->desc->snooping->snoop_by = NULL;

        ch->desc->snooping = victim->desc;
        victim->desc->snoop_by = ch->desc;
       }
}



ACMD(do_switch)
{
  struct char_data *victim;

  one_argument(argument, arg);

  if (ch->desc->original)
     send_to_char("Вы уже в чьем-то теле.\r\n", ch);
  else
  if (!*arg)
     send_to_char("Стать кем ?\r\n", ch);
  else
  if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
     send_to_char("Нет такого создания.\r\n", ch);
  else
  if (ch == victim)
     send_to_char("Вы и так им являетесь.\r\n", ch);
  else
  if (victim->desc)
     send_to_char("Это тело уже под контролем.\r\n", ch);
  else
  if (!IS_IMPL(ch) && !IS_NPC(victim))
     send_to_char("Вы не столь могущественны, чтобы контроолировать тело игрока.\r\n", ch);
  else
  if (GET_LEVEL(ch) < LVL_GRGOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_GODROOM))
     send_to_char("Вы не можете находиться в той комнате.\r\n", ch);
  else
  if (!IS_GRGOD(ch) &&
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_HOUSE) &&
      !House_can_enter(ch, GET_ROOM_VNUM(IN_ROOM(victim)))
     )
     send_to_char("Вы не сможете проникнуть на частную территорию.\r\n", ch);
  else
     {send_to_char(OK, ch);

      ch->desc->character = victim;
      ch->desc->original = ch;

      victim->desc = ch->desc;
      ch->desc = NULL;
     }
}


ACMD(do_return)
{
  if (ch->desc && ch->desc->original)
     {send_to_char("Вы вернулись в свое тело.\r\n", ch);

   /*
    * If someone switched into your original body, disconnect them.
    *   - JE 2/22/95
    *
    * Zmey: here we put someone switched in our body to disconnect state
    * but we must also NULL his pointer to our character, otherwise
    * close_socket() will damage our character's pointer to our descriptor
    * (which is assigned below in this function). 12/17/99
    */
      if (ch->desc->original->desc)
         {ch->desc->original->desc->character = NULL;
          STATE(ch->desc->original->desc) = CON_DISCONNECT;
         }
      ch->desc->character = ch->desc->original;
      ch->desc->original = NULL;

      ch->desc->character->desc = ch->desc;
      ch->desc = NULL;
     }
}



ACMD(do_load)
{
  struct char_data *mob;
  struct obj_data *obj;
  mob_vnum number;
  mob_rnum r_num;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2))
     {send_to_char("Usage: load { obj | mob } <number>\r\n", ch);
      return;
     }
  if ((number = atoi(buf2)) < 0)
     {send_to_char("Отрицательный моб опасен для Вашего здоровья !\r\n", ch);
      return;
     }
  if (is_abbrev(buf, "mob"))
     {if ((r_num = real_mobile(number)) < 0)
         {send_to_char("Нет такого моба в этом МУДе.\r\n", ch);
          return;
         }
      mob = read_mobile(r_num, REAL);
      char_to_room(mob, ch->in_room);
      if (!GET_COMMSTATE(ch))
         {act("$n порыл$u в МУДе.", TRUE, ch,0, 0, TO_ROOM);
          act("$n создал$g $N3!", FALSE, ch, 0, mob, TO_ROOM);
         }
      act("Вы создали $N3.", FALSE, ch, 0, mob, TO_CHAR);
      load_mtrigger(mob);
     }
  else
  if (is_abbrev(buf, "obj"))
     {if ((r_num = real_object(number)) < 0)
         {send_to_char("Господи, да изучи ты номера объектов.\r\n", ch);
          return;
         }
      obj = read_object(r_num, REAL);
      if (load_into_inventory)
         obj_to_char(obj, ch);
      else
         obj_to_room(obj, ch->in_room);
      if (!GET_COMMSTATE(ch))
         {act("$n покопал$u в МУДе.", TRUE, ch, 0, 0, TO_ROOM);
          act("$n создал$g $o3!", FALSE, ch, obj, 0, TO_ROOM);
         }
      act("Вы создали $o3.", FALSE, ch, obj, 0, TO_CHAR);
      load_otrigger(obj);
     }
  else
     send_to_char("Нет уж. Ты создай че-нить нормальное.\r\n", ch);
}



ACMD(do_vstat)
{
  struct char_data *mob;
  struct obj_data *obj;
  mob_vnum number;	/* or obj_vnum ... */
  mob_rnum r_num;	/* or obj_rnum ... */

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2))
     {send_to_char("Usage: vstat { obj | mob } <number>\r\n", ch);
      return;
     }
  if ((number = atoi(buf2)) < 0)
     {send_to_char("Отрицательный номер ? Оригинально !\r\n", ch);
      return;
     }
  if (is_abbrev(buf, "mob"))
     {if ((r_num = real_mobile(number)) < 0)
         {send_to_char("Обратитесь в Арктику - там ОН живет.\r\n", ch);
          return;
         }
      mob = read_mobile(r_num, REAL);
      char_to_room(mob, 0);
      do_stat_character(ch, mob);
      extract_char(mob,FALSE);
      free_char(mob);
     }
  else
  if (is_abbrev(buf, "obj"))
     {if ((r_num = real_object(number)) < 0)
         {send_to_char("Этот предмет явно перенесли в РМУД.\r\n", ch);
          return;
         }
      obj = read_object(r_num, REAL);
      do_stat_object(ch, obj);
      extract_obj(obj);
     }
  else
     send_to_char("Тут должно быть что-то типа 'obj' или 'mob'.\r\n", ch);
}




/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  struct char_data *vict, *next_v;
  struct obj_data *obj, *next_o;

  one_argument(argument, buf);

  if (*buf)
     {/* argument supplied. destroy single object
	   * or char */
      if ((vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)) != NULL)
         {if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && !GET_COMMSTATE(ch))
             {send_to_char("Да я Вас за это...\r\n", ch);
	          return;
             }
          if (!GET_COMMSTATE(ch))
             act("$n обратил$g в прах $N3.", FALSE, ch, 0, vict, TO_NOTVICT);
          if (!IS_NPC(vict))
             {sprintf(buf, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
              mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
	      if (vict->desc)
	         {STATE(vict->desc) = CON_CLOSE;
	          vict->desc->character = NULL;
	          vict->desc = NULL;
	         }
             }
          extract_char(vict,FALSE);
         }
      else
      if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents)) != NULL)
         {if (!GET_COMMSTATE(ch))
             act("$n просто разметал$g $o3 на молекулы.", FALSE, ch, obj, 0, TO_ROOM);
          extract_obj(obj);
         }
      else
         {send_to_char("Ничего похожего с таким именем нет.\r\n", ch);
          return;
         }
      send_to_char(OK, ch);
     }
  else {/* no argument. clean out the room */
        if (!GET_COMMSTATE(ch))
           {act("$n произнес$q СЛОВО... Вас окружило пламя !",
	        FALSE, ch, 0, 0, TO_ROOM);
            send_to_room("Мир стал немного чище.\r\n", ch->in_room, FALSE);
           }

        for (vict = world[ch->in_room].people; vict; vict = next_v)
            {next_v = vict->next_in_room;
             if (IS_NPC(vict))
	        extract_char(vict,FALSE);
            }

        for (obj = world[ch->in_room].contents; obj; obj = next_o)
            {next_o = obj->next_content;
             extract_obj(obj);
            }
       }
}



const char *logtypes[] = {
  "нет", "краткий", "нормальный", "полный", "\n"
};

ACMD(do_syslog)
{
  int tp;

  one_argument(argument, arg);

  if (!*arg)
     {tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) +
	        (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
      sprintf(buf, "Тип Вашего системного лога сейчас %s.\r\n", logtypes[tp]);
      send_to_char(buf, ch);
      return;
     }
  if (((tp = search_block(arg, logtypes, FALSE)) == -1))
     {send_to_char("Формат: syslog { нет | краткий | нормальный | полный }\r\n", ch);
      return;
     }
  REMOVE_BIT(PRF_FLAGS(ch, PRF_LOG2), PRF_LOG2);
  REMOVE_BIT(PRF_FLAGS(ch, PRF_LOG2), PRF_LOG2);
  SET_BIT(PRF_FLAGS(ch, PRF_LOG1), (PRF_LOG1 * (tp & 1)));
  SET_BIT(PRF_FLAGS(ch, PRF_LOG2), (PRF_LOG2 * (tp & 2) >> 1));

  sprintf(buf, "Тип Вашего системного лога сейчас %s.\r\n", logtypes[tp]);
  send_to_char(buf, ch);
}



ACMD(do_advance)
{
  struct char_data *victim;
  char *name = arg, *level = buf2;
  int newlevel, oldlevel;

  two_arguments(argument, name, level);

  if (*name)
     {if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD)))
         {send_to_char("Не найду такого игрока.\r\n", ch);
          return;
         }
      }
   else
      {send_to_char("Повысить кого ?\r\n", ch);
       return;
      }

  if (GET_LEVEL(ch) <= GET_LEVEL(victim) && !GET_COMMSTATE(ch))
     {send_to_char("Нелогично.\r\n", ch);
      return;
     }
  if (IS_NPC(victim))
     {send_to_char("Нет ! Только не моба !\r\n", ch);
      return;
     }
  if (!*level || (newlevel = atoi(level)) <= 0)
     {send_to_char("Это не похоже на уровень.\r\n", ch);
      return;
     }
  if (newlevel > LVL_IMPL)
     {sprintf(buf, "%d - максимальный возможный уровень.\r\n", LVL_IMPL);
      send_to_char(buf, ch);
      return;
     }
  if (newlevel > GET_LEVEL(ch) && !GET_COMMSTATE(ch))
     {send_to_char("Вы не можете установить уровень выше собственного.\r\n", ch);
      return;
     }
  if (newlevel == GET_LEVEL(victim))
     {act("$E и так этого уровня.", FALSE, ch, 0, victim, TO_CHAR);
      return;
     }
  oldlevel = GET_LEVEL(victim);
  if (newlevel < GET_LEVEL(victim))
     {do_start(victim,FALSE);
      GET_LEVEL(victim) = newlevel;
      send_to_char("Вас окутало облако тьмы.\r\n"
	           "Вы почувствовали себя лишенным чего-то.\r\n", victim);
     }
  else
     {act("$n сделал$g несколько странных пасов.\r\n"
          "Вам показалось, будто неземное тепло разлилось по каждой клеточке\r\n"
          "Вашего тела, наполняя его доселе невиданными Вами ощущениями.\r\n",
          FALSE, ch, 0, victim, TO_VICT);
     }

  send_to_char(OK, ch);
  if (newlevel < oldlevel)
     log("(GC) %s demoted %s from level %d to %d.",
         GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
  else
     log("(GC) %s has advanced %s to level %d (from %d)",
         GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

  gain_exp_regardless(victim, level_exp(GET_CLASS(victim),newlevel)
                              - GET_EXP(victim));
  save_char(victim, NOWHERE);
}



ACMD(do_restore)
{
  struct char_data *vict;

  one_argument(argument, buf);
  if (!*buf)
     send_to_char("Кого Вы хотите восстановить ?\r\n", ch);
  else
  if (!(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
     send_to_char(NOPERSON, ch);
  else
     {GET_HIT(vict)         = GET_REAL_MAX_HIT(vict);
      GET_MOVE(vict)        = GET_REAL_MAX_MOVE(vict);
      GET_MANA_STORED(vict) = GET_MANA_NEED(vict);

      if (IS_GRGOD(ch) && IS_IMMORTAL(vict))
         {if (IS_GRGOD(vict))
             {vict->real_abils.intel = 25;
              vict->real_abils.wis   = 25;
              vict->real_abils.dex   = 25;
              vict->real_abils.str   = 25;
              vict->real_abils.con   = 25;
              vict->real_abils.cha   = 25;
             }
         }
      update_pos(vict);
      send_to_char(OK, ch);
      if (!GET_COMMSTATE(ch))
         act("Вы были полностью восстановлены $N4!", FALSE, vict, 0, ch, TO_CHAR);
     }
}


void perform_immort_vis(struct char_data *ch)
{
  if (GET_INVIS_LEV(ch) == 0 &&
      !AFF_FLAGGED(ch, AFF_HIDE) &&
      !AFF_FLAGGED(ch, AFF_INVISIBLE) &&
      !AFF_FLAGGED(ch, AFF_CAMOUFLAGE))
     {send_to_char("Ну вот Вас и заметили. Стало ли Вам легче от этого ?\r\n", ch);
      return;
     }

  GET_INVIS_LEV(ch) = 0;
  appear(ch);
  send_to_char("Вы теперь полностью видны.\r\n", ch);
}


void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch;

  if (IS_NPC(ch))
     return;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {if (tch == ch)
          continue;
       if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level)
          act("Вы вздрогнули, когда $n растворил$u на Ваших глазах.", FALSE, ch, 0,
	          tch, TO_VICT);
       if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
          act("Вы почувствовали что-то странное, когда $n пропал$g.", FALSE, ch, 0,
	          tch, TO_VICT);
      }

  GET_INVIS_LEV(ch) = level;
  sprintf(buf, "Ваш уровень невидимости - %d.\r\n", level);
  send_to_char(buf, ch);
}


ACMD(do_invis)
{
  int level;

  if (IS_NPC(ch))
     {send_to_char("Вы не можете сделать этого.\r\n", ch);
      return;
     }

  one_argument(argument, arg);
  if (!*arg)
     {if (GET_INVIS_LEV(ch) > 0)
         perform_immort_vis(ch);
      else
         perform_immort_invis(ch, GET_LEVEL(ch));
     }
  else
     {level = atoi(arg);
      if (level > GET_LEVEL(ch) && !GET_COMMSTATE(ch))
         send_to_char("Вы не можете достичь невидимости выше Вашего уровня.\r\n", ch);
      else
      if (level < 1)
         perform_immort_vis(ch);
      else
         perform_immort_invis(ch, level);
     }
}


ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
     send_to_char("Это, пожалуй, ошибка...\r\n", ch);
  else
     {sprintf(buf, "%s\r\n", argument);
      for (pt = descriptor_list; pt; pt = pt->next)
          if (STATE(pt) == CON_PLAYING && pt->character && pt->character != ch)
	         send_to_char(buf, pt->character);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(OK, ch);
      else
         send_to_char(buf, ch);
     }
}


ACMD(do_poofset)
{
  char **msg;

  switch (subcmd)
  {case SCMD_POOFIN:    msg = &(POOFIN(ch));    break;
   case SCMD_POOFOUT:   msg = &(POOFOUT(ch));   break;
   default:    return;
  }

  skip_spaces(&argument);

  if (*msg)
     free(*msg);

  if (!*argument)
     *msg = NULL;
  else
     *msg = str_dup(argument);

  send_to_char(OK, ch);
}



ACMD(do_dc)
{
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg);
  if (!(num_to_dc = atoi(arg)))
     {send_to_char("Usage: DC <user number> (type USERS for a list)\r\n", ch);
      return;
     }
  for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

  if (!d)
     {send_to_char("Нет такого соединения.\r\n", ch);
      return;
     }
  if (d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch) && !GET_COMMSTATE(ch))
     {if (!CAN_SEE(ch, d->character))
         send_to_char("Нет такого соединения.\r\n", ch);
      else
         send_to_char("Да уж.. Это не есть праффильная идея...\r\n", ch);
      return;
     }

  /* We used to just close the socket here using close_socket(), but
   * various people pointed out this could cause a crash if you're
   * closing the person below you on the descriptor list.  Just setting
   * to CON_CLOSE leaves things in a massively inconsistent state so I
   * had to add this new flag to the descriptor.
   *
   * It is a much more logical extension for a CON_DISCONNECT to be used
   * for in-game socket closes and CON_CLOSE for out of game closings.
   * This will retain the stability of the close_me hack while being
   * neater in appearance. -gg 12/1/97
   */
  if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
     send_to_char("Соединение уже разорвано.\r\n", ch);
  else
     {
     /*
      * Remember that we can disconnect people not in the game and
      * that rather confuses the code when it expected there to be
      * a character context.
      */
      if (STATE(d) == CON_PLAYING)
         STATE(d) = CON_DISCONNECT;
      else
         STATE(d) = CON_CLOSE;

      sprintf(buf, "Соединение #%d закрыто.\r\n", num_to_dc);
      send_to_char(buf, ch);
      log("(GC) Connection closed by %s.", GET_NAME(ch));
     }

  // STATE(d) = CON_DISCONNECT;
  sprintf(buf, "Соединение #%d закрыто.\r\n", num_to_dc);
  send_to_char(buf, ch);
  log("(GC) Connection closed by %s.", GET_NAME(ch));
}



ACMD(do_wizlock)
{
  int value;
  const char *when;

  one_argument(argument, arg);
  if (*arg)
     {value = atoi(arg);
      if (value < 0 || (value > GET_LEVEL(ch) && !GET_COMMSTATE(ch)))
         {send_to_char("Неверное значение для wizlock.\r\n", ch);
          return;
         }
      circle_restrict = value;
      when = "теперь";
     }
  else
     when = "в настоящее время";

  switch (circle_restrict)
     {
  case 0:
    sprintf(buf, "Игра %s полностью открыта.\r\n", when);
    break;
  case 1:
    sprintf(buf, "Игра %s закрыта для новых игроков.\r\n", when);
    break;
  default:
    sprintf(buf, "Только игроки %d %s и выше могут %s войти в игру.\r\n",
	        circle_restrict, desc_count(circle_restrict, WHAT_LEVEL), when);
    break;
     }
  send_to_char(buf, ch);
}


ACMD(do_date)
{
  char *tmstr;
  time_t mytime;
  int d, h, m;

  if (subcmd == SCMD_DATE)
     mytime = time(0);
  else
     mytime = boot_time;

  tmstr = (char *) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (subcmd == SCMD_DATE)
     sprintf(buf, "Текущее время сервера : %s\r\n", tmstr);
  else {mytime = time(0) - boot_time;
        d = mytime / 86400;
        h = (mytime / 3600) % 24;
        m = (mytime / 60) % 60;

        sprintf(buf, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d,
	            ((d == 1) ? "" : "s"), h, m);
       }

  send_to_char(buf, ch);
}



ACMD(do_last)
{
  struct char_file_u chdata;

  one_argument(argument, arg);
  if (!*arg)
     {send_to_char("Кого Вы хотите найти ?\r\n", ch);
      return;
     }
  if (load_char(arg, &chdata) < 0)
     {send_to_char("Нет такого игрока.\r\n", ch);
      return;
     }
  if (chdata.level > GET_LEVEL(ch) && !IS_IMPL(ch))
     {send_to_char("Вы не столь уж и божественны для этого.\r\n", ch);
      return;
     }
  sprintf(buf, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\r\n",
          chdata.char_specials_saved.idnum, (int) chdata.level,
          class_abbrevs[(int) chdata.chclass], chdata.name, chdata.host,
          ctime(&chdata.last_logon));
  send_to_char(buf, ch);
}


ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char to_force[MAX_INPUT_LENGTH + 2];

  half_chop(argument, arg, to_force);

  sprintf(buf1, "$n принудил$g Вас '%s'.", to_force);

  if (!*arg || !*to_force)
     send_to_char("Кого и что Вы хотите принудить сделать ?\r\n", ch);
  else
  if (!IS_GRGOD(ch) ||
      (str_cmp("all", arg) && str_cmp("room", arg) && str_cmp("все", arg) && str_cmp("здесь", arg)))
     {if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
         send_to_char(NOPERSON, ch);
      else
      if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && !GET_COMMSTATE(ch))
         send_to_char("Господи, только не это!\r\n", ch);
      else
         {send_to_char(OK, ch);
          if (!GET_COMMSTATE(ch))
             act(buf1, TRUE, ch, NULL, vict, TO_VICT);
          sprintf(buf, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
          mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
          command_interpreter(vict, to_force);
         }
     }
  else
  if (!str_cmp("room", arg) || !str_cmp("здесь", arg))
     {send_to_char(OK, ch);
      sprintf(buf, "(GC) %s forced room %d to %s",
		      GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

      for (vict = world[ch->in_room].people; vict; vict = next_force)
          {next_force = vict->next_in_room;
           if (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch) && !GET_COMMSTATE(ch))
	          continue;
           if (!GET_COMMSTATE(ch))
              act(buf1, TRUE, ch, NULL, vict, TO_VICT);
           command_interpreter(vict, to_force);
          }
     }
  else
     { /* force all */
      send_to_char(OK, ch);
      sprintf(buf, "(GC) %s forced all to %s", GET_NAME(ch), to_force);
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

      for (i = descriptor_list; i; i = next_desc)
          {next_desc = i->next;

           if (STATE(i) != CON_PLAYING || !(vict = i->character) ||
               (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch) && !GET_COMMSTATE(ch)))
	          continue;
           if (!GET_COMMSTATE(ch))
              act(buf1, TRUE, ch, NULL, vict, TO_VICT);
           command_interpreter(vict, to_force);
          }
     }
}



ACMD(do_wiznet)
{
  struct descriptor_data *d;
  char emote = FALSE;
  char any = FALSE;
  int level = LVL_IMMORT;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
     {send_to_char("Формат: wiznet <text> | #<level> <text> | *<emotetext> |\r\n "
		           "        wiznet @<level> *<emotetext> | wiz @\r\n", ch);
      return;
     }
  switch (*argument)
     {
  case '*':
    emote = TRUE;
  case '#':
    one_argument(argument + 1, buf1);
    if (is_number(buf1))
       {half_chop(argument+1, buf1, argument);
        level = MAX(atoi(buf1), LVL_IMMORT);
        if (level > GET_LEVEL(ch) && !GET_COMMSTATE(ch))
           {send_to_char("Вы не можете изрекать выше Вашего уровня.\r\n", ch);
	        return;
           }
       }
    else
    if (emote)
       argument++;
    break;
  case '@':
    for (d = descriptor_list; d; d = d->next)
        {if (STATE(d) == CON_PLAYING &&
             IS_IMMORTAL(d->character) &&
	         !PRF_FLAGGED(d->character, PRF_NOWIZ) &&
	         (CAN_SEE(ch, d->character) || IS_IMPL(ch)))
	        {if (!any)
	            {strcpy(buf1, "Боги в игре:\r\n");
	             any = TRUE;
	            }
	         sprintf(buf1 + strlen(buf1), "  %s", GET_NAME(d->character));
	         if (PLR_FLAGGED(d->character, PLR_WRITING))
	            strcat(buf1, " (пишет)\r\n");
	         else
	         if (PLR_FLAGGED(d->character, PLR_MAILING))
	            strcat(buf1, " (пишет письмо)\r\n");
	         else
	            strcat(buf1, "\r\n");

            }
        }
    any = FALSE;
    for (d = descriptor_list; d; d = d->next)
        {if (STATE(d) == CON_PLAYING &&
             IS_IMMORTAL(d->character) &&
	         PRF_FLAGGED(d->character, PRF_NOWIZ) &&
	         CAN_SEE(ch, d->character))
	        {if (!any)
	            {strcat(buf1, "Боги вне игры:\r\n");
	             any = TRUE;
	            }
	         sprintf(buf1 + strlen(buf1), "  %s\r\n", GET_NAME(d->character));
            }
        }
    send_to_char(buf1, ch);
    return;
  case '\\':
    ++argument;
    break;
  default:
    break;
     }
  if (PRF_FLAGGED(ch, PRF_NOWIZ))
     {send_to_char("Вы вне игры!\r\n", ch);
      return;
     }
  skip_spaces(&argument);

  if (!*argument)
     {send_to_char("Не думаю, что Боги одобрят это.\r\n", ch);
      return;
     }
  if (level > LVL_IMMORT)
     {sprintf(buf1, "%s: <%d> %s%s\r\n", GET_NAME(ch), level,
	          emote ? "<--- " : "", argument);
      sprintf(buf2, "Кто-то: <%d> %s%s\r\n", level, emote ? "<--- " : "",
	          argument);
     }
  else
     {sprintf(buf1, "%s: %s%s\r\n", GET_NAME(ch), emote ? "<--- " : "",
	          argument);
      sprintf(buf2, "Кто-то: %s%s\r\n", emote ? "<--- " : "", argument);
     }

  for (d = descriptor_list; d; d = d->next)
      {if ((STATE(d) == CON_PLAYING) &&
           (GET_LEVEL(d->character) >= level) &&
	       (!PRF_FLAGGED(d->character, PRF_NOWIZ)) &&
	       (!PLR_FLAGGED(d->character, PLR_WRITING)) &&
	       (!PLR_FLAGGED(d->character, PLR_MAILING)) &&
	       (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT))))
	      {send_to_char(CCCYN(d->character, C_NRM), d->character);
           if (CAN_SEE(d->character, ch))
	          send_to_char(buf1, d->character);
           else
	          send_to_char(buf2, d->character);
           send_to_char(CCNRM(d->character, C_NRM), d->character);
          }
      }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
     send_to_char(OK, ch);
}



ACMD(do_zreset)
{
  zone_rnum i;
  zone_vnum j;

  one_argument(argument, arg);
  if (!*arg)
     {send_to_char("Укажите зону.\r\n", ch);
      return;
     }
  if (*arg == '*')
     {for (i = 0; i <= top_of_zone_table; i++)
          reset_zone(i);
      send_to_char("Перезагружаю мир.\r\n", ch);
      sprintf(buf, "(GC) %s reset entire world.", GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
      return;
     }
  else
  if (*arg == '.')
     i = world[ch->in_room].zone;
  else
     {j = atoi(arg);
      for (i = 0; i <= top_of_zone_table; i++)
          if (zone_table[i].number == j)
	         break;
     }
  if (i >= 0 && i <= top_of_zone_table)
     {reset_zone(i);
      sprintf(buf, "Перегружаю зону %d (#%d): %s.\r\n", i, zone_table[i].number,
	          zone_table[i].name);
      send_to_char(buf, ch);
      sprintf(buf, "(GC) %s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name);
      mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
     }
  else
     send_to_char("Нет такой зоны.\r\n", ch);
}


/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil)
{
  struct char_data *vict;
  long   result;
  int    times;

  one_argument(argument, arg);

  if (!*arg)
     send_to_char("Для кого ?\r\n", ch);
  else
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
     send_to_char("Нет такого игрока.\r\n", ch);
  else
  if (IS_NPC(vict))
     send_to_char("Невозможно для моба.\r\n", ch);
  else
  if (GET_LEVEL(vict) > GET_LEVEL(ch) && !GET_COMMSTATE(ch))
     send_to_char("А он ведь старше Вас....\r\n", ch);
  else
     {switch (subcmd)
         {
    case SCMD_REROLL:
      send_to_char("Перегенерирую...\r\n", ch);
      roll_real_abils(vict);
      log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
      sprintf(buf, "Новые параметры: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
	          GET_STR(vict), GET_INT(vict), GET_WIS(vict),
	          GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
      send_to_char(buf, ch);
      break;
    case SCMD_PARDON:
      if (!PLR_FLAGGED(vict, PLR_THIEF) && !PLR_FLAGGED(vict,PLR_KILLER))
         {send_to_char("Ваша цель не имеет флагов.\r\n", ch);
	      return;
         }
      REMOVE_BIT(PLR_FLAGS(vict, PLR_THIEF), PLR_THIEF);
      REMOVE_BIT(PLR_FLAGS(vict, PLR_THIEF), PLR_KILLER);
      send_to_char("Прощен.\r\n", ch);
      send_to_char("Вы прощены богами.\r\n", vict);
      sprintf(buf, "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      break;
    case SCMD_NOTITLE:
      result = PLR_TOG_CHK(vict, PLR_NOTITLE);
      sprintf(buf, "(GC) Notitle %s for %s by %s.", ONOFF(result),
	          GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case SCMD_SQUELCH:
      result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
      if (sscanf(argument,"%s %d",arg,&times) > 0)
         {sprintf(buf,"%s$N запретил$G Вам говорить.%s",
                  CCIRED(vict,C_NRM),CCNRM(vict,C_NRM));
          act(buf,FALSE,vict,0,ch,TO_CHAR);
          MUTE_DURATION(vict) = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;
         }
      else
         MUTE_DURATION(vict) = 0;
      sprintf(buf, "(GC) Squelch %s for %s by %s(%dh).", ONOFF(result),
              GET_NAME(vict), GET_NAME(ch), times);
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case SCMD_FREEZE:
      if (ch == vict)
         {send_to_char("Это слишком жестоко...\r\n", ch);
	      return;
         }
      if (PLR_FLAGGED(vict, PLR_FROZEN))
         {send_to_char("Ваша жертва и так заморожена.\r\n", ch);
	      return;
         }
      if (sscanf(argument,"%s %d",arg,&times) > 0)
         FREEZE_DURATION(vict) = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;
      else
         FREEZE_DURATION(vict) = 0;
      SET_BIT(PLR_FLAGS(vict, PLR_FROZEN), PLR_FROZEN);
      GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
      sprintf(buf,"%s$N заморозил$G Вас.%s",
              CCIRED(vict,C_NRM),CCNRM(vict,C_NRM));
      act(buf,FALSE,vict,0,ch,TO_CHAR);
      send_to_char("Адский холод сковал Ваше тело ледяным панцирем.\r\n", vict);
      send_to_char("Заморожен.\r\n", ch);
      act("Ледяной панцирь покрыл тело $n1!", FALSE, vict, 0, 0, TO_ROOM);
      sprintf(buf, "(GC) %s frozen by %s(%dh).", GET_NAME(vict), GET_NAME(ch), times);
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      break;
    case SCMD_HELL:
      if (ch == vict)
         {send_to_char("Это слишком жестоко...\r\n", ch);
	      return;
         }
      if (sscanf(argument,"%s %d",arg,&times) > 1)
         HELL_DURATION(vict) = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;
      else
         HELL_DURATION(vict) = 0;

      if (HELL_DURATION(vict))
         {if (PLR_FLAGGED(vict, PLR_HELLED))
             {send_to_char("Ваша жертва и так в темнице.\r\n", ch);
	          return;
             }
          SET_BIT(PLR_FLAGS(vict, PLR_HELLED), PLR_HELLED);
          sprintf(buf,"%s$N поместил$G Вас в темницу.%s",
                  CCIRED(vict,C_NRM),CCNRM(vict,C_NRM));
          act(buf,FALSE,vict,0,ch,TO_CHAR);
          send_to_char("Посадили.\r\n", ch);
          act("$n водворен$a в темницу !", FALSE, vict, 0, 0, TO_ROOM);
          char_from_room(vict);
          char_to_room(vict, r_helled_start_room);
          look_at_room(vict, r_helled_start_room);
          act("$n водворен$a в темницу !", FALSE, vict, 0, 0, TO_ROOM);
          sprintf(buf, "(GC) %s removed to hell by %s(%dh).", GET_NAME(vict), GET_NAME(ch), times);
          mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
         }
      else
         {if (!PLR_FLAGGED(vict, PLR_HELLED))
             {send_to_char("Ваша жертва и так на свободе.\r\n", ch);
	          return;
             }
          REMOVE_BIT(PLR_FLAGS(vict, PLR_HELLED), PLR_HELLED);
          send_to_char("Вас выпустили из темницы.\r\n", vict);
          send_to_char("Отпустили.\r\n", ch);
          if ((result = GET_LOADROOM(vict)) == NOWHERE)
             result = calc_loadroom(vict);
          result = real_room(result);
          if (result == NOWHERE)
             {if (GET_LEVEL(vict) >= LVL_IMMORT)
                 result = r_immort_start_room;
              else
                 result = r_mortal_start_room;
             }
          act("$n выпущен$a из темницы !", FALSE, vict, 0, 0, TO_ROOM);
          char_from_room(vict);
          char_to_room(vict, result);
          look_at_room(vict, result);
          act("$n выпущен$a из темницы !", FALSE, vict, 0, 0, TO_ROOM);
          sprintf(buf, "(GC) %s removed from hell by %s(%dh).", GET_NAME(vict), GET_NAME(ch), times);
          mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
         }
      break;

    case SCMD_NAME:
      if (ch == vict)
         {send_to_char("Это слишком жестоко...\r\n", ch);
	      return;
         }
      if (sscanf(argument,"%s %d",arg,&times) > 1)
         NAME_DURATION(vict) = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;
      else
         NAME_DURATION(vict) = 0;

      if (NAME_DURATION(vict))
         {if (PLR_FLAGGED(vict, PLR_NAMED))
             {send_to_char("Ваша жертва и так в комнате имени.\r\n", ch);
	          return;
             }
          SET_BIT(PLR_FLAGS(vict, PLR_NAMED), PLR_NAMED);
          sprintf(buf,"%s$N поместил$G Вас в комнату имен.%s",
                  CCIRED(vict,C_NRM),CCNRM(vict,C_NRM));
          act(buf,FALSE,vict,0,ch,TO_CHAR);
          send_to_char("Перемещен.\r\n", ch);
          act("$n водворен$a в КОМНАТУ ИМЕНИ !", FALSE, vict, 0, 0, TO_ROOM);
          char_from_room(vict);
          char_to_room(vict, r_named_start_room);
          look_at_room(vict, r_named_start_room);
          act("$n водворен$a в КОМНАТУ ИМЕНИ !", FALSE, vict, 0, 0, TO_ROOM);
          sprintf(buf, "(GC) %s removed to NAMES ROOM by %s(%dh).", GET_NAME(vict), GET_NAME(ch), times);
          mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
         }
      else
         {if (!PLR_FLAGGED(vict, PLR_NAMED))
             {send_to_char("Вашей жертвы там нет.\r\n", ch);
	          return;
             }
          REMOVE_BIT(PLR_FLAGS(vict, PLR_NAMED), PLR_NAMED);
          send_to_char("Вас выпустили из КОМНАТЫ ИМЕНИ.\r\n", vict);
          send_to_char("Выпустили.\r\n", ch);
          if ((result = GET_LOADROOM(vict)) == NOWHERE)
             result = calc_loadroom(vict);
          result = real_room(result);
          if (result == NOWHERE)
             {if (GET_LEVEL(vict) >= LVL_IMMORT)
                 result = r_immort_start_room;
              else
                 result = r_mortal_start_room;
             }
          act("$n выпущен$a из КОМНАТЫ ИМЕНИ !", FALSE, vict, 0, 0, TO_ROOM);
          char_from_room(vict);
          char_to_room(vict, result);
          look_at_room(vict, result);
          act("$n выпущен$a из КОМНАТЫ ИМЕНИ !", FALSE, vict, 0, 0, TO_ROOM);
          sprintf(buf, "(GC) %s removed from NAMES ROOM by %s(%ds).", GET_NAME(vict), GET_NAME(ch), times);
          mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
         }
      break;

    case SCMD_REGISTER:
      if (ch == vict)
         {send_to_char("Господи, не чуди...\r\n", ch);
	      return;
         }
      if (PLR_FLAGGED(vict, PLR_REGISTERED))
         {send_to_char("Респондент уже зарегистрирован.\r\n", ch);
	  return;
         }
      SET_BIT(PLR_FLAGS(vict, PLR_REGISTERED), PLR_REGISTERED);
      send_to_char("Вас зарегистрировали.\r\n", vict);
      send_to_char("Зарегистрирован.\r\n", ch);
      if (IN_ROOM(vict) == r_unreg_start_room)
         {if ((result = GET_LOADROOM(vict)) == NOWHERE)
             result = calc_loadroom(vict);
          result = real_room(result);
          if (result == NOWHERE)
             {if (GET_LEVEL(vict) >= LVL_IMMORT)
                 result = r_immort_start_room;
              else
                 result = r_mortal_start_room;
             }
          char_from_room(vict);
          char_to_room(vict, result);
          look_at_room(vict, result);
          act("$n появил$u в центре комнаты, с гордостью показывая всем штампик регистрации !", FALSE, vict, 0, 0, TO_ROOM);
	 }
      sprintf(buf, "(GC) %s registered by %s(%ds).", GET_NAME(vict), GET_NAME(ch), times);
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      break;


    case SCMD_THAW:
      if (!PLR_FLAGGED(vict, PLR_FROZEN))
         {send_to_char("Ваша цель и так не заморожена.\r\n", ch);
	      return;
         }
      if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch))
         {sprintf(buf, "Извините, %s заморожен(а) старшими богами... Вы не в состоянии разморозить %s.\r\n",
	              GET_NAME(vict), HMHR(vict));
	      send_to_char(buf, ch);
 	      return;
         }
      sprintf(buf, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      REMOVE_BIT(PLR_FLAGS(vict, PLR_FROZEN), PLR_FROZEN);
      send_to_char("Огненный шар разметал в клочки панцирь, сковывающий Ваше тело.\r\n", vict);
      send_to_char("Разморожен.\r\n", ch);
      act("Огненный шар освободил $n3 из ледового плена.", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      if (vict->affected)
         {while (vict->affected)
	            affect_remove(vict, vict->affected);
	      send_to_char("Яркая вспышка осветила Вас!\r\n"
		               "Вы почувствовали себя немного иначе.\r\n", vict);
	      send_to_char("Все афекты сняты.\r\n", ch);
         }
      else
         {send_to_char("Аффектов не было изначально.\r\n", ch);
	      return;
         }
      break;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
      break;
         }
      save_char(vict, NOWHERE);
     }
}


/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

void print_zone_to_buf(char *bufptr, zone_rnum zone)
{
  sprintf(bufptr, "%s%3d %-30.30s Age: %3d; Reset: %3d (%1d); Top: %5d\r\n",
	      bufptr, zone_table[zone].number, zone_table[zone].name,
	      zone_table[zone].age, zone_table[zone].lifespan,
	      zone_table[zone].reset_mode, zone_table[zone].top);
}


ACMD(do_show)
{
  struct char_file_u vbuf;
  int i, j, k, l, con;		/* i, j, k to specifics? */
  zone_rnum zrn;
  zone_vnum zvn;
  char self = 0;
  struct char_data *vict;
  struct obj_data *obj;
  struct descriptor_data *d;
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], birth[80];

  struct show_struct
  { const char *cmd;
    const char level;
  } fields[] =
  {
    { "nothing",	0  },				/* 0 */
    { "zones",		LVL_IMMORT },		/* 1 */
    { "player",		LVL_GOD },
    { "rent",		LVL_GOD },
    { "stats",		LVL_IMMORT },
    { "errors",		LVL_IMPL },			/* 5 */
    { "death",		LVL_GOD },
    { "godrooms",	LVL_GOD },
    { "shops",		LVL_IMMORT },
    { "houses",		LVL_GOD },
    { "snoop",		LVL_GRGOD },		/* 10 */
    { "\n", 0 }
  };

  skip_spaces(&argument);

  if (!*argument)
     {strcpy(buf, "Опции для показа:\r\n");
      for (j = 0, i = 1; fields[i].level; i++)
          if (fields[i].level <= GET_LEVEL(ch) || GET_COMMSTATE(ch))
	         sprintf(buf + strlen(buf), "%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      return;
     }

  strcpy(arg, two_arguments(argument, field, value));

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
      if (!strncmp(field, fields[l].cmd, strlen(field)))
         break;

  if (GET_LEVEL(ch) < fields[l].level && !GET_COMMSTATE(ch))
     {send_to_char("Вы не столь могущественны, чтобы узнать это.\r\n", ch);
      return;
     }
  if (!strcmp(value, "."))
     self = 1;
  buf[0] = '\0';
  switch (l)
  {
  case 1:			/* zone */
    /* tightened up by JE 4/6/93 */
    if (self)
       print_zone_to_buf(buf, world[ch->in_room].zone);
    else
    if (*value && is_number(value))
       {for (zvn = atoi(value), zrn = 0; zone_table[zrn].number != zvn && zrn <= top_of_zone_table; zrn++);
        if (zrn <= top_of_zone_table)
	       print_zone_to_buf(buf, zrn);
        else
           {send_to_char("Нет такой зоны.\r\n", ch);
	        return;
           }
       }
    else
       for (zrn = 0; zrn <= top_of_zone_table; zrn++)
	       print_zone_to_buf(buf, zrn);
    page_string(ch->desc, buf, TRUE);
    break;
  case 2:			/* player */
    if (!*value)
       {send_to_char("Уточните имя игрока.\r\n", ch);
        return;
       }

    if (load_char(value, &vbuf) < 0)
       {send_to_char("Нет такого игрока.\r\n", ch);
        return;
       }
    sprintf(buf, "Player: %-12s (%s) [%2d %s]\r\n",
            vbuf.name,
            genders[(int) vbuf.sex],
            vbuf.level,
            class_abbrevs[(int) vbuf.chclass]);
    sprintf(buf + strlen(buf),
	        "Au: %-8d  Bal: %-8ld  Exp: %-8ld  Align: %-5d\r\n",
	        vbuf.points.gold,
	        vbuf.points.bank_gold,
	        vbuf.points.exp,
	        vbuf.char_specials_saved.alignment);
    strcpy(birth, ctime(&vbuf.birth));
    sprintf(buf + strlen(buf),
	        "Started: %-20.16s  Last: %-20.16s  Played: %3dh %2dm\r\n",
	        birth,
	        ctime(&vbuf.last_logon),
	        (int) (vbuf.played / 3600),
	        (int) (vbuf.played / 60 % 60));
    send_to_char(buf, ch);
    break;
  case 3:
    if (!*value)
       {send_to_char("Уточните имя.\r\n", ch);
        return;
       }
    Crash_listrent(ch, value);
    break;
  case 4:
    i = 0;
    j = 0;
    k = 0;
    con = 0;
    for (vict = character_list; vict; vict = vict->next)
        {if (IS_NPC(vict))
	        j++;
         else
         if (CAN_SEE(ch, vict))
            {i++;
	         if (vict->desc)
	            con++;
            }
        }
    for (obj = object_list; obj; obj = obj->next)
        k++;
    strcpy(buf, "Текущее состояние:\r\n");
    sprintf(buf + strlen(buf), "  Игроков в игре - %5d, соединений - %5d\r\n",
		    i, con);
    sprintf(buf + strlen(buf), "  Всего зарегестрировано игроков - %5d\r\n",
		    top_of_p_table + 1);
    sprintf(buf + strlen(buf), "  Мобов - %5d,  прообразов мобов - %5d\r\n",
		    j, top_of_mobt + 1);
    sprintf(buf + strlen(buf), "  Предметов - %5d, прообразов предметов - %5d\r\n",
		    k, top_of_objt + 1);
    sprintf(buf + strlen(buf), "  Комнат - %5d, зон - %5d\r\n",
		    top_of_world + 1, top_of_zone_table + 1);
    sprintf(buf + strlen(buf), "  Больших буферов - %5d\r\n",
		    buf_largecount);
    sprintf(buf + strlen(buf), "  Переключенных буферов - %5d, переполненных - %5d\r\n",
		    buf_switches, buf_overflows);
    send_to_char(buf, ch);
    break;
  case 5:
    strcpy(buf, "Пустых выходов\r\n"
                "--------------\r\n");
    for (i = 0, k = 0; i <= top_of_world; i++)
        for (j = 0; j < NUM_OF_DIRS; j++)
	        if (world[i].dir_option[j] && world[i].dir_option[j]->to_room == 0)
	           sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++k, GET_ROOM_VNUM(i),
		               world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 6:
    strcpy(buf, "Смертельных выходов\r\n"
                "-------------------\r\n");
    for (i = 0, j = 0; i <= top_of_world; i++)
        if (ROOM_FLAGGED(i, ROOM_DEATH))
	       sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j,
		           GET_ROOM_VNUM(i), world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 7:
    strcpy(buf, "Комнаты для богов\r\n"
                "-----------------\r\n");
    for (i = 0, j = 0; i <= top_of_world; i++)
        if (ROOM_FLAGGED(i, ROOM_GODROOM))
           sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n",
		           ++j, GET_ROOM_VNUM(i), world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 8:
    show_shops(ch, value);
    break;
  case 9:
    hcontrol_list_houses(ch);
    break;
  case 10:
    *buf = '\0';
    send_to_char("Система негласного контроля:\r\n", ch);
    send_to_char("----------------------------\r\n", ch);
    for (d = descriptor_list; d; d = d->next)
        {if (d->snooping == NULL || d->character == NULL)
	        continue;
         if (STATE(d) != CON_PLAYING ||
             (GET_LEVEL(ch) < GET_LEVEL(d->character) && !GET_COMMSTATE(ch)))
	        continue;
         if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
	        continue;
         sprintf(buf + strlen(buf), "%-10s - подслушивается %s.\r\n",
                 GET_NAME(d->snooping->character), GET_PAD(d->character,3));
        }
    send_to_char(*buf ? buf : "Никто не подслушивается.\r\n", ch);
    break; /* snoop */
  default:
    send_to_char("Извините, неверная команда.\r\n", ch);
    break;
  }
}


/***************** The do_set function ***********************************/

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_OR_REMOVE(flagset, flags) { \
  	   if (on) SET_BIT(flagset, flags); \
	      else \
       if (off) REMOVE_BIT(flagset, flags);}

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))


/* The set options available */
  struct set_struct
  { const char *cmd;
    const char level;
    const char pcnpc;
    const char type;
  } set_fields[] =
  {
   { "brief",		LVL_GOD, 	PC, 	BINARY },  /* 0 */
   { "invstart", 	LVL_GOD, 	PC, 	BINARY },  /* 1 */
   { "title",		LVL_IMMORT, 	PC, 	MISC },
   { "nosummon", 	LVL_GRGOD, 	PC, 	BINARY },
   { "maxhit",		LVL_IMPL, 	BOTH, 	NUMBER },
   { "maxmana", 	LVL_GRGOD, 	BOTH, 	NUMBER },  /* 5 */
   { "maxmove", 	LVL_IMPL, 	BOTH, 	NUMBER },
   { "hit", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "mana",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "move",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "align",		LVL_GOD, 	BOTH, 	NUMBER },  /* 10 */
   { "str",		LVL_IMPL, 	BOTH, 	NUMBER },
   { "size",		LVL_IMPL, 	BOTH, 	NUMBER },
   { "int", 		LVL_IMPL, 	BOTH, 	NUMBER },
   { "wis", 		LVL_IMPL, 	BOTH, 	NUMBER },
   { "dex", 		LVL_IMPL, 	BOTH, 	NUMBER },  /* 15 */
   { "con", 		LVL_IMPL, 	BOTH, 	NUMBER },
   { "cha",		LVL_IMPL, 	BOTH, 	NUMBER },
   { "ac", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "gold",		LVL_IMPL, 	BOTH, 	NUMBER },
   { "bank",		LVL_IMPL, 	PC, 	NUMBER },  /* 20 */
   { "exp", 		LVL_IMPL, 	BOTH, 	NUMBER },
   { "hitroll", 	LVL_IMPL, 	BOTH, 	NUMBER },
   { "damroll", 	LVL_IMPL, 	BOTH, 	NUMBER },
   { "invis",		LVL_IMPL, 	PC, 	NUMBER },
   { "nohassle", 	LVL_IMPL, 	PC, 	BINARY },  /* 25 */
   { "frozen",		LVL_GRGOD, 	PC, 	BINARY },
   { "practices", 	LVL_GRGOD, 	PC, 	NUMBER },
   { "lessons", 	LVL_GRGOD, 	PC, 	NUMBER },
   { "drunk",		LVL_GRGOD, 	BOTH, 	MISC },
   { "hunger",		LVL_GRGOD, 	BOTH, 	MISC },    /* 30 */
   { "thirst",		LVL_GRGOD, 	BOTH, 	MISC },
   { "killer",		LVL_GOD, 	PC, 	BINARY },
   { "thief",		LVL_GOD, 	PC, 	BINARY },
   { "level",		LVL_IMPL, 	BOTH, 	NUMBER },
   { "room",		LVL_IMPL, 	BOTH, 	NUMBER },  /* 35 */
   { "roomflag", 	LVL_GRGOD, 	PC, 	BINARY },
   { "siteok",		LVL_GRGOD, 	PC, 	BINARY },
   { "deleted", 	LVL_IMPL, 	PC, 	BINARY },
   { "class",		LVL_IMPL, 	BOTH, 	MISC },
   { "nowizlist", 	LVL_GOD, 	PC, 	BINARY },  /* 40 */
   { "quest",		LVL_GOD, 	PC, 	BINARY },
   { "loadroom", 	LVL_GRGOD, 	PC, 	MISC },
   { "color",		LVL_GOD, 	PC, 	BINARY },
   { "idnum",		LVL_IMPL, 	PC, 	NUMBER },
   { "passwd",		LVL_IMPL, 	PC, 	MISC },    /* 45 */
   { "nodelete", 	LVL_GOD, 	PC, 	BINARY },
   { "sex", 		LVL_GRGOD, 	BOTH, 	MISC },
   { "age",		LVL_GRGOD,	BOTH,	NUMBER },
   { "height",		LVL_GOD,	BOTH,	NUMBER },
   { "weight",		LVL_GOD,	BOTH,	NUMBER },  /* 50 */
   { "godslike",        LVL_IMPL,       BOTH,   BINARY },
   { "godscurse",       LVL_IMPL,       BOTH,   BINARY },
   { "olc",             LVL_IMPL,       PC,     NUMBER },
   { "name",            LVL_GOD,        PC,     MISC},
   { "trgquest",        LVL_IMPL,       PC,     MISC},     /* 55 */
   { "mkill",           LVL_IMPL,       PC,     MISC},
   { "highgod",         LVL_IMPL,       PC,     MISC},
   { "glory",           LVL_IMPL,       PC,     MISC},
   { "remort",          LVL_IMPL,       PC,     BINARY},
   { "hell",            LVL_GOD,        PC,     MISC},
   { "email",           LVL_GOD,        PC,     MISC},
   { "\n",              0,              BOTH,   MISC}
  };

int perform_set(struct char_data *ch, struct char_data *vict, int mode,
		char *val_arg)
{
  int i, j, c, on = 0, off = 0, value = 0, return_code = 1, ptnum, times, result;
  char npad[NUM_PADS][256];
  room_rnum rnum;
  room_vnum rvnum;
  char output[MAX_STRING_LENGTH], *dog_pos;

  /* Check to make sure all the levels are correct */
  if (IS_IMPL(ch))
     {if (!IS_NPC(vict)                    &&
          GET_LEVEL(ch) <= GET_LEVEL(vict) &&
          !GET_COMMSTATE(ch)               &&
          vict != ch
         )
      {send_to_char("Это не так просто, как Вам кажется...\r\n", ch);
       return (0);
      }
     }
  if (GET_LEVEL(ch) < set_fields[mode].level && !GET_COMMSTATE(ch))
     {send_to_char("Кем Вы себя возомнили ?\r\n", ch);
      return (0);
     }

  /* Make sure the PC/NPC is correct */
  if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC))
     {send_to_char("Эта тварь недостойна такой чести!\r\n", ch);
      return (0);
     }
  else
  if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC))
     {act("Вы оскорбляете $S - $E ведь не моб !", FALSE, ch, 0, vict, TO_CHAR);
      return (0);
     }

  /* Find the value of the argument */
  if (set_fields[mode].type == BINARY)
     {if (!strn_cmp(val_arg, "on",  2) ||
          !strn_cmp(val_arg, "yes", 3)||
          !strn_cmp(val_arg, "вкл", 3))
         on = 1;
      else
      if (!strn_cmp(val_arg, "off", 3) ||
          !strn_cmp(val_arg, "no",  2) ||
          !strn_cmp(val_arg, "выкл",4))
         off = 1;
      if (!(on || off))
         {send_to_char("Значение может быть 'on' или 'off'.\r\n", ch);
          return (0);
         }
      sprintf(output, "%s %s для %s.",
              set_fields[mode].cmd, ONOFF(on),GET_PAD(vict,1));
     }
  else
  if (set_fields[mode].type == NUMBER)
     {value = atoi(val_arg);
      sprintf(output, "У %s %s установлено в %d.",
              GET_PAD(vict,1),set_fields[mode].cmd, value);
     }
  else
     {strcpy(output, "Хорошо.");
     }
  switch (mode)
  {
  case 0:
    SET_OR_REMOVE(PRF_FLAGS(vict, PRF_BRIEF), PRF_BRIEF);
    break;
  case 1:
    SET_OR_REMOVE(PLR_FLAGS(vict, PLR_INVSTART), PLR_INVSTART);
    break;
  case 2:
    set_title(vict, val_arg);
    sprintf(output, "Титул %s изменен : %s",
                    GET_PAD(vict,1),
                    GET_TITLE(vict) ? only_title(vict) : "убран");
    break;
  case 3:
    SET_OR_REMOVE(PRF_FLAGS(vict, PRF_SUMMONABLE), PRF_SUMMONABLE);
    sprintf(output, "Возможность призыва %s для %s.\r\n",
            ONOFF(!on), GET_PAD(vict,1));
    break;
  case 4:
    vict->points.max_hit = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 5:
    break;
  case 6:
    vict->points.max_move = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 7:
    vict->points.hit = RANGE(-9, vict->points.max_hit);
    affect_total(vict);
    break;
  case 8:
    break;
  case 9:
    break;
  case 10:
    GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
    affect_total(vict);
    break;
  case 11:
    RANGE(3, 35);
    vict->real_abils.str  = value;
    affect_total(vict);
    break;
  case 12:
    vict->real_abils.size = RANGE(1, 100);
    affect_total(vict);
    break;
  case 13:
    RANGE(3, 35);
    vict->real_abils.intel = value;
    affect_total(vict);
    break;
  case 14:
    RANGE(3, 35);
    vict->real_abils.wis = value;
    affect_total(vict);
    break;
  case 15:
    RANGE(3, 35);
    vict->real_abils.dex = value;
    affect_total(vict);
    break;
  case 16:
    RANGE(3, 35);
    vict->real_abils.con = value;
    affect_total(vict);
    break;
  case 17:
    RANGE(3, 35);
    vict->real_abils.cha = value;
    affect_total(vict);
    break;
  case 18:
    vict->real_abils.armor = RANGE(-100, 100);
    affect_total(vict);
    break;
  case 19:
    GET_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 20:
    GET_BANK_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 21:
    vict->points.exp = RANGE(0, 70000000);
    break;
  case 22:
    vict->real_abils.hitroll = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 23:
    vict->real_abils.damroll = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 24:
    if (!IS_IMPL(ch) && ch != vict)
       { send_to_char("Вы не столь Божественны, как Вам кажется!\r\n", ch);
         return (0);
       }
    GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
    break;
  case 25:
    if (!IS_IMPL(ch) && ch != vict)
       { send_to_char("Вы не столь Божественны, как Вам кажется!\r\n", ch);
         return (0);
       }
    SET_OR_REMOVE(PRF_FLAGS(vict, PRF_NOHASSLE), PRF_NOHASSLE);
    break;
  case 26:
    if (ch == vict && on)
       { send_to_char("Лучше не стоит - Вас ожидает долгая зима :)\r\n", ch);
         return (0);
       }
    SET_OR_REMOVE(PLR_FLAGS(vict, PLR_FROZEN), PLR_FROZEN);
    if (on)
       {if (sscanf(val_arg,"%d",&i) != 0)
           FREEZE_DURATION(vict) = (i > 0) ? time(NULL) + i * 60 * 60 : MAX_TIME;
        else
           FREEZE_DURATION(vict) = 0;
       }
    break;
  case 27:
  case 28:
    return_code = 0;
    break;
  case 29:
  case 30:
  case 31:
    if (!str_cmp(val_arg, "off") || !str_cmp(val_arg, "выкл"))
       {GET_COND(vict, (mode - 29)) = (char) -1; /* warning: magic number here */
        sprintf(output, "Для %s %s сейчас отключен.", GET_PAD(vict,1), set_fields[mode].cmd);
       }
    else
    if (is_number(val_arg))
       {value = atoi(val_arg);
        RANGE(0, 24);
        GET_COND(vict, (mode - 29)) = (char) value; /* and here too */
        sprintf(output, "Для %s %s установлен в %d.", GET_PAD(vict,1),
	            set_fields[mode].cmd, value);
       }
    else
       {send_to_char("Должно быть 'off' или значение от 0 до 24.\r\n", ch);
        return (0);
       }
    break;
  case 32:
    SET_OR_REMOVE(PLR_FLAGS(vict, PLR_KILLER), PLR_KILLER);
    break;
  case 33:
    SET_OR_REMOVE(PLR_FLAGS(vict, PLR_THIEF), PLR_THIEF);
    break;
  case 34:
    if (!GET_COMMSTATE(ch) &&
        (value > GET_LEVEL(ch) || value > LVL_IMPL || GET_LEVEL(vict) > GET_LEVEL(ch)))
       { send_to_char("Вы не можете установить уровень игрока выше собственного.\r\n", ch);
         return (0);
       }
    RANGE(0, LVL_IMPL);
    vict->player.level = (byte) value;
    break;
  case 35:
    if ((rnum = real_room(value)) < 0)
       {send_to_char("Поищите другой МУД. В этом МУДе нет такой комнаты.\r\n", ch);
        return (0);
       }
    if (IN_ROOM(vict) != NOWHERE)	/* Another Eric Green special. */
       char_from_room(vict);
    char_to_room(vict, rnum);
    check_horse(vict);
    break;
  case 36:
    SET_OR_REMOVE(PRF_FLAGS(vict, PRF_ROOMFLAGS), PRF_ROOMFLAGS);
    break;
  case 37:
    SET_OR_REMOVE(PLR_FLAGS(vict, PLR_SITEOK), PLR_SITEOK);
    break;
  case 38:
    SET_OR_REMOVE(PLR_FLAGS(vict, PLR_DELETED), PLR_DELETED);
    break;
  case 39:
    if ((i = parse_class(*val_arg)) == CLASS_UNDEFINED)
       {send_to_char("Нет такого класа в этой игре. Найдите себе более другую.\r\n", ch);
        return (0);
       }
    GET_CLASS(vict) = i;
    break;
  case 40:
    SET_OR_REMOVE(PLR_FLAGS(vict, PLR_NOWIZLIST), PLR_NOWIZLIST);
    break;
  case 41:
    SET_OR_REMOVE(PRF_FLAGS(vict, PRF_QUEST), PRF_QUEST);
    break;
  case 42:
    if (!str_cmp(val_arg, "off") || !str_cmp(val_arg, "выкл"))
       {REMOVE_BIT(PLR_FLAGS(vict, PLR_LOADROOM), PLR_LOADROOM);
       }
    else
    if (is_number(val_arg))
       {rvnum = atoi(val_arg);
        if (real_room(rvnum) != NOWHERE)
           {SET_BIT(PLR_FLAGS(vict, PLR_LOADROOM), PLR_LOADROOM);
	    GET_LOADROOM(vict) = rvnum;
	    sprintf(output, "%s будет входить в игру из комнаты #%d.", GET_NAME(vict),
	                    GET_LOADROOM(vict));
           }
        else
           {send_to_char("Прежде чем кого-то куда-то поместить, надо это КУДА-ТО создать.\r\n"
                         "Скажите Стрибогу - пусть зоны рисует, а не пьянствует.\r\n", ch);
	        return (0);
           }
       }
    else
       {send_to_char("Должно быть 'off' или виртуальный номер комнаты.\r\n", ch);
        return (0);
       }
    break;
  case 43:
    SET_OR_REMOVE(PRF_FLAGS(vict, PRF_COLOR_1), PRF_COLOR_1);
    SET_OR_REMOVE(PRF_FLAGS(vict, PRF_COLOR_2), PRF_COLOR_2);
    break;
  case 44:
    if (GET_IDNUM(ch) != 1 || !IS_NPC(vict))
       return (0);
    GET_IDNUM(vict) = value;
    break;
  case 45:
    if (GET_IDNUM(ch) != 1 && !GET_COMMSTATE(ch) && ch != vict)
       {send_to_char("Давайте не будем экспериментировать.\r\n", ch);
        return (0);
       }
    if (GET_IDNUM(vict) == 1 &&
        ch != vict           &&
	!GET_COMMSTATE(ch)
       )
       {send_to_char("Вы не можете ЭТО изменить.\r\n", ch);
        return (0);
       }
    strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)), MAX_PWD_LENGTH);
    *(GET_PASSWD(vict) + MAX_PWD_LENGTH) = '\0';
    sprintf(output, "Пароль изменен на '%s'.", val_arg);
    break;
  case 46:
    SET_OR_REMOVE(PLR_FLAGS(vict, PLR_NODELETE), PLR_NODELETE);
    break;
  case 47:
    if ((i = search_block(val_arg, genders, FALSE)) < 0)
       {send_to_char("Может быть 'мужчина', 'женщина', или 'бесполое'(а вот это я еще не оценил :).\r\n", ch);
        return (0);
       }
    GET_SEX(vict) = i;
    break;
  case 48:	/* set age */
    if (value < 2 || value > 200)
       {/* Arbitrary limits. */
        send_to_char("Поддерживаются возрасты от 2 до 200.\r\n", ch);
        return (0);
       }
    /*
     * NOTE: May not display the exact age specified due to the integer
     * division used elsewhere in the code.  Seems to only happen for
     * some values below the starting age (17) anyway. -gg 5/27/98
     */
    vict->player.time.birth = time(0) - ((value - 17) * SECS_PER_MUD_YEAR);
    break;

  case 49:	/* Blame/Thank Rick Glover. :) */
    GET_HEIGHT(vict) = value;
    affect_total(vict);
    break;

  case 50:
    GET_WEIGHT(vict) = value;
    affect_total(vict);
    break;

  case 51:
    if (on)
       {SET_GOD_FLAG(vict,GF_GODSLIKE);
        if (sscanf(val_arg,"%s %d",npad[0],&i) != 0)
           GODS_DURATION(vict) = (i > 0) ? time(NULL) + i * 60 * 60 : MAX_TIME;
         else
           GODS_DURATION(vict) = 0;
       }
    else
    if (off)
       CLR_GOD_FLAG(vict,GF_GODSLIKE);
    break;
  case 52:
    if (on)
       {SET_GOD_FLAG(vict,GF_GODSCURSE);
        if (sscanf(val_arg,"%s %d",npad[0],&i) != 0)
           GODS_DURATION(vict) = (i > 0) ? time(NULL) + i * 60 * 60 : MAX_TIME;
         else
           GODS_DURATION(vict) = 0;
       }
    else
    if (off)
       CLR_GOD_FLAG(vict,GF_GODSCURSE);
    break;
  case 53:
    GET_OLC_ZONE(vict) = value;
    break;
  case 54:
    /* изменение имени !!! */

    if ((i = sscanf(val_arg,"%s %s %s %s %s %s",npad[0],npad[1],npad[2],npad[3],npad[4],npad[5])) != 6)
       {sprintf(buf,"Требуется указать 6 падежей, найдено %d\r\n",i);
        send_to_char(buf,ch);
        return (0);
       }

    if (*npad[0] == '*')
       {// Only change pads
        for (i = 1; i < NUM_PADS; i++)
            if (!_parse_name(npad[i],npad[i]))
               {if (GET_PAD(vict,i))
                   free(GET_PAD(vict,i));
                CREATE(GET_PAD(vict,i),char,strlen(npad[i])+1);
                strcpy(GET_PAD(vict,i),npad[i]);
               }
        sprintf(buf,"Произведена замена падежей.\r\n");
        send_to_char(buf,ch);	
       }
    else
       {if (_parse_name(npad[0],npad[0])      ||
            strlen(npad[0]) < MIN_NAME_LENGTH ||
	    strlen(npad[0]) > MAX_NAME_LENGTH ||
            !Valid_Name(npad[0])              ||
	    reserved_word(npad[0])            ||
	    fill_word(npad[0])
           )
           {send_to_char("Некорректное имя.\r\n",ch);
            return (0);
           }
        /*
        if ((ptnum = cmp_ptable_by_name(npad[0],MIN_NAME_LENGTH)) >= 0 &&
            player_table[ptnum].unique != GET_UNIQUE(vict)
           )
           {send_to_char("Первые 4 символа этого имени совпадают еще у одного персонажа.\r\n"
                         "Для исключения различного рода недоразумений имя отклонено.\r\n", ch);
            return (0);
           }
         */
        ptnum = get_ptable_by_name(GET_NAME(vict));
        if (ptnum < 0)
           return (0);

        for (i = 0; i < NUM_PADS; i++)
            if (!_parse_name(npad[i],npad[i]))
               {if (GET_PAD(vict,i))
                   free(GET_PAD(vict,i));
                CREATE(GET_PAD(vict,i),char,strlen(npad[i])+1);
                strcpy(GET_PAD(vict,i),npad[i]);
               }
        if (GET_NAME(vict))
           free(GET_NAME(vict));
        CREATE(GET_NAME(vict),char,strlen(npad[0])+1);
        strcpy(GET_NAME(vict),npad[0]);

        free(player_table[ptnum].name);
        CREATE(player_table[ptnum].name,char,strlen(npad[0])+1);
        for (i=0, player_table[ptnum].name[i] = '\0'; npad[0][i]; i++)
            player_table[ptnum].name[i] = LOWER(npad[0][i]);
        return_code = 2;
       }
    break;

  case 55:

    if (sscanf(val_arg,"%d %s", &ptnum, npad[0]) != 2)
       {send_to_char("Формат : set <имя> trgquest <quest_num> <on|off>\r\n", ch);
        return (0);
       }
    if (!str_cmp(npad[0],"off") || !str_cmp(npad[0],"выкл"))
       {for (i = j = 0; j < vict->Questing.count; i++, j++)
            {if (vict->Questing.quests[i] == ptnum)
                j++;
             vict->Questing.quests[i] = vict->Questing.quests[j];
            }
        if (j > i)
           vict->Questing.count--;
        else
           {act("$N не выполнял$G этого квеста.",FALSE,ch,0,vict,TO_CHAR);
            return (0);
           }
       }
    else
    if (!str_cmp(npad[0],"on") || !str_cmp(npad[0],"вкл"))
       {set_quested(vict,ptnum);
       }
    else
       {
        send_to_char("Требуется on или off.\r\n",ch);
        return (0);
       }
    break;

  case 56:

    if (sscanf(val_arg,"%d %s", &ptnum, npad[0]) != 2)
       {send_to_char("Формат : set <имя> mkill <mob_vnum> <off|num>\r\n", ch);
        return (0);
       }
    if (!str_cmp(npad[0],"off") || !str_cmp(npad[0],"выкл"))
       {for (i = j = 0; j < vict->MobKill.count; i++, j++)
            {if (vict->MobKill.vnum[i] == ptnum)
                j++;
             vict->MobKill.vnum[i]    = vict->MobKill.vnum[j];
             vict->MobKill.howmany[i] = vict->MobKill.howmany[j];
            }
        if (j > i)
           vict->MobKill.count--;
        else
           {act("$N не убивал$G ни одного этого моба.",FALSE,ch,0,vict,TO_CHAR);
            return (0);
           }
       }
    else
    if ((j = atoi(npad[0])) > 0)
       {if ((c = get_kill_vnum(vict,ptnum)) != j)
           inc_kill_vnum(vict,ptnum,j-c);
        else
           {act("$N убил$G именно столько этих мобов.",FALSE,ch,0,vict,TO_CHAR);
            return (0);
           }
       }
    else
       {send_to_char("Требуется off или значение больше 0.\r\n",ch);
        return (0);
       }
    break;

  case 57:
    return (0);
    break;
  case 58:
    skip_spaces(&val_arg);
    if (!val_arg || !*val_arg || ((j = atoi(val_arg)) == 0 && str_cmp("zerro", val_arg)))
       {sprintf(output,"%s заработал%s %d %s славы.", GET_NAME(vict),
                GET_CH_SUF_1(vict), GET_GLORY(vict), desc_count(GET_GLORY(vict), WHAT_POINT));
        return_code = 0;
       }
    else
       {if (*val_arg == '-' ||
            *val_arg == '+'
	   )
           GET_GLORY(vict) = MAX(0,GET_GLORY(vict) + j);
        else
           GET_GLORY(vict) = j;
        sprintf(output,"Количество славы, которое заработал%s %s установлено в %d %s.",
                GET_CH_SUF_1(vict), GET_NAME(vict), GET_GLORY(vict), desc_count(GET_GLORY(vict), WHAT_POINT));
       }
    break;
  case 59:
    if (!GET_COMMSTATE(ch))
       {send_to_char("Coder only !\r\n",ch);
        return (0);
       }
    if (on)
       {SET_GOD_FLAG(vict,GF_REMORT);
        sprintf(output,"%s получил%s право на перевоплощение.",
                GET_NAME(vict), GET_CH_SUF_1(vict));
       }
    else
    if (off)
       {CLR_GOD_FLAG(vict,GF_GODSLIKE);
        sprintf(output,"%s утратил%s право на перевоплощение.",
                GET_NAME(vict), GET_CH_SUF_1(vict));
       }
    break;

 case 60:
      if (sscanf(val_arg,"%d",&times))
         HELL_DURATION(vict) = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;
      else
         HELL_DURATION(vict) = 0;

      if (HELL_DURATION(vict))
         {SET_BIT(PLR_FLAGS(vict, PLR_HELLED), PLR_HELLED);
	  if (IN_ROOM(vict) != NOWHERE)
	     {sprintf(buf,"%s$N поместил$G Вас в темницу.%s",
                      CCIRED(vict,C_NRM),CCNRM(vict,C_NRM));
              act(buf,FALSE,vict,0,ch,TO_CHAR);
              send_to_char("Посадили.\r\n", ch);
              act("$n водворен$a в темницу !", FALSE, vict, 0, 0, TO_ROOM);
              char_from_room(vict);
              char_to_room(vict, r_helled_start_room);
              look_at_room(vict, r_helled_start_room);
              act("$n водворен$a в темницу !", FALSE, vict, 0, 0, TO_ROOM);
	     }
          sprintf(output, "(GC) %s removed to hell by %s(%dh).", GET_NAME(vict), GET_NAME(ch), times);
         }
      else
         {REMOVE_BIT(PLR_FLAGS(vict, PLR_HELLED), PLR_HELLED);
	  if (IN_ROOM(vict) != NOWHERE)
	     {send_to_char("Вас выпустили из темницы.\r\n", vict);
              send_to_char("Отпустили.\r\n", ch);
              if ((result = GET_LOADROOM(vict)) == NOWHERE)
                 result = calc_loadroom(vict);
              result = real_room(result);
              if (result == NOWHERE)
                 {if (GET_LEVEL(vict) >= LVL_IMMORT)
                     result = r_immort_start_room;
                  else
                     result = r_mortal_start_room;
                 }
              act("$n выпущен$a из темницы !", FALSE, vict, 0, 0, TO_ROOM);
              char_from_room(vict);
              char_to_room(vict, result);
              look_at_room(vict, result);
              act("$n выпущен$a из темницы !", FALSE, vict, 0, 0, TO_ROOM);
	     }
          sprintf(output, "(GC) %s removed from hell by %s.", GET_NAME(vict), GET_NAME(ch));
         }
      break;
 case 61:
      if (*val_arg &&
          (dog_pos = strchr(val_arg,'@')) &&
	  dog_pos > val_arg &&
	  *(dog_pos + 1)
	 )
         {strncpy(GET_EMAIL(vict),val_arg,127);
	  *(GET_EMAIL(vict)+127) = '\0';
	 }
      else	
         {send_to_char("Wrong E-Mail.\r\n",ch);
	  return (0);
	 }
      break;
 default:
    send_to_char("Не могу установить это!\r\n", ch);
    return (0);
  }

  strcat(output, "\r\n");
  send_to_char(CAP(output), ch);
  return (return_code);
}



ACMD(do_set)
{
  struct char_data *vict = NULL, *cbuf = NULL;
  struct char_file_u tmp_store;
  char field[MAX_INPUT_LENGTH],
       name[MAX_INPUT_LENGTH],
	   val_arg[MAX_INPUT_LENGTH],
	   OName[MAX_INPUT_LENGTH];
  int  mode, len, player_i = 0, retval;
  char is_file = 0, is_player = 0;
  FILE *saved;

  half_chop(argument, name, buf);

  if (!strcmp(name, "file"))
     {is_file = 1;
      half_chop(buf, name, buf);
     }
  else
  if (!str_cmp(name, "player"))
     {is_player = 1;
      half_chop(buf, name, buf);
     }
  else
  if (!str_cmp(name, "mob"))
     half_chop(buf, name, buf);

  half_chop(buf, field, buf);
  strcpy(val_arg, buf);

  if (!*name || !*field)
     {send_to_char("Usage: set <victim> <field> <value>\r\n", ch);
      return;
     }

  /* find the target */
  if (!is_file)
     {if (is_player)
         {if (!(vict = get_player_vis(ch, name, FIND_CHAR_WORLD)))
             {send_to_char("Нет такого игрока.\r\n", ch);
	          return;
             }
         }
      else
         { /* is_mob */
           if (!(vict = get_char_vis(ch, name, FIND_CHAR_WORLD)))
              {send_to_char("Нет такой твари Божьей.\r\n", ch);
  	           return;
              }
         }
     }
  else
  if (is_file)
     {/* try to load the player off disk */
      CREATE(cbuf, struct char_data, 1);
      clear_char(cbuf);
      if ((player_i = load_char(name, &tmp_store)) > -1)
         {store_to_char(&tmp_store, cbuf);
          if (GET_LEVEL(cbuf) >=  GET_LEVEL(ch) && !GET_COMMSTATE(ch) &&
              GET_IDNUM(ch) != 1 && !IS_HIGHGOD(ch))
             {free_char(cbuf);
	      send_to_char("Вы не можете сделать этого.\r\n", ch);
	      return;
             }
          load_pkills(cbuf);
          vict = cbuf;
         }
      else
         {free(cbuf);
          send_to_char("Нет такого игрока.\r\n", ch);
          return;
         }
     }

  /* find the command in the list */
  len = strlen(field);
  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
      if (!strncmp(field, set_fields[mode].cmd, len))
         break;

  /* perform the set */
  strcpy(OName,GET_NAME(vict));
  retval = perform_set(ch, vict, mode, val_arg);

  /* save the character if a change was made */
  if (retval && !IS_NPC(vict))
     {if (retval == 2)
         {rename_char(vict,OName);
         }
      else
         {if (!is_file && !IS_NPC(vict))
             {save_char(vict, NOWHERE);
             }
          if (is_file)
             {char_to_store(vict, &tmp_store);
              send_to_char("Файл ",ch);
              if (USE_SINGLE_PLAYER)
                 {if (get_filename(GET_NAME(vict),name,PLAYERS_FILE) &&
                     (saved = fopen(name,"w+b")))
                     {fwrite(&tmp_store, sizeof(struct char_file_u), 1, saved);
                      fclose(saved);
                     }
                  else
                     send_to_char("НЕ ",ch);
                 }
              else
                 {fseek(player_fl, (player_i) * sizeof(struct char_file_u), SEEK_SET);
                  fwrite(&tmp_store, sizeof(struct char_file_u), 1, player_fl);
                 }
              save_pkills(ch);
              new_save_quests(cbuf);
              new_save_mkill(cbuf);
              send_to_char("сохранен.\r\n", ch);
             }
         }
     }

  /* free the memory if we allocated it earlier */
  if (is_file)
     free_char(cbuf);
}
