/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
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
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "dg_scripts.h"

/* extern variables */
extern int level_can_shout;
extern int holler_move_cost;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct auction_lot_type auction_lots[];

/* local functions */
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
int is_tell_ok(struct char_data *ch, struct char_data *vict);
ACMD(do_say);
ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
ACMD(do_spec_comm);
ACMD(do_write);
ACMD(do_page);
ACMD(do_gen_comm);
ACMD(do_qcomm);

#define SIELENCE ("Вы немы, как рыба об лед.\r\n")


ACMD(do_say)
{
  skip_spaces(&argument);

  if (AFF_FLAGGED(ch, AFF_SIELENCE))
     {send_to_char(SIELENCE, ch);
      return;
     }

  if (!*argument)
     send_to_char("Вы задумались :\"Чего бы такого сказать ?\"\r\n", ch);
  else
     {sprintf(buf, "$n сказал$g : '%s'", argument);
      act(buf, FALSE, ch, 0, 0, TO_ROOM | DG_NO_TRIG | CHECK_DEAF);
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(OK, ch);
      else
         {delete_doubledollar(argument);
          sprintf(buf, "Вы сказали : '%s'\r\n", argument);
          send_to_char(buf, ch);
         }
      speech_mtrigger(ch, argument);
      speech_wtrigger(ch, argument);
     }
}


ACMD(do_gsay)
{
  struct char_data *k;
  struct follow_type *f;

  if (AFF_FLAGGED(ch, AFF_SIELENCE))
     {send_to_char(SIELENCE, ch);
      return;
     }

  skip_spaces(&argument);

  if (!AFF_FLAGGED(ch, AFF_GROUP))
     {send_to_char("Вы не являетесь членом группы !\r\n", ch);
      return;
     }
  if (!*argument)
     send_to_char("О чем Вы хотите сообщить своей группе ?\r\n", ch);
  else
     {if (ch->master)
         k = ch->master;
      else
         k = ch;

      sprintf(buf, "$n сообщил$g группе : '%s'", argument);

      if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch))
         act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
      for (f = k->followers; f; f = f->next)
          if (AFF_FLAGGED(f->follower, AFF_GROUP) && (f->follower != ch))
	         act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);

      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(OK, ch);
      else
         {sprintf(buf, "Вы сообщили группе : '%s'\r\n", argument);
          send_to_char(buf, ch);
         }
     }
}


void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
  send_to_char(CCICYN(vict, C_NRM), vict);
  sprintf     (buf, "$n сказал$g Вам : '%s'", arg);
  act         (buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
  send_to_char(CCNRM(vict, C_NRM), vict);

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
     send_to_char(OK, ch);
  else
     {send_to_char(CCICYN(ch, C_CMP), ch);
      sprintf(buf, "Вы сказали $N2 : '%s'", arg);
      act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
      send_to_char(CCNRM(ch, C_CMP), ch);
     }

  if (!IS_NPC(vict) && !IS_NPC(ch))
     GET_LAST_TELL(vict) = GET_IDNUM(ch);
}

