/* ************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
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
#include "constants.h"
#include "screen.h"
#include "spells.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;

/* extern functions */
void   raw_kill(struct char_data * ch, struct char_data * killer);
void   check_killer(struct char_data * ch, struct char_data * vict);
int    compute_armor_class(struct char_data *ch);
int    awake_others(struct char_data * ch);
void   appear(struct char_data * ch);
int    may_kill_here(struct char_data *ch, struct char_data *victim);
int    legal_dir(struct char_data *ch, int dir, int need_specials_check, int show_msg);
int    may_pkill(struct char_data *revenger, struct char_data *killer);
int    inc_pkill(struct char_data *victim, struct char_data *killer,
                 int pkills, int prevenge);
void   inc_pkill_group(struct char_data * victim, struct char_data *killer,
                       int pkills, int prevenge);

/* local functions */
ACMD(do_assist);
ACMD(do_hit);
ACMD(do_kill);
ACMD(do_backstab);
ACMD(do_order);
ACMD(do_flee);
ACMD(do_bash);
ACMD(do_rescue);
ACMD(do_kick);

/* prevent accidental pkill */
#define CHECK_PKILL(ch,opponent) (!IS_NPC(opponent) ||\
     (opponent->master && !IS_NPC(opponent->master) && opponent->master != ch))



int check_pkill(struct char_data *ch,struct char_data *opponent,char *arg)
{char *pos;

 if (!*arg)
    return (FALSE);

 if (!CHECK_PKILL(ch,opponent))
    return (FALSE);

 if (FIGHTING(ch) == opponent || FIGHTING(opponent) == ch)
    return (FALSE);

 while (*arg && (*arg == '.' || (*arg >= '0' && *arg <= '9')))
       arg++;

 if ((pos = strchr(arg,'!')))
    *pos = '\0';

 if (str_cmp(arg, GET_NAME(opponent)))
    {send_to_char("��� ���������� ����������������� �������� ������� ��� ������ ���������.\r\n",ch);
     if (pos)
        *pos = '!';
     return (TRUE);
    }
 if (pos)
    *pos = '!';
 return (FALSE);
}

int have_mind(struct char_data *ch)
{if (!AFF_FLAGGED(ch,AFF_CHARM) && !IS_HORSE(ch))
    return (TRUE);
 return (FALSE);
}

void set_wait(struct char_data *ch, int waittime, int victim_in_room)
{if (!WAITLESS(ch) &&
     (!victim_in_room ||
      (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
     )
    )
    WAIT_STATE(ch, waittime * PULSE_VIOLENCE);
};

int set_hit(struct char_data *ch, struct char_data *victim)
{if (AFF_FLAGGED(ch,AFF_STOPFIGHT))
    {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
     return (FALSE);
    }
 if (FIGHTING(ch) || GET_MOB_HOLD(ch))
    {return (FALSE);
    }
 hit(ch,victim,TYPE_UNDEFINED,AFF_FLAGGED(ch,AFF_STOPRIGHT) ? 2 : 1);
 set_wait(ch,2,TRUE);
 return (TRUE);
};

int onhorse(struct char_data *ch)
{ if (on_horse(ch))
     {act("��� ������ $N.",FALSE,ch,0,get_horse(ch),TO_CHAR);
      return (TRUE);
     }
  return (FALSE);
};

int used_attack(struct char_data *ch)
{char *message = NULL;

 if (GET_AF_BATTLE(ch,EAF_BLOCK))
    message = "����������. �� ��������� ����������� �����.";
 else
 if (GET_AF_BATTLE(ch,EAF_PARRY))
    message = "����������. �� ��������� ���������� �����.";
 else
 if (GET_AF_BATTLE(ch,EAF_MULTYPARRY))
    message = "����������. �� ������������� �� ������� ������.";
 else
 if (!GET_EXTRA_VICTIM(ch))
    return (FALSE);
 else
    switch (GET_EXTRA_SKILL(ch))
    {case SKILL_BASH:
          message = "����������. �� ��������� ����� $N3.";
          break;
     case SKILL_KICK:
          message = "����������. �� ��������� ����� $N3.";
          break;
     case SKILL_CHOPOFF:
          message = "����������. �� ��������� ������� $N3.";
          break;
     case SKILL_DISARM:
          message = "����������. �� ��������� ����������� $N3.";
          break;
     case SKILL_THROW:
          message = "����������. �� ��������� ������� ������ � $N3.";
          break;
     default:
          return (FALSE);
    }
 if (message)
    act(message, FALSE, ch, 0, GET_EXTRA_VICTIM(ch), TO_CHAR);
 return (TRUE);
}

ACMD(do_assist)
{ struct char_data *helpee, *opponent;

  if (FIGHTING(ch))
     {send_to_char("����������. �� ���������� ����.\r\n", ch);
      return;
     }
  one_argument(argument, arg);

  if (!*arg)
     {for (helpee = world[ch->in_room].people; helpee; helpee = helpee->next_in_room)
           if (FIGHTING(helpee) && FIGHTING(helpee) != ch &&
               ((ch->master && ch->master == helpee->master) ||
                ch->master == helpee ||
                helpee->master == ch))
              break;
      if (!helpee)
         {send_to_char("���� �� ������ ������ ?\r\n", ch);
          return;
         }
     }
  else
  if (!(helpee = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {send_to_char(NOPERSON, ch);
      return;
     }
  else
  if (helpee == ch)
     {send_to_char("��� ����� ������ ������ ���� !\r\n", ch);
      return;
     }

 /*
  * Hit the same enemy the person you're helping is.
  */
 if (FIGHTING(helpee))
    opponent = FIGHTING(helpee);
 else
    for (opponent = world[ch->in_room].people;
         opponent && (FIGHTING(opponent) != helpee);
         opponent = opponent->next_in_room);

 if (!opponent)
    act("�� ����� �� ��������� � $N4!", FALSE, ch, 0, helpee, TO_CHAR);
 else
 if (!CAN_SEE(ch, opponent))
    act("�� �� ������ ���������� $N1 !", FALSE, ch, 0, helpee, TO_CHAR);
 else
 if (opponent == ch)
    act("��� $E ��������� � ���� !", FALSE, ch, 0, helpee, TO_CHAR);
 else
 if CHECK_PKILL(ch, opponent)
    act("����������� ������� '���������' ��� ��������� �� $N1.", FALSE,
        ch, 0, opponent, TO_CHAR);
 else
 if (!may_kill_here(ch, opponent))
    return;
 else
 if (set_hit(ch, opponent))
    {act("�� �������������� � �����, ������� $N2!", FALSE, ch, 0, helpee, TO_CHAR);
     act("$N �����$G ������ ��� � ����� !", 0, helpee, 0, ch, TO_CHAR);
     act("$n �������$g � ��� �� ������� $N1.", FALSE, ch, 0, helpee, TO_NOTVICT);
    }
}


ACMD(do_hit)
{
  struct char_data *vict;

  one_argument(argument, arg);

  if (!*arg)
     send_to_char("���� ����-�� ����� ?\r\n", ch);
  else
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     send_to_char("�� �� ������ ����.\r\n", ch);
  else
  if (vict == ch)
     {send_to_char("�� ������� ����... ���� ������ ��!.\r\n", ch);
      act("$n ������$g ����, � ������ �������$g '�������, ������ ����...'", FALSE, ch, 0, vict, TO_ROOM);
     }
  else
  if (!may_kill_here(ch,vict))
     return;
  else
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
     act("$N ������� ����� ��� ���, ����� ���� $S.", FALSE, ch, 0, vict, TO_CHAR);
  else
     {if (subcmd != SCMD_MURDER && check_pkill(ch, vict, arg))
          return;
      if (FIGHTING(ch))
         {if (vict == FIGHTING(ch))
             {act("�� ��� ���������� � $N4.",FALSE,ch,0,vict,TO_CHAR);
              return;
             }
          if (!FIGHTING(vict))
             {act("$N �� ��������� � ����, �� �������� $S.",FALSE,ch,0,vict,TO_CHAR);
              return;
             }
          stop_fighting(ch,FALSE);
	  set_fighting(ch,vict);
	  set_wait(ch,2,TRUE);
         }
      else
      if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch)))
         {set_hit(ch, vict);
         }
      else
         send_to_char("��� ���� �� �� ��� !\r\n", ch);
     }
}



