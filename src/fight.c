/* ************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"

/* Structures */
struct char_data *combat_list = NULL;	/* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External structures */
extern struct index_data *mob_index;
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data  *object_list;
extern struct char_data *character_list;
extern struct obj_data  *obj_proto;
extern int max_exp_gain_npc;	/* see config.c */
extern int max_npc_corpse_time, max_pc_corpse_time;
extern const int material_value[];
extern int supress_godsapply;
extern int r_helled_start_room;

/* External procedures */
char *fread_action(FILE * fl, int nr);
ACMD(do_flee);
int backstab_mult(int level);
int thaco(int ch_class, int level);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);
void battle_affect_update(struct char_data * ch);
void go_throw(struct char_data *ch, struct char_data *vict);
void go_bash(struct char_data *ch, struct char_data *vict);
void go_kick(struct char_data *ch, struct char_data *vict);
void go_rescue(struct char_data *ch, struct char_data *vict, struct char_data *tmp_ch);
void go_parry(struct char_data *ch);
void go_multyparry(struct char_data *ch);
void go_block(struct char_data *ch);
void go_touch(struct char_data *ch, struct char_data *vict);
void go_protect(struct char_data *ch, struct char_data *vict);
void go_chopoff(struct char_data *ch, struct char_data *vict);
void go_disarm(struct char_data *ch, struct char_data *vict);
const char *skill_name(int num);
int  may_kill_here(struct char_data *ch, struct char_data *victim);
int  inc_pk_values(struct char_data *ch, struct char_data *victim, int pkills, int prevenge);
int  dec_pk_values(struct char_data *ch, struct char_data *victim, int pkills, int prevenge);
int  awake_others(struct char_data *ch);
void npc_groupbattle(struct char_data *ch);
int  npc_battle_scavenge(struct char_data *ch);
int  npc_wield(struct char_data *ch);
int  npc_armor(struct char_data *ch);
int  get_room_sky(int rnum);
int  equip_in_metall(struct char_data *ch);
int  max_exp_gain_pc(struct char_data *ch);
int  max_exp_loss_pc(struct char_data *ch);
int  level_exp(int chclass, int chlevel);
int  extra_aco(int class_num, int level);
void change_fighting(struct char_data *ch, int need_stop);
/* local functions */
void perform_group_gain(struct char_data * ch, struct char_data * victim, int members, int koef);
void dam_message(int dam, struct char_data * ch, struct char_data * victim, int w_type);
void appear(struct char_data * ch);
void load_messages(void);
void check_killer(struct char_data * ch, struct char_data * vict);
void make_corpse(struct char_data * ch);
void change_alignment(struct char_data * ch, struct char_data * victim);
void death_cry(struct char_data * ch);
void raw_kill(struct char_data * ch, struct char_data * killer);
void die(struct char_data * ch, struct char_data * killer);
void group_gain(struct char_data * ch, struct char_data * victim);
void solo_gain(struct char_data * ch, struct char_data * victim);
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
void perform_violence(void);
int  compute_armor_class(struct char_data *ch);

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"ударил",    "ударить"},		/* 0 */
  {"ободрал",   "ободрать"},
  {"хлестнул",  "хлестнуть"},
  {"рубанул",   "рубануть"},
  {"укусил",    "укусить"},
  {"огрел",     "огреть"},	/* 5 */
  {"сокрушил",  "сокрушить"},
  {"резанул",   "резануть"},
  {"оцарапал",  "оцарапать"},
  {"подстрелил","подстрелить"},
  {"пырнул",    "пырнуть"},	/* 10 */
  {"уколол",    "уколоть"},
  {"ткнул",     "ткнуть"},
  {"лягнул",    "лягнуть"},
  {"боднул",    "боднуть"},
  {"клюнул",    "клюнуть"},
  {"*","*"},
  {"*","*"},
  {"*","*"},
  {"*","*"}
};

int may_pkill(struct char_data *revenger, struct char_data *killer)
{struct PK_Memory_type *i;

 if (IS_NPC(revenger))
    {if (!AFF_FLAGGED(revenger, AFF_CHARM))
        return (MOB_KILL_VICTIM);
     else
        {if (!(revenger = revenger->master) || IS_NPC(revenger))
           return (MOB_KILL_VICTIM);
        }
    }
 if (IS_NPC(killer))
    {if (!AFF_FLAGGED(killer, AFF_CHARM) && !IS_HORSE(killer))
         return (PC_KILL_MOB);
     else
        {if (!(killer = killer->master) || IS_NPC(killer))
            return (PC_KILL_MOB);
        }
    }
 if (revenger == killer || ROOM_FLAGGED(IN_ROOM(revenger),ROOM_ARENA))
    return (PC_REVENGE_PC);

 for (i = killer->pk_list; i; i = i->next)
     if (i->unique == GET_UNIQUE(revenger))
        {if (i->pkills || i->revenge)
            return (PC_REVENGE_PC);
         else	
         if (i->pthief && i->thieflast + MAX_PTHIEF_LAG > time(NULL))
	    return (PC_REVENGE_PC);
         else
            return (PC_KILL_PC);
        }
 return (PC_KILL_PC);
}

int inc_pkill(struct char_data *victim, struct char_data *killer,
              int pkills, int prevenge)
{int    result, where;

 if (victim == killer)
    return (0);

 if (IS_NPC(victim))
    {if (!AFF_FLAGGED(victim,AFF_CHARM) && !IS_HORSE(victim))
        return (0);
     else
        {where = IN_ROOM(victim);
         if (!(victim = victim->master) || 
	     IS_NPC(victim)             || 
	     IN_ROOM(victim) != where
	    )
            return (0);
        }
    }
    
 if (IS_NPC(killer))
    {if (!AFF_FLAGGED(killer,AFF_CHARM))
        return (0);
     else
        {if (!(killer = killer->master) || IS_NPC(killer))
            return (0);
        }
    }

 if (victim == killer || ROOM_FLAGGED(IN_ROOM(victim),ROOM_ARENA))
    return (0);

 if ((result = inc_pk_values(killer, victim, pkills, prevenge)) != PK_OK)
    {/* Insert code for cleaning great killers */;
    }

 if (pkills > 0)
    save_char(killer, NOWHERE);
 return (1);
}

int dec_pkill(struct char_data * victim, struct char_data *killer)
{int result;

 if (victim == killer)
    return (0);

 if (IS_NPC(killer))
    {if (!AFF_FLAGGED(killer, AFF_CHARM))
        return (0);
     else
     if (!(killer = killer->master) || IS_NPC(killer))
        return (0);
    }	
      	
 if (IS_NPC(victim))
    {if (!AFF_FLAGGED(victim, AFF_CHARM) && !IS_HORSE(victim))
        return (0);
     else
     if (!(victim = victim->master) || IS_NPC(victim))
        return (0);
    }

 if (victim == killer || ROOM_FLAGGED(IN_ROOM(victim),ROOM_ARENA))
    return (0);

 if (may_pkill(victim, killer) != PC_REVENGE_PC)
    return (0);

 result = dec_pk_values(killer, victim, 1, 0);
 save_char(killer, NOWHERE);
 return (1);
}

int  same_group(struct char_data *ch, struct char_data *tch)
{if (!ch || !tch)
    return (FALSE);

 if (IS_NPC(ch)  && ch->master   && !IS_NPC(ch->master))
    ch  = ch->master;
 if (IS_NPC(tch) && tch->master  && !IS_NPC(tch->master))
    tch = tch->master;

 // NPC's always in same group
 if ((IS_NPC(ch) && IS_NPC(tch)) || ch == tch)
    return (TRUE);

 if (!AFF_FLAGGED(ch, AFF_GROUP) ||
     !AFF_FLAGGED(tch, AFF_GROUP))
    return (FALSE);

 if (ch->master == tch ||
     tch->master == ch ||
     (ch->master && ch->master == tch->master)
    )
    return (TRUE);

 return (FALSE);
}

void inc_pkill_group(struct char_data * victim, struct char_data *killer,
                     int pkills, int prevenge)
{struct char_data *tch;
 struct follow_type *f;

 if (same_group(victim, killer))
    return;

 inc_pkill(victim,killer,pkills,prevenge);

 if (IS_NPC(victim))
    {if (victim->master && !IS_NPC(victim->master))
        victim = victim->master;
     else
        return;
    }
 if (!AFF_FLAGGED(victim,AFF_GROUP))
    return;

 tch =  victim->master ? victim->master : victim;
 if (AFF_FLAGGED(tch,AFF_GROUP)      &&
     IN_ROOM(tch) == IN_ROOM(victim) &&
     FIGHTING(tch) != killer         &&
     HERE(tch)
    )
    inc_pkill(tch,killer,pkills,prevenge);
 for (f = tch->followers; f; f = f->next)
     {if (AFF_FLAGGED(f->follower,AFF_GROUP)      &&
          IN_ROOM(f->follower) == IN_ROOM(victim) &&
          FIGHTING(f->follower) != killer         &&
	  HERE(f->follower)
	 )
         inc_pkill(f->follower,killer,pkills,prevenge);
     };
}

int dec_pkill_group(struct char_data * victim, struct char_data *killer)
{struct char_data *tch;
 struct follow_type *f;

 if (same_group(victim, killer))
    return (0);

 /* revenge victim */
 if (dec_pkill(victim, killer))
    return (1);

 /* revenge nothing */
 if (IS_NPC(victim))
    {if (victim->master && !IS_NPC(victim->master))
        victim = victim->master;
     else
        return (0);
    }
 if (!AFF_FLAGGED(victim,AFF_GROUP))
    return (0);

 /* revenge group-members   */
 tch =  victim->master ? victim->master : victim;
 for (f = tch->followers; f; f = f->next)
     {if (AFF_FLAGGED(f->follower,AFF_GROUP) &&
          FIGHTING(f->follower) == killer)
         if (dec_pkill(f->follower,killer))
            return (1);
     };

/* revenge nothing */
 return (0);
}

int  calc_leadership(struct char_data *ch)
{ int    prob, percent;
  struct char_data *leader = 0;

  if (IS_NPC(ch)                                 ||
      !AFF_FLAGGED(ch, AFF_GROUP)                ||
      (!ch->master && !ch->followers)
     )
     return (FALSE);
     
  if (ch->master)
     {if (IN_ROOM(ch) != IN_ROOM(ch->master))
         return (FALSE);
      leader = ch->master;	 
     }
  else
     leader = ch;
     
  if (!GET_SKILL(leader, SKILL_LEADERSHIP))
     return (FALSE);
     
  percent = number(1,101);
  prob    = calculate_skill(leader,SKILL_LEADERSHIP,121,0);
  if (percent > prob)
     return (FALSE);
  else
     return (TRUE);
}



/**** This function return chance of skill */
int calculate_skill (struct char_data * ch,
                     int skill_no, int max_value,
                     struct char_data * vict)
{int    skill_is, percent=0, victim_sav = SAVING_BASIC, victim_modi = 0;

if (skill_no < 1 || skill_no > MAX_SKILLS)
   {// log("ERROR: ATTEMPT USING UNKNOWN SKILL <%d>", skill_no);
    return 0;
   }
if ((skill_is = GET_SKILL(ch,skill_no)) <= 0)
   {return 0;
   }

skill_is += int_app[GET_REAL_INT(ch)].to_skilluse;
switch (skill_no)
  {
case SKILL_BACKSTAB:   /*заколоть*/
     percent = skill_is +
               dex_app[GET_REAL_DEX(ch)].reaction;
     if (awake_others(ch))
        percent -= 50;

     if (vict)
        {if (!CAN_SEE(vict, ch))
            percent += 25;
         if (GET_POS(vict) < POS_FIGHTING)
            percent +=  (20 * (POS_FIGHTING - GET_POS(vict)));
         else
         if (AFF_FLAGGED(vict, AFF_AWARNESS))
            victim_modi -= 30;
         victim_modi +=  size_app[GET_POS_SIZE(vict)].ac;
         victim_modi -=  dex_app_skill[GET_REAL_DEX(vict)].traps;
        }
     break;
case SKILL_BASH:       /*сбить*/
     percent = skill_is +
               size_app[GET_POS_SIZE(ch)].interpolate +
               (dex_app[GET_REAL_DEX(ch)].reaction * 2) +
               (GET_EQ(ch,WEAR_SHIELD) ? weapon_app[MIN(50,MAX(0,GET_OBJ_WEIGHT(GET_EQ(ch,WEAR_SHIELD))))].bashing : 0);
     if (vict)
        {victim_modi -= size_app[GET_POS_SIZE(vict)].interpolate;
         if (GET_POS(vict) < POS_FIGHTING && GET_POS(vict) > POS_SLEEPING)
            victim_modi -= 30;
	 if (GET_AF_BATTLE(vict, EAF_AWAKE))
	    victim_modi -= (GET_SKILL(vict, SKILL_AWAKE) / 2);
	    victim_modi -=dex_app[GET_REAL_DEX(ch)].reaction;
        }
     break;
case SKILL_HIDE:       /*спрятаться*/
     percent = skill_is +
               dex_app_skill[GET_REAL_DEX(ch)].hide -
               size_app[GET_POS_SIZE(ch)].ac;

     if (awake_others(ch))
        percent -= 50;

     if (IS_DARK(IN_ROOM(ch)))
        percent += 25;

     if (SECT(IN_ROOM(ch)) == SECT_CITY)
        percent -= 15;
     else
     if (SECT(IN_ROOM(ch)) == SECT_FOREST)
        percent += 20;
     else
     if (SECT(IN_ROOM(ch)) == SECT_HILLS ||
         SECT(IN_ROOM(ch)) == SECT_MOUNTAIN)
        percent += 15;

     if (vict)
        {if (AWAKE(vict))
            victim_modi -= int_app[GET_REAL_INT(vict)].observation;
        }
     break;
case SKILL_KICK:       /*пнуть*/
     percent = skill_is + GET_REAL_DEX(ch) +  GET_REAL_HR(ch) + (AFF_FLAGGED(ch,AFF_BLESS) ? 2 : 0);
     if (vict)
        {victim_modi += size_app[GET_REAL_SIZE(vict)].interpolate;
	 if (GET_AF_BATTLE(vict, EAF_AWAKE))
	    victim_modi -= (GET_SKILL(vict, SKILL_AWAKE) / 2);
        }
     break;
case SKILL_PICK_LOCK:  /*pick lock*/
     percent = skill_is +
                dex_app_skill[GET_REAL_DEX(ch)].p_locks;
     break;
case SKILL_PUNCH:      /*punch*/
     percent = skill_is;
     break;
case SKILL_RESCUE:     /*спасти*/
     percent     = skill_is +
                   dex_app[GET_REAL_DEX(ch)].reaction;
     victim_modi = 100;
     break;
case SKILL_SNEAK:      /*sneak*/
     percent = skill_is +
               dex_app_skill[GET_REAL_DEX(ch)].sneak;

     if (awake_others(ch))
        percent -= 50;

     if (SECT(IN_ROOM(ch)) == SECT_CITY)
        percent -= 10;
     if (IS_DARK(IN_ROOM(ch)))
        percent += 20;

     if (vict)
        {if (!CAN_SEE(vict, ch))
                victim_modi += 25;
         if (AWAKE(vict))
            {victim_modi -= int_app[GET_REAL_INT(vict)].observation;
            }
        }
     break;
case SKILL_STEAL:      /*steal*/
     percent = skill_is +
               dex_app_skill[GET_REAL_DEX(ch)].p_pocket;

     if (awake_others(ch))
        percent -= 50;

     if (IS_DARK(IN_ROOM(ch)))
        percent += 20;

     if (vict)
        {if (!CAN_SEE(vict, ch))
            victim_modi += 25;
         if (AWAKE(vict))
            {victim_modi -= int_app[GET_REAL_INT(vict)].observation;
             if (AFF_FLAGGED(vict, AFF_AWARNESS))
                victim_modi -= 30;
            }
        }
     break;
case SKILL_TRACK:      /*выследить*/
      percent = skill_is +
                int_app[GET_REAL_INT(ch)].observation;

      if (SECT(IN_ROOM(ch)) == SECT_FOREST ||
          SECT(IN_ROOM(ch)) == SECT_FIELD)
         percent += 10;

      percent = complex_skill_modifier(ch,SKILL_THAC0,GAPPLY_SKILL_SUCCESS,percent);

      if (SECT(IN_ROOM(ch)) == SECT_WATER_SWIM ||
          SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM ||
          SECT(IN_ROOM(ch)) == SECT_FLYING ||
          SECT(IN_ROOM(ch)) == SECT_UNDERWATER ||
          SECT(IN_ROOM(ch)) == SECT_SECRET ||
          ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK))
         percent = 0;


      if (vict)
         {victim_modi += con_app[GET_REAL_CON(vict)].hitp;
          if (AFF_FLAGGED(vict, AFF_NOTRACK) ||
              ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK))
             victim_modi = -100;
         }
      break;

case SKILL_SENSE:
     percent = skill_is +
               int_app[GET_REAL_INT(ch)].observation;

     percent = complex_skill_modifier(ch,SKILL_THAC0,GAPPLY_SKILL_SUCCESS,percent);

     if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK))
        percent = 0;

     if (vict)
         {victim_modi += con_app[GET_REAL_CON(vict)].hitp;
          if (AFF_FLAGGED(vict, AFF_NOTRACK) ||
              ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK))
             victim_modi = -100;
         }
     break;
case SKILL_MULTYPARRY:
case SKILL_PARRY:      /*парировать*/
     percent = skill_is +
               dex_app[GET_REAL_DEX(ch)].reaction;
     if (GET_AF_BATTLE(ch, EAF_AWAKE))
        percent += GET_SKILL(ch, SKILL_AWAKE);

     if (GET_EQ(ch,WEAR_HOLD) && GET_OBJ_TYPE(GET_EQ(ch,WEAR_HOLD)) == ITEM_WEAPON)
        {percent += weapon_app[MAX(0,MIN(50,GET_OBJ_WEIGHT(GET_EQ(ch,WEAR_HOLD))))].parrying;
        }
     victim_modi = 100;
     break;

case SKILL_BLOCK:      /*закрыться щитом*/
     percent = skill_is +
               dex_app[GET_REAL_DEX(ch)].reaction;
     if (GET_AF_BATTLE(ch, EAF_AWAKE))
        percent += GET_SKILL(ch, SKILL_AWAKE);

     break;

case SKILL_TOUCH:      /*захватить противника*/
     percent = skill_is +
               dex_app[GET_REAL_DEX(ch)].reaction +
               size_app[GET_POS_SIZE(vict)].interpolate;

     if (vict)
        {victim_modi -= dex_app[GET_REAL_DEX(vict)].reaction;
         victim_modi -= size_app[GET_POS_SIZE(vict)].interpolate;
        }
     break;

case SKILL_PROTECT:    /*прикрыть грудью*/
     percent = skill_is +
               dex_app[GET_REAL_DEX(ch)].reaction +
               size_app[GET_POS_SIZE(ch)].interpolate;

     victim_modi = 100;
     break;

case SKILL_BOWS:       /*луки*/
     percent = skill_is + dex_app[GET_REAL_DEX(ch)].miss_att;
     break;
case SKILL_BOTHHANDS:  /*двуручники*/
case SKILL_LONGS:      /*длинные лезвия*/
case SKILL_SPADES:     /*копья и пики*/
case SKILL_SHORTS:     /*короткие лезвия*/
case SKILL_CLUBS:      /*палицы и дубины*/
case SKILL_PICK:       /*проникающее*/
case SKILL_NONSTANDART:/*разнообразное оружие*/
case SKILL_AXES:       /*секиры*/
     percent = skill_is;
     break;
case SKILL_SATTACK:    /*атака второй рукой*/
     percent = skill_is;
     break;
case SKILL_LOOKING:    /*приглядеться*/
     percent = skill_is + int_app[GET_REAL_INT(ch)].observation;
     break;
case SKILL_HEARING:    /*прислушаться*/
     percent = skill_is + int_app[GET_REAL_INT(ch)].observation;
     break;
case SKILL_DISARM:
     percent = skill_is +
               dex_app[GET_REAL_DEX(ch)].reaction;
     if (vict)
        {victim_modi -= dex_app[GET_REAL_DEX(ch)].reaction;
         if (GET_EQ(vict,WEAR_BOTHS))
            victim_modi -= 10;
	 if (GET_AF_BATTLE(vict, EAF_AWAKE))
	    victim_modi -= (GET_SKILL(vict, SKILL_AWAKE) / 2);
        }
     break;
case SKILL_HEAL:
     percent = skill_is;
     break;
case SKILL_TURN:
     percent = skill_is;
     break;
case SKILL_ADDSHOT:
     percent = skill_is + dex_app[GET_REAL_DEX(ch)].miss_att;
     break;
case SKILL_CAMOUFLAGE:
     percent = skill_is +
               dex_app_skill[GET_REAL_DEX(ch)].hide -
               size_app[GET_POS_SIZE(ch)].ac;

     if (awake_others(ch))
        percent -= 100;

     if (IS_DARK(IN_ROOM(ch)))
        percent += 15;

     if (SECT(IN_ROOM(ch)) == SECT_CITY)
        percent -= 15;
     else
     if (SECT(IN_ROOM(ch)) == SECT_FOREST)
        percent += 10;
     else
     if (SECT(IN_ROOM(ch)) == SECT_HILLS ||
         SECT(IN_ROOM(ch)) == SECT_MOUNTAIN)
        percent += 5;

     if (vict)
        {if (AWAKE(vict))
            victim_modi -= int_app[GET_REAL_INT(vict)].observation;
        }
     break;
case SKILL_DEVIATE:
     percent = skill_is -
               size_app[GET_POS_SIZE(ch)].ac +
               dex_app[GET_REAL_DEX(ch)].reaction;

     if (equip_in_metall(ch))
        percent -= 10;

     if (vict)
        {victim_modi -= dex_app[GET_REAL_DEX(vict)].miss_att;
        }
     break;
case SKILL_CHOPOFF:
     percent = skill_is +
               dex_app[GET_REAL_DEX(ch)].reaction +
               size_app[GET_POS_SIZE(ch)].ac;

     if (equip_in_metall(ch))
        percent -= 10;

     if (vict)
        {if (!CAN_SEE(vict,ch))
            percent += 10;
         if (GET_POS(vict) < POS_SITTING)
            percent -= 50;
         if (AWAKE(vict) && AFF_FLAGGED(vict, AFF_AWARNESS))
            victim_modi -= 30;
	 if (GET_AF_BATTLE(vict, EAF_AWAKE))
	    victim_modi -= GET_SKILL(ch, SKILL_AWAKE);
         victim_modi -= dex_app[GET_REAL_DEX(vict)].reaction;
         victim_modi -= int_app[GET_REAL_INT(vict)].observation;
        }
     break;
case SKILL_REPAIR:
     percent = skill_is;
     break;
case SKILL_UPGRADE:
     percent = skill_is;
case SKILL_COURAGE:
     percent = skill_is;
     break;
case SKILL_SHIT:
     percent = skill_is;
     break;
case SKILL_MIGHTHIT:
     percent = skill_is +
               size_app[GET_POS_SIZE(ch)].shocking +
               str_app[GET_REAL_STR(ch)].tohit;

     if (vict)
        {victim_modi -=  size_app[GET_POS_SIZE(vict)].shocking;
        }
     break;
case SKILL_STUPOR:
     percent = skill_is +
               str_app[GET_REAL_STR(ch)].tohit;
     if (GET_EQ(ch,WEAR_WIELD))
        percent += weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch,WEAR_WIELD))].shocking;
     else
     if (GET_EQ(ch,WEAR_BOTHS))
        percent += weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch,WEAR_BOTHS))].shocking;

     if (vict)
        {victim_modi +=  con_app[GET_REAL_CON(vict)].critic_saving;
        }
     break;
case SKILL_POISONED:
     percent = skill_is;
     break;
case SKILL_LEADERSHIP:
     percent = skill_is + cha_app[GET_REAL_CHA(ch)].leadership;
     break;
case SKILL_PUNCTUAL:
     percent = skill_is +
               int_app[GET_REAL_INT(ch)].observation;

     if (vict)
        {victim_modi -= int_app[GET_REAL_INT(vict)].observation;
        }
     break;
case SKILL_AWAKE:
     percent = skill_is +
               int_app[GET_REAL_INT(ch)].observation;

     if (vict)
        {victim_modi -= int_app[GET_REAL_INT(vict)].observation;
        }
     break;

case SKILL_IDENTIFY:
     percent = skill_is +
               int_app[GET_REAL_INT(ch)].observation;
     break;

case SKILL_CREATE_POTION:
case SKILL_CREATE_SCROLL:
case SKILL_CREATE_WAND:
     percent = skill_is;
     break;
case SKILL_LOOK_HIDE:
     percent   = skill_is +
                 cha_app[GET_REAL_CHA(ch)].illusive;
     if (vict)
        {if (!CAN_SEE(vict,ch))
            percent += 50;
         else
         if (AWAKE(vict))
            victim_modi -= int_app[GET_REAL_INT(ch)].observation;
        }
     break;
case SKILL_ARMORED:
     percent = skill_is;
     break;
case SKILL_DRUNKOFF:
     percent = skill_is - con_app[GET_REAL_CON(ch)].hitp;
     break;
case SKILL_AID:
     percent = skill_is;
     break;
