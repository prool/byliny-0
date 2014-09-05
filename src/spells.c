/* ************************************************************************
*   File: spells.c                                      Part of CircleMUD *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
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
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "screen.h"
#include "house.h"

extern room_rnum r_mortal_start_room;
extern struct room_data *world;
extern struct obj_data *object_list, *obj_proto;
extern struct char_data *character_list;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;
extern const  char *material_name[];
extern const  char *weapon_affects[];
extern struct time_info_data time_info;
extern int  mini_mud, cmd_tell;
extern char cast_argument[MAX_INPUT_LENGTH];
extern struct house_control_rec house_control[MAX_HOUSES];
extern int num_of_houses;

void clearMemory(struct char_data * ch);
void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
void name_to_drinkcon(struct obj_data * obj, int type);
void name_from_drinkcon(struct obj_data * obj);
int  compute_armor_class(struct char_data *ch);
int  may_pkill(struct char_data *revenger, struct char_data *killer);
int  inc_pkill(struct char_data *victim, struct char_data *killer, int pkills, int prevenge);
int  inc_pkill_group(struct char_data *victim, struct char_data *killer, int pkills, int prevenge);
char *diag_weapon_to_char(struct obj_data *obj, int show_wear);
void create_rainsnow(int *wtype, int startvalue, int chance1, int chance2, int chance3);
int  calc_loadroom(struct char_data *ch);
void go_create_weapon(struct char_data *ch, struct obj_data *obj, int obj_type, int); //prool
ACMD(do_tell);

int  what_sky = SKY_CLOUDLESS;
/*
 * Special spells appear below.
 */

ASPELL(spell_create_water)
{
  int water;
  if (ch == NULL || (obj == NULL && victim == NULL))
     return;
  /* level = MAX(MIN(level, LVL_IMPL), 1);	 - not used */

  if (obj && GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
     {if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0))
         {send_to_char("Прекратите, ради бога, химичить.\r\n",ch);
          return;
          name_from_drinkcon(obj);
          GET_OBJ_VAL(obj, 2) = LIQ_BLOOD;
          name_to_drinkcon(obj, LIQ_BLOOD);
         }
      else
        {water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
         if (water > 0)
            {if (GET_OBJ_VAL(obj, 1) >= 0)
	            name_from_drinkcon(obj);
	         GET_OBJ_VAL(obj, 2) = LIQ_WATER;
	         GET_OBJ_VAL(obj, 1) += water;
	         act("Вы наполнили $o3 водой.", FALSE, ch, obj, 0, TO_CHAR);	
	         name_to_drinkcon(obj, LIQ_WATER);
	         weight_change_object(obj, water);
            }
        }
     }
  if (victim && !IS_NPC(victim))
     {gain_condition(victim,THIRST,25);
      send_to_char("Вы полностью утолили жажду.\r\n",victim);
      if (victim != ch)
         act("Вы напоили $N3.",FALSE,ch,0,victim,TO_CHAR);
     }
}

#define SUMMON_FAIL "Попытка перемещения не удалась.\r\n"
#define MIN_NEWBIE_ZONE  20
#define MAX_NEWBIE_ZONE  79
#define MAX_SUMMON_TRIES 2000

ASPELL(spell_recall)
{ room_rnum to_room = NOWHERE, fnd_room = NOWHERE;
  int       modi = 0;
  int	    j;

  if (!victim || IS_NPC(victim) || IN_ROOM(ch) != IN_ROOM(victim))
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (!IS_GOD(ch) &&
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORT)
     )
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (victim != ch)
     {if (WAITLESS(ch) && !WAITLESS(victim))
         modi += 100;  // always success
      else
      if (same_group(ch,victim))
         modi += 75;   // 75% chance to success
      else
      if (!IS_NPC(ch) || (ch->master && !IS_NPC(ch->master)))
         modi  = -100; // always fail

      if (general_savingthrow(victim, SAVING_PARA, modi))
         {send_to_char(SUMMON_FAIL,ch);
          return;
         }
     }

  if ((to_room = real_room(GET_LOADROOM(victim))) == NOWHERE)
     to_room = real_room(calc_loadroom(victim));

  if (to_room == NOWHERE)
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  for (modi = 0; modi < MAX_SUMMON_TRIES; modi++)
      {fnd_room = number(0,top_of_world);
       if ( world[to_room].zone == world[fnd_room].zone              &&
            SECT(fnd_room)      != SECT_SECRET                       &&
            !ROOM_FLAGGED(fnd_room, ROOM_DEATH      | ROOM_TUNNEL)   &&
            !ROOM_FLAGGED(fnd_room, ROOM_NOTELEPORT)                 &&
	    !ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH)                  &&
	    (!ROOM_FLAGGED(fnd_room, ROOM_GODROOM) || IS_IMMORTAL(victim)) &&
	    (!ROOM_FLAGGED(fnd_room, ROOM_PRIVATE) || House_can_enter(victim,fnd_room))
           )
	   break;
      }

  if (modi >= MAX_SUMMON_TRIES)
     {send_to_char(SUMMON_FAIL,ch);
      return;
     }
//Изменения Карда
  /* Тут вставка, если недавно убивал, и реколишься в зону любого замка,
  то тебя перенесет в комнату atrium */
  if (RENTABLE(victim))
     {for (j = 0; j < num_of_houses; j++)
          {if (get_name_by_id(house_control[j].owner) == NULL)
              continue;
          if (world[fnd_room].zone == world[real_room(house_control[j].vnum[0])].zone)
             fnd_room = real_room(house_control[j].atrium);
         }
     }
//
  act("$n исчез$q.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, fnd_room);
  check_horse(victim);
  act("$n появил$u в центре комнаты.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim,-1);
  greet_memory_mtrigger(victim);
}


ASPELL(spell_teleport)
{ room_rnum fnd_room = NOWHERE;
  int       modi = 0;

  if (victim == NULL)
     victim = ch;

  if (IN_ROOM(victim) == NOWHERE)
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (victim != ch)
     {if (same_group(ch,victim))
         modi += 25;
      if (general_savingthrow(victim, SAVING_SPELL, modi))
         {send_to_char(SUMMON_FAIL,ch);
          return;
         }
     }

  if (!IS_GOD(ch) &&
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORT)
     )
    {send_to_char("Перемещение невозможно.\r\n", ch);
     return;
    }

  for (modi = 0; modi < MAX_SUMMON_TRIES; modi++)
      {fnd_room = number(0,top_of_world);
       if ( world[fnd_room].zone == world[IN_ROOM(victim)].zone      &&
            SECT(fnd_room)       != SECT_SECRET                      &&
            !ROOM_FLAGGED(fnd_room, ROOM_DEATH      | ROOM_TUNNEL)   &&
            !ROOM_FLAGGED(fnd_room, ROOM_NOTELEPORT)                 &&
	    !ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH)                  &&
	    (!ROOM_FLAGGED(fnd_room, ROOM_GODROOM) || IS_IMMORTAL(victim)) &&
	    (!ROOM_FLAGGED(fnd_room, ROOM_PRIVATE) || House_can_enter(victim,fnd_room))
           )
	   break;
      }

  if (modi >= MAX_SUMMON_TRIES)
     {send_to_char(SUMMON_FAIL,ch);
      return;
     }

  act("$n медленно исчез$q из виду.",FALSE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, fnd_room);
  check_horse(victim);
  act("$n медленно появил$u откуда-то.", FALSE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim,-1);
  greet_memory_mtrigger(victim);
}


