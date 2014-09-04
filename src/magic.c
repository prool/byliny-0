/* ************************************************************************
*   File: magic.c                                       Part of CircleMUD *
*  Usage: low-level functions for magic; spell template code              *
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
#include "interpreter.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"

extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct index_data *obj_index;
extern struct obj_data *obj_proto;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;
extern struct spell_create_type spell_create[];
extern int mini_mud;
extern const char *spell_wear_off_msg[];
extern struct char_data *mob_proto;
extern int supress_godsapply;

byte saving_throws(int class_num, int type, int level); /* class.c */
byte extend_saving_throws(int class_num, int type, int level);
void clearMemory(struct char_data * ch);
void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
void go_flee(struct char_data * ch);
int  attack_best(struct char_data *ch, struct char_data *victim);
int  may_pkill(struct char_data *revenge, struct char_data *killer);
int  inc_pkill(struct char_data *victim, struct char_data *killer, int pkills, int prevenge);
int  inc_pkill_group(struct char_data *victim, struct char_data *killer, int pkills, int prevenge);
int  same_group(struct char_data *ch, struct char_data *och);
void alterate_object(struct obj_data *obj, int dam, int chance);
void death_cry(struct char_data *ch);
int  has_boat(struct char_data *ch);
int  check_death_trap(struct char_data *ch);
int  check_death_ice(int room, struct char_data *ch);
int  check_charmee(struct char_data *ch, struct char_data *victim);
int  slot_for_char(struct char_data *ch, int slotnum);
void cast_reaction(struct char_data *victim, struct char_data *caster, int spellnum);

/* local functions */
int mag_materials(struct char_data * ch, int item0, int item1, int item2, int extract, int verbose);
void perform_mag_groups(int level, struct char_data * ch, struct char_data * tch, int spellnum, int savetype);
int  mag_savingthrow(struct char_data * ch, int type, int modifier);
void affect_update(void);
void battle_affect_update(struct char_data * ch);

/*
 * Saving throws are now in class.c as of bpl13.
 */


/*
 * Negative apply_saving_throw[] values make saving throws better!
 * Then, so do negative modifiers.  Though people may be used to
 * the reverse of that. It's due to the code modifying the target
 * saving throw instead of the random number of the character as
 * in some other systems.
 */
int mag_savingthrow(struct char_data * ch, int type, int modifier)
{
  /* NPCs use warrior tables according to some book */
  int class_sav = CLASS_WARRIOR;
  int save;

  if (!IS_NPC(ch))
      class_sav = GET_CLASS(ch);

  save = saving_throws(class_sav, type, GET_LEVEL(ch));
  save += GET_SAVE(ch, type);
  save += modifier;

  /* Throwing a 0 is always a failure. */
  if (MAX(1, save) < number(0, 99) || IS_GOD(ch))
     return (TRUE);

  /* Oops, failed. Sorry. */
  return (FALSE);
}


int calc_anti_savings(struct char_data *ch)
{ int modi = 0;

  if (WAITLESS(ch))
     modi = 150;
  else
  if (GET_GOD_FLAG(ch, GF_GODSLIKE))
     modi = 50;
  else
  if (GET_GOD_FLAG(ch, GF_GODSCURSE))
     modi = -50;
  else
     modi = GET_CAST_SUCCESS(ch);
  return modi;
}


int general_savingthrow(struct char_data * ch, int type, int ext_apply)
{
  /* NPCs use warrior tables according to some book */
  int save;
  int class_sav = GET_CLASS(ch);

  if (IS_NPC(ch))
     {if (class_sav < CLASS_BASIC_NPC || class_sav >= CLASS_LAST_NPC)
         class_sav = CLASS_WARRIOR;
     }
  else
  if (class_sav < 0 || class_sav >= NUM_CLASSES)
     class_sav = CLASS_WARRIOR;

  save  = extend_saving_throws(class_sav, type, GET_LEVEL(ch));

  switch(type)
  {case SAVING_BASIC:
        if (PRF_FLAGGED(ch,PRF_AWAKE))
           save -= ((GET_SKILL(ch,SKILL_AWAKE) + 10) / 3);
        break;
   case SAVING_SPELL:
   case SAVING_PARA:
   case SAVING_ROD:
   case SAVING_BREATH:
   case SAVING_PETRI:
        if ((AFF_FLAGGED(ch,AFF_AIRAURA)    ||
 	     AFF_FLAGGED(ch,AFF_FIREAURA)   ||
	     AFF_FLAGGED(ch,AFF_ICEAURA))  &&
	    save > 0
	   )
           save >>= 1;
        if (PRF_FLAGGED(ch,PRF_AWAKE))
           save  -= ((GET_SKILL(ch,SKILL_AWAKE) + 10) / 3);
   break;
  }
  save += GET_SAVE(ch, type);
  save += ext_apply;

  if (IS_GOD(ch))
     save = -150;
  else
  if (GET_GOD_FLAG(ch, GF_GODSLIKE))
     save -= 50;
  else
  if (GET_GOD_FLAG(ch, GF_GODSCURSE))
     save += 50;
  if (!IS_NPC(ch))
     log("[SAVING] Name==%s type==%d save==%d",GET_NAME(ch),type,save);
  /* Throwing a 0 is always a failure. */
  if (MAX(0, save) <= number(1, 100))
     return (TRUE);

  /* Oops, failed. Sorry. */
  return (FALSE);
}




/* affect_update: called from comm.c (causes spells to wear off) */

void mobile_affect_update(void)
{
  struct affected_type *af, *next;
  struct timed_type    *timed, *timed_next;
  struct char_data *i, *i_next;
  int    was_charmed = 0, count, charmed_msg = FALSE;

  for (i = character_list; i; i = i_next)
      {i_next            = i->next;
       charmed_msg       = FALSE;
       was_charmed       = FALSE;
       supress_godsapply = TRUE;
       for (af = i->affected; IS_NPC(i) && af; af = next)
           {next = af->next;
            if (af->duration >= 1)
               {af->duration--;
                if (af->type == SPELL_CHARM && !charmed_msg && af->duration <= 1)
                   {act("$n начал$g растерянно оглядываться по сторонам.",FALSE,i,0,0,TO_ROOM);
                    charmed_msg = TRUE;
                   }
               }
            else
            if (af->duration == -1)	
                af->duration = -1;  // GODS - unlimited
            else
               {if ((af->type > 0) && (af->type <= MAX_SPELLS))
	           {if (!af->next || (af->next->type != af->type) || (af->next->duration > 0))
  	               {if (af->type > 0 &&
	                    af->type <= LAST_USED_SPELL &&
			    *spell_wear_off_msg[af->type]
			   )
	                   {send_to_char(spell_wear_off_msg[af->type], i);
	                    send_to_char("\r\n", i);
	                    if (af->type == SPELL_CHARM || af->bitvector == AFF_CHARM)
	                       was_charmed = TRUE;
	                   }
	               }
	           }
	        affect_remove(i, af);
               }
           }
       supress_godsapply = FALSE;
       //log("[MOBILE_AFFECT_UPDATE->AFFECT_TOTAL] (%s) Start",GET_NAME(i));
       affect_total(i);
       //log("[MOBILE_AFFECT_UPDATE->AFFECT_TOTAL] Stop");
       for (timed = i->timed; timed; timed = timed_next)
           {timed_next = timed->next;
            if (timed->time >= 1)
	       timed->time--;
            else
               timed_from_char(i, timed);
           }

       if (check_death_trap(i))
          continue;
       if (was_charmed)
          {stop_follower(i, SF_CHARMLOST);
          }
      }

  for (count = 0; count <= top_of_world; count++)
      check_death_ice(count,NULL);
}


void player_affect_update(void)
{
  struct affected_type *af, *next;
  struct char_data *i, *i_next;
  int    slots[MAX_SLOT], cnt, dec, sloti, slotn;

  for (i = character_list; i; i = i_next)
      {i_next = i->next;
       if (IS_NPC(i))
          continue;
       supress_godsapply = TRUE;
       for (af = i->affected; af; af = next)
           {next = af->next;
            if (af->duration >= 1)
	           af->duration--;
            else
            if (af->duration == -1)	
                af->duration = -1;
            else
               {if ((af->type > 0) && (af->type <= MAX_SPELLS))
                   {if (!af->next || (af->next->type != af->type) || (af->next->duration > 0))  	
		           {if (af->type > 0 &&
			        af->type <= LAST_USED_SPELL &&
			        *spell_wear_off_msg[af->type]
			       )
	                       {send_to_char(spell_wear_off_msg[af->type], i);
	                        send_to_char("\r\n", i);
	                       }
	                   }
	               }
	            affect_remove(i, af);
               }
           }
       for (cnt = 0; cnt < MAX_SLOT; slots[cnt++] = 0);

       for (cnt = 1; cnt <= LAST_USED_SPELL; cnt++)
           {
            if (IS_SET(GET_SPELL_TYPE(i,cnt), SPELL_TEMP) ||
                !GET_SPELL_MEM(i,cnt)                     ||
                WAITLESS(i)
               )
               continue;
            if (spell_info[cnt].min_level[(int) GET_CLASS(i)] > GET_LEVEL(i))
               GET_SPELL_MEM(i,cnt) = 0;
            else
               { sloti = spell_info[cnt].slot_forc[(int) GET_CLASS(i)];
                 slotn = slot_for_char(i,sloti);
                 slots[sloti] += GET_SPELL_MEM(i,cnt);
                 if (slotn < slots[sloti])
                    {dec = MIN(GET_SPELL_MEM(i,cnt), slots[sloti] - slotn);
                     GET_SPELL_MEM(i,cnt) -= dec;
                     slots[sloti]         -= dec;
                    }
               }
           }
       supress_godsapply = FALSE;
       //log("[PLAYER_AFFECT_UPDATE->AFFECT_TOTAL] Start");
       affect_total(i);
       //log("[PLAYER_AFFECT_UPDATE->AFFECT_TOTAL] Stop");
      }
}


/* This file update battle affects only */
void battle_affect_update(struct char_data *ch)
{
  struct affected_type *af, *next;

  supress_godsapply = TRUE;
  for (af = ch->affected; af; af = next)
      {next         = af->next;
       if (!IS_SET(af->battleflag,AF_BATTLEDEC))
          continue;
       if (af->duration >= 1)
          {if (IS_NPC(ch))
              af->duration--;
           else
              af->duration -= MIN(af->duration, SECS_PER_MUD_HOUR / SECS_PER_PLAYER_AFFECT);
          }
       else
       if (af->duration == -1)	/* No action */
          af->duration = -1;	    /* GODs only! unlimited */
       else
         {if ((af->type > 0) && (af->type <= MAX_SPELLS))
             {if (!af->next || (af->next->type != af->type) || (af->next->duration > 0))
                 {if (af->type > 0 &&
                      af->type <= LAST_USED_SPELL &&
                      *spell_wear_off_msg[af->type])
                     {send_to_char(spell_wear_off_msg[af->type], ch);
	              send_to_char("\r\n", ch);
	             }
	         }
	     }
	  affect_remove(ch, af);
         }
      }
  supress_godsapply = FALSE;
  //log("[BATTLE_AFFECT_UPDATE->AFFECT_TOTAL] Start");
  affect_total(ch);
  //log("[BATTLE_AFFECT_UPDATE->AFFECT_TOTAL] Stop");
}



/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle 3.0 use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */

int mag_item_ok(struct char_data *ch, struct obj_data *obj, int spelltype)
{  int num = 0;

   if ((!IS_SET(GET_OBJ_SKILL(obj), ITEM_RUNES) && spelltype == SPELL_RUNES) ||
      (IS_SET(GET_OBJ_SKILL(obj), ITEM_RUNES) && spelltype != SPELL_RUNES))
      return (FALSE);

   if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_USES) &&
       GET_OBJ_VAL(obj,2) <= 0)
      return (FALSE);

   if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_LAG))
      { num = 0;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG1s))
           num += 1;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG2s))
           num += 2;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG4s))
           num += 4;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG8s))
           num += 8;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG16s))
           num += 16;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG32s))
           num += 32;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG64s))
           num += 64;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG128s))
           num += 128;
        if (GET_OBJ_VAL(obj,3) + num - 5 * GET_REMORT(ch) >= time(NULL))
           return (FALSE);
      }

   if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_LEVEL))
      { num = 0;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LEVEL1))
           num += 1;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LEVEL2))
           num += 2;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LEVEL4))
           num += 4;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LEVEL8))
           num += 8;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LEVEL16))
           num += 16;
        if (GET_LEVEL(ch) + (GET_REMORT(ch) + 1) / 2 < num)
           return (FALSE);
      }

 return (TRUE);
}


