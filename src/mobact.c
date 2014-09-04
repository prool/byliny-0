/* ************************************************************************
*   File: mobact.c                                      Part of CircleMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
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
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"

/* external structs */
extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct room_data *world;
extern int no_specials;
extern struct time_info_data time_info;

ACMD(do_get);
void go_bash(struct char_data *ch, struct char_data *vict);
void go_backstab(struct char_data *ch, struct char_data *vict);
void go_disarm(struct char_data *ch, struct char_data *vict);
void go_chopoff(struct char_data *ch, struct char_data *vict);
void go_mighthit(struct char_data *ch, struct char_data *vict);
void go_stupor(struct char_data *ch, struct char_data *vict);
int  skip_hiding(struct char_data *ch, struct char_data *vict);
int  skip_sneaking(struct char_data *ch, struct char_data *vict);
int  skip_camouflage(struct char_data *ch, struct char_data *vict);
int  legal_dir(struct char_data *ch, int dir, int need_specials_check, int show_msg);
void affect_total(struct char_data * ch);
int  npc_track(struct char_data *ch);
int  npc_scavenge(struct char_data *ch);
int  npc_loot(struct char_data *ch);
int  npc_move(struct char_data *ch, int dir, int need_specials_check);
void npc_wield(struct char_data *ch);
void npc_armor(struct char_data *ch);
void npc_group(struct char_data *ch);
void npc_groupbattle(struct char_data *ch);
int  npc_walk(struct char_data *ch);
void npc_steal(struct char_data *ch);
void npc_light(struct char_data *ch);
int  may_kill_here(struct char_data *ch, struct char_data *victim);

/* local functions */
void mobile_activity(int activity_level);
void clearMemory(struct char_data * ch);

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)

int extra_aggressive(struct char_data *ch, struct char_data *victim)
{int time_ok=FALSE, no_time = TRUE, month_ok = FALSE, no_month = TRUE, agro = FALSE;

 if (!IS_NPC(ch))
    return (FALSE);

 if (MOB_FLAGGED(ch, MOB_AGGRESSIVE))
    return (TRUE);

 if (victim && MOB_FLAGGED(ch, MOB_AGGRMONO) &&
     !IS_NPC(victim) &&
     GET_RELIGION(victim) == RELIGION_MONO)
    agro = TRUE;

 if (victim && MOB_FLAGGED(ch, MOB_AGGRPOLY) &&
     !IS_NPC(victim) &&
     GET_RELIGION(victim) == RELIGION_POLY)
    agro = TRUE;

 if (MOB_FLAGGED(ch, MOB_AGGR_DAY))
    {no_time = FALSE;
     if (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)
        time_ok = TRUE;
    }

 if (MOB_FLAGGED(ch, MOB_AGGR_NIGHT))
    {no_time = FALSE;
     if (weather_info.sunlight == SUN_DARK || weather_info.sunlight == SUN_SET)
        time_ok = TRUE;
    }

 if (MOB_FLAGGED(ch, MOB_AGGR_WINTER))
    {no_month = FALSE;
     if (weather_info.season == SEASON_WINTER)
        month_ok = TRUE;
    }


 if (MOB_FLAGGED(ch, MOB_AGGR_SPRING))
    {no_month = FALSE;
     if (weather_info.season == SEASON_SPRING)
        month_ok = TRUE;
    }

 if (MOB_FLAGGED(ch, MOB_AGGR_SUMMER))
    {no_month = FALSE;
     if (weather_info.season == SEASON_SUMMER)
        month_ok = TRUE;
    }

 if (MOB_FLAGGED(ch, MOB_AGGR_AUTUMN))
    {no_month = FALSE;
     if (weather_info.season == SEASON_AUTUMN)
        month_ok = TRUE;
    }

 if (MOB_FLAGGED(ch, MOB_AGGR_FULLMOON))
    {no_time = FALSE;
     if (weather_info.moon_day >= 12 && weather_info.moon_day <= 15 &&
         (weather_info.sunlight == SUN_DARK || weather_info.sunlight == SUN_SET))
        time_ok = TRUE;
    }
 if (agro || !no_time || !no_month)
    return ((no_time || time_ok) && (no_month || month_ok));
 else
    return (FALSE);
}