ASPELL(spell_relocate)
{ room_rnum to_room, fnd_room;
  int       is_newbie;

  if (victim == NULL)
     return;

  if (GET_LEVEL(victim) > GET_LEVEL(ch) && !GET_COMMSTATE(ch))
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if ((fnd_room = IN_ROOM(victim)) == NOWHERE)
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (!IS_GOD(ch) &&
      (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTELEPORT) ||
       SECT(fnd_room) == SECT_SECRET              ||
       ROOM_FLAGGED(fnd_room, ROOM_DEATH)         ||
       ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH)     ||
       ROOM_FLAGGED(fnd_room, ROOM_TUNNEL)        ||
       ROOM_FLAGGED(fnd_room, ROOM_NOTELEPORT)    ||
       (ROOM_FLAGGED(fnd_room, ROOM_PRIVATE) && !House_can_enter(ch, fnd_room)) ||
       (ROOM_FLAGGED(fnd_room, ROOM_GODROOM) && !IS_IMMORTAL(ch))
      )
     )
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (!IS_NPC(ch))
     {if ((to_room = real_room(GET_LOADROOM(ch))) == NOWHERE)
         to_room = real_room(calc_loadroom(ch));
      is_newbie = (world[to_room].zone >= MIN_NEWBIE_ZONE && world[to_room].zone <= MAX_NEWBIE_ZONE);
      if ((is_newbie  && (world[fnd_room].zone < MIN_NEWBIE_ZONE || world[fnd_room].zone > MAX_NEWBIE_ZONE)) ||
          (!is_newbie && world[fnd_room].zone >= MIN_NEWBIE_ZONE && world[fnd_room].zone <= MAX_NEWBIE_ZONE)
	 )
         {send_to_char(SUMMON_FAIL, ch);
          return;
         }
     }
  act("$n медленно исчез$q из виду.",FALSE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, fnd_room);
  check_horse(ch);
  act("$n медленно появил$u откуда-то.", FALSE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
  if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch,GF_GODSLIKE)))
     WAIT_STATE(ch, 3 * PULSE_VIOLENCE);
}


ASPELL(spell_portal)
{ room_rnum to_room,fnd_room;
  int       is_newbie;

  if (victim == NULL)
     return;
  if (GET_LEVEL(victim) > GET_LEVEL(ch) && !GET_COMMSTATE(ch))
     return;

  fnd_room  = IN_ROOM(victim);
  if (fnd_room == NOWHERE)
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }
  if (!IS_GOD(ch) &&
      (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTELEPORT) ||
       SECT(fnd_room) == SECT_SECRET              ||
       ROOM_FLAGGED(fnd_room, ROOM_DEATH)         ||
       ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH)     ||
       ROOM_FLAGGED(fnd_room, ROOM_TUNNEL)        ||
       ROOM_FLAGGED(fnd_room, ROOM_PRIVATE)       ||
       ROOM_FLAGGED(fnd_room, ROOM_GODROOM)       ||
       ROOM_FLAGGED(fnd_room, ROOM_NOTELEPORT)
      )
     )
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (!IS_NPC(ch))
     {if ((to_room = real_room(GET_LOADROOM(ch))) == NOWHERE)
         to_room = real_room(calc_loadroom(ch));
      is_newbie = (world[to_room].zone >= MIN_NEWBIE_ZONE && world[to_room].zone <= MAX_NEWBIE_ZONE);
      if ((is_newbie  && (world[fnd_room].zone < MIN_NEWBIE_ZONE || world[fnd_room].zone > MAX_NEWBIE_ZONE)) ||
          (!is_newbie && world[fnd_room].zone >= MIN_NEWBIE_ZONE && world[fnd_room].zone <= MAX_NEWBIE_ZONE))
         {send_to_char(SUMMON_FAIL, ch);
          return;
         }
     }

  to_room                     = IN_ROOM(ch);
  world[fnd_room].portal_room = to_room;
  world[fnd_room].portal_time = 1;
  act("Лазурная пентаграмма возникла в воздухе.",FALSE,world[fnd_room].people,0,0,TO_CHAR);
  act("Лазурная пентаграмма возникла в воздухе.",FALSE,world[fnd_room].people,0,0,TO_ROOM);
  world[to_room].portal_room   = fnd_room;
  world[to_room].portal_time   = 1;
  act("Лазурная пентаграмма возникла в воздухе.",FALSE,world[to_room].people,0,0,TO_CHAR);
  act("Лазурная пентаграмма возникла в воздухе.",FALSE,world[to_room].people,0,0,TO_ROOM);
}


ASPELL(spell_chain_lightning)
{ mag_char_areas(level, ch, victim, SPELL_CHAIN_LIGHTNING, SAVING_SPELL);
}