ACMD(do_kill)
{
  struct char_data *vict;

  if (!IS_IMPL(ch))
     {do_hit(ch, argument, cmd, subcmd);
      return;
     }
  one_argument(argument, arg);

  if (!*arg)
     {send_to_char("���� �� ����� ������ ������-�� ?\r\n", ch);
     }
  else
     {if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
         send_to_char("� ��� ��� ����� :P.\r\n", ch);
      else
      if (ch == vict)
         send_to_char("�� ��������... :(\r\n", ch);
      else
      if (IS_IMPL(vict) && !GET_COMMSTATE(ch))
         send_to_char("� ���� �� ��� �������� �������� ? �����, �������, ����� !\r\n",ch);
      else
         {act("�� �������� $N3 � ����! ��������! �����!", FALSE, ch, 0, vict, TO_CHAR);
          act("$N �������$g ��� � ���� ����� ����������� ������!", FALSE, vict, 0, ch, TO_CHAR);
          act("$n ������ ���������$g �������� $N3!", FALSE, ch, 0, vict, TO_NOTVICT);
          raw_kill(vict, ch);
         }
     }
}

/************************ BACKSTAB VICTIM */
void go_backstab(struct char_data *ch, struct char_data *vict)
{ int percent, prob;


  if (onhorse(ch))
     return;

  if (may_pkill(ch,vict) != PC_REVENGE_PC)
     {inc_pkill_group(vict, ch, 1, 0);
     }
  else
     {inc_pkill(vict, ch, 0, 1);
     };


  if (((MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict)) || FIGHTING(vict)) &&
      !IS_GOD(ch)
     )
     {act("�� ��������, ��� $N �������$y ��� �������� !", FALSE, vict, 0, ch, TO_CHAR);
      act("$n �������$g ���� �������� $s !", FALSE, vict, 0, ch, TO_VICT);
      act("$n �������$g ������� $N1 �������� $s !", FALSE, vict, 0, ch, TO_NOTVICT);
      set_hit(vict,ch);
      return;
     }

  percent = number(1,skill_info[SKILL_BACKSTAB].max_percent);
  prob    = train_skill(ch, SKILL_BACKSTAB, skill_info[SKILL_BACKSTAB].max_percent, vict);

  if (GET_GOD_FLAG(vict, GF_GODSCURSE) ||
      GET_MOB_HOLD(vict))
     percent = prob;
  if (GET_GOD_FLAG(vict, GF_GODSLIKE) ||
      GET_GOD_FLAG(ch, GF_GODSCURSE))
     prob = 0;

  if (percent > prob)
     damage(ch, vict, 0, SKILL_BACKSTAB + TYPE_HIT, TRUE);
  else
     hit(ch, vict, SKILL_BACKSTAB, 1);
  set_wait(ch,2,TRUE);
}


ACMD(do_backstab)
{
  struct char_data *vict;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BACKSTAB))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (onhorse(ch))
     return;

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {send_to_char("���� �� ��� ������ ����������, ��� ������ �������� ?\r\n", ch);
      return;
     }

  if (vict == ch)
     {send_to_char("��, �����������, ������������ !\r\n", ch);
      return;
     }

  if (!may_kill_here(ch,vict))
     return;

  if (!GET_EQ(ch, WEAR_WIELD))
     {send_to_char("��������� ������� ������ � ������ ����.\r\n", ch);
      return;
     }

  if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT)
     {send_to_char("�������� ����� ������ ������ ������� !\r\n", ch);
      return;
     }

  if (AFF_FLAGGED(ch, AFF_STOPRIGHT) || AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  if (FIGHTING(vict))
     {send_to_char("���� ���� ������� ������ �������� - �� ������ ���������� !\r\n", ch);
      return;
     }
  if (check_pkill(ch, vict, arg))
     return;

  go_backstab(ch, vict);
}


/******************* CHARM ORDERS PROCEDURES */
ACMD(do_order)
{
  char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
  bool found = FALSE;
  room_rnum org_room;
  struct char_data *vict;
  struct follow_type *k, *k_next;

  half_chop(argument, name, message);

  if (!*name || !*message)
     send_to_char("��������� ��� � ���� ?\r\n", ch);
  else
  if (!(vict = get_char_vis(ch, name, FIND_CHAR_ROOM)) &&
      !is_abbrev(name, "followers") &&
      !is_abbrev(name, "����"))
    send_to_char("�� �� ������ ������ ���������.\r\n", ch);
  else
  if (ch == vict)
    send_to_char("�� ������ ������� ������������ ������ - ������ � ��������� !\r\n", ch);
  else
    {if (AFF_FLAGGED(ch, AFF_CHARM))
        {send_to_char("� ����� ��������� �� �� ������ ���� �������� �������.\r\n", ch);
         return;
        }
    if (vict)
       {sprintf(buf, "$N ��������$g ��� '%s'", message);
        act(buf, FALSE, vict, 0, ch, TO_CHAR);
        act("$n �����$g ������ $N2.", FALSE, ch, 0, vict, TO_ROOM);

        if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
            act("$n ����������� ������� �� ��������.", FALSE, vict, 0, 0, TO_ROOM);
        else
           {send_to_char(OK, ch);
            if (GET_WAIT_STATE(vict) <= 0)
               command_interpreter(vict, message);
           }
       }
    else
      {/* This is order "followers" */
       sprintf(buf, "$n ��������$g : '%s'.", message);
       act(buf, FALSE, ch, 0, vict, TO_ROOM);

       org_room = ch->in_room;

       for (k = ch->followers; k; k = k_next)
           {k_next = k->next;
	    if (org_room == k->follower->in_room)
               if (AFF_FLAGGED(k->follower, AFF_CHARM))
                  {found = TRUE;
                   if (GET_WAIT_STATE(k->follower) <= 0)
                      command_interpreter(k->follower, message);
                  }
           }
       if (found)
          send_to_char(OK, ch);
       else
          send_to_char("�� ��������� ������ ������� !\r\n", ch);
      }
  }
}

