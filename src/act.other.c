/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __ACT_OTHER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern char *class_abbrevs[];
extern int free_rent;
extern int max_filesize;
extern int nameserver_is_slow;
extern int auto_save;
extern int track_through_doors;

/* extern procedures */
void list_skills(struct char_data * ch);
void list_spells(struct char_data * ch);
void appear(struct char_data * ch);
void write_aliases(struct char_data *ch);
void perform_immort_vis(struct char_data *ch);
int  have_mind(struct char_data *ch);
SPECIAL(shop_keeper);
ACMD(do_gen_comm);
void die(struct char_data * ch, struct char_data * killer);
void Crash_rentsave(struct char_data * ch, int cost);
int  Crash_delete_file(char *name, int mask);
int  inc_pkill(struct char_data * victim, struct char_data *killer, int pkills, int prevenge);
int  HaveMind(struct char_data *ch);
char *color_value(struct char_data *ch,int real, int max);
int  posi_value(int real, int max);
int  inc_pk_thiefs(struct char_data *ch, struct char_data *victim);
/* local functions */
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_sneak);
ACMD(do_hide);
ACMD(do_steal);
ACMD(do_spells);
ACMD(do_skills);
ACMD(do_visible);
ACMD(do_title);
int perform_group(struct char_data *ch, struct char_data *vict);
void print_group(struct char_data *ch);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_split);
ACMD(do_use);
ACMD(do_wimpy);
ACMD(do_display);
ACMD(do_gen_write);
ACMD(do_gen_tog);
ACMD(do_courage);
ACMD(do_toggle);
ACMD(do_color);


ACMD(do_quit)
{
  struct descriptor_data *d, *next_d;

  if (IS_NPC(ch) || !ch->desc)
     return;

  if (subcmd != SCMD_QUIT)
     send_to_char("Вам стоит набрать эту команду полностью во избежание недоразумений!\r\n", ch);
  else
  if (GET_POS(ch) == POS_FIGHTING)
     send_to_char("Угу ! Щаз-з-з!  Вы, батенька, деретесь !\r\n", ch);
  else
  if (GET_POS(ch) < POS_STUNNED)
     {send_to_char("Вас пригласила к себе владелица косы...\r\n", ch);
      die(ch, NULL);
     }
  else
  if (AFF_FLAGGED(ch,AFF_SLEEP))
     {return;
     }
  else
     {int loadroom = ch->in_room;
      if (RENTABLE(ch))
         {send_to_char("В связи с боевыми действиями эвакуация временно прекращена.\r\n",ch);
	  return;
         }	
      if (!GET_INVIS_LEV(ch))
         act("$n покинул$g игру.", TRUE, ch, 0, 0, TO_ROOM);
      sprintf(buf, "%s quit the game.", GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      send_to_char("До свидания, странник... Мы ждем тебя снова !\r\n", ch);

      /*
       * kill off all sockets connected to the same player as the one who is
       * trying to quit.  Helps to maintain sanity as well as prevent duping.
       */
      for (d = descriptor_list; d; d = next_d)
          {next_d = d->next;
           if (d == ch->desc)
              continue;
           if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
              STATE(d) = CON_DISCONNECT;
          }

      if (free_rent || IS_GOD(ch))
         Crash_rentsave(ch, 0);
      else
         Crash_delete_file(GET_NAME(ch),CRASH_DELETE_OLD | CRASH_DELETE_NEW);
      /* Char is saved in extract char */
      extract_char(ch,FALSE);

      /* If someone is quitting in their house, let them load back here */
      if (ROOM_FLAGGED(loadroom, ROOM_HOUSE))
         save_char(ch, loadroom);
     }
}



ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Only tell the char we're saving if they actually typed "save" */
  if (cmd)
     { /*
        * This prevents item duplication by two PC's using coordinated saves
        * (or one PC with a house) and system crashes. Note that houses are
        * still automatically saved without this enabled. This code assumes
        * that guest immortals aren't trustworthy. If you've disabled guest
        * immortal advances from mortality, you may want < instead of <=.
        */
      if (auto_save && GET_LEVEL(ch) <= LVL_IMMORT)
         {send_to_char("Записываю синонимы.\r\n", ch);
          write_aliases(ch);
          return;
         }
      sprintf(buf, "Записываю %s и алиасы.\r\n", GET_NAME(ch));
      send_to_char(buf, ch);
     }

  write_aliases(ch);
  save_char(ch, NOWHERE);
  Crash_crashsave(ch);
  if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE_CRASH))
      House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char("Эта команда недоступна в этом месте !\r\n", ch);
}

int equip_in_metall(struct char_data * ch)
{int i, wgt = 0;

 if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
    return (FALSE);
 if (IS_GOD(ch))
    return (FALSE);

 for (i = 0; i < NUM_WEARS; i++)
     {if (GET_EQ(ch,i) &&
          GET_OBJ_TYPE(GET_EQ(ch,i))  == ITEM_ARMOR &&
          GET_OBJ_MATER(GET_EQ(ch,i)) <= MAT_COLOR
         )
         wgt += GET_OBJ_WEIGHT(GET_EQ(ch,i));
     }

 if (wgt > GET_REAL_STR(ch))
    return (TRUE);

 return (FALSE);
}

int awake_others(struct char_data * ch)
{int i;

 if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
    return (FALSE);

 if (IS_GOD(ch))
    return (FALSE);

 if (AFF_FLAGGED(ch, AFF_STAIRS)      ||
     AFF_FLAGGED(ch, AFF_SANCTUARY)   ||
     AFF_FLAGGED(ch, AFF_SINGLELIGHT) ||
     AFF_FLAGGED(ch, AFF_HOLYLIGHT)
    )
    return (TRUE);

 for (i = 0; i < NUM_WEARS; i++)
     {if (GET_EQ(ch,i))
         if ((GET_OBJ_TYPE(GET_EQ(ch,i))        == ITEM_ARMOR &&
              GET_EQ(ch,i)->obj_flags.Obj_mater <= MAT_COLOR) ||
             OBJ_FLAGGED(GET_EQ(ch,i), ITEM_HUM) ||
             OBJ_FLAGGED(GET_EQ(ch,i), ITEM_GLOW)
            )
            return (FALSE);
      }
 return (FALSE);
}

int check_awake(struct char_data *ch, int what)
{int i, retval = 0, wgt = 0;

 if (!IS_GOD(ch))
    {if (IS_SET(what, ACHECK_AFFECTS) &&
         (AFF_FLAGGED(ch, AFF_STAIRS) ||
          AFF_FLAGGED(ch, AFF_SANCTUARY)
         )
        )
        SET_BIT(retval, ACHECK_AFFECTS);

     if (IS_SET(what, ACHECK_LIGHT)  &&
         IS_DEFAULTDARK(IN_ROOM(ch)) &&
         (AFF_FLAGGED(ch, AFF_SINGLELIGHT) ||
          AFF_FLAGGED(ch, AFF_HOLYLIGHT)
         )
        )
        SET_BIT(retval, ACHECK_LIGHT);

     for (i = 0; i < NUM_WEARS; i++)
         {if (!GET_EQ(ch,i))
             continue;

          if (IS_SET(what,ACHECK_HUMMING) &&
              OBJ_FLAGGED(GET_EQ(ch,i), ITEM_HUM)
             )
             SET_BIT(retval, ACHECK_HUMMING);

          if (IS_SET(what,ACHECK_GLOWING) &&
              OBJ_FLAGGED(GET_EQ(ch,i), ITEM_GLOW)
             )
             SET_BIT(retval, ACHECK_GLOWING);

          if (IS_SET(what, ACHECK_LIGHT)  &&
              IS_DEFAULTDARK(IN_ROOM(ch)) &&
              GET_OBJ_TYPE(GET_EQ(ch,i)) == ITEM_LIGHT &&
              GET_OBJ_VAL(GET_EQ(ch,i), 2)
             )
             SET_BIT(retval, ACHECK_LIGHT);

          if (GET_OBJ_TYPE(GET_EQ(ch,i))  == ITEM_ARMOR &&
              GET_OBJ_MATER(GET_EQ(ch,i)) <= MAT_COLOR
             )
             wgt += GET_OBJ_WEIGHT(GET_EQ(ch,i));
         }

     if (IS_SET(what, ACHECK_WEIGHT) &&
         wgt > GET_REAL_STR(ch) * 2
        )
        SET_BIT(retval, ACHECK_WEIGHT);
    }
 return (retval);
}