ASPELL(spell_summon)
{ room_rnum to_room, fnd_room;
  int       is_newbie;

  if (ch == NULL || victim == NULL || ch == victim)
     return;

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3) &&
      !GET_COMMSTATE(ch) && !IS_NPC(ch)
     )
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  fnd_room = IN_ROOM(ch);
  if (fnd_room == NOWHERE || IN_ROOM(victim) == NOWHERE)
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (!IS_GOD(ch) && !IS_NPC(victim) &&
      (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOSUMMON) ||
       ROOM_FLAGGED(fnd_room, ROOM_NOSUMMON)        ||
       SECT(IN_ROOM(ch)) == SECT_SECRET             ||
       ROOM_FLAGGED(fnd_room, ROOM_DEATH)           ||
       ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH)       ||
       ROOM_FLAGGED(fnd_room, ROOM_TUNNEL)          ||
       (ROOM_FLAGGED(fnd_room, ROOM_PRIVATE) && !House_can_enter(victim, fnd_room)) ||
       ROOM_FLAGGED(fnd_room, ROOM_GODROOM)
     )
    )
    {send_to_char(SUMMON_FAIL, ch);
     return;
    }

  if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_AGGRESSIVE))
     {act("Вы произнесли СЛОВО, призвав $N1, котор$W прибыл$G через поля и веси к Вам.\r\n"
          "Но агрессивный характер заставил Вас отправить $S назад.",
          FALSE, ch, 0, victim, TO_CHAR);
      return;
     }

  if (!same_group(ch,victim))
     {if (may_pkill(ch,victim) != PC_REVENGE_PC)
         inc_pkill_group(victim,ch,1,0);
      else
         inc_pkill(victim,ch,0,1);
     }


  if (!IS_IMMORTAL(ch) &&
      ((IS_NPC(ch) && IS_NPC(victim)) ||
       (!IS_NPC(ch) && ((IS_NPC(victim) && MOB_FLAGGED(victim, MOB_NOSUMMON)) ||
                        general_savingthrow(victim, SAVING_SPELL, 0))
       )
      )
     )
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (!IS_NPC(victim))
     {if ((to_room = real_room(GET_LOADROOM(ch))) == NOWHERE)
         to_room = real_room(calc_loadroom(ch));
      is_newbie = (world[to_room].zone >= MIN_NEWBIE_ZONE && world[to_room].zone <= MAX_NEWBIE_ZONE);
      if ((is_newbie  && (world[fnd_room].zone < MIN_NEWBIE_ZONE || world[fnd_room].zone > MAX_NEWBIE_ZONE))  ||
          (!is_newbie && world[fnd_room].zone >= MIN_NEWBIE_ZONE && world[fnd_room].zone <= MAX_NEWBIE_ZONE)
         )
        {send_to_char(SUMMON_FAIL, ch);
         return;
        }
     }


  act("$n растворил$u на Ваших глазах.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, fnd_room);
  check_horse(victim);
  act("$n прибыл$g по вызову.", TRUE, victim, 0, 0, TO_ROOM);
  act("$n призвал$g Вас!", FALSE, ch, 0, victim, TO_VICT);
  look_at_room(victim, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}


ASPELL(spell_locate_object)
{
  struct obj_data *i;
  char name[MAX_INPUT_LENGTH];
  int j;

  /*
   * FIXME: This is broken.  The spell parser routines took the argument
   * the player gave to the spell and located an object with that keyword.
   * Since we're passed the object and not the keyword we can only guess
   * at what the player originally meant to search for. -gg
   */
  if (!obj)
     return;
  strcpy(name, fname(cast_argument));
  j = level / 2;


  for (i = object_list; i && (j > 0); i = i->next)
      {if (!isname(name, i->name))
          continue;

       if (i->carried_by)
          sprintf(buf, "%s находится у %s в инвентаре.\r\n",
	                   i->short_description, PERS(i->carried_by, ch, 1));
       else
       if (i->in_room != NOWHERE)
          sprintf(buf, "%s находится в  %s.\r\n", i->short_description,
                  world[i->in_room].name);
       else
       if (i->in_obj)
          sprintf(buf, "%s находится в %s.\r\n", i->short_description,
 	               i->in_obj->PNames[5]);
       else
       if (i->worn_by)
          sprintf(buf, "%s одет%s на %s.\r\n",
 	              i->short_description,
 	              GET_OBJ_SUF_6(i),
 	              PERS(i->worn_by, ch, 3));
       else
          sprintf(buf, "Местоположение %s неопределимо.\r\n",
	              OBJN(i,ch,1));

       CAP(buf);
       send_to_char(buf, ch);
       j--;
      }

  if (j == level / 2)
     send_to_char("Вы ничего не чувствуете.\r\n", ch);
}

ASPELL(spell_create_weapon)
{ go_create_weapon(ch,NULL,what_sky, 0); // prool
}


int check_charmee(struct char_data *ch, struct char_data *victim)
{struct follow_type *k;
 int    cha_summ = 0, hp_summ = 0;

 for (k = ch->followers; k; k = k->next)
     {if (AFF_FLAGGED(k->follower,AFF_CHARM) && k->follower->master == ch)
         {cha_summ++;
          hp_summ += GET_REAL_MAX_HIT(k->follower);
         }
     }

 if (cha_summ >= (GET_LEVEL(ch) + 9) / 10)
    {send_to_char("Вы не сможете управлять столькими последователями.\r\n",ch);
     return (FALSE);
    }

 if (!WAITLESS(ch) &&
     hp_summ + GET_REAL_MAX_HIT(victim) >= cha_app[GET_REAL_CHA(ch)].charms
    )
    {send_to_char("Вам не под силу управлять такой боевой мощью.\r\n",ch);
     return (FALSE);
    }
 return (TRUE);
}

ASPELL(spell_charm)
{ struct affected_type af;

  if (victim == NULL || ch == NULL)
     return;

  if (victim == ch)
     send_to_char("Вы просто очарованы своим внешним видом !\r\n", ch);
  else
  if (!IS_NPC(victim))
     {send_to_char("Вы не можете очаровать реального игрока !\r\n", ch);
      if (may_pkill(ch,victim) != PC_REVENGE_PC)
         inc_pkill(victim,ch,1,0);
      else
         inc_pkill(victim,ch,0,1);
     }
  else
  if (!IS_IMMORTAL(ch) && 
      (AFF_FLAGGED(victim, AFF_SANCTUARY) ||
       MOB_FLAGGED(victim, MOB_PROTECT)
      )
     )
     send_to_char("Ваша жертва освящена Богами !\r\n", ch);
  else
  if (!IS_IMMORTAL(ch) && MOB_FLAGGED(victim, MOB_NOCHARM))
     send_to_char("Ваша жертва устойчива к этому !\r\n", ch);
  else
  if (AFF_FLAGGED(ch, AFF_CHARM))
     send_to_char("Вы сами очарованы кем-то и не можете иметь последователей.\r\n", ch);
  else
  if (AFF_FLAGGED(victim, AFF_CHARM)         ||
      MOB_FLAGGED(victim, MOB_AGGRESSIVE)    ||
      MOB_FLAGGED(victim, MOB_AGGRMONO)      ||
      MOB_FLAGGED(victim, MOB_AGGRPOLY)      ||
      MOB_FLAGGED(victim, MOB_AGGR_DAY)      ||
      MOB_FLAGGED(victim, MOB_AGGR_NIGHT)    ||
      MOB_FLAGGED(victim, MOB_AGGR_FULLMOON) ||
      MOB_FLAGGED(victim, MOB_AGGR_WINTER)   ||
      MOB_FLAGGED(victim, MOB_AGGR_SPRING)   ||
      MOB_FLAGGED(victim, MOB_AGGR_SUMMER)   ||
      MOB_FLAGGED(victim, MOB_AGGR_AUTUMN)
     )
     send_to_char("Ваша магия потерпела неудачу.\r\n", ch);
  else
  if (IS_HORSE(victim))
     send_to_char("Это боевой скакун, а не хухры-мухры.\r\n", ch);
  else
  if (FIGHTING(victim) || GET_POS(victim) < POS_RESTING)
     act("$M сейчас, похоже, не до Вас.",FALSE,ch,0,victim,TO_CHAR);
  else
  if (circle_follow(victim, ch))
     send_to_char("Следование по кругу запрещено.\r\n", ch);
  else
  if (!IS_IMMORTAL(ch) &&
      general_savingthrow(victim, SAVING_PARA, (GET_REAL_CHA(ch) - 10) * 3))
     send_to_char("Ваша магия потерпела неудачу.\r\n", ch);
  else
     {if (!check_charmee(ch,victim))
         return;

      if (victim->master)
         {if (stop_follower(victim,SF_MASTERDIE))
             return;
         }
      affect_from_char(victim, SPELL_CHARM);
      add_follower(victim, ch);
      af.type = SPELL_CHARM;
      if (GET_REAL_INT(victim) > GET_REAL_INT(ch))
         af.duration = pc_duration(victim,GET_REAL_CHA(ch),0,0,0,0);
      else
         af.duration = pc_duration(victim,GET_REAL_CHA(ch)+number(1,10),0,0,0,0);
      af.modifier    = 0;
      af.location    = 0;
      af.bitvector   = AFF_CHARM;
      af.battleflag  = 0;
      affect_to_char(victim, &af);

      act("$n покорил$g Ваше сердце настолько, что Вы готовы на все ради н$s.", FALSE, ch, 0, victim, TO_VICT);
      if (IS_NPC(victim))
         {REMOVE_BIT(MOB_FLAGS(victim, MOB_AGGRESSIVE), MOB_AGGRESSIVE);
          REMOVE_BIT(MOB_FLAGS(victim, MOB_SPEC), MOB_SPEC);
         }
     }
}

ACMD(do_findhelpee)
{ struct char_data     *helpee;
  struct follow_type   *k;
  struct affected_type af;
  int    cost, times;

  if (IS_NPC(ch) ||
      (!WAITLESS(ch) && GET_CLASS(ch) != CLASS_MERCHANT)
     )
     {send_to_char("Вам недоступно это !\r\n",ch);
      return;
     }

  if (subcmd == SCMD_FREEHELPEE)
     {for (k = ch->followers; k; k=k->next)
          if (AFF_FLAGGED(k->follower, AFF_HELPER) &&
              AFF_FLAGGED(k->follower, AFF_CHARM))
             break;
      if (k)
         {act("Вы рассчитали $N3.",FALSE,ch,0,k->follower,TO_CHAR);
          affect_from_char(k->follower, SPELL_CHARM);
          stop_follower(k->follower, SF_CHARMLOST);
         }
      else
         act("У Вас нет наемников !",FALSE,ch,0,0,TO_CHAR);
      return;
     }

  argument = one_argument(argument, arg);

  if (!*arg)
     {for (k = ch->followers; k; k=k->next)
          if (AFF_FLAGGED(k->follower, AFF_HELPER) &&
              AFF_FLAGGED(k->follower, AFF_CHARM))
             break;
      if (k)
         act("Вашим наемником является $N.",FALSE,ch,0,k->follower,TO_CHAR);
      else
         act("У Вас нет наемников !",FALSE,ch,0,0,TO_CHAR);
      return;
     }

  if (!(helpee = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {send_to_char("Вы не видите никого похожего.\r\n", ch);
      return;
     }
  for (k = ch->followers; k; k=k->next)
      if (AFF_FLAGGED(k->follower, AFF_HELPER) &&
          AFF_FLAGGED(k->follower, AFF_CHARM))
         break;
  if (k)
     {act("Вы уже наняли $N3.",FALSE,ch,0,k->follower,TO_CHAR);
      return;
     }

  if (helpee == ch)
     send_to_char("И как Вы это представляете - нанять самого себя ?\r\n", ch);
  else
  if (!IS_NPC(helpee))
     send_to_char("Вы не можете нанять реального игрока !\r\n", ch);
  else
  if (!WAITLESS(ch) && !NPC_FLAGGED(helpee, NPC_HELPED))
     act("$N не нанимается !",FALSE,ch,0,helpee,TO_CHAR);
  else
  if (AFF_FLAGGED(helpee, AFF_CHARM))
     act("$N под чьим-то контролем.",FALSE,ch,0,helpee,TO_CHAR);
  else
  if (IS_HORSE(helpee))
     send_to_char("Это боевой скакун, а не хухры-мухры.\r\n", ch);
  else
  if (FIGHTING(helpee) || GET_POS(helpee) < POS_RESTING)
     act("$M сейчас, похоже, не до Вас.",FALSE,ch,0,helpee,TO_CHAR);
  else
  if (circle_follow(helpee, ch))
     send_to_char("Следование по кругу запрещено.\r\n", ch);
  else
     {one_argument(argument, arg);
      if (!arg)
          times = 0;
      else
      if ((times = atoi(arg)) < 0)
         {act("Уточните время, на которое Вы хотите нанять $N3.",FALSE,ch,0,helpee,TO_CHAR);
          return;
         }
      if (!times)
         {cost = GET_LEVEL(helpee) * TIME_KOEFF;
          sprintf(buf,"$n сказал$g Вам : \"Один час моих услуг стоит %d %s\".\r\n",
                  cost,desc_count(cost, WHAT_MONEYu));
          act(buf,FALSE,helpee,0,ch,TO_VICT);
          return;
         }
      cost =  (WAITLESS(ch) ? 0 : 1) * GET_LEVEL(helpee) * TIME_KOEFF * times;
      if (cost > GET_GOLD(ch))
         {sprintf(buf,"$n сказал$g Вам : \" Мои услуги за %d %s стоят %d %s - это тебе не по карману.\"",
                  times, desc_count(times, WHAT_HOUR),
                  cost, desc_count(cost, WHAT_MONEYu));
          act(buf,FALSE,helpee,0,ch,TO_VICT);		
          return;
         }
      if (GET_LEVEL(ch) < GET_GOLD(helpee))
         {sprintf(buf,"$n сказал$g Вам : \" Вы слишком малы для того, чтоб я служил Вам.\"");
          act(buf,FALSE,helpee,0,ch,TO_VICT);		
          return;
         }	 
       if (helpee->master)
          {if (stop_follower(helpee,SF_MASTERDIE))
              return;
          }
       GET_GOLD(ch)  -= cost;
       affect_from_char(helpee, AFF_CHARM);
       sprintf(buf,"$n сказал$g Вам : \"Приказывай, %s !\"",
               GET_SEX(ch) == SEX_FEMALE ? "хозяйка" : "хозяин");
       act(buf,FALSE,helpee,0,ch,TO_VICT);
       add_follower(helpee, ch);
       af.type        = SPELL_CHARM;
       af.duration    = pc_duration(helpee,times*TIME_KOEFF,0,0,0,0);
       af.modifier    = 0;
       af.location    = 0;
       af.bitvector   = AFF_CHARM;
       af.battleflag  = 0;
       affect_to_char(helpee, &af);
       SET_BIT(AFF_FLAGS(helpee, AFF_HELPER), AFF_HELPER);
       if (IS_NPC(helpee))
          {REMOVE_BIT(MOB_FLAGS(helpee, MOB_AGGRESSIVE), MOB_AGGRESSIVE);
           REMOVE_BIT(MOB_FLAGS(helpee, MOB_SPEC), MOB_SPEC);
          }
     }
}


void show_weapon(struct char_data *ch, struct obj_data *obj)
{
  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
    {*buf = '\0';
     if (CAN_WEAR(obj, ITEM_WEAR_WIELD))
        sprintf(buf, "Можно взять %s в правую руку.\r\n", OBJN(obj,ch,3));
     if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
        sprintf(buf+strlen(buf), "Можно взять %s в левую руку.\r\n", OBJN(obj,ch,3));
     if (CAN_WEAR(obj, ITEM_WEAR_BOTHS))
        sprintf(buf+strlen(buf), "Можно взять %s в обе руки.\r\n", OBJN(obj,ch,3));
     if (*buf)
        send_to_char(buf,ch);
    }
}


void mort_show_obj_values(struct obj_data *obj, struct char_data *ch,
                          int fullness)
{int i, found, drndice = 0, drsdice = 0, count = 0;

 send_to_char("Вы узнали следующее:\r\n", ch);
 sprintf(buf, "Предмет \"%s\", тип : ", obj->short_description);
 sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
 strcat(buf, buf2);
 strcat(buf, "\r\n");
 send_to_char(buf, ch);

 strcpy(buf, diag_weapon_to_char(obj, TRUE));
 if (*buf)
    send_to_char(buf, ch);

 if (fullness < 20)
    return;

 //show_weapon(ch, obj);

 sprintf(buf, "Вес: %d, Цена: %d, Рента: %d(%d)\r\n",
         GET_OBJ_WEIGHT(obj),
         GET_OBJ_COST(obj),
         GET_OBJ_RENT(obj),
         GET_OBJ_RENTEQ(obj));
 send_to_char(buf, ch);

 if (fullness < 30)
    return;

 send_to_char("Материал : ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprinttype(obj->obj_flags.Obj_mater, material_name, buf);
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 if (fullness < 40)
    return;

 send_to_char("Неудобен : ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.no_flag, no_bits, buf, ",");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 if (fullness < 50)
    return;

 send_to_char("Недоступен : ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.anti_flag, anti_bits, buf, ",");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 if (fullness < 60)
    return;

 send_to_char("Имеет экстрафлаги: ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.extra_flags, extra_bits, buf, ",");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 if (fullness < 75)
    return;

 switch (GET_OBJ_TYPE(obj))
 {
 case ITEM_SCROLL:
 case ITEM_POTION:
      sprintf(buf, "Содержит заклинания: ");
      if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
	     sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 1)));
	  if (GET_OBJ_VAL(obj, 2) >= 1 && GET_OBJ_VAL(obj, 2) < MAX_SPELLS)
	     sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 2)));
	  if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
	     sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 3)));
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
 case ITEM_WAND:
 case ITEM_STAFF:
      sprintf(buf, "Вызывает заклинания: ");
      if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
         sprintf(buf + strlen(buf), " %s\r\n", spell_name(GET_OBJ_VAL(obj, 3)));
      sprintf(buf + strlen(buf), "Зарядов %d (осталось %d).\r\n",
	          GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
      send_to_char(buf, ch);
      break;
 case ITEM_WEAPON:
      drndice = GET_OBJ_VAL(obj, 1);
      drsdice = GET_OBJ_VAL(obj, 2);
      sprintf(buf, "Наносимые повреждения '%dD%d'", drndice,drsdice);
      sprintf(buf + strlen(buf), " среднее %.1f.\r\n",
	          ((drsdice + 1) * drndice / 2.0));
      send_to_char(buf, ch);
      break;
 case ITEM_ARMOR:
      drndice = GET_OBJ_VAL(obj, 0);
      drsdice = GET_OBJ_VAL(obj, 1);
      sprintf(buf, "защита (AC) : %d\r\n", drndice);
      send_to_char(buf, ch);
      sprintf(buf, "броня       : %d\r\n", drsdice);
      send_to_char(buf, ch);
      break;
 case ITEM_BOOK:
      if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
         {drndice = GET_OBJ_VAL(obj,1);
          drsdice = spell_info[GET_OBJ_VAL(obj,1)].min_level[(int) GET_CLASS(ch)];
          sprintf(buf, "содержит заклинание        : \"%s\"\r\n", spell_info[drndice].name);
          send_to_char(buf,ch);
          sprintf(buf, "уровень изучения (для Вас) : %d\r\n", drsdice);
          send_to_char(buf,ch);
         }
      break;
 case ITEM_INGRADIENT:

      sprintbit(GET_OBJ_SKILL(obj),ingradient_bits,buf2);
      sprintf(buf,"%s\r\n",buf2);
      send_to_char(buf,ch);

      if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_USES))
         {sprintf(buf,"можно применить %d раз\r\n",GET_OBJ_VAL(obj,2));
          send_to_char(buf,ch);
         }

      if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_LAG))
         {sprintf(buf,"можно применить 1 раз в %d сек",(i = GET_OBJ_VAL(obj,0) & 0xFF));
          if (GET_OBJ_VAL(obj,3) == 0 ||
              GET_OBJ_VAL(obj,3) + i < time(NULL))
             strcat(buf,"(можно применять).\r\n");
          else
             sprintf(buf + strlen(buf),"(осталось %ld сек).\r\n",GET_OBJ_VAL(obj,3) + i - time(NULL));
          send_to_char(buf,ch);
         }

      if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_LEVEL))
         {sprintf(buf,"можно применить c %d уровня.\r\n",(GET_OBJ_VAL(obj,0) >> 8) & 0x1F);
          send_to_char(buf,ch);
         }

      if ((i = real_object(GET_OBJ_VAL(obj,1))) >= 0)
         {sprintf(buf,"прототип %s%s%s.\r\n",
                      CCICYN(ch, C_NRM),
                      (obj_proto+i)->PNames[0],
                      CCNRM(ch,C_NRM));
          send_to_char(buf,ch);
         }
      break;

 }


 if (fullness < 90)
    return;

 send_to_char("Накладывает на Вас аффекты: ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.affects, weapon_affects, buf, ",");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 if (fullness < 100)
    return;

 found = FALSE;
 count = MAX_OBJ_AFFECT;
 for (i = 0; i < count; i++)
     {drndice = obj->affected[i].location;
      drsdice = obj->affected[i].modifier;
      if ((drndice != APPLY_NONE) &&
          (drsdice != 0))
         {if (!found)
	         {send_to_char("Дополнительные свойства :\r\n", ch);
	          found = TRUE;
	         }
	      sprinttype(drndice, apply_types, buf2);
	      sprintf(buf, "   %s%s%s изменяет на %s%s%d%s\r\n",
	              CCCYN(ch, C_NRM),buf2,CCNRM(ch,C_NRM),
	              CCCYN(ch, C_NRM),
		      drsdice > 0 ? "+" : "",drsdice,
		      CCNRM(ch,C_NRM));
	      send_to_char(buf, ch);
          }
     }
}