void extract_item(struct char_data *ch, struct obj_data *obj, int spelltype)
{  int extract = FALSE;
   if (!obj)
      return;

   GET_OBJ_VAL(obj,3) = time(NULL);

   if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_USES))
      {GET_OBJ_VAL(obj,2)--;
       if (GET_OBJ_VAL(obj,2) <= 0 && IS_SET(GET_OBJ_SKILL(obj), ITEM_DECAY_EMPTY))
          extract = TRUE;
      }
   else
   if (spelltype != SPELL_RUNES)
      extract = TRUE;

   if (extract)
      {if (spelltype == SPELL_RUNES)
          act("$o рассыпал$U у Вас в руках.",FALSE,ch,obj,0,TO_CHAR);
       obj_from_char(obj);
       extract_obj(obj);
      }
}



int check_recipe_items(struct char_data * ch, int spellnum, int spelltype, int extract)
{
  struct obj_data *obj;
  struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL, *obj3 = NULL, *objo=NULL;
  int    item0=-1, item1=-1, item2=-1, item3=-1;
  int    create=0, obj_num=-1, skillnum=-1, percent=0, num=0;
  struct spell_create_item *items;

  if (spellnum <= 0 || spellnum > MAX_SPELLS)
     return (FALSE);
  if (spelltype == SPELL_ITEMS)
     {items = &spell_create[spellnum].items;
     }
  else
  if (spelltype == SPELL_POTION)
     {items    = &spell_create[spellnum].potion;
      skillnum = SKILL_CREATE_POTION;
      create   = 1;
     }
  else
  if (spelltype == SPELL_WAND)
     {items    = &spell_create[spellnum].wand;
      skillnum = SKILL_CREATE_WAND;
      create   = 1;
     }
  else
  if (spelltype == SPELL_SCROLL)
     {items    = &spell_create[spellnum].scroll;
      skillnum = SKILL_CREATE_SCROLL;
      create   = 1;
     }
  else
  if (spelltype == SPELL_RUNES)
     {items    = &spell_create[spellnum].runes;
     }
  else
     return (FALSE);

  if (((spelltype == SPELL_RUNES || spelltype == SPELL_ITEMS) &&
       (item3=items->rnumber)  +
       (item0=items->items[0]) +
       (item1=items->items[1]) +
       (item2=items->items[2]) < -3) ||
      ((spelltype == SPELL_SCROLL || spelltype == SPELL_WAND || spelltype == SPELL_POTION) &&
        ((obj_num = items->rnumber) < 0 ||
         (item0 = items->items[0]) +
         (item1 = items->items[1]) +
         (item2 = items->items[2]) < -2))
     )
     return (FALSE);

  for (obj = ch->carrying; obj; obj = obj->next_content)
      {if (item0 >= 0 && (percent = real_object(item0)) >= 0 &&
           GET_OBJ_VAL(obj,1) == GET_OBJ_VAL(obj_proto+percent,1) &&
           mag_item_ok(ch,obj,spelltype))
          {obj0  = obj;
           item0 = -2;
           objo  = obj0;
           num++;
          }
       else
       if (item1 >= 0 && (percent = real_object(item1)) >= 0 &&
           GET_OBJ_VAL(obj,1) == GET_OBJ_VAL(obj_proto+percent,1) &&
           mag_item_ok(ch,obj,spelltype))
          {obj1  = obj;
           item1 = -2;
           objo  = obj1;
           num++;
          }
       else
       if (item2 >= 0 && (percent = real_object(item2)) >= 0 &&
           GET_OBJ_VAL(obj,1) == GET_OBJ_VAL(obj_proto+percent,1) &&
           mag_item_ok(ch,obj,spelltype))
          {obj2  = obj;
           item2 = -2;
           objo  = obj2;
           num++;
          }
       else
       if (item3 >= 0 && (percent = real_object(item3)) >= 0 &&
           GET_OBJ_VAL(obj,1) == GET_OBJ_VAL(obj_proto+percent,1) &&
           mag_item_ok(ch,obj,spelltype))
          {obj3  = obj;
           item3 = -2;
           objo  = obj3;
           num++;
          }
      };

  log("%d %d %d %d",items->items[0],items->items[1],items->items[2],items->rnumber);
  log("%d %d %d %d",item0,item1,item2,item3);
  if (!objo ||
      (items->items[0] >= 0 && item0 >= 0) ||
      (items->items[1] >= 0 && item1 >= 0) ||
      (items->items[2] >= 0 && item2 >= 0) ||
      (items->rnumber  >= 0 && item3 >= 0))
     return (FALSE);
  log("OK!");
  if (extract)
     {if (spelltype == SPELL_RUNES)
         strcpy(buf,"Вы сложили ");
      else
         strcpy(buf,"Вы взяли ");
      if (create)
         {if (!(obj = read_object(obj_num, VIRTUAL)))
             return (FALSE);
          else
             {percent = number(1,100);
              if (skillnum > 0 && percent > train_skill(ch,skillnum,percent,0))
                 percent = -1;
             }
         }
      if (item0 == -2)
         {strcat(buf,CCWHT(ch, C_NRM));
          strcat(buf,obj0->PNames[3]);
          strcat(buf,", ");
         }
      if (item1 == -2)
         {strcat(buf, CCWHT(ch, C_NRM));
          strcat(buf,obj1->PNames[3]);
          strcat(buf,", ");
         }
      if (item2 == -2)
         {strcat(buf, CCWHT(ch, C_NRM));
          strcat(buf,obj2->PNames[3]);
          strcat(buf,", ");
         }
      if (item3 == -2)
         {strcat(buf, CCWHT(ch, C_NRM));
          strcat(buf,obj3->PNames[3]);
          strcat(buf,", ");
         }
      strcat(buf, CCNRM(ch, C_NRM));
      if (create)
         {if (percent >= 0)
             {strcat(buf," и создали $o3.");
              act(buf,FALSE,ch,obj,0,TO_CHAR);
              act("$n создал$g $o3.",FALSE,ch,obj,0,TO_ROOM);
              obj_to_char(obj,ch);
             }
          else
             {strcat(buf," и попытались создать $o3.\r\n"
                         "Ничего не вышло.");
              act(buf,FALSE,ch,obj,0,TO_CHAR);
              extract_obj(obj);
             }
         }
      else
         {if (spelltype == SPELL_ITEMS)
             {strcat(buf, "и создали магическую смесь.\r\n");
              act(buf,FALSE,ch,0,0,TO_CHAR);
              act("$n смешал$g что-то в своей ноше.\r\n"
                  "Вы почувствовали резкий запах.", TRUE, ch, NULL, NULL, TO_ROOM);
             }
          else
          if (spelltype == SPELL_RUNES)
             {sprintf(buf+strlen(buf), "котор%s вспыхнул%s ярким светом.\r\n",
                      num > 1 ? "ые" : GET_OBJ_SUF_3(objo),
                      num > 1 ? "и"  : GET_OBJ_SUF_1(objo));
              act(buf,FALSE,ch,0,0,TO_CHAR);
              act("$n сложил$g руны, которые вспыхнули ярким пламенем.",
                  TRUE, ch, NULL, NULL, TO_ROOM);
             }
         }
       extract_item(ch,obj0,spelltype);
       extract_item(ch,obj1,spelltype);
       extract_item(ch,obj2,spelltype);
       extract_item(ch,obj3,spelltype);
     }
  return (TRUE);
}


int check_recipe_values(struct char_data * ch, int spellnum, int spelltype, int showrecipe)
{
  int    item0=-1, item1=-1, item2=-1, obj_num=-1;
  struct spell_create_item *items;

  if (spellnum <= 0 || spellnum > MAX_SPELLS)
     return (FALSE);
  if (spelltype == SPELL_ITEMS)
     {items    = &spell_create[spellnum].items;
     }
  else
  if (spelltype == SPELL_POTION)
     {items = &spell_create[spellnum].potion;
     }
  else
  if (spelltype == SPELL_WAND)
     {items = &spell_create[spellnum].wand;
     }
  else
  if (spelltype == SPELL_SCROLL)
     {items = &spell_create[spellnum].scroll;
     }
  else
  if (spelltype == SPELL_RUNES)
     {items = &spell_create[spellnum].runes;
     }
  else
     return (FALSE);

  if (((obj_num = real_object(items->rnumber)) < 0 &&
       spelltype != SPELL_ITEMS && spelltype != SPELL_RUNES)  ||
      ((item0 = real_object(items->items[0])) +
       (item1 = real_object(items->items[1])) +
       (item2 = real_object(items->items[2])) < -2)
     )
     {if (showrecipe)
         send_to_char("Боги хранят в секрете этот рецепт.\n\r",ch);
      return (FALSE);
     }

  if (!showrecipe)
     return (TRUE);
  else
     {strcpy(buf,"Вам потребуется :\r\n");
      if (item0 >= 0)
         {strcat(buf, CCIRED(ch, C_NRM));
          strcat(buf,obj_proto[item0].PNames[0]);
          strcat(buf,"\r\n");
         }
      if (item1 >= 0)
         {strcat(buf, CCIYEL(ch, C_NRM));
          strcat(buf,obj_proto[item1].PNames[0]);
          strcat(buf,"\r\n");
         }
      if (item2 >= 0)
         {strcat(buf, CCIGRN(ch, C_NRM));
          strcat(buf,obj_proto[item2].PNames[0]);
          strcat(buf,"\r\n");
         }
      if (obj_num >= 0 && (spelltype == SPELL_ITEMS || spelltype == SPELL_RUNES))
         {strcat(buf, CCIBLU(ch, C_NRM));
          strcat(buf,obj_proto[obj_num].PNames[0]);
          strcat(buf,"\r\n");
         }

      strcat(buf, CCNRM(ch, C_NRM));
      if (spelltype == SPELL_ITEMS || spelltype == SPELL_RUNES)
         {strcat(buf,"для создания магии '");
          strcat(buf,spell_name(spellnum));
          strcat(buf,"'.");
         }
      else
         {strcat(buf,"для создания ");
          strcat(buf, obj_proto[obj_num].PNames[1]);
         }
      act(buf,FALSE,ch,0,0,TO_CHAR);
     }

  return (TRUE);
}


void do_sacrifice(struct char_data *ch, int dam)
{GET_HIT(ch) = MIN(GET_HIT(ch)+MAX(1,dam),
                   GET_REAL_MAX_HIT(ch) + GET_REAL_MAX_HIT(ch) * GET_LEVEL(ch)/ 10);
 update_pos(ch);
}
/*
 * Every spell that does damage comes through here.  This calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 *
 * -1 = dead, otherwise the amount of damage done.
 */
#define SpINFO   spell_info[spellnum]