/********************** FLEE PROCEDURE */
void go_flee(struct char_data * ch)
{ int    i, attempt, loss, scandirs = 0, was_in = IN_ROOM(ch);
  struct char_data *was_fighting;

  if (on_horse(ch) &&
      GET_POS(get_horse(ch)) >= POS_FIGHTING &&
      !GET_MOB_HOLD(get_horse(ch)))
     {if (!WAITLESS(ch))
         WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
      while (scandirs != (1 << NUM_OF_DIRS) - 1)
           { attempt = number(0, NUM_OF_DIRS-1);
             if (IS_SET(scandirs, (1 << attempt)))
                continue;
             SET_BIT(scandirs, (1 << attempt));
             if (!legal_dir(ch,attempt,TRUE,FALSE) ||
                 ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)
		)
                continue;
             was_fighting = FIGHTING(ch);
             if (do_simple_move(ch,attempt | 0x80,TRUE))
                {act("����$W $N �����$Q ��� �� ���.",FALSE,ch,0,get_horse(ch),TO_CHAR);
                 if (was_fighting && !IS_NPC(ch))
                    {loss  = GET_REAL_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
                     loss *= GET_LEVEL(was_fighting);
                     if (!IS_THIEF(ch)    &&
		         !IS_MERCHANT(ch) &&
			 !ROOM_FLAGGED(was_in,ROOM_ARENA)
		        )
                        gain_exp(ch, -loss);
                    }
                 return;
                }
           }
     }

  if (GET_MOB_HOLD(ch))
     return;
  if (AFF_FLAGGED(ch,AFF_NOFLEE))
     {send_to_char("��������� ����� ������ ��� �������.\r\n",ch);
      return;
     }
  if (GET_WAIT(ch) > 0)
     return;
  if (GET_POS(ch) < POS_FIGHTING)
     {send_to_char("�� �� ������ ������� �� ����� ���������.\r\n",ch);
      return;
     }
  if (!WAITLESS(ch))
     WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
  for (i = 0; i < 6; i++)
      { attempt = number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
        if (legal_dir(ch,attempt,TRUE,FALSE) &&
            !ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)
	   )
           {act("$n �����������$g � �����$u ������� !", TRUE, ch, 0, 0, TO_ROOM);
            was_fighting = FIGHTING(ch);
            if (do_simple_move(ch, attempt | 0x80, TRUE))
               {send_to_char("�� ������ ������� � ���� �����.\r\n", ch);
                if (was_fighting && !IS_NPC(ch))
                   {loss  = GET_REAL_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
                    loss *= GET_LEVEL(was_fighting);
                     if (!IS_THIEF(ch)    &&
		         !IS_MERCHANT(ch) &&
			 !ROOM_FLAGGED(was_in,ROOM_ARENA)
		        )
                        gain_exp(ch, -loss);
                   }
               }
            else
               {act("$n �����������$g � �������$u �������, �� �� ����$q!", FALSE, ch, 0, 0, TO_ROOM);
                send_to_char("������ �������� ����. �� �� ������ ������� !\r\n", ch);
               }
            return;
           }
      }
  send_to_char("������ �������� ����. �� �� ������ ������� !\r\n", ch);
}


void go_dir_flee(struct char_data * ch, int direction)
{ int    attempt, loss, scandirs = 0, was_in = IN_ROOM(ch);
  struct char_data *was_fighting;

  if (GET_MOB_HOLD(ch))
     return;
  if (AFF_FLAGGED(ch,AFF_NOFLEE))
     {send_to_char("��������� ����� ������ ��� �������.\r\n",ch);
      return;
     }
  if (GET_WAIT(ch) > 0)
     return;
  if (GET_POS(ch) < POS_FIGHTING)
     {send_to_char("�� �� ������� ������� �� ����� ���������.\r\n",ch);
      return;
     }

  if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch,GF_GODSLIKE)))
     WAIT_STATE(ch, 1 * PULSE_VIOLENCE);

  while (scandirs != (1 << NUM_OF_DIRS) - 1)
        { attempt   = direction >= 0 ? direction : number(0, NUM_OF_DIRS-1);
          direction = -1;
          if (IS_SET(scandirs, (1 << attempt)))
             continue;
          SET_BIT(scandirs, (1 << attempt));
          if (!legal_dir(ch,attempt,TRUE,FALSE) ||
              ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)
             )
             continue;
          act("$n �����������$g � �������$u �������.", FALSE, ch, 0, 0, TO_ROOM);
          was_fighting = FIGHTING(ch);
          if (do_simple_move(ch,attempt | 0x80,TRUE))
             {send_to_char("�� ������ ������� � ���� �����.\r\n",ch);
              if (was_fighting && !IS_NPC(ch))
                 {loss  = GET_REAL_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
      	          loss *= GET_LEVEL(was_fighting);
                  if (!IS_THIEF(ch)    &&
                      !IS_MERCHANT(ch) &&
		      !ROOM_FLAGGED(was_in,ROOM_ARENA)
		     )
                    gain_exp(ch, -loss);
	         }
              return;
             }
          else
             send_to_char("������ �������� ���� ! �� �� ������ c������.\r\n",ch);
        }
}


const char *FleeDirs[] =
{"�����",
 "������",
 "��",
 "�����",
 "�����",
 "����",
 "\n"
};

ACMD(do_flee)
{ int direction = -1;
  if (!FIGHTING(ch))
     {send_to_char("�� �� ���� �� � ��� �� ���������� !\r\n", ch);
      return;
     }
  if (IS_THIEF(ch) || IS_MERCHANT(ch) || IS_IMMORTAL(ch) || GET_GOD_FLAG(ch,GF_GODSLIKE))
     {one_argument(argument,arg);
      if ( (direction = search_block(arg, dirs, FALSE))     >= 0 ||
           (direction = search_block(arg, FleeDirs, FALSE)) >= 0 )
         {go_dir_flee(ch, direction);
          return;
         }
     }
  go_flee(ch);
}