int awake_hide(struct char_data *ch)
{if (IS_GOD(ch))
    return (FALSE);
 return check_awake(ch, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING | ACHECK_GLOWING | ACHECK_WEIGHT);
}

int awake_invis(struct char_data *ch)
{if (IS_GOD(ch))
    return (FALSE);
 return check_awake(ch, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING | ACHECK_GLOWING);
}

int awake_camouflage(struct char_data *ch)
{if (IS_GOD(ch))
    return (FALSE);
 return check_awake(ch, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING | ACHECK_GLOWING);
}

int awake_sneak(struct char_data *ch)
{if (IS_GOD(ch))
    return (FALSE);
 return check_awake(ch, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING | ACHECK_GLOWING | ACHECK_WEIGHT);
}

int awaking(struct char_data *ch, int mode)
{if (IS_GOD(ch))
    return (FALSE);
 if (IS_SET(mode,AW_HIDE) && awake_hide(ch))
    return (TRUE);
 if (IS_SET(mode,AW_INVIS) && awake_invis(ch))
    return (TRUE);
 if (IS_SET(mode,AW_CAMOUFLAGE) && awake_camouflage(ch))
    return (TRUE);
 if (IS_SET(mode,AW_SNEAK) && awake_sneak(ch))
    return (TRUE);
 return (FALSE);
}

int char_humming(struct char_data *ch)
{int i;

 if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
    return (FALSE);

 for (i = 0; i < NUM_WEARS; i++)
     {if (GET_EQ(ch,i) &&
          OBJ_FLAGGED(GET_EQ(ch,i), ITEM_HUM))
          return (TRUE);
     }
 return (FALSE);
}

int char_glowing(struct char_data *ch)
{int i;

 if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
    return (FALSE);

 for (i = 0; i < NUM_WEARS; i++)
     {if (GET_EQ(ch,i) &&
          OBJ_FLAGGED(GET_EQ(ch,i), ITEM_GLOW))
          return (TRUE);
     }
 return (FALSE);
}


ACMD(do_sneak)
{
  struct affected_type af;
  ubyte  prob, percent;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SNEAK))
     {send_to_char("Но Вы не знаете как.\r\n", ch);
      return;
     }

  if (on_horse(ch))
     {act("Вам стоит подумать о мягкой обуви для $N1", FALSE, ch, 0, get_horse(ch), TO_CHAR);
      return;
     }

  affect_from_char(ch, SPELL_SNEAK);

  if (affected_by_spell(ch, SPELL_SNEAK))
     {send_to_char("Вы уже пытаетесь красться.\r\n", ch);
      return;
     }

  send_to_char("Хорошо, Вы попытаетесь двигаться бесшумно.\r\n", ch);
  REMOVE_BIT(EXTRA_FLAGS(ch,EXTRA_FAILSNEAK), EXTRA_FAILSNEAK);
  percent = number(1,skill_info[SKILL_SNEAK].max_percent);
  prob    = calculate_skill(ch,SKILL_SNEAK,skill_info[SKILL_SNEAK].max_percent,0);

  af.type       = SPELL_SNEAK;
  af.duration   = pc_duration(ch,0,GET_LEVEL(ch),8,0,1);
  af.modifier   = 0;
  af.location   = APPLY_NONE;
  af.battleflag = 0;
  if (percent > prob)
     af.bitvector = 0;
  else
     af.bitvector = AFF_SNEAK;
  affect_to_char(ch, &af);
}

ACMD(do_camouflage)
{
  struct affected_type af;
  struct timed_type    timed;
  ubyte  prob, percent;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_CAMOUFLAGE))
     {send_to_char("Но Вы не знаете как.\r\n", ch);
      return;
     }

  if (on_horse(ch))
     {send_to_char("Вы замаскировались под статую Юрия Долгорукого.\r\n", ch);
      return;
     }

  if (timed_by_skill(ch, SKILL_CAMOUFLAGE))
     {send_to_char("У Вас пока не хватает фантазии. Побудьте немного самим собой.\r\n", ch);
      return;
     }

  if (IS_IMMORTAL(ch))
     affect_from_char(ch, SPELL_CAMOUFLAGE);

  if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
     {send_to_char("Вы уже маскируетесь.\r\n", ch);
      return;
     }

  send_to_char("Вы начали усиленно маскироваться.\r\n", ch);
  REMOVE_BIT(EXTRA_FLAGS(ch,EXTRA_FAILCAMOUFLAGE), EXTRA_FAILCAMOUFLAGE);
  percent = number(1,skill_info[SKILL_CAMOUFLAGE].max_percent);
  prob    = calculate_skill(ch, SKILL_CAMOUFLAGE, skill_info[SKILL_CAMOUFLAGE].max_percent, 0);

  af.type       = SPELL_CAMOUFLAGE;
  af.duration   = pc_duration(ch,0,GET_LEVEL(ch),6,0,2);
  af.modifier   = world[IN_ROOM(ch)].zone;
  af.location   = APPLY_NONE;
  af.battleflag = 0;
  if (percent > prob)
     af.bitvector = 0;
  else
     af.bitvector = AFF_CAMOUFLAGE;
  affect_to_char(ch, &af);
  if (!IS_IMMORTAL(ch))
     {timed.skill = SKILL_CAMOUFLAGE;
      timed.time  = 2;
      timed_to_char(ch, &timed);
     }
}

ACMD(do_hide)
{
  struct affected_type af;
  ubyte  prob, percent;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_HIDE))
     {send_to_char("Но Вы не знаете как.\r\n", ch);
      return;
     }

  if (on_horse(ch))
     {act("А куда Вы хотите спрятать $N3 ?", FALSE, ch, 0, get_horse(ch), TO_CHAR);
      return;
     }

  affect_from_char(ch, SPELL_HIDE);

  if (affected_by_spell(ch, SPELL_HIDE))
     {send_to_char("Вы уже пытаетесь спрятаться.\r\n", ch);
      return;
     }

  send_to_char("Хорошо, Вы попытаетесь спрятаться.\r\n", ch);
  REMOVE_BIT(EXTRA_FLAGS(ch,EXTRA_FAILHIDE), EXTRA_FAILHIDE);
  percent = number(1,skill_info[SKILL_HIDE].max_percent);
  prob    = calculate_skill(ch, SKILL_HIDE, skill_info[SKILL_HIDE].max_percent, 0);

  af.type       = SPELL_HIDE;
  af.duration   = pc_duration(ch,0,GET_LEVEL(ch),8,0,1);
  af.modifier   = 0;
  af.location   = APPLY_NONE;
  af.battleflag = 0;
  if (percent > prob)
     af.bitvector = 0;
  else
     af.bitvector = AFF_HIDE;
  affect_to_char(ch, &af);
}