int is_tell_ok(struct char_data *ch, struct char_data *vict)
{ if (ch == vict)
     {send_to_char("Вы начали потихоньку разговаривать с самим собой.\r\n", ch);
      return (FALSE);
     }
  else
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
     {send_to_char("Вам запрещено обращаться к другим игрокам.\r\n", ch);
      return (FALSE);
     }
  else
  if (!IS_NPC(vict) && !vict->desc)        /* linkless */
     {act("$N потерял$G связь в этот момент.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
      return (FALSE);
     }
  else
  if (PLR_FLAGGED(vict, PLR_WRITING))
     {act("$N пишет сообщение - повторите попозже.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
      return (FALSE);
     }

  if (IS_GOD(ch))
     return (TRUE);

  if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
     send_to_char("Стены заглушили Ваши слова.\r\n", ch);
  else
  if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_DEAF)) ||
      ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF)
     )
     act("$N не сможет Вас услышать.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else
  if (GET_POS(vict) < POS_RESTING)
     act("$N Вас не услышит.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else
    return (TRUE);

  return (FALSE);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
  struct char_data *vict = NULL;

  if (AFF_FLAGGED(ch, AFF_SIELENCE))
     {send_to_char(SIELENCE, ch);
      return;
     }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
     {send_to_char("Что и кому Вы хотите сказать ?\r\n", ch);
     }
  else
  if (!IS_IMMORTAL(ch) && !(vict = get_player_vis(ch, buf, FIND_CHAR_WORLD)))
     {send_to_char(NOPERSON, ch);
     }
  else
  if (IS_IMMORTAL(ch) && !(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
     {send_to_char(NOPERSON, ch);
     }
  else
  if (is_tell_ok(ch, vict))
     {perform_tell(ch, vict, buf2);
     }
}


ACMD(do_reply)
{
  struct char_data *tch = character_list;

  if (IS_NPC(ch))
     return;

  if (AFF_FLAGGED(ch, AFF_SIELENCE))
     {send_to_char(SIELENCE, ch);
      return;
     }

  skip_spaces(&argument);

  if (GET_LAST_TELL(ch) == NOBODY)
     send_to_char("Вам некому ответить !\r\n", ch);
  else
  if (!*argument)
     send_to_char("Что Вы собираетесь ответить ?\r\n", ch);
  else
     {/*
      * Make sure the person you're replying to is still playing by searching
      * for them.  Note, now last tell is stored as player IDnum instead of
      * a pointer, which is much better because it's safer, plus will still
      * work if someone logs out and back in again.
      */
				
     /*
      * XXX: A descriptor list based search would be faster although
      *      we could not find link dead people.  Not that they can
      *      hear tells anyway. :) -gg 2/24/98
      */
      while (tch != NULL && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)))
            tch = tch->next;

      if (tch == NULL)
         send_to_char("Этого игрока уже нет в игре.",ch);
      else
      if (is_tell_ok(ch, tch))
         perform_tell(ch, tch, argument);
     }
}


ACMD(do_spec_comm)
{
  struct char_data *vict;
  const char *action_sing, *action_plur, *action_others;
  char  *vict1, *vict2, vict3[MAX_INPUT_LENGTH];

  if (AFF_FLAGGED(ch,AFF_SIELENCE))
     {send_to_char(SIELENCE, ch);
      return;
     }

  if (subcmd == SCMD_WHISPER)
     {action_sing = "шепнуть";
      vict1       = "кому";
      vict2       = "Вам";
      action_plur = "прошептал";
      action_others = "$n что-то прошептал$g $N2.";
     }
  else
     {action_sing = "спросить";
      vict1       = "у кого";
      vict2       = "у Вас";
      action_plur = "спросил";
      action_others = "$n задал$g $N2 вопрос.";
     }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
     {sprintf(buf, "Что Вы хотите %s.. и %s ?\r\n", action_sing, vict1);
      send_to_char(buf, ch);
     }
  else
  if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
     send_to_char(NOPERSON, ch);
  else
  if (vict == ch)
     send_to_char("От Ваших уст до ушей - всего одна ладонь...\r\n", ch);
  else
     {if (subcmd == SCMD_WHISPER)
         sprintf(vict3,"%s",GET_PAD(vict,2));
      else
         sprintf(vict3,"у %s",GET_PAD(vict,1));

      sprintf(buf, "$n %s$g %s : '%s'", action_plur, vict2, buf2);
      act(buf, FALSE, ch, 0, vict, TO_VICT);

      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(OK, ch);
      else
         {sprintf(buf, "Вы %sи %s : '%s'\r\n", action_plur, vict3, buf2);
          send_to_char(buf, ch);
         }

      act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
     }
}



#define MAX_NOTE_LENGTH 1000	/* arbitrary */

ACMD(do_write)
{
  struct obj_data *paper, *pen = NULL;
  char *papername, *penname;

  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
     return;

  if (!*papername)
     {/* nothing was delivered */
      send_to_char("Написать ?  Чем ?  И на чем ?\r\n", ch);
      return;
     }
  if (*penname)
     {/* there were two arguments */
      if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
         {sprintf(buf, "У Вас нет %s.\r\n", papername);
          send_to_char(buf, ch);
          return;
         }
      if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying)))
         {sprintf(buf, "У Вас нет %s.\r\n", penname);
          send_to_char(buf, ch);
          return;
         }
     }
  else
     {/* there was one arg.. let's see what we can find */
      if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
         {sprintf(buf, "Вы не видите %s в инвентаре.\r\n", papername);
          send_to_char(buf, ch);
          return;
         }
      if (GET_OBJ_TYPE(paper) == ITEM_PEN)
         {/* oops, a pen.. */
          pen = paper;
          paper = NULL;
         }
      else
      if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
         {send_to_char("Вы не можете на ЭТОМ писать.\r\n", ch);
          return;
         }
       /* One object was found.. now for the other one. */
       if (!GET_EQ(ch, WEAR_HOLD))
          {sprintf(buf, "Вы нечем писать !\r\n");
           send_to_char(buf, ch);
           return;
           }
        if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD)))
           {send_to_char("Вы держите что-то невидимое!  Жаль, но писать этим трудно!!\r\n", ch);
            return;
           }
        if (pen)
           paper = GET_EQ(ch, WEAR_HOLD);
        else
           pen = GET_EQ(ch, WEAR_HOLD);
     }


  /* ok.. now let's see what kind of stuff we've found */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN)
     act("Вы не уметете писать $o4.", FALSE, ch, pen, 0, TO_CHAR);
  else
  if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
     act("Вы не можете писать на $o5.", FALSE, ch, paper, 0, TO_CHAR);
  else
  if (paper->action_description)
     send_to_char("Там уже что-то записано.\r\n", ch);
  else
     {/* we can write - hooray! */
      /* this is the PERFECT code example of how to set up:
       * a) the text editor with a message already loaed
       * b) the abort buffer if the player aborts the message
       */
      ch->desc->backstr = NULL;
      send_to_char("Можете писать.  (/s СОХРАНИТЬ ЗАПИСЬ  /h ПОМОЩЬ)\r\n", ch);
      /* ok, here we check for a message ALREADY on the paper */
      if (paper->action_description)
         {/* we str_dup the original text to the descriptors->backstr */
          ch->desc->backstr = str_dup(paper->action_description);
          /* send to the player what was on the paper (cause this is already */
          /* loaded into the editor) */
          send_to_char(paper->action_description, ch);
         }
      act("$n начал$g писать.", TRUE, ch, 0, 0, TO_ROOM);
      /* assign the descriptor's->str the value of the pointer to the text */
      /* pointer so that we can reallocate as needed (hopefully that made */
      /* sense :>) */
      string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, NULL);
     }
}

ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;

  half_chop(argument, arg, buf2);

  if (IS_NPC(ch))
     send_to_char("Создания-не-персонажи этого не могут.. ступайте.\r\n", ch);
  else
  if (!*arg)
     send_to_char("Whom do you wish to page?\r\n", ch);
  else
     {sprintf(buf, "\007\007*$n* %s", buf2);
      if (!str_cmp(arg, "all") || !str_cmp(arg, "все"))
         {if (IS_GRGOD(ch))
             {for (d = descriptor_list; d; d = d->next)
	              if (STATE(d) == CON_PLAYING && d->character)
	                 act(buf, FALSE, ch, 0, d->character, TO_VICT);
             }
          else
	         send_to_char("Это доступно только БОГАМ !\r\n", ch);
          return;
         }
      if ((vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)) != NULL)
         {act(buf, FALSE, ch, 0, vict, TO_VICT);
          if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	         send_to_char(OK, ch);
          else
 	        act(buf, FALSE, ch, 0, vict, TO_CHAR);
         }
      else
         send_to_char("Такой игрок отсутствует !\r\n", ch);
     }
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