case SKILL_FIRE:
     percent = skill_is;
     if (get_room_sky(IN_ROOM(ch)) == SKY_RAINING)
        percent -= 50;
     else
     if (get_room_sky(IN_ROOM(ch)) != SKY_LIGHTNING)
        percent -= number(10,25);
case SKILL_HORSE:
     percent = skill_is +
               cha_app[GET_REAL_CHA(ch)].leadership;
default:
     percent = skill_is;
     break;
  }

percent = complex_skill_modifier(ch,skill_no,GAPPLY_SKILL_SUCCESS,percent);
if ((skill_is = number(0,99)) >= 95)
   percent = 0;
else
if (skill_is <= cha_app[GET_REAL_CHA(ch)].morale + GET_MORALE(ch))
   percent = skill_info[skill_no].max_percent;
else
if (vict && general_savingthrow(vict, victim_sav, victim_modi))
   percent = 0;

if (IS_IMMORTAL(ch)               ||      // бессмертный
    GET_GOD_FLAG(ch, GF_GODSLIKE)         // спецфлаг
   )
   percent = MAX(percent,skill_info[skill_no].max_percent);
else
if (GET_GOD_FLAG(ch, GF_GODSCURSE) ||
    (vict && GET_GOD_FLAG(vict, GF_GODSLIKE)))
   percent = 0;
else
if (vict && GET_GOD_FLAG(vict, GF_GODSCURSE))
   percent = MAX(percent,skill_info[skill_no].max_percent);
else
   percent = MIN(MAX(0,percent), max_value);

return (percent);
}

void improove_skill(struct char_data *ch, int skill_no, int success,
                    struct char_data *victim)
{int    skill_is, diff=0, how_many=0, prob, div;

 if (IS_NPC(ch))
    return;

 if (IS_IMMORTAL(ch) ||
     ((!victim ||
       OK_GAIN_EXP(ch, victim)
      )                                            &&
      IN_ROOM(ch) != NOWHERE                       &&
      !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)    &&
// Стрибог
      !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA)       &&
//
      (diff = wis_app[GET_REAL_WIS(ch)].max_learn_l20 * GET_LEVEL(ch) / 20 -
              GET_SKILL(ch,skill_no)
       ) > 0                                       &&
      GET_SKILL(ch,skill_no) < MAX_EXP_PERCENT + GET_REMORT(ch) * 5
     )
    )
    {for (prob = how_many = 0; prob <= MAX_SKILLS; prob++)
         if (GET_SKILL(ch, prob))
            how_many++;
     /* Success - multy by 2 */
     prob  = success ? 20000 : 15000;

     div   =  int_app[GET_REAL_INT(ch)].improove/* + diff */;

     if ((int) GET_CLASS(ch) >= 0 && (int) GET_CLASS(ch) < NUM_CLASSES)
        div += (skill_info[skill_no].k_improove[(int)GET_CLASS(ch)] / 100);

     prob /= (MAX(1,div));

     if ((diff = how_many - wis_app[GET_REAL_WIS(ch)].max_skills) < 0)
        prob  += (5 * diff);
     else
        prob  += (10 * diff);
     prob    += number(1, GET_SKILL(ch,skill_no) * 5);

     skill_is = number(1, MAX(1, prob));

     // if (!IS_NPC(ch))
//        log("Player %s skill '%d' - need to improove %d(%d-%d)",
//            GET_NAME(ch), skill_no, skill_is, div, prob);

     if ( ( victim  && skill_is <= GET_REAL_INT(ch) * GET_LEVEL(victim) / GET_LEVEL(ch) ) ||
          ( !victim && skill_is <= GET_REAL_INT(ch) ))
       {if (success)
           sprintf(buf,"%sВы повысили уровень умения \"%s\".%s\r\n",
                   CCICYN(ch,C_NRM),skill_name(skill_no),CCNRM(ch,C_NRM));
        else
           sprintf(buf,"%sПоняв свои ошибки, Вы повысили уровень умения \"%s\".%s\r\n",
                   CCICYN(ch,C_NRM),skill_name(skill_no),CCNRM(ch,C_NRM));
        send_to_char(buf,ch);
        GET_SKILL(ch,skill_no) += number(1,2);
        if (!IS_IMMORTAL(ch))
           GET_SKILL(ch,skill_no) = MIN(MAX_EXP_PERCENT + GET_REMORT(ch) * 5,GET_SKILL(ch,skill_no));
       }
   }
}


int train_skill (struct char_data * ch, int skill_no, int max_value,
                struct char_data * vict)
{int    percent = 0;

percent = calculate_skill(ch,skill_no,max_value,vict);
if (!IS_NPC(ch))
   {if (skill_no != SKILL_SATTACK   &&
        GET_SKILL(ch, skill_no) > 0 &&
        (!vict  ||
         (IS_NPC(vict) && !MOB_FLAGGED(vict, MOB_PROTECT) &&
          !AFF_FLAGGED(vict, AFF_CHARM) && !IS_HORSE(vict)
         )
        )
       )
      improove_skill(ch,skill_no,percent >= max_value, vict);
   }
else
if (GET_SKILL(ch, skill_no) > 0  &&
    GET_REAL_INT(ch) <= number(0,1000 - 20 * GET_REAL_WIS(ch)) &&
    GET_SKILL(ch, skill_no) < skill_info[skill_no].max_percent
   )
   GET_SKILL(ch,skill_no)++;

return (percent);
}

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_MAGIC))

/* The Fight related routines */

void appear(struct char_data * ch)
{ int appear_msg = AFF_FLAGGED(ch, AFF_INVISIBLE)  ||
                   AFF_FLAGGED(ch, AFF_CAMOUFLAGE) ||
                   AFF_FLAGGED(ch, AFF_HIDE);

  if (affected_by_spell(ch, SPELL_INVISIBLE))
     affect_from_char(ch, SPELL_INVISIBLE);
  if (affected_by_spell(ch, SPELL_HIDE))
     affect_from_char(ch, SPELL_HIDE);
  if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
     affect_from_char(ch, SPELL_CAMOUFLAGE);

  REMOVE_BIT(AFF_FLAGS(ch, AFF_INVISIBLE), AFF_INVISIBLE);
  REMOVE_BIT(AFF_FLAGS(ch, AFF_HIDE),      AFF_HIDE);
  REMOVE_BIT(AFF_FLAGS(ch, AFF_CAMOUFLAGE),AFF_CAMOUFLAGE);

  if (appear_msg)
     {if (GET_LEVEL(ch) < LVL_IMMORT)
         act("$n медленно появил$u из пустоты.", FALSE, ch, 0, 0, TO_ROOM);
      else
         act("Вы почувствовали странное присутствие $n1.",
     	     FALSE, ch, 0, 0, TO_ROOM);
     }	
}


int compute_armor_class(struct char_data *ch)
{
  int armorclass = GET_REAL_AC(ch);

  if (AWAKE(ch))
     {armorclass += dex_app[GET_REAL_DEX(ch)].defensive * 10;
      armorclass += extra_aco((int)GET_CLASS(ch),(int)GET_LEVEL(ch));
     };

  armorclass += (size_app[GET_POS_SIZE(ch)].ac * 10);

  armorclass = MIN(100, armorclass);

  if (GET_AF_BATTLE(ch, EAF_PUNCTUAL))
     armorclass -= 40;

  return (MAX(-100, armorclass));      /* -100 is lowest */
}


void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r")))
     {log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
      exit(1);
     }
  for (i = 0; i < MAX_MESSAGES; i++)
    {fight_messages[i].a_type            = 0;
     fight_messages[i].number_of_attacks = 0;
     fight_messages[i].msg               = 0;
    }


  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*'))
        fgets(chk, 128, fl);

  while (*chk == 'M')
    {fgets(chk, 128, fl);
     sscanf(chk, " %d\n", &type);
     for (i = 0; (i < MAX_MESSAGES)                 &&
                 (fight_messages[i].a_type != type) &&
	             (fight_messages[i].a_type);
	      i++);
     if (i >= MAX_MESSAGES)
        {log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
         exit(1);
        }
     log("BATTLE MESSAGE %d(%d)",i,type);
     CREATE(messages, struct message_type, 1);
     fight_messages[i].number_of_attacks++;
     fight_messages[i].a_type = type;
     messages->next           = fight_messages[i].msg;
     fight_messages[i].msg    = messages;

     messages->die_msg.attacker_msg  = fread_action(fl, i);
     messages->die_msg.victim_msg    = fread_action(fl, i);
     messages->die_msg.room_msg      = fread_action(fl, i);
     messages->miss_msg.attacker_msg = fread_action(fl, i);
     messages->miss_msg.victim_msg   = fread_action(fl, i);
     messages->miss_msg.room_msg     = fread_action(fl, i);
     messages->hit_msg.attacker_msg  = fread_action(fl, i);
     messages->hit_msg.victim_msg    = fread_action(fl, i);
     messages->hit_msg.room_msg      = fread_action(fl, i);
     messages->god_msg.attacker_msg  = fread_action(fl, i);
     messages->god_msg.victim_msg    = fread_action(fl, i);
     messages->god_msg.room_msg      = fread_action(fl, i);
     fgets(chk, 128, fl);
     while (!feof(fl) && (*chk == '\n' || *chk == '*'))
           fgets(chk, 128, fl);
    }

  fclose(fl);
}


void update_pos(struct char_data * victim)
{
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
     GET_POS(victim) = GET_POS(victim);
  else
  if (GET_HIT(victim) > 0)
     GET_POS(victim) = POS_STANDING;
  else
  if (GET_HIT(victim) <= -11)
     GET_POS(victim) = POS_DEAD;
  else
  if (GET_HIT(victim) <= -6)
     GET_POS(victim) = POS_MORTALLYW;
  else
  if (GET_HIT(victim) <= -3)
     GET_POS(victim) = POS_INCAP;
  else
     GET_POS(victim) = POS_STUNNED;

  if (AFF_FLAGGED(victim,AFF_SLEEP) &&
      GET_POS(victim) != POS_SLEEPING
     )
     affect_from_char(victim,SPELL_SLEEP);

  if (on_horse(victim) &&
      GET_POS(victim) < POS_FIGHTING
     )
     horse_drop(get_horse(victim));

  if (IS_HORSE(victim) &&
      GET_POS(victim) < POS_FIGHTING &&
      on_horse(victim->master)
     )
     horse_drop(victim);
}


void check_killer(struct char_data * ch, struct char_data * vict)
{
  /* char buf[256]; */

  if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
     return;
  if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
     return;

  /* SET_BIT(PLR_FLAGS(ch), PLR_KILLER);

     sprintf(buf, "PC Killer bit set on %s for initiating attack on %s at %s.",
	         GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
     mudlog(buf, BRF, LVL_IMMORT, TRUE);
   */
}

void set_battle_pos(struct char_data *ch)
{ switch (GET_POS(ch))
  {case POS_STANDING:
        GET_POS(ch)  = POS_FIGHTING;
        break;
   case POS_RESTING:
   case POS_SITTING:
   case POS_SLEEPING:
        if (GET_WAIT(ch) <= 0 &&
            !GET_MOB_HOLD(ch) &&
            !AFF_FLAGGED(ch, AFF_SLEEP) &&
            !AFF_FLAGGED(ch, AFF_CHARM))
           {if (IS_NPC(ch))
               {act("$n встал$g на ноги.",FALSE,ch,0,0,TO_ROOM);
                GET_POS(ch)  = POS_FIGHTING;
               }
            else
            if (!IS_NPC(ch) && GET_POS(ch) == POS_SLEEPING)
               {act("Вы проснулись и сели.",FALSE,ch,0,0,TO_CHAR);
                act("$n проснул$u и сел$g.",FALSE,ch,0,0,TO_ROOM);
                GET_POS(ch)  = POS_SITTING;
               }
           }
        break;
  }
}

void restore_battle_pos(struct char_data *ch)
{switch (GET_POS(ch))
 {case POS_FIGHTING:
        GET_POS(ch)  = POS_STANDING;
        break;
  case POS_RESTING:
  case POS_SITTING:
  case POS_SLEEPING:
       if (IS_NPC(ch)                  &&
           GET_WAIT(ch) <= 0           &&
           !GET_MOB_HOLD(ch)           &&
           !AFF_FLAGGED(ch, AFF_SLEEP) &&
           !AFF_FLAGGED(ch, AFF_CHARM))
          {act("$n встал$g на ноги.",FALSE,ch,0,0,TO_ROOM);
           GET_POS(ch)  = POS_STANDING;
          }
       break;
 }
 if (AFF_FLAGGED(ch,AFF_SLEEP))
    GET_POS(ch) = POS_SLEEPING;
}

/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data * ch, struct char_data * vict)
{
  if (ch == vict)
     return;

  if (FIGHTING(ch))
     {log("SYSERR: set_fighting(%s->%s) when already fighting(%s)...",
          GET_NAME(ch),GET_NAME(vict),GET_NAME(FIGHTING(ch)));
      // core_dump();
      return;
     }

  if ((IS_NPC(ch)   && MOB_FLAGGED(ch, MOB_NOFIGHT)) ||
      (IS_NPC(vict) && MOB_FLAGGED(ch, MOB_NOFIGHT)))
     return;

  // if (AFF_FLAGGED(ch,AFF_STOPFIGHT))
  //    return;

  ch->next_fighting = combat_list;
  combat_list       = ch;

  if (AFF_FLAGGED(ch, AFF_SLEEP))
     affect_from_char(ch, SPELL_SLEEP);
  FIGHTING(ch) = vict;
  NUL_AF_BATTLE(ch);
  PROTECTING(ch) = 0;
  TOUCHING(ch)   = 0;
  INITIATIVE(ch) = 0;
  BATTLECNTR(ch) = 0;
  SET_EXTRA(ch,0,NULL);
  set_battle_pos(ch);
  /* Set combat style */
  if (!AFF_FLAGGED(ch,AFF_COURAGE) &&
      !AFF_FLAGGED(ch,AFF_DRUNKED) &&
      !AFF_FLAGGED(ch,AFF_ABSTINENT))
     {if (PRF_FLAGGED(ch, PRF_PUNCTUAL))
         SET_AF_BATTLE(ch, EAF_PUNCTUAL);
      else
      if (PRF_FLAGGED(ch, PRF_AWAKE))
         SET_AF_BATTLE(ch, EAF_AWAKE);
     }
  check_killer(ch, vict);
}

/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data * ch, int switch_others)
{struct char_data *temp, *found;

 if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

 REMOVE_FROM_LIST(ch, combat_list, next_fighting);
 ch->next_fighting  = NULL;
 PROTECTING(ch)     = NULL;
 TOUCHING(ch)       = NULL;
 FIGHTING(ch)       = NULL;
 INITIATIVE(ch)     = 0;
 BATTLECNTR(ch)     = 0;
 SET_EXTRA(ch,0,NULL);
 restore_battle_pos(ch);
 NUL_AF_BATTLE(ch);
 // sprintf(buf,"[Stop fighting] %s - %s\r\n",GET_NAME(ch),switch_others ? "switching" : "no switching");
 // send_to_gods(buf);
 /**** switch others *****/

 for (temp = combat_list; temp; temp = temp->next_fighting)
     {if (PROTECTING(temp) == ch)
         {PROTECTING(temp) = NULL;
          CLR_AF_BATTLE(temp, EAF_PROTECT);
         }
      if (TOUCHING(temp) == ch)
         {TOUCHING(temp) = NULL;
          CLR_AF_BATTLE(temp, EAF_TOUCH);
         }
      if (GET_EXTRA_VICTIM(temp) == ch)
         SET_EXTRA(temp, 0, NULL);
      if (GET_CAST_CHAR(temp) == ch)
         SET_CAST(temp, 0, NULL, NULL);
      if (FIGHTING(temp) == ch && switch_others)
         {log("[Stop fighting] Change victim for fighting");
          for (found = combat_list; found; found = found->next_fighting)
              if (found != ch && FIGHTING(found) == temp)
                 {act("Вы переключили свое внимание на $N3.",FALSE,temp,0,found,TO_CHAR);
                  FIGHTING(temp) = found;
                  break;
                 }
          // if (!found)
          //    stop_fighting(temp,FALSE);
         }
     }
 update_pos(ch);
}

void make_arena_corpse(struct char_data * ch, struct char_data *killer)
{struct obj_data *corpse;
 struct extra_descr_data *exdesc;

 corpse              = create_obj();
 corpse->item_number = NOTHING;
 corpse->in_room     = NOWHERE;
 GET_OBJ_SEX(corpse) = SEX_MALE;

 sprintf(buf2, "Останки %s лежат на земле.", GET_PAD(ch,1));
 corpse->description = str_dup(buf2);

 sprintf(buf2, "труп %s", GET_PAD(ch,1));
 corpse->short_description = str_dup(buf2);

 sprintf(buf2, "останки %s", GET_PAD(ch,1));
 corpse->PNames[0] = str_dup(buf2);
 corpse->name      = str_dup(buf2);

 sprintf(buf2, "останков %s", GET_PAD(ch,1));
 corpse->PNames[1] = str_dup(buf2);
 sprintf(buf2, "останкам %s", GET_PAD(ch,1));
 corpse->PNames[2] = str_dup(buf2);
 sprintf(buf2, "останки %s", GET_PAD(ch,1));
 corpse->PNames[3] = str_dup(buf2);
 sprintf(buf2, "останками %s", GET_PAD(ch,1));
 corpse->PNames[4] = str_dup(buf2);
 sprintf(buf2, "останках %s", GET_PAD(ch,1));
 corpse->PNames[5] = str_dup(buf2);

 GET_OBJ_TYPE(corpse)                 = ITEM_CONTAINER;
 GET_OBJ_WEAR(corpse)                 = ITEM_WEAR_TAKE;
 GET_OBJ_EXTRA(corpse,ITEM_NODONATE)  = ITEM_NODONATE;
 GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
 GET_OBJ_VAL(corpse, 2) = IS_NPC(ch) ? GET_MOB_VNUM(ch) : -1;
 GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
 GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch);
 GET_OBJ_RENT(corpse)   = 100000;
 GET_OBJ_TIMER(corpse) = max_pc_corpse_time * 2;
 CREATE(exdesc, struct extra_descr_data, 1);
 exdesc->keyword     = strdup(corpse->PNames[0]);
 if (killer)
    sprintf(buf,"Убит на арене %s.\r\n",GET_PAD(killer,4));
 else
    sprintf(buf,"Умер на арене.\r\n");
 exdesc->description = strdup(buf);
 exdesc->next = corpse->ex_description;
 corpse->ex_description = exdesc;
 obj_to_room(corpse, ch->in_room);
}





void make_corpse(struct char_data * ch)
{struct obj_data *corpse, *o, *obj;
 struct obj_data *money;
 int i;

 if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_CORPSE))
    return;

 corpse              = create_obj();
 corpse->item_number = NOTHING;
 corpse->in_room     = NOWHERE;
 GET_OBJ_SEX(corpse) = SEX_MALE;

 sprintf(buf2, "Труп %s лежит на земле.", GET_PAD(ch,1));
 corpse->description = str_dup(buf2);

 sprintf(buf2, "труп %s", GET_PAD(ch,1));
 corpse->short_description = str_dup(buf2);

 sprintf(buf2, "труп %s", GET_PAD(ch,1));
 corpse->PNames[0] = str_dup(buf2);
 corpse->name      = str_dup(buf2);

 sprintf(buf2, "трупа %s", GET_PAD(ch,1));
 corpse->PNames[1] = str_dup(buf2);
 sprintf(buf2, "трупу %s", GET_PAD(ch,1));
 corpse->PNames[2] = str_dup(buf2);
 sprintf(buf2, "труп %s", GET_PAD(ch,1));
 corpse->PNames[3] = str_dup(buf2);
 sprintf(buf2, "трупом %s", GET_PAD(ch,1));
 corpse->PNames[4] = str_dup(buf2);
 sprintf(buf2, "трупе %s", GET_PAD(ch,1));
 corpse->PNames[5] = str_dup(buf2);

 GET_OBJ_TYPE(corpse)   = ITEM_CONTAINER;
 GET_OBJ_WEAR(corpse)   = ITEM_WEAR_TAKE;
 GET_OBJ_EXTRA(corpse,ITEM_NODONATE)  |= ITEM_NODONATE;
 GET_OBJ_EXTRA(corpse,ITEM_NOSELL)    |= ITEM_NOSELL;
 GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
 GET_OBJ_VAL(corpse, 2) = IS_NPC(ch) ? GET_MOB_VNUM(ch) : -1;
 GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
 GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
 GET_OBJ_RENT(corpse) = 100000;
 if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = max_npc_corpse_time * 2;
 else
    GET_OBJ_TIMER(corpse) = max_pc_corpse_time * 2;

 /* transfer character's inventory to the corpse */
 corpse->contains = ch->carrying;
 for (o = corpse->contains; o != NULL; o = o->next_content)
     {
      o->in_obj = corpse;
     }
 object_list_new_owner(corpse, NULL);

 /* transfer character's equipment to the corpse */
 for (i = 0; i < NUM_WEARS; i++)
     if (GET_EQ(ch, i))
        {remove_otrigger(GET_EQ(ch, i), ch);
         obj = unequip_char(ch, i);
         obj_to_obj(obj, corpse);
        }

 /* transfer gold */
 if (GET_GOLD(ch) > 0)
   {/* following 'if' clause added to fix gold duplication loophole */
    if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc))
       {money = create_money(GET_GOLD(ch));
        obj_to_obj(money, corpse);
       }
    GET_GOLD(ch) = 0;
   }

 ch->carrying = NULL;
 IS_CARRYING_N(ch) = 0;
 IS_CARRYING_W(ch) = 0;

 obj_to_room(corpse, ch->in_room);
}


/* When ch kills victim */
void change_alignment(struct char_data * ch, struct char_data * victim)
{
  /*
   * new alignment change algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast.
   */
  GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}



void death_cry(struct char_data * ch)
{
  int door;

  act("Кровушка стынет в жилах от предсмертного крика $n1.", FALSE, ch, 0, 0, TO_ROOM);

  for (door = 0; door < NUM_OF_DIRS; door++)
      if (CAN_GO(ch, door))
         send_to_room("Кровушка стынет в жилах от чьего-то предсмертного крика.\r\n",
                      world[ch->in_room].dir_option[door]->to_room, TRUE
                     );
}



void raw_kill(struct char_data * ch, struct char_data * killer)
{ struct char_data *hitter;
  struct affected_type *af, *naf;
  int to_room;

  if (FIGHTING(ch))
     stop_fighting(ch,TRUE);

  for (hitter = combat_list; hitter; hitter = hitter->next_fighting)
      if (FIGHTING(hitter) == ch)
         WAIT_STATE(hitter, 0);

  supress_godsapply = TRUE;
  for (af = ch->affected; af; af = naf)
      {naf = af->next;
       if (!IS_SET(af->battleflag, AF_DEADKEEP))
          affect_remove(ch, af);
      }
  supress_godsapply = FALSE;
  affect_total(ch);

  if (killer)
     {if (death_mtrigger(ch, killer))
         {if (IN_ROOM(ch) != NOWHERE)
	     death_cry(ch);
	 }
     }
  else
     {death_cry(ch);
     }

  if (IN_ROOM(ch) != NOWHERE)
     {if (!IS_NPC(ch) && ROOM_FLAGGED(IN_ROOM(ch),ROOM_ARENA))
         {make_arena_corpse(ch,killer);
	  change_fighting(ch,TRUE);
          FORGET_ALL(ch);
	  GET_HIT(ch) = 1;
	  GET_POS(ch) = POS_SITTING;
          char_from_room(ch);
          if ((to_room = real_room(GET_LOADROOM(ch))) == NOWHERE)
	     {SET_BIT(PLR_FLAGS(ch, PLR_HELLED), PLR_HELLED);
	      HELL_DURATION(ch) = time(0) + 6;
              to_room = r_helled_start_room;
	     }
          char_to_room(ch,to_room);
          look_at_room(ch,to_room);
          act("$n со стонами упал$g с небес...",FALSE,ch,0,0,TO_ROOM);
         }
      else
         {make_corpse(ch);
          if (!IS_NPC(ch))
             {FORGET_ALL(ch);
              if (killer && IS_NPC(killer) && MEMORY(killer))
                 forget(killer,ch);
              /*
              for (hitter = character_list; hitter && IS_NPC(hitter) && MEMORY(hitter); hitter = hitter->next)
                  forget(hitter, ch);
               */
             }
          extract_char(ch,TRUE);
         }
     }
}