/************************** BASH PROCEDURES */
void go_bash(struct char_data * ch, struct char_data * vict)
{
  int percent=0, prob;

  if (AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_STOPLEFT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  if (onhorse(ch))
     return;

  if (!(IS_NPC(ch)              ||         // ���
        GET_EQ(ch, WEAR_SHIELD) ||         // ���� ���
        IS_IMMORTAL(ch)         ||         // �����������
        GET_MOB_HOLD(vict)      ||         // ���� ���������
        GET_GOD_FLAG(vict,GF_GODSCURSE)    // ���� ��������
       ))
     {send_to_char("�� �� ������ ������� ����� ��� ����.\r\n", ch);
      return;
     };

  if (GET_POS(ch) < POS_FIGHTING)
     {send_to_char("��� ����� ������ �� ����.\r\n", ch);
      return;
     }

  percent = number(1,skill_info[SKILL_BASH].max_percent);
  prob    = train_skill(ch, SKILL_BASH, skill_info[SKILL_BASH].max_percent, vict);
  if (GET_GOD_FLAG(ch,GF_GODSLIKE) || GET_MOB_HOLD(vict))
     prob = percent;
  if (vict && GET_GOD_FLAG(vict,GF_GODSCURSE))
     prob = percent;
  if (GET_GOD_FLAG(ch,GF_GODSCURSE))
     prob = 0;
  if (MOB_FLAGGED(vict, MOB_NOBASH))
     prob = 0;

  if (percent > prob)
     {damage(ch, vict, 0, SKILL_BASH + TYPE_HIT, TRUE);
      GET_POS(ch) = POS_SITTING;
      prob = 3;
     }
  else
    {/*
      * If we bash a player and they wimp out, they will move to the previous
      * room before we set them sitting.  If we try to set the victim sitting
      * first to make sure they don't flee, then we can't bash them!  So now
      * we only set them sitting if they didn't flee. -gg 9/21/98
      */
      int   dam = str_app[GET_REAL_STR(ch)].todam + GET_REAL_DR(ch) +
                  MAX(0,GET_SKILL(ch,SKILL_BASH)/10 - 5) + GET_LEVEL(ch) / 5;
/* prool: add "\" in end of line  */
      log("[BASH params] = actor = %s, actorlevel = %d, actordex = %d\
           target=  %s, targetlevel = %d, targetdex = %d ,skill = %d,\
           dice = %d, dam = %d", GET_NAME(ch), GET_LEVEL(ch), GET_REAL_DEX(ch),
	   GET_NAME(vict), GET_LEVEL(vict), GET_REAL_DEX(vict),
	   percent, prob, dam);		
      prob  = 0;
      if (damage(ch, vict, dam, SKILL_BASH + TYPE_HIT, FALSE) > 0)
         {/* -1 = dead, 0 = miss */
          prob = 3;
          if (IN_ROOM(ch) == IN_ROOM(vict))
             {GET_POS(vict) = POS_SITTING;
              if (on_horse(vict))
                 {act("�� ����� � $N1.",FALSE,vict,0,get_horse(vict),TO_CHAR);
                  REMOVE_BIT(AFF_FLAGS(vict,AFF_HORSE), AFF_HORSE);
                 }
              if (IS_HORSE(vict) && on_horse(vict->master))
                 horse_drop(vict);
             }
          set_wait(vict,prob,FALSE);
          prob = 2;
         }
     }
  set_wait(ch,prob,TRUE);
}


ACMD(do_bash)
{
  struct char_data *vict=NULL;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BASH))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (onhorse(ch))
     return;

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
    {if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
        vict = FIGHTING(ch);
     else
        {send_to_char("���� �� �� ��� ������ ������� ����� ?\r\n", ch);
         return;
        }
    }

  if (vict == ch)
     {send_to_char("��� ����������� ���� ������ ��� ������... �� ������������� ���� �����.\r\n", ch);
      return;
     }

  if (!may_kill_here(ch,vict))
     return;

  if (check_pkill(ch,vict,arg))
     return;

  if (IS_IMPL(ch) || !FIGHTING(ch))
     go_bash(ch, vict);
  else
  if (!used_attack(ch))
     {act("������. �� ����������� ����� $N3.",FALSE,ch,0,vict,TO_CHAR);
      SET_EXTRA(ch,SKILL_BASH,vict);
     }
}

/******************** RESCUE PROCEDURES */
void go_rescue(struct char_data * ch, struct char_data * vict, struct char_data * tmp_ch)
{ int percent, prob;

  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  percent = number(1,skill_info[SKILL_RESCUE].max_percent * 130 / 100);
  prob    = calculate_skill(ch, SKILL_RESCUE, skill_info[SKILL_RESCUE].max_percent, vict);
  improove_skill(ch, SKILL_RESCUE, prob >= percent, 0);

  if (GET_GOD_FLAG(ch,GF_GODSLIKE))
     prob = percent;
  if (GET_GOD_FLAG(ch,GF_GODSCURSE))
     prob = 0;

  if (percent != skill_info[SKILL_RESCUE].max_percent &&
      percent > prob
     )
     {act("�� ���������� �������� ������ $N3", FALSE, ch, 0, vict, TO_CHAR);
      set_wait(ch,1,FALSE);
      return;
     }

  act("����� �����, �� ���������� ������ $N3 !", FALSE, ch, 0, vict, TO_CHAR);
  act("�� ���� ������� $N4. �� ���������� ���� �����!", FALSE, vict, 0, ch, TO_CHAR);
  act("$n ���������� ����$q $N3!", TRUE, ch, 0, vict, TO_NOTVICT);

  if (FIGHTING(vict) == tmp_ch)
     stop_fighting(vict,FALSE);
  if (FIGHTING(tmp_ch))
     stop_fighting(tmp_ch,FALSE);
  if (FIGHTING(ch))
     stop_fighting(ch,FALSE);

  if (!IS_NPC(ch))
     {if (may_pkill(ch,tmp_ch) != PC_REVENGE_PC)
         inc_pkill_group(tmp_ch, ch, 1, 0);
      else
         inc_pkill(tmp_ch, ch, 0, 1);
     };
  set_fighting(ch, tmp_ch);
  set_fighting(tmp_ch, ch);
  set_wait(ch,   1, FALSE);
  set_wait(vict, 2, FALSE);
}

ACMD(do_rescue)
{
  struct char_data *vict, *tmp_ch;

  if (!GET_SKILL(ch, SKILL_RESCUE))
     {send_to_char("�� �� �� ������ ���.\r\n", ch);
      return;
     }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {send_to_char("���� �� ������ ������ ?\r\n", ch);
      return;
     }

  if (vict == ch)
     {send_to_char("���� �������� �� ������ �������� ������ �����.\r\n", ch);
      return;
     }
  if (FIGHTING(ch) == vict)
     {send_to_char("�� ��������� ������ ���������� ��� ?\r\n"
                   "��� �� � ��� �� ������ ���� � ���� ?\r\n", ch);
      return;
     }

  for (tmp_ch = world[ch->in_room].people;
       tmp_ch && (FIGHTING(tmp_ch) != vict);
       tmp_ch = tmp_ch->next_in_room
      );

  if (!tmp_ch)
     {act("�� ����� �� ��������� � $N4!", FALSE, ch, 0, vict, TO_CHAR);
      return;
     }

  if (!may_kill_here(ch,tmp_ch))
     return;

  go_rescue(ch, vict, tmp_ch);
}