const char *auction_cmd[] =
{"поставить", "set",
 "снять",     "close",
 "ставка",    "value",
 "продать",   "sell",
 "\n"
};

void showlots(struct char_data *ch)
{int i;
 for (i = 0; i < MAX_AUCTION_LOT; i++)
     {if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
         {sprintf(buf,"Аукцион : лот %2d - свободен.\r\n",i);
          send_to_char(buf,ch);
          continue;
         }
      if (GET_LOT(i)->prefect && GET_LOT(i)->prefect != ch)
         {sprintf(buf,"Аукцион : лот %2d - %s%s%s (частный заказ).\r\n",
                  i,
                  CCIYEL(ch,C_NRM),GET_LOT(i)->item->PNames[0],CCNRM(ch,C_NRM));
          send_to_char(buf,ch);
          continue;
         }

      sprintf(buf,"Аукцион : лот %2d - %s%s%s - ставка %d %s, попытка %d, владелец %s",
              i,
              CCIYEL(ch,C_NRM),GET_LOT(i)->item->PNames[0],CCNRM(ch,C_NRM),
              GET_LOT(i)->cost, desc_count(GET_LOT(i)->cost, WHAT_MONEYa),
              GET_LOT(i)->tact < 0 ? 1 : GET_LOT(i)->tact + 1,
              GET_NAME(GET_LOT(i)->seller));
      if (GET_LOT(i)->prefect && GET_LOT(i)->prefect_unique == GET_UNIQUE(ch))
         {strcat(buf,", (для Вас).\r\n");
         }
      else
         {strcat(buf,".\r\n");
         }
      send_to_char(buf,ch);
     }
}