int attack_best(struct char_data *ch, struct char_data *victim)
{ if (victim)
     {if (GET_SKILL(ch, SKILL_BACKSTAB) && !FIGHTING(victim))
         {go_backstab(ch, victim);
          return (TRUE);
         }
      if (GET_SKILL(ch, SKILL_MIGHTHIT))
         {go_mighthit(ch,victim);
          return (TRUE);
         }
      if (GET_SKILL(ch, SKILL_STUPOR))
         {go_stupor(ch,victim);
          return (TRUE);
         }
      if (GET_SKILL(ch, SKILL_BASH))
         {go_bash(ch, victim);
          return (TRUE);
         }
      if (GET_SKILL(ch, SKILL_DISARM))
         {go_disarm(ch, victim);
         }
      if (GET_SKILL(ch, SKILL_CHOPOFF))
         {go_chopoff(ch, victim);
         }
      if (!FIGHTING(ch))
         hit(ch,victim,TYPE_UNDEFINED,1);
      return (TRUE);
     }
  else
     return (FALSE);
}

#define KILL_FIGHTING   (1 << 0)
#define CHECK_HITS      (1 << 10)
#define SKIP_HIDING     (1 << 11)
#define SKIP_CAMOUFLAGE (1 << 12)
#define SKIP_SNEAKING   (1 << 13)



