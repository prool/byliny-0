/* ************************************************************************
*  File: db.script.c                             Part of Death's Gate MUD *
*                                                                         *
*  Usage: Contains routines to handle db functions for scripts and trigs  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: egreen $
*  $Date: 1996/09/30 21:27:54 $
*  $Revision: 3.7 $
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "dg_event.h"
#include "comm.h"
#include "spells.h"

void trig_data_copy(trig_data *this_data, const trig_data *trg);
void trig_data_free(trig_data *this_data);

extern struct index_data **trig_index;
extern int top_of_trigt;

extern struct index_data *mob_index;
extern struct index_data *obj_index;

extern void half_chop(char *string, char *arg1, char *arg2);
extern void asciiflag_conv(char *flag, void *value);

int check_recipe_items(struct char_data * ch, int spellnum, int spelltype, int extract);
int check_recipe_values(struct char_data * ch, int spellnum, int spelltype, int showrecipe);

void parse_trigger(FILE *trig_f, int nr)
{
    int t[2], k, attach_type;
    char line[256], *cmds, *s, flags[256];
    struct cmdlist_element *cle;
    index_data *index;
    trig_data *trig;

    CREATE(trig, trig_data, 1);
    CREATE(index, index_data, 1);

    index->vnum = nr;
    index->number = 0;
    index->func = NULL;
    index->proto = trig;

    sprintf(buf2, "trig vnum %d", nr);

    trig->nr = top_of_trigt;
    trig->name = fread_string(trig_f, buf2);

    get_line(trig_f, line);
    k = sscanf(line, "%d %s %d", &attach_type, flags, t);
    trig->attach_type = (byte)attach_type;
    asciiflag_conv(flags, &trig->trigger_type);
    trig->narg = (k == 3) ? t[0] : 0;

    trig->arglist = fread_string(trig_f, buf2);
  
    cmds = s = fread_string(trig_f, buf2);

    CREATE(trig->cmdlist, struct cmdlist_element, 1);
    trig->cmdlist->cmd = str_dup(strtok(s, "\n\r"));
    cle = trig->cmdlist;

    while ((s = strtok(NULL, "\n\r"))) 
          {CREATE(cle->next, struct cmdlist_element, 1);
	       cle = cle->next;
	       cle->cmd = str_dup(s);
          }

    free(cmds);

    trig_index[top_of_trigt++] = index;
}


/*
 * create a new trigger from a prototype.
 * nr is the real number of the trigger.
 */
trig_data *read_trigger(int nr)
{
    index_data *index;
    trig_data *trig;

    if (nr >= top_of_trigt) return NULL;
    if ((index = trig_index[nr]) == NULL)
	return NULL;

    CREATE(trig, trig_data, 1);
    trig_data_copy(trig, index->proto);

    index->number++;

    return trig;
}


/* release memory allocated for a variable list */
void free_varlist(struct trig_var_data *vd)
{
    struct trig_var_data *i, *j;

    for (i = vd; i;) 
        {j = i;
	     i = i->next;
	     if (j->name)
	        free(j->name);
	     if (j->value)
	        free(j->value);
	     free(j);
        }
}


/* release memory allocated for a script */
void free_script(struct script_data *sc)
{
    trig_data *t1, *t2;

    for (t1 = TRIGGERS(sc); t1 ;) 
        {t2 = t1;
	     t1 = t1->next;
	     trig_data_free(t2);
        }

    free_varlist(sc->global_vars);

    free(sc);
}

void trig_data_init(trig_data *this_data)
{
    this_data->nr           = NOTHING;
    this_data->data_type    = 0;
    this_data->name         = NULL;
    this_data->trigger_type = 0;
    this_data->cmdlist      = NULL;
    this_data->curr_state   = NULL;
    this_data->narg         = 0;
    this_data->arglist      = NULL;
    this_data->depth        = 0;
    this_data->wait_event   = NULL;
    this_data->purged       = FALSE;
    this_data->var_list     = NULL;

    this_data->next = NULL;  
}