int auction_drive(struct char_data *ch, char *argument)
{int    mode = -1, value = -1, lot = -1;
 struct char_data *tch = NULL;
 struct auction_lot_type *lotis;
 struct obj_data *obj;
 char   operation[MAX_INPUT_LENGTH], whom[MAX_INPUT_LENGTH];

 if (!*argument)
    {showlots(ch);
     return (FALSE);
    }
 argument = one_argument(argument, operation);
 if ((mode = search_block(operation, auction_cmd, FALSE)) < 0)
    {send_to_char("Команды аукциона : поставить, снять, ставка, продать.\r\n",ch);
     return (FALSE);
    }
 mode >>= 1;
 switch(mode)
 {case 0: // Set lot
          if (!(lotis = free_auction(&lot)))
             {send_to_char("Нет свободных брокеров.\r\n",ch);
              return (FALSE);
             }
           *operation = '\0';
           *whom      = '\0';
           if (!sscanf(argument,"%s %d %s",operation,&value, whom))
              {send_to_char("Формат: аукцион поставить вещь [нач. ставка] [для кого]\r\n",ch);
               return (FALSE);
              }
           if (!*operation)
             {send_to_char("Не указан предмет.\r\n",ch);
              return (FALSE);
             }
           if (!(obj = get_obj_in_list_vis(ch, operation, ch->carrying)))
              {send_to_char("У Вас этого нет.\r\n", ch);
               return (FALSE);
              }
           if (OBJ_FLAGGED(obj,ITEM_DECAY)   ||
               OBJ_FLAGGED(obj,ITEM_NORENT)  ||
	       OBJ_FLAGGED(obj,ITEM_NODROP)  ||
	       OBJ_FLAGGED(obj,ITEM_NOSELL)  ||
               obj->obj_flags.cost      <= 0 ||
               obj->obj_flags.Obj_owner  > 0)
              {send_to_char("Этот предмет не предназначен для аукциона.\r\n",ch);
               return(FALSE);
              }
           if (obj_on_auction(obj))
              {send_to_char("Вы уже поставили на аукцион этот предмет.\r\n", ch);
               return (FALSE);
              }
           if (obj->contains)
              {sprintf(buf,"Опустошите %s перед продажей.\r\n",obj->PNames[3]);
               send_to_char(buf,ch);
               return (FALSE);
              }
           if (value <= 0)
              {value = MAX(1, GET_OBJ_COST(obj));
              };
           if (*whom)
              {if (!(tch = get_char_vis(ch, whom, FIND_CHAR_WORLD)))
                  {send_to_char("Вы не видите этого игрока.\r\n", ch);
                   return (FALSE);
                  }
               if (IS_NPC(tch))
                  {send_to_char("О этом персонаже позаботятся Боги.\r\n", ch);
                   return (FALSE);
                  }
               if (ch == tch)
                  {send_to_char("Но это же Вы !\r\n",ch);
                   return (FALSE);
                  }
              };
           lotis->item_id = GET_ID(obj);
           lotis->item    = obj;
           lotis->cost    = value;
           lotis->tact    = -1;
           lotis->seller_unique = GET_UNIQUE(ch);
           lotis->seller        = ch;
           lotis->buyer_unique  = lotis->prefect_unique = -1;
           lotis->buyer         = lotis->prefect = NULL;
           if (tch)
              {lotis->prefect_unique = GET_UNIQUE(tch);
               lotis->prefect        = tch;
              }
           lotis->cost = value;
           if (tch)
              {sprintf(buf,"Вы выставили на аукцион $O3 за %d %s (для %s)",
                      value,desc_count(value,WHAT_MONEYu),GET_PAD(tch,1));
              }
           else
              {sprintf(buf,"Вы выставили на аукцион $O3 за %d %s",
                       value,desc_count(value,WHAT_MONEYu));
              }
           act(buf,FALSE,ch,0,obj,TO_CHAR);
           sprintf(buf,"Аукцион : новый лот %d - %s - начальная ставка %d %s.", lot,
                   obj->PNames[0], value, desc_count(value,WHAT_MONEYa));
           return (TRUE);
           break;
  case 1: // Close
           if (!sscanf(argument,"%d",&lot))
              {send_to_char("Не указан номер лота.\r\n",ch);
               return (FALSE);
              }
           if (lot < 0 || lot >= MAX_AUCTION_LOT)
              {send_to_char("Неверный номер лота.\r\n",ch);
               return (FALSE);
              }
           if (GET_LOT(lot)->seller != ch ||
               GET_LOT(lot)->seller_unique != GET_UNIQUE(ch))
              {send_to_char("Это не Ваш лот.\r\n",ch);
               return (FALSE);
              }
           act("Вы сняли $O3 с аукциона.\r\n",FALSE,ch,0,GET_LOT(lot)->item,TO_CHAR);
           sprintf(buf,"Аукцион : лот %d(%s) снят%s с аукциона владельцем.\r\n", lot,
                   GET_LOT(lot)->item->PNames[0],
                   GET_OBJ_SUF_6(GET_LOT(lot)->item));
           clear_auction(lot);
           return (TRUE);
           break;
  case 2:  // Set
           if (sscanf(argument,"%d %d",&lot,&value) != 2)
              {send_to_char("Формат: аукцион ставка лот новая.цена\r\n",ch);
               return (FALSE);
              }
           if (lot < 0 || lot >= MAX_AUCTION_LOT)
              {send_to_char("Неверный номер лота.\r\n",ch);
               return (FALSE);
              }
           if (!GET_LOT(lot)->item   || GET_LOT(lot)->item_id <= 0 ||
               !GET_LOT(lot)->seller || GET_LOT(lot)->seller_unique <= 0)
              {send_to_char("Лот пуст.\r\n",ch);
               return (FALSE);
              }
           if (GET_LOT(lot)->seller == ch ||
               GET_LOT(lot)->seller_unique == GET_UNIQUE(ch))
              {send_to_char("Но это же Ваш лот !\r\n",ch);
               return (FALSE);
              }
            if (GET_LOT(lot)->prefect && GET_LOT(lot)->prefect_unique > 0 &&
                (GET_LOT(lot)->prefect != ch || GET_LOT(lot)->prefect_unique != GET_UNIQUE(ch)))
               {send_to_char("Этот лот имеет другого покупателя.\r\n",ch);
                return (FALSE);
               }
            if (GET_LOT(lot)->item->carried_by != GET_LOT(lot)->seller)
               {send_to_char("Вещь утеряна владельцем.\r\n",ch);
                sprintf(buf, "Аукцион : лот %d (%s) снят, ввиду смены владельца.",
                        lot, GET_LOT(lot)->item->PNames[0]);
                clear_auction(lot);
                return (TRUE);
               }
            if (value < GET_LOT(lot)->cost)
               {send_to_char("Ваша ставка ниже текущей.\r\n",ch);
                return (FALSE);
               }
            if (GET_LOT(lot)->buyer &&
                GET_UNIQUE(ch) != GET_LOT(lot)->buyer_unique &&
                value < GET_LOT(lot)->cost + MAX(1,GET_LOT(lot)->cost / 20))
               {send_to_char("Повышайте ставку не ниже 5% текущей.\r\n",ch);
                return (FALSE);
               }
            if (value > GET_GOLD(ch) + GET_BANK_GOLD(ch))
               {send_to_char("У Вас нет такой суммы.\r\n",ch);
                return (FALSE);
               }
            GET_LOT(lot)->cost         = value;
            GET_LOT(lot)->tact         = -1;
            GET_LOT(lot)->buyer        = ch;
            GET_LOT(lot)->buyer_unique = GET_UNIQUE(ch);
            sprintf(buf,"Хорошо, Вы согласны заплатить %d %s за %s (лот %d).\r\n",
                    value, desc_count(value,WHAT_MONEYu),
                    GET_LOT(lot)->item->PNames[3], lot);
            send_to_char(buf,ch);
            sprintf(buf,"Принята ставка %s на лот %d(%s) %d %s.\r\n",
                    GET_PAD(ch,1), lot, GET_LOT(lot)->item->PNames[0],
                    value, desc_count(value, WHAT_MONEYa));
            send_to_char(buf,GET_LOT(lot)->seller);
            sprintf(buf, "Аукцион : лот %d(%s) - новая ставка %d %s.",lot,
                    GET_LOT(lot)->item->PNames[0], value, desc_count(value,WHAT_MONEYa));
            return (TRUE);
            break;

  case 3:  // Sell
           if (!sscanf(argument,"%d",&lot))
              {send_to_char("Не указан номер лота.\r\n",ch);
               return (FALSE);
              }
           if (lot < 0 || lot >= MAX_AUCTION_LOT)
              {send_to_char("Неверный номер лота.\r\n",ch);
               return (FALSE);
              }
           if (GET_LOT(lot)->seller != ch ||
               GET_LOT(lot)->seller_unique != GET_UNIQUE(ch))
              {send_to_char("Это не Ваш лот.\r\n",ch);
               return (FALSE);
              }
           if (!GET_LOT(lot)->buyer)
              {send_to_char("Покупателя на Ваш товар пока нет.\r\n",ch);
               return (FALSE);
              }

           GET_LOT(lot)->prefect        = GET_LOT(lot)->buyer;
           GET_LOT(lot)->prefect_unique = GET_LOT(lot)->buyer_unique;
           if (GET_LOT(lot)->tact < MAX_AUCTION_TACT_BUY)
              {sprintf(whom,"Аукцион : лот %d(%s) продан с аукциона за %d %s.",lot,GET_LOT(lot)->item->PNames[0],
                       GET_LOT(lot)->cost, desc_count(GET_LOT(lot)->cost,WHAT_MONEYu));
               GET_LOT(lot)->tact = MAX_AUCTION_TACT_BUY;
              }
           else
              *whom = '\0';
           sell_auction(lot);
           if (*whom)
              {strcpy (buf, whom);
               return (TRUE);
              }
           return (FALSE);
           break;
 }
 return (FALSE);
}



