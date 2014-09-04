/* ************************************************************************
*   File: handler.c                                     Part of CircleMUD *
*  Usage: internal funcs: moving and finding chars/objs                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "constants.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "screen.h"
#include "dg_scripts.h"

/* external vars */
extern struct char_data *combat_list;
extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern struct obj_data *obj_proto;
extern const  char *MENU;
extern struct gods_celebrate_apply_type  *Mono_apply;
extern struct gods_celebrate_apply_type  *Poly_apply;
extern int    supress_godsapply;
extern const struct new_flag clear_flags;

/* local functions */
int apply_ac(struct char_data * ch, int eq_pos);
int apply_armour(struct char_data * ch, int eq_pos);
void update_object(struct obj_data * obj, int use);
void update_char_objects(struct char_data * ch);

/* external functions */
int  invalid_anti_class(struct char_data *ch, struct obj_data *obj);
int  invalid_unique(struct char_data *ch, struct obj_data *obj);
int  invalid_no_class(struct char_data *ch, struct obj_data *obj);
void remove_follower(struct char_data * ch);
void clearMemory(struct char_data * ch);
void die_follower(struct char_data * ch);
int  extra_damroll(int class_num, int level);
int  Crash_delete_file(char *name, int mask);
ACMD(do_return);

char *fname(const char *namelist)
{
  static char holder[30];
  register char *point;

  for (point = holder; a_isalpha(*namelist); namelist++, point++)
      *point = *namelist;

  *point = '\0';

  return (holder);
}

/*
int isname(const char *str, const char *namelist)
{
  const char *curname, *curstr;

  curname = namelist;
  for (;;)
    {for (curstr = str;; curstr++, curname++)
         {if (!*curstr && !a_isalpha(*curname))
	         return (1);

          if (!*curname)
	         return (0);

          if (!*curstr || *curname == ' ')
	      break;

          if (LOWER(*curstr) != LOWER(*curname))
	      break;
         }

     for (; a_isalpha(*curname); curname++);
     if (!*curname)
        return (0);
     curname++;
     }
}
*/

int isname(const char *str, const char *namelist)
{ int   once_ok = FALSE;
  const char *curname, *curstr, *laststr;

  if (!namelist || !*namelist)
     return (FALSE);

  curname = namelist;
  curstr  = laststr = str;
  for (;;)
      {once_ok = FALSE;
       for (;;curstr++, curname++)
           {if (!*curstr)
	       return (once_ok);
	    if (curstr != laststr && *curstr == '!')
	       if (a_isalnum(*curname))
	          {curstr = laststr;
	           break;
	          }
	    if (!a_isalnum(*curstr))
	       {for(;!a_isalnum(*curstr);curstr++)
	           {if (!*curstr)
	               return (once_ok);
                   }
                laststr=curstr;
  	        break;
	       }
            if (!*curname)
  	       return (FALSE);
            if (!a_isalnum(*curname))
  	       {curstr = laststr;
	        break;
	       }
            if (LOWER(*curstr) != LOWER(*curname))
	       {curstr = laststr;
	        break;
	       }
	    else
	       once_ok = TRUE;
           }
     /* skip to next name */
     for (; a_isalnum(*curname); curname++);
     for (; !a_isalnum(*curname); curname++)
         {if (!*curname)
             return (FALSE);
         }
     }
}

void set_quested(struct char_data *ch, int quest)
{ int i;
  if (IS_NPC(ch) || IS_IMMORTAL(ch))
     return;
  if (ch->Questing.quests)
     {for (i = 0; i < ch->Questing.count; i++)
          if (*(ch->Questing.quests+i) == quest)
             return;
      if (!(ch->Questing.count % 10L))
         RECREATE(ch->Questing.quests, int, (ch->Questing.count / 10L + 1) * 10L);
     }
  else
     {ch->Questing.count  = 0;
      CREATE(ch->Questing.quests, int, 10);
     }
  *(ch->Questing.quests+ch->Questing.count++) = quest;
}

int     get_quested(struct char_data *ch, int quest)
{ int i;
  if (IS_NPC(ch) || IS_IMMORTAL(ch))
     return (FALSE);
  if (ch->Questing.quests)
     {for (i = 0; i < ch->Questing.count; i++)
          if (*(ch->Questing.quests+i) == quest)
             return (TRUE);
     }
  return (FALSE);
}

void    inc_kill_vnum(struct char_data *ch, int vnum, int incvalue)
{ int i;
  if (IS_NPC(ch) || IS_IMMORTAL(ch) || vnum < 0)
     return;

  if (ch->MobKill.vnum)
     {for (i = 0; i < ch->MobKill.count; i++)
          if (*(ch->MobKill.vnum+i) == vnum)
             {*(ch->MobKill.howmany+i) += incvalue;
              return;
             }
      if (!(ch->MobKill.count % 10L))
         {RECREATE(ch->MobKill.howmany, int, (ch->MobKill.count / 10L + 1) * 10L);
          RECREATE(ch->MobKill.vnum,    int, (ch->MobKill.count / 10L + 1) * 10L);
         }
     }
  else
     {ch->MobKill.count  = 0;
      CREATE(ch->MobKill.vnum,    int, 10);
      CREATE(ch->MobKill.howmany, int, 10);
     }

  *(ch->MobKill.vnum+ch->MobKill.count)       = vnum;
  *(ch->MobKill.howmany+ch->MobKill.count++)  = incvalue;
}

int     get_kill_vnum(struct char_data *ch, int vnum)
{ int i;
  if (IS_NPC(ch) || IS_IMMORTAL(ch) || vnum < 0)
     return (0);
  if (ch->MobKill.vnum)
     {for (i = 0; i < ch->MobKill.count; i++)
          if (*(ch->MobKill.vnum+i) == vnum)
             return (*(ch->MobKill.howmany+i));
     }
  return (0);
}

#define LIGHT_NO    0
#define LIGHT_YES   1
#define LIGHT_UNDEF 2

void check_light(struct char_data *ch,
                 int was_equip, int was_single, int was_holylight, int was_holydark,
                 int koef)
{ int light_equip = FALSE;
  if (IN_ROOM(ch) == NOWHERE)
     return;
  //if (IS_IMMORTAL(ch))
  //   {sprintf(buf,"%d %d %d (%d)\r\n",world[IN_ROOM(ch)].light,world[IN_ROOM(ch)].glight,world[IN_ROOM(ch)].gdark,koef);
  //    send_to_char(buf,ch);
  //   }
  if (GET_EQ(ch, WEAR_LIGHT))
     {if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
         {if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))
             {//send_to_char("Light OK!\r\n",ch);
              light_equip = TRUE;
	     }
         }
     }
  // In equipment
  if (light_equip)
     {if (was_equip == LIGHT_NO)
         world[ch->in_room].light = MAX(0,world[ch->in_room].light + koef);
     }
  else
     {if (was_equip == LIGHT_YES)
         world[ch->in_room].light = MAX(0,world[ch->in_room].light - koef);
     }
  // Singleligt affect
  if (AFF_FLAGGED(ch, AFF_SINGLELIGHT))
     {if (was_single == LIGHT_NO)
         world[ch->in_room].light = MAX(0,world[ch->in_room].light + koef);
     }
  else
     {if (was_single == LIGHT_YES)
         world[ch->in_room].light = MAX(0,world[ch->in_room].light - koef);
     }
  // Holyligh affect
  if (AFF_FLAGGED(ch, AFF_HOLYLIGHT))
     {if (was_holylight == LIGHT_NO)
         world[ch->in_room].glight = MAX(0,world[ch->in_room].glight + koef);
     }
  else
     {if (was_holylight == LIGHT_YES)
         world[ch->in_room].glight = MAX(0,world[ch->in_room].glight - koef);
     }
  // Holydark affect
  // if (IS_IMMORTAL(ch))
  //    {sprintf(buf,"holydark was %d\r\n",was_holydark);
  //     send_to_char(buf,ch);
  //    }
  if (AFF_FLAGGED(ch, AFF_HOLYDARK))
     {// if (IS_IMMORTAL(ch))
      //    send_to_char("holydark on\r\n",ch);
      if (was_holydark == LIGHT_NO)
         world[ch->in_room].gdark = MAX(0,world[ch->in_room].gdark + koef);
     }
  else
     {// if (IS_IMMORTAL(ch))
      //   send_to_char("HOLYDARK OFF\r\n",ch);
      if (was_holydark == LIGHT_YES)
         world[ch->in_room].gdark = MAX(0,world[ch->in_room].gdark - koef);
     }
  //if (IS_IMMORTAL(ch))
  //   {sprintf(buf,"%d %d %d (%d)\r\n",world[IN_ROOM(ch)].light,world[IN_ROOM(ch)].glight,world[IN_ROOM(ch)].gdark,koef);
  //    send_to_char(buf,ch);
  //   }
}