void imm_show_obj_values(struct obj_data *obj, struct char_data *ch)
{int i, found;

 send_to_char("Вы узнали следующее:\r\n", ch);
 sprintf(buf, "Предмет \"%s\", тип : ", obj->short_description);
 sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
 strcat(buf, buf2);
 strcat(buf, "\r\n");
 send_to_char(buf, ch);

 strcpy(buf, diag_weapon_to_char(obj, TRUE));
 if (*buf)
    send_to_char(buf, ch);

 //show_weapon(ch, obj);

 send_to_char("Материал : ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprinttype(obj->obj_flags.Obj_mater, material_name, buf);
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 sprintf(buf,"Таймер : %d\r\n", GET_OBJ_TIMER(obj));
 send_to_char(buf, ch);

 send_to_char("Накладывает на Вас аффекты: ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.affects, weapon_affects, buf, ",");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 send_to_char("Имеет экстрафлаги: ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.extra_flags, extra_bits, buf, ",");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 send_to_char("Недоступен : ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.anti_flag, anti_bits, buf, ",");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 send_to_char("Неудобен : ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.no_flag, no_bits, buf, ",");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 sprintf(buf, "Вес: %d, Цена: %d, Рента: %d(%d)\r\n",
              GET_OBJ_WEIGHT(obj),
	          GET_OBJ_COST(obj),
	          GET_OBJ_RENT(obj),
	          GET_OBJ_RENTEQ(obj));
 send_to_char(buf, ch);

 switch (GET_OBJ_TYPE(obj))
 {
 case ITEM_SCROLL:
 case ITEM_POTION:
      sprintf(buf, "Содержит заклинания: ");
      if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
	     sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 1)));
      if (GET_OBJ_VAL(obj, 2) >= 1 && GET_OBJ_VAL(obj, 2) < MAX_SPELLS)
	     sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 2)));
      if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
	     sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 3)));
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
 case ITEM_WAND:
 case ITEM_STAFF:
      sprintf(buf, "Вызывает заклинания: ");
      if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
         sprintf(buf + strlen(buf), " %s\r\n", spell_name(GET_OBJ_VAL(obj, 3)));
      sprintf(buf + strlen(buf), "Зарядов %d (осталось %d).\r\n",
	          GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
      send_to_char(buf, ch);
      break;
 case ITEM_WEAPON:
      sprintf(buf, "Наносимые повреждения '%dD%d'",
              GET_OBJ_VAL(obj, 1),GET_OBJ_VAL(obj, 2));
      sprintf(buf + strlen(buf), " среднее %.1f.\r\n",
	          (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
      send_to_char(buf, ch);
      break;
 case ITEM_ARMOR:
      sprintf(buf, "защита (AC) : %d\r\n", GET_OBJ_VAL(obj, 0));
      send_to_char(buf, ch);
      sprintf(buf, "броня       : %d\r\n", GET_OBJ_VAL(obj, 1));
      send_to_char(buf, ch);
      break;
 case ITEM_BOOK:
      if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
         {sprintf(buf, "содержит заклинание        : \"%s\"\r\n",
                  spell_info[GET_OBJ_VAL(obj,1)].name);
          send_to_char(buf,ch);
          sprintf(buf, "уровень изучения (для Вас) : %d\r\n",
                  spell_info[GET_OBJ_VAL(obj,1)].min_level[(int)GET_CLASS(ch)]);
          send_to_char(buf,ch);
        }
      break;
 case ITEM_INGRADIENT:

      sprintbit(GET_OBJ_SKILL(obj),ingradient_bits,buf2);
      sprintf(buf,"%s\r\n",buf2);
      send_to_char(buf,ch);

      if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_USES))
         {sprintf(buf,"можно применить %d раз\r\n",GET_OBJ_VAL(obj,2));
          send_to_char(buf,ch);
         }

      if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_LAG))
         {sprintf(buf,"можно применить 1 раз в %d сек",(i = GET_OBJ_VAL(obj,0) & 0xFF));
          if (GET_OBJ_VAL(obj,3) == 0 ||
              GET_OBJ_VAL(obj,3) + i < time(NULL))
             strcat(buf,"(можно применять).\r\n");
          else
             sprintf(buf + strlen(buf),"(осталось %ld сек).\r\n",GET_OBJ_VAL(obj,3) + i - time(NULL));
          send_to_char(buf,ch);
         }

      if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_LEVEL))
         {sprintf(buf,"можно применить c %d уровня.\r\n",(GET_OBJ_VAL(obj,0) >> 8) & 0x1F);
          send_to_char(buf,ch);
         }

      if ((i = real_object(GET_OBJ_VAL(obj,1))) >= 0)
         {sprintf(buf,"прототип %s%s%s.\r\n",
                      CCICYN(ch, C_NRM),
                      (obj_proto+i)->PNames[0],
                      CCNRM(ch,C_NRM));
          send_to_char(buf,ch);
         }
      break;
 }

 found = FALSE;
 for (i = 0; i < MAX_OBJ_AFFECT; i++)
     {if ((obj->affected[i].location != APPLY_NONE) &&
          (obj->affected[i].modifier != 0))
	     {if (!found)
	         {send_to_char("Дополнительные свойства :\r\n", ch);
	          found = TRUE;
	         }
	      sprinttype(obj->affected[i].location, apply_types, buf2);
	      sprintf(buf, "   %s%s%s изменяет на %s%s%d%s\r\n",
	                   CCCYN(ch, C_NRM),buf2,CCNRM(ch,C_NRM),
	                   CCCYN(ch, C_NRM),
			   obj->affected[i].modifier > 0 ? "+" : "",
			   obj->affected[i].modifier,
			   CCNRM(ch,C_NRM));
          send_to_char(buf, ch);
         }
     }
}