ACMD(do_gen_comm)
{
  struct descriptor_data *i;
  char color_on[24];

  /* Array of flags which must _not_ be set in order for comm to be heard */
  int channels[] = {
    0,
    PRF_DEAF,
    PRF_NOGOSS,
    PRF_NOAUCT,
    PRF_NOGRATZ,
    0
  };
// Исправления Стрибога
/*  int access_levels[] = {
   4,
   1,
   4,
   1,
   1,
   1
 }; */
  /*
   * com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   *           [3] a color string.
   */
  const char *com_msgs[][6] = {
    {"Вы не можете орать.\r\n", /* holler */
     "орать",
     "",
     KIYEL,
     "заорали",
     "заорал$g"},

    {"Вам запрещено кричать.\r\n", /* shout */
     "кричать",
     "Прежде, включите себе разрешение кричать.\r\n",
     KIYEL,
     "закричали",
     "закричал$g"},

    {"Вам недозволено болтать.\r\n", /* gossip */
     "болтать",
     "Вы вне видимости канала.\r\n",
     KIYEL,
     "заметили",
     "заметил$g"},

    {"Вам не к лицу торговаться.\r\n", /* auction */
     "торговать",
     "Вы вне видимости канала.\r\n",
     KIYEL,
     "попробовали поторговаться",
     "вступил$g в торг"},

    {"Ваши поздравления ни к чему.\r\n", /* congratulate */
     "поздравлять",
     "Вы вне видимости канала.\r\n",
     KIBLU,
     "присоединились к поздравлениям",
     "провозгласил$g"}
  };

  /* to keep pets, etc from being ordered to shout */
  if (!ch->desc)
     return;

  if (AFF_FLAGGED(ch, AFF_SIELENCE))
     {send_to_char(SIELENCE, ch);
      return;
     }

  if (PLR_FLAGGED(ch, PLR_NOSHOUT))
     {send_to_char(com_msgs[subcmd][0], ch);
      return;
     }
  if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
     {send_to_char("Стены заглушили Ваши слова.\r\n", ch);
      return;
     }

  /* level_can_shout defined in config.c */
  if (GET_LEVEL(ch) < level_can_shout)
//  access_levels[subcmd])
     {sprintf(buf1, "Вам стоит достичь хотя бы %d уровня, чтобы Вы могли %s.\r\n",
              level_can_shout, com_msgs[subcmd][1]);
      send_to_char(buf1, ch);
      return;
     }

  /* make sure the char is on the channel */
  if (PRF_FLAGGED(ch, channels[subcmd]))
     {send_to_char(com_msgs[subcmd][2], ch);
      return;
     }

  /* skip leading spaces */
  skip_spaces(&argument);

  /* make sure that there is something there to say! */
  if (!*argument && subcmd != SCMD_AUCTION)
     {sprintf(buf1, "ЛЕГКО ! Но, Ярило Вас побери, %s %s ???\r\n",
              subcmd == SCMD_GRATZ ? "КОГО" : "ЧТО", com_msgs[subcmd][1]);
      send_to_char(buf1, ch);
      return;
     }

  if (subcmd == SCMD_HOLLER || subcmd == SCMD_GOSSIP)
     {if (!check_moves(ch,holler_move_cost))
         return;
     }

  /* set up the color on code */
  strcpy(color_on, com_msgs[subcmd][3]);

  /* first, set up strings to be given to the communicator */
  if (subcmd == SCMD_AUCTION)
     {*buf = '\0';

      if (!auction_drive(ch,argument))
         return;
     }
  else
     {if (PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(OK, ch);
      else
         {if (COLOR_LEV(ch) >= C_CMP)
             sprintf(buf1, "%sВы %s : '%s'%s", color_on, com_msgs[subcmd][4],
	                 argument, KNRM);
          else
             sprintf(buf1, "Вы %s : '%s'", com_msgs[subcmd][4], argument);
          act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
         }


      sprintf(buf, "$n %s : '%s'", com_msgs[subcmd][5], argument);
     }

  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next)
      {if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
           !PRF_FLAGGED(i->character, channels[subcmd]) &&
           !PLR_FLAGGED(i->character, PLR_WRITING) &&
           !ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF) &&
           GET_POS(i->character) > POS_SLEEPING
          )
	  {if (subcmd == SCMD_SHOUT &&
	       ((world[ch->in_room].zone != world[i->character->in_room].zone) ||
                !AWAKE(i->character)))
               continue;

           if (COLOR_LEV(i->character) >= C_NRM)
              send_to_char(color_on, i->character);
           act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
           if (COLOR_LEV(i->character) >= C_NRM)
              send_to_char(KNRM, i->character);
          }
      }
}