void go_steal(struct char_data *ch, struct char_data *vict, char *obj_name)
{ int    percent, gold, eq_pos, ohoh = 0, success=0, prob;
  struct obj_data *obj;

  if (!vict)
     return;

  if (!WAITLESS(ch) && FIGHTING(vict))
     {act("$N слишком быстро перемещается.",FALSE,ch,0,vict,TO_CHAR);
      return;
     }

  if (!WAITLESS(ch) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_ARENA))
     {send_to_char("Воровство при поединке недопустимо.\r\n",ch);
      return;
     }

   /* 101% is a complete failure */
  percent = number(1,skill_info[SKILL_SNEAK].max_percent);

  if (WAITLESS(ch) ||
      (GET_POS(vict) <= POS_SLEEPING && !AFF_FLAGGED(vict,AFF_SLEEP)))
     success  = 1;		/* ALWAYS SUCCESS, unless heavy object. */

  if (!AWAKE(vict))	/* Easier to steal from sleeping people. */
     percent = MAX(percent - 50, 0);

  /* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
  if (IS_IMMORTAL(vict)                ||
      GET_GOD_FLAG(vict, GF_GODSLIKE)  ||
      GET_MOB_SPEC(vict) == shop_keeper)
     success = 0;		/* Failure */

  if (str_cmp(obj_name, "coins") &&
      str_cmp(obj_name, "gold")  &&
      str_cmp(obj_name, "кун")   &&
      str_cmp(obj_name, "деньги")
     )
     {if (!(obj = get_obj_in_list_vis(ch, obj_name, vict->carrying)))
         {for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
              if (GET_EQ(vict, eq_pos) &&
                  (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
                  CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos)))
                 {obj = GET_EQ(vict, eq_pos);
                  break;
                 }
          if (!obj)
             {act("А у н$S этого и нет - ха-ха-ха (2 раза)...", FALSE, ch, 0, vict, TO_CHAR);
              return;
             }
          else
             {/* It is equipment */
              if (!success)
                 {send_to_char("Украсть ? Из экипировки ? Щаз-з-з !\r\n", ch);
                  return;
                 }
              else
              if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
                 {send_to_char("Вы не сможете унести столько предметов.\r\n", ch);
                  return;
                 }
              else
              if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch))
                 {send_to_char("Вы не сможете унести такой вес.\r\n", ch);
                  return;
                 }
              else
                 {act("Вы раздели $N3 и взяли $o3.", FALSE, ch, obj, vict, TO_CHAR);
                  act("$n украл$g $o3 у $N1.", FALSE, ch, obj, vict, TO_NOTVICT);
                  obj_to_char(unequip_char(vict, eq_pos), ch);
                 }
             }
         }
      else {/* obj found in inventory */
            percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */
	    prob     = calculate_skill(ch, SKILL_STEAL, percent, vict);
	    if (CAN_SEE(vict,ch))
	       improove_skill(ch,SKILL_STEAL,0,vict);
	    if (!WAITLESS(ch) && AFF_FLAGGED(vict,AFF_SLEEP))
	       prob = 0;
            if (percent > prob && !success)
               {ohoh = TRUE;
                send_to_char("Атас.. Дружина на конях !\r\n", ch);
                act("$n пытал$u обокрасть Вас!", FALSE, ch, 0, vict, TO_VICT);
                act("$n пытал$u украсть нечто у $N1.", TRUE, ch, 0, vict, TO_NOTVICT);
               }
            else
               {/* Steal the item */
                if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))
                    {if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch))
                        {obj_from_char(obj);
                         obj_to_char(obj, ch);
                         act("Вы украли $o3 у $N1 !", FALSE, ch, obj, vict, TO_CHAR);
                        }
                    }
                 else
                    {send_to_char("Вы не можете столько нести.\r\n", ch);
		     return;
		    }
               }
           }
     }
  else
     {/* Steal some coins */
      prob     = calculate_skill(ch, SKILL_STEAL, percent, vict);
      if (CAN_SEE(vict,ch))
         improove_skill(ch,SKILL_STEAL,0,vict);
      if (!WAITLESS(ch) && AFF_FLAGGED(vict,AFF_SLEEP))
         prob = 0;
      if (percent > prob && !success)
         {ohoh = TRUE;
          send_to_char("Вы влипли.. Вас посодют.. А Вы не воруйте..\r\n", ch);
          act("Вы обнаружили руку $n1 в своем кармане.", FALSE, ch, 0, vict, TO_VICT);
          act("$n пытал$u спионерить деньги у $N1.", TRUE, ch, 0, vict, TO_NOTVICT);
         }
      else
         {/* Steal some gold coins */
	  if (!GET_GOLD(vict))
             {act("$E богат$A, как амбарная мышь :)",FALSE,ch,0,vict,TO_CHAR);
	      return;
	     }
	  else
	     {gold = (int) ((GET_GOLD(vict) * number(1, 10)) / 100);
              gold = MIN(GET_LEVEL(ch) * 100, gold);
              if (gold > 0)
                 {GET_GOLD(ch)   += gold;
                  GET_GOLD(vict) -= gold;
                  if (gold > 1)
                     {sprintf(buf, "УР-Р-Р-А!  Вы таки сперли %d %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));
                      send_to_char(buf, ch);
                     }
                  else
                     send_to_char("УРА-А-А ! Вы сперли :) 1 (одну) куну :(.\r\n", ch);
                 }
              else
                 send_to_char("Вы ничего не сумели украсть...\r\n", ch);
             }
         }
     }
  if (!WAITLESS(ch) && ohoh)
     WAIT_STATE(ch, 5 * PULSE_VIOLENCE);
  inc_pk_thiefs(ch,vict);
  if (ohoh &&
      IS_NPC(vict) &&
      AWAKE(vict) &&
      CAN_SEE(vict, ch) &&
      MAY_ATTACK(vict)
     )
     hit(vict, ch, TYPE_UNDEFINED, 1);
}


ACMD(do_steal)
{
  struct char_data *vict;
  char   vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STEAL))
     {send_to_char("Но Вы не знаете как.\r\n", ch);
      return;
     }

  if (!WAITLESS(ch) && on_horse(ch))
     {send_to_char("Верхом это сделать затруднительно.\r\n", ch);
      return;
     }

  two_arguments(argument, obj_name, vict_name);

  if (!(vict = get_char_vis(ch, vict_name, FIND_CHAR_ROOM)))
     {send_to_char("Украсть у кого ?\r\n", ch);
      return;
     }
  else
  if (vict == ch)
     {send_to_char("Попробуйте набрать \"бросить <n> кун\".\r\n", ch);
      return;
     }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) &&
      !(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
     {send_to_char("Здесь слишком мирно. Вам не хочется нарушать сию благодать...\r\n", ch);
      return;
     }

  go_steal(ch, vict, obj_name);
}



ACMD(do_skills)
{
  if (IS_NPC(ch))
     return;
  list_skills(ch);
}

ACMD(do_spells)
{
  if (IS_NPC(ch))
     return;
  list_spells(ch);
}



ACMD(do_visible)
{
  if (IS_IMMORTAL(ch))
     {perform_immort_vis(ch);
      return;
     }

  if (AFF_FLAGGED(ch, AFF_INVISIBLE) || AFF_FLAGGED(ch, AFF_CAMOUFLAGE))
     {appear(ch);
      send_to_char("Вы перестали быть невидимым.\r\n", ch);
     }
  else
     send_to_char("Вы и так видимы.\r\n", ch);
}