void die(struct char_data * ch, struct char_data * killer)
{ int    is_pk = 0, dec_exp;
  struct PK_Memory_type *pk, *best_pk=NULL;
  struct char_data *master=NULL;
  struct follow_type *f;

  if (IS_NPC(ch) || !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA))
     {if (!(IS_NPC(ch) || IS_IMMORTAL(ch) || GET_GOD_FLAG(ch,GF_GODSLIKE)))
         {dec_exp = number(GET_EXP(ch)/100, GET_EXP(ch) / 20) +
                    (level_exp(GET_CLASS(ch),GET_LEVEL(ch)+1) - level_exp(GET_CLASS(ch),GET_LEVEL(ch))) / 3;
          gain_exp(ch, -dec_exp);
          sprintf(buf,"Вы потеряли %d %s опыта.\r\n",dec_exp,desc_count(dec_exp,WHAT_POINT));
         }

      /* calculate MobKill */
      if (IS_NPC(ch) && killer)
         {if (IS_NPC(killer) &&
	      AFF_FLAGGED(killer, AFF_CHARM) &&
	      killer->master
	     )
             master = killer->master;
          else
          if (!IS_NPC(killer))
             master = killer;
	  if (master)
	     {struct char_data *leader = master->master ? master->master : master,
	                       *maxer  = master;
              if (AFF_FLAGGED(master, AFF_GROUP))
	         {int g_count = 0;
                  for (f = leader->followers; f; f = f->next)
                      if (!IS_NPC(f->follower) &&
		          AFF_FLAGGED(f->follower, AFF_GROUP) &&
                          f->follower->in_room == ch->in_room
			 )
			 {if (!number(0,++g_count))
			     maxer = f->follower;
			 }
	          }
              inc_kill_vnum(maxer, GET_MOB_VNUM(ch), 1);
             }
	  }
      /* train LEADERSHIP */
      if (IS_NPC(ch)                                     &&
          !IS_NPC(killer)                                &&
          AFF_FLAGGED(killer,AFF_GROUP)                  &&
          killer->master                                 &&
          GET_SKILL(killer->master,SKILL_LEADERSHIP) > 0 &&
          IN_ROOM(killer) == IN_ROOM(killer->master)
         )
         improove_skill(killer->master,SKILL_LEADERSHIP,number(0,1),ch);

      if (!IS_NPC(ch) && killer)
         {/* decrease LEADERSHIP */
          if (IS_NPC(killer)            &&
              AFF_FLAGGED(ch,AFF_GROUP) &&
              ch->master                &&
             IN_ROOM(ch) == IN_ROOM(ch->master))
             {if (GET_SKILL(ch->master, SKILL_LEADERSHIP) > 1)
                 GET_SKILL(ch->master, SKILL_LEADERSHIP)--;
             }
          is_pk = dec_pkill_group(killer,ch);
          for (pk = ch->pk_list; pk; pk = pk->next)
              {if (pk->revenge)
                  {if (!best_pk)
                      {if (time(NULL) - pk->revengelast <= MAX_REVENGE_LAG &&
                           pk->revenge > 0)
                          best_pk = pk;
                      }
                   else
                   if (best_pk->revengelast < pk->revengelast &&
                       pk->revenge > 0)
                      best_pk = pk;
                  }
               pk->revenge     = 0;
               pk->revengelast = time(NULL);
              }
          if (!is_pk && best_pk)
             best_pk->revenge = MAX_REVENGE;
         }
     }
  raw_kill(ch, killer);
  // if (killer)
  //   log("Killer lag is %d", GET_WAIT(killer));
}

int  get_extend_exp(int exp, struct char_data *ch, struct char_data *victim)
{int base,diff;
 int koef;

 if (!IS_NPC(victim) || IS_NPC(ch))
    return (exp);

 for (koef = 100, base = 0, diff = get_kill_vnum(ch,GET_MOB_VNUM(victim));
      base < diff && koef > 5; base++, koef = koef * 95 / 100);

 log("[Expierence] Mob %s - %d %d(%d) %d",GET_NAME(victim),exp,base,diff,koef);
 exp = exp * MAX(5, koef) / 100;

 // if (!(base = victim->mob_specials.MaxFactor))
 //    return (exp);
 //
 // if ((diff = get_kill_vnum(ch,GET_MOB_VNUM(victim)) - base) <= 0)
 //    return (exp);
 // exp = exp * base / (base+diff);

 return (exp);
}

void perform_group_gain(struct char_data * ch,
                        struct char_data * victim,
			int members, int koef)
{
  int exp;

  if (IS_NPC(ch) || !OK_GAIN_EXP(ch,victim))
     {send_to_char("Ваше деяние никто не оценил.\r\n",ch);
      return;
     }

  exp = GET_EXP(victim) / MAX(members,1);

  if (IS_NPC(ch))
     exp = MIN(max_exp_gain_npc, exp);
  else
     exp = MIN(max_exp_gain_pc(ch), get_extend_exp(exp, ch, victim));
  exp   = exp * koef / 100;

  if (!IS_NPC(ch))
     exp = MIN(max_exp_gain_pc(ch),exp);
  exp   = MAX(1,exp);

  if (exp > 1)
     {sprintf(buf2, "Ваш опыт повысился на %d %s.\r\n", exp, desc_count(exp, WHAT_POINT));
      send_to_char(buf2, ch);
     }
  else
    send_to_char("Ваш опыт повысился всего-лишь на маленькую единичку.\r\n", ch);

  gain_exp(ch, exp);
  change_alignment(ch, victim);
}


void group_gain(struct char_data * ch, struct char_data * victim)
{
  int    tot_members, koef = 100, maxlevel, minlevel;
  struct char_data *k;
  struct follow_type *f;

  maxlevel = minlevel = GET_LEVEL(ch);

  if (!(k = ch->master))
     k = ch;

  if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
     {tot_members = 1;
      maxlevel = minlevel = GET_LEVEL(k);
     }
  else
     tot_members = 0;

  for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
          f->follower->in_room == ch->in_room
	 )
         {tot_members++;
          if (!IS_NPC(f->follower))
             {minlevel = MIN(minlevel, GET_LEVEL(f->follower));
              maxlevel = MAX(maxlevel, GET_LEVEL(f->follower));
             }
         }

  /* prevent illegal xp creation when killing players */
  if (maxlevel - minlevel > 5)
     koef -= 50;

  if (calc_leadership(ch))
     koef += 20;

  if (AFF_FLAGGED(k, AFF_GROUP) && k->in_room == ch->in_room)
     perform_group_gain(k, victim, tot_members, koef);

  for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
          f->follower->in_room == ch->in_room
	 )
         perform_group_gain(f->follower, victim, tot_members, koef);
}


void solo_gain(struct char_data * ch, struct char_data * victim)
{
  int exp;

  if (IS_NPC(ch) || !OK_GAIN_EXP(ch, victim))
     {send_to_char("Ваше деяние никто не оценил.\r\n",ch);
      return;
     }

  if (IS_NPC(ch))
     {exp  = MIN(max_exp_gain_npc, GET_EXP(victim));
      exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
     }
  else
     {exp = get_extend_exp(GET_EXP(victim), ch, victim);
      exp = MIN(max_exp_gain_pc(ch), exp);
     };

  if (!IS_NPC(ch))
     exp = MIN(max_exp_gain_pc(ch),exp);
  exp = MAX(1,exp);

  if (exp > 1)
     {sprintf(buf2, "Ваш опыт повысился на %d %s.\r\n", exp, desc_count(exp, WHAT_POINT));
      send_to_char(buf2, ch);
     }
  else
    send_to_char("Ваш опыт повысился всего лишь на маленькую единичку.\r\n", ch);

  gain_exp(ch, exp);
  change_alignment(ch, victim);
}


char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
  static char buf[256];
  char *cp = buf;

  for (; *str; str++)
   {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
                        }
                     }
    else
       *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}


/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data * ch, struct char_data * victim,
		      int w_type)
{
  char *buf;
  int msgnum;

  static struct dam_weapon_type
  {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_weapons[] = {

    /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

    {
      "$n попытал$u #W $N3, но промахнул$u.",	/* 0: 0     */
      "Вы попытались #W $N3, но промахнулись.",
      "$n попытал$u #W Вас, но промахнул$u."
    },

    {
      "$n легонько #w$g $N3.",	/*  1..2  */
      "Вы легонько #wи $N3.",
      "$n легонько #w$g Вас."
    },

    {
      "$n слегка #w$g $N3.",	/*  3..4  */
      "Вы слегка #wи $N3.",
      "$n слегка #w$g Вас."
    },

    {
      "$n #w$g $N3.",	/*  5..6  */
      "Вы #wи $N3.",
      "$n #w$g Вас."
    },

    {
      "$n #w$g $N3.",	/* 7..10  */
      "Вы #wи $N3.",
      "$n #w$g Вас."
    },

    {
      "$n сильно #w$g $N3.",	/* 11..14  */
      "Вы сильно #wи $N3.",
      "$n сильно #w$g Вас."
    },

    {
      "$n очень сильно #w$g $N3.",	/*  15..18  */
      "Вы очень сильно #wи $N3.",
      "$n очень сильно #w$g Вас."
    },

    {
      "$n чрезвычайно сильно #w$g $N3.",	/*  19..22  */
      "Вы чрезвычайно сильно #wи $N3.",
      "$n чрезвычайно сильно #w$g Вас."
    },

    {
      "$n БОЛЬНО #w$g $N3.",	/*  23..30  */
      "Вы БОЛЬНО #wи $N3.",
      "$n БОЛЬНО #w$g Вас."
    },


    {
      "$n ОЧЕНЬ БОЛЬНО #w$g $N3.",	/*    31..40  */
      "Вы ОЧЕНЬ БОЛЬНО #wи $N3.",
      "$n ОЧЕНЬ БОЛЬНО #w$g Вас."
    },

    {
      "$n СМЕРТЕЛЬНО #w$g $N3.",	/* > 40  */
      "Вы СМЕРТЕЛЬНО #wи $N3.",
      "$n СМЕРТЕЛЬНО #w$g Вас."
    }
  };


  if (w_type >= TYPE_HIT && w_type < TYPE_MAGIC)
     w_type -= TYPE_HIT; /* Change to base of table with text */
  else
     w_type  = TYPE_HIT;

  if (dam == 0)		msgnum = 0;
  else if (dam <= 2)    msgnum = 1;
  else if (dam <= 4)    msgnum = 2;
  else if (dam <= 6)    msgnum = 3;
  else if (dam <= 10)   msgnum = 4;
  else if (dam <= 14)   msgnum = 5;
  else if (dam <= 19)   msgnum = 6;
  else if (dam <= 25)   msgnum = 7;
  else if (dam <= 35)   msgnum = 8;
  else if (dam <= 50)   msgnum = 9;
  else			        msgnum = 10;
  /* damage message to onlookers */
  buf = replace_string(dam_weapons[msgnum].to_room,
	                   attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);

  /* damage message to damager */
  if (dam)
     send_to_char(CCIYEL(ch, C_CMP), ch);
  else
     send_to_char(CCYEL(ch, C_CMP), ch);
  buf = replace_string(dam_weapons[msgnum].to_char,
	                   attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_CHAR);
  send_to_char(CCNRM(ch, C_CMP), ch);

  /* damage message to damagee */
  if (dam)
     send_to_char(CCIRED(victim, C_CMP), victim);
  else
     send_to_char(CCIRED(victim, C_CMP), victim);
  buf = replace_string(dam_weapons[msgnum].to_victim,
	                   attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
  send_to_char(CCNRM(victim, C_CMP), victim);
}


/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
#define DUMMY_KNIGHT 390
#define DUMMY_SHIELD 391
#define DUMMY_WEAPON 392

int skill_message(int dam, struct char_data * ch, struct char_data * vict,
 	          int attacktype)
{
  int    i, j, nr, weap_i;
  struct message_type *msg;
  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD) ? GET_EQ(ch, WEAR_WIELD) : GET_EQ(ch, WEAR_BOTHS);

  // log("[SKILL MESSAGE] Message for skill %d",attacktype);
  for (i = 0; i < MAX_MESSAGES; i++)
      {if (fight_messages[i].a_type == attacktype)
          {
           nr = dice(1, fight_messages[i].number_of_attacks);
           // log("[SKILL MESSAGE] %d(%d)",fight_messages[i].number_of_attacks,nr);
           for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	           msg = msg->next;

           switch (attacktype)
           {case SKILL_BACKSTAB+TYPE_HIT:
                 if (!(weap=GET_EQ(ch, WEAR_WIELD))  && (weap_i = real_object(DUMMY_KNIGHT)) >= 0)
                    weap = (obj_proto + weap_i);
		 break;
            case SKILL_THROW+TYPE_HIT:
                 if (!(weap=GET_EQ(ch, WEAR_WIELD))  && (weap_i = real_object(DUMMY_KNIGHT)) >= 0)
                    weap = (obj_proto + weap_i);
		 break;
            case SKILL_BASH+TYPE_HIT:
                 if (!(weap=GET_EQ(ch, WEAR_SHIELD)) && (weap_i = real_object(DUMMY_SHIELD)) >= 0)
                    weap = (obj_proto + weap_i);
		 break;
            case TYPE_HIT:
                 weap = NULL;
		 break;
            default:
                 if (!weap && (weap_i = real_object(DUMMY_WEAPON)) >= 0)
                    weap = (obj_proto + weap_i);
           }
	
           if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMMORT))
              {	switch (attacktype)
                {case SKILL_BACKSTAB+TYPE_HIT:
                 case SKILL_THROW+TYPE_HIT:
                 case SKILL_BASH+TYPE_HIT:
                 case SKILL_KICK+TYPE_HIT:
                      send_to_char(CCWHT(ch, C_CMP), ch);
                      break;
                 default:
                      send_to_char(CCYEL(ch, C_CMP), ch);
                      break;
                }
                act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
                send_to_char(CCNRM(ch, C_CMP), ch);

                act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
                act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
              }
           else
           if (dam != 0)
              {if (GET_POS(vict) == POS_DEAD)
                  {send_to_char(CCIYEL(ch, C_CMP), ch);
                   act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
                   send_to_char(CCNRM(ch, C_CMP), ch);

                   send_to_char(CCIRED(vict, C_CMP), vict);
                   act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
                   send_to_char(CCNRM(vict, C_CMP), vict);
                   act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
                  }
               else
                  {send_to_char(CCIYEL(ch, C_CMP), ch);
                   act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
                   send_to_char(CCNRM(ch, C_CMP), ch);
                   send_to_char(CCIRED(vict, C_CMP), vict);
                   act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
                   send_to_char(CCNRM(vict, C_CMP), vict);
                   act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
                  }
              }
           else
           if (ch != vict)
              {	/* Dam == 0 */
                switch (attacktype)
                {case SKILL_BACKSTAB+TYPE_HIT:
                 case SKILL_THROW+TYPE_HIT:
                 case SKILL_BASH+TYPE_HIT:
                 case SKILL_KICK+TYPE_HIT:
                      send_to_char(CCWHT(ch, C_CMP), ch);
                      break;
                 default:
                      send_to_char(CCYEL(ch, C_CMP), ch);
                      break;
                }
                act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
                send_to_char(CCNRM(ch, C_CMP), ch);

                send_to_char(CCRED(vict, C_CMP), vict);
                act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
                send_to_char(CCNRM(vict, C_CMP), vict);

                act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
              }
          return (1);
      }
  }
  return (0);
}


/* Alterate equipment
 *
 */
void alterate_object(struct obj_data *obj, int dam, int chance)
{if (!obj)
    return;
 dam = number (0, dam * (material_value[GET_OBJ_MATER(obj)] + 30) /
                  MAX(1, GET_OBJ_MAX(obj) *
		         (IS_OBJ_STAT(obj,ITEM_NODROP) ? 5 : IS_OBJ_STAT(obj,ITEM_BLESS) ? 15 : 10) *
			 (GET_OBJ_SKILL(obj) == SKILL_BOWS ? 3 : 1)
		      )
              );

 if (dam > 0 && chance >= number(1, 100))
    {if ((GET_OBJ_CUR(obj) -= dam) <= 0)
        {if (obj->worn_by)
            act("$o рассыпал$U, не выдержав повреждений.", FALSE, obj->worn_by, obj, 0, TO_CHAR);
         else
         if (obj->carried_by)
            act("$o рассыпал$U, не выдержав повреждений.", FALSE, obj->carried_by, obj, 0, TO_CHAR);
         extract_obj(obj);
        }
    }
}



void alt_equip(struct char_data *ch, int pos, int dam, int chance)
{
 // calculate chance if
 if (pos == NOWHERE)
    {pos = number(0,100);
     if (pos < 3)
        pos = WEAR_FINGER_R + number(0,1);
     else
     if (pos < 6)
        pos = WEAR_NECK_1 + number(0,1);
     else
     if (pos < 20)
        pos = WEAR_BODY;
     else
     if (pos < 30)
        pos = WEAR_HEAD;
     else
     if (pos < 45)
        pos = WEAR_LEGS;
     else
     if (pos < 50)
        pos = WEAR_FEET;
     else
     if (pos < 58)
        pos = WEAR_HANDS;
     else
     if (pos < 66)
        pos = WEAR_ARMS;
     else
     if (pos < 76)
        pos = WEAR_SHIELD;
     else
     if (pos < 86)
        pos = WEAR_ABOUT;
     else
     if (pos < 90)
        pos = WEAR_WAIST;
     else
     if (pos < 94)
        pos = WEAR_WRIST_R + number(0,1);
     else
        pos = WEAR_HOLD;
    }

 if (pos <= 0 || pos > WEAR_BOTHS || !GET_EQ(ch,pos) || dam < 0)
    return;
 alterate_object(GET_EQ(ch,pos), dam, chance);
}

/*  Global variables for critical damage */
int was_critic = FALSE;
int dam_critic = 0;

void haemorragia(struct char_data *ch, int percent)
{struct affected_type af[3];
 int    i;

 af[0].type      = SPELL_HAEMORRAGIA;
 af[0].location  = APPLY_HITREG;
 af[0].modifier  = -percent;
 af[0].duration  = pc_duration(ch,number(1, 10+con_app[GET_REAL_CON(ch)].critic_saving),0,0,0,0);
 af[0].bitvector = 0;
 af[0].battleflag= 0;
 af[1].type      = SPELL_HAEMORRAGIA;
 af[1].location  = APPLY_MOVEREG;
 af[1].modifier  = -percent;
 af[1].duration  = af[0].duration;
 af[1].bitvector = 0;
 af[1].battleflag= 0;
 af[2].type      = SPELL_HAEMORRAGIA;
 af[2].location  = APPLY_MANAREG;
 af[2].modifier  = -percent;
 af[2].duration  = af[0].duration;
 af[2].bitvector = 0;
 af[2].battleflag= 0;

 for (i = 0; i < 3; i++)
     affect_join(ch, &af[i], TRUE, FALSE, TRUE, FALSE);
}