void affect_modify(struct char_data * ch, byte loc, sbyte mod,
                   bitvector_t bitv, bool add)
{ if (add)
     {SET_BIT(AFF_FLAGS(ch, bitv), bitv);
     }
  else
     {REMOVE_BIT(AFF_FLAGS(ch, bitv), bitv);
      mod = -mod;
     }

  switch (loc)
  {
  case APPLY_NONE:
    break;
  case APPLY_STR:
    GET_STR_ADD(ch) += mod;
    break;
  case APPLY_DEX:
    GET_DEX_ADD(ch) += mod;
    break;
  case APPLY_INT:
    GET_INT_ADD(ch) += mod;
    break;
  case APPLY_WIS:
    GET_WIS_ADD(ch) += mod;
    break;
  case APPLY_CON:
    GET_CON_ADD(ch) += mod;
    break;
  case APPLY_CHA:
    GET_CHA_ADD(ch) += mod;
    break;
  case APPLY_CLASS:
    break;

  /*
   * My personal thoughts on these two would be to set the person to the
   * value of the apply.  That way you won't have to worry about people
   * making +1 level things to be imp (you restrict anything that gives
   * immortal level of course).  It also makes more sense to set someone
   * to a class rather than adding to the class number. -gg
   */

  case APPLY_LEVEL:
    break;
  case APPLY_AGE:
    GET_AGE_ADD(ch)    += mod;
    break;
  case APPLY_CHAR_WEIGHT:
    GET_WEIGHT_ADD(ch) += mod;
    break;
  case APPLY_CHAR_HEIGHT:
    GET_HEIGHT_ADD(ch) += mod;
    break;
  case APPLY_MANAREG:
    GET_MANAREG(ch) += mod;
    break;
  case APPLY_HIT:
    GET_HIT_ADD(ch) += mod;
    break;
  case APPLY_MOVE:
    GET_MOVE_ADD(ch) += mod;
    break;
  case APPLY_GOLD:
    break;
  case APPLY_EXP:
    break;
  case APPLY_AC:
    GET_AC_ADD(ch) += mod;
    break;
  case APPLY_HITROLL:
    GET_HR_ADD(ch) += mod;
    break;
  case APPLY_DAMROLL:
    GET_DR_ADD(ch) += mod;
    break;
  case APPLY_SAVING_PARA:
    GET_SAVE(ch, SAVING_PARA)   += mod;
    break;
  case APPLY_SAVING_ROD:
    GET_SAVE(ch, SAVING_ROD)    += mod;
    break;
  case APPLY_SAVING_PETRI:
    GET_SAVE(ch, SAVING_PETRI)  += mod;
    break;
  case APPLY_SAVING_BREATH:
    GET_SAVE(ch, SAVING_BREATH) += mod;
    break;
  case APPLY_SAVING_SPELL:
    GET_SAVE(ch, SAVING_SPELL)  += mod;
    break;
  case APPLY_SAVING_BASIC:
    GET_SAVE(ch, SAVING_BASIC)  += mod;
    break;
  case APPLY_HITREG:
    GET_HITREG(ch) += mod;
    break;
  case APPLY_MOVEREG:
    GET_MOVEREG(ch) += mod;
    break;
  case APPLY_C1:
  case APPLY_C2:
  case APPLY_C3:
  case APPLY_C4:
  case APPLY_C5:
  case APPLY_C6:
  case APPLY_C7:
  case APPLY_C8:
  case APPLY_C9:
    GET_SLOT(ch,loc-APPLY_C1) += mod;
    break;
  case APPLY_SIZE:
    GET_SIZE_ADD(ch) += mod;
    break;
  case APPLY_ARMOUR:
    GET_ARMOUR(ch)   += mod;
    break;
  case APPLY_POISON:
    GET_POISON(ch)   += mod;
    break;
  case APPLY_CAST_SUCCESS:
    GET_CAST_SUCCESS(ch) += mod;
    break;
  case APPLY_MORALE:
    GET_MORALE(ch)       += mod;
    break;
  case APPLY_INITIATIVE:
    GET_INITIATIVE(ch)   += mod;
    break;
  case APPLY_RELIGION:
    if (add)
       GET_PRAY(ch)      |= mod;
    else
       GET_PRAY(ch)      &= mod;
    break;
  case APPLY_ABSORBE:
    GET_ABSORBE(ch)      += mod;
    break;
  default:
    log("SYSERR: Unknown apply adjust %d attempt (%s, affect_modify).", loc, __FILE__);
    break;

  } /* switch */
}

void total_gods_affect(struct char_data *ch)
{
 struct gods_celebrate_apply_type  *cur = NULL;

 if (IS_NPC(ch) || supress_godsapply)
    return;
 // Set new affects
 if (GET_RELIGION(ch) == RELIGION_POLY)
    cur = Poly_apply;
 else
    cur = Mono_apply;
 // log("[GODAFFECT] Start function...");
 for (; cur; cur=cur->next)
     if (cur->gapply_type == GAPPLY_AFFECT)
        {affect_modify(ch,0,0,cur->modi,TRUE);
        }
     else
     if (cur->gapply_type == GAPPLY_MODIFIER)
        {affect_modify(ch,cur->what,cur->modi,0,TRUE);
        };
 // log("[GODAFFECT] Stop function...");
}


int char_saved_aff[] =
{AFF_GROUP,
 AFF_HORSE,
 0
};

int char_stealth_aff[] =
{AFF_HIDE,
 AFF_SNEAK,
 AFF_CAMOUFLAGE,
 0
};

/* This updates a character by subtracting everything he is affected by */
/* restoring original abilities, and then affecting all again           */
void affect_total(struct char_data * ch)
{ struct affected_type *af;
  struct obj_data          *obj;
  struct extra_affects_type *extra_affect   = NULL;
  struct obj_affected_type *extra_modifier = NULL;
  int    i, j;
  struct new_flag saved;

  // Clear all affect, because recalc one
  memset((char *)&ch->add_abils,0,sizeof(struct char_played_ability_data));

  // PC's clear all affects, because recalc one
  if (!IS_NPC(ch))
     {saved = ch->char_specials.saved.affected_by;
      ch->char_specials.saved.affected_by = clear_flags;
      for (i = 0; (j = char_saved_aff[i]); i++)
          if (IS_SET(GET_FLAG(saved,j),j))
             SET_BIT(AFF_FLAGS(ch,j),j);
     }

  // Apply some GODS affects and modifiers
  total_gods_affect(ch);

  /* move object modifiers */
  for (i = 0; i < NUM_WEARS; i++)
      {if ((obj = GET_EQ(ch, i)))
          {
           if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
              {GET_AC_ADD(ch) -= apply_ac(ch, i);
               GET_ARMOUR(ch) += apply_armour(ch, i);
              }
           /* Update weapon applies */
           for (j = 0; j < MAX_OBJ_AFFECT; j++)
	           affect_modify(ch, GET_EQ(ch, i)->affected[j].location,
		                         GET_EQ(ch, i)->affected[j].modifier,
		                         0, TRUE);
		   /* Update weapon bitvectors */
           for (j = 0; weapon_affect[j].aff_bitvector >= 0; j++)
               {if (!IS_SET(GET_FLAG(obj->obj_flags.affects,weapon_affect[j].aff_pos),
                            weapon_affect[j].aff_pos) ||
                    weapon_affect[j].aff_bitvector == 0)
                    continue;
                affect_modify(ch, APPLY_NONE, 0,
                              weapon_affect[j].aff_bitvector,TRUE);
               }
		  }
      }

  /* move affect modifiers */
  for (af = ch->affected; af; af = af->next)
      affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);

  /* move race and class modifiers */
  if (!IS_NPC(ch))
     {if ((int) GET_CLASS(ch) >= 0 && (int) GET_CLASS(ch) < NUM_CLASSES)
         {extra_affect   = class_app[(int) GET_CLASS(ch)].extra_affects;
          extra_modifier = class_app[(int) GET_CLASS(ch)].extra_modifiers;
	
          for (i = 0; extra_affect && (extra_affect+i)->affect != -1; i++)
              affect_modify(ch,APPLY_NONE,0,(extra_affect+i)->affect,(extra_affect+i)->set_or_clear);
          for (i = 0; extra_modifier && (extra_modifier + i)->location != -1; i++)
              affect_modify(ch,(extra_modifier+i)->location,(extra_modifier+i)->modifier,0,TRUE);
         }

      if (GET_RACE(ch) < NUM_RACES)
         {extra_affect   = race_app[(int) GET_RACE(ch)].extra_affects;
          extra_modifier = race_app[(int) GET_RACE(ch)].extra_modifiers;
          for (i = 0; extra_affect && (extra_affect+i)->affect != -1; i++)
              affect_modify(ch,APPLY_NONE,0,(extra_affect+i)->affect,(extra_affect+i)->set_or_clear);
          for (i = 0; extra_modifier && (extra_modifier + i)->location != -1; i++)
              affect_modify(ch,(extra_modifier+i)->location,(extra_modifier+i)->modifier,0,TRUE);
         }
       // Apply other PC modifiers
       switch (IS_CARRYING_W(ch) * 10 / MAX(1,CAN_CARRY_W(ch)))
       {case 10: case 9: case 8:
             GET_DEX_ADD(ch) -= 2;
             break;
        case 7: case 6: case 5:
             GET_DEX_ADD(ch) -= 1;
             break;
       }
       GET_DR_ADD(ch) += extra_damroll((int)GET_CLASS(ch),(int)GET_LEVEL(ch));
       GET_HITREG(ch) += ((int)GET_LEVEL(ch) + 4) / 5 * 10;
       if (GET_CON_ADD(ch))
          {i = class_app[(int)GET_CLASS(ch)].koef_con * GET_CON_ADD(ch) * GET_LEVEL(ch) / 100;
           GET_HIT_ADD(ch) += i;
           if ((i = GET_MAX_HIT(ch) + GET_HIT_ADD(ch)) < 1)
              GET_HIT_ADD(ch) -= (i-1);
          }
       if (!WAITLESS(ch) && on_horse(ch))
          {REMOVE_BIT(AFF_FLAGS(ch,AFF_HIDE),AFF_HIDE);
           REMOVE_BIT(AFF_FLAGS(ch,AFF_SNEAK),AFF_SNEAK);
           REMOVE_BIT(AFF_FLAGS(ch,AFF_CAMOUFLAGE),AFF_CAMOUFLAGE);
           REMOVE_BIT(AFF_FLAGS(ch,AFF_INVISIBLE),AFF_INVISIBLE);      
	  }
     }

  /* correctize all weapon */
  if (!IS_NPC(ch) && (obj=GET_EQ(ch,WEAR_BOTHS)) && !IS_IMMORTAL(ch) && !OK_BOTH(ch,obj))
     {act("Вам слишком тяжело держать $o3 в обоих руках !",FALSE,ch,obj,0,TO_CHAR);
      act("$n прекратил$g использовать $o3.",FALSE,ch,obj,0,TO_ROOM);
      obj_to_char(unequip_char(ch,WEAR_BOTHS),ch);
      return;
     }
  if (!IS_NPC(ch) && (obj=GET_EQ(ch,WEAR_WIELD)) && !IS_IMMORTAL(ch) && !OK_WIELD(ch,obj))
     {act("Вам слишком тяжело держать $o3 в правой руке !",FALSE,ch,obj,0,TO_CHAR);
      act("$n прекратил$g использовать $o3.",FALSE,ch,obj,0,TO_ROOM);
      obj_to_char(unequip_char(ch,WEAR_WIELD),ch);
      return;
     }
  if (!IS_NPC(ch) && (obj=GET_EQ(ch,WEAR_HOLD)) && !IS_IMMORTAL(ch) && !OK_HELD(ch,obj))
     {act("Вам слишком тяжело держать $o3 в левой руке !",FALSE,ch,obj,0,TO_CHAR);
      act("$n прекратил$g использовать $o3.",FALSE,ch,obj,0,TO_ROOM);
      obj_to_char(unequip_char(ch,WEAR_HOLD),ch);
      return;
     }

  /* calculate DAMAGE value */
  GET_DAMAGE(ch) = (str_app[STRENGTH_APPLY_INDEX(ch)].todam + GET_REAL_DR(ch)) * 2;
  if ((obj = GET_EQ(ch,WEAR_BOTHS)) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
     GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2)+1)) >> 1;
  else
     {if ((obj = GET_EQ(ch,WEAR_WIELD)) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
         GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2)+1)) >> 1;
      if ((obj = GET_EQ(ch,WEAR_HOLD)) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
         GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2)+1)) >> 1;
     }

  /* Calculate CASTER value */
  for (GET_CASTER(ch) = 0, i = 1; !IS_NPC(ch) && i <= MAX_SPELLS; i++)
      if (IS_SET(GET_SPELL_TYPE(ch,i), SPELL_KNOW | SPELL_TEMP))
         GET_CASTER(ch) += (spell_info[i].danger * GET_SPELL_MEM(ch,i));

  /* Check steal affects */
  for (i = 0; (j = char_stealth_aff[i]); i++)
      {if ( IS_SET(GET_FLAG(saved,j),j) &&
           !IS_SET(AFF_FLAGS(ch,j), j)
	  )
          CHECK_AGRO(ch) = TRUE;
      }
      
  if (FIGHTING(ch))
     {REMOVE_BIT(AFF_FLAGS(ch,AFF_HIDE),AFF_HIDE);
      REMOVE_BIT(AFF_FLAGS(ch,AFF_SNEAK),AFF_SNEAK);
      REMOVE_BIT(AFF_FLAGS(ch,AFF_CAMOUFLAGE),AFF_CAMOUFLAGE);      
      REMOVE_BIT(AFF_FLAGS(ch,AFF_INVISIBLE),AFF_INVISIBLE);      
     }
}