int mag_damage(int level, struct char_data * ch, struct char_data * victim,
  		       int spellnum, int savetype)
{
  int    dam = 0, rand = 0, count=1, modi=0, ndice = 0, sdice = 0, adice = 0,
         no_savings = FALSE;
  struct obj_data *obj=NULL;
  struct affected_type af;

  if (victim == NULL || IN_ROOM(victim) == NOWHERE ||
      ch     == NULL
     )
     return (0);


  if (ch != victim)
     {if (may_pkill(ch,victim) != PC_REVENGE_PC)
         {inc_pkill_group(victim, ch, 1, 0);
         }
      else
         {inc_pkill(victim, ch, 0, 1);
         }
     }
//  log("[MAG DAMAGE] %s damage %s (%d)",GET_NAME(ch),GET_NAME(victim),spellnum);

  // Magic glass
  if (ch != victim &&
      spellnum < MAX_SPELLS &&
      ((AFF_FLAGGED(victim,AFF_MAGICGLASS) && number(1,100) < GET_LEVEL(victim)
       ) ||
       (IS_GOD(victim) && (IS_NPC(ch) || GET_LEVEL(victim) > GET_LEVEL(ch))
       )
      )
     )
    {act("Магическое зеркало $N1 отразило Вашу магию !",FALSE,ch,0,victim,TO_CHAR);
     act("Ваше магическое зеркало отразило поражение $n1 !",FALSE,ch,0,victim,TO_VICT);
     return(mag_damage(level,ch,ch,spellnum,savetype));
    }

  if (ch != victim)
     {modi  = calc_anti_savings(ch);
      modi += wis_app[GET_REAL_WIS(victim)].char_savings;
     }

  switch (spellnum)
  {
  /******** ДЛЯ ВСЕХ МАГОВ *********/
  // магическая стрела - для всех с 1го левела 1го круга(8 слотов)
  // *** мин 15 макс 45 (360)
  // нейтрал
  case SPELL_MAGIC_MISSILE:
    ndice = 2;
    sdice = 4;
    adice = 10;
    count = (level+9) / 10;
    break;
  // ледяное прикосновение - для всех с 7го левела 3го круга(7 слотов)
  // *** мин 29.5 макс 55.5  (390)
  // нейтрал
  case SPELL_CHILL_TOUCH:
    ndice = 15;
    sdice = 2;
    adice = level;
    break;
   // кислота - для всех с 18го левела 5го круга (6 слотов)
   // *** мин 48 макс 70 (420)
   // нейтрал
  case SPELL_ACID:
    obj = NULL;
    if (IS_NPC(victim))
       {rand = number(1,50);
        if (rand <= WEAR_BOTHS)
           obj = GET_EQ(victim, rand);
        else
           for (rand -= WEAR_BOTHS, obj = victim->carrying; rand && obj;
               rand--, obj = obj->next_content);
       }
    if (obj)
       {ndice = 6;
        sdice = 10;
        adice = level;
        act("Кислота покрыла $o3.",FALSE,victim,obj,0,TO_CHAR);
        alterate_object(obj,number(level*2,level*4),100);
       }
    else
       {ndice = 6;
        sdice = 15;
        adice = (level - 18) * 2;
       }
    break;

  // землетрясение чернокнижники 22 уровень 7 круг (4)
  // *** мин 48 макс 60 (240)
  // нейтрал
  case SPELL_EARTHQUAKE:
    ndice = 6;
    sdice = 15;
    adice = (level - 22) * 2;
    if ((GET_GOD_FLAG(victim, GF_GODSCURSE) ||
         GET_MOB_HOLD(victim)               ||
         number(0,100) <= 30
        ) &&
        GET_POS(victim) > POS_SITTING &&
        !WAITLESS(victim)
       )
       {act("$n3 повалило на землю.",FALSE,victim,0,0,TO_ROOM);
        act("Вас повалило на землю.",FALSE,victim,0,0,TO_CHAR);
        GET_POS(victim) = POS_SITTING;
        update_pos(victim);
        WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
       }
    break;
  // Высосать жизнь - некроманы - уровень 18 круг 6й (5)
  // *** мин 54 макс 66 (330)
  case SPELL_SACRIFICE:
    if (victim == ch)
       return (0);
    if (!WAITLESS(victim))
       {ndice = 8;
        sdice = 8;
        adice = level;
       }
    break;

  /********** ДЛЯ ФРАГЕРОВ **********/
  // горящие руки - с 1го левела 1го круга (8 слотов)
  // *** мин 21 мах 30 (240)
  // ОГОНЬ
  case SPELL_BURNING_HANDS:
    ndice = 10;
    sdice = 3;
    adice = (level + 2) / 3;
    break;
  // обжигающая хватка - с 4го левела 2го круга (8 слотов)
  // *** мин 36 макс 45 (360)
  // ОГОНЬ
   case SPELL_SHOCKING_GRASP:
    ndice = 10;
    sdice = 6;
    adice = (level + 2) / 3;
    break;
  // молния - с 7го левела 3го круга (7 слотов)
  // *** мин 18 - макс 45 (315)
  // ВОЗДУХ
  case SPELL_LIGHTNING_BOLT:
    ndice = 3;
    sdice = 5;
    adice = 0;
    count = (level + 5) / 6;
    break;
  // яркий блик - с 7го 3го круга (7 слотов)
  // *** мин 33 - макс 40 (280)
  // ОГОНЬ
  case SPELL_SHINEFLASH:
    ndice = 10;
    sdice = 5;
    adice = (level + 2) / 3;
    if ((GET_GOD_FLAG(victim, GF_GODSCURSE) ||
         number(0,100) <= GET_LEVEL(ch) * 3
        )                               &&
        !AFF_FLAGGED(victim, AFF_BLIND) &&
        !WAITLESS(victim)
       )
       {act("$n ослеп$q !",FALSE,victim,0,0,TO_ROOM);
        act("Вы ослепли !",FALSE,victim,0,0,TO_CHAR);
        af.type      = SPELL_BLINDNESS;
        af.location  = APPLY_HITROLL;
        af.modifier  = 0;
        af.duration  = pc_duration(victim,2,level+7,8,0,0);
        af.bitvector = AFF_BLIND;
        af.battleflag= AF_BATTLEDEC;
        affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
       }
    break;
  // шаровая молния - с 10го левела 4го круга (6 слотов)
  // *** мин 35 макс 55 (330)
  // ВОЗДУХ
  case SPELL_CALL_LIGHTNING:
    ndice = 7;
    sdice = 6;
    adice = level;
    break;
  // огненная стрела - уровень 14 круг 5 (6 слотов)
  // *** мин 44 макс 60 (360)
  // ОГОНЬ
  case SPELL_COLOR_SPRAY:
    ndice = 10;
    sdice = 5;
    adice = level;
    break;
  // ледяной ветер - уровень 14 круг 5 (6 слотов)
  // *** мин 44 макс 60 (360)
  // ВОДА
  case SPELL_CONE_OF_COLD:
    ndice = 10;
    sdice = 5;
    adice = level;
    break;
  // Огненный шар - уровень 18 круг 6 (5 слотов)
  // *** мин 66 макс 80 (400)
  // ОГОНЬ
  case SPELL_FIREBALL:
    ndice = 11;
    sdice = 11;
    adice = (level - 18) * 2;
    break;
  // Огненный поток - уровень 18 круг 6 (5 слотов)
  // ***  мин 38 макс 50 (250)
  // ОГОНЬ, ареа
  case SPELL_FIREBLAST:
    ndice = 10;
    sdice = 3;
    adice = level;
    break;
  // метеоритный шторм - уровень 22 круг 7 (4 слота)
  // *** мин 66 макс 80  (240)
  // нейтрал, ареа
  case SPELL_METEORSTORM:
    ndice = 11;
    sdice = 11;
    adice = (level - 22) * 3;
    break;
  // цепь молний - уровень 22 круг 7 (4 слота)
  // *** мин 76 макс 100 (400)
  // ВОЗДУХ, ареа
  case SPELL_CHAIN_LIGHTNING:
    ndice = 4;
    sdice = 4;
    adice = level * 3;
    break;
  // гнев богов - уровень 26 круг 8 (2 слота)
  // *** мин 226 макс 250 (500)
  // ВОДА
  case SPELL_IMPLOSION:
    ndice = 10;
    sdice = 13;
    adice = level * 6;
    break;
  // ледяной шторм - 26 левела 8й круг (2)
  // *** мин 55 макс 75 (150)
  // ВОДА, ареа
  case SPELL_ICESTORM:
    ndice = 10;
    sdice = 10;
    adice = (level - 26) * 5;
    if ((GET_GOD_FLAG(victim, GF_GODSCURSE) ||
         WAITLESS(ch)  ||  number(1,100) <= 70)  &&
        !WAITLESS(victim)
       )
       {act("$n3 оглушило.",FALSE,victim,0,0,TO_ROOM);
        act("Вас оглушило.",FALSE,victim,0,0,TO_CHAR);
        WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
        af.type      = SPELL_ICESTORM;
        af.location  = 0;
        af.modifier  = 0;
        af.duration  = pc_duration(victim,2,0,0,0,0);
        af.bitvector = AFF_STOPFIGHT;
        af.battleflag= AF_BATTLEDEC;
        affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
       }
    break;
  // суд богов - уровень 28 круг 9 (1 слот)
  // *** мин 188 макс 200 (200)
  // ВОЗДУХ, ареа
  case SPELL_ARMAGEDDON:
    ndice = 10;
    sdice = 3;
    adice = level * 6;
    break;



  /******* ХАЙЛЕВЕЛ СУПЕРДАМАДЖ МАГИЯ ******/
  // истощить энергию - круг 28 уровень 9 (1)
  // для всех
  case SPELL_ENERGY_DRAIN:
   dam        = -1;
   no_savings = TRUE;
   if (ch == victim ||
       (number(1,100) <= 33 &&
        !general_savingthrow(victim, SAVING_ROD,modi)
       )
      )
      {int i;
       for (i = 0; i <= MAX_SPELLS; GET_SPELL_MEM(victim,i++) = 0);
      };
   break;
  // каменное проклятие - круг 28 уровень 9 (1)
  // для всех
  case SPELL_STUNNING:
   dam        = -1;
   no_savings = TRUE;
   if (ch == victim ||
       ((rand = number(1,100)) < GET_LEVEL(ch) &&
        !general_savingthrow(victim, SAVING_ROD,modi)
       )
      )
      {dam = MAX(1, GET_HIT(victim) + 1);
      }
   else
   if (rand > 50 && rand < 60 && !WAITLESS(ch))
      {send_to_char("Ваша магия обратилась против Вас.\r\n",ch);
       GET_HIT(ch) = 1;
      }
   break;
  // круг пустоты - круг 28 уровень 9 (1)
  // для всех
  case SPELL_VACUUM:
   dam        = -1;
   no_savings = TRUE;
   if (ch == victim ||
       (number(1,100) <= 33 &&
        !general_savingthrow(victim, SAVING_ROD,modi)
       )
      )
      {dam = MAX(1, GET_HIT(victim) + 1);
      };
   break;


  /********* СПЕЦИФИЧНАЯ ДЛЯ КЛЕРИКОВ МАГИЯ **********/
  case SPELL_DAMAGE_LIGHT:
    ndice = 4;
    sdice = 3;
    adice = (level + 2) / 3;
    break;
  case SPELL_DAMAGE_SERIOUS:
    ndice = 8;
    sdice = 3;
    adice = (level + 1) / 2;
    break;
  case SPELL_DAMAGE_CRITIC:
    ndice = 15;
    sdice = 3;
    adice = (level + 1) / 2;
    break;
  case SPELL_DISPEL_EVIL:
    ndice = 4;
    sdice = 4;
    adice = level;
    if (ch != victim  &&
        IS_EVIL(ch)   &&
        !WAITLESS(ch) &&
        GET_HIT(ch) > 1
       )
       {send_to_char("Ваша магия обратилась против Вас.", ch);
        GET_HIT(ch) = 1;
       }
    if (!IS_EVIL(victim))
       {if (victim != ch)
           act("Боги защитили $N3 от Вашей магии.", FALSE, ch, 0, victim, TO_CHAR);
        return (0);
       };
    break;
  case SPELL_DISPEL_GOOD:
    ndice = 4;
    sdice = 4;
    adice = level;
    if (ch != victim  &&
        IS_GOOD(ch)   &&
        !WAITLESS(ch) &&
        GET_HIT(ch) > 1
       )
       {send_to_char("Ваша магия обратилась против Вас.", ch);
        GET_HIT(ch) = 1;
       }
    if (!IS_GOOD(victim))
       {if (victim != ch)
           act("Боги защитили $N3 от Вашей магии.", FALSE, ch, 0, victim, TO_CHAR);
        return (0);
       };
    break;
  case SPELL_HARM:
    ndice = 7;
    sdice = 10;
    adice = (level + 4) / 5;
    break;

  case SPELL_FIRE_BREATH:
  case SPELL_GAS_BREATH:
  case SPELL_FROST_BREATH:
  case SPELL_ACID_BREATH:
  case SPELL_LIGHTNING_BREATH:
    if (!IS_NPC(ch))
       return (0);
    ndice = ch->mob_specials.damnodice;
    sdice = ch->mob_specials.damsizedice;
    adice = GET_REAL_DR(ch) + str_app[STRENGTH_APPLY_INDEX(ch)].todam;
    break;
  case SPELL_FEAR:
    if (!general_savingthrow(victim, savetype, calc_anti_savings(ch)) &&
        !MOB_FLAGGED(victim, MOB_NOFEAR)
       )
       {go_flee(victim);
        return (0);
       }
    dam        = -1;
    no_savings = TRUE;
    break;
  } /* switch(spellnum) */

  if (!dam && !no_savings)
     {double koeff = 1;
      if (IS_NPC(victim))
         {if (NPC_FLAGGED(victim,NPC_FIRECREATURE))
             {if (IS_SET(SpINFO.violent,MTYPE_FIRE))
                 koeff /= 2;
              if (IS_SET(SpINFO.violent,MTYPE_WATER))
                 koeff *= 2;
             }
          if (NPC_FLAGGED(victim,NPC_AIRCREATURE))
             {if (IS_SET(SpINFO.violent,MTYPE_EARTH))
                 koeff *= 2;
              if (IS_SET(SpINFO.violent,MTYPE_AIR))
                 koeff /= 2;
             }
          if (NPC_FLAGGED(victim,NPC_WATERCREATURE))
             {if (IS_SET(SpINFO.violent,MTYPE_FIRE))
                 koeff *= 2;
              if (IS_SET(SpINFO.violent,MTYPE_WATER))
                 koeff /= 2;
             }
          if (NPC_FLAGGED(victim,NPC_EARTHCREATURE));
             {if (IS_SET(SpINFO.violent,MTYPE_EARTH))
                 koeff /= 2;
              if (IS_SET(SpINFO.violent,MTYPE_AIR))
                 koeff *= 2;
             }
         }
      dam = dice(ndice,sdice) + adice;
      // sprintf(buf,"Базовое повреждение - %d хп",dam);
      // act(buf,FALSE,ch,0,0,TO_CHAR);
      dam = complex_spell_modifier(ch,spellnum,GAPPLY_SPELL_EFFECT,dam);
      // sprintf(buf,"Погодный модификатор - %d хп",dam);
      // act(buf,FALSE,ch,0,0,TO_CHAR);
      if (AFF_FLAGGED(victim, AFF_SANCTUARY))
         koeff *= 2;
      if (ch != victim && general_savingthrow(victim, savetype, modi))
         koeff /= 2;
      if (AFF_FLAGGED(victim, AFF_PRISMATICAURA))
         koeff /= 2;
      koeff -= (koeff * MIN((GET_ABSORBE(victim)+1)/2,25) / 100);
      if (ch != victim &&
          AFF_FLAGGED(victim,AFF_FIREAURA) &&
          IS_SET(SpINFO.violent,MTYPE_WATER) &&
          number(1,100) <= 25
         )
         dam = 0;
      if (ch != victim &&
          AFF_FLAGGED(victim,AFF_ICEAURA) &&
          IS_SET(SpINFO.violent,MTYPE_FIRE) &&
          number(1,100) <= 25
         )
         dam = 0;
      if (ch != victim &&
          AFF_FLAGGED(victim,AFF_AIRAURA) &&
          IS_SET(SpINFO.violent,MTYPE_AIR) &&
          number(1,100) <= 25
         )
         dam = 0;
      if (dam > 0)
         {koeff *= 1000;
          dam    = MAX(1, dam * MAX(300,MIN(koeff,2500)) / 1000);
          if (spellnum == SPELL_SACRIFICE && ch != victim)
             {int sacrifice = MAX(1,MIN(dam*count,GET_HIT(victim)));
	      do_sacrifice(ch,sacrifice);
              if (!IS_NPC(ch))
                 {struct follow_type *f;
                  for (f = ch->followers; f; f = f->next)
                      if (IS_NPC(f->follower) &&
                          AFF_FLAGGED(f->follower,AFF_CHARM) &&
                          MOB_FLAGGED(f->follower,MOB_CORPSE)
                         )
                         do_sacrifice(f->follower,sacrifice);
                 }
             }
         }
     }
  dam = MAX(0,dam);

  // And finally, inflict the damage and call fight_mtrg because hit() not called
  for (; count > 0 && rand >= 0; count--)
      {if (IN_ROOM(ch)     != NOWHERE    &&
           IN_ROOM(victim) != NOWHERE    &&
	   GET_POS(ch)     > POS_STUNNED &&
	   GET_POS(victim) > POS_DEAD
	  )
	  fight_mtrigger(ch);
       if (IN_ROOM(ch)     != NOWHERE   &&
           IN_ROOM(victim) != NOWHERE    &&
	   GET_POS(ch)     > POS_STUNNED &&
	   GET_POS(victim) > POS_DEAD
	  )
          rand = damage(ch, victim, dam, spellnum, count <= 1);
      }
  return rand;
}