ACMD(do_courage)
{ struct obj_data      *obj;
  struct affected_type af[3];
  struct timed_type    timed;

  if (IS_NPC(ch))	/* Cannot use GET_COND() on mobs. */
     return;

  if (!GET_SKILL(ch, SKILL_COURAGE))
     {send_to_char("Вам это не по силам.\r\n", ch);
      return;
     }

  if (timed_by_skill(ch, SKILL_COURAGE))
     {send_to_char("Вы не можете слишком часто впадать в ярость.\r\n", ch);
      return;
     }

  timed.skill = SKILL_COURAGE;
  timed.time  = 6;
  timed_to_char(ch, &timed);
  /******************** Remove to hit()
  percent = number(1,skill_info[SKILL_COURAGE].max_percent+GET_REAL_MAX_HIT(ch)-GET_HIT(ch));
  prob    = train_skill(ch,SKILL_COURAGE,skill_info[SKILL_COURAGE].max_percent,0);
  if (percent > prob)
     {af[0].type      = SPELL_COURAGE;
      af[0].duration  = pc_duration(ch,3,0,0,0,0);
      af[0].modifier  = 0;
      af[0].location  = APPLY_DAMROLL;
      af[0].bitvector = AFF_NOFLEE;
      af[0].battleflag= 0;
      af[1].type      = SPELL_COURAGE;
      af[1].duration  = pc_duration(ch,3,0,0,0,0);
      af[1].modifier  = 0;
      af[1].location  = APPLY_HITROLL;
      af[1].bitvector = AFF_NOFLEE;
      af[1].battleflag= 0;
      af[2].type      = SPELL_COURAGE;
      af[2].duration  = pc_duration(ch,3,0,0,0,0);
      af[2].modifier  = 20;
      af[2].location  = APPLY_AC;
      af[2].bitvector = AFF_NOFLEE;
      af[2].battleflag= 0;
     }
  else
     {af[0].type      = SPELL_COURAGE;
      af[0].duration  = pc_duration(ch,3,0,0,0,0);
      af[0].modifier  = MAX(1, (prob+19) / 20);
      af[0].location  = APPLY_DAMROLL;
      af[0].bitvector = AFF_NOFLEE;
      af[0].battleflag= 0;
      af[1].type      = SPELL_COURAGE;
      af[1].duration  = pc_duration(ch,3,0,0,0,0);
      af[1].modifier  = MAX(1, (prob+9) / 10);
      af[1].location  = APPLY_HITROLL;
      af[1].bitvector = AFF_NOFLEE;
      af[1].battleflag= 0;
      af[2].type      = SPELL_COURAGE;
      af[2].duration  = pc_duration(ch,3,0,0,0,0);
      af[2].modifier  = 20;
      af[2].location  = APPLY_AC;
      af[2].bitvector = AFF_NOFLEE;
      af[2].battleflag= 0;
     }
   for (prob = 0; prob < 3; prob++)
       affect_join(ch,&af[prob],TRUE,FALSE,TRUE,FALSE);
   ************************************/
   af[0].type      = SPELL_COURAGE;
   af[0].duration  = pc_duration(ch,3,0,0,0,0);
   af[0].modifier  = 20;
   af[0].location  = APPLY_AC;
   af[0].bitvector = AFF_NOFLEE;
   af[0].battleflag= 0;
   affect_join(ch,&af[0],TRUE,FALSE,TRUE,FALSE);

   send_to_char("Вы пришли в ярость.\r\n",ch);
   if ((obj = GET_EQ(ch,WEAR_WIELD)) || (obj = GET_EQ(ch,WEAR_BOTHS)))
      strcpy(buf, "Глаза $n1 налились кровью и $e яростно сжал$g в руках $o3.");
   else
      strcpy(buf, "Глаза $n1 налились кровью.");
   act(buf, FALSE, ch, obj, 0, TO_ROOM);
}



ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
     send_to_char("И зачем попу баян ?\r\n", ch);
  else
  if (PLR_FLAGGED(ch, PLR_NOTITLE))
     send_to_char("Это слишком сложно для Вас !\r\n", ch);
  else
  if (strstr(argument, "(") || strstr(argument, ")"))
     send_to_char("Титул не может содержать символов ( или ).\r\n", ch);
  else
  if (strlen(argument) > MAX_TITLE_LENGTH)
     {sprintf(buf, "Извините, титул должен быть не длинее %d символов.\r\n",
	          MAX_TITLE_LENGTH);
      send_to_char(buf, ch);
     }
  else
     {set_title(ch, argument);
      sprintf(buf, "Хорошо, теперь Вас назовут \"%s\".\r\n",
              race_or_title(ch));
      send_to_char(buf, ch);
     }
}


int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (AFF_FLAGGED(vict, AFF_GROUP) ||
      AFF_FLAGGED(vict, AFF_CHARM) ||
      IS_HORSE(vict)
     )
     return (FALSE);

  SET_BIT(AFF_FLAGS(vict, AFF_GROUP), AFF_GROUP);
  if (ch != vict)
     {act("$N принят$A в члены Вашего кружка (тьфу-ты, группы :).", FALSE, ch, 0, vict, TO_CHAR);
      act("Вы приняты в группу $n1.", FALSE, ch, 0, vict, TO_VICT);
      act("$N принят$A в группу $n1.", FALSE, ch, 0, vict, TO_NOTVICT);
     }
  return (TRUE);
}

int low_charm(struct char_data *ch)
{struct affected_type *aff;
 for (aff = ch->affected; aff; aff = aff->next)
     if (aff->type == SPELL_CHARM && aff->duration <= 1)
        return (TRUE);
 return (FALSE);
}

void print_one_line(struct char_data *ch, struct char_data *k, int leader, int header)
{ int  ok, div;
  char *WORD_STATE[] =
       {"При смерти",
        "Оч.тяж.ран",
        "Оч.тяж.ран",
        " Тяж.ранен",
        " Тяж.ранен",
        "  Ранен   ",
        "  Ранен   ",
        "  Ранен   ",
        "Лег.ранен ",
        "Лег.ранен ",
        "Слег.ранен",
        " Невредим "};
  char *MOVE_STATE[] =
       {"Истощен",
        "Истощен",
        "О.устал",
        " Устал ",
        " Устал ",
        "Л.устал",
        "Л.устал",
        "Хорошо ",
        "Хорошо ",
        "Хорошо ",
        "Отдохн.",
        " Полон "};
  char *POS_STATE[] =
        {"Умер",
         "Истекает кровью",
         "При смерти",
         "Без сознания",
         "Спит",
         "Отдыхает",
         "Сидит",
         "Сражается",
         "Стоит"};
         
  if (IS_NPC(k))
     {if (!header)
         send_to_char("Персонаж       | Здоровье |Рядом| Доп | Положение\r\n",ch);
      sprintf(buf, "%s%-15s%s|",CCIBLU(ch, C_NRM), CAP(GET_NAME(k)), CCNRM(ch, C_NRM));
      sprintf(buf+strlen(buf), "%s%10s%s|",
              color_value(ch,GET_HIT(k),GET_REAL_MAX_HIT(k)),
              WORD_STATE[posi_value(GET_HIT(k),GET_REAL_MAX_HIT(k))+1],
              CCNRM(ch,C_NRM));
              
      ok = IN_ROOM(ch) == IN_ROOM(k);
      sprintf(buf+strlen(buf), "%s%5s%s|",
              ok ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM),
              ok ? " Да  " : " Нет ",
              CCNRM(ch, C_NRM));

      sprintf(buf+strlen(buf)," %s%s%s%s%s%s%s |",
              CCIYEL(ch, C_NRM),
              (AFF_FLAGGED(k,AFF_SINGLELIGHT) ||
               AFF_FLAGGED(k,AFF_HOLYLIGHT)   ||
               (GET_EQ(k,WEAR_LIGHT) && GET_OBJ_VAL(GET_EQ(k,WEAR_LIGHT),2))) ? "С" : " ",
              CCIBLU(ch, C_NRM),
              AFF_FLAGGED(k,AFF_FLY) ? "П" : " ",
              CCYEL(ch, C_NRM),
              low_charm(k) ? "Т" : " ",
              CCNRM(ch, C_NRM));
      sprintf(buf+strlen(buf),"%s",POS_STATE[(int) GET_POS(k)]);
      act(buf, FALSE, ch, 0, k, TO_CHAR);
     }
  else
     {if (!header)
         send_to_char("Персонаж       | Здоровье |Энергия|Рядом|Учить| Доп | Кто | Положение\r\n",ch);
     
      sprintf(buf, "%s%-15s%s|",CCIBLU(ch, C_NRM), CAP(GET_NAME(k)), CCNRM(ch, C_NRM));
      sprintf(buf+strlen(buf), "%s%10s%s|",
              color_value(ch,GET_HIT(k),GET_REAL_MAX_HIT(k)),
              WORD_STATE[posi_value(GET_HIT(k),GET_REAL_MAX_HIT(k))+1],
              CCNRM(ch,C_NRM));

      sprintf(buf+strlen(buf), "%s%7s%s|",
              color_value(ch,GET_MOVE(k),GET_REAL_MAX_MOVE(k)),
              MOVE_STATE[posi_value(GET_MOVE(k),GET_REAL_MAX_MOVE(k))+1],
              CCNRM(ch,C_NRM));

      ok = IN_ROOM(ch) == IN_ROOM(k);
      sprintf(buf+strlen(buf), "%s%5s%s|",
              ok ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM),
              ok ? " Да  " : " Нет ",
              CCNRM(ch, C_NRM));

      if ((ok = GET_MANA_NEED(k)))
         {div = mana_gain(k);
          if (div > 0)
             {ok = MAX(0, ok - GET_MANA_STORED(k));
              ok = ok / div + (ok % div > 0);
             }
          else
             ok = -1;
         }
      sprintf(buf+strlen(buf), "%s%5d%s|",
              ok ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM),
              ok,
              CCNRM(ch, C_NRM));

      sprintf(buf+strlen(buf)," %s%s%s%s%s%s%s |",
              CCIYEL(ch, C_NRM),
              (AFF_FLAGGED(k,AFF_SINGLELIGHT) ||
               AFF_FLAGGED(k,AFF_HOLYLIGHT)   ||
              (GET_EQ(k,WEAR_LIGHT) && GET_OBJ_VAL(GET_EQ(k,WEAR_LIGHT),2))) ? "С" : " ",
               CCIBLU(ch, C_NRM),
               AFF_FLAGGED(k,AFF_FLY) ? "П" : " ",
               CCYEL(ch, C_NRM),
               on_horse(k) ? "В" : " ",
               CCNRM(ch, C_NRM));
      sprintf(buf+strlen(buf),"%5s|",
              leader ? "Лидер" : ""
             );
      sprintf(buf+strlen(buf),"%s",POS_STATE[(int) GET_POS(k)]);
      act(buf, FALSE, ch, 0, k, TO_CHAR);
     }
}