/* Insert an affect_type in a char_data structure
   Automatically sets apropriate bits and apply's */
void affect_to_char(struct char_data * ch, struct affected_type * af)
{ long   was_lgt = AFF_FLAGGED(ch,AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hlgt= AFF_FLAGGED(ch,AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hdrk= AFF_FLAGGED(ch,AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO;
  struct affected_type *affected_alloc;

  CREATE(affected_alloc, struct affected_type, 1);

  *affected_alloc      = *af;
  affected_alloc->next = ch->affected;
  ch->affected         = affected_alloc;

  affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);
  //log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Start");
  affect_total(ch);
  //log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Stop");
  check_light(ch,LIGHT_UNDEF,was_lgt,was_hlgt,was_hdrk,1);
}



/*
 * Remove an affected_type structure from a char (called when duration
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply
 */

void affect_remove(struct char_data * ch, struct affected_type * af)
{ int    was_lgt = AFF_FLAGGED(ch, AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hlgt= AFF_FLAGGED(ch, AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hdrk= AFF_FLAGGED(ch, AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO,
         duration;

  struct affected_type *temp;
  int    change = 0;

  // if (IS_IMMORTAL(ch))
  //   {sprintf(buf,"<%d>\r\n",was_hdrk);
  //    send_to_char(buf,ch);
  //   }

  if (ch->affected == NULL)
     {log("SYSERR: affect_remove(%s) when no affects...",GET_NAME(ch));
      // core_dump();
      return;
     }

  affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);
  if (af->type == SPELL_ABSTINENT)
     {GET_DRUNK_STATE(ch) = GET_COND(ch,DRUNK) = MIN(GET_COND(ch,DRUNK), CHAR_DRUNKED-1);
     }
  else
  if (af->type == SPELL_DRUNKED)
     {duration = pc_duration(ch,3,MAX(0,GET_DRUNK_STATE(ch)-CHAR_DRUNKED),0,0,0);
      if (af->location == APPLY_AC)
         {af->type      = SPELL_ABSTINENT;
          af->duration  = duration;
          af->modifier  = 20;
          af->bitvector = AFF_ABSTINENT;
          change        = 1;
         }
      else
      if (af->location == APPLY_HITROLL)
         {af->type     = SPELL_ABSTINENT;
          af->duration  = duration;
          af->modifier  = -2;
          af->bitvector = AFF_ABSTINENT;
          change        = 1;
         }
      else
      if (af->location == APPLY_DAMROLL)
         {af->type      = SPELL_ABSTINENT;
          af->duration  = duration;
          af->modifier  = -2;
          af->bitvector = AFF_ABSTINENT;
          change        = 1;
         }
     }
  if (change)
     affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);
  else
     {REMOVE_FROM_LIST(af, ch->affected, next);
      free(af);
     }
  //log("[AFFECT_REMOVE->AFFECT_TOTAL] Start");
  affect_total(ch);
  //log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Stop");
  check_light(ch,LIGHT_UNDEF,was_lgt,was_hlgt,was_hdrk,1);
}



/* Call affect_remove with every spell of spelltype "skill" */
void affect_from_char(struct char_data * ch, int type)
{ struct affected_type *hjp, *next;

  for (hjp = ch->affected; hjp; hjp = next)
      {next = hjp->next;
       if (hjp->type == type)
          {affect_remove(ch, hjp);
          }
      }

  if (IS_NPC(ch) && type == SPELL_CHARM)
     EXTRACT_TIMER(ch) = 5;
}

/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX),
 * FALSE indicates not affected.
 */
bool affected_by_spell(struct char_data * ch, int type)
{
  struct affected_type *hjp;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
      if (hjp->type == type)
         return (TRUE);

  return (FALSE);
}


void affect_join(struct char_data * ch, struct affected_type * af,
		      bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
  struct affected_type *hjp;
  bool   found = FALSE;

  for (hjp = ch->affected; !found && hjp && af->location; hjp = hjp->next)
      {if ((hjp->type == af->type) && (hjp->location == af->location))
          {if (add_dur)
              af->duration += hjp->duration;
           if (avg_dur)
              af->duration /= 2;
           if (add_mod)
              af->modifier += hjp->modifier;
           if (avg_mod)
              af->modifier /= 2;
           affect_remove(ch, hjp);
           affect_to_char(ch, af);
           found = TRUE;
          }
      }
  if (!found)
     {affect_to_char(ch, af);
     }
}

/* Insert an timed_type in a char_data structure */
void timed_to_char(struct char_data * ch, struct timed_type * timed)
{ struct timed_type *timed_alloc;

  CREATE(timed_alloc, struct timed_type, 1);

  *timed_alloc      = *timed;
  timed_alloc->next = ch->timed;
  ch->timed         = timed_alloc;
}

void timed_from_char(struct char_data * ch, struct timed_type * timed)
{
  struct timed_type *temp;

  if (ch->timed == NULL)
     {log("SYSERR: timed_from_char(%s) when no timed...",GET_NAME(ch));
      // core_dump();
      return;
     }

  REMOVE_FROM_LIST(timed, ch->timed, next);
  free(timed);
}

int timed_by_skill(struct char_data * ch, int skill)
{
  struct timed_type *hjp;

  for (hjp = ch->timed; hjp; hjp = hjp->next)
      if (hjp->skill == skill)
         return (hjp->time);

  return (0);
}


/* move a player out of a room */
void char_from_room(struct char_data * ch)
{ struct char_data *temp;

  if (ch == NULL || ch->in_room == NOWHERE)
     {log("SYSERR: NULL character or NOWHERE in %s, char_from_room", __FILE__);
      return;
     }

  if (FIGHTING(ch) != NULL)
     stop_fighting(ch,TRUE);

  check_light(ch,LIGHT_NO,LIGHT_NO,LIGHT_NO,LIGHT_NO,-1);
  REMOVE_FROM_LIST(ch, world[ch->in_room].people, next_in_room);
  ch->in_room      = NOWHERE;
  ch->next_in_room = NULL;
}


/* place a character in a room */
void char_to_room(struct char_data * ch, room_rnum room)
{
  if (ch == NULL || room < 0 || room > top_of_world)
     log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p",
		  room, top_of_world, ch);
  else
     {ch->next_in_room   = world[room].people;
      world[room].people = ch;
      ch->in_room        = room;
      check_light(ch,LIGHT_NO,LIGHT_NO,LIGHT_NO,LIGHT_NO,1);
      REMOVE_BIT(EXTRA_FLAGS(ch,EXTRA_FAILHIDE), EXTRA_FAILHIDE);
      REMOVE_BIT(EXTRA_FLAGS(ch,EXTRA_FAILSNEAK), EXTRA_FAILSNEAK);
      REMOVE_BIT(EXTRA_FLAGS(ch,EXTRA_FAILCAMOUFLAGE), EXTRA_FAILCAMOUFLAGE);
      if (IS_GRGOD(ch) && PRF_FLAGGED(ch,PRF_CODERINFO))
         {sprintf(buf,"%sКомната=%s%d %sСвет=%s%d %sОсвещ=%s%d %sКостер=%s%d \r\n"
	                  "%sТьма=%s%d %sЗапрет=%s%d %sСолнце=%s%d %sНебо=%s%d %sЛуна=%s%d.\r\n",
	              CCNRM(ch,C_NRM), CCINRM(ch, C_NRM), room,
	              CCRED(ch,C_NRM), CCIRED(ch, C_NRM), world[room].light,
	              CCGRN(ch,C_NRM), CCIGRN(ch, C_NRM), world[room].glight,
	          	  CCYEL(ch,C_NRM), CCIYEL(ch, C_NRM), world[room].fires,
	          	  CCBLU(ch,C_NRM), CCIBLU(ch, C_NRM), world[room].gdark,
	          	  CCMAG(ch,C_NRM), CCIMAG(ch, C_NRM), world[room].forbidden,
	          	  CCCYN(ch,C_NRM), CCICYN(ch, C_NRM), weather_info.sky,
	          	  CCWHT(ch,C_NRM), CCIWHT(ch, C_NRM), weather_info.sunlight,
	          	  CCYEL(ch,C_NRM), CCIYEL(ch, C_NRM), weather_info.moon_day	          	
	          	  );
          send_to_char(buf,ch);
         }

      /* Stop fighting now, if we left. */
      if (FIGHTING(ch) && IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)))
         {stop_fighting(FIGHTING(ch),FALSE);
          stop_fighting(ch,TRUE);
         }
     }
}