/*
 * Every spell that does an affect comes through here.  This determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
*/

#define MAX_SPELL_AFFECTS 5	/* change if more needed */
#define SpINFO spell_info[spellnum]

int pc_duration(struct char_data *ch,int cnst, int level, int level_divisor, int min, int max)
{int result = 0;
 if (IS_NPC(ch))
    {result = cnst;
     if (level > 0 && level_divisor > 0)
        level  = level / level_divisor;
     else
        level  = 0;
     if (min > 0)
        level = MIN(level, min);
     if (max > 0)
        level = MAX(level, max);
     return (level + result);
    }
 result = cnst  * SECS_PER_MUD_HOUR;
 if (level > 0 && level_divisor > 0)
    level  = level * SECS_PER_MUD_HOUR / level_divisor;
 else
    level  = 0;
 if (min > 0)
    level = MIN(level, min * SECS_PER_MUD_HOUR);
 if (max > 0)
    level = MAX(level, max * SECS_PER_MUD_HOUR);
 result = (level + result) / SECS_PER_PLAYER_AFFECT;
 return ( result );
}


void mag_affects(int level,    struct char_data * ch, struct char_data * victim,
		         int spellnum, int savetype)
{
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool   accum_affect  = FALSE, accum_duration = FALSE, success = TRUE;
  const  char *to_vict = NULL, *to_room = NULL;
  int    i, modi=0, heal = FALSE;


  if (victim == NULL || IN_ROOM(victim) == NOWHERE ||
      ch == NULL
     )
     return;

  // Magic glass
  if (ch != victim   &&
      SpINFO.violent &&
      ((AFF_FLAGGED(victim,AFF_MAGICGLASS) && number(1,100) < GET_LEVEL(victim)
       ) ||
       (IS_GOD(victim) && (IS_NPC(ch) || GET_LEVEL(victim) > GET_LEVEL(ch))
       )
      )
     )
    {act("Магическое зеркало $N1 отразило Вашу магию !",FALSE,ch,0,victim,TO_CHAR);
     act("Ваше магическое зеркало отразило поражение $n1 !",FALSE,ch,0,victim,TO_VICT);
     mag_affects(level,ch,ch,spellnum,savetype);
     return;
    }


  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      {af[i].type       = spellnum;
       af[i].bitvector  = 0;
       af[i].modifier   = 0;
       af[i].battleflag = 0;
       af[i].location   = APPLY_NONE;
      }

  /* decreese modi for failing, increese fo success */
  if (ch != victim)
     {modi  = con_app[GET_REAL_CON(victim)].affect_saving;
      modi += calc_anti_savings(ch);
     }

  log("[MAG Affect] Modifier value for %s (caster %s) = %d(spell %d)",
      GET_NAME(victim), GET_NAME(ch), modi, spellnum);

  switch (spellnum)
  {
  case SPELL_CHILL_TOUCH:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT,ch);
        success = FALSE;
        break;
       }
    af[0].location    = APPLY_STR;
    af[0].duration    = pc_duration(victim,2,level,4,6,0);
    af[0].modifier    = -1;
    af[0].battleflag  = AF_BATTLEDEC;
    accum_duration    = TRUE;
    to_room           = "Боевый пыл $n1 несколько остыл.";
    to_vict           = "Вы почувствовали себя слабее !";
    break;

  case SPELL_ENERGY_DRAIN:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT,ch);
        success = FALSE;
        break;
       }
    af[0].duration = pc_duration(victim,2,level,4,6,0);
    af[0].location = APPLY_STR;
    af[0].modifier    = -2;
    af[0].battleflag  = AF_BATTLEDEC;
    accum_duration    = TRUE;
    to_room           = "$n стал$g немного слабее.";
    to_vict           = "Вы почувствовали себя слабее !";
    break;

  case SPELL_WEAKNESS:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT,ch);
        success = FALSE;
        break;
       }
    if (affected_by_spell(victim,SPELL_STRENGTH))
       {affect_from_char(victim,SPELL_STRENGTH);
        success = FALSE;
        break;
       }
    af[0].duration    = pc_duration(victim,4,level,6,4,0);
    af[0].location    = APPLY_STR;
    af[0].modifier    = -1;
    af[0].battleflag  = AF_BATTLEDEC;
    accum_duration    = TRUE;
    to_room           = "$n стал$g немного слабее.";
    to_vict           = "Вы почувствовали себя слабее !";
    break;

  case SPELL_STONESKIN:
    af[0].location = APPLY_ABSORBE;
    af[0].modifier = (level * 2 + 1) / 3;
    af[0].duration = pc_duration(victim,0,level+5,6,0,0);
    accum_duration = TRUE;
    to_room        = "Кожа $n1 покрылась каменными пластинами.";
    to_vict        = "Вы стали менее чувствительны к ударам.";
    break;

  case SPELL_FAST_REGENERATION:
    af[0].location  = APPLY_HITREG;
    af[0].modifier  = 50;
    af[0].duration  = pc_duration(victim,2,level,5,0,0);
    af[1].location  = APPLY_MOVEREG;
    af[1].modifier  = 50;
    af[1].duration  = pc_duration(victim,2,level,5,0,0);
    accum_duration  = TRUE;
    to_room          = "$n расцвел$g на Ваших глазах.";
    to_vict          = "Вас наполнила живительная сила.";
    break;

  case SPELL_AIR_SHIELD:
    af[0].bitvector  = AFF_AIRSHIELD;
    af[0].battleflag = TRUE;
    af[0].duration   = pc_duration(victim,8,0,0,0,0);
    to_room          = "$n3 окутал воздушный щит.";
    to_vict          = "Вас окутал воздушный щит.";
    break;

  case SPELL_FIRE_SHIELD:
    af[0].bitvector  = AFF_FIRESHIELD;
    af[0].battleflag = TRUE;
    af[0].duration   = pc_duration(victim,8,0,0,0,0);
    to_room          = "$n3 окутал огненный щит.";
    to_vict          = "Вас окутал огненный щит.";
    break;

  case SPELL_ICE_SHIELD:
    af[0].bitvector  = AFF_ICESHIELD;
    af[0].battleflag = TRUE;
    af[0].duration   = pc_duration(victim,8,0,0,0,0);
    to_room          = "$n3 окутал ледяной щит.";
    to_vict          = "Вас окутал ледяной щит.";
    break;

  case SPELL_AIR_AURA:
    af[0].bitvector  = AFF_AIRAURA;
    af[0].duration   = pc_duration(victim,6,0,0,0,0);
    to_room          = "$n3 окружила воздушная аура.";
    to_vict          = "Вас окружила воздушная аура.";
    break;

  case SPELL_FIRE_AURA:
    af[0].bitvector  = AFF_FIREAURA;
    af[0].duration   = pc_duration(victim,6,0,0,0,0);
    accum_duration   = TRUE;
    to_room          = "$n3 окружила огненная аура.";
    to_vict          = "Вас окружила огненная аура.";
    break;

  case SPELL_ICE_AURA:
    af[0].bitvector  = AFF_ICEAURA;
    af[0].duration   = pc_duration(victim,6,0,0,0,0);
    accum_duration   = TRUE;
    to_room          = "$n3 окружила ледяная аура.";
    to_vict          = "Вас окружила ледяная аура.";
    break;


  case SPELL_CLOUDLY:
    af[0].location = APPLY_AC;
    af[0].modifier = -20;
    af[0].duration = pc_duration(victim,0,level,2,0,0);
    accum_duration = TRUE;
    to_room = "Очертания $n1 расплылись и стали менее отчетливыми.";
    to_vict = "Ваше тело стало прозрачным, как туман.";
    break;

  case SPELL_ARMOR:
    af[0].location = APPLY_AC;
    af[0].modifier = -20;
    af[0].duration = pc_duration(victim,0,level,2,0,0);
    af[1].location = APPLY_SAVING_BASIC;
    af[1].modifier = -5;
    af[1].duration = af[0].duration;
    af[1].location = APPLY_SAVING_SPELL;
    af[1].modifier = -5;
    af[1].duration = af[0].duration;
    accum_duration = TRUE;
    to_room = "Вокруг $n1 вспыхнул белый щит и тут же погас.";
    to_vict = "Вы почувствовали вокруг себя невидимую защиту.";
    break;

  case SPELL_BLESS:
    af[0].location = APPLY_SAVING_SPELL;
    af[0].modifier = -5;
    af[0].duration = pc_duration(victim,6,0,0,0,0);
    af[0].bitvector= AFF_BLESS;
    af[1].location = APPLY_SAVING_PARA;
    af[1].modifier = -5;
    af[1].duration = pc_duration(victim,6,0,0,0,0);
    af[1].bitvector= AFF_BLESS;
    to_room = "$n осветил$u на миг неземным светом.";
    to_vict = "Боги одарили Вас своей улыбкой.";
    break;

  case SPELL_AWARNESS:
    af[0].duration = pc_duration(victim,6,0,0,0,0);
    af[0].bitvector= AFF_AWARNESS;
    to_room        = "$n начал$g внимательно осматриваться по сторонам.";
    to_vict        = "Вы стали более внимательны к окружающему.";
    break;

  case SPELL_SHIELD:
    af[0].duration   = pc_duration(victim,4,0,0,0,0);
    af[0].bitvector  = AFF_SHIELD;
    af[0].location   = APPLY_SAVING_SPELL;
    af[0].modifier   = -10;
    af[0].battleflag = AF_BATTLEDEC;
    af[1].duration   = pc_duration(victim,4,0,0,0,0);
    af[1].bitvector  = AFF_SHIELD;
    af[1].location   = APPLY_SAVING_PARA;
    af[1].modifier   = -10;
    af[1].battleflag = AF_BATTLEDEC;
    af[2].duration   = pc_duration(victim,4,0,0,0,0);
    af[2].bitvector  = AFF_SHIELD;
    af[2].location   = APPLY_SAVING_BASIC;
    af[2].modifier   = -10;
    af[2].battleflag = AF_BATTLEDEC;

    to_room = "$n покрыл$u сверкающим коконом.";
    to_vict = "Вас покрыл голубой кокон.";
    break;

  case SPELL_HASTE:
    if (affected_by_spell(victim, SPELL_SLOW))
       {affect_from_char(victim, SPELL_SLOW);
        success = FALSE;
       }
    else
       {af[0].duration = pc_duration(victim,9,0,0,0,0);
        af[0].bitvector= AFF_HASTE;
        to_vict = "Вы начали двигаться быстрее.";
        to_room = "$n начал$g двигаться заметно быстрее.";
       }
    break;

  case SPELL_PROTECT_MAGIC:
    af[0].location = APPLY_SAVING_SPELL;
    af[0].modifier = -20;
    af[0].duration = pc_duration(victim,2,level,4,0,0);
    accum_duration = TRUE;
    to_room = "$n осветил$u на миг тусклым светом.";
    to_vict = "Боги взяли Вас под свою опеку.";
    break;

  case SPELL_ENLARGE:
    if (affected_by_spell(victim, SPELL_ENLESS))
       {affect_from_char(victim, SPELL_ENLESS);
        success = FALSE;
       }
    else
       {af[0].location = APPLY_SIZE;
        af[0].modifier = 5 + level/3;
        af[0].duration = pc_duration(victim,0,level,4,0,0);
        accum_duration = TRUE;
        to_room  = "$n начал$g расти, как на дрожжах.";
        to_vict  = "Вы стали крупнее.";
       }
    break;

  case SPELL_ENLESS:
    if (affected_by_spell(victim, SPELL_ENLARGE))
       {affect_from_char(victim, SPELL_ENLARGE);
        success = FALSE;
       }
    else
       {af[0].location = APPLY_SIZE;
        af[0].modifier = -(5 + level/3);
        af[0].duration = pc_duration(victim,0,level,4,0,0);
        accum_duration = TRUE;
        to_room  = "$n скукожил$u.";
        to_vict  = "Вы стали мельче.";
       }
    break;

  case SPELL_MAGICGLASS:
    af[0].bitvector= AFF_MAGICGLASS;
    af[0].duration = pc_duration(victim,1,GET_REAL_INT(ch),9,0,0);
    accum_duration = TRUE;
    to_room  = "$n покрыла зеркальная пелена.";
    to_vict  = "Вас прокрыло зеркало магии.";
    break;

  case SPELL_STONEHAND:
    af[0].bitvector= AFF_STONEHAND;
    af[0].duration = pc_duration(victim,0,GET_LEVEL(ch),3,0,0);
    accum_duration = TRUE;
    to_room  = "Руки $n1 задубели.";
    to_vict  = "Ваши руки задубели.";
    break;

  case SPELL_PRISMATICAURA:
    if (!IS_NPC(ch) && !same_group(ch,victim))
       {send_to_char("Только на себя или одногруппника !\r\n",ch);
        return;
       }
    af[0].bitvector= AFF_PRISMATICAURA;
    af[0].duration = pc_duration(victim,2,GET_LEVEL(ch) + (GET_REAL_INT(ch) >> 1),1,0,0);
    accum_duration = TRUE;
    to_room  = "$n3 покрыла призматическая аура.";
    to_vict  = "Вас покрыла призматическая аура.";
    break;

  case SPELL_STAIRS:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].duration  = pc_duration(victim,0,level,3,0,0);
    af[0].bitvector = AFF_STAIRS;
    to_room = "$n0 стал$g отчетливо заметен !";
    to_vict = "Вы стали отчетливо заметны !";
    break;

  case SPELL_MINDLESS:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].location  = APPLY_MANAREG;
    af[0].modifier  = -50;
    af[0].duration  = pc_duration(victim,0,GET_REAL_WIS(ch) + GET_REAL_INT(ch),10,0,0);
    af[1].location  = APPLY_CAST_SUCCESS;
    af[1].modifier  = -50;
    af[1].duration  = af[0].duration;
    af[2].location  = APPLY_HITROLL;
    af[2].modifier  = -5;
    af[2].duration  = af[0].duration;

    to_room = "$n0 стал$g слаб на голову !";
    to_vict = "Ваш разум помутилcя !";
    break;


  case SPELL_POWER_BLINDNESS:
  case SPELL_BLINDNESS:
    if (MOB_FLAGGED(victim,MOB_NOBLIND) ||
        ((ch != victim) && general_savingthrow(victim, savetype, modi))
       )
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].type      = SPELL_BLINDNESS;
    af[0].location  = APPLY_HITROLL;
    af[0].modifier  = 0;
    af[0].duration  = spellnum == SPELL_POWER_BLINDNESS ?
                      pc_duration(victim,3,level,6,0,0) :
                      pc_duration(victim,2,level,8,0,0);
    af[0].bitvector = AFF_BLIND;
    af[0].battleflag= AF_BATTLEDEC;
    to_room = "$n0 ослеп$q !";
    to_vict = "Вы ослепли !";
    break;

  case SPELL_MADNESS:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].duration  = pc_duration(victim,3,0,0,0,0);
    af[0].bitvector = AFF_NOFLEE;
    to_room = "Теперь $n не сможет сбежать из боя !";
    to_vict = "Вас обуяло безумие боя !";
    break;

  case SPELL_WEB:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].location  = APPLY_HITROLL;
    af[0].modifier  = -2;
    af[0].duration  = pc_duration(victim,3,level,6,0,0);
    af[0].battleflag= AF_BATTLEDEC;
    af[0].bitvector = AFF_NOFLEE;
    af[1].location  = APPLY_AC;
    af[1].modifier  = 20;
    af[1].duration  = af[0].duration;
    af[1].battleflag= AF_BATTLEDEC;
    af[1].bitvector = AFF_NOFLEE;
    to_room = "$n1 покрыла невидимая паутина, сковывая $s движения !";
    to_vict = "Вас покрыла невидимая паутина !";
    break;


  case SPELL_CURSE:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }
    af[0].location  = APPLY_INITIATIVE;
    af[0].duration  = pc_duration(victim,1,level,2,0,0);
    af[0].modifier  = -5;
    af[0].bitvector = AFF_CURSE;

    accum_duration = TRUE;
    accum_affect   = TRUE;
    to_room = "Красное сияние вспыхнуло над $n4 и тут же погасло !";
    to_vict = "Боги сурово поглядели на Вас.";
    break;

  case SPELL_SLOW:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    if (affected_by_spell(victim, SPELL_HASTE))
       {affect_from_char(victim, SPELL_HASTE);
        success = FALSE;
        break;
       }

    af[0].duration  = pc_duration(victim,9,0,0,0,0);
    af[0].bitvector = AFF_SLOW;
    to_room         = "Движения $n1 заметно замедлились.";
    to_vict         = "Ваши движения заметно замедлились.";
    break;

  case SPELL_DETECT_ALIGN:
    af[0].duration  = pc_duration(victim,12,level,1,0,0);
    af[0].bitvector = AFF_DETECT_ALIGN;
    accum_duration  = TRUE;
    to_vict         = "Ваши глаза приобрели зеленый оттенок.";
    to_room         = "Глаза $n1 приобрели зеленый оттенок.";
    break;

  case SPELL_DETECT_INVIS:
    af[0].duration  = pc_duration(victim,10,level,1,0,0);
    af[0].bitvector = AFF_DETECT_INVIS;
    accum_duration  = TRUE;
    to_vict         = "Ваши глаза приобрели золотистый оттенок.";
    to_room         = "Глаза $n1 приобрели золотистый оттенок.";
    break;

  case SPELL_DETECT_MAGIC:
    af[0].duration  = pc_duration(victim,10,level,1,0,0);
    af[0].bitvector = AFF_DETECT_MAGIC;
    accum_duration = TRUE;
    to_vict        = "Ваши глаза приобрели желтый оттенок.";
    to_room        = "Глаза $n1 приобрели желтый оттенок.";
    break;

  case SPELL_INFRAVISION:
    af[0].duration  = pc_duration(victim,12,level,1,0,0);
    af[0].bitvector = AFF_INFRAVISION;
    accum_duration  = TRUE;
    to_vict         = "Ваши глаза приобрели красный оттенок.";
    to_room         = "Глаза $n1 приобрели красный оттенок.";
    break;

  case SPELL_DETECT_POISON:
    af[0].duration  = pc_duration(victim,12,level,1,0,0);
    af[0].bitvector = AFF_DETECT_POISON;
    accum_duration  = TRUE;
    to_vict         = "Ваши глаза приобрели карий оттенок.";
    to_room         = "Глаза $n1 приобрели карий оттенок.";
    break;

  case SPELL_INVISIBLE:
    if (!victim)
       victim = ch;

    af[0].duration  = pc_duration(victim,12,level,4,0,0);
    af[0].modifier  = -40;
    af[0].location  = APPLY_AC;
    af[0].bitvector = AFF_INVISIBLE;
    accum_duration  = TRUE;
    to_vict         = "Вы стали невидимы для окружающих.";
    to_room         = "$n медленно растворил$u в пустоте.";
    break;

  case SPELL_PLAQUE:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].location  = APPLY_HITREG;
    af[0].duration  = pc_duration(victim,0,level,2,0,0);
    af[0].modifier  = -95;
    af[1].location  = APPLY_MANAREG;
    af[1].duration  = pc_duration(victim,0,level,2,0,0);
    af[1].modifier  = -95;
    af[2].location  = APPLY_MOVEREG;
    af[2].duration  = pc_duration(victim,0,level,2,0,0);
    af[2].modifier  = -95;
    to_vict         = "Вас скрутило в жестокой лихорадке.";
    to_room         = "$n1 скрутило в жестокой лихорадке.";
    break;


  case SPELL_POISON:
    if (ch != victim &&
        general_savingthrow(victim, savetype, modi + con_app[GET_REAL_CON(victim)].poison_saving)
       )
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].location  = APPLY_STR;
    af[0].duration  = pc_duration(victim,0,level,1,0,0);
    af[0].modifier  = -2;
    af[0].bitvector = AFF_POISON;
    af[1].location  = APPLY_POISON;
    af[1].duration  = af[0].duration;
    af[1].modifier  = level;
    af[1].bitvector = AFF_POISON;
    to_vict         = "Вы почувствовали себя отравленным.";
    to_room         = "$n позеленел$g от действия яда.";
    break;

  case SPELL_PROT_FROM_EVIL:
    af[0].duration  = pc_duration(victim,24,0,0,0,0);
    af[0].bitvector = AFF_PROTECT_EVIL;
    accum_duration  = TRUE;
    to_vict         = "Вы подавили в себе страх к тьме.";
    break;

  case SPELL_EVILESS:
    af[0].duration  = pc_duration(victim,6,0,0,0,0);
    af[0].location  = APPLY_DAMROLL;
    af[0].modifier  = 20;
    af[0].bitvector = AFF_EVILESS;
    af[1].duration  = pc_duration(victim,6,0,0,0,0);
    af[1].location  = APPLY_HITROLL;
    af[1].modifier  = 5;
    af[1].bitvector = AFF_EVILESS;
    af[2].duration  = pc_duration(victim,6,0,0,0,0);
    af[2].location  = APPLY_HIT;
    af[2].modifier  = GET_REAL_MAX_HIT(victim);
    af[2].bitvector = AFF_EVILESS;
    heal            = TRUE;
    to_vict         = "Черное облако покрыло Вас.";
    to_room         = "Черное облако покрыло $n3 с головы до пят.";
    break;

  case SPELL_SANCTUARY:
    if (!IS_NPC(ch) && !same_group(ch,victim))
       {send_to_char("Только на себя или одногруппника !\r\n",ch);
        return;
       }
    af[0].duration  = pc_duration(victim,3,level,5,0,0);
    af[0].bitvector = AFF_SANCTUARY;
    to_vict         = "Белая аура мгновенно окружила Вас.";
    to_room         = "Белая аура покрыла $n3 с головы до пят.";
    break;

  case SPELL_SLEEP:
    if (MOB_FLAGGED(victim, MOB_NOSLEEP) ||
        (ch != victim && general_savingthrow(victim, SAVING_PARA, modi))
       )
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       };

    af[0].duration  = pc_duration(victim,1,level,6,1,6);
    af[0].bitvector = AFF_SLEEP;
    af[0].battleflag= AF_BATTLEDEC;
    if (GET_POS(victim) > POS_SLEEPING && success)
      {send_to_char("Вы слишком устали...  Спать... Спа...\r\n", victim);
       act("$n прилег$q подремать.", TRUE, victim, 0, 0, TO_ROOM);
       GET_POS(victim) = POS_SLEEPING;
      }
    break;

  case SPELL_STRENGTH:
    if (affected_by_spell(victim, SPELL_WEAKNESS))
       {affect_from_char(victim, SPELL_WEAKNESS);
        success = FALSE;
        break;
       }
    af[0].location = APPLY_STR;
    af[0].duration = pc_duration(victim,4,level,4,0,0);
    if (ch == victim)
       af[0].modifier = (level + 9) / 10;
    else
       af[0].modifier = (level + 14) / 15;
    accum_duration = TRUE;
    accum_affect   = TRUE;
    to_vict        = "Вы почувствовали себя сильнее.";
    to_room        = "Мышцы $n1 налились силой.";
    break;

  case SPELL_SENSE_LIFE:
    to_vict         = "Вы способны разглядеть даже микроба.";
    af[0].duration  = pc_duration(victim,0,level,5,0,0);
    af[0].bitvector = AFF_SENSE_LIFE;
    accum_duration  = TRUE;
    break;

  case SPELL_WATERWALK:
    af[0].duration  = pc_duration(victim,24,0,0,0,0);
    af[0].bitvector = AFF_WATERWALK;
    accum_duration  = TRUE;
    to_vict         = "На рыбалку Вы можете отправляться без лодки.";
    break;

  case SPELL_WATERBREATH:
    af[0].duration  = pc_duration(victim,12,level,4,0,0);
    af[0].bitvector = AFF_WATERBREATH;
    accum_duration  = TRUE;
    to_vict         = "У Вас выросли жабры.";
    to_room         = "У $n1 выросли жабры.";
    break;

  case SPELL_POWER_HOLD:
  case SPELL_HOLD:
    if (MOB_FLAGGED(victim, MOB_NOHOLD) ||
        (ch != victim && general_savingthrow(victim, SAVING_PARA, modi))
       )
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].type       = SPELL_HOLD;
    af[0].duration   = spellnum == SPELL_POWER_HOLD ?
                       pc_duration(victim,2,level+7,8,2,5) :
                       pc_duration(victim,1,level+9,10,1,3);
    af[0].bitvector  = AFF_HOLD;
    af[0].battleflag = AF_BATTLEDEC;
    to_room          = "$n0 замер$q на месте !";
    to_vict          = "Вы замерли на месте, не в силах пошевельнуться.";
    break;

  case SPELL_POWER_SIELENCE:
  case SPELL_SIELENCE:
    if (MOB_FLAGGED(victim, MOB_NOSIELENCE) ||
        (ch != victim && general_savingthrow(victim, savetype, modi))
       )
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].type       = SPELL_SIELENCE;
    af[0].duration   = spellnum == SPELL_POWER_SIELENCE ?
                       pc_duration(victim,2,level+3,4,6,0) :
                       pc_duration(victim,2,level+7,8,3,0);
    af[0].bitvector  = AFF_SIELENCE;
    af[0].battleflag = AF_BATTLEDEC;
    to_room          = "$n0 прикусил$g язык !";
    to_vict          = "Вы не в состоянии вымолвить ни слова.";
    break;

  case SPELL_FLY:
    if (on_horse(victim))
       {send_to_char(NOEFFECT,ch);
        success = FALSE;
        break;
       }
    af[0].duration   = pc_duration(victim,10,level,2,0,0);
    af[0].bitvector  = AFF_FLY;
    to_room          = "$n0 медленно поднял$u в воздух.";
    to_vict          = "Вы медленно поднялись в воздух.";
    break;

  case SPELL_BLINK:
    af[0].duration   = pc_duration(victim,0,level,4,0,0);
    af[0].bitvector  = AFF_BLINK;
    to_room          = "$n начал$g мигать.";
    to_vict          = "Вы начали мигать.";
    break;

  case SPELL_NOFLEE:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].duration   = pc_duration(victim,3,level,4,4,0);
    af[0].bitvector  = AFF_NOFLEE;
    to_room          = "$n0 теперь прикован$a к $N2.";
    to_vict          = "Вы не сможете покинуть $N1.";
    break;

  case SPELL_LIGHT:
    af[0].duration   = pc_duration(victim,6,level,4,4,0);
    af[0].bitvector  = AFF_HOLYLIGHT;
    to_room          = "$n0 начал$g светиться ярким светом.";
    to_vict          = "Вы засветились, освещая комнату.";
    break;

  case SPELL_DARKNESS:
    af[0].duration   = pc_duration(victim,6,level,4,4,0);
    af[0].bitvector  = AFF_HOLYDARK;
    to_room          = "$n0 погрузил$g комнату во мрак.";
    to_vict          = "Вы погрузили комнату в непроглядную тьму.";
    break;
  }

  /*
   * If this is a mob that has this affect set in its mob file, do not
   * perform the affect.  This prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */
  if (IS_NPC(victim) && success)
     for (i = 0; i < MAX_SPELL_AFFECTS && success; i++)
         if (AFF_FLAGGED(victim, af[i].bitvector))
            {send_to_char(NOEFFECT, ch);
	     success = FALSE;
            }

  /*
   * If the victim is already affected by this spell
   */

  if (affected_by_spell(victim,spellnum) && success)
     {send_to_char(NOEFFECT, ch);
      success = FALSE;
     }

  /* Calculate PKILL's affects
     1) "NPC affect PC spellflag"  for victim
     2) "NPC affect NPC spellflag" if victim cann't pkill FICHTING(victim)
   */
   if (ch != victim)
      {//send_to_char("Start\r\n",ch);
       //send_to_char("Start\r\n",victim);
       if (IS_SET(SpINFO.routines, NPC_AFFECT_PC))
          {//send_to_char("1\r\n",ch);
           //send_to_char("1\r\n",victim);
	   if (may_pkill(ch,victim) != PC_REVENGE_PC)
              {//send_to_char("As agressor\r\n",ch);
               //send_to_char("As agressor\r\n",victim);
	       inc_pkill_group(victim,ch,1,0);
	      }
           else
              inc_pkill(victim,ch,0,1);
          }
       else
       if (IS_SET(SpINFO.routines, NPC_AFFECT_NPC) && FIGHTING(victim))
          {//send_to_char("2\r\n",ch);
           //send_to_char("2\r\n",victim);
	   if (may_pkill(ch,FIGHTING(victim)) != PC_REVENGE_PC)
              {//send_to_char("As helpee\r\n",ch);
               //send_to_char("As helpee\r\n",victim);
	       inc_pkill_group(FIGHTING(victim),ch,1,0);
	      }
           else
              inc_pkill(FIGHTING(victim),ch,0,1);
          }
       //send_to_char("Stop\r\n",ch);
       //send_to_char("Stop\r\n",victim);	
      }

   for (i = 0; success && i < MAX_SPELL_AFFECTS; i++)
       {if (af[i].bitvector || af[i].location != APPLY_NONE)
           {af[i].duration = complex_spell_modifier(ch,spellnum,GAPPLY_SPELL_EFFECT,af[i].duration);
            affect_join(victim, af+i, accum_duration, FALSE, accum_affect, FALSE);
	   }
       }

   if (success)
      {if (heal)
          GET_HIT(victim) = MAX(GET_HIT(victim), GET_REAL_MAX_HIT(victim));
       if (spellnum == SPELL_POISON)
          victim->Poisoner = GET_ID(ch);
       if (to_vict != NULL)
          act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
       if (to_room != NULL)
          act(to_room, TRUE, victim, 0, ch, TO_ROOM);
      }
}