void trig_data_copy(trig_data *this_data, const trig_data *trg)
{
    trig_data_init(this_data);

    this_data->nr           = trg->nr;
    this_data->attach_type  = trg->attach_type;
    this_data->data_type    = trg->data_type;
    this_data->name         = str_dup(trg->name);
    this_data->trigger_type = trg->trigger_type;
    this_data->cmdlist      = trg->cmdlist;
    this_data->narg         = trg->narg;
    if (trg->arglist) 
       this_data->arglist   = str_dup(trg->arglist);
}


void trig_data_free(trig_data *this_data)
{
/*    struct cmdlist_element *i, *j;*/

    free(this_data->name);

    /*
     * The command list is a memory leak right now!
     *
    if (cmdlist != trigg->cmdlist || this_data->proto)
	   for (i = cmdlist; i;) 
	       {j = i;
	        i = i->next;
	        free(j->cmd);
	        free(j);
	       }
	*/

    free(this_data->arglist);
  
    free_varlist(this_data->var_list);

    if (this_data->wait_event)
	   remove_event(this_data->wait_event);

    free(this_data);
}

/* for mobs and rooms: */
void dg_read_trigger(FILE *fp, void *proto, int type)
{
  char line[256];
  char junk[8];
  int vnum, rnum, count;
  char_data *mob;
  room_data *room;
  struct trig_proto_list *trg_proto, *new_trg;

  get_line(fp, line);
  count = sscanf(line,"%s %d",junk,&vnum);

  if (count != 2) 
     {/* should do a better job of making this message */
      log("SYSERR: Error assigning trigger!");
      return;
     }

  rnum = real_trigger(vnum);
  if (rnum<0) 
     {sprintf(line,"SYSERR: Trigger vnum #%d asked for but non-existant!", vnum);
      log(line);
      return;
     }

  switch(type) 
     {
    case MOB_TRIGGER:
      CREATE(new_trg, struct trig_proto_list, 1);
      new_trg->vnum = vnum;
      new_trg->next = NULL;

      mob = (char_data *)proto;
      trg_proto = mob->proto_script;
      if (!trg_proto) 
         {mob->proto_script = trg_proto = new_trg;
         } 
      else 
         {while (trg_proto->next) 
                trg_proto = trg_proto->next;
          trg_proto->next = new_trg;
         }
      break;
    case WLD_TRIGGER:
      CREATE(new_trg, struct trig_proto_list, 1);
      new_trg->vnum = vnum;
      new_trg->next = NULL;
      room = (room_data *)proto;
      trg_proto = room->proto_script;
      if (!trg_proto) 
         {room->proto_script = trg_proto = new_trg;
         } 
      else 
         {while (trg_proto->next) 
                trg_proto = trg_proto->next;
          trg_proto->next = new_trg;
         }

      if (rnum>=0) 
         {if (!(room->script))
             CREATE(room->script, struct script_data, 1);
          add_trigger(SCRIPT(room), read_trigger(rnum), -1);
         } 
      else 
         {sprintf(line,"SYSERR: non-existant trigger #%d assigned to room #%d",
                  vnum, room->number);
          log(line);
         }
      break;
    default:
      sprintf(line,"SYSERR: Trigger vnum #%d assigned to non-mob/obj/room",
              vnum);
      log(line);
      }
}

void dg_obj_trigger(char *line, struct obj_data *obj)
{
  char junk[8];
  int vnum, rnum, count;
  struct trig_proto_list *trg_proto, *new_trg;

  count = sscanf(line,"%s %d",junk,&vnum);

  if (count != 2) 
     {/* should do a better job of making this message */
      log("SYSERR: Error assigning trigger!");
      return;
     }

  rnum = real_trigger(vnum);
  if (rnum<0) 
     {sprintf(line,"SYSERR: Trigger vnum #%d asked for but non-existant!", vnum);
      log(line);
      return;
     }

  CREATE(new_trg, struct trig_proto_list, 1);
  new_trg->vnum = vnum;
  new_trg->next = NULL;

  trg_proto = obj->proto_script;
  if (!trg_proto) 
     {obj->proto_script = trg_proto = new_trg;
     } 
  else 
     {while (trg_proto->next) 
            trg_proto = trg_proto->next;
      trg_proto->next = new_trg;
     }
}