ACMD(do_mobshout)
{ struct descriptor_data *i;

  /* to keep pets, etc from being ordered to shout */
  if (!(IS_NPC(ch) || WAITLESS(ch)))
     return;
  if (AFF_FLAGGED(ch,AFF_CHARM))
     return;
  sprintf(buf, "$n заорал$g : '%s'", argument);

  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next)
      {if (STATE(i) == CON_PLAYING && i->character &&
           !PLR_FLAGGED(i->character, PLR_WRITING) &&
           GET_POS(i->character) > POS_SLEEPING
          )
	  {if (COLOR_LEV(i->character) >= C_NRM)
              send_to_char(KIYEL, i->character);
           act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
           if (COLOR_LEV(i->character) >= C_NRM)
              send_to_char(KNRM, i->character);
          }
      }
}

ACMD(do_qcomm)
{
  struct descriptor_data *i;

  if (!PRF_FLAGGED(ch, PRF_QUEST))
     {send_to_char("У Вас нет никаких заданий !\r\n", ch);
      return;
     }
  skip_spaces(&argument);

  if (!*argument)
     {sprintf(buf, "ЛЕГКО ! Но ЧТО %s ?\r\n", CMD_NAME);
      CAP(buf);
      send_to_char(buf, ch);
     }
  else
     {if (PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(OK, ch);
      else
         {if (subcmd == SCMD_QSAY)
	         sprintf(buf, "Вы доложили : '%s'", argument);
          else
	         strcpy(buf, argument);
          act(buf, FALSE, ch, 0, argument, TO_CHAR);
         }

      if (subcmd == SCMD_QSAY)
         sprintf(buf, "$n доложил$g : '%s'", argument);
      else
         strcpy(buf, argument);

      for (i = descriptor_list; i; i = i->next)
          if (STATE(i) == CON_PLAYING && i != ch->desc &&
	          PRF_FLAGGED(i->character, PRF_QUEST))
	      act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
     }
}