/*
 * This function is used to provide services to mag_groups.  This function
 * is the one you should change to add new group spells.
 */

void perform_mag_groups(int level, struct char_data * ch,
			struct char_data * tch, int spellnum, int savetype)
{
  switch (spellnum)
  {case SPELL_GROUP_HEAL:
    mag_points(level, ch, tch, SPELL_HEAL, savetype);
    break;
   case SPELL_GROUP_ARMOR:
    mag_affects(level, ch, tch, SPELL_ARMOR, savetype);
    break;
   case SPELL_GROUP_RECALL:
    spell_recall(level, ch, tch, NULL);
    break;
   case SPELL_GROUP_STRENGTH:
    mag_affects(level, ch, tch, SPELL_STRENGTH, savetype);
    break;
   case SPELL_GROUP_BLESS:
    mag_affects(level, ch, tch, SPELL_BLESS, savetype);
    break;
   case SPELL_GROUP_HASTE:
    mag_affects(level, ch, tch, SPELL_HASTE, savetype);
    break;
   case SPELL_GROUP_FLY:
    mag_affects(level, ch, tch, SPELL_FLY, savetype);
    break;
   case SPELL_GROUP_INVISIBLE:
    mag_affects(level, ch, tch, SPELL_INVISIBLE, savetype);
    break;
   default:
    return;
  }
}