/*******************  KICK PROCEDURES */
void go_kick(struct char_data *ch, struct char_data * vict)
{ int percent, prob;

  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  if (onhorse(ch))
     return;

  /* 101% is a complete failure */
  percent = ((10 - (compute_armor_class(vict) / 10)) * 2) +
            number(1, skill_info[SKILL_KICK].max_percent);
  prob    = train_skill(ch, SKILL_KICK, skill_info[SKILL_KICK].max_percent, vict);
  if (GET_GOD_FLAG(vict,GF_GODSCURSE) || GET_MOB_HOLD(vict) > 0)
     prob = percent;
  if (GET_GOD_FLAG(ch,GF_GODSCURSE) || on_horse(vict))
     prob = 0;

  if (percent > prob)
     {damage(ch, vict, 0, SKILL_KICK + TYPE_HIT, TRUE);
      prob = 2;
     }
  else
     {int dam = str_app[GET_REAL_STR(ch)].todam + GET_REAL_DR(ch) +
                MAX(0,GET_SKILL(ch,SKILL_KICK)/10 - 5) + GET_LEVEL(ch) / 3;
      damage(ch, vict, dam, SKILL_KICK + TYPE_HIT, TRUE);
      prob = 2;
     }
  set_wait(ch, prob, TRUE);
}

ACMD(do_kick)
{ struct char_data *vict=NULL;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_KICK))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (onhorse(ch))
     return;

  one_argument(argument, arg);
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
         vict = FIGHTING(ch);
      else
         {send_to_char("��� ��� ��� ������ �������� ��� ������ ������ ?\r\n", ch);
          return;
         }
     }
  if (vict == ch)
     {send_to_char("�� ������ ����� ���� ! ����������, �� ����� � ���...\r\n", ch);
      return;
     }

  if (!may_kill_here(ch, vict))
     return;

  if (check_pkill(ch, vict, arg))
     return;

  if (IS_IMPL(ch) || !FIGHTING(ch))
     go_kick(ch, vict);
  else
  if (!used_attack(ch))
     {act("������. �� ����������� ����� $N3.",FALSE,ch,0,vict,TO_CHAR);
      SET_EXTRA(ch, SKILL_KICK, vict);
     }
}

/******************** BLOCK PROCEDURES */
void go_block(struct char_data * ch)
{ if (AFF_FLAGGED(ch, AFF_STOPLEFT))
     {send_to_char("���� ���� ������������.\r\n", ch);
      return;
     }
  SET_AF_BATTLE(ch,EAF_BLOCK);
  send_to_char("������, �� ���������� �������� ����� ��������� �����.",ch);
}

ACMD(do_block)
{
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BLOCK))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }
  if (!FIGHTING(ch))
     {send_to_char("�� �� �� � ��� �� ���������� ?\r\n", ch);
      return;
     };
  if (!(IS_NPC(ch)              ||            // ���
        GET_EQ(ch, WEAR_SHIELD) ||            // ���� ���
        IS_IMMORTAL(ch)         ||            // �����������
        GET_GOD_FLAG(ch,GF_GODSLIKE)          // ��������
       ))
      {send_to_char("�� �� ������ ������� ��� ��� ����.\r\n", ch);
       return;
      }
  if (GET_AF_BATTLE(ch,EAF_BLOCK))
     {send_to_char("�� ��� ������������� ����� !\r\n", ch);
      return;
     }
  go_block(ch);
}

/***************** MULTYPARRY PROCEDURES */
void go_multyparry (struct char_data * ch)
{ if (AFF_FLAGGED(ch, AFF_STOPRIGHT) ||
      AFF_FLAGGED(ch, AFF_STOPLEFT)  ||
      AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  SET_AF_BATTLE(ch,EAF_MULTYPARRY);
  send_to_char("�� ���������� ������������ ������� ������.",ch);
}

ACMD(do_multyparry)
{
  struct obj_data *primary=GET_EQ(ch,WEAR_WIELD),
                  *offhand=GET_EQ(ch,WEAR_HOLD);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_MULTYPARRY))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }
  if (!FIGHTING(ch))
     {send_to_char("�� �� �� � ��� �� ���������� ?\r\n", ch);
      return;
     }
  if (!(IS_NPC(ch)                                           || // ���
        (primary && GET_OBJ_TYPE(primary) == ITEM_WEAPON &&
         offhand && GET_OBJ_TYPE(offhand) == ITEM_WEAPON)    || // ��� ������
        IS_IMMORTAL(ch)                                      || // �����������
        GET_GOD_FLAG(ch,GF_GODSLIKE)                            // ��������
       ))
      {send_to_char("�� �� ������ �������� ����� ����������.\r\n", ch);
       return;
      }
  if (GET_AF_BATTLE(ch,EAF_STUPOR))
     {send_to_char("���������� ! �� ���������� �������� ����������.\r\n", ch);
      return;
     }
  go_multyparry(ch);
}




/***************** PARRY PROCEDURES */
void go_parry (struct char_data * ch)
{ if (AFF_FLAGGED(ch, AFF_STOPRIGHT) ||
      AFF_FLAGGED(ch, AFF_STOPLEFT)  ||
      AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  SET_AF_BATTLE(ch,EAF_PARRY);
  send_to_char("�� ���������� ��������� ��������� �����.",ch);
}

ACMD(do_parry)
{
  struct obj_data *primary=GET_EQ(ch,WEAR_WIELD),
                  *offhand=GET_EQ(ch,WEAR_HOLD);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_PARRY))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }
  if (!FIGHTING(ch))
     {send_to_char("�� �� �� � ��� �� ���������� ?\r\n", ch);
      return;
     }
  if (!(IS_NPC(ch)                                           || // ���
        (primary && GET_OBJ_TYPE(primary) == ITEM_WEAPON &&
         offhand && GET_OBJ_TYPE(offhand) == ITEM_WEAPON)    || // ��� ������
        IS_IMMORTAL(ch)                                      || // �����������
        GET_GOD_FLAG(ch,GF_GODSLIKE)                            // ��������
       ))
      {send_to_char("�� �� ������ ��������� ����� ����������.\r\n", ch);
       return;
      }
  if (GET_AF_BATTLE(ch,EAF_STUPOR))
     {send_to_char("���������� ! �� ���������� �������� ����������.\r\n", ch);
      return;
     }
  go_parry(ch);
}

/*************** PROTECT PROCEDURES */
void go_protect (struct char_data * ch, struct char_data * vict)
{
  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  PROTECTING(ch) = vict;
  act("�� ����������� �������� $N1 �� ��������� �����.",FALSE,ch,0,vict,TO_CHAR);
  SET_AF_BATTLE(ch,EAF_PROTECT);
}

ACMD(do_protect)
{
  struct char_data *vict, *tch;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_PROTECT))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {send_to_char("� ��� ��� ������ ��� ������ ������ ?\r\n", ch);
      return;
     };

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
      if (FIGHTING(tch) == vict)
         break;

  if (!tch)
     {act("�� $N3 ����� �� �������.",FALSE,ch,0,vict,TO_CHAR);
      return;
     }

  if (vict == ch)
     {send_to_char("���������� ���������� ����� ��� ���������� �����.",ch);
      return;
     }

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
      {if (FIGHTING(tch) == vict && !may_kill_here(ch, tch))
          return;
      }
  go_protect(ch, vict);
}