int perform_best_mob_attack(struct char_data *ch, int extmode)
{ struct char_data *vict, *victim = NULL, *use_light = NULL, *min_hp = NULL,
                          *min_lvl = NULL, *caster = NULL,   *best   = NULL;
  int    kill_this, extra_aggr=0;
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
      {/*
       sprintf(buf,"Vic-%s,B-%s,V-%s,L-%s,Lev-%s,H-%s,C-%s",
              GET_NAME(vict),
	      best ? GET_NAME(best) : "None",
	      victim  ? GET_NAME(victim) : "None",	      
	      use_light ? GET_NAME(use_light) : "None",
	      min_lvl   ? GET_NAME(min_lvl) : "None",	      
	      min_hp    ? GET_NAME(min_hp) : "None",
	      caster    ? GET_NAME(caster) : "None");
       act(buf,FALSE,ch,0,0,TO_ROOM);
       */
       if (IS_NPC(vict)                    || 
           PRF_FLAGGED(vict, PRF_NOHASSLE) ||
	   !MAY_SEE(ch,vict)               ||
	   !may_kill_here(ch,vict)
	  )
          continue;
       kill_this = FALSE;

       // Mobile too damage
       if (IS_SET(extmode, CHECK_HITS) &&
           MOB_FLAGGED(ch, MOB_WIMPY)  &&
           AWAKE(vict)                 &&
           GET_HIT(ch) * 2 < GET_REAL_MAX_HIT(ch)
          )
          continue;

       // Mobile helpers...
       if (IS_SET(extmode, KILL_FIGHTING)         &&
           FIGHTING(vict)                         &&
           FIGHTING(vict) != ch                   &&
           IS_NPC(FIGHTING(vict))                 &&
	   !AFF_FLAGGED(FIGHTING(vict),AFF_CHARM) &&
	   SAME_ALIGN(ch,FIGHTING(vict))
	  )
          kill_this = TRUE;
       else
       // ... but no aggressive for this char
       if (!(extra_aggr = extra_aggressive(ch,vict)))
          continue;

       // skip sneaking, hiding and camouflaging pc
       if (IS_SET(extmode, SKIP_SNEAKING))
          {skip_sneaking(vict,ch);
           if (EXTRA_FLAGGED(vict, EXTRA_FAILSNEAK))
              REMOVE_BIT(AFF_FLAGS(vict, AFF_SNEAK), AFF_SNEAK);
           if (AFF_FLAGGED(vict, AFF_SNEAK))
	      continue;
          }

       if (IS_SET(extmode, SKIP_HIDING))
          {skip_hiding(vict,ch);
           if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE))
              REMOVE_BIT(AFF_FLAGS(vict, AFF_HIDE), AFF_HIDE);
          }

       if (IS_SET(extmode, SKIP_CAMOUFLAGE))
          {skip_camouflage(vict,ch);
           if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE))
              REMOVE_BIT(AFF_FLAGS(vict, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);
          }

       if (!CAN_SEE(ch, vict))
          continue;

       // Mobile aggresive
       if (!kill_this && extra_aggr)
          {if (GET_CLASS(vict) == CLASS_MERCHANT &&
               number(1,GET_LEVEL(vict) * GET_REAL_CHA(vict)) >
               number(1,GET_LEVEL(ch)   * GET_REAL_INT(ch))
              )
              continue;
           kill_this = TRUE;
          }
	
       if (!kill_this)
          continue;

       // define victim if not defined
       if (!victim)
          victim = vict;
	
       if (IS_DEFAULTDARK(IN_ROOM(ch)) &&
           ((GET_EQ(vict,ITEM_LIGHT) && GET_OBJ_VAL(GET_EQ(vict,ITEM_LIGHT),2)) ||
            ((AFF_FLAGGED(vict, AFF_SINGLELIGHT) || 
              AFF_FLAGGED(vict, AFF_HOLYLIGHT)) &&
              !AFF_FLAGGED(vict, AFF_HOLYDARK))
           ) &&
           (!use_light || GET_REAL_CHA(use_light) > GET_REAL_CHA(vict))
          )
          use_light = vict;

       if (!min_hp ||
           GET_HIT(vict)   + GET_REAL_CHA(vict)   * 10 < 
	   GET_HIT(min_hp) + GET_REAL_CHA(min_hp) * 10
          )
          min_hp = vict;
	  
       if (!min_lvl ||
           GET_LEVEL(vict)    + number(1,GET_REAL_CHA(vict)) < 
	   GET_LEVEL(min_lvl) + number(1,GET_REAL_CHA(min_lvl))
          )
          min_lvl = vict;
	  
       if (IS_CASTER(vict) && 
           (!caster || 
            GET_CASTER(caster) * GET_REAL_CHA(vict) < 
	    GET_CASTER(vict)   * GET_REAL_CHA(caster)
           )
          )
          caster = vict;
       /*
       sprintf(buf,"%s here !",GET_NAME(vict));	  
       act(buf,FALSE,ch,0,0,TO_ROOM);	  
       */
      }

  if (GET_REAL_INT(ch) < 5+number(1,6))
     best = victim;
  else
  if (GET_REAL_INT(ch) < 10+number(1,6))
     best = use_light ? use_light : victim;
  else
  if (GET_REAL_INT(ch) < 15+number(1,6))
     best = min_lvl ? min_lvl : (use_light ? use_light : victim);
  else
  if (GET_REAL_INT(ch) < 25+number(1,6))
     best = caster ? caster : (min_lvl ? min_lvl : (use_light ? use_light : victim));
  else
     best = min_hp ? min_hp : (caster ? caster : (min_lvl ? min_lvl : (use_light ? use_light : victim)));

  if (best)
     {/*
      sprintf(buf,"B-%s,V-%s,L-%s,Lev-%s,H-%s,C-%s",
              GET_NAME(best),
	      GET_NAME(victim),
	      use_light ? GET_NAME(use_light) : "None",
	      min_lvl   ? GET_NAME(min_lvl) : "None",	      
	      min_hp    ? GET_NAME(min_hp) : "None",
	      caster    ? GET_NAME(caster) : "None");
      act(buf,FALSE,ch,0,0,TO_ROOM);
      */
      if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) > POS_SLEEPING)
         {act("$n вскочил$g на ноги.",FALSE,ch,0,0,TO_ROOM);
          GET_POS(ch) = POS_STANDING;
         }

      if (IS_SET(extmode,KILL_FIGHTING) && FIGHTING(best))
         act("$n вступил$g в битву на стороне $N1.",FALSE,ch,0,FIGHTING(best),TO_ROOM);

      if (!attack_best(ch, best) && !FIGHTING(ch))
         hit(ch,best,TYPE_UNDEFINED,1);
      return (TRUE);
     }
  return (FALSE);
}

int perform_best_horde_attack(struct char_data *ch, int extmode)
{struct char_data *vict;
 if (perform_best_mob_attack(ch, extmode))
    return (TRUE);

 for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
     {if (!IS_NPC(vict)                   || 
          !MAY_SEE(ch,vict)               
         )
         continue;
      if (!SAME_ALIGN(ch,vict))	  
         {if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) > POS_SLEEPING)
             {act("$n вскочил$g на ноги.",FALSE,ch,0,0,TO_ROOM);
              GET_POS(ch) = POS_STANDING;
             }
          if (!attack_best(ch, vict) && !FIGHTING(ch))
             hit(ch,vict,TYPE_UNDEFINED,1);
          return (TRUE);
         }
     }
  return (FALSE);
}