void restore_object(struct obj_data *obj, struct char_data *ch)
{int i,j;
 if ((i = GET_OBJ_RNUM(obj)) < 0)
    return;
 if (GET_OBJ_OWNER(obj) &&
     OBJ_FLAGGED(obj, ITEM_NODONATE) &&
     (!ch || GET_UNIQUE(ch) != GET_OBJ_OWNER(obj))
    )
    {GET_OBJ_VAL(obj,0) = GET_OBJ_VAL(obj_proto+i, 0);
     GET_OBJ_VAL(obj,1) = GET_OBJ_VAL(obj_proto+i, 1);
     GET_OBJ_VAL(obj,2) = GET_OBJ_VAL(obj_proto+i, 2);
     GET_OBJ_VAL(obj,3) = GET_OBJ_VAL(obj_proto+i, 3);
     GET_OBJ_MATER(obj) = GET_OBJ_MATER(obj_proto+i);
     GET_OBJ_MAX(obj)   = GET_OBJ_MAX(obj_proto+i);
     GET_OBJ_CUR(obj)   = 1;
     GET_OBJ_WEIGHT(obj)= GET_OBJ_WEIGHT(obj_proto+i);
     GET_OBJ_TIMER(obj) = 24*60;
     obj->obj_flags.extra_flags = (obj_proto+i)->obj_flags.extra_flags;
     obj->obj_flags.affects     = (obj_proto+i)->obj_flags.affects;
     GET_OBJ_WEAR(obj)  = GET_OBJ_WEAR(obj_proto+i);
     GET_OBJ_OWNER(obj) = 0;
     for (j = 0; j < MAX_OBJ_AFFECT; j++)
         obj->affected[j] = (obj_proto+i)->affected[j];
    }
}


/* give an object to a char   */
void obj_to_char(struct obj_data * object, struct char_data * ch)
{ struct obj_data *obj;
  int    may_carry = TRUE;
  if (object && ch)
     {restore_object(object,ch);
      if (invalid_anti_class(ch, object) || invalid_unique(ch, object))
         may_carry = FALSE;

      for (obj = object->contains; obj && may_carry; obj = obj->next_content)
          if (invalid_anti_class(ch, obj) || invalid_unique(ch,obj))
             may_carry = FALSE;

      if (!may_carry)
         {act("Вас обожгло при попытке взять $o3.", FALSE, ch, object, 0, TO_CHAR);
          act("$n попытал$u взять $o3 - и чудом не сгорел$g.", FALSE, ch, object, 0, TO_ROOM);
          obj_to_room(object, IN_ROOM(ch));
          return;
         }
	
      if (!IS_NPC(ch))
         SET_BIT(GET_OBJ_EXTRA(object, ITEM_TICKTIMER), ITEM_TICKTIMER);
	
      object->next_content = ch->carrying;
      ch->carrying         = object;
      object->carried_by   = ch;
      object->in_room      = NOWHERE;
      IS_CARRYING_W(ch)   += GET_OBJ_WEIGHT(object);
      IS_CARRYING_N(ch)++;

      /* set flag for crash-save system, but not on mobs! */
      if (!IS_NPC(ch))
         SET_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
     }
  else
     log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
}


/* take an object from a char */
void obj_from_char(struct obj_data * object)
{
  struct obj_data *temp;

  if (object == NULL)
     {log("SYSERR: NULL object passed to obj_from_char.");
      return;
     }
  REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

  /* set flag for crash-save system, but not on mobs! */
  if (!IS_NPC(object->carried_by))
     SET_BIT(PLR_FLAGS(object->carried_by, PLR_CRASH), PLR_CRASH);

  IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->carried_by)--;
  object->carried_by   = NULL;
  object->next_content = NULL;
}



/* Return the effect of a piece of armor in position eq_pos */
int apply_ac(struct char_data * ch, int eq_pos)
{
  int factor = 1;

  if (GET_EQ(ch, eq_pos) == NULL)
     {log("SYSERR: apply_ac(%s,%d) when no equip...",GET_NAME(ch), eq_pos);
      // core_dump();
      return (0);
     }

  if (!(GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) == ITEM_ARMOR))
     return (0);

  switch (eq_pos)
         {
  case WEAR_BODY:
    factor = 3;
    break;			/* 30% */
  case WEAR_HEAD:
    factor = 2;
    break;			/* 20% */
  case WEAR_LEGS:
    factor = 2;
    break;			/* 20% */
  default:
    factor = 1;
    break;			/* all others 10% */
          }

  return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
}

int apply_armour(struct char_data * ch, int eq_pos)
{ int    factor = 1;
  struct obj_data *obj = GET_EQ(ch,eq_pos);

  if (!obj)
     {log("SYSERR: apply_armor(%s,%d) when no equip...",GET_NAME(ch),eq_pos);
      // core_dump();
      return (0);
     }

  if (!(GET_OBJ_TYPE(obj) == ITEM_ARMOR))
     return (0);

  switch (eq_pos)
         {
  case WEAR_BODY:
    factor = 3;
    break;			/* 30% */
  case WEAR_HEAD:
    factor = 2;
    break;			/* 20% */
  case WEAR_LEGS:
    factor = 2;
    break;			/* 20% */
  default:
    factor = 1;
    break;			/* all others 10% */
          }


  return ( factor * GET_OBJ_VAL(obj, 1) * GET_OBJ_CUR(obj) / MAX(1,GET_OBJ_MAX(obj)) );
}

int invalid_align(struct char_data *ch, struct obj_data *obj)
{ if (IS_NPC(ch) || IS_IMMORTAL(ch))
     return (FALSE);
  if (IS_OBJ_ANTI(obj, ITEM_AN_MONO) && GET_RELIGION(ch) == RELIGION_MONO)
     return TRUE;
  if (IS_OBJ_ANTI(obj, ITEM_AN_POLY) && GET_RELIGION(ch) == RELIGION_POLY)
     return TRUE;
  return FALSE;
}