void assign_triggers(void *i, int type)
{
  char_data *mob;
  obj_data *obj;
  struct room_data *room;
  int rnum;
  char buf[256];
  struct trig_proto_list *trg_proto;

  switch (type)
  {
    case MOB_TRIGGER:
      mob = (char_data *)i;
      trg_proto = mob->proto_script;
      while (trg_proto) 
            {rnum = real_trigger(trg_proto->vnum);
             if (rnum==-1) 
                {sprintf(buf,"SYSERR: trigger #%d non-existant, for mob #%d",
                         trg_proto->vnum, mob_index[mob->nr].vnum);
                 log(buf);
                } 
             else 
                {if (!SCRIPT(mob))
                    CREATE(SCRIPT(mob), struct script_data, 1);
                 add_trigger(SCRIPT(mob), read_trigger(rnum), -1);
                }
             trg_proto = trg_proto->next;
            }
      break;
    case OBJ_TRIGGER:
      obj = (obj_data *)i;
      trg_proto = obj->proto_script;
      while (trg_proto) 
            {rnum = real_trigger(trg_proto->vnum);
             if (rnum==-1) 
                {sprintf(buf,"SYSERR: trigger #%d non-existant, for obj #%d",
                         trg_proto->vnum, obj_index[obj->item_number].vnum);
                 log(buf);
                } 
             else 
                {if (!SCRIPT(obj))
                    CREATE(SCRIPT(obj), struct script_data, 1);
                 add_trigger(SCRIPT(obj), read_trigger(rnum), -1);
                }
             trg_proto = trg_proto->next;
            }
      break;
    case WLD_TRIGGER:
      room = (struct room_data *)i;
      trg_proto = room->proto_script;
      while (trg_proto) 
            {rnum = real_trigger(trg_proto->vnum);
             if (rnum==-1) 
                {sprintf(buf,"SYSERR: trigger #%d non-existant, for room #%d",
                         trg_proto->vnum, room->number);
                 log(buf);
                } 
             else 
                {if (!SCRIPT(room))
                     CREATE(SCRIPT(room), struct script_data, 1);
                 add_trigger(SCRIPT(room), read_trigger(rnum), -1);
                }
             trg_proto = trg_proto->next;
            }
      break;
    default:
      log("SYSERR: unknown type for assign_triggers()");
      break;
  }
}

void trg_skillturn(struct char_data *ch, int skillnum, int skilldiff)
{if (GET_SKILL(ch, skillnum))
    {if (skilldiff)
        return;
     else
        {sprintf(buf,"��� ������ ������ '%s'.\r\n", skill_name(skillnum));
         send_to_char(buf,ch);
         GET_SKILL(ch, skillnum) = 0;
        }
     }    
  else
     {if (skilldiff)      
         {sprintf(buf,"�� ������� ������ '%s'.\r\n", skill_name(skillnum));
          send_to_char(buf,ch);
          GET_SKILL(ch, skillnum) = 5;
         };
     }    
}

void trg_skilladd(struct char_data *ch, int skillnum, int skilldiff)
{int skill = GET_SKILL(ch, skillnum);
 GET_SKILL(ch, skillnum) = MAX(1, MIN(GET_SKILL(ch, skillnum) + 
                               skilldiff, 200));
 if (skill > GET_SKILL(ch, skillnum))
    {sprintf(buf,"���� ������ '%s' ����������.\r\n", skill_name(skillnum));
     send_to_char(buf,ch);
    }
 else   
 if (skill < GET_SKILL(ch, skillnum))
    {sprintf(buf,"�� �������� ���� ������ '%s'.\r\n", skill_name(skillnum));
     send_to_char(buf,ch);
    }
 else
    {sprintf(buf,"���� ������ �������� ���������� '%s '.\r\n"
         ,skill_name(skillnum));
     send_to_char(buf,ch);
    }    
}