#define IDENT_SELF_LEVEL 6

void mort_show_char_values(struct char_data *victim, struct char_data *ch, int fullness)
{struct affected_type *aff;
 int    found, val0, val1, val2;

 sprintf(buf, "Имя: %s\r\n", GET_NAME(victim));
 send_to_char(buf, ch);
 if (!IS_NPC(victim) && victim == ch)
    {sprintf(buf, "Написание : %s/%s/%s/%s/%s/%s\r\n",
                 GET_PAD(victim,0),GET_PAD(victim,1),GET_PAD(victim,2),
                 GET_PAD(victim,3),GET_PAD(victim,4),GET_PAD(victim,5));
     send_to_char(buf,ch);
    }

 if (!IS_NPC(victim) && victim == ch)
    {sprintf(buf, "Возраст %s  : %d лет, %d месяцев, %d дней и %d часов.\r\n",
              GET_PAD(victim,1),
              age(victim)->year,
              age(victim)->month,
              age(victim)->day,
              age(victim)->hours);
      send_to_char(buf, ch);
     }
 if (fullness < 20 && ch != victim)
    return;

 val0 = GET_HEIGHT(victim);
 val1 = GET_WEIGHT(victim);
 val2 = GET_SIZE(victim);
 sprintf(buf, /*"Рост %d ,*/" Вес %d, Размер %d\r\n", /*val0, */val1, val2);
 send_to_char(buf, ch);
 if (fullness < 60 && ch != victim)
    return;

 val0 = GET_LEVEL(victim);
 val1 = GET_HIT(victim);
 val2 = GET_REAL_MAX_HIT(victim);
 sprintf(buf, "Уровень : %d, может выдержать повреждений : %d(%d)\r\n",
         val0, val1, val2);
 send_to_char(buf,ch);	
 if (fullness < 90 && ch != victim)
    return;

 val0 = compute_armor_class(victim);
 val1 = GET_HR(victim);
 val2 = GET_DR(victim);
 sprintf(buf, "Защита : %d, Атака : %d, Повреждения : %d\r\n",
         val0, val1, val2);
 send_to_char(buf, ch);

 if (fullness < 100 || (ch != victim && !IS_NPC(victim)))
    return;

 val0 = GET_STR(victim);
 val1 = GET_INT(victim);
 val2 = GET_WIS(victim);
 sprintf(buf, "Сила: %d, Ум: %d, Муд: %d, ", val0, val1, val2);
 val0 = GET_DEX(victim);
 val1 = GET_CON(victim);
 val2 = GET_CHA(victim);
 sprintf(buf+strlen(buf),"Ловк: %d, Тел: %d, Обаян: %d\r\n",val0,val1,val2);
 send_to_char(buf, ch); 	

 if (fullness < 120 || (ch != victim && !IS_NPC(victim)))
    return;

 for (aff = victim->affected, found = FALSE; aff; aff = aff->next)
     {if (aff->location != APPLY_NONE &&
          aff->modifier != 0)
         {if (!found)
             {send_to_char("Дополнительные свойства :\r\n", ch);
              found = TRUE;
              send_to_char(CCIRED(ch, C_NRM), ch);
             }
          sprinttype(aff->location, apply_types, buf2);
          sprintf(buf, "   %s изменяет на %s%d\r\n", buf2,
                  aff->modifier > 0 ? "+" : "", aff->modifier);
          send_to_char(buf, ch);
         }
     }
 send_to_char(CCNRM(ch, C_NRM), ch);

 send_to_char("Аффекты :\r\n", ch);
 send_to_char(CCICYN(ch, C_NRM), ch);
 sprintbits(victim->char_specials.saved.affected_by, affected_bits, buf2, "\r\n");
 sprintf(buf, "%s\r\n", buf2);
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);
}