int preequip_char(struct char_data * ch, struct obj_data * obj, int pos)
{ int    was_lgt = AFF_FLAGGED(ch, AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hlgt= AFF_FLAGGED(ch, AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hdrk= AFF_FLAGGED(ch, AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO,
         was_lamp= FALSE;

  if (pos < 0 || pos >= NUM_WEARS)
     {log("SYSERR: preequip(%s,%d) in unknown pos...",GET_NAME(ch),pos);
      // core_dump();
      return(FALSE);
     }

  if (GET_EQ(ch, pos))
     {log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch),
	      obj->short_description);
      return(FALSE);
     }
  if (obj->carried_by)
     {log("SYSERR: EQUIP: %s - Obj is carried_by when equip.", OBJN(obj,ch,0));
      return(FALSE);
     }
  if (obj->in_room != NOWHERE)
     {log("SYSERR: EQUIP: %s - Obj is in_room when equip.", OBJN(obj,ch,0));
      return(FALSE);
     }

  if (invalid_anti_class(ch, obj))
     {act("Вас обожгло при попытке надеть $o3.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n попытал$u использовать $o3 - и чудом не обгорел$g.", FALSE, ch, obj, 0, TO_ROOM);
      obj_to_room(obj, IN_ROOM(ch));
      return(FALSE);
     }

  if ((!IS_NPC(ch) && invalid_align(ch, obj)) ||
      invalid_no_class(ch, obj)               ||
      (IS_NPC(ch) && (OBJ_FLAGGED(obj,ITEM_SHARPEN) ||
                      OBJ_FLAGGED(obj,ITEM_ARMORED)))
     )
     {act("$o3 явно не предназначен$A для Вас.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n попытал$u использовать $o3, но у н$s ничего не получилось.", FALSE, ch, obj, 0, TO_ROOM);
      obj_to_char(obj, ch);
      return(FALSE);
     }

  if (GET_EQ(ch,WEAR_LIGHT) &&
      GET_OBJ_TYPE(GET_EQ(ch,WEAR_LIGHT)) == ITEM_LIGHT &&
      GET_OBJ_VAL(GET_EQ(ch,WEAR_LIGHT), 2))
     was_lamp = TRUE;

  GET_EQ(ch, pos)   = obj;
  CHECK_AGRO(ch)    = TRUE;
  obj->worn_by      = ch;
  obj->worn_on      = pos;
  obj->next_content = NULL;

  if (ch->in_room == NOWHERE)
     log("SYSERR: ch->in_room = NOWHERE when equipping char %s.", GET_NAME(ch));

  //log("[PREEQUIP_CHAR(%s)->AFFECT_TOTAL] Start",GET_NAME(ch));
  affect_total(ch);
  //log("[PREEQUIP_CHAR(%s)->AFFECT_TOTAL] Stop",GET_NAME(ch));
  check_light(ch,was_lamp,was_lgt,was_hlgt,was_hdrk,1);
  return (TRUE);
}

void postequip_char(struct char_data * ch, struct obj_data * obj)
{ int j;

  if (IN_ROOM(ch) != NOWHERE)
     for (j = 0; weapon_affect[j].aff_bitvector >= 0; j++)
         {if (!IS_SET(GET_FLAG(obj->obj_flags.affects,weapon_affect[j].aff_pos),
                      weapon_affect[j].aff_pos) ||
              weapon_affect[j].aff_spell == 0)
             continue;
          if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
             {act("Магия $o1 потерпела неудачу и развеялась по воздуху",FALSE,ch,obj,0,TO_ROOM);
              act("Магия $o1 потерпела неудачу и развеялась по воздуху",FALSE,ch,obj,0,TO_CHAR);
             }
          else
             {mag_affects(obj->obj_flags.Obj_level, ch, ch,
                          weapon_affect[j].aff_spell, SAVING_SPELL);
             }
         }
}




void equip_char(struct char_data * ch, struct obj_data * obj, int pos)
{ int    was_lgt = AFF_FLAGGED(ch, AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hlgt= AFF_FLAGGED(ch, AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hdrk= AFF_FLAGGED(ch, AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO,
         was_lamp= FALSE;
  int j, skip_total = IS_SET(pos,0x80), no_cast = IS_SET(pos,0x40);

  REMOVE_BIT(pos,(0x80|0x40));

  if (pos < 0 || pos >= NUM_WEARS)
     {log("SYSERR: equip_char(%s,%d) in unknown pos...",GET_NAME(ch),pos);
      // core_dump();
      return;
     }

  if (GET_EQ(ch, pos))
     {log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch),
	      obj->short_description);
      return;
     }
  if (obj->carried_by)
     {log("SYSERR: EQUIP: %s - Obj is carried_by when equip.", OBJN(obj,ch,0));
      return;
     }
  if (obj->in_room != NOWHERE)
     {log("SYSERR: EQUIP: %s - Obj is in_room when equip.", OBJN(obj,ch,0));
      return;
     }

  if (invalid_anti_class(ch, obj))
     {act("Вас обожгло при попытке надеть $o3.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n попытал$u одеть $o3 - и чудом не обгорел$g.", FALSE, ch, obj, 0, TO_ROOM);
      obj_to_room(obj, IN_ROOM(ch));
      return;
     }

  if ((!IS_NPC(ch) && invalid_align(ch, obj)) || invalid_no_class(ch, obj))
     {act("$o3 явно не предназначен$A для Вас.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n попытал$u одеть $o3, но у н$s ничего не получилось.", FALSE, ch, obj, 0, TO_ROOM);
      obj_to_char(obj, ch);
      return;
     }

  if (GET_EQ(ch,WEAR_LIGHT) &&
      GET_OBJ_TYPE(GET_EQ(ch,WEAR_LIGHT)) == ITEM_LIGHT &&
      GET_OBJ_VAL(GET_EQ(ch,WEAR_LIGHT), 2))
     was_lamp = TRUE;

  GET_EQ(ch, pos)   = obj;
  CHECK_AGRO(ch)    = TRUE;
  obj->worn_by      = ch;
  obj->worn_on      = pos;
  obj->next_content = NULL;

  if (ch->in_room == NOWHERE)
     log("SYSERR: ch->in_room = NOWHERE when equipping char %s.", GET_NAME(ch));


  for (j = 0; j < MAX_OBJ_AFFECT; j++)
      affect_modify(ch, obj->affected[j].location,
		            obj->affected[j].modifier,
		            0, TRUE);

  if (IN_ROOM(ch) != NOWHERE)
     for (j = 0; weapon_affect[j].aff_bitvector >= 0; j++)
         {if (!IS_SET(GET_FLAG(obj->obj_flags.affects,weapon_affect[j].aff_pos),
                      weapon_affect[j].aff_pos) ||
              weapon_affect[j].aff_spell == 0)
             continue;
          if (!no_cast)
             {log("[EQUIPPING] Casting magic...");
              if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
                 {act("Магия $o1 потерпела неудачу и развеялась по воздуху",FALSE,ch,obj,0,TO_ROOM);
                  act("Магия $o1 потерпела неудачу и развеялась по воздуху",FALSE,ch,obj,0,TO_CHAR);
                 }
              else
                 {mag_affects(obj->obj_flags.Obj_level, ch, ch,
                              weapon_affect[j].aff_spell, SAVING_SPELL);
                 }
             }
         }

  if (!skip_total)
     {//log("[EQUIP_CHAR(%s)->AFFECT_TOTAL] Start",GET_NAME(ch));
      affect_total(ch);
      //log("[EQUIP_CHAR(%s)->AFFECT_TOTAL] Stop",GET_NAME(ch));
      check_light(ch,was_lamp,was_lgt,was_hlgt,was_hdrk,1);
     }
}



struct obj_data *unequip_char(struct char_data * ch, int pos)
{ int    was_lgt = AFF_FLAGGED(ch, AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hlgt= AFF_FLAGGED(ch, AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hdrk= AFF_FLAGGED(ch, AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO,
         was_lamp= FALSE;

  int    j, skip_total = IS_SET(pos,0x80);
  struct obj_data *obj;

  REMOVE_BIT(pos,(0x80|0x40));

  if ((pos < 0 || pos >= NUM_WEARS) || GET_EQ(ch, pos) == NULL)
     {log("SYSERR: unequip_char(%s,%d) - unused pos or no equip...", GET_NAME(ch), pos);
      // core_dump();
      return (NULL);
     }

  if (GET_EQ(ch,WEAR_LIGHT) &&
      GET_OBJ_TYPE(GET_EQ(ch,WEAR_LIGHT)) == ITEM_LIGHT &&
      GET_OBJ_VAL(GET_EQ(ch,WEAR_LIGHT), 2))
     was_lamp = TRUE;

  obj               = GET_EQ(ch, pos);
  obj->worn_by      = NULL;
  obj->next_content = NULL;
  obj->worn_on      = -1;
  GET_EQ(ch, pos)   = NULL;

  if (ch->in_room == NOWHERE)
     log("SYSERR: ch->in_room = NOWHERE when unequipping char %s.", GET_NAME(ch));

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
      affect_modify(ch, obj->affected[j].location,
	            obj->affected[j].modifier,
	            0, FALSE);
		
  if (IN_ROOM(ch) != NOWHERE)		
     for (j = 0; weapon_affect[j].aff_bitvector >= 0; j++)
         {if (!IS_SET(GET_FLAG(obj->obj_flags.affects,weapon_affect[j].aff_pos),
                      weapon_affect[j].aff_pos) ||
              weapon_affect[j].aff_bitvector == 0
             )
             continue;
          affect_modify(ch, APPLY_NONE, 0, weapon_affect[j].aff_bitvector, FALSE);
         }

  if (!skip_total)
     {//log("[UNEQUIP_CHAR->AFFECT_TOTAL] Start");
      affect_total(ch);
      //log("[UNEQUIP_CHAR->AFFECT_TOTAL] Stop");
      check_light(ch,was_lamp,was_lgt,was_hlgt,was_hdrk,1);
     }

  return (obj);
}


int get_number(char **name)
{
  int i;
  char *ppos;
  char number[MAX_INPUT_LENGTH];

  *number = '\0';

  if ((ppos = strchr(*name, '.')) != NULL)
     {*ppos = '\0';
      strcpy(number, *name);
      for (i = 0; *(number + i); i++)
          {if (!isdigit(*(number + i)))
	          {*ppos = '.';
	           return (1);
	          }
	      }
      strcpy(*name, ppos+1);
      return (atoi(number));
     }
  return (1);
}



/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *get_obj_in_list_num(int num, struct obj_data * list)
{
  struct obj_data *i;

  for (i = list; i; i = i->next_content)
      if (GET_OBJ_RNUM(i) == num)
         return (i);

  return (NULL);
}



/* search the entire world for an object number, and return a pointer  */
struct obj_data *get_obj_num(obj_rnum nr)
{
  struct obj_data *i;

  for (i = object_list; i; i = i->next)
      if (GET_OBJ_RNUM(i) == nr)
         return (i);

  return (NULL);
}



/* search a room for a char, and return a pointer if found..  */
struct char_data *get_char_room(char *name, room_rnum room)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
     return (NULL);

  for (i = world[room].people; i && (j <= number); i = i->next_in_room)
      if (isname(tmp, i->player.name))
         if (++j == number)
	        return (i);

  return (NULL);
}



/* search all over the world for a char num, and return a pointer if found */
struct char_data *get_char_num(mob_rnum nr)
{
  struct char_data *i;

  for (i = character_list; i; i = i->next)
      if (GET_MOB_RNUM(i) == nr)
         return (i);

  return (NULL);
}


const int money_destroy_timer  = 60;
const int death_destroy_timer  = 5;
const int room_destroy_timer   = 10;
const int room_nodestroy_timer = -1;
const int script_destroy_timer = 1; /** !!! Never set less than ONE **/

/* put an object in a room */
void obj_to_room(struct obj_data * object, room_rnum room)
{ int sect = 0;
  if (!object || room < 0 || room > top_of_world)
     {log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d/%d, obj %p)",
	      room, top_of_world, object);
      if (object)
         extract_obj(object);
     }
  else
     {restore_object(object,0);
      object->next_content = world[room].contents;
      world[room].contents = object;
      object->in_room      = room;
      object->carried_by   = NULL;
      object->worn_by      = NULL;
      sect                 = real_sector(room);
      if (ROOM_FLAGGED(room, ROOM_HOUSE))
         SET_BIT(ROOM_FLAGS(room, ROOM_HOUSE_CRASH), ROOM_HOUSE_CRASH);
      if (object->proto_script || object->script)
         GET_OBJ_DESTROY(object)  = script_destroy_timer;
      else
      if (OBJ_FLAGGED(object, ITEM_NODECAY))
         GET_OBJ_DESTROY(object)  = room_nodestroy_timer;
      else
      if ((
           (sect == SECT_WATER_SWIM || sect == SECT_WATER_NOSWIM) &&
           !IS_CORPSE(object) &&
           !OBJ_FLAGGED(object, ITEM_SWIMMING)
          ) ||
          ((sect == SECT_FLYING ) &&
           !IS_CORPSE(object) &&
           !OBJ_FLAGGED(object, ITEM_FLYING)
          )
         )
         {extract_obj(object);
         }
      else
      if (OBJ_FLAGGED(object, ITEM_DECAY) ||
          (OBJ_FLAGGED(object, ITEM_ZONEDECAY) &&
           GET_OBJ_ZONE(object) != NOWHERE &&
           GET_OBJ_ZONE(object) != world[room].zone
	  )
	 )
         {act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер", FALSE,
              world[room].people, object, 0, TO_ROOM);
          act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер", FALSE,
              world[room].people, object, 0, TO_CHAR);
          extract_obj(object);
         }
      else
      if (GET_OBJ_TYPE(object) == ITEM_MONEY)
         GET_OBJ_DESTROY(object)  = money_destroy_timer;
      else
      if (ROOM_FLAGGED(room, ROOM_DEATH))
         GET_OBJ_DESTROY(object)  = death_destroy_timer;
      else
         GET_OBJ_DESTROY(object)  = room_destroy_timer;
     }
}