void trg_spellturn(struct char_data *ch, int spellnum, int spelldiff)
{int spell = GET_SPELL_TYPE(ch, spellnum);

 if (spell & SPELL_KNOW)
    {if (spelldiff)
        return;
     else
        {sprintf(buf,"�� ������� ������ ���������� '%s'.\r\n", spell_name(spellnum));
         send_to_char(buf,ch);
         REMOVE_BIT(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW);
         if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_TEMP))
            GET_SPELL_MEM(ch, spellnum) = 0;
        }
    }    
 else
    {if (spelldiff)
        {sprintf(buf,"�� �������� ���������� '%s'.\r\n", spell_name(spellnum));
         send_to_char(buf,ch);
         SET_BIT(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW);
        };
    }
}

void trg_spelladd(struct char_data *ch, int spellnum, int spelldiff)
{int spell = GET_SPELL_MEM(ch, spellnum);
 GET_SPELL_MEM(ch, spellnum) = MAX(0, MIN(spell + spelldiff,50));
      
 if (spell > GET_SPELL_MEM(ch, spellnum))
    {if (GET_SPELL_MEM(ch, spellnum))
        {sprintf(buf,"�� ������ ����� ���������� '%s'.\r\n", 
                 spell_name(spellnum));
        }
     else
        {sprintf(buf,"�� ������ ��� ���������� '%s'.\r\n", 
                     spell_name(spellnum));
         REMOVE_BIT(GET_SPELL_TYPE(ch,spellnum), SPELL_TEMP);            
        }             
     send_to_char(buf,ch);
    }
 else
 if (spell < GET_SPELL_MEM(ch, spellnum))
    {if (!IS_SET(GET_SPELL_TYPE(ch,spellnum), SPELL_KNOW))
         SET_BIT(GET_SPELL_TYPE(ch,spellnum), SPELL_TEMP);
     sprintf(buf,"�� ������� ��������� ���������� '%s'.\r\n", 
             spell_name(spellnum));
     send_to_char(buf,ch);           
    }
}

void trg_spellitem(struct char_data *ch, int spellnum, int spelldiff, int spell)
{char type [MAX_STRING_LENGTH];

 if ((spelldiff && IS_SET(GET_SPELL_TYPE(ch, spellnum), spell)) ||
     (!spelldiff && !IS_SET(GET_SPELL_TYPE(ch, spellnum), spell)))
    return;
 if (!spelldiff)
    {REMOVE_BIT(GET_SPELL_TYPE(ch,spellnum), spell);
     switch (spell)
            {
            case SPELL_SCROLL:
            strcpy(type,"�������� ������"); 
            break;
            case SPELL_POTION:
            strcpy(type,"������������� �������"); 
            break;
            case SPELL_WAND:
            strcpy(type,"������������ ������"); 
            break;
            case SPELL_ITEMS:
            strcpy(type,"���������� �����"); 
            break;
            case SPELL_RUNES:
            strcpy(type,"������������� ���");
            break;
               };
     sprintf(buf,"�� �������� ������ %s '%s'", type, spell_name(spellnum));
    }
 else
    {SET_BIT(GET_SPELL_TYPE(ch, spellnum), spell);   
     switch (spell)
            {
            case SPELL_SCROLL:
            if (!GET_SKILL(ch, SKILL_CREATE_SCROLL))
               SET_SKILL(ch,SKILL_CREATE_SCROLL,5);
            strcpy(type,"�������� ������"); 
            break;
            case SPELL_POTION:
            if (!GET_SKILL(ch, SKILL_CREATE_POTION))
               SET_SKILL(ch,SKILL_CREATE_POTION,5);
            strcpy(type,"������������� �������"); 
            break;
            case SPELL_WAND:
            if (!GET_SKILL(ch, SKILL_CREATE_WAND))
               SET_SKILL(ch,SKILL_CREATE_WAND,5);            
            strcpy(type,"������������ ������"); 
            break;
            case SPELL_ITEMS:
            strcpy(type,"���������� �����"); 
            break;
            case SPELL_RUNES:
            strcpy(type,"������������� ���");
            break;
            };
     sprintf(buf,"�� ��������� ������ %s '%s'", type, spell_name(spellnum));
     send_to_char(buf,ch);
     check_recipe_items(ch,spellnum,spell,TRUE);
    }
}