/************** TOUCH PROCEDURES */
void go_touch(struct char_data * ch, struct char_data * vict)
{
  if (AFF_FLAGGED(ch, AFF_STOPRIGHT) || AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
      return;
     }
  act("�� ����������� ����������� ��������� ����� $N1.",FALSE,ch,0,vict,TO_CHAR);
  SET_AF_BATTLE(ch,EAF_TOUCH);
  TOUCHING(ch) = vict;
}

ACMD(do_touch)
{ struct obj_data *primary=GET_EQ(ch,WEAR_WIELD) ? GET_EQ(ch,WEAR_WIELD) : GET_EQ(ch,WEAR_BOTHS);
  struct char_data *vict=NULL;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TOUCH))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }
  if (!(IS_IMMORTAL(ch)                    ||        // �����������
        IS_NPC(ch)                         ||        // ���
        GET_GOD_FLAG(ch, GF_GODSLIKE)      ||        // ��������
        !primary                                     // ��� ������
       ))
     {send_to_char("� ��� ������ ����.", ch);
      return;
     }
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
          if (FIGHTING(vict) == ch)
             break;
      if (!vict)
         {if (!FIGHTING(ch))
             {send_to_char("�� �� �� � ��� �� ����������.\r\n", ch);
              return;
             }
          else
             vict = FIGHTING(ch);
         }
     }

  if (ch == vict)
     {send_to_char(GET_NAME(ch),ch);
      send_to_char(", �� ������ �� �������, �������� ����������� �����.\r\n",ch);
      return;
     }
  if (FIGHTING(vict) != ch && FIGHTING(ch) != vict)
     {act("�� �� �� ���������� � $N4.",FALSE,ch,0,vict,TO_CHAR);
      return;
     }
  if (GET_AF_BATTLE(ch,EAF_MIGHTHIT))
     {send_to_char("����������. �� ������������� � ������������ �����.\r\n", ch);
      return;
     }

  if (check_pkill(ch, vict, arg))
     return;

  go_touch(ch, vict);
}

/************** DEVIATE PROCEDURES */
void go_deviate(struct char_data * ch)
{ if (AFF_FLAGGED(ch,AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }
  if (onhorse(ch))
     return;
  SET_AF_BATTLE(ch, EAF_DEVIATE);
  send_to_char("������, �� ����������� ���������� �� ��������� ����� !", ch);
}

ACMD(do_deviate)
{
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_DEVIATE))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (!(FIGHTING(ch)))
     {send_to_char("�� �� ���� �� � ��� �� ���������� !", ch);
      return;
     }

  if (onhorse(ch))
     return;

  if (GET_AF_BATTLE(ch,EAF_DEVIATE))
     {send_to_char("�� � ��� ���������, ��� ������.\r\n",ch);
      return;
     };
  go_deviate(ch);
}

/************** DISARM PROCEDURES */
void go_disarm(struct char_data * ch, struct char_data * vict)
{ int   percent, prob, pos=0;
  struct obj_data *wielded = GET_EQ(vict, WEAR_WIELD) ? GET_EQ(vict, WEAR_WIELD) :
                                                        GET_EQ(vict, WEAR_BOTHS),
                  *helded  = GET_EQ(vict, WEAR_HOLD);

  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  if (!((wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) ||
        (helded  && GET_OBJ_TYPE(helded)  == ITEM_WEAPON)))
     return;
  if (number(1,100) > 30)
     pos = wielded ? (GET_EQ(vict, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_WIELD) : WEAR_HOLD;
  else
     pos = helded  ? WEAR_HOLD : (GET_EQ(vict, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_WIELD);

  if (!pos || !GET_EQ(vict,pos))
     return;

  percent = number(1,skill_info[SKILL_DISARM].max_percent);
  prob    = train_skill(ch, SKILL_DISARM, skill_info[SKILL_DISARM].max_percent, vict);
  if (IS_IMMORTAL(ch)                 ||
      GET_GOD_FLAG(vict,GF_GODSCURSE) ||
      GET_GOD_FLAG(ch,GF_GODSLIKE))
     prob = percent;
  if (IS_IMMORTAL(vict)               ||
      GET_GOD_FLAG(ch,GF_GODSCURSE)   ||
      GET_GOD_FLAG(vict, GF_GODSLIKE))
     prob = 0;


  if (percent > prob || OBJ_FLAGGED(GET_EQ(vict,pos), ITEM_NODISARM))
     {sprintf(buf,"%s�� �� ������ ����������� %s...%s\r\n",
              CCWHT(ch,C_NRM),GET_PAD(vict,3),CCNRM(ch,C_NRM));
      send_to_char(buf,ch);
      // act("�� �� ������ ����������� $N1 !",FALSE,ch,0,vict,TO_CHAR);
      prob = 3;
     }
  else
     {wielded = unequip_char(vict,pos);
      sprintf(buf,"%s�� ����� ������ %s �� ��� %s...%s\r\n",
              CCIBLU(ch,C_NRM),
              wielded->PNames[3],GET_PAD(vict,1),
              CCNRM(ch,C_NRM)
             );
      send_to_char(buf,ch);
      // act("�� ����� ������ $o3 �� ��� $N1.",FALSE,ch,wielded,vict,TO_CHAR);
      act("$n ����� �����$g $o3 �� ����� ���.",FALSE,ch,wielded,vict,TO_VICT);
      act("$n ����� �����$g $o3 �� ��� $N1.",TRUE,ch,wielded,vict,TO_NOTVICT);
      prob = 2;
      if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_ARENA))
         obj_to_char(wielded, vict);
      else
         obj_to_room(wielded, IN_ROOM(vict));
     }

  if (may_pkill(ch,vict) != PC_REVENGE_PC)
     {inc_pkill_group(vict, ch, 1, 0);
     }
  else
     {inc_pkill(vict, ch, 0, 1);
     }
  appear(ch);
  if (IS_NPC(vict) && CAN_SEE(vict,ch) && have_mind(vict) &&
      GET_WAIT(ch) <= 0)
     {set_hit(vict,ch);
     }
  set_wait(ch, prob, FALSE);
}

ACMD(do_disarm)
{ struct char_data *vict=NULL;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_DISARM))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
         vict = FIGHTING(ch);
      else
         {send_to_char("���� ������������� ?\r\n", ch);
          return;
         }
     };

  if (ch == vict)
     {send_to_char(GET_NAME(ch),ch);
      send_to_char(", ���������� ������� \"����� <��������.������>\".\r\n",ch);
      return;
     }

  if (!may_kill_here(ch,vict))
     return;

  if (!((GET_EQ(vict,WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(vict,WEAR_WIELD)) == ITEM_WEAPON) ||
        (GET_EQ(vict,WEAR_HOLD)  && GET_OBJ_TYPE(GET_EQ(vict,WEAR_HOLD))  == ITEM_WEAPON) ||
        (GET_EQ(vict,WEAR_BOTHS) && GET_OBJ_TYPE(GET_EQ(vict,WEAR_BOTHS)) == ITEM_WEAPON)
       ))
     {send_to_char("�� �� ������ ����������� ���������� ��������.\r\n",ch);
      return;
     }

  if (check_pkill(ch, vict, arg))
     return;

  if (IS_IMPL(ch) || !FIGHTING(ch))
     go_disarm(ch, vict);
  else
     if (!used_attack(ch))
        {act("������. �� ����������� ���������� $N3.",FALSE,ch,0,vict,TO_CHAR);
         SET_EXTRA(ch, SKILL_DISARM, vict);
        }
}