int compute_critical(struct char_data *ch, struct char_data *victim, int dam)
{char   *to_char=NULL, *to_vict=NULL;
 struct affected_type af[4];
 struct obj_data *obj;
 int    i, unequip_pos = 0;

 for (i = 0; i < 4; i++)
     {af[i].type       = 0;
      af[i].location   = APPLY_NONE;
      af[i].bitvector  = 0;
      af[i].modifier   = 0;
      af[i].battleflag = 0;
      af[i].duration   = pc_duration(victim,2,0,0,0,0);
     }

 was_critic = FALSE;
 switch (number(1,10))
 {case 1: case 2: case 3: case 4: // FEETS
          switch (dam_critic)
          {case 1: case 2: case 3:
                   // Nothing
                   return dam;
           case 5: // Hit genus, victim bashed, speed/2
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   dam += (GET_REAL_MAX_HIT(victim)/10);
           case 4: // victim bashed
                   GET_POS(victim) = POS_SITTING;
                   WAIT_STATE(victim, 2*PULSE_VIOLENCE);
                   to_char = "повалило $N3 на землю";
                   to_vict = "повредило Вам колено";
                   break;
           case 6: // foot damaged, speed/2
                   dam += (GET_REAL_MAX_HIT(victim)/9);
                   to_char = "замедлило движения $N1";
                   to_vict = "сломало Вам лодыжку";
                   SET_AF_BATTLE(victim,EAF_SLOW);
                   break;
           case 7: case 9: // armor damaged else foot damaged, speed/4
                   if (GET_EQ(victim,WEAR_LEGS))
                      alt_equip(victim,WEAR_LEGS,100,100);
                   else
                      {dam    += (GET_REAL_MAX_HIT(victim)/7);
                       to_char = "замедлило движения $N1";
                       to_vict = "сломало Вам ногу";
                       af[0].type      = SPELL_BATTLE;
                       af[0].bitvector = AFF_NOFLEE;
                       SET_AF_BATTLE(victim, EAF_SLOW);
                      }
                   break;
            case 8: // femor damaged, no speed
                    dam    += (GET_REAL_MAX_HIT(victim)/8);
                    to_char = "сильно замедлило движения $N1";
                    to_vict = "сломало Вам бедро";
                    af[0].type      = SPELL_BATTLE;
                    af[0].bitvector = AFF_NOFLEE;
                    haemorragia(victim,20);
                    SET_AF_BATTLE(victim, EAF_SLOW);
                    break;
            case 10: // genus damaged, no speed, -2HR
                    dam    += (GET_REAL_MAX_HIT(victim)/10);
                    to_char = "сильно замедлило движения $N1";
                    to_vict = "раздробило Вам колено";
                    af[0].type      = SPELL_BATTLE;
                    af[0].location  = APPLY_HITROLL;
                    af[0].modifier  = -2;
                    af[0].bitvector = AFF_NOFLEE;
                    SET_AF_BATTLE(victim, EAF_SLOW);
                    break;
             case 11: // femor damaged, no speed, no attack
                    dam    += (GET_REAL_MAX_HIT(victim)/8);
                    to_char = "вывело $N3 из строя";
                    to_vict = "раздробило Вам бедро";
                    af[0].type      = SPELL_BATTLE;
                    af[0].bitvector = AFF_STOPFIGHT;
                    af[0].duration  = pc_duration(victim,1,0,0,0,0);
                    af[1].type      = SPELL_BATTLE;
                    af[1].bitvector = AFF_NOFLEE;
                    haemorragia(victim,20);
                    SET_AF_BATTLE(victim, EAF_SLOW);
                    break;
             default: // femor damaged, no speed, no attack
                    if (dam_critic > 12)
                       dam += (GET_REAL_MAX_HIT(victim)/5);
                    else
                       dam += (GET_REAL_MAX_HIT(victim)/7);
                    to_char = "вывело $N3 из строя";
                    to_vict = "изуродовало Вам ногу";
                    af[0].type      = SPELL_BATTLE;
                    af[0].bitvector = AFF_STOPFIGHT;
                    af[0].duration  = pc_duration(victim,1,0,0,0,0);
                    af[1].type      = SPELL_BATTLE;
                    af[1].bitvector = AFF_NOFLEE;
                    haemorragia(victim,50);
                    SET_AF_BATTLE(victim, EAF_SLOW);
                    break;
          }
          break;
case 5: //  ABDOMINAL
          switch (dam_critic)
          {case 1: case 2: case 3:
                   // nothing
                   return dam;
           case 4: // waits 1d6
                   WAIT_STATE(victim, number(2,6)*PULSE_VIOLENCE);
                   to_char = "сбило $N2 дыхание";
                   to_vict = "сбило Вам дыхание";
                   break;

           case 5: // abdomin damaged, waits 1, speed/2
                   dam += (GET_REAL_MAX_HIT(victim)/7);
                   WAIT_STATE(victim, 2*PULSE_VIOLENCE);
                   to_char = "ранило $N3 в живот";
                   to_vict = "ранило Вас в живот";
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   break;
           case 6: // armor damaged else dam*3, waits 1d6
                   WAIT_STATE(victim, number(2,6)*PULSE_VIOLENCE);
                   if (GET_EQ(victim,WEAR_WAIST))
                      alt_equip(victim,WEAR_WAIST,100,100);
                   else
                      dam += (GET_REAL_MAX_HIT(victim)/5);
                   to_char = "повредило $N2 живот";
                   to_vict = "повредило Вам живот";
                   break;
           case 7: case 8: // abdomin damage, speed/2, HR-2
                   dam    += (GET_REAL_MAX_HIT(victim)/7);
                   to_char = "ранило $N3 в живот";
                   to_vict = "ранило Вас в живот";
                   af[0].type      = SPELL_BATTLE;
                   af[0].location  = APPLY_HITROLL;
                   af[0].modifier  = -2;
                   af[0].bitvector = AFF_NOFLEE;
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   break;
           case 9: // armor damaged, abdomin damaged, speed/2, HR-2
                   dam    += (GET_REAL_MAX_HIT(victim)/7);
                   alt_equip(victim,WEAR_BODY,100,100);
                   to_char = "ранило $N3 в живот";
                   to_vict = "ранило Вас в живот";
                   af[0].type      = SPELL_BATTLE;
                   af[0].location  = APPLY_HITROLL;
                   af[0].modifier  = -2;
                   af[0].bitvector = AFF_NOFLEE;
                   haemorragia(victim,20);
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   break;
           case 10: // abdomin damaged, no speed, no attack
                   dam    += (GET_REAL_MAX_HIT(victim)/7);
                   to_char = "повредило $N2 живот";
                   to_vict = "повредило Вам живот";
                   af[0].type      = SPELL_BATTLE;
                   af[0].bitvector = AFF_STOPFIGHT;
                   af[0].duration  = pc_duration(victim,1,0,0,0,0);
                   af[1].type      = SPELL_BATTLE;
                   af[1].bitvector = AFF_NOFLEE;
                   haemorragia(victim,20);
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   break;
           case 11: // abdomin damaged, no speed, no attack
                   dam    += (GET_REAL_MAX_HIT(victim)/4);
                   to_char = "разорвало $N2 живот";
                   to_vict = "разорвало Вам живот";
                   af[0].type      = SPELL_BATTLE;
                   af[0].bitvector = AFF_STOPFIGHT;
                   af[0].duration  = pc_duration(victim,1,0,0,0,0);
                   af[1].type      = SPELL_BATTLE;
                   af[1].bitvector = AFF_NOFLEE;
                   haemorragia(victim,40);
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   break;
           default: // abdomin damaged, hits = 0
                   dam     = GET_HIT(victim);
                   to_char = "размозжило $N2 живот";
                   to_vict = "размозжило Вам живот";
                   haemorragia(victim,60);
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   break;
          }
        break;
case 6: case 7: // CHEST
          switch (dam_critic)
          {case 1: case 2: case 3:
                   // nothing
                   return dam;
           case 4: // waits 1d4, bashed
                   WAIT_STATE(victim, number(2,5)*PULSE_VIOLENCE);
                   GET_POS(victim) = POS_SITTING;
                   to_char = "повредило $N2 грудь";
                   to_vict = "повредило Вам грудь";
                   break;
           case 5: // chest damaged, waits 1, speed/2
                   dam += (GET_REAL_MAX_HIT(victim)/6);
                   WAIT_STATE(victim, 2*PULSE_VIOLENCE);
                   to_char = "повредило $N2 туловище";
                   to_vict = "повредило Вам туловище";
                   af[0].type      = SPELL_BATTLE;
                   af[0].bitvector = AFF_NOFLEE;
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   break;
           case 6: // shield damaged, chest damaged, speed/2
                   alt_equip(victim,WEAR_SHIELD,100,100);
                   dam += (GET_REAL_MAX_HIT(victim)/6);
                   to_char = "повредило $N2 туловище";
                   to_vict = "повредило Вам туловище";
                   af[0].type      = SPELL_BATTLE;
                   af[0].bitvector = AFF_NOFLEE;
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   break;
           case 7: // srmor damaged, chest damaged, speed/2, HR-2
                   alt_equip(victim,WEAR_BODY,100,100);
                   dam    += (GET_REAL_MAX_HIT(victim)/6);
                   to_char = "повредило $N2 туловище";
                   to_vict = "повредило Вам туловище";
                   af[0].type      = SPELL_BATTLE;
                   af[0].location  = APPLY_HITROLL;
                   af[0].modifier  = -2;
                   af[0].bitvector = AFF_NOFLEE;
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   break;
           case 8: // chest damaged, no speed, no attack
                   dam    += (GET_REAL_MAX_HIT(victim)/6);
                   to_char = "вывело $N3 из строя";
                   to_vict = "повредило Вам туловище";
                   af[0].type      = SPELL_BATTLE;
                   af[0].bitvector = AFF_STOPFIGHT;
                   af[0].duration  = pc_duration(victim,1,0,0,0,0);
                   af[1].type      = SPELL_BATTLE;
                   af[1].bitvector = AFF_NOFLEE;
                   haemorragia(victim,20);
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   break;
           case 9: // chest damaged, speed/2, HR-2
                   dam    += (GET_REAL_MAX_HIT(victim)/6);
                   to_char = "заставило $N3 ослабить натиск";
                   to_vict = "сломало Вам ребра";
                   af[0].type      = SPELL_BATTLE;
                   af[0].location  = APPLY_HITROLL;
                   af[0].modifier  = -2;
                   af[1].type      = SPELL_BATTLE;
                   af[1].bitvector = AFF_NOFLEE;
                   haemorragia(victim,20);
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   break;
           case 10: // chest damaged, no speed, no attack
                   dam    += (GET_REAL_MAX_HIT(victim)/8);
                   to_char = "вывело $N3 из строя";
                   to_vict = "сломало Вам ребра";
                   af[0].type      = SPELL_BATTLE;
                   af[0].bitvector = AFF_STOPFIGHT;
                   af[0].duration  = pc_duration(victim,1,0,0,0,0);
                   af[1].type      = SPELL_BATTLE;
                   af[1].bitvector = AFF_NOFLEE;
                   haemorragia(victim,40);
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   break;
           case 11: // chest crushed, hits 0
                   af[0].type      = SPELL_BATTLE;
                   af[0].bitvector = AFF_STOPFIGHT;
                   af[0].duration  = pc_duration(victim,1,0,0,0,0);
                   dam     = GET_HIT(victim);
                   haemorragia(victim,50);
                   to_char = "вывело $N3 из строя";
                   to_vict = "разорвало Вам грудь";
                   break;
           default: // chest crushed, killing
                   af[0].type      = SPELL_BATTLE;
                   af[0].bitvector = AFF_STOPFIGHT;
                   af[0].duration  = pc_duration(victim,1,0,0,0,0);
                   dam     = GET_HIT(victim)+11;
                   haemorragia(victim,60);
                   to_char = "вывело $N3 из строя";
                   to_vict = "размозжило Вам грудь";
                   break;
          }
        break;
case 8: case 9: // HANDS
          switch (dam_critic)
          {case 1: case 2: case 3:
                   return dam;
           case 4: // hands damaged, weapon/shield putdown
                   to_char = "ослабило натиск $N1";
                   to_vict = "ранило Вам руку";
                   if (GET_EQ(victim,WEAR_BOTHS))
                      unequip_pos = WEAR_BOTHS;
                   else
                   if (GET_EQ(victim,WEAR_WIELD))
                      unequip_pos = WEAR_WIELD;
                   else
                   if (GET_EQ(victim,WEAR_HOLD))
                      unequip_pos = WEAR_HOLD;
                   else
                   if (GET_EQ(victim,WEAR_SHIELD))
                      unequip_pos = WEAR_SHIELD;
                   break;
           case 5: // hands damaged, shield damaged/weapon putdown
                   to_char = "ослабило натиск $N1";
                   to_vict = "ранило Вас в руку";
                   if (GET_EQ(victim,WEAR_SHIELD))
                      alt_equip(victim, WEAR_SHIELD, 100, 100);
                   else
                   if (GET_EQ(victim,WEAR_BOTHS))
                      unequip_pos = WEAR_BOTHS;
                   else
                   if (GET_EQ(victim,WEAR_WIELD))
                      unequip_pos = WEAR_WIELD;
                   else
                   if (GET_EQ(victim,WEAR_HOLD))
                      unequip_pos = WEAR_HOLD;
                   break;

           case 6: // hands damaged, HR-2, shield putdown
                   to_char = "ослабило натиск $N1";
                   to_vict = "сломало Вам руку";
                   if (GET_EQ(victim,WEAR_SHIELD))
                      unequip_pos = WEAR_SHIELD;
                   af[0].type      = SPELL_BATTLE;
                   af[0].location  = APPLY_HITROLL;
                   af[0].modifier  = -2;
                   break;
           case 7: // armor damaged, hand damaged if no armour
                   if (GET_EQ(victim, WEAR_ARMS))
                      alt_equip(victim,WEAR_ARMS,100,100);
                   else
                      alt_equip(victim,WEAR_HANDS,100,100);
                   if (!GET_EQ(victim,WEAR_ARMS) && !GET_EQ(victim,WEAR_HANDS))
                      dam    += (GET_REAL_MAX_HIT(victim)/8);
                   to_char = "ослабило атаку $N1";
                   to_vict = "повредило Вам руку";
                   break;
           case 8: // shield damaged, hands damaged, waits 1
                   alt_equip(victim,WEAR_SHIELD,100,100);
                   WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
                   dam    += (GET_REAL_MAX_HIT(victim)/8);
                   to_char = "придержало $N3";
                   to_vict = "повредило Вам руку";
                   break;
           case 9: // weapon putdown, hands damaged, waits 1d4
                   WAIT_STATE(victim, number(2,4) * PULSE_VIOLENCE);
                   if (GET_EQ(victim,WEAR_BOTHS))
                      unequip_pos = WEAR_BOTHS;
                   else
                   if (GET_EQ(victim,WEAR_WIELD))
                      unequip_pos = WEAR_WIELD;
                   else
                   if (GET_EQ(victim,WEAR_HOLD))
                      unequip_pos = WEAR_HOLD;
                   dam    += (GET_REAL_MAX_HIT(victim)/8);
                   to_char = "придержало $N3";
                   to_vict = "повредило Вам руку";
                   break;
           case 10: // hand damaged, no attack this
                   if (!AFF_FLAGGED(victim, AFF_STOPRIGHT))
                      {to_char = "ослабило атаку $N1";
                       to_vict = "изуродовало Вам правую руку";
                       af[0].type      = SPELL_BATTLE;
                       af[0].bitvector = AFF_STOPRIGHT;
                       af[0].duration  = pc_duration(victim,1,0,0,0,0);
                      }
                   else
                   if (!AFF_FLAGGED(victim, AFF_STOPLEFT))
                      {to_char = "ослабило атаку $N1";
                       to_vict = "изуродовало Вам левую руку";
                       af[0].type      = SPELL_BATTLE;
                       af[0].bitvector = AFF_STOPLEFT;
                       af[0].duration  = pc_duration(victim,1,0,0,0,0);
                      }
                   else
                      {to_char = "вывело $N3 из строя";
                       to_vict = "вывело Вас из строя";
                       af[0].type      = SPELL_BATTLE;
                       af[0].bitvector = AFF_STOPFIGHT;
                       af[0].duration  = pc_duration(victim,1,0,0,0,0);
                      }
                   haemorragia(victim,20);
                   break;
           default: // no hand attack, no speed, dam*2 if >= 13
                   if (!AFF_FLAGGED(victim, AFF_STOPRIGHT))
                      {to_char = "ослабило натиск $N1";
                       to_vict = "изуродовало Вам правую руку";
                       af[0].type      = SPELL_BATTLE;
                       af[0].bitvector = AFF_STOPRIGHT;
                       af[0].duration  = pc_duration(victim,1,0,0,0,0);
                      }
                   else
                   if (!AFF_FLAGGED(victim, AFF_STOPLEFT))
                      {to_char = "ослабило натиск $N1";
                       to_vict = "изуродовало Вам левую руку";
                       af[0].type      = SPELL_BATTLE;
                       af[0].bitvector = AFF_STOPLEFT;
                       af[0].duration  = pc_duration(victim,1,0,0,0,0);
                      }
                   else
                      {to_char = "вывело $N3 из строя";
                       to_vict = "вывело Вас из строя";
                       af[0].type      = SPELL_BATTLE;
                       af[0].bitvector = AFF_STOPFIGHT;
                       af[0].duration  = pc_duration(victim,1,0,0,0,0);
                      }
                   af[1].type      = SPELL_BATTLE;
                   af[1].bitvector = AFF_NOFLEE;
                   haemorragia(victim,30);
                   if (dam_critic >= 13)
                      dam *= 2;
                   SET_AF_BATTLE(victim, EAF_SLOW);
                   break;
          }
        break;
default: // HEAD
          switch (dam_critic)
          {case 1: case 2: case 3:
                   // nothing
                   return dam;
           case 4: // waits 1d6
                   WAIT_STATE(victim, number(2,6) *PULSE_VIOLENCE);
                   to_char = "помутило $N2 сознание";
                   to_vict = "помутило Ваше сознание";
                   break;

           case 5: // head damaged, cap putdown, waits 1, HR-2 if no cap
                   WAIT_STATE(victim, 2*PULSE_VIOLENCE);
                   if (GET_EQ(victim,WEAR_HEAD))
                      unequip_pos = WEAR_HEAD;
                   else
                      {af[0].type     = SPELL_BATTLE;
                       af[0].location = APPLY_HITROLL;
                       af[0].modifier = -2;
                      }
                   dam    += (GET_REAL_MAX_HIT(victim)/3);
                   to_char = "повредило $N2 голову";
                   to_vict = "повредило Вам голову";
                   break;
           case 6: // head damaged
                   af[0].type     = SPELL_BATTLE;
                   af[0].location = APPLY_HITROLL;
                   af[0].modifier = -2;
                   dam    += (GET_REAL_MAX_HIT(victim)/3);
                   to_char = "повредило $N2 голову";
                   to_vict = "повредило Вам голову";
                   break;
           case 7: // cap damaged, waits 1d6, speed/2, HR-4
                   WAIT_STATE(victim, 2*PULSE_VIOLENCE);
                   alt_equip(victim,WEAR_HEAD,100,100);
                   af[0].type     = SPELL_BATTLE;
                   af[0].location = APPLY_HITROLL;
                   af[0].modifier = -4;
                   af[0].bitvector= AFF_NOFLEE;
                   to_char = "ранило $N3 в голову";
                   to_vict = "ранило Вас в голову";
                   break;
           case 8: // cap damaged, hits 0
                   WAIT_STATE(victim, 4*PULSE_VIOLENCE);
                   alt_equip(victim,WEAR_HEAD,100,100);
                   dam     = GET_HIT(victim);
                   to_char = "отбило у $N1 сознание";
                   to_vict = "отбило у Вас сознание";
                   haemorragia(victim,20);
                   break;
           case 9: // head damaged, no speed, no attack
                   af[0].type      = SPELL_BATTLE;
                   af[0].bitvector = AFF_STOPFIGHT;
                   af[0].duration  = pc_duration(victim,1,0,0,0,0);
                   haemorragia(victim,30);
                   dam    += (GET_REAL_MAX_HIT(victim)/3);
                   to_char = "повергло $N3 в оцепенение";
                   to_vict = "повергло Вас в оцепенение";
                   break;
           case 10: // head damaged, -1 INT/WIS/CHA
                   dam            += (GET_REAL_MAX_HIT(victim)/2);
                   af[0].type      = SPELL_BATTLE;
                   af[0].location  = APPLY_INT;
                   af[0].modifier  = -1;
                   af[0].duration  = pc_duration(victim,number(1,6) * 24,0,0,0,0);
                   af[0].battleflag= AF_DEADKEEP;
                   af[1].type      = SPELL_BATTLE;
                   af[1].location  = APPLY_WIS;
                   af[1].modifier  = -1;
                   af[1].duration  = pc_duration(victim,number(1,6) * 24,0,0,0,0);
                   af[1].battleflag= AF_DEADKEEP;
                   af[2].type      = SPELL_BATTLE;
                   af[2].location  = APPLY_CHA;
                   af[2].modifier  = -1;
                   af[2].duration  = pc_duration(victim,number(1,6) * 24,0,0,0,0);
                   af[2].battleflag= AF_DEADKEEP;
                   af[3].type     = SPELL_BATTLE;
                   af[3].bitvector= AFF_STOPFIGHT;
                   af[3].duration = pc_duration(victim,number(1,6) * 24,0,0,0,0);
                   haemorragia(victim,50);
                   to_char = "сорвало у $N1 крышу";
                   to_vict = "сорвало у Вас крышу";
                   break;
           case 11: // hits 0, WIS/2, INT/2, CHA/2
                   dam             = GET_HIT(victim);
                   af[0].type      = SPELL_BATTLE;
                   af[0].location  = APPLY_INT;
                   af[0].modifier  = -GET_INT(victim)/2;
                   af[0].duration  = pc_duration(victim,number(1,6) * 24,0,0,0,0);
                   af[0].battleflag= AF_DEADKEEP;
                   af[1].type      = SPELL_BATTLE;
                   af[1].location  = APPLY_WIS;
                   af[1].modifier  = -GET_WIS(victim)/2;
                   af[1].duration  = pc_duration(victim,number(1,6) * 24,0,0,0,0);
                   af[1].battleflag= AF_DEADKEEP;
                   af[2].type      = SPELL_BATTLE;
                   af[2].location  = APPLY_CHA;
                   af[2].modifier  = -GET_CHA(victim)/2;
                   af[2].duration  = pc_duration(victim,number(1,6) * 24,0,0,0,0);
                   af[2].battleflag= AF_DEADKEEP;
                   haemorragia(victim,60);
                   to_char = "сорвало у $N1 крышу";
                   to_vict = "сорвало у Вас крышу";
                   break;
           default: // killed
                   af[0].type      = SPELL_BATTLE;
                   af[0].location  = APPLY_INT;
                   af[0].modifier  = -GET_INT(victim)/2;
                   af[0].duration  = pc_duration(victim,number(1,6) * 24,0,0,0,0);
                   af[0].battleflag= AF_DEADKEEP;
                   af[1].type      = SPELL_BATTLE;
                   af[1].location  = APPLY_WIS;
                   af[1].modifier  = -GET_WIS(victim)/2;
                   af[1].duration  = pc_duration(victim,number(1,6) * 24,0,0,0,0);
                   af[1].battleflag= AF_DEADKEEP;
                   af[2].type      = SPELL_BATTLE;
                   af[2].location  = APPLY_CHA;
                   af[2].modifier  = -GET_CHA(victim)/2;
                   af[2].duration  = pc_duration(victim,number(1,6) * 24,0,0,0,0);
                   af[2].battleflag= AF_DEADKEEP;
                   dam     = GET_HIT(victim) + 11;
                   to_char = "размозжило $N2 голову";
                   to_vict = "размозжило Вам голову";
                   haemorragia(victim,90);
                   break;
          }
        break;
 }

 for (i = 0; i < 4; i++)
     if (af[i].type)
        affect_join(victim, af+i, TRUE, FALSE, TRUE, FALSE);
 if (unequip_pos && GET_EQ(victim,unequip_pos))
    {obj = unequip_char(victim,unequip_pos);
     if (!IS_NPC(victim) && ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
        obj_to_char(obj,victim);
     else
        obj_to_room(obj,IN_ROOM(victim));
    }
 if (to_char)
    {sprintf(buf,"%sВаше точное попадание %s.%s",
            CCIGRN(ch, C_NRM), to_char, CCNRM(ch, C_NRM));
     act(buf, FALSE, ch, 0, victim, TO_CHAR);
    }
 if (to_vict)
    {sprintf(buf,"%sМеткое попадание $n1 %s.%s",
            CCIRED(victim, C_NRM), to_vict, CCNRM(victim, C_NRM));
     act(buf, FALSE, ch, 0, victim, TO_VICT);
    }
 return dam;
}

void poison_victim(struct char_data *ch, struct char_data *vict, int modifier)
{ struct    affected_type af[4];
  int       i;

  /* change strength */
  af[0].type         = SPELL_POISON;
  af[0].location     = APPLY_STR;
  af[0].duration     = pc_duration(vict,0,MAX(2, GET_LEVEL(ch)-GET_LEVEL(vict)),2,0,1);
  af[0].modifier     = - MIN(2, (modifier + 29) / 40);
  af[0].bitvector    = AFF_POISON;
  af[0].battleflag   = 0;
  /* change damroll */
  af[1].type        = SPELL_POISON;
  af[1].location    = APPLY_DAMROLL;
  af[1].duration    = af[0].duration;
  af[1].modifier    = - MIN(2, (modifier + 29) / 30);
  af[1].bitvector   = AFF_POISON;
  af[1].battleflag  = 0;
  /* change hitroll */
 af[2].type        = SPELL_POISON;
 af[2].location    = APPLY_HITROLL;
 af[2].duration    = af[0].duration;
 af[2].modifier    = -MIN(2, (modifier + 19) / 20);
 af[2].bitvector   = AFF_POISON;
 af[2].battleflag  = 0;
  /* change poison level */
 af[3].type        = SPELL_POISON;
 af[3].location    = APPLY_POISON;
 af[3].duration    = af[0].duration;
 af[3].modifier    = GET_LEVEL(ch);
 af[3].bitvector   = AFF_POISON;
 af[3].battleflag  = 0;

 for (i = 0; i < 4; i++)
     affect_join(vict, af+i, FALSE, FALSE, FALSE, FALSE);
 vict->Poisoner    = GET_ID(ch);
 act("Вы отравили $N3.", FALSE, ch, 0, vict, TO_CHAR);
 act("$n отравил$g Вас.", FALSE, ch,0, vict, TO_VICT);
}

int extdamage(struct char_data * ch,
              struct char_data * victim,
	      int dam,
              int attacktype,
	      struct obj_data *wielded,
	      int mayflee)
{int    prob, percent=0, lag = 0, i, mem_dam = dam;
 struct affected_type af;

 if (!victim)
    {return (0);
    }

 if (dam < 0)
    dam     = 0;

 // MIGHT_HIT
 if (attacktype == TYPE_HIT &&
     GET_AF_BATTLE(ch, EAF_MIGHTHIT) &&
     GET_WAIT(ch) <= 0
    )
    {CLR_AF_BATTLE(ch, EAF_MIGHTHIT);
     if (IS_NPC(ch)       ||
         IS_IMMORTAL(ch)  ||
         !(GET_EQ(ch,WEAR_BOTHS)  || GET_EQ(ch,WEAR_WIELD) ||
           GET_EQ(ch,WEAR_HOLD)   || GET_EQ(ch,WEAR_LIGHT) ||
           GET_EQ(ch,WEAR_SHIELD) || GET_AF_BATTLE(ch, EAF_TOUCH))
         )
        {percent = number(1,skill_info[SKILL_MIGHTHIT].max_percent);
         prob    = train_skill(ch, SKILL_MIGHTHIT, skill_info[SKILL_MIGHTHIT].max_percent, victim);
	 if (GET_MOB_HOLD(victim))
	    prob = MAX(prob, percent);
         if (prob * 100 / percent < 100 || dam == 0)
            {sprintf(buf,"%sВаш богатырский удар пропал впустую.%s\r\n",
                     CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
             send_to_char(buf,ch);
             lag = 3;
             dam = 0;
            }
         else
         if (prob * 100 / percent < 150)
            {sprintf(buf,"%sВаш богатырский удар задел %s.%s\r\n",
                     CCBLU(ch, C_NRM), PERS(victim,ch,3), CCNRM(ch, C_NRM));
             send_to_char(buf,ch);
             lag  = 3;
             dam += (dam / 2);
            }
         else
         if (prob * 100 / percent < 400)
            {sprintf(buf,"%sВаш богатырский удар пошатнул %s.%s\r\n",
                     CCGRN(ch, C_NRM), PERS(victim,ch,3), CCNRM(ch, C_NRM));
             send_to_char(buf,ch);
             lag  = 2;
             dam += (dam / 1);
             WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
             af.type         = SPELL_BATTLE;
             af.bitvector    = AFF_STOPFIGHT;
             af.location     = 0;
             af.modifier     = 0;
             af.duration     = pc_duration(victim,2,0,0,0,0);
             af.battleflag   = AF_BATTLEDEC;
             affect_join(victim, &af, TRUE, FALSE, TRUE, FALSE);
             sprintf(buf,"%sВаше сознание затуманилось после удара %s.%s\r\n",
                     CCIRED(victim, C_NRM), PERS(ch,victim,1), CCNRM(victim, C_NRM));
             send_to_char(buf, victim);
            }
         else
            {sprintf(buf,"%sВаш богатырский удар сотряс %s.%s\r\n",
                     CCIGRN(ch, C_NRM), PERS(victim,ch,3), CCNRM(ch, C_NRM));
             send_to_char(buf,ch);
             lag  = 2;
             dam *= 4;
             WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
             af.type         = SPELL_BATTLE;
             af.bitvector    = AFF_STOPFIGHT;
             af.location     = 0;
             af.modifier     = 0;
             af.duration     = pc_duration(victim,3,0,0,0,0);
             af.battleflag   = AF_BATTLEDEC;
             affect_join(victim, &af, TRUE, FALSE, TRUE, FALSE);
             sprintf(buf,"%sВаше сознание померкло после удара %s.%s\r\n",
                     CCIRED(victim, C_NRM), PERS(ch,victim,1), CCNRM(victim, C_NRM));
             send_to_char(buf, victim);
            }
         if (!WAITLESS(ch))
            WAIT_STATE(ch, lag * PULSE_VIOLENCE);
        }
    }
 // STUPOR
 else
 if (GET_AF_BATTLE(ch,EAF_STUPOR) &&
     GET_WAIT(ch) <= 0
    )
    {CLR_AF_BATTLE(ch, EAF_STUPOR);
     if (IS_NPC(ch)       ||
         IS_IMMORTAL(ch)  ||
         (wielded &&
          GET_OBJ_WEIGHT(wielded) > 21 &&
          GET_OBJ_SKILL(wielded) != SKILL_BOWS &&
          !GET_AF_BATTLE(ch, EAF_PARRY) &&
          !GET_AF_BATTLE(ch, EAF_MULTYPARRY)
         )
        )
        {percent = number(1,skill_info[SKILL_STUPOR].max_percent);
         prob    = train_skill(ch, SKILL_STUPOR, skill_info[SKILL_STUPOR].max_percent, victim);
	 if (GET_MOB_HOLD(victim))
	    prob = MAX(prob, percent * 150 / 100 + 1);
         if (prob * 100 / percent < 150 || dam == 0)
            {sprintf(buf,"%sВы попытались оглушить %s, но не смогли.%s\r\n",
                     CCCYN(ch, C_NRM), PERS(victim,ch,3), CCNRM(ch, C_NRM));
             send_to_char(buf,ch);
             lag = 3;
             dam = 0;
            }
         else
         if (prob * 100 / percent < 300)
            {sprintf(buf,"%sВаша мощная атака оглушила %s.%s\r\n",
                     CCBLU(ch, C_NRM), PERS(victim,ch,3), CCNRM(ch, C_NRM));
             send_to_char(buf,ch);
             lag  = 2;
             WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
             sprintf(buf,"%sВаше сознание помутилось после удара %s.%s\r\n",
                     CCIRED(victim, C_NRM), PERS(ch,victim,1), CCNRM(victim, C_NRM));
             send_to_char(buf, victim);
            }
         else
            {sprintf(buf,"%sВаш мощнейший удар сбил %s с ног.%s\r\n",
                     CCIGRN(ch, C_NRM), PERS(victim,ch,3), CCNRM(ch, C_NRM));
             send_to_char(buf,ch);
             lag  = 2;
             WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
             if (GET_POS(victim) > POS_SITTING)
                {GET_POS(victim)        = POS_SITTING;
                 sprintf(buf,"%sОглушающий удар %s сбил Вас с ног.%s\r\n",
                         CCIRED(victim, C_NRM), PERS(ch,victim,3), CCNRM(victim, C_NRM));
                 send_to_char(buf, victim);
                }
             else
                {sprintf(buf,"%sВаше сознание помутилось после удара %s.%s\r\n",
                         CCIRED(victim, C_NRM), PERS(ch,victim,1), CCNRM(victim, C_NRM));
                 send_to_char(buf, victim);
                }
            }
         if (!WAITLESS(ch))
            WAIT_STATE(ch, lag * PULSE_VIOLENCE);
        }
    }
 // Calculate poisoned weapon
 else
 if (dam && wielded && timed_by_skill(ch, SKILL_POISONED))
    {for (i = 0; i < MAX_OBJ_AFFECT; i++)
         if (wielded->affected[i].location == APPLY_POISON)
            break;
     if (i < MAX_OBJ_AFFECT                &&
         wielded->affected[i].modifier > 0 &&
         !AFF_FLAGGED(victim, AFF_POISON)  &&
         !WAITLESS(victim)
	)
        {percent = number(1,skill_info[SKILL_POISONED].max_percent);
         prob    = calculate_skill(ch, SKILL_POISONED, skill_info[SKILL_POISONED].max_percent, victim);
         if (prob >= percent &&
             !general_savingthrow(victim, SAVING_ROD, con_app[GET_REAL_CON(victim)].poison_saving)
            )
            {improove_skill(ch, SKILL_POISONED, TRUE, victim);
             poison_victim(ch,victim, prob - percent);
             wielded->affected[i].modifier--;
            }
        }
    }
 // Calculate mob-poisoner
 else
 if (dam &&
     IS_NPC(ch) &&
     NPC_FLAGGED(ch, NPC_POISON) &&
     !AFF_FLAGGED(ch,AFF_CHARM)  &&
     GET_WAIT(ch) <= 0           &&
     !AFF_FLAGGED(victim, AFF_POISON) &&
     number(0,100) < GET_LIKES(ch) + GET_LEVEL(ch) - GET_LEVEL(victim) &&
     !general_savingthrow(victim, SAVING_ROD, con_app[GET_REAL_CON(victim)].poison_saving)
    )
    poison_victim(ch,victim,MAX(1,GET_LEVEL(ch) - GET_LEVEL(victim)) * 10);

 if (mem_dam >= 0)
    return (damage(ch, victim, dam, attacktype,mayflee));
 else
    {return (0);
    }
}



/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim  died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */

void char_dam_message(int dam, struct char_data *ch, struct char_data *victim, int attacktype, int mayflee)
{ switch (GET_POS(victim))
  {case POS_MORTALLYW:
    act("$n смертельно ранен$a и умрет, если $m не помогут.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("Вы смертельно ранены и умрете, если Вам не помогут.\r\n", victim);
    break;
   case POS_INCAP:
    act("$n без сознания и медленно умирает. Помогите же $m.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("Вы без сознания и медленно умираете, брошенные без помощи.\r\n", victim);
    break;
   case POS_STUNNED:
    act("$n без сознания, но возможно $e еще повоюет (попозже :).", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("Сознание покинуло Вас. В битве от Вас пока проку мало.\r\n", victim);
    break;
   case POS_DEAD:
    act("$n мертв$g, $s душа медленно подымается в небеса.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char("Вы мертвы!  Hам очень жаль...\r\n", victim);
    break;
   default:			/* >= POSITION SLEEPING */
    if (dam > (GET_REAL_MAX_HIT(victim) / 4))
        send_to_char("Это действительно БОЛЬНО !\r\n", victim);

    if (dam > 0 && GET_HIT(victim) < (GET_REAL_MAX_HIT(victim) / 4))
       {sprintf(buf2, "%s Вы желаете, чтобы Ваши раны не кровоточили так сильно ! %s\r\n",
	            CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
        send_to_char(buf2, victim);
       }
    if (ch != victim   &&
        IS_NPC(victim) &&
        GET_HIT(victim) < (GET_REAL_MAX_HIT(victim) / 4)  &&
        MOB_FLAGGED(victim, MOB_WIMPY)                    &&
	mayflee                                           &&
        GET_POS(victim) > POS_SITTING
       )
       do_flee(victim, NULL, 0, 0);

    if (ch != victim         &&
        !IS_NPC(victim)      &&
	HERE(victim)         &&
        GET_WIMP_LEV(victim) &&
	GET_HIT(victim) < GET_WIMP_LEV(victim)            &&
	mayflee                                           &&
	GET_POS(victim) > POS_SITTING
       )
       {send_to_char("Вы запаниковали и попытались убежать !\r\n", victim);
        do_flee(victim, NULL, 0, 0);
       }
    break;
   }
}




int damage(struct char_data * ch, struct char_data * victim, int dam, int attacktype, int mayflee)
{ int FS_damage = 0;

  if (!ch || !victim)
     {return (0);
     }


  if (IN_ROOM(victim) == NOWHERE || IN_ROOM(ch) == NOWHERE ||
      IN_ROOM(ch) != IN_ROOM(victim)
     )
     {log("SYSERR: Attempt to damage '%s' in room NOWHERE by '%s'.",
          GET_NAME(victim), GET_NAME(ch));
      return (0);
     }

  if (GET_POS(victim) <= POS_DEAD)
     {log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
		  GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
      die(victim,NULL);
      return (0);			/* -je, 7/7/92 */
     }

  //
  if (damage_mtrigger(ch,victim))
     {return (0);
     }

  // Shopkeeper protection
  if (!ok_damage_shopkeeper(ch, victim))
     {return (0);
     }

  // No fight mobiles
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOFIGHT))
     {return (0);
     }

  if (IS_NPC(victim) && MOB_FLAGGED(ch, MOB_NOFIGHT))
     {act("Боги предотвратили Ваше нападение на $N3.",FALSE,ch,0,victim,TO_CHAR);
      return (0);
     }

  // You can't damage an immortal!
  if (IS_GOD(victim))
     dam = 0;
  else
  if (IS_IMMORTAL(victim) ||
      GET_GOD_FLAG(victim,GF_GODSLIKE))
     dam /= 4;
  else
  if (GET_GOD_FLAG(victim,GF_GODSCURSE))
     dam *= 2;

  if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
     remember(victim, ch);
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_MEMORY) && !IS_NPC(victim))
     remember(ch, victim);

  //*************** If the attacker is invisible, he becomes visible
  appear(ch);

  if (victim != ch)
    {//**************** Start the attacker fighting the victim
     if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
        {if (may_pkill(ch,victim) != PC_REVENGE_PC)
            {inc_pkill_group(victim, ch, 1, 0);
            }
         else
            {inc_pkill(victim, ch, 0, 1);
            }
         set_fighting(ch, victim);
         npc_groupbattle(ch);
        }

     //***************** Start the victim fighting the attacker
     if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL))
        {set_fighting(victim, ch);
         npc_groupbattle(victim);
        }
    }

  //**************** If you attack a pet, it hates your guts
  if (victim != ch)
     {if (victim->master == ch)
         {if (stop_follower(victim, SF_EMPTY))
             return (-1);
         }
      else
      if (ch->master == victim ||
          victim->master == ch ||
          (ch->master &&
	   ch->master == victim->master
	  )
	 )
        {if (stop_follower(ch, SF_EMPTY))
            return (-1);
        }
     }

  //*************** If negative damage - return
  if (dam < 0                    ||
      IN_ROOM(ch) == NOWHERE     ||
      IN_ROOM(victim) == NOWHERE ||
      IN_ROOM(ch) != IN_ROOM(victim)
     )
     return (0);

  check_killer(ch, victim);

  if (victim != ch)
     {if (dam && AFF_FLAGGED(victim, AFF_SHIELD))
         {act("Магический кокон полностью поглотил удар $N1.",FALSE,victim,0,ch,TO_CHAR);
          act("Магический кокон вокруг $N1 полностью поглотил Ваш удар.",FALSE,ch,0,victim,TO_CHAR);
          act("Магический кокон вокруг $N1 полностью поглотил удар $n1.",TRUE,ch,0,victim,TO_NOTVICT);
          return (0);
         }
	
      if (dam > 0 && !was_critic && AFF_FLAGGED(victim, AFF_FIRESHIELD))
         {FS_damage = dam * 20 / 100;
	  dam      -= (dam * number(10,30) / 100);
	 }
	
      if (dam > 0 && !was_critic && AFF_FLAGGED(victim, AFF_ICESHIELD))
     	 {act("Ледяной щит принял часть удара на себя.",FALSE,ch,0,victim,TO_VICT);
          act("Ледяной щит вокруг $N1 смягчил Ваш удар.",FALSE,ch,0,victim,TO_CHAR);
          act("Ледяной щит вокруг $N1 смягчил удар $n1.",TRUE,ch,0,victim,TO_NOTVICT);
	  dam -= (dam * number(30,50) / 100);
	 }

      if (dam > 0 && !was_critic && AFF_FLAGGED(victim, AFF_AIRSHIELD))
     	 {act("Воздушный щит смягчил удар $n1.",FALSE,ch,0,victim,TO_VICT);
          act("Воздушный щит вокруг $N1 ослабил Ваш удар.",FALSE,ch,0,victim,TO_CHAR);
          act("Воздушный щит вокруг $N1 ослабил удар $n1.",TRUE,ch,0,victim,TO_NOTVICT);
	  dam -= (dam * number(30,50) / 100);
	 }

      if (dam && IS_WEAPON(attacktype))
         {alt_equip(victim,NOWHERE,dam,50);
          if (!was_critic)
             {int decrease = MIN(25,(GET_ABSORBE(victim)+1)/2) + GET_ARMOUR(victim);
              if (decrease >= number(dam, dam * 50))
                 {act("Ваши доспехи полностью поглотили удар $n1.",FALSE,ch,0,victim,TO_VICT);
                  act("Доспехи $N1 полностью поглотили Ваш удар.",FALSE,ch,0,victim,TO_CHAR);
                  act("Доспехи $N1 полностью поглотили удар $n1.",TRUE,ch,0,victim,TO_NOTVICT);
                  return(0);
                 }
              dam  -= (dam * MIN(50,decrease) / 100);
             }
         }
     }
  else
  if (MOB_FLAGGED(victim, MOB_PROTECT))
     {return (0);
     }

  //*************** Set the maximum damage per round and subtract the hit points
  if (MOB_FLAGGED(victim, MOB_PROTECT))
     {act("$n находится под защитой Богов.",FALSE,victim,0,0,TO_ROOM);
      return (0);
     }
  // log("[DAMAGE] Compute critic...");
  dam = MAX(dam, 0);
  if (dam && was_critic)
     {FS_damage = 0;
      dam = compute_critical(ch,victim,dam);
     }

  if (attacktype == SPELL_FIRE_SHIELD)
     {if ((GET_HIT(victim) -= dam) < 1)
         GET_HIT(victim) = 1;
     }
  else
     GET_HIT(victim) -= dam;

  //*************** Gain exp for the hit
  if (ch != victim &&
      OK_GAIN_EXP(ch, victim)
     )
     gain_exp(ch, IS_NPC(ch) ? GET_LEVEL(victim) * dam : (GET_LEVEL(victim) * dam + 4) / 5);
  // log("[DAMAGE] Updating pos...");
  update_pos(victim);


  // * skill_message sends a message from the messages file in lib/misc.
  //  * dam_message just sends a generic "You hit $n extremely hard.".
  // * skill_message is preferable to dam_message because it is more
  // * descriptive.
  // *
  // * If we are _not_ attacking with a weapon (i.e. a spell), always use
  // * skill_message. If we are attacking with a weapon: If this is a miss or a
  // * death blow, send a skill_message if one exists; if not, default to a
  // * dam_message. Otherwise, always send a dam_message.
  // log("[DAMAGE] Attack message...");

  if (!IS_WEAPON(attacktype))
     skill_message(dam, ch, victim, attacktype);
  else
    {if (GET_POS(victim) == POS_DEAD || dam == 0)
        {if (!skill_message(dam, ch, victim, attacktype))
            dam_message(dam, ch, victim, attacktype);
        }
     else
        {dam_message(dam, ch, victim, attacktype);
        }
    }

  // log("[DAMAGE] Victim message...");
  //******** Use send_to_char -- act() doesn't send message if you are DEAD.
  char_dam_message(dam,ch,victim,attacktype,mayflee);
  // log("[DAMAGE] Flee etc...");
  // *********** Help out poor linkless people who are attacked */
  if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED)
     {/*
      do_flee(victim, NULL, 0, 0);
      if (!FIGHTING(victim))
         {act("$n был$g спасен$a Богами.", FALSE, victim, 0, 0, TO_ROOM);
          GET_WAS_IN(victim) = victim->in_room;
          char_from_room(victim);
          char_to_room(victim, STRANGE_ROOM);
         }
       */
     }

  // *********** Stop someone from fighting if they're stunned or worse
  if ((GET_POS(victim) <= POS_STUNNED) && (FIGHTING(victim) != NULL))
     {stop_fighting(victim,GET_POS(victim) <= POS_DEAD);
     }

  // *********** Uh oh.  Victim died.
  if (GET_POS(victim) == POS_DEAD)
     {struct char_data *killer = NULL;

      if (IS_NPC(victim) || victim->desc)
         {if (victim == ch && IN_ROOM(victim) != NOWHERE)
             {if (attacktype == SPELL_POISON)
                 {struct char_data *poisoner;
                  for (poisoner = world[IN_ROOM(victim)].people; poisoner; poisoner = poisoner->next_in_room)
                      if (poisoner != victim && GET_ID(poisoner) == victim->Poisoner)
                         killer = poisoner;
                 }
              else
              if (attacktype == TYPE_SUFFERING)
                 {struct char_data *attacker;
                  for (attacker = world[IN_ROOM(victim)].people; attacker; attacker = attacker->next_in_room)
                      if (FIGHTING(attacker) == victim)
                         killer = attacker;
                 }
             }
          if (ch != victim)
             killer = ch;
         }

      if (killer)
         {if (AFF_FLAGGED(killer, AFF_GROUP))
	     group_gain(killer, victim);
	  else
	  if (AFF_FLAGGED(killer,AFF_CHARM) && killer->master)
	     {if (IN_ROOM(killer) == IN_ROOM(killer->master))
	         {if (!IS_NPC(killer->master) && AFF_FLAGGED(killer->master,AFF_GROUP))
	             group_gain(killer->master,victim);
                  else
                     {solo_gain(killer->master, victim);
		      solo_gain(killer,victim);
		     }
		 }
              else
                 solo_gain(killer, victim);
             }
          else
             solo_gain(killer, victim);
         }
      if (!IS_NPC(victim))
         {sprintf(buf2, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch),
	          IN_ROOM(victim) != NOWHERE ? world[victim->in_room].name : "NOWHERE");
          mudlog(buf2, BRF, LVL_IMMORT, TRUE);
          if (MOB_FLAGGED(ch, MOB_MEMORY))
	         forget(ch, victim);
         }
      die(victim,ch);
      return (-1);
     }
  if (FS_damage                     &&
      FIGHTING(victim)              &&
      GET_POS(victim) > POS_STUNNED &&
      IN_ROOM(victim) != NOWHERE
     )
     damage(victim,ch,FS_damage,SPELL_FIRE_SHIELD,FALSE);
  return (dam);
}