void imm_show_char_values(struct char_data *victim, struct char_data *ch)
{struct affected_type *aff;
 int    found;

 sprintf(buf, "Имя: %s\r\n", GET_NAME(victim));
 send_to_char(buf, ch);
 sprintf(buf, "Написание : %s/%s/%s/%s/%s/%s\r\n",
         GET_PAD(victim,0),GET_PAD(victim,1),GET_PAD(victim,2),
         GET_PAD(victim,3),GET_PAD(victim,4),GET_PAD(victim,5));
 send_to_char(buf,ch);

 if (!IS_NPC(victim))
    {sprintf(buf, "Возраст %s  : %d лет, %d месяцев, %d дней и %d часов.\r\n",
              GET_PAD(victim,1),
              age(victim)->year,
              age(victim)->month,
	          age(victim)->day,
	          age(victim)->hours);
      send_to_char(buf, ch);
     }

 sprintf(buf, "Рост %d(%s%d%s), Вес %d(%s%d%s), Размер %d(%s%d%s)\r\n",
         GET_HEIGHT(victim),
         CCIRED(ch, C_NRM), GET_REAL_HEIGHT(victim), CCNRM(ch, C_NRM),
         GET_WEIGHT(victim),
         CCIRED(ch, C_NRM), GET_REAL_WEIGHT(victim), CCNRM(ch, C_NRM),
         GET_SIZE(victim),
         CCIRED(ch, C_NRM), GET_REAL_SIZE(victim), CCNRM(ch, C_NRM)
         );
 send_to_char(buf, ch);

 sprintf(buf, "Уровень : %d, может выдержать повреждений : %d(%d,%s%d%s)\r\n",
	          GET_LEVEL(victim), GET_HIT(victim), GET_MAX_HIT(victim),
	          CCIRED(ch, C_NRM), GET_REAL_MAX_HIT(victim), CCNRM(ch, C_NRM)
	          );
 send_to_char(buf,ch);	

 sprintf(buf, "Защита : %d(%s%d%s), Атака : %d(%s%d%s), Повреждения : %d(%s%d%s)\r\n",
         GET_AC(victim),
         CCIRED(ch, C_NRM), compute_armor_class(victim), CCNRM(ch, C_NRM),
         GET_HR(victim),
         CCIRED(ch, C_NRM), GET_REAL_HR(victim), CCNRM(ch, C_NRM),
         GET_DR(victim),
         CCIRED(ch, C_NRM), GET_REAL_DR(victim), CCNRM(ch, C_NRM));
 send_to_char(buf, ch);

 sprintf(buf, "Сила: %d, Ум: %d, Муд: %d, Ловк: %d, Тел: %d, Обаян: %d\r\n",
     	                    GET_STR(victim), GET_INT(victim),GET_WIS(victim),
	                        GET_DEX(victim), GET_CON(victim), GET_CHA(victim));
 send_to_char(buf, ch); 	
 sprintf(buf, "Сила: %s%d%s, Ум: %s%d%s, Муд: %s%d%s, Ловк: %s%d%s, Тел: %s%d%s, Обаян: %s%d%s\r\n",
         CCIRED(ch, C_NRM), GET_REAL_STR(victim), CCNRM(ch, C_NRM),
         CCIRED(ch, C_NRM), GET_REAL_INT(victim), CCNRM(ch, C_NRM),
         CCIRED(ch, C_NRM), GET_REAL_WIS(victim), CCNRM(ch, C_NRM),
         CCIRED(ch, C_NRM), GET_REAL_DEX(victim), CCNRM(ch, C_NRM),
         CCIRED(ch, C_NRM), GET_REAL_CON(victim), CCNRM(ch, C_NRM),
         CCIRED(ch, C_NRM), GET_REAL_CHA(victim), CCNRM(ch, C_NRM)
        );
 send_to_char(buf, ch);

 for (aff = victim->affected, found = FALSE; aff; aff = aff->next)
     {if (aff->location != APPLY_NONE &&
          aff->modifier != 0)
         {if (!found)
             {send_to_char("Дополнительные свойства :\r\n", ch);
              found = TRUE;
              send_to_char(CCIRED(ch, C_NRM), ch);
             }
          sprinttype(aff->location, apply_types, buf2);
          sprintf(buf, "   %s изменяет на %s%d\r\n", buf2,
                  aff->modifier > 0 ? "+" : "", aff->modifier);
          send_to_char(buf, ch);
         }
     }
 send_to_char(CCNRM(ch, C_NRM), ch);

 send_to_char("Аффекты :\r\n", ch);
 send_to_char(CCIBLU(ch, C_NRM), ch);
 sprintbits(victim->char_specials.saved.affected_by, affected_bits, buf2, "\r\n");
 sprintf(buf, "%s\r\n", buf2);
 if (victim->followers)
    sprintf(buf + strlen(buf), "Имеет последователей.\r\n");
 else
 if (victim->master)
    sprintf(buf + strlen(buf), "Следует за %s.\r\n", GET_PAD(victim->master, 4));
 sprintf(buf + strlen(buf), "Уровень повреждений %d, уровень заклинаний %d.\r\n", GET_DAMAGE(victim), GET_CASTER(victim));
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);
}