void do_aggressive_mob(struct char_data *ch, int check_sneak)
{ struct char_data *vict, *next_ch, *next_vict, *victim;
  int    mode = check_sneak ? SKIP_SNEAKING : 0;
  memory_rec *names;

  if (IN_ROOM(ch) == NOWHERE)
     return;

  for (ch = world[IN_ROOM(ch)].people; ch; ch = next_ch)
      { next_ch = ch->next_in_room;
        if (!IS_NPC(ch)                ||
	    !MAY_ATTACK(ch)            ||
            AFF_FLAGGED(ch, AFF_BLIND)
	   )
           continue;

        /****************  Horde */
	if (MOB_FLAGGED(ch,MOB_HORDE))
	   {perform_best_horde_attack(ch,mode | SKIP_HIDING | SKIP_CAMOUFLAGE);
            continue;
           }	       
	       
        /****************  Aggressive Mobs */
        if (extra_aggressive(ch,NULL))
           {if (perform_best_mob_attack(ch,mode | SKIP_HIDING | SKIP_CAMOUFLAGE | CHECK_HITS))
               continue;
           }

        /****************  Helper Mobs     */
        if (MOB_FLAGGED(ch, MOB_HELPER))
           {if (perform_best_mob_attack(ch,mode | KILL_FIGHTING | CHECK_HITS))
               continue;
           }

        /*****************  Mob Memory      */
        if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch))
           {victim = NULL;
            // Find memory in room
            for (vict = world[ch->in_room].people; vict && !victim; vict = next_vict)
                {next_vict = vict->next_in_room;
                 if (IS_NPC(vict) || 
		     PRF_FLAGGED(vict, PRF_NOHASSLE)
		    )
	            continue;
                 for (names = MEMORY(ch); names && !victim; names = names->next)
                     if (names->id == GET_IDNUM(vict))
                        {if (!MAY_SEE(ch,vict) ||
                             !may_kill_here(ch,vict)
			    )
			    continue;
			 if (check_sneak)
	                    {skip_sneaking(vict,ch);
			     if (EXTRA_FLAGGED(vict, EXTRA_FAILSNEAK))
                                REMOVE_BIT(AFF_FLAGS(vict, AFF_SNEAK), AFF_SNEAK);
	                     if (AFF_FLAGGED(vict, AFF_SNEAK))
			        {continue;
			        }
	                    }
	                  skip_hiding(vict,ch);
                          if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE))
                             REMOVE_BIT(AFF_FLAGS(vict, AFF_HIDE), AFF_HIDE);
	                  skip_camouflage(vict,ch);
                          if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE))
                             REMOVE_BIT(AFF_FLAGS(vict, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);
                          if (CAN_SEE(ch,vict))
	                     victim = vict;
	                 }
                 }
            // Is memory found ?
            if (victim)
               {if (GET_POS(ch) < POS_FIGHTING && 
	            GET_POS(ch) > POS_SLEEPING
		   )
	           {act("$n вскочил$g на ноги.",FALSE,ch,0,0,TO_ROOM);
	            GET_POS(ch) = POS_STANDING;
	           }
	        if (GET_CLASS(ch) == CLASS_ANIMAL ||
	            GET_CLASS(ch) == CLASS_BASIC_NPC)
	            act("$n вспомнил$g $N3.",FALSE,ch,0,victim,TO_ROOM);
	        else
                    act("'$N - ты пытал$U убить меня ! Попал$U ! Умри !!!', воскликнул$g $n.",
	                   FALSE, ch, 0, victim, TO_ROOM);

	        if (!attack_best(ch, victim))
		   {hit(ch, victim, TYPE_UNDEFINED, 1);
		   }
	        continue;
	       }
           }
      }
}