/* Take an object from a room */
void obj_from_room(struct obj_data * object)
{
  struct obj_data *temp;

  if (!object || object->in_room == NOWHERE)
     {log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room",
	       object, object->in_room);
      return;
     }

  REMOVE_FROM_LIST(object, world[object->in_room].contents, next_content);

  if (ROOM_FLAGGED(object->in_room, ROOM_HOUSE))
     SET_BIT(ROOM_FLAGS(object->in_room, ROOM_HOUSE_CRASH), ROOM_HOUSE_CRASH);
  object->in_room      = NOWHERE;
  object->next_content = NULL;
}


/* put an object in an object (quaint)  */
void obj_to_obj(struct obj_data * obj, struct obj_data * obj_to)
{
  struct obj_data *tmp_obj;

  if (!obj || !obj_to || obj == obj_to)
     {log("SYSERR: NULL object (%p) or same source (%p) and target (%p) obj passed to obj_to_obj.",
	      obj, obj, obj_to);
      return;
     }

  obj->next_content = obj_to->contains;
  obj_to->contains  = obj;
  obj->in_obj       = obj_to;

  for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
      GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);

  /* top level object.  Subtract weight from inventory if necessary. */
  GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
  if (tmp_obj->carried_by)
     IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
}


/* remove an object from an object */
void obj_from_obj(struct obj_data * obj)
{
  struct obj_data *temp, *obj_from;

  if (obj->in_obj == NULL)
     {log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
      return;
     }
  obj_from = obj->in_obj;
  REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

  /* Subtract weight from containers container */
  for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
      GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);

  /* Subtract weight from char that carries the object */
  GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
  if (temp->carried_by)
     IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj);

  obj->in_obj       = NULL;
  obj->next_content = NULL;
}


/* Set all carried_by to point to new owner */
void object_list_new_owner(struct obj_data * list, struct char_data * ch)
{
  if (list)
     {object_list_new_owner(list->contains, ch);
      object_list_new_owner(list->next_content, ch);
      list->carried_by = ch;
     }
}


/* Extract an object from the world */
void extract_obj(struct obj_data * obj)
{ char   name[MAX_STRING_LENGTH];
  struct obj_data *temp;

  strcpy(name,obj->PNames[0]);
  log("Start extract obj %s",name);
  if (obj->worn_by != NULL)
     if (unequip_char(obj->worn_by, obj->worn_on) != obj)
        log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
  if (obj->in_room != NOWHERE)
     obj_from_room(obj);
  else
  if (obj->carried_by)
     obj_from_char(obj);
  else
  if (obj->in_obj)
     obj_from_obj(obj);

  /* Get rid of the contents of the object, as well. */
  while (obj->contains)
        extract_obj(obj->contains);

  check_auction(NULL,obj);
  REMOVE_FROM_LIST(obj, object_list, next);

  if (GET_OBJ_RNUM(obj) >= 0)
     (obj_index[GET_OBJ_RNUM(obj)].number)--;

  if (SCRIPT(obj))
     extract_script(SCRIPT(obj));

  free_obj(obj);
  log("Stop extract obj %s",name);
}



void update_object(struct obj_data * obj, int use)
{
  /* dont update objects with a timer trigger */
  if (!SCRIPT_CHECK(obj, OTRIG_TIMER) &&
      GET_OBJ_TIMER(obj) > 0          &&
      OBJ_FLAGGED(obj, ITEM_TICKTIMER)
     )
     GET_OBJ_TIMER(obj) -= use;
  if (obj->contains)
     update_object(obj->contains, use);
  if (obj->next_content)
     update_object(obj->next_content, use);
}


void update_char_objects(struct char_data * ch)
{
  int i;

  if (GET_EQ(ch, WEAR_LIGHT) != NULL)
     {if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
         {if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2) > 0)
             {i = --GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2);
  	      if (i == 1)
	         {act("Ваш$G $o замерцал$G и начал$G угасать.\r\n",
	              FALSE, ch, GET_EQ(ch, WEAR_LIGHT), 0, TO_CHAR);
                  act("$o $n1 замерцал$G и начал$G угасать.",
	              FALSE, ch, GET_EQ(ch, WEAR_LIGHT), 0, TO_ROOM);
                 }
              else
              if (i == 0)
                 {act("Ваш$G $o погас$Q.\r\n",
                      FALSE, ch, GET_EQ(ch, WEAR_LIGHT), 0, TO_CHAR);
                  act("$o $n1 погас$Q.",
                      FALSE, ch, GET_EQ(ch, WEAR_LIGHT), 0, TO_ROOM);
		  if (IN_ROOM(ch) != NOWHERE)
                     {if (world[IN_ROOM(ch)].light > 0)
                         world[IN_ROOM(ch)].light -= 1;
                     }
                  if (OBJ_FLAGGED(GET_EQ(ch,WEAR_LIGHT), ITEM_DECAY))
                     extract_obj(GET_EQ(ch,WEAR_LIGHT));
                 }
             }
         }
     }

  for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i))
         update_object(GET_EQ(ch, i), 1);

  if (ch->carrying)
     update_object(ch->carrying, 1);
}