ASPELL(spell_eviless)
{struct char_data *tch;

 for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    if (tch && IS_NPC(tch) && tch->master == ch && MOB_FLAGGED(tch,MOB_CORPSE))
       mag_affects(GET_LEVEL(ch),ch,tch,SPELL_EVILESS,0);
}

/*
 * Every spell that affects the group should run through here
 * perform_mag_groups contains the switch statement to send us to the right
 * magic.
 *
 * group spells affect everyone grouped with the caster who is in the room,
 * caster last.
 *
 * To add new group spells, you shouldn't have to change anything in
 * mag_groups -- just add a new case to perform_mag_groups.
 */

void mag_groups(int level, struct char_data * ch, int spellnum, int savetype)
{
  struct char_data *tch;
  /*  struct follow_type *f, *f_next; */

  if (ch == NULL)
     return;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      if (same_group(ch, tch))
         perform_mag_groups(level, ch, tch, spellnum, savetype);

  /******************** Original function ***********************
  if (!AFF_FLAGGED(ch, AFF_GROUP))
     return;
  if (ch->master != NULL)
     k = ch->master;
  else
     k = ch;
  for (f = k->followers; f; f = f_next)
      {f_next = f->next;
       tch    = f->follower;
       if (tch->in_room != ch->in_room)
          continue;
       if (!AFF_FLAGGED(tch, AFF_GROUP))
          continue;
       if (ch == tch)
          continue;
       perform_mag_groups(level, ch, tch, spellnum, savetype);
      }

  if ((k != ch) && AFF_FLAGGED(k, AFF_GROUP))
     perform_mag_groups(level, ch, k, spellnum, savetype);
  perform_mag_groups(level, ch, ch, spellnum, savetype);
 **************************************************************/
}


struct char_list_data
{ struct char_data      *ch;
  struct char_list_data *next;
};

/*
 * mass spells affect every creature in the room except the caster.
 *
 * No spells of this class currently implemented as of Circle 3.0.
 */

void mag_masses(int level, struct char_data * ch, int spellnum, int savetype)
{
  struct char_data *tch;
  struct char_list_data *char_list = NULL, *char_one;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {CREATE(char_one, struct char_list_data, 1);
       char_one->ch   = tch;
       char_one->next = char_list;
       char_list      = char_one;
      }

  for (char_one = char_list; char_one; char_one = char_one->next)
      {tch = char_one->ch;
       if (IS_IMMORTAL(tch))                            /* immortal   */
          {char_one->ch = NULL;
           continue;
          }
       if (same_group(ch,tch))
          {char_one->ch = NULL;
           continue;                                     /* same groups */
          }
       switch (spellnum)
       {case SPELL_MASS_BLINDNESS:
            mag_affects(level, ch, tch, SPELL_BLINDNESS, SAVING_SPELL);
            break;
        case SPELL_MASS_HOLD:
            mag_affects(level, ch, tch, SPELL_HOLD,  SAVING_PARA);
            break;
        case SPELL_MASS_CURSE:
            mag_affects(level, ch, tch, SPELL_CURSE,  SAVING_SPELL);
            break;
        case SPELL_MASS_SIELENCE:
            mag_affects(level, ch, tch, SPELL_SIELENCE,  SAVING_SPELL);
            break;
        case SPELL_MASS_SLOW:
            mag_affects(level, ch, tch, SPELL_SLOW,  SAVING_SPELL);
            break;
        default:
            break;
       }
      }

  for (char_one = char_list; char_one; char_one = char_list)
      {char_list = char_one->next;
       cast_reaction(char_one->ch,ch,spellnum);
       free(char_one);
      }
}