/************************** CHOPOFF PROCEDURES */
void go_chopoff(struct char_data * ch, struct char_data * vict)
{
  int percent, prob;

  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
      return;
     }
  if (onhorse(ch))
     return;

  percent = number(1,skill_info[SKILL_CHOPOFF].max_percent);
  prob    = train_skill(ch, SKILL_CHOPOFF, skill_info[SKILL_CHOPOFF].max_percent, vict);
  if (GET_GOD_FLAG(ch,GF_GODSLIKE) ||
      GET_MOB_HOLD(vict) > 0       ||
      GET_GOD_FLAG(vict,GF_GODSCURSE))
     prob = percent;

  if (GET_GOD_FLAG(ch,GF_GODSCURSE)   ||
      GET_GOD_FLAG(vict, GF_GODSLIKE) ||
      on_horse(vict)                  ||
      GET_POS(vict) < POS_FIGHTING    ||
      MOB_FLAGGED(vict, MOB_NOBASH)   ||
      IS_IMMORTAL(vict))
     prob = 0;

  if (percent > prob)
     {sprintf(buf,"%s�� ���������� ������� %s, �� ����� ����...%s\r\n",
              CCWHT(ch,C_NRM),GET_PAD(vict,3),CCNRM(ch,C_NRM));
      send_to_char(buf,ch);
      // act("�� ���������� ������� $N3, �� ����� ����...",FALSE,ch,0,vict,TO_CHAR);
      act("$n �������$u ������� ���, �� ����$g ���$g.",FALSE,ch,0,vict,TO_VICT);
      act("$n �������$u ������� $N3, �� ����$g ���$g.",TRUE,ch,0,vict,TO_NOTVICT);

      GET_POS(ch) = POS_SITTING;
      prob = 3;
     }
  else
     {sprintf(buf,"%s�� ������� ��������, ����� ������ %s �� �����.%s\r\n",
              CCIBLU(ch,C_NRM),GET_PAD(vict,3),CCNRM(ch,C_NRM));
      send_to_char(buf,ch);
      // act("�� ����� �������� $N3, ������ $S �� �����.",FALSE,ch,0,vict,TO_CHAR);
      act("$n ����� ������$q ���, ������ �� ����.",FALSE,ch,0,vict,TO_VICT);
      act("$n ����� ������$q $N3, ������ $S �� �����.",TRUE,ch,0,vict,TO_NOTVICT);
      set_wait(vict, 3, FALSE);
      if (IN_ROOM(ch) == IN_ROOM(vict))
         GET_POS(vict) = POS_SITTING;
      if (IS_HORSE(vict) && on_horse(vict->master))
         horse_drop(vict);
      prob = 1;
     }

  if (may_pkill(ch,vict) != PC_REVENGE_PC)
     {inc_pkill_group(vict, ch, 1, 0);
     }
  else
     {inc_pkill(vict, ch, 0, 1);
     }
  appear(ch);
  if (IS_NPC(vict)         &&
      CAN_SEE(vict,ch)     &&
      have_mind(vict)      &&
      GET_WAIT_STATE(vict) <= 0)
     set_hit(vict,ch);

  set_wait(ch, prob, FALSE);
}


ACMD(do_chopoff)
{
  struct char_data *vict=NULL;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_CHOPOFF))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (onhorse(ch))
     return;

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
    {if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
        vict = FIGHTING(ch);
     else
        {send_to_char("���� �� ����������� ������� ?\r\n", ch);
         return;
        }
    }

  if (vict == ch)
     {send_to_char("�� ������ ��������������� �������� <�����>.\r\n", ch);
      return;
     }

  if (!may_kill_here(ch,vict))
     return;

  if (check_pkill(ch, vict, arg))
     return;

  if (IS_IMPL(ch) || !FIGHTING(ch))
     go_chopoff(ch, vict);
  else
     if (!used_attack(ch))
        {act("������. �� ����������� ������� $N3.",FALSE,ch,0,vict,TO_CHAR);
         SET_EXTRA(ch, SKILL_CHOPOFF, vict);
        }
}

/************************** STUPOR PROCEDURES */
void go_stupor(struct char_data * ch, struct char_data * victim)
{
  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  if (!FIGHTING(ch))
     {SET_AF_BATTLE(ch, EAF_STUPOR);
      hit(ch, victim, TYPE_NOPARRY, 1);
      set_wait(ch, 2, TRUE);
     }
  else
  if (FIGHTING(ch) != victim)
     act("����������. �� �� ���������� � $N4.", FALSE, ch, 0, victim, TO_CHAR);
  else
     {act("�� ����������� �������� $N3.", FALSE, ch, 0, victim, TO_CHAR);
      SET_AF_BATTLE(ch, EAF_STUPOR);
     }
}

ACMD(do_stupor)
{
  struct char_data *vict=NULL;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STUPOR))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
    {if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
        vict = FIGHTING(ch);
     else
        {send_to_char("���� �� ������ �������� ?\r\n", ch);
         return;
        }
    }

  if (vict == ch)
     {send_to_char("�� ������ �������, �������� ���� ����������� �����.\r\n", ch);
      return;
     }

  if (GET_AF_BATTLE(ch, EAF_PARRY))
     {send_to_char("����������. �� ������������� �� ���������� ����.\r\n", ch);
      return;
     }

  if (!may_kill_here(ch, vict))
     return;

  if (check_pkill(ch, vict, arg))
     return;

  go_stupor(ch, vict);
}

/************************** MIGHTHIT PROCEDURES */
void go_mighthit(struct char_data * ch, struct char_data * victim)
{
  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  if (!FIGHTING(ch))
     {SET_AF_BATTLE(ch, EAF_MIGHTHIT);
      hit(ch, victim, TYPE_NOPARRY, 1);
      set_wait(ch, 2, TRUE);
     }
  else
  if (FIGHTING(ch) != victim)
     act("����������. �� �� ���������� � $N4.", FALSE, ch, 0, victim, TO_CHAR);
  else
     {act("�� ����������� ������� ����������� ���� �� $N2.", FALSE, ch, 0, victim, TO_CHAR);
      SET_AF_BATTLE(ch, EAF_MIGHTHIT);
     }
}