/**** This function realize second shot for bows *******/
void exthit(struct char_data * ch, int type, int weapon)
{ struct obj_data *wielded = NULL;
  int    percent, prob;

  if (IS_NPC(ch))
     {if (MOB_FLAGGED(ch,MOB_EADECREASE) &&
          weapon > 1)
         {if (ch->mob_specials.ExtraAttack * GET_HIT(ch) * 2 <
              weapon                       * GET_REAL_MAX_HIT(ch))
          return;
         }
      if (MOB_FLAGGED(ch,(MOB_FIREBREATH | MOB_GASBREATH | MOB_FROSTBREATH |
                          MOB_ACIDBREATH | MOB_LIGHTBREATH)))
         {for (prob = percent = 0; prob <= 4; prob++)
              if (MOB_FLAGGED(ch, (INT_TWO | (1 << prob))))
                 percent++;
          percent = weapon % percent;
          for (prob = 0; prob <= 4; prob++)
              if (MOB_FLAGGED(ch, (INT_TWO | (1 << prob))))
                 {if (percent)
                     percent--;
                  else
                     break;
                 }
          mag_damage(GET_LEVEL(ch),ch,FIGHTING(ch),SPELL_FIRE_BREATH+MIN(prob,4),SAVING_SPELL);
          return;
         }
     }

  if (weapon == 1)
     {if (!(wielded = GET_EQ(ch,WEAR_WIELD)))
         wielded = GET_EQ(ch,WEAR_BOTHS);
     }
  else
  if (weapon == 2)
     wielded = GET_EQ(ch,WEAR_HOLD);
  percent = number(1,skill_info[SKILL_ADDSHOT].max_percent);
  if (wielded &&
      GET_OBJ_SKILL(wielded) == SKILL_BOWS &&
      ((prob = train_skill(ch,SKILL_ADDSHOT,skill_info[SKILL_ADDSHOT].max_percent,0)) >= percent ||
       WAITLESS(ch)
      )
     )
     {hit(ch, FIGHTING(ch), type, weapon);
// Изменено Стрибогом     
//      if (prob / percent > 4 && FIGHTING(ch))
      if (prob > (percent * 2)  && FIGHTING(ch))
         hit(ch, FIGHTING(ch), type, weapon);
     }
  hit(ch, FIGHTING(ch), type, weapon);
}