/*
 * Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *  area spells have limited targets within the room.
*/

void mag_areas(int level, struct char_data * ch, int spellnum, int savetype)
{
  struct char_data *tch;
  const  char *to_char = NULL, *to_room = NULL;
  struct char_list_data *char_list = NULL, *char_one;

  if (ch == NULL)
     return;

  /*
   * to add spells to this fn, just add the message here plus an entry
   * in mag_damage for the damaging part of the spell.
   */
  switch (spellnum)
  {
  case SPELL_FORBIDDEN:
    if (world[IN_ROOM(ch)].forbidden)
       {send_to_char("Ваша магия потерпела неудачу.\r\n", ch);
        return;
       }
    world[IN_ROOM(ch)].forbidden = 1 + (GET_LEVEL(ch) + 14) / 15;
    send_to_char("Вы запечатали магией все входы в комнату.\r\n", ch);
    return;
  case SPELL_EARTHQUAKE:
    to_char = "Вы опустили руки и земля начала дрожать вокруг Вас !";
    to_room = "$n опустил$g руки и земля ушла у Вас из под ног !";
    break;
  case SPELL_FIREBLAST:
    to_char = "Вы потерли ладони и на них выспыхнуло яркое пламя !";
    to_room = "$n потер$g ладони и на них вспыхнуло яркое пламя !";
    break;
  case SPELL_ARMAGEDDON:
    to_char = "Вы сплели руки в замысловатом жесте и все потускнело !";
    to_room = "$n сплел$g руки в замысловатом жесте и все потускнело !";
    break;
  case SPELL_ICESTORM:
    to_char = "Вы воздели руки к небу и тысячи мелких льдинок хлынули вниз !";
    to_room = "$n воздел$g руки к небу и тысячи мелких льдинок хлынули вниз !";
    break;
  case SPELL_METEORSTORM:
    to_char = "Вы воздели руки к небу и огромные глыбы посыпались с небес !";
    to_room = "$n воздел$g руки к небу и огромные глыбы посыпались с небес !";
    break;

  default:
    return;
  }
  if (to_char != NULL)
     act(to_char, FALSE, ch, 0, 0, TO_CHAR);
  if (to_room != NULL)
     act(to_room, FALSE, ch, 0, 0, TO_ROOM);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {CREATE(char_one, struct char_list_data, 1);
       char_one->ch   = tch;
       char_one->next = char_list;
       char_list      = char_one;
      }

  for (char_one = char_list; char_one; char_one = char_one->next)
      {tch = char_one->ch;
       if (IS_IMMORTAL(tch))               /* immortal    */
          continue;
       if (IN_ROOM(ch) == NOWHERE ||       /* Something killed in process ... */
           IN_ROOM(tch) == NOWHERE
          )
          continue;
       switch (spellnum)
       {case SPELL_DISPEL_EVIL:
        case SPELL_DISPEL_GOOD:
             mag_damage(level, ch, tch, spellnum, SAVING_SPELL);
             break;
        default:
             if (tch != ch && !same_group(ch, tch))
                mag_damage(level, ch, tch, spellnum, SAVING_SPELL);
             break;
       }
      }

  for (char_one = char_list; char_one; char_one = char_list)
      {char_list = char_one->next;
       free(char_one);
      }
}

void mag_char_areas(int level, struct char_data * ch,
                    struct char_data *victim, int spellnum, int savetype)
{
  struct char_data *tch;
  const char *to_char = NULL, *to_room = NULL;
  int   decay=2;
  struct char_list_data *char_list = NULL, *char_one;

  if (ch == NULL)
      return;

  /*
   * to add spells to this fn, just add the message here plus an entry
   * in mag_damage for the damaging part of the spell.
   */
  switch (spellnum)
  {
  case SPELL_CHAIN_LIGHTNING:
    to_char = "Вы подняли руки к небу и оно осветилось яркими вспышками !";
    to_room ="$n поднял$g руки к небу и оно осветилось яркими вспышками !";
    decay   = 5;
    break;
  default:
    return;
  }

  if (to_char != NULL)
     act(to_char, FALSE, ch, 0, 0, TO_CHAR);
  if (to_room != NULL)
     act(to_room, FALSE, ch, 0, 0, TO_ROOM);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {CREATE(char_one, struct char_list_data, 1);
       char_one->ch   = tch;
       char_one->next = char_list;
       char_list      = char_one;
      }

  /* First - damage victim */
  mag_damage(level, ch, victim, spellnum, SAVING_SPELL);
  level -= decay;

  /* Damage other room members */
  for (char_one = char_list; char_one && level > 0; char_one = char_one->next)
      {tch = char_one->ch;
       if (IS_IMMORTAL(tch) || tch == victim || tch == ch)
          continue;
       if (IN_ROOM(ch) == NOWHERE || IN_ROOM(tch) == NOWHERE)
          continue;
       if (same_group(ch, tch))
          continue;
       mag_damage(level, ch, tch, spellnum, SAVING_SPELL);
       level -= decay;
      }

  for (char_one = char_list; char_one; char_one = char_list)
      {char_list = char_one->next;
       free(char_one);
      }
}


/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 *
 *  None of these spells are currently implemented in Circle 3.0; these
 *  were taken as examples from the JediMUD code.  Summons can be used
 *  for spells like clone, ariel servant, etc.
 *
 * 10/15/97 (gg) - Implemented Animate Dead and Clone.
 */

/*
 * These use act(), don't put the \r\n.
 */
const char *mag_summon_msgs[] = {
  "\r\n",
  "$n сделал$g несколько изящних пассов - Вы почувствовали странное дуновение !",
  "$n поднял$g труп !",
  "$N появил$G из клубов голубого дыма !",
  "$N появил$G из клубов зеленого дыма !",
  "$N появил$G из клубов красного дыма !",
  "$n сделал$g несколько изящных пассов - Вас обдало порывом холодного ветра.",
  "$n сделал$g несколько изящных пассов, от чего Ваши волосы встали дыбом.",
  "$n сделал$g несколько изящных пассов, обдав Вас нестерпимым жаром.",
  "$n сделал$g несколько изящных пассов, вызвав у Вас приступ тошноты.",
  "$n раздвоил$u !",
  "$n оживил$g труп !",
  "$n призвал$g защитника !"
};

/*
 * Keep the \r\n because these use send_to_char.
 */
const char *mag_summon_fail_msgs[] =
{
  "\r\n",
  "Нет такого существа в мире.\r\n",
  "Жаль, сорвалось...\r\n",
  "Ничего.\r\n",
  "Черт ! Ничего не вышло.\r\n",
  "Вы не смогли сделать этого !\r\n",
  "Ваша магия провалилась.\r\n",
  "У Вас нет подходящего трупа !\r\n"
};

/* These mobiles do not exist. */
#define MOB_SKELETON      100
#define MOB_ZOMBIE        101
#define MOB_BONEDOG       102
#define MOB_BONEDRAGON    103
#define MOB_KEEPER        104
#define MOB_FIREKEEPER    105

void mag_summons(int level, struct char_data * ch, struct obj_data * obj,
		      int spellnum, int savetype)
{
  struct   char_data     *mob = NULL;
  struct   obj_data      *tobj, *next_obj;
  struct   affected_type af;
  int      pfail = 0, msg = 0, fmsg = 0, handle_corpse = FALSE,keeper = FALSE;
  mob_vnum mob_num;

  if (ch == NULL)
     return;

  switch (spellnum)
     {
  case SPELL_CLONE:
    msg     = 10;
    fmsg    = number(2, 6);	/* Random fail message. */
    mob_num = MOB_BONEDRAGON;
    pfail   = 50;	        /* 50% failure, should be based on something later. */
    keeper  = TRUE;
    break;

  case SPELL_SUMMON_KEEPER:
  case SPELL_SUMMON_FIREKEEPER:
    msg     = 12;
    fmsg    = number(2, 6);
    mob_num = spellnum == SPELL_SUMMON_KEEPER ? MOB_KEEPER : MOB_FIREKEEPER;
    pfail   = 50;
    keeper  = TRUE;
    break;

  case SPELL_ANIMATE_DEAD:
    if (obj == NULL || !IS_CORPSE(obj))
       {act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
        return;
       }
    mob_num = GET_OBJ_VAL(obj,2);
    if (mob_num <= 0 )
       mob_num = MOB_SKELETON;
    else
       {pfail = 110 - con_app[GET_CON(mob_proto+real_mobile(mob_num))].ressurection -
                number(1,GET_LEVEL(ch));
        if (GET_LEVEL(mob_proto+real_mobile(mob_num)) <= 9)
           mob_num = MOB_SKELETON;
        else
        if (GET_LEVEL(mob_proto+real_mobile(mob_num)) <= 15)
           mob_num = MOB_ZOMBIE;
        else
        if (GET_LEVEL(mob_proto+real_mobile(mob_num)) <= 24)
           mob_num = MOB_BONEDOG;
        else
           mob_num = MOB_BONEDRAGON;
       }
    handle_corpse = TRUE;
    msg           = number(1, 9);
    fmsg          = number(2, 6);
//    pfail        += 10;	
    break;

  case SPELL_RESSURECTION:
    if (obj == NULL || !IS_CORPSE(obj))
       {act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
        return;
       }
    if ((mob_num = GET_OBJ_VAL(obj,2)) <= 0)
       {send_to_char("Вы не можете поднять труп этого монстра !\r\n",ch);
        return;
       }
    handle_corpse = TRUE;
    msg           = 11;
    fmsg          = number(2, 6);
    pfail = 110 - con_app[GET_CON(mob_proto+real_mobile(mob_num))].ressurection -
            number(1,GET_LEVEL(ch));
    break;

  default:
    return;
     }

  if (AFF_FLAGGED(ch, AFF_CHARM))
     {send_to_char("Вы слишком зависимы, чтобы искать себе последователей !\r\n", ch);
      return;
     }

  if (!IS_IMMORTAL(ch) && number(0, 101) < pfail)
     {send_to_char(mag_summon_fail_msgs[fmsg], ch);
      if (handle_corpse)
         extract_obj(obj);
      return;
     }

  if (!(mob = read_mobile(mob_num, VIRTUAL)))
     {send_to_char("Вы точно не помните, как создать данного монстра.\r\n", ch);
      return;
     }
  char_to_room(mob, ch->in_room);
  if (!check_charmee(ch,mob))
     {extract_char(mob,FALSE);
      if (handle_corpse)
         extract_obj(obj);
      return;
     }

  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;
  af.type            = SPELL_CHARM;
  if (weather_info.moon_day < 14)
     af.duration     = pc_duration(mob,GET_REAL_CHA(ch) + number(0,weather_info.moon_day % 14),0,0,0,0);
  else
     af.duration     = pc_duration(mob,GET_REAL_CHA(ch) + number(0,14 - weather_info.moon_day % 14),0,0,0,0);
  af.modifier        = 0;
  af.location        = 0;
  af.bitvector       = AFF_CHARM;
  af.battleflag      = 0;
  affect_to_char(mob, &af);
  if (keeper)
     {af.bitvector                = AFF_HELPER;
      affect_to_char(mob, &af);
      GET_SKILL(mob,SKILL_RESCUE) = 100;
     }

  SET_BIT(MOB_FLAGS(mob, MOB_CORPSE), MOB_CORPSE);
  if (spellnum == SPELL_CLONE)
     {/* Don't mess up the proto with strcpy. */
      mob->player.name        = str_dup(GET_NAME(ch));
      mob->player.short_descr = str_dup(GET_NAME(ch));
      mob->player.long_descr  = NULL;
      GET_PAD(mob,0)          = str_dup(GET_PAD(ch,0));
      GET_PAD(mob,1)          = str_dup(GET_PAD(ch,1));
      GET_PAD(mob,2)          = str_dup(GET_PAD(ch,2));
      GET_PAD(mob,3)          = str_dup(GET_PAD(ch,3));
      GET_PAD(mob,4)          = str_dup(GET_PAD(ch,4));
      GET_PAD(mob,5)          = str_dup(GET_PAD(ch,5));

      GET_STR(mob)       = GET_STR(ch);
      GET_INT(mob)       = GET_INT(ch);
      GET_WIS(mob)       = GET_WIS(ch);
      GET_DEX(mob)       = GET_DEX(ch);
      GET_CON(mob)       = GET_CON(ch);
      GET_CHA(mob)       = GET_CHA(ch);

      GET_LEVEL(mob)     = GET_LEVEL(ch);
      GET_HR(mob)        = GET_HR(ch);
      GET_AC(mob)        = GET_AC(ch);
      GET_DR(mob)        = GET_DR(ch);

      GET_MAX_HIT(mob)   = GET_MAX_HIT(ch);
      GET_HIT(mob)       = GET_MAX_HIT(ch);
      mob->mob_specials.damnodice   = 0;
      mob->mob_specials.damsizedice = 0;
      GET_GOLD(mob)      = 0;
      GET_GOLD_NoDs(mob) = 0;
      GET_GOLD_SiDs(mob) = 0;
      GET_EXP(mob)       = 0;

      GET_POS(mob)         = POS_STANDING;
      GET_DEFAULT_POS(mob) = POS_STANDING;
      GET_SEX(mob)         = GET_SEX(ch);

      GET_CLASS(mob)       = GET_CLASS(ch);
      GET_WEIGHT(mob)      = GET_WEIGHT(ch);
      GET_HEIGHT(mob)      = GET_HEIGHT(ch);
      GET_SIZE(mob)        = GET_SIZE(ch);
     }
   act(mag_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM);
   load_mtrigger(mob);
   add_follower(mob, ch);
   if (handle_corpse)
      {for (tobj = obj->contains; tobj; tobj = next_obj)
           {next_obj = tobj->next_content;
            obj_from_obj(tobj);
            obj_to_char(tobj, mob);
           }
       extract_obj(obj);
      }
}