void print_group(struct char_data *ch)
{ int    gfound = 0, cfound = 0;
  struct char_data *k;
  struct follow_type *f;

  if (AFF_FLAGGED(ch, AFF_GROUP))
     {send_to_char("Ваша группа состоит из:\r\n", ch);
      k = (ch->master ? ch->master : ch);
      if (AFF_FLAGGED(k, AFF_GROUP))
         print_one_line(ch,k,TRUE,gfound++);
      for (f = k->followers; f; f = f->next)
          {if (!AFF_FLAGGED(f->follower, AFF_GROUP))
              continue;
           print_one_line(ch,f->follower,FALSE,gfound++);
          }
     }
   for (f = ch->followers; f; f = f->next)
       {if (!AFF_FLAGGED(f->follower,AFF_CHARM))
           continue;
        if (!cfound)           
           send_to_char("Ваши последователи:\r\n",ch);
        print_one_line(ch,f->follower,FALSE,cfound++);
       }
   if (!gfound && !cfound)  
      send_to_char("Но Вы же не член (в лучшем смысле этого слова) группы!\r\n", ch);    
}



ACMD(do_group)
{ struct char_data *vict;
  struct follow_type *f;
  int found;

  one_argument(argument, buf);

  if (!*buf)
     {print_group(ch);
      return;
     }

  if (GET_POS(ch) < POS_RESTING)
     {send_to_char("Трудно управлять группой в таком состоянии.\r\n",ch);
      return;
     }

  if (ch->master)
     {act("Вы не можете управлять группой. Вы еще не ведущий.",
	       FALSE, ch, 0, 0, TO_CHAR);
      return;
     }

  if (!ch->followers)
     {send_to_char("За Вами никто не следует.\r\n",ch);
      return;
     }

  if (!str_cmp(buf, "all") ||
      !str_cmp(buf, "все"))
     {perform_group(ch, ch);
      for (found = 0, f = ch->followers; f; f = f->next)
          found += perform_group(ch, f->follower);
      if (!found)
         send_to_char("Все, кто за Вами следуют, уже включены в Вашу группу.\r\n", ch);
      return;
     }

  if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
     send_to_char(NOPERSON, ch);
  else
  if ((vict->master != ch) && (vict != ch))
     act("$N2 нужно следовать за Вами, чтобы стать членом Вашей группы.", FALSE, ch, 0, vict, TO_CHAR);
  else
     {if (!AFF_FLAGGED(vict, AFF_GROUP))
         {if (AFF_FLAGGED(vict,AFF_CHARM) ||
	      IS_HORSE(vict)
	     )
	    {send_to_char("Только равноправные персонажи могут быть включены в группу.\r\n",ch);
	     send_to_char("Только равноправные персонажи могут быть включены в группу.\r\n",vict);
	    };
	  perform_group(ch,ch);
          perform_group(ch,vict);
         }
      else
      if (ch != vict)
	     {act("$N исключен$A из состава Вашей группы.", FALSE, ch, 0, vict, TO_CHAR);
          act("Вы исключены из группы $n1!", FALSE, ch, 0, vict, TO_VICT);
          act("$N был$G исключен$A из группы $n1!", FALSE, ch, 0, vict, TO_NOTVICT);
          REMOVE_BIT(AFF_FLAGS(vict, AFF_GROUP), AFF_GROUP);
         }
     }
}

ACMD(do_ungroup)
{ struct follow_type *f, *next_fol;
  struct char_data *tch;

  one_argument(argument, buf);

  if (!*buf)
     {if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP)))
         {send_to_char("Вы же не лидер группы!\r\n", ch);
          return;
         }
      sprintf(buf2, "Вы исключены из группы %s.\r\n", GET_PAD(ch,1));
      for (f = ch->followers; f; f = next_fol)
          {next_fol = f->next;
           if (AFF_FLAGGED(f->follower, AFF_GROUP))
              {REMOVE_BIT(AFF_FLAGS(f->follower, AFF_GROUP), AFF_GROUP);
	           send_to_char(buf2, f->follower);
               if (!AFF_FLAGGED(f->follower, AFF_CHARM) &&
                   !(IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_HORSE)))
	              stop_follower(f->follower, SF_EMPTY);
              }
          }
      REMOVE_BIT(AFF_FLAGS(ch, AFF_GROUP), AFF_GROUP);
      send_to_char("Вы распустили группу.\r\n", ch);
      return;
     }
  if (!(tch = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
     {send_to_char("Нет такого игрока в Вашей группе!\r\n", ch);
      return;
     }
  if (tch->master != ch)
     {send_to_char("А он и не думал за Вами следовать!\r\n", ch);
      return;
     }

  if (!AFF_FLAGGED(tch, AFF_GROUP))
     {send_to_char("Этот игрок не входит в состав Вашей группы.\r\n", ch);
      return;
     }

  REMOVE_BIT(AFF_FLAGS(tch, AFF_GROUP), AFF_GROUP);

  act("$N более не член Вашей группы.", FALSE, ch, 0, tch, TO_CHAR);
  act("Вы исключены из группы $n1 !", FALSE, ch, 0, tch, TO_VICT);
  act("$N был$G изгнан$A из группы $n1 !", FALSE, ch, 0, tch, TO_NOTVICT);

  if (!AFF_FLAGGED(tch, AFF_CHARM) &&
      !IS_HORSE(tch)
     )
     stop_follower(tch, SF_EMPTY);
}

ACMD(do_report)
{ struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch,AFF_GROUP) &&
      !AFF_FLAGGED(ch,AFF_CHARM)
     )
     {send_to_char("И перед кем Вы отчитываетесь ?\r\n", ch);
      return;
     }
  sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V\r\n",
          GET_NAME(ch), GET_CH_SUF_1(ch),
          GET_HIT(ch), GET_REAL_MAX_HIT(ch),
          GET_MOVE(ch), GET_REAL_MAX_MOVE(ch));
  CAP(buf);
  k = (ch->master ? ch->master : ch);
  for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
         send_to_char(buf, f->follower);
  if (k != ch)
     send_to_char(buf, k);
  send_to_char("Вы доложили о состоянии всем членам Вашей группы.\r\n", ch);
}