ASPELL(skill_identify)
{
  if (obj)
     if (IS_IMMORTAL(ch))
        imm_show_obj_values(obj,ch);
     else
        mort_show_obj_values(obj,ch,train_skill(ch,SKILL_IDENTIFY,skill_info[SKILL_IDENTIFY].max_percent,0));
  else
  if (victim)
     {if (IS_IMMORTAL(ch))
         imm_show_char_values(victim,ch);
      else
      if (GET_LEVEL(victim) < 3 )
         {send_to_char("Вы можете опознать только персонажа, достигнувшего третьего уровня.\r\n", ch);
          return;	  
         }
         mort_show_char_values(victim,ch,train_skill(ch,SKILL_IDENTIFY,skill_info[SKILL_IDENTIFY].max_percent,victim));
     }
}

ASPELL(spell_identify)
{
  if (obj)
     mort_show_obj_values(obj,ch,100);
  else
  if (victim)
     {if (victim != ch)
         {send_to_char("С помощью магии нельзя опознать другое существо.\r\n", ch);
          return;
	 }
     if (GET_LEVEL(victim) < 3 )
         {send_to_char("Вы можете опознать себя только достигнув третьего уровня.\r\n", ch);
          return;	  
         }
      mort_show_char_values(victim,ch,100);
     }
}