void hit(struct char_data * ch, struct char_data * victim, int type, int weapon)
{
  struct obj_data *wielded=NULL;
  struct char_data *vict;
  int w_type = 0, victim_ac, calc_thaco, dam, diceroll, prob, range, skill = 0,
      weapon_pos = WEAR_WIELD, percent, is_shit = (weapon == 2) ? 1 : 0;

  if (!victim)
     return;

  /* check if the character has a fight trigger */
  fight_mtrigger(ch);

  /* Do some sanity checking, in case someone flees, etc. */
  if (ch->in_room != victim->in_room || ch->in_room == NOWHERE)
     {if (FIGHTING(ch) && FIGHTING(ch) == victim)
         stop_fighting(ch,TRUE);
      return;
     }

  /* Stand awarness mobs */
  if (CAN_SEE(victim, ch) &&
      !FIGHTING(victim)   &&
      ((IS_NPC(victim)    &&
        (GET_HIT(victim) < GET_MAX_HIT(victim) ||
         MOB_FLAGGED(victim, MOB_AWARE)
        )
       ) ||
       AFF_FLAGGED(victim, AFF_AWARNESS)
      )                     &&
      !GET_MOB_HOLD(victim) &&
      GET_WAIT(victim) <= 0)
     set_battle_pos(victim);

  /* Find weapon for attack number weapon */
  if (weapon == 1)
     {if (!(wielded=GET_EQ(ch,WEAR_WIELD)))
         {wielded    = GET_EQ(ch,WEAR_BOTHS);
          weapon_pos = WEAR_BOTHS;
         }
     }
  else
  if (weapon == 2)
     {wielded    = GET_EQ(ch,WEAR_HOLD);
      weapon_pos = WEAR_HOLD;
     }

  calc_thaco = 0;
  victim_ac  = 0;
  dam        = 0;

  /* Find the weapon type (for display purposes only) */
  if (type == SKILL_THROW)
     {diceroll = 100;
      weapon   = 100;
      skill    = SKILL_THROW;
      w_type   = type + TYPE_HIT;
     }
  else
  if (type == SKILL_BACKSTAB)
     {diceroll = 100;
      weapon   = 100;
      skill    = SKILL_BACKSTAB;
      w_type   = type + TYPE_HIT;
     }
  else
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
     {skill    = GET_OBJ_SKILL(wielded);
      diceroll = number(1,skill_info[skill].max_percent);
      weapon   = train_skill(ch,skill,skill_info[skill].max_percent,victim);

      if (!IS_NPC(ch))
         {// Two-handed attack - decreace TWO HANDS
          if (weapon == 1 &&
              GET_EQ(ch,WEAR_HOLD) &&
              GET_OBJ_TYPE(GET_EQ(ch,WEAR_HOLD)) == ITEM_WEAPON)
             {if (weapon < EXPERT_WEAPON)
                 calc_thaco += 2;
              else
                 calc_thaco += 0;
             }
          else
          if (weapon == 2 && GET_EQ(ch,WEAR_WIELD) &&
              GET_OBJ_TYPE(GET_EQ(ch,WEAR_WIELD)) == ITEM_WEAPON)
             {if (weapon < EXPERT_WEAPON)
                 calc_thaco += 4;
              else
                 calc_thaco += 2;
             }

          // Apply HR for light weapon
          percent = 0;
          switch (weapon_pos)
          {case WEAR_WIELD: percent = (str_app[STRENGTH_APPLY_INDEX(ch)].wield_w  - GET_OBJ_WEIGHT(wielded) + 1) / 2;
           case WEAR_HOLD : percent = (str_app[STRENGTH_APPLY_INDEX(ch)].hold_w  - GET_OBJ_WEIGHT(wielded) + 1)  / 2;
           case WEAR_BOTHS: percent = (str_app[STRENGTH_APPLY_INDEX(ch)].wield_w +
                                       str_app[STRENGTH_APPLY_INDEX(ch)].hold_w  - GET_OBJ_WEIGHT(wielded) + 1)  / 2;
          }
          calc_thaco -= MIN(5, MAX(percent,0));


          // Optimize weapon
             switch((int) GET_CLASS(ch))
          {case CLASS_CLERIC:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 0; dam += 1; break;
                  case SKILL_AXES: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 0; dam += 0; break;
                  case SKILL_PICK: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_SPADES: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= 0; dam += 0; break;
                }
                break;
           case CLASS_BATTLEMAGE:
           case CLASS_DEFENDERMAGE:
           case CLASS_CHARMMAGE:
           case CLASS_NECROMANCER:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_AXES: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 0; dam += 0; break;
                  case SKILL_PICK: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_SPADES: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= 0; dam += 0; break;
                }
                break;
           case CLASS_WARRIOR:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 2; dam += 0; break;
                  case SKILL_AXES: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco -= -2; dam += 0; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 0; dam += 2; break;
                  case SKILL_PICK: 		calc_thaco -= -2; dam += 0; break;
                  case SKILL_SPADES: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= 0; dam += 0; break;
                }
                break;

           case CLASS_RANGER:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_AXES: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco += 1; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 1; dam += 0; break;
                  case SKILL_BOTHHANDS: 	calc_thaco += 1; dam += 0; break;
                  case SKILL_PICK: 		calc_thaco += 1; dam += 0; break;
                  case SKILL_SPADES: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= 2; dam += 1; break;
                }
                break;

           case CLASS_GUARD:
           case CLASS_PALADINE:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_AXES: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 1; dam += 0; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 1; dam += 3; break;
                  case SKILL_PICK: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_SPADES: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= 1; dam += 0; break;
                }
                break;
           case CLASS_THIEF:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= -1; dam += 0; break;
                  case SKILL_AXES: 		calc_thaco -= -1; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco -= -1; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco -= 0; dam += 1; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= -1; dam += 0; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= -1; dam += 0; break;
                  case SKILL_PICK: 		calc_thaco -= 0; dam += 1; break;
                  case SKILL_SPADES: 		calc_thaco -= -1; dam += 1; break;
                  case SKILL_BOWS: 		calc_thaco -= -1; dam += 0; break;
                }
                break;
           case CLASS_ASSASINE:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= -1; dam += 0; break;
                  case SKILL_AXES: 		calc_thaco -= -1; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco -= 2; dam += 7; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= -1; dam += 4; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= -1; dam += 0; break;
                  case SKILL_PICK: 		calc_thaco -= 2; dam += 7; break;
                  case SKILL_SPADES: 		calc_thaco -= -1; dam += 4; break;
                  case SKILL_BOWS: 		calc_thaco -= -1; dam += 0; break;
                }
           case CLASS_SMITH:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_AXES: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_LONGS: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_SHORTS: 		calc_thaco -= -1; dam += -1; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 0; dam += 0; break;
                  case SKILL_PICK: 		calc_thaco -= -1; dam += -1; break;
                  case SKILL_SPADES: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= -1; dam += -1; break;
                }
                break;
           case CLASS_MERCHANT:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_AXES: 		calc_thaco -= -1; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco -= -1; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 1; dam += 1; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= -1; dam += -1; break;
                  case SKILL_PICK: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_SPADES: 		calc_thaco -= -1; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= -1; dam += 0; break;
                }
                break;
           case CLASS_DRUID:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 1; dam += 2; break;
                  case SKILL_AXES: 		calc_thaco -= -1; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco -= -1; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 1; dam += 0; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= -1; dam += 0; break;
                  case SKILL_PICK: 		calc_thaco -= -1; dam += 0; break;
                  case SKILL_SPADES: 		calc_thaco -= -1; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= -1; dam += 0; break;
                }

                break;
          }

          // Penalty for unknown weapon type
          if (GET_SKILL(ch, skill) == 0)
             {weapon   = 0;
              diceroll = class_app[(int) GET_CLASS(ch)].unknown_weapon_fault * 10;
             }

          // Bonus for expert weapon
          if (weapon >= EXPERT_WEAPON)
             calc_thaco -= 1;
          // Bonus for leadership
          if (calc_leadership(ch))
             calc_thaco -= 2;
         }
      w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
     }
  else
     {skill      = SKILL_PUNCH;
      weapon_pos = 0;
      diceroll   = number(0,skill_info[skill].max_percent);
      weapon     = train_skill(ch,skill,skill_info[skill].max_percent,victim);

      if (!IS_NPC(ch))
         {// is this SHIT
          if (is_shit)
             {diceroll += number(0,skill_info[SKILL_SHIT].max_percent);
              weapon   += train_skill(ch,SKILL_SHIT,skill_info[SKILL_SHIT].max_percent,victim);
             }
          // Bonus for expert PUNCH
          if (weapon >= EXPERT_WEAPON)
             calc_thaco -= 1;
          // Bonus for leadership
          if (calc_leadership(ch))
             calc_thaco -= 2;
         }

      if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
         w_type  = ch->mob_specials.attack_type + TYPE_HIT;
      else
         w_type += TYPE_HIT;
     }

  // courage
  if (affected_by_spell(ch, SPELL_COURAGE))
     {range = number(1,skill_info[SKILL_COURAGE].max_percent+GET_REAL_MAX_HIT(ch)-GET_HIT(ch));
      prob  = train_skill(ch,SKILL_COURAGE,skill_info[SKILL_COURAGE].max_percent,victim);
      if (prob > range)
         {dam        += ((GET_SKILL(ch,SKILL_COURAGE)+19) / 20);
          calc_thaco += ((GET_SKILL(ch,SKILL_COURAGE)+9) / 10);
         }
     }


  if (!IS_NPC(ch) && skill != SKILL_THROW && skill != SKILL_BACKSTAB)
     {// PUNCTUAL style - decrease PC damage
      if (GET_AF_BATTLE(ch, EAF_PUNCTUAL))
         {calc_thaco += 0;
	  dam        -= 0;
	 }
      //    AWAKE style - decrease PC hitroll
      if (GET_AF_BATTLE(ch, EAF_AWAKE))
         {calc_thaco  += ((GET_SKILL(ch,SKILL_AWAKE) + 9) / 10)+2;
	  dam         -= 0;
	 }

      // Casters use weather, int and wisdom
      if (IS_CASTER(ch))
         { calc_thaco += (10 - complex_skill_modifier(ch,SKILL_THAC0,GAPPLY_SKILL_SUCCESS,10));
           calc_thaco -= (int) ((GET_REAL_INT(ch) - 13) / 1.5);
           calc_thaco -= (int) ((GET_REAL_WIS(ch) - 13) / 1.5);
         }

      // Horse modifier for attacker
      if (on_horse(ch))
         {prob    = train_skill(ch,SKILL_HORSE,skill_info[SKILL_HORSE].max_percent,0);
          dam    += ((prob+19) / 20);
          range   = number(1,skill_info[SKILL_HORSE].max_percent);
          if (range > prob)
             calc_thaco += ((range - prob) + 19 / 20);
          else
             calc_thaco -= ((prob-range) + 19 / 20);
         }
      // Skill level increase damage
      if (GET_SKILL(ch,skill) >= 60)
         dam += ((GET_SKILL(ch,skill) - 50) / 10);
     }

  // not can see (blind, dark, etc)
  if (!CAN_SEE(ch, victim))
     calc_thaco += 4;
  if (!CAN_SEE(victim, ch))
     calc_thaco -= 4;

  // bless
  if (AFF_FLAGGED(ch, AFF_BLESS))
     {calc_thaco -= 2;
     }
  // curse
  if (AFF_FLAGGED(ch, AFF_CURSE))
     {calc_thaco += 2;
      dam        -= 1;
     }

  // some protects
  if (AFF_FLAGGED(victim, AFF_PROTECT_EVIL) && IS_EVIL(ch))
     calc_thaco  += 2;
  if (AFF_FLAGGED(victim, AFF_PROTECT_GOOD) && IS_GOOD(ch))
     calc_thaco  += 2;

  // "Dirty" methods for battle
  if (skill != SKILL_THROW && skill != SKILL_BACKSTAB)
     {prob = (GET_SKILL(ch,skill)     + cha_app[GET_REAL_CHA(ch)].illusive) -
             (GET_SKILL(victim,skill) + int_app[GET_REAL_INT(victim)].observation);
      if ( prob >= 30 && !GET_AF_BATTLE(victim, EAF_AWAKE) &&
          (IS_NPC(ch) || !GET_AF_BATTLE(ch, EAF_PUNCTUAL))
         )
         { calc_thaco -= (GET_SKILL(ch,skill) - GET_SKILL(victim,skill) > 60 ? 2 : 1);
           if (!IS_NPC(victim))
              dam += (prob >= 70 ? 3 : (prob >= 50 ? 2 : 1));
         }
     }

  // AWAKE style for victim
  if (GET_AF_BATTLE(victim, EAF_AWAKE)    &&
      !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
      !GET_MOB_HOLD(victim)               &&
      train_skill(victim,SKILL_AWAKE,skill_info[SKILL_AWAKE].max_percent,ch) >=
                  number(1,skill_info[SKILL_AWAKE].max_percent))
     {dam        -= IS_NPC(ch) ? 5 : 5;
      calc_thaco += IS_NPC(ch) ? 2 : 4;
     }

  // Calculate the THAC0 of the attacker
  if (!IS_NPC(ch))
     {calc_thaco += thaco((int) GET_CLASS(ch), (int) GET_LEVEL(ch));
      calc_thaco -= (str_app[STRENGTH_APPLY_INDEX(ch)].tohit + GET_REAL_HR(ch));
      calc_thaco += ((diceroll - weapon + 9) / 10);
     }
  else		
     {calc_thaco += (20 - (weapon+9)/10);
      calc_thaco -= (str_app[STRENGTH_APPLY_INDEX(ch)].tohit + GET_REAL_HR(ch));
     }
       //  log("Attacker : %s", GET_NAME(ch));
       //  log("THAC0    : %d ", calc_thaco);

  // Calculate the raw armor including magic armor.  Lower AC is better.
  victim_ac += compute_armor_class(victim);
  victim_ac /= 10;
  if (GET_POS(victim) < POS_FIGHTING)
     victim_ac += 2;
  if (GET_POS(victim) < POS_RESTING)
     victim_ac += 2;
  if (AFF_FLAGGED(victim, AFF_STAIRS) && victim_ac < 10)
     victim_ac = (victim_ac + 10) >> 1;


       //  log("Target : %s", GET_NAME(victim));
       //  log("AC     : %d ", victim_ac);

  // roll the die and take your chances...
  diceroll   = number(1, 20);

       // log("THAC0 - %d  AC - %d Diceroll - %d",calc_thaco,victim_ac, diceroll);
       // sprintf(buf,"THAC0 - %d  Diceroll - %d    AC - %d(%d)\r\n", calc_thaco, diceroll,victim_ac,compute_armor_class(victim));
       // send_to_char(buf,ch);
       //  send_to_char(buf,victim);

  // decide whether this is a hit or a miss
  if (( ((diceroll < 20) && AWAKE(victim)) &&
        ((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac))) )
    {/* the attacker missed the victim */
     extdamage(ch, victim, 0, w_type, wielded, TRUE);
    }
  else
   {// blink
    if (AFF_FLAGGED(victim,AFF_BLINK) && number(1,100) <= 20)
       {sprintf(buf,"%sНа мгновение Вы исчезли из поля зрения противника.%s\r\n",
                CCINRM(victim,C_NRM),CCNRM(victim,C_NRM));
        send_to_char(buf,victim);
        extdamage(ch, victim, 0, w_type, wielded, TRUE);
	return;
       }

   // okay, we know the guy has been hit.  now calculate damage.

   // Start with the damage bonuses: the damroll and strength apply
   dam += str_app[STRENGTH_APPLY_INDEX(ch)].todam;
   dam += GET_REAL_DR(ch);

   if (IS_NPC(ch))
      {dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
      }

   if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
      {// Add weapon-based damage if a weapon is being wielded
       percent = dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
       if (!IS_NPC(ch))
          percent = MIN(percent, percent * GET_OBJ_CUR(wielded) /
	                         MAX(1, GET_OBJ_MAX(wielded)));
       dam += MAX(1,percent);
      }
   else
      {// If no weapon, add bare hand damage instead
       if (AFF_FLAGGED(ch, AFF_STONEHAND))
          {if (GET_CLASS(ch)==CLASS_WARRIOR)
              dam += number(5, 10+GET_LEVEL(ch)/5);
	  else
              dam += number(5, 10);
	  }
       else
       if (!IS_NPC(ch))
          {if (GET_CLASS(ch)==CLASS_WARRIOR)
              dam += number(1, 3+GET_LEVEL(ch)/5);
	  else
              dam += number(1, 3);
	  }
      }

   // Change victim, if protector present
   for (vict = world[IN_ROOM(victim)].people;
        vict && type != TYPE_NOPARRY;
        vict = vict->next_in_room)
       { if (PROTECTING(vict) == victim &&
             !AFF_FLAGGED(vict, AFF_STOPFIGHT) &&
             GET_WAIT(vict) <= 0     &&
             !GET_MOB_HOLD(vict)     &&
             GET_POS(vict) >= POS_FIGHTING)
            {percent = number(1,skill_info[SKILL_PROTECT].max_percent);
             prob    = calculate_skill(vict,SKILL_PROTECT,skill_info[SKILL_PROTECT].max_percent,victim);
	     improove_skill(vict,SKILL_PROTECT,prob >= percent,0);
             if (WAITLESS(vict))
                percent = prob;
             if (GET_GOD_FLAG(vict, GF_GODSCURSE))
                percent = 0;
             CLR_AF_BATTLE(vict, EAF_PROTECT);
             PROTECTING(vict) = NULL;
             if (prob < percent)
                {act("Вы не смогли прикрыть $N3.", FALSE, vict, 0, victim, TO_CHAR);
                 act("$N не смог$Q прикрыть Вас.", FALSE, victim, 0, vict, TO_CHAR);
                 act("$n не смог$q прикрыть $N3.", TRUE, vict, 0, victim, TO_NOTVICT);
                 prob = 3;
                }
             else
                {act("Вы героически прикрыли $N3, приняв удар на себя", FALSE, vict, 0, victim, TO_CHAR);
                 act("$N героически прикрыл$G Вас, приняв удар на себя", FALSE, victim, 0, vict, TO_CHAR);
                 act("$n героически прикрыл$g $N3, приняв удар на себя", TRUE, vict, 0, victim, TO_NOTVICT);
                 victim = vict;
                 prob   = 2;
                }
             if (!IS_IMMORTAL(vict) &&
                 !GET_GOD_FLAG(vict,GF_GODSLIKE))
                WAIT_STATE(vict, 2*PULSE_VIOLENCE);
             if (victim == vict)
                break;
            }
       }

     // Include a damage multiplier if victim isn't ready to fight:
     // Position sitting  1.5 x normal
     // Position resting  2.0 x normal
     // Position sleeping 2.5 x normal
     // Position stunned  3.0 x normal
     // Position incap    3.5 x normal
     // Position mortally 4.0 x normal
     //
     // Note, this is a hack because it depends on the particular
     // values of the POSITION_XXX constants.
     //
    if (GET_POS(ch) < POS_FIGHTING)
       dam -= (dam * (POS_FIGHTING - GET_POS(ch)) / 4);

    if (GET_POS(victim) < POS_FIGHTING)
       dam += (dam * (POS_FIGHTING - GET_POS(victim)) / 3);

    if (GET_MOB_HOLD(victim))
       dam += (dam >> 1);

    // Cut damage in half if victim has sanct, to a minimum 1
    if (AFF_FLAGGED(victim, AFF_PRISMATICAURA))
       dam *= 2;
    if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
       dam /= 2;

    // at least 1 hp damage min per hit
    dam = MAX(1, dam);

    if (weapon_pos)
       alt_equip(ch, weapon_pos, dam, 10);
    was_critic = FALSE;
    dam_critic = 0;

    if (type == SKILL_BACKSTAB)
       {dam *= (GET_COMMSTATE(ch) ? 25 : backstab_mult(GET_LEVEL(ch)));
        extdamage(ch, victim, dam, w_type, 0, TRUE);
        return;
       }
    else
    if (type == SKILL_THROW)
       {dam *= (GET_COMMSTATE(ch) ? 10 : (calculate_skill(ch,SKILL_THROW,skill_info[SKILL_THROW].max_percent,victim) + 19) / 20);
        extdamage(ch, victim, dam, w_type, 0, TRUE);
        return;
       }
    else
       {// Critical hits
        if (GET_AF_BATTLE(ch, EAF_PUNCTUAL) &&
            GET_WAIT(ch) <= 0 &&
            diceroll >= 18 - GET_MOB_HOLD(victim)
           )
           {percent = train_skill(ch,SKILL_PUNCTUAL,skill_info[SKILL_PUNCTUAL].max_percent,victim);
            if (!WAITLESS(ch))
               WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
            if (percent >= number(1, skill_info[SKILL_PUNCTUAL].max_percent) &&
                ( calc_thaco - diceroll < victim_ac - 5 ||
                  percent >= skill_info[SKILL_PUNCTUAL].max_percent
                )
               )
               {was_critic = TRUE;

                if (percent == skill_info[SKILL_PUNCTUAL].max_percent)
                   dam_critic = dice(2,8);
                else
                if (!wielded ||
                    weapon_app[GET_OBJ_WEIGHT(wielded)].shocking <
                    size_app[GET_POS_SIZE(victim)].shocking
                   )
                  dam_critic = dice(1,6);
                else
                if (weapon_app[GET_OBJ_WEIGHT(wielded)].shocking ==
                    size_app[GET_POS_SIZE(victim)].shocking
                   )
                  dam_critic = dice(2,4);
                else
                if (weapon_app[GET_OBJ_WEIGHT(wielded)].shocking <=
                    size_app[GET_POS_SIZE(victim)].shocking * 2
                   )
                  dam_critic = dice(3,4); // dice(2,6);
                else
                  dam_critic = dice(4,4); // dice(2,8);
                if (!WAITLESS(ch))
                   WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
               }
           }
        /**** обработаем ситуацию ЗАХВАТ */
        for (vict = world[IN_ROOM(ch)].people;
             vict && dam >= 0 && type != TYPE_NOPARRY;
             vict = vict->next_in_room)
            { if (TOUCHING(vict) == ch &&
                  !AFF_FLAGGED(vict, AFF_STOPFIGHT) &&
                  !AFF_FLAGGED(vict, AFF_STOPRIGHT) &&
                  GET_WAIT(vict) <= 0 &&
                  !GET_MOB_HOLD(vict) &&
                  (IS_IMMORTAL(vict) ||
                   IS_NPC(vict)      ||
                   GET_GOD_FLAG(vict, GF_GODSLIKE) ||
                   !(GET_EQ(vict,WEAR_WIELD)|| GET_EQ(vict,WEAR_BOTHS))
                  ) &&
                  GET_POS(vict) > POS_SLEEPING)
                 {percent = number(1,skill_info[SKILL_TOUCH].max_percent);
                  prob    = train_skill(vict,SKILL_TOUCH,skill_info[SKILL_TOUCH].max_percent,ch);
                  if (IS_IMMORTAL(vict) || GET_GOD_FLAG(vict, GF_GODSLIKE))
                     percent = prob;
                  if (GET_GOD_FLAG(vict, GF_GODSCURSE))
                     percent = 0;
                  CLR_AF_BATTLE(vict, EAF_TOUCH);
                  SET_AF_BATTLE(vict, EAF_USEDRIGHT);
                  TOUCHING(vict) = NULL;
                  if (prob < percent)
                     {act("Вы не смогли перехватить атаку $N1.", FALSE, vict, 0, ch, TO_CHAR);
                      act("$N не смог$Q перехватить Вашу атаку.", FALSE, ch, 0, vict, TO_CHAR);
                      act("$n не смог$q перехватить атаку $N1.", TRUE, vict, 0, ch, TO_NOTVICT);
                      prob = 2;
                     }
                  else
                     {act("Вы перехватили атаку $N1.", FALSE, vict, 0, ch, TO_CHAR);
                      act("$N перехватил$G Вашу атаку.", FALSE, ch, 0, vict, TO_CHAR);
                      act("$n перехватил$g атаку $N1.", TRUE, vict, 0, ch, TO_NOTVICT);
                      dam    = -1;
                      prob   = 1;
                     }
                  if (!WAITLESS(vict))
                     WAIT_STATE(vict, prob*PULSE_VIOLENCE);
                 }
            }

        /**** Обработаем команду   УКЛОНИТЬСЯ */
        if (dam > 0 && type != TYPE_NOPARRY &&
            GET_AF_BATTLE(victim, EAF_DEVIATE) &&
            GET_WAIT(victim) <= 0 &&
            !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
            GET_MOB_HOLD(victim) == 0 &&
	    BATTLECNTR(victim) <= (GET_LEVEL(victim) + 7 / 8)
	   )
           {range = number(1,skill_info[SKILL_DEVIATE].max_percent);
            prob  = train_skill(victim,SKILL_DEVIATE,skill_info[SKILL_DEVIATE].max_percent,ch);
            if (GET_GOD_FLAG(victim,GF_GODSCURSE))
               prob = 0;
            prob  = (int)(prob * 100 / range);
            if (prob < 60)
               {act("Вы не смогли уклонитьcя от атаки $N1", FALSE, victim, 0, ch, TO_CHAR);
                act("$N не сумел$G уклониться от Вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
                act("$n не сумел$g уклониться от атаки $N1", TRUE, victim, 0, ch, TO_NOTVICT);
               }
            else
            if (prob < 100)
               {act("Вы немного уклонились от атаки $N1", FALSE, victim, 0, ch, TO_CHAR);
                act("$N немного уклонил$U от Вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
                act("$n немного уклонил$u от атаки $N1", TRUE, victim, 0, ch, TO_NOTVICT);
                dam  = (int)(dam/1.5);
               }
            else
            if (prob < 200)
               {act("Вы частично уклонились от атаки $N1", FALSE, victim, 0, ch, TO_CHAR);
                act("$N частично уклонил$U от Вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
                act("$n частично уклонил$u от атаки $N1", TRUE, victim, 0, ch, TO_NOTVICT);
                dam  = (int)(dam/2);
               }
            else
               {act("Вы уклонились от атаки $N1", FALSE, victim, 0, ch, TO_CHAR);
                act("$N уклонил$U от Вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
                act("$n уклонил$u от атаки $N1", TRUE, victim, 0, ch, TO_NOTVICT);
                dam  = -1;
               }
	    BATTLECNTR(victim)++;
           }
        else
        /**** обработаем команду  ПАРИРОВАТЬ */
        if (dam > 0 && type != TYPE_NOPARRY &&
            GET_AF_BATTLE(victim, EAF_PARRY) &&
            !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
            !AFF_FLAGGED(victim, AFF_STOPRIGHT) &&
            !AFF_FLAGGED(victim, AFF_STOPLEFT) &&
            GET_WAIT(victim) <= 0 &&
            GET_MOB_HOLD(victim) == 0)
           {if (!(
                  (GET_EQ(victim,WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(victim,WEAR_WIELD)) == ITEM_WEAPON &&
                   GET_EQ(victim,WEAR_HOLD)  && GET_OBJ_TYPE(GET_EQ(victim,WEAR_HOLD))  == ITEM_WEAPON    ) ||
                   IS_NPC(victim) ||
                   IS_IMMORTAL(victim) ||
                   GET_GOD_FLAG(victim,GF_GODSLIKE)))
               {send_to_char("У Вас нечем отклонить атаку противника\r\n",victim);
	        CLR_AF_BATTLE(victim,EAF_PARRY);
               }
            else
               {range = number(1,skill_info[SKILL_PARRY].max_percent);
                prob  = train_skill(victim,SKILL_PARRY,skill_info[SKILL_PARRY].max_percent,ch);
                prob  = (int)(prob * 100 / range);

                if (prob < 70 ||
                    ((skill == SKILL_BOWS || w_type == TYPE_MAUL) && !IS_IMMORTAL(victim)))
                   {act("Вы не смогли отбить атаку $N1", FALSE,victim,0,ch,TO_CHAR);
                    act("$N не сумел$G отбить Вашу атаку", FALSE,ch,0,victim,TO_CHAR);
                    act("$n не сумел$g отбить атаку $N1", TRUE,victim,0,ch,TO_NOTVICT);
                    prob = 2;
                    SET_AF_BATTLE(victim, EAF_USEDLEFT);		
                   }
                else
                if (prob < 100)
                   {act("Вы немного отклонили атаку $N1",FALSE,victim,0,ch,TO_CHAR);
                    act("$N немного отклонил$G Вашу атаку",FALSE,ch,0,victim,TO_CHAR);
                    act("$n немного отклонил$g атаку $N1",TRUE,victim,0,ch,TO_NOTVICT);
                    alt_equip(victim, number(0,2) ? WEAR_WIELD : WEAR_HOLD, dam, 10);
                    prob = 1;
                    dam  = (int)(dam / 1.5);
                    SET_AF_BATTLE(victim, EAF_USEDLEFT);		
                   }
                else
                if (prob < 170)
                   {act("Вы частично отклонили атаку $N1",FALSE,victim,0,ch,TO_CHAR);
                    act("$N частично отклонил$G Вашу атаку",FALSE,ch,0,victim,TO_CHAR);
                    act("$n частично отклонил$g атаку $N1",TRUE,victim,0,ch,TO_NOTVICT);
                    alt_equip(victim, number(0,2) ? WEAR_WIELD : WEAR_HOLD, dam, 15);
                    prob = 0;
                    dam  = (int)(dam / 2);
                    SET_AF_BATTLE(victim, EAF_USEDLEFT);		
                   }
                else
                   {act("Вы полностью отклонили атаку $N1",FALSE,victim,0,ch,TO_CHAR);
                    act("$N полностью отклонил$G Вашу атаку",FALSE,ch,0,victim,TO_CHAR);
                    act("$n полностью отклонил$g атаку $N1",TRUE,victim,0,ch,TO_NOTVICT);
                    alt_equip(victim, number(0,2) ? WEAR_WIELD : WEAR_HOLD, dam, 25);
                    prob = 0;
                    dam  = -1;
                   }
                if (!WAITLESS(ch) && prob)
                   WAIT_STATE(victim, PULSE_VIOLENCE * prob);
                CLR_AF_BATTLE(victim, EAF_PARRY);
               }
           }
        else
        /**** обработаем команду  ВЕЕРНАЯ ЗАЩИТА */
        if (dam > 0 && type != TYPE_NOPARRY &&
            GET_AF_BATTLE(victim, EAF_MULTYPARRY) &&
            !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
            !AFF_FLAGGED(victim, AFF_STOPRIGHT) &&
            !AFF_FLAGGED(victim, AFF_STOPLEFT) &&
            BATTLECNTR(victim) < (GET_LEVEL(victim) + 4) / 5 &&
            GET_WAIT(victim) <= 0 &&
            GET_MOB_HOLD(victim) == 0)
           {if (!(
                  (GET_EQ(victim,WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(victim,WEAR_WIELD)) == ITEM_WEAPON &&
                   GET_EQ(victim,WEAR_HOLD)  && GET_OBJ_TYPE(GET_EQ(victim,WEAR_HOLD))  == ITEM_WEAPON    ) ||
                   IS_NPC(victim) ||
                   IS_IMMORTAL(victim) ||
                   GET_GOD_FLAG(victim,GF_GODSLIKE)))
               send_to_char("У Вас нечем отклонять атаки противников\r\n",victim);
            else
               {range = number(1,skill_info[SKILL_MULTYPARRY].max_percent) + 10 * BATTLECNTR(victim);
                prob  = train_skill(victim,SKILL_MULTYPARRY,skill_info[SKILL_MULTYPARRY].max_percent + BATTLECNTR(ch)*10,ch);
                prob  = (int)(prob * 100 / range);
                if ((skill == SKILL_BOWS || w_type == TYPE_MAUL) && !IS_IMMORTAL(victim))
                   prob = 0;
                else
                   BATTLECNTR(victim)++;

                if (prob < 50)
                   {act("Вы не смогли отбить атаку $N1", FALSE,victim,0,ch,TO_CHAR);
                    act("$N не сумел$G отбить Вашу атаку", FALSE,ch,0,victim,TO_CHAR);
                    act("$n не сумел$g отбить атаку $N1", TRUE,victim,0,ch,TO_NOTVICT);
                   }
                else
                if (prob < 90)
                   {act("Вы немного отклонили атаку $N1",FALSE,victim,0,ch,TO_CHAR);
                    act("$N немного отклонил$G Вашу атаку",FALSE,ch,0,victim,TO_CHAR);
                    act("$n немного отклонил$g атаку $N1",TRUE,victim,0,ch,TO_NOTVICT);
                    alt_equip(victim, number(0,2) ? WEAR_WIELD : WEAR_HOLD, dam, 10);
                    dam  = (int)(dam / 1.5);
                   }
                else
                if (prob < 180)
                   {act("Вы частично отклонили атаку $N1",FALSE,victim,0,ch,TO_CHAR);
                    act("$N частично отклонил$G Вашу атаку",FALSE,ch,0,victim,TO_CHAR);
                    act("$n частично отклонил$g атаку $N1",TRUE,victim,0,ch,TO_NOTVICT);
                    alt_equip(victim, number(0,2) ? WEAR_WIELD : WEAR_HOLD, dam, 15);
                    dam  = (int)(dam / 2);
                   }
                else
                   {act("Вы полностью отклонили атаку $N1",FALSE,victim,0,ch,TO_CHAR);
                    act("$N полностью отклонил$G Вашу атаку",FALSE,ch,0,victim,TO_CHAR);
                    act("$n полностью отклонил$g атаку $N1",TRUE,victim,0,ch,TO_NOTVICT);
                    alt_equip(victim, number(0,2) ? WEAR_WIELD : WEAR_HOLD, dam, 25);
                    dam  = -1;
                   }
               }
           }
        else
        /**** Обработаем команду   БЛОКИРОВАТЬ */
        if (dam > 0 && type != TYPE_NOPARRY &&
            GET_AF_BATTLE(victim, EAF_BLOCK) &&
            !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
            !AFF_FLAGGED(victim, AFF_STOPLEFT) &&
            GET_WAIT(victim) <= 0 &&
            GET_MOB_HOLD(victim) == 0 &&
            BATTLECNTR(victim) < (GET_LEVEL(victim) + 8) / 9
           )
           {
            if (!(GET_EQ(victim, WEAR_SHIELD) ||
                  IS_NPC(victim)              ||
                  IS_IMMORTAL(victim)         ||
                  GET_GOD_FLAG(victim,GF_GODSLIKE)))
               send_to_char("У Вас нечем отразить атаку противника\r\n",ch);
            else
               {range = number(1,skill_info[SKILL_BLOCK].max_percent);
                prob  = train_skill(victim,SKILL_BLOCK,skill_info[SKILL_BLOCK].max_percent,ch);
                prob  = (int)(prob * 100 / range);
		BATTLECNTR(victim)++;
                if (prob < 100)
                   {act("Вы не смогли отразить атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
                    act("$N не сумел$G отразить Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
                    act("$n не сумел$g отразить атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT);
                   }
                else
                if (prob < 150)
                   {act("Вы немного отразили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
                    act("$N немного отразил$G Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
                    act("$n немного отразил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT);
                    alt_equip(victim,WEAR_SHIELD,dam,10);
                    dam  = (int)(dam/1.5);
                   }
                else
                if (prob < 250)
                   {act("Вы частично отразили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
                    act("$N частично отразил$G Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
                    act("$n частично отразил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT);
                    alt_equip(victim,WEAR_SHIELD,dam,15);
                    dam  = (int)(dam/2);
                   }
                else
                   {act("Вы полностью отразили атаку $N1", FALSE, victim, 0, ch, TO_CHAR);
                    act("$N полностью отразил$G Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
                    act("$n полностью отразил$g атаку $N1", TRUE, victim, 0, ch, TO_NOTVICT);
                    alt_equip(victim,WEAR_SHIELD,dam,25);
                    dam  = -1;
                   }
               }
           };

        extdamage(ch, victim, dam, w_type, wielded, TRUE);
        was_critic = FALSE;
        dam_critic = 0;
       }
   }

  /* check if the victim has a hitprcnt trigger */
  hitprcnt_mtrigger(victim);
}


int GET_MAXDAMAGE(struct char_data *ch)
{
  if (AFF_FLAGGED(ch,AFF_HOLD))
     return 0;
  else
     return GET_DAMAGE(ch);
}

int GET_MAXCASTER(struct char_data *ch)
{ if (AFF_FLAGGED(ch,AFF_HOLD) || AFF_FLAGGED(ch,AFF_SIELENCE) || GET_WAIT(ch) > 0)
     return 0;
  else
     return IS_IMMORTAL(ch) ? 1 : GET_CASTER(ch);
}

#define GET_HP_PERC(ch) ((int)(GET_HIT(ch) * 100 / GET_MAX_HIT(ch)))
#define POOR_DAMAGE  15
#define POOR_CASTER  5
#define MAX_PROBES   0
#define SpINFO       spell_info[i]

int in_same_battle(struct char_data *npc, struct char_data *pc, int opponent)
{ int ch_friend_npc, ch_friend_pc, vict_friend_npc, vict_friend_pc;
  struct  char_data *ch, *vict, *npc_master, *pc_master, *ch_master, *vict_master;

  if (npc == pc)
     return (!opponent);
  if (FIGHTING(npc) == pc)    // NPC fight PC - opponent
     return (opponent);
  if (FIGHTING(pc)  == npc)   // PC fight NPC - opponent
     return (opponent);
  if (FIGHTING(npc) && FIGHTING(npc) == FIGHTING(pc))
     return (!opponent);     // Fight same victim - friend
  if (AFF_FLAGGED(pc,AFF_HORSE) || AFF_FLAGGED(pc,AFF_CHARM))
     return (opponent);

  npc_master = npc->master ? npc->master : npc;
  pc_master  = pc->master  ? pc->master : pc;

  for (ch = world[IN_ROOM(npc)].people; ch; ch = ch->next)
      {if (!FIGHTING(ch))
          continue;
       ch_master = ch->master ? ch->master : ch;
       ch_friend_npc = (ch_master == npc_master) ||
                       (IS_NPC(ch) && IS_NPC(npc) &&
                        !AFF_FLAGGED(ch, AFF_CHARM) && !AFF_FLAGGED(npc, AFF_CHARM) &&
                        !AFF_FLAGGED(ch, AFF_HORSE) && !AFF_FLAGGED(npc, AFF_HORSE)
                       );
       ch_friend_pc  = (ch_master == pc_master) ||
                       (IS_NPC(ch) && IS_NPC(pc) &&
                        !AFF_FLAGGED(ch, AFF_CHARM) && !AFF_FLAGGED(pc, AFF_CHARM) &&
                        !AFF_FLAGGED(ch, AFF_HORSE) && !AFF_FLAGGED(pc, AFF_HORSE)
                       );
       if (FIGHTING(ch) == pc &&
           ch_friend_npc)             // Friend NPC fight PC - opponent
          return (opponent);
       if (FIGHTING(pc) == ch &&
           ch_friend_npc)             // PC fight friend NPC - opponent
          return (opponent);
       if (FIGHTING(npc) == ch &&
           ch_friend_pc)              // NPC fight friend PC - opponent
          return (opponent);
       if (FIGHTING(ch)  == npc &&
           ch_friend_pc)              // Friend PC fight NPC - opponent
          return (opponent);
       vict        = FIGHTING(ch);
       vict_master = vict->master ? vict->master : vict;
       vict_friend_npc = (vict_master == npc_master) ||
                         (IS_NPC(vict) && IS_NPC(npc) &&
                          !AFF_FLAGGED(vict, AFF_CHARM) && !AFF_FLAGGED(npc, AFF_CHARM) &&
                          !AFF_FLAGGED(vict, AFF_HORSE) && !AFF_FLAGGED(npc, AFF_HORSE)
                         );
       vict_friend_pc  = (vict_master == pc_master) ||
                         (IS_NPC(vict) && IS_NPC(pc) &&
                          !AFF_FLAGGED(vict, AFF_CHARM) && !AFF_FLAGGED(pc, AFF_CHARM) &&
                          !AFF_FLAGGED(vict, AFF_HORSE) && !AFF_FLAGGED(pc, AFF_HORSE)
                         );
       if (ch_friend_npc && vict_friend_pc)
          return (opponent);          // Friend NPC fight friend PC - opponent
       if (ch_friend_pc && vict_friend_npc)
          return (opponent);          // Friend PC fight friend NPC - opponent
      }

  return (!opponent);
}

struct char_data *find_friend_cure(struct char_data *caster, int spellnum)
{struct char_data *vict, *victim = NULL;
 int    vict_val = 0, AFF_USED = 0;
 switch (spellnum)
 {case SPELL_CURE_LIGHT :
       AFF_USED = 80;
       break;
  case SPELL_CURE_SERIOUS :
       AFF_USED = 70;
       break;
  case SPELL_EXTRA_HITS :
  case SPELL_CURE_CRITIC :
       AFF_USED = 50;
       break;
  case SPELL_HEAL :
  case SPELL_GROUP_HEAL :
       AFF_USED = 30;
       break;
 }

 if (AFF_FLAGGED(caster,AFF_CHARM) && AFF_FLAGGED(caster,AFF_HELPER))
    {if (GET_HP_PERC(caster) < AFF_USED)
        return (caster);
     else
     if (caster->master                             &&
         !IS_NPC(caster->master)                    &&
	 CAN_SEE(caster, caster->master)            &&
	 IN_ROOM(caster->master) == IN_ROOM(caster) &&
         FIGHTING(caster->master)                   &&
	 GET_HP_PERC(caster->master) < AFF_USED
	)
        return (caster->master);
     return (NULL);
    }

 for (vict = world[IN_ROOM(caster)].people; AFF_USED && vict; vict = vict->next_in_room)
     {if (!IS_NPC(vict)               ||
          AFF_FLAGGED(vict,AFF_CHARM) ||
	  !CAN_SEE(caster, vict)
	 )
         continue;
      if (!FIGHTING(vict) && !MOB_FLAGGED(vict, MOB_HELPER))
         continue;
      if (GET_HP_PERC(vict) < AFF_USED && (!victim || vict_val > GET_HP_PERC(vict)))
         {victim   = vict;
          vict_val = GET_HP_PERC(vict);
          if (GET_REAL_INT(caster) < number(10,20))
             break;
         }
     }
 return (victim);
}

struct char_data *find_friend(struct char_data *caster, int spellnum)
{struct char_data *vict, *victim = NULL;
 int    vict_val = 0, AFF_USED = 0, spellreal = -1;
 switch (spellnum)
 {case SPELL_CURE_BLIND :
       SET_BIT(AFF_USED, AFF_BLIND);
       break;
  case SPELL_REMOVE_POISON :
       SET_BIT(AFF_USED, AFF_POISON);
       break;
  case SPELL_REMOVE_HOLD :
       SET_BIT(AFF_USED, AFF_HOLD);
       break;
  case SPELL_REMOVE_CURSE :
       SET_BIT(AFF_USED, AFF_CURSE);
       break;
  case SPELL_REMOVE_SIELENCE :
       SET_BIT(AFF_USED, AFF_SIELENCE);
       break;
  case SPELL_CURE_PLAQUE :
       spellreal = SPELL_PLAQUE;
       break;

  }
 if (AFF_FLAGGED(caster,AFF_CHARM) && AFF_FLAGGED(caster,AFF_HELPER))
    {if (AFF_FLAGGED(caster, AFF_USED) ||
         affected_by_spell(caster,spellreal)
	)
        return (caster);
     else
     if (caster->master                             &&
         !IS_NPC(caster->master)                    &&
	 CAN_SEE(caster, caster->master)            &&
	 IN_ROOM(caster->master) == IN_ROOM(caster) &&
         FIGHTING(caster->master)                   &&
         (AFF_FLAGGED(caster->master, AFF_USED) ||
	  affected_by_spell(caster->master,spellreal)
	 )
        )
        return (caster->master);
     return (NULL);
    }

  for (vict = world[IN_ROOM(caster)].people; AFF_USED && vict; vict = vict->next_in_room)
      {if (!IS_NPC(vict) || AFF_FLAGGED(vict,AFF_CHARM) || !CAN_SEE(caster, vict))
          continue;
       if (!AFF_FLAGGED(vict, AFF_USED))
          continue;
       if (!FIGHTING(vict) && !MOB_FLAGGED(vict, MOB_HELPER))
          continue;
       if (!victim || vict_val < GET_MAXDAMAGE(vict))
          {victim   = vict;
           vict_val = GET_MAXDAMAGE(vict);
           if (GET_REAL_INT(caster) < number(10,20))
              break;
           }
      }
  return (victim);
}

struct char_data  *find_caster(struct char_data *caster, int spellnum)
{struct char_data *vict = NULL, *victim = NULL;
 int    vict_val = 0, AFF_USED, spellreal = -1;
 AFF_USED = 0;
 switch (spellnum)
 {case SPELL_CURE_BLIND :
       SET_BIT(AFF_USED, AFF_BLIND);
       break;
  case SPELL_REMOVE_POISON :
       SET_BIT(AFF_USED, AFF_POISON);
       break;
  case SPELL_REMOVE_HOLD :
       SET_BIT(AFF_USED, AFF_HOLD);
       break;
  case SPELL_REMOVE_CURSE :
       SET_BIT(AFF_USED, AFF_CURSE);
       break;
  case SPELL_REMOVE_SIELENCE :
       SET_BIT(AFF_USED, AFF_SIELENCE);
       break;
  case SPELL_CURE_PLAQUE :
       spellreal = SPELL_PLAQUE;
       break;
 }

 if (AFF_FLAGGED(caster,AFF_CHARM) && AFF_FLAGGED(caster,AFF_HELPER))
    {if (AFF_FLAGGED(caster, AFF_USED) ||
         affected_by_spell(caster,spellreal)
	)
        return (caster);
     else
     if (caster->master                             &&
         !IS_NPC(caster->master)                    &&
	 CAN_SEE(caster, caster->master)            &&
	 IN_ROOM(caster->master) == IN_ROOM(caster) &&
         FIGHTING(caster->master)                   &&
         (AFF_FLAGGED(caster->master, AFF_USED) ||
	  affected_by_spell(caster->master,spellreal)
	 )
        )
        return (caster->master);
     return (NULL);
    }

 for (vict = world[IN_ROOM(caster)].people; AFF_USED && vict; vict = vict->next_in_room)
     {if (!IS_NPC(vict) || AFF_FLAGGED(vict,AFF_CHARM) || !CAN_SEE(caster, vict))
         continue;
      if (!AFF_FLAGGED(vict, AFF_USED))
         continue;
      if (!FIGHTING(vict) && !MOB_FLAGGED(vict, MOB_HELPER))
         continue;
      if (!victim || vict_val < GET_MAXCASTER(vict))
         {victim   = vict;
          vict_val = GET_MAXCASTER(vict);
          if (GET_REAL_INT(caster) < number(10,20))
             break;
         }
     }
 return (victim);
}


struct  char_data *find_affectee(struct char_data *caster, int spellnum)
{struct char_data *vict, *victim = NULL;
 int    vict_val = 0, spellreal = spellnum;

 if (spellreal == SPELL_GROUP_ARMOR)
    spellreal = SPELL_ARMOR;
 else
 if (spellreal == SPELL_GROUP_STRENGTH)
    spellreal = SPELL_STRENGTH;
 else
 if (spellreal == SPELL_GROUP_BLESS)
    spellreal = SPELL_BLESS;
 else
 if (spellreal == SPELL_GROUP_HASTE)
    spellreal = SPELL_HASTE;

 if (AFF_FLAGGED(caster,AFF_CHARM) && AFF_FLAGGED(caster,AFF_HELPER))
    {if (!affected_by_spell(caster,spellreal))
        return (caster);
     else
     if (caster->master                             &&
         !IS_NPC(caster->master)                    &&
	 CAN_SEE(caster, caster->master)            &&
	 IN_ROOM(caster->master) == IN_ROOM(caster) &&
         FIGHTING(caster->master)                   &&
	 !affected_by_spell(caster->master,spellreal)
	)
        return (caster->master);
     return (NULL);
    }

 if (GET_REAL_INT(caster) > number(5,15))
    for (vict = world[IN_ROOM(caster)].people; vict; vict = vict->next_in_room)
        {if (!IS_NPC(vict) || AFF_FLAGGED(vict,AFF_CHARM) || !CAN_SEE(caster, vict))
            continue;
         if (!FIGHTING(vict) || AFF_FLAGGED(vict, AFF_HOLD) ||
             affected_by_spell(vict,spellreal))
            continue;
         if (!victim || vict_val < GET_MAXDAMAGE(vict))
            {victim = vict;
             vict_val = GET_MAXDAMAGE(vict);
            }
        }
 if (!victim && !affected_by_spell(caster,spellreal))
    victim = caster;

 return (victim);
}

struct  char_data *find_opp_affectee(struct char_data *caster, int spellnum)
{struct char_data *vict, *victim = NULL;
 int    vict_val = 0, spellreal = spellnum;

 if (spellreal == SPELL_POWER_HOLD || spellreal == SPELL_MASS_HOLD)
    spellreal = SPELL_HOLD;
 else
 if (spellreal == SPELL_POWER_BLINDNESS || spellreal == SPELL_MASS_BLINDNESS)
    spellreal = SPELL_BLINDNESS;
 else
 if (spellreal == SPELL_POWER_SIELENCE || spellreal == SPELL_MASS_SIELENCE)
    spellreal = SPELL_SIELENCE;
 else
 if (spellreal == SPELL_MASS_CURSE)
    spellreal = SPELL_CURSE;
 else
 if (spellreal == SPELL_MASS_SLOW)
    spellreal = SPELL_SLOW;

 if (GET_REAL_INT(caster) > number(10,20))
    for (vict = world[caster->in_room].people; vict; vict = vict->next_in_room)
        {if ((IS_NPC(vict) && !AFF_FLAGGED(vict,AFF_CHARM)) ||
	     !CAN_SEE(caster, vict)
	    )
            continue;
         if ((!FIGHTING(vict) && (GET_REAL_INT(caster) < number(20,27) || !in_same_battle(caster,vict,TRUE))) ||
             AFF_FLAGGED(vict, AFF_HOLD) || affected_by_spell(vict,spellreal))
            continue;
         if (!victim || vict_val < GET_MAXDAMAGE(vict))
            {victim   = vict;
             vict_val = GET_MAXDAMAGE(vict);
            }
        }
	
 if (!victim && FIGHTING(caster) && !affected_by_spell(FIGHTING(caster),spellreal))
    victim = FIGHTING(caster);
 return (victim);
}

struct  char_data *find_opp_caster(struct char_data *caster)
{ struct char_data *vict=NULL, *victim = NULL;
  int    vict_val = 0;

  for (vict = world[IN_ROOM(caster)].people; vict; vict = vict->next_in_room)
      {if (IS_NPC(vict) && !AFF_FLAGGED(vict,AFF_CHARM))
          continue;
       if ((!FIGHTING(vict) && (GET_REAL_INT(caster) < number(15, 25) || !in_same_battle(caster,vict,TRUE))) ||
           AFF_FLAGGED(vict, AFF_HOLD)    ||
	   AFF_FLAGGED(vict,AFF_SIELENCE) ||
	   (!CAN_SEE(caster, vict) && FIGHTING(caster) != vict)
	  )
          continue;
       if (vict_val < GET_MAXCASTER(vict))
          {victim   = vict;
           vict_val = GET_MAXCASTER(vict);
          }
      }
  return (victim);
}

struct  char_data *find_damagee(struct char_data *caster)
{struct char_data *vict, *victim = NULL;
 int    vict_val = 0;

 if (GET_REAL_INT(caster) > number(10, 20))
    for (vict = world[IN_ROOM(caster)].people; vict; vict = vict->next_in_room)
        {if ((IS_NPC(vict) && !AFF_FLAGGED(vict,AFF_CHARM)) ||
	     !CAN_SEE(caster, vict)
	    )
            continue;
         if ((!FIGHTING(vict) && (GET_REAL_INT(caster) < number(20,27) || !in_same_battle(caster,vict,TRUE))) ||
             AFF_FLAGGED(vict, AFF_HOLD))
            continue;
         if (GET_REAL_INT(caster) >= number(25,30))
            {if (!victim || vict_val < GET_MAXCASTER(vict))
                {victim = vict;
                 vict_val = GET_MAXCASTER(vict);
                }
            }
         else
         if (!victim || vict_val < GET_MAXDAMAGE(vict))
            {victim   = vict;
             vict_val = GET_MAXDAMAGE(vict);
            }
        }
 if (!victim)
    victim = FIGHTING(caster);

 return (victim);
}

struct  char_data *find_minhp(struct char_data *caster)
{ struct char_data *vict, *victim = NULL;
  int    vict_val = 0;

  if (GET_REAL_INT(caster) > number(10,20))
     for (vict = world[IN_ROOM(caster)].people; vict; vict = vict->next_in_room)
         {if ((IS_NPC(vict) && !AFF_FLAGGED(vict,AFF_CHARM)) ||
	      !CAN_SEE(caster, vict)
	     )
            continue;
          if (!FIGHTING(vict) &&
              (GET_REAL_INT(caster) < number(20,27) || !in_same_battle(caster, vict, TRUE))
	     )
             continue;
          if (!victim || vict_val > GET_HIT(vict))
             {victim = vict;
              vict_val = GET_HIT(vict);
             }
         }
  if (!victim)
     victim = FIGHTING(caster);

  return (victim);
}

struct  char_data *find_cure(struct char_data *caster, struct char_data *patient, int *spellnum)
{if (GET_HP_PERC(patient) <= number(20,33))
    {if (GET_SPELL_MEM(caster,SPELL_EXTRA_HITS))
        *spellnum = SPELL_EXTRA_HITS;
     else
     if (GET_SPELL_MEM(caster,SPELL_HEAL))
        *spellnum = SPELL_HEAL;
     else
     if (GET_SPELL_MEM(caster,SPELL_CURE_CRITIC))
        *spellnum = SPELL_CURE_CRITIC;
     else
     if (GET_SPELL_MEM(caster,SPELL_GROUP_HEAL))
        *spellnum = SPELL_GROUP_HEAL;
    }
 else
 if (GET_HP_PERC(patient) <= number(50,65))
    {if (GET_SPELL_MEM(caster,SPELL_CURE_CRITIC))
        *spellnum = SPELL_CURE_CRITIC;
     else
     if (GET_SPELL_MEM(caster,SPELL_CURE_SERIOUS))
        *spellnum = SPELL_CURE_SERIOUS;
     else
     if (GET_SPELL_MEM(caster,SPELL_CURE_LIGHT))
        *spellnum = SPELL_CURE_LIGHT;
    }
 if (*spellnum)
    return (patient);
 else
    return (NULL);
}

void    mob_casting (struct char_data * ch)
{ struct char_data  *victim;
  char   battle_spells[MAX_STRING_LENGTH];
  int    lag=GET_WAIT(ch), i, spellnum, spells, sp_num;
  struct obj_data   *item;

  if (AFF_FLAGGED(ch, AFF_CHARM)    ||
      AFF_FLAGGED(ch, AFF_HOLD)     ||
      AFF_FLAGGED(ch, AFF_SIELENCE) ||
      lag > 0
     )
     return;

  memset(&battle_spells,0,sizeof(battle_spells));
  for (i=1,spells=0;i<=MAX_SPELLS;i++)
      if (GET_SPELL_MEM(ch,i) && IS_SET(SpINFO.routines,NPC_CALCULATE))
         battle_spells[spells++]  = i;

  for (item = ch->carrying;
       spells < MAX_STRING_LENGTH    &&
       item                          &&
       GET_CLASS(ch) != CLASS_ANIMAL &&
        !AFF_FLAGGED(ch, AFF_CHARM);
       item = item->next_content)
      switch(GET_OBJ_TYPE(item))
      { case ITEM_WAND:
        case ITEM_STAFF: if (GET_OBJ_VAL(item,2) > 0 &&
                             IS_SET(spell_info[GET_OBJ_VAL(item,3)].routines,NPC_CALCULATE))
                            battle_spells[spells++] = GET_OBJ_VAL(item,3);
                         break;
        case ITEM_POTION:
                         for (i = 1; i <= 3; i++)
                             if (IS_SET(spell_info[GET_OBJ_VAL(item,i)].routines,NPC_AFFECT_NPC|NPC_UNAFFECT_NPC|NPC_UNAFFECT_NPC_CASTER))
                                battle_spells[spells++] = GET_OBJ_VAL(item,i);
                         break;
        case ITEM_SCROLL:
                         for (i = 1; i <= 3; i++)
                             if (IS_SET(spell_info[GET_OBJ_VAL(item,i)].routines,NPC_CALCULATE))
                                battle_spells[spells++] = GET_OBJ_VAL(item,i);
                         break;
      }

  // перво-наперво  -  лечим себя
  spellnum = 0;
  victim   = find_cure(ch,ch,&spellnum);
  // Ищем рандомную заклинашку и цель для нее
  for (i = 0; !victim && spells && i < GET_REAL_INT(ch) / 5; i++)
      if (!spellnum && (spellnum = battle_spells[(sp_num = number(0,spells-1))]) &&
          spellnum > 0 && spellnum <= MAX_SPELLS)
         {// sprintf(buf,"$n using spell '%s', %d from %d",
          //         spell_name(spellnum), sp_num, spells);
          // act(buf,FALSE,ch,0,FIGHTING(ch),TO_VICT);
          if (spell_info[spellnum].routines & NPC_DAMAGE_PC_MINHP)
             {if (!AFF_FLAGGED(ch, AFF_CHARM))
                 victim = find_minhp(ch);
             }
          else
          if (spell_info[spellnum].routines & NPC_DAMAGE_PC)
             {if (!AFF_FLAGGED(ch, AFF_CHARM))
                 victim  = find_damagee(ch);
             }
          else
          if (spell_info[spellnum].routines & NPC_AFFECT_PC_CASTER)
             {if (!AFF_FLAGGED(ch, AFF_CHARM))
                 victim  = find_opp_caster(ch);
             }
          else
          if (spell_info[spellnum].routines & NPC_AFFECT_PC)
             {if (!AFF_FLAGGED(ch, AFF_CHARM))
                 victim  = find_opp_affectee(ch, spellnum);
             }
          else
          if (spell_info[spellnum].routines & NPC_AFFECT_NPC)
             victim  = find_affectee(ch, spellnum);
          else
          if (spell_info[spellnum].routines & NPC_UNAFFECT_NPC_CASTER)
             victim  = find_caster(ch, spellnum);
          else
          if (spell_info[spellnum].routines & NPC_UNAFFECT_NPC)
             victim  = find_friend(ch, spellnum);
          else
          if (spell_info[spellnum].routines & NPC_DUMMY)
             victim  = find_friend_cure(ch, spellnum);
          else
             spellnum = 0;
         }
  if (spellnum && victim)
     { // Is this object spell ?
       for (item = ch->carrying;
            !AFF_FLAGGED(ch, AFF_CHARM) &&
	    item                        &&
	    GET_CLASS(ch) != CLASS_ANIMAL;
            item = item->next_content
           )
           switch(GET_OBJ_TYPE(item))
           { case ITEM_WAND:
             case ITEM_STAFF: if (GET_OBJ_VAL(item,2) > 0 &&
                                  GET_OBJ_VAL(item,3) == spellnum)
                                 {mag_objectmagic(ch,item,GET_NAME(victim));
                                  return;
                                 }
                              break;
             case ITEM_POTION:
                              for (i = 1; i <= 3; i++)
                                  if (GET_OBJ_VAL(item,i) == spellnum)
                                     {if (ch != victim)
                                         {obj_from_char(item);
                                          act("$n передал$g $o3 $N2.",FALSE,ch,item,victim,TO_ROOM);
                                          obj_to_char(item,victim);
                                         }
                                      else
                                         victim = ch;
                                      mag_objectmagic(victim,item,GET_NAME(victim));
                                      return;
                                     }
                              break;
             case ITEM_SCROLL:
                              for (i = 1; i <= 3; i++)
                                   if (GET_OBJ_VAL(item,i) == spellnum)
                                      {mag_objectmagic(ch,item,GET_NAME(victim));
                                       return;
                                      }
                              break;
            }

      cast_spell(ch,victim,NULL,spellnum);
     }
}

#define  MAY_LIKES(ch)   ((!AFF_FLAGGED(ch, AFF_CHARM) || AFF_FLAGGED(ch, AFF_HELPER)) && \
                          AWAKE(ch) && GET_WAIT(ch) <= 0)


/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{ struct char_data *ch, *vict, *caster, *damager;
  int    i, do_this, initiative, max_init = 0, min_init = 100,
         sk_use = 0, sk_num = 0;
  struct helper_data_type *helpee;

  // Step 0.0 Summons mob helpers

  for (ch = combat_list; ch; ch = next_combat_list)
      {next_combat_list = ch->next_fighting;
       // Extract battler if no opponent
       if (FIGHTING(ch) == NULL ||
           IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)) ||
           IN_ROOM(ch) == NOWHERE)
          {stop_fighting(ch,TRUE);
           continue;
          }
       if (GET_MOB_HOLD(ch)               ||
           !IS_NPC(ch)                    ||
           GET_WAIT(ch) > 0               ||
           GET_POS(ch) < POS_FIGHTING     ||
           AFF_FLAGGED(ch, AFF_CHARM)     ||
	   AFF_FLAGGED(ch, AFF_STOPFIGHT) ||
           AFF_FLAGGED(ch, AFF_SIELENCE))
          continue;

       if (!PRF_FLAGGED(FIGHTING(ch),PRF_NOHASSLE))		
          for (sk_use = 0, helpee = GET_HELPER(ch); helpee;
               helpee=helpee->next_helper)
              for (vict = character_list; vict; vict = vict->next)
                  {if (!IS_NPC(vict)                          ||
                       GET_MOB_VNUM(vict) != helpee->mob_vnum ||
                       AFF_FLAGGED(vict, AFF_HOLD)            ||
                       AFF_FLAGGED(vict, AFF_CHARM)           ||
                       AFF_FLAGGED(vict, AFF_BLIND)           ||
                       GET_WAIT(vict) > 0                     ||
                       GET_POS(vict) < POS_STANDING           ||
                       IN_ROOM(vict) == NOWHERE               ||
                       FIGHTING(vict)
                      )
                      continue;
                   if (!sk_use       &&
                       !(GET_CLASS(ch) == CLASS_ANIMAL ||
                        GET_CLASS(ch) == CLASS_BASIC_NPC)
                      )
                      act("$n воззвал$g : \"На помощь, мои верные соратники !\"",FALSE,ch,0,0,TO_ROOM);
                  if (IN_ROOM(vict) != IN_ROOM(ch))
                     {char_from_room(vict);
                      char_to_room(vict, IN_ROOM(ch));
                      act("$n прибыл$g на зов и вступил$g на помощь $N2.",FALSE,vict,0,ch,TO_ROOM);
                     }
                  else
                     act("$n вступил$g в битву на стороне $N1.",FALSE,vict,0,ch,TO_ROOM);
                  set_fighting(vict, FIGHTING(ch));
               };
      }


  // Step 1. Define initiative, mob casting and mob flag skills
  for (ch = combat_list; ch; ch = next_combat_list)
      {next_combat_list = ch->next_fighting;
       // Initialize initiative
       INITIATIVE(ch) = 0;
       BATTLECNTR(ch) = 0;
       SET_AF_BATTLE(ch,EAF_STAND);
       if (affected_by_spell(ch,SPELL_SLEEP))
          SET_AF_BATTLE(ch,EAF_SLEEP);
       if (GET_MOB_HOLD(ch)               ||
           AFF_FLAGGED(ch, AFF_STOPFIGHT) ||
           IN_ROOM(ch) == NOWHERE)
          continue;
	
       // Mobs stand up	
       if (IS_NPC(ch)                 &&
           GET_POS(ch) < POS_FIGHTING &&
           GET_POS(ch) > POS_STUNNED  &&
           GET_WAIT(ch) <= 0          &&
           !GET_MOB_HOLD(ch)          &&
           !AFF_FLAGGED(ch, AFF_SLEEP)
          )
          {GET_POS(ch) = POS_FIGHTING;
           act("$n встал$g на ноги.", TRUE, ch, 0, 0, TO_ROOM);
          }	

       // For NPC without lags and charms make it likes
       if (IS_NPC(ch) && MAY_LIKES(ch))
          {// Get weapon from room
           if (!AFF_FLAGGED(ch, AFF_CHARM) && npc_battle_scavenge(ch))
              {npc_wield(ch);
               npc_armor(ch);
              }
           // Set some flag-skills
           // 1) parry
           do_this = number(0,100);
           sk_use  = FALSE;
           if (!sk_use && do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_PARRY) )
              {SET_AF_BATTLE(ch,EAF_PARRY);
               sk_use = TRUE;
              }

           // 2) blocking
           do_this = number(0,100);
           if (!sk_use && do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_BLOCK))
              {SET_AF_BATTLE(ch,EAF_BLOCK);
               sk_use = TRUE;
              }

           // 3) multyparry
           do_this = number(0,100);
           if (!sk_use && do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_MULTYPARRY) )
              {SET_AF_BATTLE(ch,EAF_MULTYPARRY);
               sk_use = TRUE;
              }


           // 4) deviate
           do_this = number(0,100);
           if (!sk_use && do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_DEVIATE))
              {SET_AF_BATTLE(ch,EAF_DEVIATE);
               sk_use = TRUE;
              }

           // 5) stupor
           do_this = number(0,100);
           if (!sk_use && do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_STUPOR))
              {SET_AF_BATTLE(ch,EAF_STUPOR);
               sk_use = TRUE;
              }

           // 6) mighthit
           do_this = number(0,100);
           if (!sk_use && do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_STUPOR))
              {SET_AF_BATTLE(ch,EAF_MIGHTHIT);
               sk_use = TRUE;
              }

           // 7) styles
           do_this = number(0,100);
           if (do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_AWAKE) > number(1,101))
              SET_AF_BATTLE(ch,EAF_AWAKE);
           else
              CLR_AF_BATTLE(ch,EAF_AWAKE);
           do_this = number(0,100);
           if (do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_PUNCTUAL) > number(1,101))
              SET_AF_BATTLE(ch,EAF_PUNCTUAL);
           else
              CLR_AF_BATTLE(ch,EAF_PUNCTUAL);
          }

       initiative = size_app[GET_POS_SIZE(ch)].initiative;
       if ((i = number(1,10)) == 10)
          initiative -= 1;
       else
          initiative += i;

       initiative    += GET_INITIATIVE(ch);
       if (!IS_NPC(ch))
          switch (IS_CARRYING_W(ch) * 10 / MAX(1,CAN_CARRY_W(ch)))
          {case 10: case 9: case 8:
            initiative -= 2;
            break;
           case 7: case 6: case 5:
            initiative -= 1;
            break;
          }

       if (GET_AF_BATTLE(ch,EAF_AWAKE))
          initiative -= 2;
       if (GET_AF_BATTLE(ch,EAF_PUNCTUAL))
          initiative -= 1;
       if (AFF_FLAGGED(ch,AFF_SLOW))
          initiative -= 5;
       if (AFF_FLAGGED(ch,AFF_HASTE))
          initiative += 5;
       if (GET_WAIT(ch) > 0)
          initiative -= 1;
       if (calc_leadership(ch))
          initiative += 2;
       if (GET_AF_BATTLE(ch, EAF_SLOW))
          initiative  = 1;

       initiative = MAX(initiative, 1);
       INITIATIVE(ch) = initiative;
       SET_AF_BATTLE(ch, EAF_FIRST);
       max_init = MAX(max_init, initiative);
       min_init = MIN(min_init, initiative);
      }

  /* Process fighting           */
  for (initiative = max_init; initiative >= min_init; initiative--)
      for (ch = combat_list; ch; ch = next_combat_list)
          {next_combat_list = ch->next_fighting;
           if (INITIATIVE(ch) != initiative ||
               IN_ROOM(ch) == NOWHERE)
              continue;
           // If mob cast 'hold' when initiative setted
           if (AFF_FLAGGED(ch, AFF_HOLD) ||
               AFF_FLAGGED(ch, AFF_STOPFIGHT) ||
	       !AWAKE(ch)
	      )
              continue;
           // If mob cast 'fear', 'teleport', 'recall', etc when initiative setted
           if (!FIGHTING(ch) ||
               IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))
              )
              continue;

           if (IS_NPC(ch))
              {// Select extra_attack type
               // if (GET_AF_BATTLE(ch,EAF_MULTYPARRY))
               //    continue;

               // Cast spells
               if (MAY_LIKES(ch))
                  mob_casting(ch);
               if (!FIGHTING(ch) ||
                   IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))
                  )
                  continue;

               if (AFF_FLAGGED(ch, AFF_CHARM)         &&
                   AFF_FLAGGED(ch, AFF_HELPER)        &&
                   ch->master && !IS_NPC(ch->master)  &&
		   CAN_SEE(ch, ch->master)            &&
                   IN_ROOM(ch) == IN_ROOM(ch->master) &&
		   AWAKE(ch)
                  )
                  {for (vict = world[IN_ROOM(ch)].people; vict;
                        vict = vict->next_in_room)
                       if (FIGHTING(vict) == ch->master &&
                           vict           != ch)
                          break;
                   if (vict &&
                       (GET_SKILL(ch, SKILL_RESCUE) || GET_REAL_INT(ch) < number(0,100)
                       )
                      )
                      go_rescue(ch,ch->master,vict);
                   else
                   if (vict && GET_SKILL(ch, SKILL_PROTECT))
                      go_protect(ch,ch->master);
                  }
               else
               for (sk_num = 0, sk_use = GET_REAL_INT(ch)
	            /*,sprintf(buf,"{%d}-{%d}\r\n",sk_use,GET_WAIT(ch))*/
		    /*,send_to_char(buf,FIGHTING(ch))*/;
                    MAY_LIKES(ch) && sk_use > 0;
                    sk_use--)
                   {do_this = number(0,100);
                    if (do_this > GET_LIKES(ch))
                       continue;
                    do_this = number(0,100);
		    //sprintf(buf,"<%d>\r\n",do_this);
		    //send_to_char(buf,FIGHTING(ch));
                    if (do_this < 10)
                       sk_num = SKILL_BASH;
                    else
                    if (do_this < 20)
                       sk_num = SKILL_DISARM;
                    else
                    if (do_this < 30)
                       sk_num = SKILL_KICK;
                    else
                    if (do_this < 40)
                       sk_num = SKILL_PROTECT;
                    else
                    if (do_this < 50)
                       sk_num = SKILL_RESCUE;
                    else
                    if (do_this < 60 && !TOUCHING(ch))
                       sk_num = SKILL_TOUCH;
                    else
                    if (do_this < 70)
                       sk_num = SKILL_CHOPOFF;
                    else
                       sk_num = SKILL_BASH;
                    if (GET_SKILL(ch,sk_num) <= 0)
                       sk_num = 0;
                    if (!sk_num)
                       continue;
                    //else
                    //   act("Victim prepare to skill '$F'.",FALSE,FIGHTING(ch),0,skill_name(sk_num),TO_CHAR);


                    if (sk_num == SKILL_TOUCH)
                       {sk_use = 0;
                        go_touch(ch,FIGHTING(ch));
                       }

                    if (sk_num == SKILL_RESCUE || sk_num == SKILL_PROTECT)
                       {caster = NULL;
                        damager= NULL;
                        if (GET_REAL_INT(ch) < number(5,20))
                           {for(vict = world[ch->in_room].people;
                                vict && !caster;
                                vict = vict->next_in_room)
                               {if (!FIGHTING(vict) ||
                                    (IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_CHARM)))
                                   continue;
                                if (FIGHTING(vict) != ch)
                                   {caster = FIGHTING(vict);
                                    damager= vict;
                                   }
                                }
                           }
                        else
                           {for(vict = world[ch->in_room].people; vict;
                                vict = vict->next_in_room)
                               {if (!FIGHTING(vict) ||
                                    (IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_CHARM)))
                                   continue;
                                if (FIGHTING(vict) != ch)
                                   if (!caster ||
                                       GET_HIT(FIGHTING(vict)) < GET_HIT(caster))
                                      {caster = FIGHTING(vict);
                                       damager= vict;
                                      }
                                }
                           }
                        if (sk_num == SKILL_RESCUE && caster && damager && CAN_SEE(ch, caster))
                           {sk_use = 0;
                            go_rescue(ch,caster,damager);
                           }
                        if (sk_num == SKILL_PROTECT && caster && CAN_SEE(ch, caster))
                           {sk_use = 0;
                            go_protect(ch, caster);
                           }
                       }

                    if (sk_num == SKILL_BASH || sk_num == SKILL_CHOPOFF || sk_num == SKILL_DISARM)
                       {caster = NULL;
                        damager= NULL;
                        if (GET_REAL_INT(ch) < number(15,25))
                           {caster = FIGHTING(ch);
                            damager= FIGHTING(ch);
                           }
                        else
                           {for(vict = world[ch->in_room].people; vict;
                                vict = vict->next_in_room)
                               {if ((IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_CHARM)) ||
                                    !FIGHTING(vict))
                                   continue;
                                if ((AFF_FLAGGED(vict, AFF_HOLD) && GET_POS(vict) < POS_FIGHTING) ||
                                    (IS_CASTER(vict) &&
                                     (AFF_FLAGGED(vict, AFF_HOLD) || AFF_FLAGGED(vict, AFF_SIELENCE) || GET_WAIT(vict) > 0)
                                    )
                                   )
                                  continue;
                               if (!caster  ||
                                   (IS_CASTER(vict) && GET_CASTER(vict) > GET_CASTER(caster))
                                  )
                                  caster  = vict;
                               if (!damager || GET_DAMAGE(vict) > GET_DAMAGE(damager))
                                  damager = vict;
                              }
                           }
                        if (caster &&
			    (CAN_SEE(ch, caster) || FIGHTING(ch) == caster) &&
			    GET_CASTER(caster) > POOR_CASTER &&
                            (sk_num == SKILL_BASH || sk_num == SKILL_CHOPOFF)
                           )
                           {if (sk_num == SKILL_BASH)
                               {if (GET_POS(caster) >= POS_FIGHTING ||
                                    calculate_skill(ch,SKILL_BASH,200,caster) > number(50, 80)
                                   )
                                   {sk_use = 0;
                                    go_bash(ch, caster);
                                   }
                               }
                            else
                               {if (GET_POS(caster) >= POS_FIGHTING ||
                                    calculate_skill(ch,SKILL_CHOPOFF,200,caster) > number(50,80)
                                   )
                                   {sk_use = 0;
                                    go_chopoff(ch, caster);
                                   }
                               }
                           }
                        if (sk_use &&
			    damager &&
			    (CAN_SEE(ch, damager) || FIGHTING(ch) == damager)
			   )
                           {if (sk_num == SKILL_BASH)
                               {if (on_horse(damager))
                                    {sk_use = 0;
                                     go_bash(ch, get_horse(damager));
                                    }
                                else
                                if (GET_POS(damager) >= POS_FIGHTING ||
                                    calculate_skill(ch,SKILL_BASH,200,damager) > number(50,80)
                                   )
                                   {sk_use = 0;
                                    go_bash(ch, damager);
                                   }
                               }
                            else
                            if (sk_num == SKILL_CHOPOFF)
                               {if (on_horse(damager))
                                   {sk_use = 0;
                                    go_chopoff(ch, get_horse(damager));
                                   }
                                else
                                if (GET_POS(damager) >= POS_FIGHTING ||
                                    calculate_skill(ch,SKILL_CHOPOFF,200,damager) > number(50,80)
                                   )
                                   {sk_use = 0;
                                    go_chopoff(ch, damager);
                                   }
                               }
                            else
                            if (sk_num == SKILL_DISARM &&
                                (GET_EQ(damager,WEAR_WIELD) ||
                                 GET_EQ(damager,WEAR_BOTHS) ||
                                 (GET_EQ(damager,WEAR_HOLD) && GET_OBJ_TYPE(GET_EQ(damager,WEAR_HOLD)) == ITEM_WEAPON)))
                                {sk_use = 0;
                                 go_disarm(ch, damager);
                                }
                           }
                       }

                    if (sk_num == SKILL_KICK && !on_horse(FIGHTING(ch)))
                       {sk_use = 0;
                        go_kick(ch,FIGHTING(ch));
                       }
                   }

               if (!FIGHTING(ch) ||
                   IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))
                  )
                  continue;

               /***** удар основным оружием или рукой */
               if (!AFF_FLAGGED(ch, AFF_STOPRIGHT))
                  exthit(ch, TYPE_UNDEFINED, 1);

               /***** экстраатаки */
               for(i = 1; i <= ch->mob_specials.ExtraAttack; i++)
                  {if (AFF_FLAGGED(ch, AFF_STOPFIGHT) ||
                       (i == 1 && AFF_FLAGGED(ch, AFF_STOPLEFT)))
                      continue;
                   exthit(ch, TYPE_UNDEFINED, i+1);
                  }
              }
           else /* PLAYERS - only one hit per round */
              {if (GET_POS(ch) > POS_STUNNED  &&
                   GET_POS(ch) < POS_FIGHTING &&
                   GET_AF_BATTLE(ch, EAF_STAND))
                  {sprintf(buf,"%sВам лучше встать на ноги !%s\r\n",
                               CCWHT(ch,C_NRM),CCNRM(ch,C_NRM));
                   send_to_char(buf, ch);
                   CLR_AF_BATTLE(ch, EAF_STAND);
                  }

               if (GET_CAST_SPELL(ch) && GET_WAIT(ch) <= 0)
                  {if (AFF_FLAGGED(ch, AFF_SIELENCE))
                      send_to_char("Вы не смогли вымолвить и слова.\r\n",ch);
                   else
                      {cast_spell(ch, GET_CAST_CHAR(ch), GET_CAST_OBJ(ch), GET_CAST_SPELL(ch));
                       if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE) || CHECK_WAIT(ch)))
                          WAIT_STATE(ch, PULSE_VIOLENCE);
                       SET_CAST(ch, 0, NULL, NULL);
                      }
                   if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }

               if (GET_AF_BATTLE(ch,EAF_MULTYPARRY))
                  continue;

               if (GET_EXTRA_SKILL(ch) == SKILL_THROW &&
                   GET_EXTRA_VICTIM(ch) &&
                   GET_WAIT(ch) <= 0)
                  {go_throw(ch,GET_EXTRA_VICTIM(ch));
                   SET_EXTRA(ch,0,NULL);
                   if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }


               if (GET_EXTRA_SKILL(ch) == SKILL_BASH &&
                   GET_EXTRA_VICTIM(ch) &&
                   GET_WAIT(ch) <= 0)
                  {go_bash(ch,GET_EXTRA_VICTIM(ch));
                   SET_EXTRA(ch,0,NULL);
                   if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }

               if (GET_EXTRA_SKILL(ch) == SKILL_KICK &&
                   GET_EXTRA_VICTIM(ch) && GET_WAIT(ch) <= 0)
                  {go_kick(ch,GET_EXTRA_VICTIM(ch));
                   SET_EXTRA(ch,0,NULL);
                   if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }

               if (GET_EXTRA_SKILL(ch) == SKILL_CHOPOFF &&
                   GET_EXTRA_VICTIM(ch) && GET_WAIT(ch) <= 0)
                  {go_chopoff(ch,GET_EXTRA_VICTIM(ch));
                   SET_EXTRA(ch,0,NULL);
                   if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }

               if (GET_EXTRA_SKILL(ch) == SKILL_DISARM &&
                   GET_EXTRA_VICTIM(ch) && GET_WAIT(ch) <= 0)
                  {go_disarm(ch,GET_EXTRA_VICTIM(ch));
                   SET_EXTRA(ch,0,NULL);
                   if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }
		
               if (!FIGHTING(ch) ||
                   IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))
                  )
                  continue;
               /***** удар основным оружием или рукой */
               if (GET_AF_BATTLE(ch, EAF_FIRST))
                  {if (!AFF_FLAGGED(ch, AFF_STOPRIGHT) &&
                       (IS_IMMORTAL(ch) ||
                        GET_GOD_FLAG(ch, GF_GODSLIKE) ||
                        !GET_AF_BATTLE(ch, EAF_USEDRIGHT)
                       )
                      )
                      exthit(ch, TYPE_UNDEFINED, 1);
                   CLR_AF_BATTLE(ch, EAF_FIRST);
                   SET_AF_BATTLE(ch, EAF_SECOND);
                   if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }

               /***** удар вторым оружием если оно есть и умение позволяет */
               if ( GET_EQ(ch,WEAR_HOLD)                               &&
                    GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_WEAPON &&
                    GET_AF_BATTLE(ch, EAF_SECOND)                      &&
                    !AFF_FLAGGED(ch, AFF_STOPLEFT)                     &&
                    (IS_IMMORTAL(ch) ||
                     GET_GOD_FLAG(ch,GF_GODSLIKE) ||
                     GET_SKILL(ch,SKILL_SATTACK) > number(1,101)
                    )
                  )
                  {if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch,GF_GODSLIKE) ||
                       !GET_AF_BATTLE(ch, EAF_USEDLEFT))
                      exthit(ch, TYPE_UNDEFINED, 2);
                   CLR_AF_BATTLE(ch, EAF_SECOND);
                  }
               else
               /***** удар второй рукой если она свободна и умение позволяет */
               if ( !GET_EQ(ch,WEAR_HOLD)   && !GET_EQ(ch,WEAR_LIGHT)        &&
                    !GET_EQ(ch,WEAR_SHIELD) && !GET_EQ(ch,WEAR_BOTHS)        &&
                    !AFF_FLAGGED(ch,AFF_STOPLEFT)                            &&
                    GET_AF_BATTLE(ch, EAF_SECOND)                            &&
                    GET_SKILL(ch,SKILL_SHIT)
                  )
                  {if (IS_IMMORTAL(ch) || !GET_AF_BATTLE(ch, EAF_USEDLEFT))
                      exthit(ch, TYPE_UNDEFINED, 2);
                   CLR_AF_BATTLE(ch, EAF_SECOND);
                  }
              }
          }

  /* Decrement mobs lag */
  for (ch = combat_list; ch; ch = ch->next_fighting)
      {if (IN_ROOM(ch) == NOWHERE)
          continue;

       CLR_AF_BATTLE(ch, EAF_FIRST);
       CLR_AF_BATTLE(ch, EAF_SECOND);
       CLR_AF_BATTLE(ch, EAF_USEDLEFT);
       CLR_AF_BATTLE(ch, EAF_USEDRIGHT);
       CLR_AF_BATTLE(ch, EAF_MULTYPARRY);
       if (GET_AF_BATTLE(ch, EAF_SLEEP))
          affect_from_char(ch, SPELL_SLEEP);
       if (GET_AF_BATTLE(ch, EAF_BLOCK))
          {CLR_AF_BATTLE(ch, EAF_BLOCK);
           if (!WAITLESS(ch) && GET_WAIT(ch) < PULSE_VIOLENCE)
              WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
          }
       if (GET_AF_BATTLE(ch, EAF_DEVIATE))
          {CLR_AF_BATTLE(ch, EAF_DEVIATE);
	   if (!WAITLESS(ch) && GET_WAIT(ch) < PULSE_VIOLENCE)
	      WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
          }	
       battle_affect_update(ch);
      }
}