ACMD(do_split)
{
  int amount, num, share, rest;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
     return;

  one_argument(argument, buf);

  if (is_number(buf))
     {amount = atoi(buf);
      if (amount <= 0)
         {send_to_char("И как Вы это планируете сделать ?\r\n", ch);
          return;
         }
      if (amount > GET_GOLD(ch))
         {send_to_char("И где бы взять Вам столько денег ?.\r\n", ch);
          return;
         }

      k = (ch->master ? ch->master : ch);

      if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
         num = 1;
      else
         num = 0;

      for (f = k->followers; f; f = f->next)
          if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	      !IS_NPC(f->follower)                &&
	      IN_ROOM(f->follower) == IN_ROOM(ch)
	     )
	     num++;

      if (num && AFF_FLAGGED(ch, AFF_GROUP))
         {share = amount / num;
          rest  = amount % num;
         }
      else
         {send_to_char("С кем Вы хотите разделить это добро ?\r\n", ch);
          return;
         }

      GET_GOLD(ch) -= share * (num - 1);

      sprintf(buf, "%s разделил%s %d %s; Вам досталось %d.\r\n",
              GET_NAME(ch), GET_CH_SUF_1(ch),
              amount, desc_count(amount,WHAT_MONEYu), share);
      if (AFF_FLAGGED(k, AFF_GROUP) &&
          IN_ROOM(k) == IN_ROOM(ch) &&
          !IS_NPC(k)                &&
	  k != ch
	 )
         { GET_GOLD(k) += share;
           send_to_char(buf, k);
         }
      for (f = k->followers; f; f = f->next)
          {if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	       !IS_NPC(f->follower)                &&
	       IN_ROOM(f->follower) == IN_ROOM(ch) &&
	       f->follower != ch)
	      {GET_GOLD(f->follower) += share;
	       send_to_char(buf, f->follower);
              }
           }
      sprintf(buf, "Вы разделили %d %s на %d  -  по %d каждому.\r\n",
	          amount, desc_count(amount,WHAT_MONEYu), num, share);
      if (rest)
         sprintf(buf + strlen(buf), "Как истинный еврей Вы оставили %d %s (которые не смогли разделить нацело) себе.\r\n",
                 rest, desc_count(rest, WHAT_MONEYu));
      send_to_char(buf, ch);
     }
  else
     {send_to_char("Сколько и чего Вы хотите разделить ?\r\n", ch);
      return;
     }
}



ACMD(do_use)
{
  struct obj_data *mag_item;
  int    do_hold = 0;

  half_chop(argument, arg, buf);
  if (!*arg)
     {sprintf(buf2, "Что Вы хотите %s?\r\n", CMD_NAME);
      send_to_char(buf2, ch);
      return;
     }
  mag_item = GET_EQ(ch, WEAR_HOLD);

  if (!mag_item || !isname(arg, mag_item->name))
     {switch (subcmd)
         {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->carrying)))
         {sprintf(buf2, "Акститесь, нет у Вас %s.\r\n", arg);
	      send_to_char(buf2, ch);
	      return;
         };
      break;
    case SCMD_USE:
      sprintf(buf2, "Возьмите в руку '%s' перед применением !\r\n", arg);
      send_to_char(buf2, ch);
      return;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      return;
         }
     }
  switch (subcmd)
     {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION)
       {send_to_char("Осушить Вы можете только напиток (ну, Богам еще пЫво по вкусу ;)\r\n", ch);
        return;
       }
    do_hold = 1;
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL)
       {send_to_char("Пригодны для зачитывания только свитки.\r\n", ch);
        return;
       }
    do_hold = 1;
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	    (GET_OBJ_TYPE(mag_item) != ITEM_STAFF))
	   {send_to_char("Применять можно только магические предметы !\r\n", ch);
        return;
       }
    break;
     }
  if (do_hold && GET_EQ(ch,WEAR_HOLD) != mag_item)
     {do_hold = GET_EQ(ch, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_HOLD;
      if (GET_EQ(ch,do_hold))
         {act("Вы прекратили использовать $o3.",FALSE,ch,GET_EQ(ch,do_hold),0,TO_CHAR);
          act("$n прекратил$g использовать $o3.",FALSE,ch,GET_EQ(ch,do_hold),0,TO_ROOM);
          obj_to_char(unequip_char(ch,do_hold), ch);
         }
      if (GET_EQ(ch,WEAR_HOLD))
         obj_to_char(unequip_char(ch,WEAR_HOLD), ch);
      act("Вы взяли $o3 в левую руку.", FALSE, ch, mag_item, 0, TO_CHAR);
      act("$n взял$g $o в левую руку.", FALSE, ch, mag_item, 0, TO_ROOM);
      obj_from_char(mag_item);
      equip_char(ch,mag_item,WEAR_HOLD);
     }
  mag_objectmagic(ch, mag_item, buf);
}



ACMD(do_wimpy)
{
  int wimp_lev;

  /* 'wimp_level' is a player_special. -gg 2/25/98 */
  if (IS_NPC(ch))
     return;

  one_argument(argument, arg);

  if (!*arg)
     {if (GET_WIMP_LEV(ch))
         {sprintf(buf, "Вы попытаетесь бежать при %d ХП.\r\n",
	                   GET_WIMP_LEV(ch));
          send_to_char(buf, ch);
          return;
         }
      else
         {send_to_char("Вы будете драться, драться и драться (пока не помрете, ессно...)\r\n", ch);
          return;
         }
     }
  if (isdigit(*arg))
     {if ((wimp_lev = atoi(arg)) != 0)
         {if (wimp_lev < 0)
	         send_to_char("Да, перегрев похоже. С такими хитами Вы и так помрете :)\r\n", ch);
          else
          if (wimp_lev > GET_REAL_MAX_HIT(ch))
	         send_to_char("Осталось только дожить до такого количества ХП.\r\n", ch);
          else
          if (wimp_lev > (GET_REAL_MAX_HIT(ch) / 2))
	         send_to_char("Размечтались. Сбечь то можна, но ! Не менее половины максимальных ХП.\r\n", ch);
          else
             {sprintf(buf, "Ладушки. Вы сбегите(или сбежите) по достижению %d ХП.\r\n",
		              wimp_lev);
	          send_to_char(buf, ch);
	          GET_WIMP_LEV(ch) = wimp_lev;
             }
         }
      else
         {send_to_char("Вы будете сражаться до конца (скорее всего своего ;).\r\n", ch);
          GET_WIMP_LEV(ch) = 0;
         }
     }
  else
     send_to_char("Уточните, при достижении какого количества ХП Вы планируете сбежать (0 - драться до смерти)\r\n", ch);
}