/*
 * Cannot use this spell on an equipped object or it will mess up the
 * wielding character's hit/dam totals.
 */
ASPELL(spell_enchant_weapon)
{
  int i, last = 0;

  if (ch == NULL || obj == NULL)
    return;

  /* Either already enchanted or not a weapon. */
  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON ||
      OBJ_FLAGGED(obj, ITEM_MAGIC)
     )
     return;

  /* Make sure no other affections. */
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != APPLY_NONE)
         {if (WAITLESS(ch))
             last = i;
          else
             return;
         }

  SET_BIT(GET_OBJ_EXTRA(obj, ITEM_MAGIC), ITEM_MAGIC);

  if ((++last) < MAX_OBJ_AFFECT)
     {obj->affected[last].location = APPLY_HITROLL;
      obj->affected[last].modifier = 1 + (WAITLESS(ch) ? 3 : (level >= 18));
     }
  if ((++last) < MAX_OBJ_AFFECT)
     {obj->affected[last].location = APPLY_DAMROLL;
      obj->affected[last].modifier = 1 + (WAITLESS(ch) ? 3 : (level >= 20));
     }

  if (!WAITLESS(ch))
     GET_OBJ_TIMER(obj) = MAX(GET_OBJ_TIMER(obj), 4 * 24 * 60);

  if (GET_RELIGION(ch) == RELIGION_MONO)
     {SET_BIT(GET_FLAG(obj->obj_flags.anti_flag, ITEM_AN_POLY), ITEM_AN_POLY);
      act("$o вспыхнул$G на миг голубым светом и тут же потух$Q.", FALSE, ch, obj, 0, TO_CHAR);
     }
  else
  if (GET_RELIGION(ch) == RELIGION_POLY)
     {SET_BIT(GET_FLAG(obj->obj_flags.anti_flag, ITEM_AN_MONO), ITEM_AN_MONO);
      act("$o вспыхнул$G на миг красным светом и тут же потух$Q.", FALSE, ch, obj, 0, TO_CHAR);
     }
  else
     act("$o вспыхнул$G на миг желтым светом и тут же потух$Q.", FALSE, ch, obj, 0, TO_CHAR);
}


ASPELL(spell_detect_poison)
{
  if (victim)
     {if (victim == ch)
         {if (AFF_FLAGGED(victim, AFF_POISON))
             send_to_char("Вы чувствуете яд в своей крови.\r\n", ch);
          else
             send_to_char("Вы чувствуете себя здоровым.\r\n", ch);
         }
      else
         {if (AFF_FLAGGED(victim, AFF_POISON))
             act("Аура $N1 пропитана ядом.", FALSE, ch, 0, victim, TO_CHAR);
          else
             act("Аура $N1 имеет ровную окраску.", FALSE, ch, 0, victim, TO_CHAR);
         }
     }

  if (obj)
     {switch (GET_OBJ_TYPE(obj))
      {
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
      if (GET_OBJ_VAL(obj, 3))
	act("Аура $o1 пропитана ядом.",FALSE,ch,obj,0,TO_CHAR);
      else
	act("Аура $o1 имеет ровную окраску.", FALSE, ch, obj, 0,TO_CHAR);
      break;
    default:
      send_to_char("Этот предмет не может быть отравлен.\r\n", ch);
      }
     }
}

ASPELL(spell_control_weather)
{char *sky_info=NULL;
 int  i, duration, zone, sky_type = 0;

 if (what_sky > SKY_LIGHTNING)
    what_sky = SKY_LIGHTNING;

 switch (what_sky)
 {case SKY_CLOUDLESS:
    sky_info = "Небо покрылось облаками.";
    break;
  case SKY_CLOUDY:
    sky_info = "Небо покрылось тяжелыми тучами.";
    break;
  case SKY_RAINING:
    if (time_info.month >= MONTH_MAY      &&   time_info.month <= MONTH_OCTOBER)
       {sky_info = "Начался проливной дождь.";
        create_rainsnow(&sky_type,WEATHER_LIGHTRAIN,0,50,50);
       }
    else
    if (time_info.month >= MONTH_DECEMBER ||   time_info.month <= MONTH_FEBRUARY)
       {sky_info = "Повалил снег.";
        create_rainsnow(&sky_type,WEATHER_LIGHTSNOW,0,50,50);
       }
    else
    if (time_info.month == MONTH_MART || time_info.month == MONTH_NOVEMBER)
       {if (weather_info.temperature > 2)
           {sky_info = "Начался проливной дождь.";
            create_rainsnow(&sky_type,WEATHER_LIGHTRAIN,0,50,50);
           }
        else
           {sky_info = "Повалил снег.";
            create_rainsnow(&sky_type,WEATHER_LIGHTSNOW,0,50,50);
           }
       }
    break;
  case SKY_LIGHTNING:
    sky_info = "На небе не осталось ни единого облачка.";
    break;
  default:
    break;
  }

 if (sky_info)
    { duration = MAX(GET_LEVEL(ch) / 8, 2);
      zone     = world[ch->in_room].zone;
      for (i = 0; i <= top_of_world; i++)
          if (world[i].zone == zone && SECT(i) != SECT_INSIDE && SECT(i) != SECT_CITY)
             {world[i].weather.sky          = what_sky;
              world[i].weather.weather_type = sky_type;
              world[i].weather.duration     = duration;
              if (world[i].people)
                 {act(sky_info,FALSE,world[i].people,0,0,TO_ROOM);
                  act(sky_info,FALSE,world[i].people,0,0,TO_CHAR);
                 }
             }
    }
}