void mag_points(int level, struct char_data * ch, struct char_data * victim,
		     int spellnum, int savetype)
{
  int hit = 0, move = 0;

  if (victim == NULL)
     return;

  switch (spellnum)
  {
  case SPELL_CURE_LIGHT:
    hit = dice(6, 3) + (level+2) / 3;
    send_to_char("Вы почувствовали себя немножко лучше.\r\n", victim);
    break;
  case SPELL_CURE_SERIOUS:
    hit = dice(25,2) + (level+2) / 3;
    send_to_char("Вы почувствовали себя намного лучше.\r\n", victim);
    break;
  case SPELL_CURE_CRITIC:
    hit = dice(45,2) + level;
    send_to_char("Вы почувствовали себя значительно лучше.\r\n", victim);
    break;
  case SPELL_HEAL:
    hit = GET_REAL_MAX_HIT(victim);
    send_to_char("Вы почувствовали себя здоровым.\r\n", victim);
    break;
  case SPELL_REFRESH:
    move = GET_REAL_MAX_MOVE(victim);
    send_to_char("Вы почувствовали себя полным сил.\r\n", victim);
    break;
  case SPELL_EXTRA_HITS:
    hit = dice(10,level/3) + level;
    send_to_char("По Вашему телу начала струиться живительная сила.\r\n",victim);
    break;
  case SPELL_FULL:
    if (!IS_NPC(victim))
       {GET_COND(victim, THIRST) = 24;
        GET_COND(victim, FULL)   = 24;
        send_to_char("Вы полноcтью насытились.\r\n", victim);
       }
  }

  hit = complex_spell_modifier(ch,spellnum,GAPPLY_SPELL_EFFECT,hit);

  if (hit && FIGHTING(victim) && ch != victim)
     {if (may_pkill(ch,FIGHTING(victim)) != PC_REVENGE_PC)
         inc_pkill_group(FIGHTING(victim),ch,1,0);
      else
         inc_pkill(FIGHTING(victim),ch,0,1);
     }

  if (spellnum == SPELL_EXTRA_HITS)
     GET_HIT(victim) = MIN(GET_HIT(victim)+hit, GET_REAL_MAX_HIT(victim) + GET_REAL_MAX_HIT(victim) * 33 / 100);
  else
  if (GET_HIT(victim) < GET_REAL_MAX_HIT(victim))
     GET_HIT(victim) = MIN(GET_HIT(victim)+hit, GET_REAL_MAX_HIT(victim));

  GET_MOVE(victim) = MIN(GET_REAL_MAX_MOVE(victim), GET_MOVE(victim) + move);
  update_pos(victim);
}


void mag_unaffects(int level, struct char_data * ch, struct char_data * victim,
		        int spellnum, int type)
{ struct affected_type *hjp;
  int    spell = 0, remove = 0;
  const  char *to_vict = NULL, *to_room = NULL;

  if (victim == NULL)
    return;

  switch (spellnum)
  {
  case SPELL_HEAL:
    return;
  case SPELL_CURE_BLIND:
    spell   = SPELL_BLINDNESS;
    to_vict = "К Вам вернулась способность видеть.";
    to_room = "$n прозрел$g.";
    break;
  case SPELL_REMOVE_POISON:
    spell   = SPELL_POISON;
    to_vict = "Тепло заполнило Ваше тело.";
    to_room = "$n выглядит лучше.";
    break;
  case SPELL_CURE_PLAQUE:
    spell   = SPELL_PLAQUE;
    to_vict = "Лихорадка прекратилась.";
    break;
  case SPELL_REMOVE_CURSE:
    spell   = SPELL_CURSE;
    to_vict = "Боги вернули Вам свое покровительство.";
    break;
  case SPELL_REMOVE_HOLD:
    spell   = SPELL_HOLD;
    to_vict = "К Вам вернулась способность двигаться.";
    break;
  case SPELL_REMOVE_SIELENCE:
    spell   = SPELL_SIELENCE;
    to_vict = "К Вам вернулась способность разговаривать.";
    break;
  case SPELL_DISPELL_MAGIC:
    if (!IS_NPC(ch) && !same_group(ch,victim))
       {send_to_char("Только на себя или одногруппника !\r\n",ch);
        return;
       }
    for (spell=0, hjp = victim->affected; hjp; hjp = hjp->next,spell++);
    if (!spell)
       {send_to_char(NOEFFECT,ch);
        return;
       }
    spell = number(1,spell);
    for (hjp=victim->affected; spell > 1 && hjp; hjp = hjp->next,spell--);
    if (!hjp                           || 
        hjp->bitvector == AFF_CHARM    || 
        !spell_info[hjp->type].name    ||
        *spell_info[hjp->type].name == '!')
       {send_to_char(NOEFFECT,ch);
        return;
       }
       
    spell  = hjp->type;
    remove = TRUE;
    break;
  default:
    log("SYSERR: unknown spellnum %d passed to mag_unaffects.", spellnum);
    return;
  }

  if (!affected_by_spell(victim, spell))
     {if (spellnum != SPELL_HEAL)		/* 'cure blindness' message. */
         send_to_char(NOEFFECT, ch);
      return;
     }
  spellnum = spell;
  if (ch != victim && !remove)
     {if (IS_SET(SpINFO.routines, NPC_AFFECT_NPC))
         {if (may_pkill(ch,victim) != PC_REVENGE_PC)
             inc_pkill_group(victim,ch,1,0);
          else
             inc_pkill(victim,ch,0,1);
         }
      else
      if (IS_SET(SpINFO.routines, NPC_AFFECT_PC) && FIGHTING(victim))
         {if (may_pkill(ch,FIGHTING(victim)) != PC_REVENGE_PC)
             inc_pkill_group(FIGHTING(victim),ch,1,0);
          else
             inc_pkill(FIGHTING(victim),ch,0,1);
         }
     }
  affect_from_char(victim, spell);
  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}


void mag_alter_objs(int level, struct char_data * ch, struct obj_data * obj,
		            int spellnum, int savetype)
{
  const char *to_char = NULL, *to_room = NULL;

  if (obj == NULL)
    return;

  switch (spellnum)
  { case SPELL_BLESS:
      if (!IS_OBJ_STAT(obj, ITEM_BLESS) &&
          (GET_OBJ_WEIGHT(obj) <= 5 * GET_LEVEL(ch))
         )
	 {SET_BIT(GET_OBJ_EXTRA(obj, ITEM_BLESS), ITEM_BLESS);
	  if (IS_OBJ_STAT(obj,ITEM_NODROP))
	     {REMOVE_BIT(GET_OBJ_EXTRA(obj, ITEM_NODROP), ITEM_NODROP);
              if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
                 GET_OBJ_VAL(obj, 2)++;
             }
          GET_OBJ_MAX(obj) += MAX(GET_OBJ_MAX(obj) >> 2, 1);
	  GET_OBJ_CUR(obj)  = GET_OBJ_MAX(obj);
	  to_char = "$o вспыхнул$G голубым светом и тут же погас$Q.";
         }
      break;
    case SPELL_CURSE:
      if (!IS_OBJ_STAT(obj, ITEM_NODROP))
         {SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NODROP), ITEM_NODROP);
	  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
             {if (GET_OBJ_VAL(obj,2) > 0)
                 GET_OBJ_VAL(obj, 2)--;
             }
          else
          if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
             {GET_OBJ_VAL(obj,0)--;
              GET_OBJ_VAL(obj,1)--;
             }
	  to_char = "$o вспыхнул$G красным светом и тут же погас$Q.";
         }
      break;
    case SPELL_INVISIBLE:
      if (!IS_OBJ_STAT(obj, ITEM_NOINVIS) && !IS_OBJ_STAT(obj, ITEM_INVISIBLE))
         {SET_BIT(GET_FLAG(obj->obj_flags.extra_flags,ITEM_INVISIBLE), ITEM_INVISIBLE);
          to_char = "$o растворил$U в пустоте.";
         }
      break;
    case SPELL_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
           (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
           (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, 3))
           {GET_OBJ_VAL(obj, 3) = 1;
            to_char = "$o отравлен$G.";
           }
      break;
    case SPELL_REMOVE_CURSE:
      if (IS_OBJ_STAT(obj,ITEM_NODROP))
         {REMOVE_BIT(GET_OBJ_EXTRA(obj, ITEM_NODROP), ITEM_NODROP);
          if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
             GET_OBJ_VAL(obj, 2)++;
          to_char = "$o вспыхнул$G розовым светом и тут же погас$Q.";
         }
      break;
    case SPELL_REMOVE_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
           (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
           (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && GET_OBJ_VAL(obj, 3))
         {GET_OBJ_VAL(obj, 3) = 0;
          to_char = "$o стал$G вполне пригодным к применению.";
         }
      break;
    case SPELL_ACID:
      alterate_object(obj,number(GET_LEVEL(ch)*2,GET_LEVEL(ch)*4),100);
      break;
    case SPELL_REPAIR:
      GET_OBJ_CUR(obj) = GET_OBJ_MAX(obj);
      to_char = "Вы полностью восстановили $o3.";
      break;
  }

  if (to_char == NULL)
    send_to_char(NOEFFECT, ch);
  else
    act(to_char, TRUE, ch, obj, 0, TO_CHAR);

  if (to_room != NULL)
    act(to_room, TRUE, ch, obj, 0, TO_ROOM);
/*
  else
  if (to_char != NULL)
    act(to_char, TRUE, ch, obj, 0, TO_ROOM);
 */
}



void mag_creations(int level, struct char_data * ch, int spellnum)
{
  struct obj_data *tobj;
  obj_vnum z;

  if (ch == NULL)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1); - Hm, not used. */

  switch (spellnum)
  {
  case SPELL_CREATE_FOOD:
    z = START_BREAD;
    break;
  case SPELL_CREATE_LIGHT:
    z = CREATE_LIGHT;
    break;
  default:
    send_to_char("Spell unimplemented, it would seem.\r\n", ch);
    return;
  }

  if (!(tobj = read_object(z, VIRTUAL)))
     {send_to_char("Что-то не видно образа для создания.\r\n", ch);
      log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
	      spellnum, z);
      return;
     }
  act("$n создал$g $o3.", FALSE, ch, tobj, 0, TO_ROOM);
  act("Вы создали $o3.", FALSE, ch, tobj, 0, TO_CHAR);
  load_otrigger(tobj);

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
     {send_to_char("Вы не сможете унести столько предметов.\r\n", ch);
      obj_to_room(tobj, IN_ROOM(ch));
     }
  else
  if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(tobj) > CAN_CARRY_W(ch))
     {send_to_char("Вы не сможете унести такой вес.\r\n", ch);
      obj_to_room(tobj, IN_ROOM(ch));
     }
  else
     obj_to_char(tobj, ch);
}