ACMD(do_display)
{
  size_t i;

  if (IS_NPC(ch))
     {send_to_char("И зачем это монстру ? Не юродствуйте.\r\n", ch);
      return;
     }
  skip_spaces(&argument);

  if (!*argument)
     {send_to_char("Формат: статус { { Ж | Э | З | В | Д | У | О } | все | нет }\r\n", ch);
      return;
     }
  if (!str_cmp(argument, "on") || !str_cmp(argument, "all") ||
      !str_cmp(argument, "вкл") || !str_cmp(argument, "все"))
    { SET_BIT(PRF_FLAGS(ch, PRF_DISPHP),   PRF_DISPHP);
      SET_BIT(PRF_FLAGS(ch, PRF_DISPMANA), PRF_DISPMANA);
      SET_BIT(PRF_FLAGS(ch, PRF_DISPMOVE), PRF_DISPMOVE);
      SET_BIT(PRF_FLAGS(ch, PRF_DISPEXITS),   PRF_DISPEXITS);
      SET_BIT(PRF_FLAGS(ch, PRF_DISPGOLD), PRF_DISPGOLD);
      SET_BIT(PRF_FLAGS(ch, PRF_DISPLEVEL),   PRF_DISPLEVEL);
      SET_BIT(PRF_FLAGS(ch, PRF_DISPEXP), PRF_DISPEXP);
     }
  else
  if (!str_cmp(argument, "off") || !str_cmp(argument, "none") ||
      !str_cmp(argument, "выкл") || !str_cmp(argument, "нет"))
     {REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPHP),   PRF_DISPHP);
      REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPMANA), PRF_DISPMANA);
      REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPMOVE), PRF_DISPMOVE);
      REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPEXITS),   PRF_DISPEXITS);
      REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPGOLD), PRF_DISPGOLD);
      REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPLEVEL),   PRF_DISPLEVEL);
      REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPEXP), PRF_DISPEXP);
     }
  else
     {REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPHP),   PRF_DISPHP);
      REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPMANA), PRF_DISPMANA);
      REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPMOVE), PRF_DISPMOVE);
      REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPEXITS),   PRF_DISPEXITS);
      REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPGOLD), PRF_DISPGOLD);
      REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPLEVEL),   PRF_DISPLEVEL);
      REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPEXP), PRF_DISPEXP);

      for (i = 0; i < strlen(argument); i++)
          {switch (LOWER(argument[i]))
              {
      case 'h':
      case 'ж':
	SET_BIT(PRF_FLAGS(ch, PRF_DISPHP), PRF_DISPHP);
	break;
      case 'w':
      case 'з':
	SET_BIT(PRF_FLAGS(ch, PRF_DISPMANA), PRF_DISPMANA);
	break;
      case 'm':
      case 'э':
	SET_BIT(PRF_FLAGS(ch, PRF_DISPMANA), PRF_DISPMOVE);
	break;
      case 'e':
      case 'в':
	SET_BIT(PRF_FLAGS(ch, PRF_DISPEXITS), PRF_DISPEXITS);
	break;
      case 'g':
      case 'д':
	SET_BIT(PRF_FLAGS(ch, PRF_DISPGOLD), PRF_DISPGOLD);
	break;
      case 'l':
      case 'у':
	SET_BIT(PRF_FLAGS(ch, PRF_DISPLEVEL), PRF_DISPLEVEL);
	break;
      case 'x':
      case 'о':
	SET_BIT(PRF_FLAGS(ch, PRF_DISPEXP), PRF_DISPEXP);
	break;
      default:
    send_to_char("Формат: статус { { Ж | Э | З | В | Д | У | О } | все | нет }\r\n", ch);
	return;
              }
          }
     }

  send_to_char(OK, ch);
}



ACMD(do_gen_write)
{
  FILE *fl;
  char *tmp, buf[MAX_STRING_LENGTH];
  const char *filename;
  struct stat fbuf;
  time_t ct;

  switch (subcmd)
     {
  case SCMD_BUG:
    filename = BUG_FILE;
    break;
  case SCMD_TYPO:
    filename = TYPO_FILE;
    break;
  case SCMD_IDEA:
    filename = IDEA_FILE;
    break;
  default:
    return;
     }

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (IS_NPC(ch))
     {send_to_char("То, что вас посетило, оставьте при себе. Плиз-з-з.\r\n", ch);
      return;
     }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
     {send_to_char("Это какая то ошибка...\r\n", ch);
      return;
     }
  sprintf(buf, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);
  mudlog(buf, CMP, LVL_IMMORT, FALSE);

  if (stat(filename, &fbuf) < 0)
     {perror("SYSERR: Can't stat() file");
      return;
     }
  if (fbuf.st_size >= max_filesize)
     {send_to_char("Да, набросали всего столько, что файл переполнен. Пишите на microkod@kursknet.ru.\r\n", ch);
      return;
     }
  if (!(fl = fopen(filename, "a")))
     {perror("SYSERR: do_gen_write");
      send_to_char("Не смог открыть файл. Просто не смог :(.\r\n", ch);
      return;
     }
  fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
	      GET_ROOM_VNUM(IN_ROOM(ch)), argument);
  fclose(fl);
  send_to_char("Записали. Заранее благодарны.\r\n"
               "                        Боги.\r\n", ch);
}



#define TOG_OFF 0
#define TOG_ON  1
const char *gen_tog_type[] =
{"автовыходы",   "autoexits",
 "краткий",      "brief",
 "сжатый",       "compact",
 "задание",      "quest",
 "цвет",         "color",
 "аукцион",      "noauction",
 "болтать",      "nogossip",
 "поздравления", "nogratz",
 "нападения",    "nohassle",
 "повтор",       "norepeat",
 "обращения",    "notell",
 "призыв",       "nosummon",
 "кричать",     "noshout",
 "частный",      "nowiz",
 "флаги",        "roomflags",
 "замедление",   "slowns",
 "выслеживание", "trackthru",
 "супервидение", "holylight",
 "кодер",         "coder",
 "автозаучивание","automem",
 "\n"
};

struct gen_tog_param_type
{ int level;
  int subcmd;
} gen_tog_param[] =
{ {0,   SCMD_AUTOEXIT},
  {0,   SCMD_BRIEF},
  {0,   SCMD_COMPACT},
  {LVL_GRGOD, SCMD_QUEST},
  {0,   SCMD_COLOR},
  {0,   SCMD_NOAUCTION},
  {0,   SCMD_NOGOSSIP},
  {0,   SCMD_NOGRATZ},
  {LVL_GRGOD, SCMD_NOHASSLE},
  {0,   SCMD_NOREPEAT},
  {0,   SCMD_DEAF},
  {LVL_GRGOD, SCMD_NOSUMMON},
  {0,   SCMD_NOTELL},
  {LVL_GOD,   SCMD_NOWIZ},
  {LVL_GRGOD, SCMD_ROOMFLAGS},
  {LVL_IMPL,  SCMD_SLOWNS},
  {LVL_GOD,   SCMD_TRACK},
  {LVL_GOD,   SCMD_HOLYLIGHT},
  {LVL_GOD,   SCMD_CODERINFO},
  {0,         SCMD_AUTOMEM}
};

ACMD(do_mode)
{ int i, showhelp = FALSE;
  if (IS_NPC(ch))
     return;

  argument = one_argument(argument,arg);
  if (!*arg)
     {do_toggle(ch, argument, 0, 0);
      return;
     }
   else
   if (*arg == '?')
      showhelp = TRUE;
   else
   if ((i = search_block(arg, gen_tog_type, FALSE)) < 0)
      showhelp = TRUE;
   else
   if (GET_LEVEL(ch) < gen_tog_param[i >> 1].level && !GET_COMMSTATE(ch))
      {send_to_char("Эта команда Вам недоступна.\r\n",ch);
       showhelp = TRUE;
      }
   else
      do_gen_tog(ch, argument, 0, gen_tog_param[i >> 1].subcmd);

   if (showhelp)
      {strcpy(buf,"Вы можете установить следующее.\r\n");
       for (i = 0; *gen_tog_type[i<<1] != '\n'; i++)
           if (GET_LEVEL(ch) >= gen_tog_param[i].level || GET_COMMSTATE(ch))
              sprintf(buf+strlen(buf),"%-20s(%s)\r\n",gen_tog_type[i<<1],gen_tog_type[(i<<1)+1]);
       strcat(buf,"\r\n");
       send_to_char(buf,ch);
      }
}