void mobile_activity(int activity_level)
{
  struct char_data *ch, *next_ch, *vict, *first, *victim;
  struct room_direction_data * rdata=NULL;
  int    door, found, max, start_battle, was_in;
  memory_rec *names;

  for (ch = character_list; ch; ch = next_ch)
      {next_ch = ch->next;

     
       if (!IS_MOB(ch))
          continue;
       if (GET_WAIT(ch) > 0)
          GET_WAIT(ch)--;
       else
          GET_WAIT(ch) = 0;
	  
       if (GET_ACTIVITY(ch) != activity_level ||
           (was_in = IN_ROOM(ch)) == NOWHERE  ||
           GET_ROOM_VNUM(IN_ROOM(ch)) % 100 == 99
	  )
          continue;

       start_battle = FALSE;

       // Examine call for special procedure
       if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials)
          {if (mob_index[GET_MOB_RNUM(ch)].func == NULL)
              {log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
                   GET_NAME(ch), GET_MOB_VNUM(ch));
               REMOVE_BIT(MOB_FLAGS(ch, MOB_SPEC), MOB_SPEC);
              }
           else
              {if ((mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, ""))
                  continue; // go to next char
              }
          }

       // Extract free horses
       if (GET_MOB_VNUM(ch) == HORSE_VNUM && !ch->master)
          {act("Возникший как из-под земли цыган ловко вскочил на $n3 и унесся прочь.",FALSE,ch,0,0,TO_ROOM);
           extract_char(ch,FALSE);
           continue;
          }
	  
       // Extract uncharmed mobs
       if (EXTRACT_TIMER(ch) > 0)
          {if (ch->master)
	      EXTRACT_TIMER(ch) = 0;
	   else     
              {EXTRACT_TIMER(ch)--;
	       if (!EXTRACT_TIMER(ch))
	          {extract_char(ch,FALSE);
	           continue;
	          }
	       }   
	  }

       // If the mob has no specproc, do the default actions
       if (FIGHTING(ch)                   ||
           GET_POS(ch) <= POS_STUNNED     ||
	   GET_WAIT(ch) > 0               ||
           AFF_FLAGGED(ch, AFF_CHARM)     ||
           AFF_FLAGGED(ch, AFF_HOLD)      ||
           AFF_FLAGGED(ch, AFF_STOPFIGHT) ||
	   AFF_FLAGGED(ch, AFF_SLEEP)     
	  )
	  continue;
	  
       if (IS_HORSE(ch))
          {if (GET_POS(ch) < POS_FIGHTING)
	      GET_POS(ch) = POS_STANDING;
	   continue;
	  }
	  
       if (GET_POS(ch) == POS_SLEEPING &&
           GET_DEFAULT_POS(ch) > POS_SLEEPING
	  )
	  {GET_POS(ch) = GET_DEFAULT_POS(ch);
	   act("$n проснул$u.",FALSE,ch,0,0,TO_ROOM);
          }
	  	   
       if (!AWAKE(ch))
          continue;
	  
       for (vict = world[ch->in_room].people, max = FALSE;
            vict; vict = vict->next_in_room)
           {if (ch == vict)
               continue;
            if (FIGHTING(vict) == ch)
               break;
            if (!IS_NPC(vict) && CAN_SEE(ch,vict))
               max = TRUE;
           }

       // Mob is under attack
       if (vict)
          continue;

       // Mob attemp rest
       if (!max &&
           GET_HIT(ch) < GET_REAL_MAX_HIT(ch) &&
           GET_POS(ch) > POS_RESTING)
          {act("$n присел$g отдохнуть.",FALSE,ch,0,0,TO_ROOM);
           GET_POS(ch) = POS_RESTING;
          }	  
	  
	  
       // Mob return to default pos if full rested
       if (GET_HIT(ch) >= GET_REAL_MAX_HIT(ch) &&
           GET_POS(ch) != GET_DEFAULT_POS(ch)
	  )
          switch (GET_DEFAULT_POS(ch))
          {case POS_STANDING:
                act("$n встал$g на ноги.",FALSE,ch,0,0,TO_ROOM);
                GET_POS(ch) = POS_STANDING;
                break;
           case POS_SITTING:
                act("$n сел$g.",FALSE,ch,0,0,TO_ROOM);
                GET_POS(ch) = POS_SITTING;
                break;
           case POS_RESTING:
                act("$n присел$g отдохнуть.",FALSE,ch,0,0,TO_ROOM);
                GET_POS(ch) = POS_RESTING;
                break;
           case POS_SLEEPING:
                act("$n уснул$g.",FALSE,ch,0,0,TO_ROOM);
                GET_POS(ch) = POS_SLEEPING;
                break;
          }
	  
       // look at room before moving
       do_aggressive_mob(ch, FALSE);

       // if mob attack something
       if (FIGHTING(ch) || 
           GET_WAIT(ch) >  0)
          continue;

       /* Scavenger (picking up objects) */
       if (MOB_FLAGGED(ch, MOB_SCAVENGER))
          npc_scavenge(ch);

       /* Looting the corpses            */
       if (MOB_FLAGGED(ch, MOB_LOOTER))
          npc_loot(ch);

       if (NPC_FLAGGED(ch, NPC_WIELDING))
          npc_wield(ch);

       if (NPC_FLAGGED(ch, NPC_ARMORING))
          npc_armor(ch);

       if (NPC_FLAGGED(ch, NPC_STEALING))
          npc_steal(ch);

       if (GET_POS(ch) == POS_STANDING && NPC_FLAGGED(ch, NPC_INVIS))
          SET_BIT(AFF_FLAGS(ch, AFF_INVISIBLE), AFF_INVISIBLE);

       if (GET_POS(ch) == POS_STANDING && NPC_FLAGGED(ch, NPC_MOVEFLY))
          SET_BIT(AFF_FLAGS(ch, AFF_FLY), AFF_FLY);

       if (GET_POS(ch) == POS_STANDING &&
           NPC_FLAGGED(ch, NPC_SNEAK))
          {if (calculate_skill(ch,SKILL_SNEAK,100,0) >= number(0,100))
              SET_BIT(AFF_FLAGS(ch, AFF_SNEAK), AFF_SNEAK);
           else
              REMOVE_BIT(AFF_FLAGS(ch, AFF_SNEAK), AFF_SNEAK);
           //log("[MOBILE_ACTIVITY->AFFECT_TOTAL] Sneak start");
           affect_total(ch);
           //log("[MOBILE_ACTIVITY->AFFECT_TOTAL] Sneak stop");
          }

       if (GET_POS(ch) == POS_STANDING &&
           NPC_FLAGGED(ch, NPC_CAMOUFLAGE))
          {if (calculate_skill(ch,SKILL_CAMOUFLAGE,100,0) >= number(0,100))
              SET_BIT(AFF_FLAGS(ch, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);
           else
              REMOVE_BIT(AFF_FLAGS(ch, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);
           //log("[MOBILE_ACTIVITY->AFFECT_TOTAL] Camouflage start");
           affect_total(ch);
           //log("[MOBILE_ACTIVITY->AFFECT_TOTAL] Camouflage stop");
          }

       door = BFS_ERROR;

       // Helpers go to some dest
       if (door == BFS_ERROR &&
           MOB_FLAGGED(ch, MOB_HELPER) &&
           !MOB_FLAGGED(ch, MOB_SENTINEL) &&
           !AFF_FLAGGED(ch,AFF_BLIND) &&
           !ch->master &&
           GET_POS(ch) == POS_STANDING
          )
          {for (found = FALSE, door = 0; door < NUM_OF_DIRS; door++)
               {for(rdata = EXIT(ch, door), max = MAX(1,GET_REAL_INT(ch) / 10);
                    max > 0 && !found; max--)
                   {if (!rdata ||
                        rdata->to_room == NOWHERE ||
                        IS_SET(rdata->exit_info, EX_CLOSED) ||
                        !legal_dir(ch,door,TRUE,FALSE) ||
                        world[rdata->to_room].forbidden ||
                        IS_DARK(rdata->to_room) ||
                        (MOB_FLAGGED(ch, MOB_STAY_ZONE) &&
                         world[IN_ROOM(ch)].zone != world[rdata->to_room].zone
                        )
                       )
                       break;
                    for (first = world[rdata->to_room].people; first; first = first->next)
                        if (IS_NPC(first) &&
                            !AFF_FLAGGED(first, AFF_CHARM) &&
                            !IS_HORSE(first) &&
                            CAN_SEE(ch, first) &&
                            FIGHTING(first) &&
			    SAME_ALIGN(ch,first)
                           )
                           {found = TRUE;
                            break;
                           }
                    rdata = world[rdata->to_room].dir_option[door];
                   }
                if (found)
                   break;
               }
           if (!found)
              door = BFS_ERROR;
          }

       if (GET_DEST(ch) != NOWHERE &&
           GET_POS(ch) > POS_FIGHTING &&
           door == BFS_ERROR)
          {npc_group(ch);
           door = npc_walk(ch);
          }

       if (GET_SKILL(ch, SKILL_TRACK) &&
           GET_POS(ch) > POS_FIGHTING &&
           MEMORY(ch) &&
           door == BFS_ERROR)
          door = npc_track(ch);

       if (door == BFS_ALREADY_THERE)
          {do_aggressive_mob(ch,FALSE);
           continue;
          }

       if (door == BFS_ERROR)
          door = number(0, 18);

       /* Mob Movement */
       if (!MOB_FLAGGED(ch, MOB_SENTINEL) &&
           GET_POS(ch) == POS_STANDING &&
           (door >= 0 && door < NUM_OF_DIRS) &&
           EXIT(ch, door) &&
           EXIT(ch,door)->to_room != NOWHERE &&
           legal_dir(ch,door,TRUE,FALSE) &&
           !world[EXIT(ch, door)->to_room].forbidden &&
           (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
            world[EXIT(ch, door)->to_room].zone == world[ch->in_room].zone
           )
          )
          {npc_move(ch, door, 1);
           npc_group(ch);
           npc_groupbattle(ch);
          }

       npc_light(ch);

       /*****************  Mob Memory      */
       if (MOB_FLAGGED(ch, MOB_MEMORY) &&
           MEMORY(ch)                  &&
           GET_POS(ch) > POS_SLEEPING  &&
	   !AFF_FLAGGED(ch, AFF_BLIND) &&
           !FIGHTING(ch))
          {victim = NULL;
           // Find memory in world
           for (names = MEMORY(ch); names && !victim && GET_SPELL_MEM(ch,SPELL_SUMMON) > 0;
                names = names->next)
               for (vict = character_list; vict && !victim; vict = vict->next)
                   if (names->id == GET_IDNUM(vict)      &&
                       PRF_FLAGGED(vict, PRF_SUMMONABLE) &&
                       CAN_SEE(ch, vict)                 &&
                       !PRF_FLAGGED(vict, PRF_NOHASSLE))
                      {cast_spell(ch,vict,0,SPELL_SUMMON);
                       if (IN_ROOM(vict) == IN_ROOM(ch) && 
		           CAN_SEE(ch,vict)
			  )
                          {victim      = vict;
                           names->time = 0;
                          }
                       if (GET_SPELL_MEM(ch,SPELL_TELEPORT) <= 0)
                          break;
                      }
          }

       if (was_in != IN_ROOM(ch))
          do_aggressive_mob(ch, FALSE);
       /* Add new mobile actions here */

  }				/* end for() */
}