void change_fighting(struct char_data *ch, int need_stop)
{ struct char_data *k, *j, *temp;

  for (k = character_list; k; k = temp)
      {temp = k->next;
       if (PROTECTING(k) == ch)
          {PROTECTING(k) = NULL;
           CLR_AF_BATTLE(k,EAF_PROTECT);
          }
       if (TOUCHING(k) == ch)
          {TOUCHING(k) = NULL;
           CLR_AF_BATTLE(k,EAF_PROTECT);
          }
       if (GET_EXTRA_VICTIM(k) == ch)
          SET_EXTRA(k, 0, NULL);
       if (GET_CAST_CHAR(k) == ch)
          SET_CAST(k, 0, NULL, NULL);
       if (FIGHTING(k) == ch && IN_ROOM(k) != NOWHERE)
          {log("[Change fighting] Change victim");
           for (j = world[IN_ROOM(ch)].people; j; j = j->next_in_room)
               if (FIGHTING(j) == k)
                  {act("Вы переключили внимание на $N3.",FALSE,k,0,j,TO_CHAR);
                   act("$n переключил$u на Вас !",FALSE,k,0,j,TO_VICT);
                   FIGHTING(k) = j;
                   break;
                  }
           if (!j && need_stop)
              stop_fighting(k,FALSE);
          }
      }
}

/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char(struct char_data * ch, int clear_objs)
{ char   name[MAX_STRING_LENGTH];
  struct descriptor_data *t_desc;
  struct obj_data *obj;
  int    i, freed = 0;


  if (MOB_FLAGGED(ch,MOB_FREE) || MOB_FLAGGED(ch,MOB_DELETE))
     return;

  strcpy(name,GET_NAME(ch));

  log("[Extract char] Start function for char %s",name);
  if (!IS_NPC(ch) && !ch->desc)
     {log("[Extract char] Extract descriptors");
      for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next)
          if (t_desc->original == ch)
	         do_return(t_desc->character, NULL, 0, 0);
     }

  if (ch->in_room == NOWHERE)
     {log("SYSERR: NOWHERE extracting char %s. (%s, extract_char)",GET_NAME(ch), __FILE__);
      return;
      // exit(1);
     }

  log("[Extract char] Die followers");
  if (ch->followers || ch->master)
     die_follower(ch);

  /* Forget snooping, if applicable */
  log("[Extract char] Stop snooping");
  if (ch->desc)
     {if (ch->desc->snooping)
         {ch->desc->snooping->snoop_by = NULL;
          ch->desc->snooping = NULL;
         }
      if (ch->desc->snoop_by)
         {SEND_TO_Q("Ваша жертва теперь недоступна.\r\n",
		            ch->desc->snoop_by);
          ch->desc->snoop_by->snooping = NULL;
          ch->desc->snoop_by = NULL;
         }
     }

  /* transfer objects to room, if any */
  log("[Extract char] Drop objects");
  while (ch->carrying)
        {obj = ch->carrying;
         obj_from_char(obj);
         act("Вы выбросили $o3 на землю.", FALSE, ch, obj, 0, TO_CHAR);
         act("$n бросил$g $o3 на землю.", FALSE, ch, obj, 0, TO_ROOM);
         obj_to_room(obj, ch->in_room);
        }

  /* transfer equipment to room, if any */
  log("[Extract char] Drop equipment");
  for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i))
         {act("Вы сняли $o3 и выбросили на землю.", FALSE, ch, GET_EQ(ch,i), 0, TO_CHAR);
          act("$n снял$g $o3 и бросил$g на землю.", FALSE, ch, GET_EQ(ch,i), 0, TO_ROOM);
          obj_to_room(unequip_char(ch, i), ch->in_room);
         }

  log("[Extract char] Stop fighting self");
  if (FIGHTING(ch))
     stop_fighting(ch,TRUE);

  log("[Extract char] Stop all fight for opponee");
  change_fighting(ch,TRUE);


  log("[Extract char] Remove char from room");
  char_from_room(ch);

  /* pull the char from the list */
  SET_BIT(MOB_FLAGS(ch, MOB_DELETE), MOB_DELETE);
  //REMOVE_FROM_LIST(ch, character_list, next);


  if (ch->desc && ch->desc->original)
     do_return(ch, NULL, 0, 0);

  if (!IS_NPC(ch))
     {log("[Extract char] All save for PC");
      check_auction(ch,NULL);
      save_char(ch,NOWHERE);
      Crash_delete_crashfile(ch);
      if (clear_objs)
         Crash_delete_file(GET_NAME(ch), CRASH_DELETE_OLD | CRASH_DELETE_NEW);
     }
  else
     {log("[Extract char] All clear for NPC");
      if (GET_MOB_RNUM(ch) > -1)		/* if mobile */
         mob_index[GET_MOB_RNUM(ch)].number--;
      clearMemory(ch);		/* Only NPC's can have memory */
      if (SCRIPT(ch))
         extract_script(SCRIPT(ch));
      if (SCRIPT_MEM(ch))
         extract_script_mem(SCRIPT_MEM(ch));

      SET_BIT(MOB_FLAGS(ch,MOB_FREE), MOB_FREE);
      //free_char(ch);
      freed = 1;
     }

  if (!freed && ch->desc != NULL)
     { STATE(ch->desc) = CON_MENU;
       SEND_TO_Q(MENU, ch->desc);
     }
  else
     {/* if a player gets purged from within the game */
      if (!freed)
         {SET_BIT(MOB_FLAGS(ch,MOB_FREE), MOB_FREE);
          //free_char(ch);
         }
     }
  log("[Extract char] Stop function for char %s",name);
}


/* Extract a MOB completely from the world, and destroy his stuff */
void extract_mob(struct char_data * ch)
{ int    i;

  if (MOB_FLAGGED(ch,MOB_FREE) || MOB_FLAGGED(ch,MOB_DELETE))
     return;

  if (ch->in_room == NOWHERE)
     {log("SYSERR: NOWHERE extracting char %s. (%s, extract_mob)",GET_NAME(ch), __FILE__);
      return;
      exit(1);
     }
  if (ch->followers || ch->master)
     die_follower(ch);

  /* Forget snooping, if applicable */
  if (ch->desc)
     {if (ch->desc->snooping)
         {ch->desc->snooping->snoop_by = NULL;
          ch->desc->snooping = NULL;
         }
      if (ch->desc->snoop_by)
         {SEND_TO_Q("Ваша жертва теперь недоступна.\r\n",
		            ch->desc->snoop_by);
          ch->desc->snoop_by->snooping = NULL;
          ch->desc->snoop_by = NULL;
         }
     }

  /* extract objects, if any */
  while (ch->carrying)
        extract_obj(ch->carrying);

  /* transfer equipment to room, if any */
  for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i))
         extract_obj(GET_EQ(ch,i));

  if (FIGHTING(ch))
     stop_fighting(ch,TRUE);

  log("[Extract mob] Stop all fight for opponee");
  change_fighting(ch,TRUE);

  char_from_room(ch);

  /* pull the char from the list */
  SET_BIT(MOB_FLAGS(ch, MOB_DELETE), MOB_DELETE);
  // REMOVE_FROM_LIST(ch, character_list, next);


  if (ch->desc && ch->desc->original)
     do_return(ch, NULL, 0, 0);

  if (GET_MOB_RNUM(ch) > -1)
     mob_index[GET_MOB_RNUM(ch)].number--;
  clearMemory(ch);
  if (SCRIPT(ch))
     extract_script(SCRIPT(ch));
  if (SCRIPT_MEM(ch))
     extract_script_mem(SCRIPT_MEM(ch));

  SET_BIT(MOB_FLAGS(ch, MOB_FREE), MOB_FREE);
  // free_char(ch);
}




/* ***********************************************************************
* Here follows high-level versions of some earlier routines, ie functions*
* which incorporate the actual player-data                               *.
*********************************************************************** */


struct char_data *get_player_vis(struct char_data * ch, char *name, int inroom)
{
  struct char_data *i;

  for (i = character_list; i; i = i->next)
      {if (IS_NPC(i) || !i->desc)
          continue;
       if (inroom == FIND_CHAR_ROOM &&
           i->in_room != ch->in_room
	  )
          continue;
       //if (str_cmp(i->player.name, name))
       //   continue;
       if (!CAN_SEE(ch, i))
          continue;
       if (!isname(name, i->player.name))
          continue;
       return (i);
      }

  return (NULL);
}


struct char_data *get_char_room_vis(struct char_data * ch, char *name)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* JE 7/18/94 :-) :-) */
  if (!str_cmp(name, "self") || !str_cmp(name, "me")  ||
      !str_cmp(name, "я")    || !str_cmp(name, "меня")||
      !str_cmp(name, "себя"))
     return (ch);

  /* 0.<name> means PC with name */
  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
     return (get_player_vis(ch, tmp, FIND_CHAR_ROOM));

  for (i = world[ch->in_room].people; i && j <= number; i = i->next_in_room)
      if ((IS_NPC(i) || i->desc) && CAN_SEE(ch, i) && isname(tmp, i->player.name))
	 if (++j == number)
	    return (i);

  return (NULL);
}


struct char_data *get_char_vis(struct char_data * ch, char *name, int where)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* check the room first */
  if (where == FIND_CHAR_ROOM)
     return get_char_room_vis(ch, name);
  else
  if (where == FIND_CHAR_WORLD)
     {if ((i = get_char_room_vis(ch, name)) != NULL)
         return (i);

      strcpy(tmp, name);
      if (!(number = get_number(&tmp)))
         return get_player_vis(ch, tmp, 0);

      for (i = character_list; i && (j <= number); i = i->next)
          if ((IS_NPC(i) || i->desc) && CAN_SEE(ch, i) && isname(tmp, i->player.name))
             if (++j == number)
                return (i);
     }

  return (NULL);
}