ACMD(do_gen_tog)
{ long result;

  const char *tog_messages[][2] = {
    {"Вы защищены от призыва.\r\n",
     "Вы можете быть призваны.\r\n"},
    {"Nohassle disabled.\r\n",
     "Nohassle enabled.\r\n"},
    {"Краткий режим выключен.\r\n",
     "Краткий режим включен.\r\n"},
    {"Сжатый режим выключен.\r\n",
     "Сжатый режим включен.\r\n"},
    {"К Вам можно обратиться.\r\n",
     "Вы глухи к обращениями.\r\n"},
    {"Вам будут выводиться сообщения аукциона.\r\n",
     "Вы отключены от участия в аукционе.\r\n"},
    {"Вы слышите все крики.\r\n",
     "Вы глухи к крикам.\r\n"},
    {"Вы слышите всю болтовню.\r\n",
     "Вы глухи к болтовне.\r\n"},
    {"Вы слышите все поздравления.\r\n",
     "Вы глухи к поздравлениям.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
     "You are now deaf to the Wiz-channel.\r\n"},
    {"Вы больше не выполняете задания.\r\n",
     "Вы выполняете задание!\r\n"},
    {"Вы больше не будете видеть флаги комнат.\r\n",
     "Вы способны различать флаги комнат.\r\n"},
    {"Ваши сообщения будут дублироваться.\r\n",
     "Ваши сообщения не будут дублироваться Вам.\r\n"},
    {"HolyLight mode off.\r\n",
     "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
     "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Показ выходов автоматически выключен.\r\n",
     "Показ выходов автоматически включен.\r\n"},
    {"Вы не можете более выследить сквозь двери.\r\n",
     "Вы способны выслеживать сквозь двери.\r\n"},
    {"\r\n",
     "\r\n"},
    {"Режим показа дополнительной информации выключен.\r\n",
     "Режим показа дополнительной информации включен.\r\n"},
    {"Автозаучивание заклинаний выключено.\r\n",
     "Автозаучивание заклинаний включено.\r\n"}
  };


  if (IS_NPC(ch))
     return;

  switch (subcmd)
     {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_DEAF:
    result = PRF_TOG_CHK(ch, PRF_DEAF);
    break;
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_ROOMFLAGS:
    result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_SLOWNS:
    result = (nameserver_is_slow = !nameserver_is_slow);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_TRACK:
    result = (track_through_doors = !track_through_doors);
    break;
  case SCMD_CODERINFO:
    result = PRF_TOG_CHK(ch, PRF_CODERINFO);
    break;
  case SCMD_AUTOMEM:
    result = PRF_TOG_CHK(ch, PRF_AUTOMEM);
    break;
  case SCMD_COLOR:
    do_color(ch,argument,0,0);
    return;
    break;
  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    return;
     }

  if (result)
     send_to_char(tog_messages[subcmd][TOG_ON], ch);
  else
     send_to_char(tog_messages[subcmd][TOG_OFF], ch);

  return;
}

ACMD(do_pray)
{ int    metter = -1, i;
  struct obj_data *obj = NULL;
  struct affected_type af;
  struct timed_type    timed;


  if (IS_NPC(ch))
     return;

  if (!IS_IMMORTAL(ch) &&
      ((subcmd == SCMD_DONATE && GET_RELIGION(ch) != RELIGION_POLY) ||
       (subcmd == SCMD_PRAY   && GET_RELIGION(ch) != RELIGION_MONO))
     )
     {send_to_char("Не кощунствуйте!\r\n",ch);
      return;
     }

  if (subcmd == SCMD_DONATE && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_POLY))
     {send_to_char("Найдите подходящее место для Вашей жертвы.\r\n",ch);
      return;
     }
  if (subcmd == SCMD_PRAY && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_MONO))
     {send_to_char("Это место не обладает необходимой святосью.\r\n",ch);
      return;
     }

  half_chop(argument, arg, buf);

  if (!*arg || (metter = search_block(arg, pray_whom, FALSE)) < 0)
     {if (subcmd == SCMD_DONATE)
         {send_to_char("Вы можете принести жертву :\r\n",ch);
          for (metter = 0; *(pray_metter[metter]) != '\n'; metter++)
              if (*(pray_metter[metter]) == '-')
                 {send_to_char(pray_metter[metter],ch);
                  send_to_char("\r\n",ch);
                 }
          send_to_char("Укажите, кому и что Вы хотите жертвовать.\r\n",ch);
         }
      else
      if (subcmd == SCMD_PRAY)
         {send_to_char("Вы можете вознести молитву :\r\n",ch);
          for (metter = 0; *(pray_metter[metter]) != '\n'; metter++)
              if (*(pray_metter[metter]) == '*')
                 {send_to_char(pray_metter[metter],ch);
                  send_to_char("\r\n",ch);
                 }
          send_to_char("Укажите, кому Вы хотите вознести молитву.\r\n",ch);
         }
      return;
     }

  if (subcmd == SCMD_DONATE && *(pray_metter[metter]) != '-')
     {send_to_char("Приносите жертвы только своим Богам.\r\n",ch);
      return;
     }

  if (subcmd == SCMD_PRAY && *(pray_metter[metter]) != '*')
     {send_to_char("Возносите молитвы только своим Богам.\r\n",ch);
      return;
     }

  if (subcmd == SCMD_DONATE)
     {if (!*buf || !(obj = get_obj_in_list_vis(ch,buf,ch->carrying)))
         {send_to_char("Вы должны пожертвовать что-то стоящее.\r\n",ch);
          return;
         }
      if (GET_OBJ_TYPE(obj) != ITEM_FOOD && GET_OBJ_TYPE(obj) != ITEM_TREASURE)
         {send_to_char("Богам неугодна эта жертва.\r\n",ch);
          return;
         }
     }
  else
  if (subcmd == SCMD_PRAY)
     {if (GET_GOLD(ch) < 10)
         {send_to_char("У Вас не хватит денег на свечку.\r\n",ch);
          return;
         }
     }
  else
     return;

  if (!IS_IMMORTAL(ch) &&
      (timed_by_skill(ch, SKILL_RELIGION) || affected_by_spell(ch,SPELL_RELIGION)))
     {send_to_char("Вы не можете так часто взывать к Богам.\r\n",ch);
      return;
     }

  timed.skill = SKILL_RELIGION;
  timed.time  = 12;
  timed_to_char(ch, &timed);

  for (i = 0; pray_affect[i].metter >= 0; i++)
      if (pray_affect[i].metter == metter)
         {af.type       = SPELL_RELIGION;
          af.duration   = pc_duration(ch,12,0,0,0,0);
          af.modifier   = pray_affect[i].modifier;
          af.location   = pray_affect[i].location;
          af.bitvector  = pray_affect[i].bitvector;
          af.battleflag = pray_affect[i].battleflag;
          affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
         }

  if (subcmd == SCMD_PRAY)
     {sprintf(buf,"$n затеплил$g свечку и вознес$q молитву %s.",pray_whom[metter]);
      act(buf,FALSE,ch,0,0,TO_ROOM);
      sprintf(buf,"Вы затеплили свечку и вознесли молитву %s.",pray_whom[metter]);
      act(buf,FALSE,ch,0,0,TO_CHAR);
      GET_GOLD(ch) -= 10;
     }
  else
  if (subcmd == SCMD_DONATE && obj)
     {sprintf(buf,"$n принес$q $o3 в жертву %s.",pray_whom[metter]);
      act(buf,FALSE,ch,obj,0,TO_ROOM);
      sprintf(buf,"Вы принесли $o3 в жертву %s.",pray_whom[metter]);
      act(buf,FALSE,ch,obj,0,TO_CHAR);
      obj_from_char(obj);
      extract_obj(obj);
     }
}