ACMD(do_mighthit)
{
  struct char_data *vict=NULL;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_MIGHTHIT))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
         vict = FIGHTING(ch);
      else
         {send_to_char("���� �� ������ ������ ������� ?\r\n", ch);
          return;
         }
     }

  if (vict == ch)
     {send_to_char("�� ������ ������� ����. �� �� � �� �����.\r\n", ch);
      return;
     }

  if (GET_AF_BATTLE(ch, EAF_TOUCH))
     {send_to_char("����������. �� ������������� �� ������� ����������.\r\n", ch);
      return;
     }
  if (!(IS_NPC(ch) || IS_IMMORTAL(ch)) &&
       (GET_EQ(ch,WEAR_BOTHS) || GET_EQ(ch,WEAR_WIELD)  ||
        GET_EQ(ch,WEAR_HOLD)  || GET_EQ(ch,WEAR_SHIELD) ||
        GET_EQ(ch,WEAR_LIGHT)))
     {send_to_char("���� ���������� ������ ��� ������� ����.\r\n", ch);
      return;
     }

  if (!may_kill_here(ch, vict))
     return;

  if (check_pkill(ch, vict, arg))
     return;

  go_mighthit(ch, vict);
}

const char *cstyles[] =
{ "normal",
  "�������",
  "punctual",
  "������",
  "awake",
  "����������",
  "\n"
};

ACMD(do_style)
{
  int tp;

  one_argument(argument, arg);



  if (!*arg)
    {sprintf(buf, "�� ���������� %s ������.\r\n",
             IS_SET(PRF_FLAGS(ch, PRF_PUNCTUAL), PRF_PUNCTUAL) ? "������"  :
             IS_SET(PRF_FLAGS(ch, PRF_AWAKE),    PRF_AWAKE)    ? "����������" : "�������");
     send_to_char(buf, ch);
     return;
    }
  if (((tp = search_block(arg, cstyles, FALSE)) == -1))
     {send_to_char("������: ����� { ������� | ������ | ���������� }\r\n", ch);
      return;
     }
  tp >>= 1;
  if ((tp == 1 && !GET_SKILL(ch,SKILL_PUNCTUAL)) ||
      (tp == 2 && !GET_SKILL(ch,SKILL_AWAKE)))
     {send_to_char("��� ���������� ����� ����� ���.\r\n",ch);
      return;
     }

  REMOVE_BIT(PRF_FLAGS(ch, PRF_PUNCTUAL), PRF_PUNCTUAL);
  REMOVE_BIT(PRF_FLAGS(ch, PRF_AWAKE),    PRF_AWAKE);

  SET_BIT(PRF_FLAGS(ch, PRF_PUNCTUAL), PRF_PUNCTUAL * (tp == 1));
  SET_BIT(PRF_FLAGS(ch, PRF_AWAKE),    PRF_AWAKE    * (tp == 2));

  if (FIGHTING(ch) && !(AFF_FLAGGED(ch, AFF_COURAGE)||
                        AFF_FLAGGED(ch, AFF_DRUNKED)||
                        AFF_FLAGGED(ch, AFF_ABSTINENT)))
     {CLR_AF_BATTLE(ch, EAF_PUNCTUAL);
      CLR_AF_BATTLE(ch, EAF_AWAKE);
      if (tp == 1)
         SET_AF_BATTLE(ch, EAF_PUNCTUAL);
      else
      if (tp == 2)
         SET_AF_BATTLE(ch, EAF_AWAKE);
     }

  sprintf(buf, "�� ������� %s%s%s ����� ���.\r\n",
          CCRED(ch, C_SPR),
          tp == 0 ? "�������" : tp == 1 ?  "������" : "����������",
          CCNRM(ch, C_OFF));
  send_to_char(buf, ch);
  if (!WAITLESS(ch))
     WAIT_STATE(ch, PULSE_VIOLENCE);	
}

/****************** STOPFIGHT ************************************/
ACMD(do_stopfight)
{
  struct char_data *tmp_ch;

  if (!FIGHTING(ch) || IS_NPC(ch))
     {send_to_char("�� �� �� �� � ��� �� ����������.\r\n", ch);
      return;
     }

  if (GET_POS(ch) < POS_FIGHTING)
     {send_to_char("�� ����� ��������� ��������� ����������.\r\n", ch);
      return;
     }

  for (tmp_ch = world[IN_ROOM(ch)].people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
      if (FIGHTING(tmp_ch) == ch)
         break;

  if (tmp_ch)
     {send_to_char("����������, �� ���������� �� ���� �����.\r\n", ch);
      return;
     }
  else
     {stop_fighting(ch,TRUE);
      if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
         WAIT_STATE(ch, PULSE_VIOLENCE);
      send_to_char("�� ��������� �� �����.\r\n", ch);
      act("$n �����$g �� �����.",FALSE,ch,0,0,TO_ROOM);
     }
}

/************** THROW PROCEDURES */
void go_throw(struct char_data * ch, struct char_data * vict)
{ int   percent, prob;
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);

  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  if (!(wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON))
     {send_to_char("��� �� ������ ������� ?\r\n", ch);
      return;
     }

  if (!IS_IMMORTAL(ch) && !OBJ_FLAGGED(wielded, ITEM_THROWING))
     {act("$o �� ������������$A ��� �������.",FALSE,ch,wielded,0,TO_CHAR);
      return;
     }
  percent = number(1,skill_info[SKILL_THROW].max_percent);
  prob    = train_skill(ch, SKILL_THROW, skill_info[SKILL_THROW].max_percent, vict);
  if (IS_IMMORTAL(ch)                 ||
      GET_GOD_FLAG(vict,GF_GODSCURSE) ||
      GET_GOD_FLAG(ch,GF_GODSLIKE))
     prob = percent;
  if (IS_IMMORTAL(vict)               ||
      GET_GOD_FLAG(ch,GF_GODSCURSE)   ||
      GET_GOD_FLAG(vict, GF_GODSLIKE))
     prob = 0;

  // log("Start throw");
  if (percent > prob)
     damage(ch,vict,0,SKILL_THROW + TYPE_HIT, TRUE);
  else
     hit(ch,vict,SKILL_THROW,1);
  // log("[THROW] Start extract weapon...");
  if (GET_EQ(ch,WEAR_WIELD))
     {wielded = unequip_char(ch,WEAR_WIELD);
      if (IN_ROOM(vict) != NOWHERE)
         obj_to_char(wielded,vict);
      else
         obj_to_room(wielded,IN_ROOM(ch));
     }
  // log("[THROW] Miss stop extract weapon...");
  set_wait(ch, 3, TRUE);
  // log("Stop throw");
}

ACMD(do_throw)
{ struct char_data *vict=NULL;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_THROW))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
         {vict = FIGHTING(ch);
         }
     else
         {send_to_char("� ���� ����� ?\r\n", ch);
          return;
         }
     };

  if (ch == vict)
     {send_to_char("�� ������, � �� ������ ������ !\r\n",ch);
      return;
     }

  if (!may_kill_here(ch,vict))
     return;

  if (check_pkill(ch, vict, arg))
     return;

  if (IS_IMPL(ch) || !FIGHTING(ch))
     go_throw(ch, vict);
  else
     if (!used_attack(ch))
        {act("������. �� ����������� ������� ������ � $N3.",FALSE,ch,0,vict,TO_CHAR);
         SET_EXTRA(ch, SKILL_THROW, vict);
        }
}