struct obj_data *get_obj_in_list_vis(struct char_data * ch, char *name,
				                     struct obj_data * list)
{
  struct obj_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
     return (NULL);

  for (i = list; i && (j <= number); i = i->next_content)
      if (isname(tmp, i->name))
         if (CAN_SEE_OBJ(ch, i))
	    if (++j == number)
	       {//* sprintf(buf,"Show obj %d %s %x ", number, i->name, i);
	        //* send_to_char(buf,ch);
                return (i);
	       }

  return (NULL);
}




/* search the entire world for an object, and return a pointer  */
struct obj_data *get_obj_vis(struct char_data * ch, char *name)
{
  struct obj_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* scan items carried */
  if ((i = get_obj_in_list_vis(ch, name, ch->carrying)) != NULL)
     return (i);

  /* scan room */
  if ((i = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)) != NULL)
     return (i);

  strcpy(tmp, name);
  if ((number = get_number(&tmp)) == 0)
     return (NULL);

  /* ok.. no luck yet. scan the entire obj list   */
  for (i = object_list; i && (j <= number); i = i->next)
      if (isname(tmp, i->name))
         if (CAN_SEE_OBJ(ch, i))
	        if (++j == number)
	           return (i);

  return (NULL);
}



struct obj_data *get_object_in_equip_vis(struct char_data * ch,
		           char *arg, struct obj_data * equipment[], int *j)
{ int  l, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, arg);
  if ((number = get_number(&tmp)) == 0)
     return (NULL);

  for ((*j) = 0, l = 0; (*j) < NUM_WEARS; (*j)++)
      if (equipment[(*j)])
         if (CAN_SEE_OBJ(ch, equipment[(*j)]))
	        if (isname(arg, equipment[(*j)]->name))
	           if (++l == number)
                  return (equipment[(*j)]);

  return (NULL);
}


struct obj_data *get_obj_in_eq_vis(struct char_data * ch,
 		                      char *arg)
{ int  l, number, j;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, arg);
  if ((number = get_number(&tmp)) == 0)
     return (NULL);

  for (j = 0, l = 0; j < NUM_WEARS; j++)
      if (GET_EQ(ch,j))
         if (CAN_SEE_OBJ(ch, GET_EQ(ch,j)))
	        if (isname(arg, GET_EQ(ch,j)->name))
	           if (++l == number)
                      return (GET_EQ(ch,j));

  return (NULL);
}


char *money_desc(int amount, int padis)
{
  static char buf[128];
  const  char *single[6][2] =
         { {"а",  "а"},
           {"ой", "ы"},
           {"ой", "е"},
           {"у",  "у"},
           {"ой", "ой"},
           {"ой", "е"} },
              *plural[6][3] =
          { {"ая", "а", "а"},
            {"ой", "и", "ы"},
            {"ой", "е", "е"},
            {"ую", "у", "у"},
            {"ой", "ой","ой"},
            {"ой", "е", "е"}};

  if (amount <= 0)
     {log("SYSERR: Try to create negative or 0 money (%d).", amount);
      return (NULL);
     }
  if (amount == 1)
     { sprintf(buf, "одн%s кун%s",
              single[padis][0], single[padis][1]);
     }
  else
  if (amount <= 10)
     sprintf(buf, "малюсеньк%s горстк%s кун",
             plural[padis][0], plural[padis][1]);
  else
  if (amount <= 20)
     sprintf(buf, "маленьк%s горстк%s кун",
            plural[padis][0], plural[padis][1]);
  else
  if (amount <= 75)
     sprintf(buf, "небольш%s горстк%s кун",
            plural[padis][0], plural[padis][1]);
  else
  if (amount <= 200)
     sprintf(buf, "маленьк%s кучк%s кун",
            plural[padis][0], plural[padis][1]);
  else
  if (amount <= 1000)
     sprintf(buf, "небольш%s кучк%s кун",
            plural[padis][0], plural[padis][1]);
  else
  if (amount <= 5000)
     sprintf(buf, "кучк%s кун",
            plural[padis][1]);
  else
  if (amount <= 10000)
     sprintf(buf, "больш%s кучк%s кун",
            plural[padis][0], plural[padis][1]);
  else
  if (amount <= 20000)
     sprintf(buf, "груд%s кун",
            plural[padis][2]);
  else
  if (amount <= 75000)
     sprintf(buf, "больш%s груд%s кун",
            plural[padis][0], plural[padis][2]);
  else
  if (amount <= 150000)
     sprintf(buf, "горк%s кун",
            plural[padis][1]);
  else
  if (amount <= 250000)
     sprintf(buf, "гор%s кун",
            plural[padis][2]);
  else
    sprintf(buf, "огромн%s гор%s кун",
           plural[padis][0], plural[padis][2]);

  return (buf);
}


struct obj_data *create_money(int amount)
{ int    i;
  struct obj_data *obj;
  struct extra_descr_data *new_descr;
  char   buf[200];

  if (amount <= 0)
     {log("SYSERR: Try to create negative or 0 money. (%d)", amount);
      return (NULL);
     }
  obj = create_obj();
  CREATE(new_descr, struct extra_descr_data, 1);

  if (amount == 1)
     {sprintf(buf,"coin gold кун деньги денег монет %s",money_desc(amount,0));
      obj->name               = str_dup(buf);
      obj->short_description = str_dup("куна");
      obj->description       = str_dup("Одна куна лежит здесь.");
      new_descr->keyword     = str_dup("coin gold монет кун денег");
      new_descr->description = str_dup("Всего лишь одна куна.");
      for(i = 0; i < NUM_PADS; i++)
         obj->PNames[i] = str_dup(money_desc(amount,i));
     }
  else
     {sprintf(buf,"coins gold кун денег %s",money_desc(amount,0));
      obj->name                    = str_dup(buf);
      obj->short_description      = str_dup(money_desc(amount,0));
      for(i = 0; i < NUM_PADS; i++)
         obj->PNames[i] = str_dup(money_desc(amount,i));

      sprintf(buf, "Здесь лежит %s.", money_desc(amount, 0));
      obj->description = str_dup(CAP(buf));

      new_descr->keyword = str_dup("coins gold кун денег");
     }

  new_descr->next     = NULL;
  obj->ex_description = new_descr;

  GET_OBJ_TYPE(obj)                  = ITEM_MONEY;
  GET_OBJ_WEAR(obj)                  = ITEM_WEAR_TAKE;
  GET_OBJ_SEX(obj)                   = SEX_FEMALE;
  GET_OBJ_VAL(obj, 0)                = amount;
  GET_OBJ_COST(obj)                  = amount;
  GET_OBJ_MAX(obj)                   = 100;
  GET_OBJ_CUR(obj)                   = 100;
  GET_OBJ_TIMER(obj)                 = 24*60*7;
  GET_OBJ_WEIGHT(obj)                = 1;
  GET_OBJ_EXTRA(obj,ITEM_NODONATE)  |= ITEM_NODONATE;
  GET_OBJ_EXTRA(obj,ITEM_NOSELL)    |= ITEM_NOSELL;

  obj->item_number    = NOTHING;
  
  return (obj);
}


/* Generic Find, designed to find any object/character
 *
 * Calling:
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be NULL if no character was found, otherwise points
 * **tar_obj Will be NULL if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in.
 */
int generic_find(char *arg, bitvector_t bitvector, struct char_data * ch,
		     struct char_data ** tar_ch, struct obj_data ** tar_obj)
{
  char name[256];

  *tar_ch = NULL;
  *tar_obj = NULL;

  one_argument(arg, name);

  if (!*name)
     return (0);

  if (IS_SET(bitvector, FIND_CHAR_ROOM))
     {/* Find person in room */
      if ((*tar_ch = get_char_vis(ch, name, FIND_CHAR_ROOM)) != NULL)
         return (FIND_CHAR_ROOM);
     }
  if (IS_SET(bitvector, FIND_CHAR_WORLD))
     {if ((*tar_ch = get_char_vis(ch, name, FIND_CHAR_WORLD)) != NULL)
         return (FIND_CHAR_WORLD);
     }
  if (IS_SET(bitvector, FIND_OBJ_EQUIP))
     {if ((*tar_obj = get_obj_in_eq_vis(ch, name)) != NULL)
         return (FIND_OBJ_EQUIP);
     }
  if (IS_SET(bitvector, FIND_OBJ_INV))
     {if ((*tar_obj = get_obj_in_list_vis(ch, name, ch->carrying)) != NULL)
         return (FIND_OBJ_INV);
     }
  if (IS_SET(bitvector, FIND_OBJ_ROOM))
     {if ((*tar_obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)) != NULL)
         return (FIND_OBJ_ROOM);
     }
  if (IS_SET(bitvector, FIND_OBJ_WORLD))
     {if ((*tar_obj = get_obj_vis(ch, name)))
         return (FIND_OBJ_WORLD);
     }
  return (0);
}


/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg)
{
  if (!str_cmp(arg, "all") || !str_cmp(arg, "все"))
     return (FIND_ALL);
  else
  if (!strn_cmp(arg, "all.", 4) || !strn_cmp(arg, "все.", 4))
     {strcpy(arg, arg + 4);
      return (FIND_ALLDOT);
     }
  else
     return (FIND_INDIV);
}