/* Mob Memory Routines */

/* make ch remember victim */
void remember(struct char_data * ch, struct char_data * victim)
{ struct timed_type timed;
  memory_rec *tmp;
  bool present = FALSE;

  if (!IS_NPC(ch)    ||
      IS_NPC(victim) ||
      PRF_FLAGGED(victim,PRF_NOHASSLE) ||
      !MOB_FLAGGED(ch, MOB_MEMORY))
     return;

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
      if (tmp->id == GET_IDNUM(victim))
         {if (tmp->time > 0)
             tmp->time = time(NULL) + TIME_KOEFF * SECS_PER_MUD_DAY * GET_REAL_INT(ch);
          present = TRUE;
         }

  if (!present)
     {CREATE(tmp, memory_rec, 1);
      tmp->next  = MEMORY(ch);
      tmp->id    = GET_IDNUM(victim);
      tmp->time  = time(NULL) + TIME_KOEFF * SECS_PER_MUD_DAY * GET_REAL_INT(ch);
      MEMORY(ch) = tmp;
     }
     
  if (!timed_by_skill(victim, SKILL_HIDETRACK))
     {timed.skill = SKILL_HIDETRACK;
      timed.time  = GET_SKILL(ch,SKILL_TRACK) ? 6 : 3;
      timed_to_char(victim,&timed);
     }
}


/* make ch forget victim */
void forget(struct char_data * ch, struct char_data * victim)
{
  memory_rec *curr, *prev = NULL;

  if (!(curr = MEMORY(ch)))
     return;

  while (curr && curr->id != GET_IDNUM(victim))
        {prev = curr;
         curr = curr->next;
        }

  if (!curr)
    return;			/* person wasn't there at all. */

  if (curr == MEMORY(ch))
    MEMORY(ch) = curr->next;
  else
    prev->next = curr->next;

  free(curr);
}


/* erase ch's memory */
void clearMemory(struct char_data * ch)
{
  memory_rec *curr, *next;

  curr = MEMORY(ch);

  while (curr)
    {next = curr->next;
     free(curr);
     curr = next;
    }
  MEMORY(ch) = NULL;
}