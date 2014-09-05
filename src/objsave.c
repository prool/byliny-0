/* ************************************************************************
*   File: objsave.c                                     Part of CircleMUD *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * AutoEQ by Burkhard Knopf <burkhard.knopf@informatik.tu-clausthal.de>
 */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"

/* these factors should be unique integers */
#define RENT_FACTOR 	1
#define CRYO_FACTOR 	4

#define LOC_INVENTORY	0
#define MAX_BAG_ROWS	5
#define ITEM_DESTROYED  100

extern struct room_data *world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern struct obj_data *obj_proto;
extern int top_of_p_table;
extern int rent_file_timeout, crash_file_timeout;
extern int free_rent;
extern int max_obj_save;	/* change in config.c */
extern long last_rent_check;
extern room_rnum r_helled_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;

#define MAKESIZE(number) (sizeof(struct save_info) + sizeof(struct save_time_info) * number)
#define SAVEINFO(number) ((player_table+number)->timer)
#define SAVESIZE(number) (sizeof(struct save_info) +\
                          sizeof(struct save_time_info) * (player_table+number)->timer->rent.nitems)

/* Extern functions */
ACMD(do_tell);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
int    invalid_no_class(struct char_data *ch, struct obj_data *obj);
int    invalid_anti_class(struct char_data *ch, struct obj_data *obj);
int    invalid_unique(struct char_data *ch, struct obj_data *obj);
int    min_rent_cost(struct char_data *ch);
void   name_from_drinkcon(struct obj_data * obj);
void   name_to_drinkcon(struct obj_data * obj, int type);
int    delete_char(char *name);
struct obj_data *create_obj(void);
void   asciiflag_conv(char *flag, void *value);
void   tascii(int *pointer, int num_planes, char *ascii);
int    get_ptable_by_name(char *name);


/* local functions */
void Crash_extract_norent_eq(struct char_data *ch);
int  auto_equip(struct char_data *ch, struct obj_data *obj, int location);
int Crash_offer_rent(struct char_data * ch, struct char_data * receptionist, int display, int factor, int *totalcost);
int Crash_report_unrentables(struct char_data * ch, struct char_data * recep, struct obj_data * obj);
void Crash_report_rent(struct char_data * ch, struct char_data * recep, struct obj_data * obj, int *cost, long *nitems, int display, int factor, int equip);
struct obj_data *Obj_from_store(struct obj_file_elem object, int *location);
int Obj_to_store(struct obj_data * obj, FILE * fl, int location);
void update_obj_file(void);
int  Crash_write_rentcode(struct char_data * ch, FILE * fl, struct save_rent_info * rent);
int  gen_receptionist(struct char_data * ch, struct char_data * recep, int cmd, char *arg, int mode);
int  Crash_save(int iplayer, struct obj_data * obj, int location);
void Crash_rent_deadline(struct char_data * ch, struct char_data * recep, long cost);
void Crash_restore_weight(struct obj_data * obj);
void Crash_extract_objs(struct obj_data * obj);
int  Crash_is_unrentable(struct obj_data * obj);
void Crash_extract_norents(struct obj_data * obj);
void Crash_extract_expensive(struct obj_data * obj);
void Crash_calculate_rent(struct obj_data * obj, int *cost);
void Crash_rentsave(struct char_data * ch, int cost);
void Crash_cryosave(struct char_data * ch, int cost);


#define DIV_CHAR  '#'
#define END_CHAR  '$'
#define END_LINE  '\n'
#define END_LINES '~'
#define COM_CHAR  '*'

int get_buf_line(char **source, char *target)
{char *otarget = target;
 int  empty = TRUE;

 *target = '\0';
 for (;
      **source && **source != DIV_CHAR && **source != END_CHAR;
      (*source)++
     )
     {if (**source == END_LINE)
         {if (empty || *otarget == COM_CHAR)
             {target  = otarget;
              *target = '\0';
              continue;
             }
          (*source)++;
          return (TRUE);
         }
      *target = **source;
      if (!isspace(*target++))
         empty = FALSE;
      *target = '\0';
     }
 return (FALSE);
}

int get_buf_lines(char **source, char *target)
{
 *target = '\0';

  for (;
       **source && **source != DIV_CHAR && **source != END_CHAR;
       (*source)++
      )
      {if (**source == END_LINES)
          {(*source)++;
           if (**source == END_LINE)
              (*source)++
;
           return (TRUE);
          }
       *(target++) = **source;
       *target = '\0';
      }
 return (FALSE);
}

char *str_copy(char *source)
{char *target=NULL;
 if (!source || !*source)
    {CREATE(target,char,1);
     *target = '\0';
    }
 else
    {CREATE(target,char,strlen(source)+1);
     strcpy(target,source)
;
    }
 return (target);
}

#define NUM_OF_PAD 6

// Данная процедура выбирает предмет из буфера
struct obj_data *read_one_object(char **data, int *error)
{ char   buffer[MAX_INPUT_LENGTH], f0[MAX_STRING_LENGTH],
         f1[MAX_STRING_LENGTH], f2[MAX_STRING_LENGTH];
  int    vnum, i, j, t[5];
  struct obj_data *object = NULL;
  struct extra_descr_data *new_descr;

  *error = TRUE;
  // Станем на начало предмета
  for(;**data != DIV_CHAR; (*data)++)
     if (!**data || **data == END_CHAR)
        return (object);

  // Пропустим #
  (*data)++;
  // Считаем vnum предмета
  if (!get_buf_line(data,buffer))
     return (object);
  if (!(vnum = atoi(buffer)))
     return (object);

  if (vnum < 0)
     {// Предмет не имеет прототипа
      object = create_obj();
      GET_OBJ_RNUM(object) = NOTHING;
      if (!get_buf_lines(data,buffer))
         return (object);
      // Алиасы
      GET_OBJ_ALIAS(object) = str_copy(buffer)
;
      object->name          = str_copy(buffer);
      // Падежи
      for (i=0; i < NUM_OF_PAD; i++)
          {if (!get_buf_lines(data,buffer))
              return (object);
           GET_OBJ_PNAME(object,i) = str_copy(buffer);
          }
      // Описание когда на земле
      if (!get_buf_lines(data,buffer))
         return (object);
      GET_OBJ_DESC(object) = str_copy(buffer);
      // Описание при действии
      if (!get_buf_lines(data,buffer))
         return (object);
      GET_OBJ_ACT(object) = str_copy(buffer);
     }
  else
  if (!(object = read_object(vnum, VIRTUAL)))
     return (object);
  else     
     obj_index[object->item_number].stored--;

  if (!get_buf_line(data, buffer) ||
      sscanf(buffer, " %s %d %d %d", f0, t+1, t+2, t+3) != 4
     )
     return (object);
  asciiflag_conv(f0, &GET_OBJ_SKILL(object));
  GET_OBJ_MAX(object)   = t[1];
  GET_OBJ_CUR(object)   = t[2];
  GET_OBJ_MATER(object) = t[3];

  if (!get_buf_line(data, buffer) ||
       sscanf(buffer, " %d %d %d %d", t, t+1, t+2, t+3) != 4
     )
     return (object);
  GET_OBJ_SEX(object)   = t[0];
  GET_OBJ_TIMER(object) = t[1];
  GET_OBJ_SPELL(object) = t[2];
  GET_OBJ_LEVEL(object) = t[3];

  if (!get_buf_line(data, buffer) ||
      sscanf(buffer, " %s %s %s", f0, f1, f2) != 3
     )
     return (object);
  asciiflag_conv(f0, &GET_OBJ_AFFECTS(object));
  asciiflag_conv(f1, &GET_OBJ_ANTI(object));
  asciiflag_conv(f2, &GET_OBJ_NO(object));

  if (!get_buf_line(data,buffer) ||
      sscanf(buffer, " %d %s %s", t, f1, f2) != 3
     )
     return (object);
  GET_OBJ_TYPE(object) = t[0];
  asciiflag_conv(f1, &GET_OBJ_EXTRA(object,0));
  asciiflag_conv(f2, &GET_OBJ_WEAR(object));

  if (!get_buf_line(data,buffer) ||
      sscanf(buffer, "%s %d %d %d", f0, t + 1, t + 2, t + 3) != 4
     )
     return (object);
  asciiflag_conv(f0,&GET_OBJ_VAL(object,0));
  GET_OBJ_VAL(object,1) = t[1];
  GET_OBJ_VAL(object,2) = t[2];
  GET_OBJ_VAL(object,3) = t[3];

  if (!get_buf_line(data,buffer) ||
      sscanf(buffer, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4
     )
     return (object);
  GET_OBJ_WEIGHT(object)   = t[0];
  GET_OBJ_COST(object)     = t[1];
  GET_OBJ_RENT(object)     = t[2];
  GET_OBJ_RENTEQ(object)   = t[3];

  if (!get_buf_line(data,buffer) ||
      sscanf(buffer, "%d %d", t, t + 1) != 2
     )
     return (object);
  object->worn_on       = t[0];
  GET_OBJ_OWNER(object) = t[1];

  // Проверить вес фляг и т.п.
  if (GET_OBJ_TYPE(object) == ITEM_DRINKCON ||
      GET_OBJ_TYPE(object) == ITEM_FOUNTAIN
     )
    {if (GET_OBJ_WEIGHT(object) < GET_OBJ_VAL(object,1))
        GET_OBJ_WEIGHT(object) = GET_OBJ_VAL(object,1) + 5;
    }

  object->ex_description = NULL; // Exlude doubling ex_description !!!
  *error                 = FALSE;
  j                      = 0;

  for (;;)
     {if (!get_buf_line(data, buffer))
         return (object);
      switch (*buffer)
         { case 'E':
                    CREATE(new_descr, struct extra_descr_data, 1);
                    if (!get_buf_lines(data, buffer))
                       {free(new_descr);
                        return (object);
                       }
                    new_descr->keyword = str_copy(buffer);
                    if (!get_buf_lines(data, buffer))
                       {free(new_descr->keyword);
                        free(new_descr);
                        return (object);
                       }
                    new_descr->description = str_copy(buffer);
                    new_descr->next = object->ex_description;
                    object->ex_description = new_descr;
                    break;
            case 'A':
                    if (j >= MAX_OBJ_AFFECT)
                       return (object);
                    if (!get_buf_line(data,buffer))
                       return (object)
;
                    if (sscanf(buffer, " %d %d ", t, t + 1) == 2)
                       {object->affected[j].location = t[0];
                        object->affected[j].modifier = t[1];
                        j++;
                       }
                    break;
             default:
                    break;
         }
     }
  return (object);
}

// Данная процедура помещает предмет в буфер
void write_one_object(char **data, struct obj_data *object, int location)
{ char   buf[MAX_STRING_LENGTH];
  struct extra_descr_data *descr;
  int    count = 0, i, j;

  count += sprintf(*data+count,"#%d\n",GET_OBJ_VNUM(object));
  if (GET_OBJ_VNUM(object) < 0)
     {// Предмет не имеет прототипа
      // Алиасы
      count += sprintf(*data+count,"%s~\n",GET_OBJ_ALIAS(object));
      // Падежи
      for (i=0; i<NUM_OF_PAD;i++)
           count += sprintf(*data+count,"%s~\n",GET_OBJ_PNAME(object,i));
      // Описание когда на земле
      count += sprintf(*data+count,"%s~\n",
                       GET_OBJ_DESC(object) ? GET_OBJ_DESC(object) : "");
      // Описание при действии
      count += sprintf(*data+count,"%s~\n",
                       GET_OBJ_ACT(object) ? GET_OBJ_ACT(object) : "");
     }

  count += sprintf(*data+count,"%d %d %d %d\n",
                   GET_OBJ_SKILL(object),
                   GET_OBJ_MAX(object),
                   GET_OBJ_CUR(object),
                   GET_OBJ_MATER(object));
  count += sprintf(*data+count,"%d %d %d %d\n",
                   GET_OBJ_SEX(object),
                   GET_OBJ_TIMER(object),
                   GET_OBJ_SPELL(object),
                   GET_OBJ_LEVEL(object));
  *buf = '\0';
  tascii((int *)&GET_OBJ_AFFECTS(object),4,buf);
  tascii((int *)&GET_OBJ_ANTI(object),4,buf);
  tascii((int *)&GET_OBJ_NO(object),4,buf);
  count += sprintf(*data+count,"%s\n",
                   buf);
  *buf = '\0';
  tascii(&GET_OBJ_EXTRA(object,0),4,buf);
  tascii(&GET_OBJ_WEAR(object),4,buf);
  count += sprintf(*data+count,"%d %s\n",
                   GET_OBJ_TYPE(object),
		   buf);
  count += sprintf(*data+count,"%d %d %d %d\n",
                   GET_OBJ_VAL(object,0),
                   GET_OBJ_VAL(object,1),
                   GET_OBJ_VAL(object,2),
                   GET_OBJ_VAL(object,3));
  count += sprintf(*data+count,"%d %d %d %d\n",
                   GET_OBJ_WEIGHT(object),
                   GET_OBJ_COST(object),
                   GET_OBJ_RENT(object),
                   GET_OBJ_RENTEQ(object));
  count += sprintf(*data+count,"%d %d\n",
                   location,
                   GET_OBJ_OWNER(object));
  for (descr = object->ex_description; descr; descr = descr->next)
      count += sprintf(*data+count,"E\n%s~\n%s~\n",
                       descr->keyword ? descr->keyword : "",
                       descr->description
 ? descr->description : ""
                      );
  for (j=0; j < MAX_OBJ_AFFECT;j++)
      if (object->affected[j].location)
         count += sprintf(*data+count,"A\n%d %d\n",
                           object->affected[j].location,
                           object->affected[j].modifier
                         );
  *data += count;
  **data = '\0';
}



struct obj_data *Obj_from_store(struct obj_file_elem object, int *location)
{ struct obj_data *obj;
  int j;

  *location = 0;
  if ((j = real_object(object.item_number)) >= 0)
     {if (object.location == ITEM_DESTROYED)
         return (NULL);
      if (obj_index[j].stored <= 0)
         {log("ATTEMPT load object, which number grater than stored ...");
          return (NULL);
         }
      obj = read_object(object.item_number, VIRTUAL);
      if (!obj)
         return (NULL);
      obj_index[j].stored--;
      if (object.timer <= 0)
         {extract_obj(obj);
          return (NULL);
         }
      *location                   = object.location;
      GET_OBJ_VAL(obj, 0)         = object.value[0];
      GET_OBJ_VAL(obj, 1)         = object.value[1];
      GET_OBJ_VAL(obj, 2)         = object.value[2];
      GET_OBJ_VAL(obj, 3)         = object.value[3];
      GET_OBJ_WEIGHT(obj)         = object.weight;
      GET_OBJ_TIMER(obj)          = object.timer;
      obj->obj_flags.Obj_max      = object.maxstate;
      obj->obj_flags.Obj_cur      = object.curstate;
      obj->obj_flags.Obj_owner    = object.owner;
      obj->obj_flags.Obj_mater    = object.mater;
      obj->obj_flags.extra_flags  = object.extra_flags;
      obj->obj_flags.affects      = object.bitvector;
      obj->obj_flags.wear_flags   = object.wear_flags;


      for (j = 0; j < MAX_OBJ_AFFECT; j++)
          obj->affected[j] = object.affected[j];
      if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
         {name_from_drinkcon(obj);
          if (GET_OBJ_VAL(obj,1))
             name_to_drinkcon(obj, GET_OBJ_VAL(obj,2));
         }

      return (obj);
     }
  else
     return (NULL);
}



int Obj_to_store(struct obj_data * obj, FILE * fl, int location)
{
  int j;
  struct obj_file_elem object;

  object.item_number = GET_OBJ_VNUM(obj);
  object.location    = location;
  object.value[0]    = GET_OBJ_VAL(obj, 0);
  object.value[1]    = GET_OBJ_VAL(obj, 1);
  object.value[2]    = GET_OBJ_VAL(obj, 2);
  object.value[3]    = GET_OBJ_VAL(obj, 3);
  object.weight      = GET_OBJ_WEIGHT(obj);
  object.timer       = GET_OBJ_TIMER(obj);
  object.maxstate    = obj->obj_flags.Obj_max;
  object.curstate    = obj->obj_flags.Obj_cur;
  object.owner       = obj->obj_flags.Obj_owner;
  object.mater       = obj->obj_flags.Obj_mater;

  object.bitvector   = obj->obj_flags.affects;
  object.extra_flags = obj->obj_flags.extra_flags;
  object.wear_flags  = obj->obj_flags.wear_flags;
  for (j = 0; j < MAX_OBJ_AFFECT; j++)
      object.affected[j] = obj->affected[j];

  if (fwrite(&object, sizeof(struct obj_file_elem), 1, fl) < 1)
     {perror("SYSERR: error writing object in Obj_to_store");
      return (0);
     }
  return (1);
}

int auto_equip(struct char_data *ch, struct obj_data *obj, int location)
{
  int j;

  // Lots of checks...
  if (location > 0)
     {// Was wearing it.
      switch (j = (location - 1))
         {
    case WEAR_LIGHT:
      break;
    case WEAR_FINGER_R:
    case WEAR_FINGER_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_FINGER)) // not fitting
         location = LOC_INVENTORY;
      break;
    case WEAR_NECK_1:
    case WEAR_NECK_2:
      if (!CAN_WEAR(obj, ITEM_WEAR_NECK))
         location = LOC_INVENTORY;
      break;
    case WEAR_BODY:
      if (!CAN_WEAR(obj, ITEM_WEAR_BODY))
         location = LOC_INVENTORY;
      break;
    case WEAR_HEAD:
      if (!CAN_WEAR(obj, ITEM_WEAR_HEAD))
         location = LOC_INVENTORY;
      break;
    case WEAR_LEGS:
      if (!CAN_WEAR(obj, ITEM_WEAR_LEGS))
         location = LOC_INVENTORY;
      break;
    case WEAR_FEET:
      if (!CAN_WEAR(obj, ITEM_WEAR_FEET))
         location = LOC_INVENTORY;
      break;
    case WEAR_HANDS:
      if (!CAN_WEAR(obj, ITEM_WEAR_HANDS))
         location = LOC_INVENTORY;
      break;
    case WEAR_ARMS:
      if (!CAN_WEAR(obj, ITEM_WEAR_ARMS))
         location = LOC_INVENTORY;
      break;
    case WEAR_SHIELD:
      if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
         location = LOC_INVENTORY;
      break;
    case WEAR_ABOUT:
      if (!CAN_WEAR(obj, ITEM_WEAR_ABOUT))
         location = LOC_INVENTORY;
      break;
    case WEAR_WAIST:
      if (!CAN_WEAR(obj, ITEM_WEAR_WAIST))
        location = LOC_INVENTORY;
      break;
    case WEAR_WRIST_R:
    case WEAR_WRIST_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_WRIST))
         location = LOC_INVENTORY;
      break;
    case WEAR_WIELD:
      if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
         location = LOC_INVENTORY;
      break;
    case WEAR_HOLD:
      if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
	     break;
      location = LOC_INVENTORY;
      break;
    case WEAR_BOTHS:
      if (CAN_WEAR(obj, ITEM_WEAR_BOTHS))
         break;
    default:
      location = LOC_INVENTORY;
    }

    if (location > 0)
       {/* Wearable. */
        if (!GET_EQ(ch,j))
           {/*
	         * Check the characters's alignment to prevent them from being
	         * zapped through the auto-equipping.
             */
            if (invalid_align(ch, obj)      ||
                invalid_anti_class(ch, obj) ||
                invalid_no_class(ch,obj))
               location = LOC_INVENTORY;
            else
               {equip_char(ch, obj, j|0x80|0x40);
                log("Equipped with %s %d",(obj)->short_description, j);
               }
           }
        else
           {/* Oops, saved a player with double equipment? */
            char aeq[128];
            sprintf(aeq, "SYSERR: autoeq: '%s' already equipped in position %d.", GET_NAME(ch), location);
            mudlog(aeq, BRF, LVL_IMMORT, TRUE);
            location = LOC_INVENTORY;
           }
       }
  }
  if (location <= 0)	/* Inventory */
     obj_to_char(obj, ch);
  return (location);
}


int Crash_delete_file(char *name, int mask)
{ int   retcode = TRUE;
  char  filename[MAX_STRING_LENGTH];
  FILE *fl;

  if (IS_SET(mask,CRASH_DELETE_OLD) &&
      get_filename(name, filename, CRASH_FILE)
     )
     {if (!(fl = fopen(filename, "rb")))
         {if (errno != ENOENT)	/* if it fails but NOT because of no file */
             log("SYSERR: deleting OLD crash file %s (1): %s", filename, strerror(errno));
          retcode = FALSE;
         }
      else
         {fclose(fl);
          /* if it fails, NOT because of no file */
          if (remove(filename) < 0 && errno != ENOENT)
             {log("SYSERR: deleting OLD crash file %s (2): %s", filename, strerror(errno));
              retcode = FALSE;
             }
         }
     }

  if (IS_SET(mask,CRASH_DELETE_NEW) &&
      get_filename(name, filename, TEXT_CRASH_FILE)
     )
     {if (!(fl = fopen(filename, "rb")))
         {if (errno != ENOENT)	/* if it fails but NOT because of no file */
             log("SYSERR: deleting crash file %s (1): %s", filename, strerror(errno));
          retcode = FALSE;
         }
      else
         {fclose(fl);
          /* if it fails, NOT because of no file */
          if (remove(filename) < 0 && errno != ENOENT)
             {log("SYSERR: deleting crash file %s (2): %s", filename, strerror(errno));
              retcode = FALSE;
             }
         }
     }

  if (IS_SET(mask,CRASH_DELETE_NEW) &&
      get_filename(name, filename, TIME_CRASH_FILE)
     )
     {if (!(fl = fopen(filename, "rb")))
         {if (errno != ENOENT)	/* if it fails but NOT because of no file */
             log("SYSERR: deleting crash file %s (1): %s", filename, strerror(errno));
          retcode = FALSE;
         }
      else
         {fclose(fl);
          /* if it fails, NOT because of no file */
          if (remove(filename) < 0 && errno != ENOENT)
             {log("SYSERR: deleting crash file %s (2): %s", filename, strerror(errno));
              retcode = FALSE;
             }
         }
     }
  return (retcode);
}


int Crash_delete_crashfile(struct char_data * ch)
{ int    retcode = TRUE;
  char   fname[MAX_INPUT_LENGTH];
  struct save_rent_info rent;
  FILE  *fl;

  if (get_filename(GET_NAME(ch), fname, CRASH_FILE))
     {if (!(fl = fopen(fname, "rb")))
         {if (errno != ENOENT)	/* if it fails, NOT because of no file */
             log("SYSERR: checking for crash file %s (3): %s", fname, strerror(errno));
          retcode = FALSE;
         }
      else
         {fread(&rent, sizeof(struct save_rent_info), 1, fl);
          fclose(fl);
          if (rent.rentcode == RENT_CRASH)
             Crash_delete_file(GET_NAME(ch),CRASH_DELETE_OLD);
         }
     }

  if (get_filename(GET_NAME(ch), fname, TIME_CRASH_FILE))
     {if (!(fl = fopen(fname, "rb")))
         {if (errno != ENOENT)	/* if it fails, NOT because of no file */
             log("SYSERR: checking for crash file %s (3): %s", fname, strerror(errno));
          retcode = FALSE;
         }
      else
         {fread(&rent, sizeof(struct save_rent_info), 1, fl);
          fclose(fl);
          if (rent.rentcode == RENT_CRASH)
             Crash_delete_file(GET_NAME(ch),CRASH_DELETE_NEW);
         }
     }
  return (retcode);
}


int Crash_clean_file(char *name)
{ int    retcode = TRUE;
  char   fname[MAX_STRING_LENGTH], filetype[20];
  struct save_rent_info rent;
  FILE  *fl;

  if (get_filename(name, fname, CRASH_FILE))
     {/*
       * open for write so that permission problems will be flagged now, at boot
       * time.
       */
      if (!(fl = fopen(fname, "r+b")))
         {if (errno != ENOENT)	/* if it fails, NOT because of no file */
             log("SYSERR: OPENING OBJECT FILE %s (4): %s", fname, strerror(errno));
          retcode = FALSE;
         }
      else
         {fread(&rent, sizeof(struct save_rent_info), 1, fl);
          fclose(fl);
          if (rent.rentcode == RENT_CRASH ||
              rent.rentcode == RENT_FORCED||
              rent.rentcode == RENT_TIMEDOUT
             )
             {if (rent.time < time(0) - (crash_file_timeout * SECS_PER_REAL_DAY))
                 {Crash_delete_file(name,CRASH_DELETE_OLD);
                  switch (rent.rentcode)
                  {case RENT_CRASH:
                        strcpy(filetype, "crash");
                        break;
                   case RENT_FORCED:
                        strcpy(filetype, "forced rent");
                        break;
                   case RENT_TIMEDOUT:
                        strcpy(filetype, "idlesave");
                        break;
                   default:
                        strcpy(filetype, "UNKNOWN!");
                        break;
                  }
                  log("    Deleting %s's %s file.", name, filetype);
                 }
             }
          else
          if (rent.rentcode == RENT_RENTED)
             {if (rent.time < time(0) - (rent_file_timeout * SECS_PER_REAL_DAY))
                 {Crash_delete_file(name,CRASH_DELETE_OLD);
                  log("    Deleting %s's rent file.", name);
                 }
             }
         }

     }
  if (get_filename(name, fname, TIME_CRASH_FILE))
     {if (!(fl = fopen(fname, "r+b")))
         {if (errno != ENOENT)	/* if it fails, NOT because of no file */
             log("SYSERR: OPENING OBJECT FILE %s (4): %s", fname, strerror(errno));
          retcode = FALSE;
         }
      else
         {fread(&rent, sizeof(struct save_rent_info), 1, fl);
          fclose(fl);
          if (rent.rentcode == RENT_CRASH ||
              rent.rentcode == RENT_FORCED||
              rent.rentcode == RENT_TIMEDOUT
             )
             {if (rent.time < time(0) - (crash_file_timeout * SECS_PER_REAL_DAY))
                 {Crash_delete_file(name,CRASH_DELETE_NEW);
                  switch (rent.rentcode)
                  {case RENT_CRASH:
                        strcpy(filetype, "crash");
                        break;
                   case RENT_FORCED:
                        strcpy(filetype, "forced rent");
                        break;
                   case RENT_TIMEDOUT:
                        strcpy(filetype, "idlesave");
                        break;
                   default:
                        strcpy(filetype, "UNKNOWN!");
                        break;
                  }
                  log("    Deleting %s's %s file.", name, filetype);
                 }
             }
          else
          if (rent.rentcode == RENT_RENTED)
             {if (rent.time < time(0) - (rent_file_timeout * SECS_PER_REAL_DAY))
                 {Crash_delete_file(name,CRASH_DELETE_NEW);
                  log("    Deleting %s's rent file.", name);
                 }
             }
         }
    }
  return (retcode);
}

void update_obj_file(void)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
      if (player_table[i].activity >= 0)
         Crash_clean_file(player_table[i].name);
}

void Crash_create_timer(int index, int num)
{if (player_table[index].timer)
    free(player_table[index].timer);
// prool:
#define MAKEINFO(pointer, number) CREATE(pointer, char, MAKESIZE(number))
// MAKEINFO((char *)player_table[index].timer, num);

/*
#define CREATE(result, type, number)  do {\
	if ((number) * sizeof(char) <= 0)	\
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);	\
	if (!((result) = (char *) calloc ((number), sizeof(char))))	\
		{ perror("SYSERR: malloc failure"); abort(); } } while(0)
*/

// CREATE((char *)player_table[index].timer , char, MAKESIZE(num));
// define CREATE(result, type, number)
 do {
	if ((MAKESIZE(num)) * sizeof(char) <= 0)	
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);	
	if (!( player_table[index].timer = (void *) calloc ((MAKESIZE(num)), sizeof(char))))	
		{ perror("SYSERR: malloc failure"); abort(); } } while(0);

 memset(player_table[index].timer,0,MAKESIZE(num));
}

/********** THIS ROUTINE CALC ITEMS IN CHAR EQUIP ********************/
void Crash_calc_objects(int index, int storing)
{
  FILE   *fl;
  char   fname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], name[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  int    count = 0, rnum, num = 0;
  struct save_rent_info rent;
  struct save_time_info info;

#ifndef USE_AUTOEQ
  return;
#endif
  strcpy(name, player_table[index].name);

  if (get_filename(name, fname, CRASH_FILE))
     {if (!(fl = fopen(fname, "rb")))
         log("[CO] : %s has no OLD rent file.\r\n", name);
      else
         {fread(&rent, sizeof(struct save_rent_info), 1, fl);
          sprintf (buf,"[CO] for %s :", name);
          switch (rent.rentcode)
          {case RENT_RENTED:
                strcat(buf, " Rent ");
                break;
           case RENT_CRASH:
                strcat(buf, " Crash ");
                break;
           case RENT_CRYO:
                strcat(buf, " Cryo ");
                break;
           case RENT_TIMEDOUT:
           case RENT_FORCED:
                strcat(buf, " TimedOut ");
                break;
           default:
                strcat(buf, " Undef ");
                break;
          }
          while (!feof(fl))
                {fread(&object, sizeof(struct obj_file_elem), 1, fl);
                 if (ferror(fl))
                    break;
                 if (!feof(fl))
                    {if (object.location != ITEM_DESTROYED &&
                         (rnum = real_object(object.item_number)) >= 0
			)
                        {count++;
                         if (storing)
			    obj_index[rnum].stored++;
                        }
                    }
	        }
          log("%s All objects = %d", buf, count);
          fclose(fl);
	 }
     }

  if (get_filename(name, fname, TIME_CRASH_FILE))
     {Crash_create_timer(index,0);
      if (!(fl = fopen(fname, "rb")))
         log("[CO] : %s has no NEW rent file.\r\n", name);
      else
         {fread(&rent, sizeof(struct save_rent_info), 1, fl);
          sprintf (buf,"[CO] for %s :", name);
          switch (rent.rentcode)
          {case RENT_RENTED:
                strcat(buf, " Rent ");
                break;
           case RENT_CRASH:
                strcat(buf, " Crash ");
                break;
           case RENT_CRYO:
                strcat(buf, " Cryo ");
                break;
           case RENT_TIMEDOUT:
           case RENT_FORCED:
                strcat(buf, " TimedOut ");
                break;
           default:
                strcat(buf, " Undef ");
		rent.rentcode = RENT_CRASH;
                break;
          }
	  rent.nitems = 0;
          while (!feof(fl))
                {fread(&info, sizeof(struct save_time_info), 1, fl);
                 if (ferror(fl))
                    {rent.nitems = -1;
                     break;
                    }
                 if (!feof(fl))
                    {if (info.timer && info.vnum)
		        rent.nitems++;	
                     if (info.timer > 0 &&
                         (rnum = real_object(info.vnum)) >= 0
			)
                        {count++;
                         if (storing)
			    obj_index[rnum].stored++;
                        }
                    }
	        }
	 fclose(fl);
	 player_table[index].timer->rent = rent;
	 if ((fl = fopen(fname, "r+b")))
	    {fwrite(&rent, sizeof(struct save_rent_info), 1, fl);
	     if (rent.nitems >= 0)
	        {Crash_create_timer(index,rent.nitems);	
		 player_table[index].timer->rent = rent;
	         fflush(fl);
   	         num = 0;
	         while (!feof(fl))
	               {fread(&info, sizeof(struct save_time_info), 1, fl);
		        if (ferror(fl))
		           break;
   		        if (!feof(fl)      &&
		            info.vnum      &&
			    info.timer     &&
		            num < rent.nitems
			   )
                           {player_table[index].timer->time[num++] = info;			
			   }
		       }
		}
	     fclose(fl);
	    }
          log("%s All objects = %d", buf, count);
          // fclose(fl); // prool: commented, bikoz crash here (two fclose!)
	 }
     }
}

void Crash_timer_obj(int index, long timer_dec)
{
  FILE   *fl;
  char   fname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], name[MAX_STRING_LENGTH];
  struct obj_file_elem *objects = NULL;
  int    reccount = 0, count = 0, idelete = 0, rnum, i;
  long   size;
  struct save_rent_info rent;

#ifndef USE_AUTOEQ
        return;
#endif

  strcpy(name,player_table[index].name);
  if (get_filename(name, fname, CRASH_FILE))
     {if (!(fl = fopen(fname, "rb")))
         log("[TO] %s has no OLD rent file.", name);
      else
         {fseek(fl, 0L, SEEK_END);
          size = ftell(fl);
          if (size < sizeof(struct save_rent_info))
             {log("[TO] %s - error structure of file.", name);
              fclose(fl);
	     }
	  else
             {rewind(fl);
              fread(&rent, sizeof(struct save_rent_info), 1, fl);
              size    -= sizeof(struct save_rent_info);
              reccount = MAX(0, MIN(MAX_SAVED_ITEMS, size / sizeof(struct obj_file_elem)));
              if (reccount)
                 {CREATE(objects, struct obj_file_elem, reccount);
                  reccount = fread(objects, sizeof(struct obj_file_elem), reccount, fl);
                  reccount = MIN(MAX_SAVED_ITEMS, reccount);
                 }
              fclose(fl);

              sprintf (buf,"[TO] OLD for %s(%d items) :", name, reccount);

              switch (rent.rentcode)
              {case RENT_RENTED:
                    strcat(buf, " Rent ");
                    break;
               case RENT_CRASH:
                    strcat(buf, " Crash ");
                    break;
               case RENT_CRYO:
                    strcat(buf, " Cryo ");
                    break;
               case RENT_TIMEDOUT:
               case RENT_FORCED:
                    strcat(buf, " TimedOut ");
                    break;
               default:
                    strcat(buf, " Undef ");
                    break;
              }
              if (reccount)
                 {for (i = 0; i < reccount; i++)
                      {if ((objects+i)->location != ITEM_DESTROYED)
                          {rnum = real_object((objects+i)->item_number);
                           if (rnum < 0)
                              continue;
                           // log("Item %d timer %d",rnum,object.timer);
                           count++;
                           if ((objects+i)->timer > timer_dec)
                              {(objects+i)->timer -= timer_dec;
                              }
                           else
                              {(objects+i)->timer    = -1;
                               (objects+i)->location = ITEM_DESTROYED;
                               idelete++;
                               if (rnum >= 0)
                                  {if (obj_index[rnum].stored > 0)
                                      obj_index[rnum].stored--;
                                   log("[TO] Player %s : item %s deleted - time outted",name,(obj_proto+rnum)->PNames[0]);
                                  }
                              };
                          }
                      }

                   if ((fl = fopen(fname,"w+b")))
                      {fwrite(&rent, sizeof(struct save_rent_info), 1, fl);
                       fwrite(objects, sizeof(struct obj_file_elem), reccount, fl);
                       fclose(fl);
                      }
                   else
                      {strcat(buf,"[cann't save new]");
                       reccount = 0;
                      }
                 }
              if (objects)
                 free(objects);
              if (reccount)
                 log("%s Objects-%d,  Delete-%d,  Timer-%ld", buf, count, idelete, timer_dec);
              else
                 log("ERROR recognised %s", buf);
	     }
	 }
     }	
  if (get_filename(name, fname, TIME_CRASH_FILE))
     {if (!player_table[index].timer)
         log("[TO] %s has no NEW rent file.", name);
      else
         {log("[TO] %s has %d items.", name, player_table[index].timer->rent.nitems);
	  for (i = count = 0; i < player_table[index].timer->rent.nitems; i++)
              if (player_table[index].timer->time[i].timer >= 0)
                 {rnum = real_object(player_table[index].timer->time[i].vnum);
                  count++;
                  if (player_table[index].timer->time[i].timer > timer_dec)
                     player_table[index].timer->time[i].timer -= timer_dec;
                  else
                     {player_table[index].timer->time[i].timer    = -1;
                      idelete++;
                      if (rnum >= 0)
                         {if (obj_index[rnum].stored > 0)
                             obj_index[rnum].stored--;
                          log("[TO] Player %s : item %s deleted - time outted",name,(obj_proto+rnum)->PNames[0]);
                         }
                      };
                  }
          sprintf (buf,"[TO] NEW for %s(%d items) :", name, count);
          if ((fl = fopen(fname,"wb")))
             {fwrite(player_table[index].timer,
	             sizeof(struct save_rent_info) +
	                    player_table[index].timer->rent.nitems *
			    sizeof(struct save_time_info),
		     1, fl);
              fclose(fl);
             }
          else
             {strcat(buf,"[cann't save new]");
              reccount = 0;
             }
          log("%s Objects-%d,  Delete-%d,  Timer-%ld", buf, count, idelete, timer_dec);
	 }
     }	
}

void Crash_listrent(struct char_data * ch, char *name)
{
  FILE *fl;
  char   fname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  struct obj_data *obj;
  struct save_rent_info rent;
  struct save_time_info data;
  int    i;

  if (get_filename(name, fname, CRASH_FILE))
     {if (!(fl = fopen(fname, "rb")))
         {sprintf(buf, "%s has no OLD rent file.\r\n", name);
          send_to_char(buf, ch);
         }
      else
         {sprintf(buf, "%s\r\n", fname);
          if (!feof(fl))
             fread(&rent, sizeof(struct save_rent_info), 1, fl);
          switch (rent.rentcode)
          {
case RENT_RENTED:
                strcat(buf, "Rent\r\n");
                break;
           case RENT_CRASH:
	        strcat(buf, "Crash\r\n");
                break;
           case RENT_CRYO:
                strcat(buf, "Cryo\r\n");
                break;
           case RENT_TIMEDOUT:
           case RENT_FORCED:
                strcat(buf, "TimedOut\r\n");
                break;
           default:
                strcat(buf, "Undef\r\n");
                break;
          }
          while (!feof(fl))
                {fread(&object, sizeof(struct obj_file_elem), 1, fl);
                 if (ferror(fl))
                    break;
                 if (!feof(fl))
                    {if ((i = real_object(object.item_number)) > -1)
                        {obj = read_object(object.item_number, VIRTUAL);
#if USE_AUTOEQ
                         sprintf(buf + strlen(buf), " [%5d] (%5dau) <%2d> %-20s\r\n",
		                 object.item_number, GET_OBJ_RENT(obj),
		                 object.location, obj->short_description);
#else
    	                 sprintf(buf + strlen(buf), " [%5d] (%5dau) %-20s\r\n",
		                 object.item_number, GET_OBJ_RENT(obj),
		                 obj->short_description);
#endif
	                 extract_obj(obj);
	                 if (strlen(buf) > MAX_STRING_LENGTH - 80)
	                    {strcat(buf, "** Excessive rent listing. **\r\n");
	                     break;
	                    }
                        }
                     }
	        }
          send_to_char(buf, ch);

          fclose(fl);
	 }
     }	

  if (get_filename(name, fname, TIME_CRASH_FILE))
     {if (!(fl = fopen(fname, "rb")))
         {sprintf(buf, "%s has no NEW rent file.\r\n", name);
          send_to_char(buf, ch);
         }
      else
         {sprintf(buf, "%s\r\n", fname);
          if (!feof(fl))
             fread(&rent, sizeof(struct save_rent_info), 1, fl);
          switch (rent.rentcode)
          {
case RENT_RENTED:
                strcat(buf, "Rent\r\n");
                break;
           case RENT_CRASH:
	        strcat(buf, "Crash\r\n");
                break;
           case RENT_CRYO:
                strcat(buf, "Cryo\r\n");
                break;
           case RENT_TIMEDOUT:
           case RENT_FORCED:
                strcat(buf, "TimedOut\r\n");
                break;
           default:
                strcat(buf, "Undef\r\n");
                break;
          }
          while (!feof(fl))
                {fread(&data, sizeof(struct save_time_info), 1, fl);
                 if (ferror(fl))
                    break;
                 if (!feof(fl))
                    {if ((i = real_object(data.vnum)) > -1)
                        {sprintf(buf + strlen(buf), " [%5d] (%5dau) <%2d> %-20s\r\n",
		                 data.vnum, GET_OBJ_RENT(obj_proto+i),
		                 data.timer, (obj_proto+i)->short_description);
	                 if (strlen(buf) > MAX_STRING_LENGTH - 80)
	                    {strcat(buf, "** Excessive rent listing. **\r\n");
	                     break;
	                    }
                        }
                     }
	        }
          send_to_char(buf, ch);

          fclose(fl);
	 }
     }	
}



int Crash_write_rentcode(struct char_data * ch, FILE * fl, struct save_rent_info * rent)
{
  if (fwrite(rent, sizeof(struct save_rent_info), 1, fl) < 1)
     {perror("SYSERR: writing rent code");
      return (0);
     }
  return (1);
}



/*
 * Return values:
 *  0 - successful load, keep char in rent room.
 *  1 - load failure or load of crash items -- put char in temple.
 *  2 - rented equipment lost (no $)
 */
struct container_list_type {
 struct obj_data            *tank;
 struct container_list_type *next;
 int    location;
};

int Crash_load_otype(struct char_data * ch)
{
  FILE  *fl;
  char   fname[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  struct save_rent_info rent;
  int    cost, orig_rent_code, num_objs = 0, iplayer=get_ptable_by_name(GET_NAME(ch));
  float  num_of_days;
  struct obj_data *obj, *obj2, *obj_list=NULL;
  /* AutoEQ addition. */
  int    location;
  struct container_list_type *tank_list = NULL, *tank;

  /* Empty all of the container lists (you never know ...) */
  if (!get_filename(GET_NAME(ch), fname, CRASH_FILE) ||
      iplayer < 0
     )
     return (1);
  if (!(fl = fopen(fname, "r+b")))
     {if (errno != ENOENT)
         {/* if it fails, NOT because of no file */
          log("SYSERR: READING OBJECT FILE %s (5): %s", fname, strerror(errno));
          send_to_char("\r\n********************* NOTICE *********************\r\n"
		               "Проблемы с восстановлением Ваших вещей из файла.\r\n"
		               "Обращайтесь за помощью к Богам.\r\n", ch);
         }
      sprintf(buf, "%s entering game with no equipment.", GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      Crash_timer_obj(iplayer, 1000000L);
      return (1);
     }

  if (!feof(fl))
      fread(&rent, sizeof(struct save_rent_info), 1, fl);
  else
      {fclose(fl);
       log("SYSERR: Crash_load: %s's rent file was empty!", GET_NAME(ch));
       return (1);
      }



  if (rent.rentcode == RENT_RENTED ||
      rent.rentcode == RENT_TIMEDOUT)
     {num_of_days = (float) (time(0) - rent.time) / SECS_PER_REAL_DAY;
      sprintf(buf,"%s was %1.2f days in rent",GET_NAME(ch),num_of_days);
      mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      cost = (int) (rent.net_cost_per_diem * num_of_days);
      if (cost < 0)
         cost = 0;
      if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch))
         {
fclose(fl);
	  sprintf(buf, "%s entering game, rented equipment lost (no $).",
	              GET_NAME(ch));
          mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
          GET_GOLD(ch) = GET_BANK_GOLD(ch) = 0;
          Crash_timer_obj(iplayer, 1000000L);
          Crash_crashsave(ch);
          return (2);
         }
      else
         {if (cost)
             {sprintf(buf,"С Вас содрали %d %s за постой.\r\n",cost,desc_count(cost,WHAT_MONEYu));
              send_to_char(buf,ch);
             }
          GET_BANK_GOLD(ch) -= MAX(cost - GET_GOLD(ch), 0);
          GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
          save_char(ch, NOWHERE);
         }
     }
  switch (orig_rent_code = rent.rentcode)
  {
  case RENT_RENTED:
    sprintf(buf, "%s un-renting and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRASH:
    sprintf(buf, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRYO:
    sprintf(buf, "%s un-cryo'ing and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_FORCED:
  case RENT_TIMEDOUT:
    sprintf(buf, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  default:
    sprintf(buf, "SYSERR: %s entering game with undefined rent code %d.", GET_NAME(ch), rent.rentcode);
    mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  }

  while (!feof(fl))
        {if (!fread(&object, sizeof(struct obj_file_elem), 1, fl) ||
             ferror(fl))
            {sprintf(buf,"SYSERR: Reading crash file: Crash_load(%s[%d items])",fname,num_objs);
             perror(buf);
	     break;
            }
         if (feof(fl))
            break;
         ++num_objs;
         if ((obj = Obj_from_store(object, &location)) == NULL)
            {send_to_char("Что-то рассыпалось от длительного использования.\r\n",ch);
             continue;
            }
         // Check timer
         if (!last_rent_check)
            last_rent_check = time(NULL);
         if ((obj->obj_flags.Obj_timer - (time(NULL) - last_rent_check) / 60) <= 0)
            {sprintf(buf,"%s рассыпал%s от длительного использования.\r\n",
                     CAP(obj->PNames[0]),GET_OBJ_SUF_2(obj));
             send_to_char(buf,ch);
             extract_obj(obj);
             continue;
            }
         // Check valid class
         if (invalid_anti_class(ch,obj) || invalid_unique(ch,obj))
            {sprintf(buf,"%s рассыпал%s, как запрещенн%s для Вас.\r\n",
                     CAP(obj->PNames[0]),GET_OBJ_SUF_2(obj),GET_OBJ_SUF_3(obj));
             send_to_char(buf,ch);
             extract_obj(obj);
             continue;
            }
         obj->next_content = obj_list;
         obj_list          = obj;
         obj->worn_on      = location;
	 // sprintf(buf,"You have %s at %d.\r\n",obj->PNames[0],location);
	 // send_to_char(buf,ch);
        }
  for (obj = obj_list; obj; obj = obj2)
      {obj2 = obj->next_content;
       if (obj->worn_on >= 0)
          {// Equipped or in inventory
           if (obj2 && obj2->worn_on < 0 && GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
              {//This is container and it is not free
               CREATE(tank, struct container_list_type, 1);
               tank->next     = tank_list;
               tank->tank     = obj;
               tank->location = 0;
               tank_list      = tank;
              }
           else
              while (tank_list)
                    {//Clear tanks list
                     tank      = tank_list;
                     tank_list = tank->next;
                     free(tank);
                    }
           location = obj->worn_on;
           obj->worn_on = 0;
	
           auto_equip(ch, obj, location);
	   // sprintf(buf,"Attempt equip you %s at %d.\r\n",obj->PNames[0],location);
	   // send_to_char(buf,ch);
          }
       else
          {if (obj2 && obj2->worn_on < obj->worn_on && GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
              {//This is container and it is not free
               CREATE(tank, struct container_list_type, 1);
               tank->next     = tank_list;
               tank->tank     = obj;
               tank->location = obj->worn_on;
               tank_list      = tank;
              }
           else
              while (tank_list)
                 // Clear all tank than less or equal this object location
                    if (tank_list->location > obj->worn_on)
                       break;
                    else
                       {tank      = tank_list;
                        tank_list = tank->next;
                        free(tank);
                       }
           obj->worn_on = 0;
           if (tank_list)
              {// sprintf(buf,"Attempt put %s to %s.\r\n",obj->PNames[0],tank_list->tank->PNames[0]);
               // send_to_char(buf,ch);
	       obj_to_obj(obj,tank_list->tank);
	      }
           else
	      {// sprintf(buf,"Attempt give you %s.\r\n",obj->PNames[0]);
               // send_to_char(buf,ch);
	       obj_to_char(obj,ch);
	      }
          }
      }
  while (tank_list)
        {//Clear tanks list
         tank      = tank_list;
         tank_list = tank->next;
         free(tank);
        }
  //log("[CRASH_LOAD->AFFECT_TOTAL] Start");
  affect_total(ch);
  //log("[CRASH_LOAD->AFFECT_TOTAL] Start");
  /* Little hoarding check. -gg 3/1/98 */
  // sprintf(fname, "%s (level %d) has %d objects (max %d).",
  //	      GET_NAME(ch), GET_LEVEL(ch), num_objs, max_obj_save);
  // mudlog(fname, NRM, MAX(GET_INVIS_LEV(ch), LVL_GOD), TRUE);

  /* turn this into a crash file by re-writing the control block */
  rent.rentcode = RENT_CRASH;
  rent.time     = time(0);
  rewind(fl);
  Crash_write_rentcode(ch, fl, &rent);
  fclose(fl);

  if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
    return (0);
  else
    return (1);
}


int Crash_load_ntype(struct char_data * ch)
{
  FILE  *fl;
  char   fname[MAX_STRING_LENGTH], *data, *readdata;
  struct save_rent_info rent;
  int    cost, orig_rent_code, num_objs = 0, reccount, fsize, error, iplayer;
  float  num_of_days;
  struct obj_data *obj, *obj2, *obj_list=NULL;
  int    location;
  struct container_list_type *tank_list = NULL, *tank, *tank_to;

  if ((iplayer=get_ptable_by_name(GET_NAME(ch))) < 0 ||
      !get_filename(GET_NAME(ch), fname, TIME_CRASH_FILE))
     return (1);

  if (!(fl = fopen(fname, "r+b")))
     {if (errno != ENOENT)
         {/* if it fails, NOT because of no file */
          log("SYSERR: READING OBJECT FILE %s (5): %s", fname, strerror(errno));
          send_to_char("\r\n** Нет файла таймера **\r\n"
		       "Проблемы с восстановлением Ваших вещей из файла.\r\n"
		       "Обращайтесь за помощью к Богам.\r\n", ch);
         }
      sprintf(buf, "%s entering game with no equipment.", GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      Crash_timer_obj(iplayer, 1000000L);
      return (1);
     }

  if (!feof(fl))
      fread(&rent, sizeof(struct save_rent_info), 1, fl);
  else
      {fclose(fl);
       send_to_char("\r\n** Файл таймера пустой **\r\n"
		    "Проблемы с восстановлением Ваших вещей из файла.\r\n"
		       "Обращайтесь за помощью к Богам.\r\n", ch);
       log("SYSERR: Crash_load: %s's rent file was empty!", GET_NAME(ch));
       return (1);
      }
  fclose(fl);

  if (rent.rentcode == RENT_RENTED ||
      rent.rentcode == RENT_TIMEDOUT
     )
     {num_of_days = (float) (time(0) - rent.time) / SECS_PER_REAL_DAY;
      sprintf(buf,"%s was %1.2f days in rent",GET_NAME(ch),num_of_days);
      mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      cost = (int) (rent.net_cost_per_diem * num_of_days);
      if (cost < 0)
         cost = 0;
      if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch))
         {send_to_char("\r\n** Не хватит оплатить постой **\r\n"
, ch);
	  sprintf(buf, "%s entering game, rented equipment lost (no $).",
	          GET_NAME(ch));
          mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
          GET_GOLD(ch) = GET_BANK_GOLD(ch) = 0;
          Crash_timer_obj(iplayer, 1000000L);
          return (2);
         }
      else
         {if (cost)
             {sprintf(buf,"С Вас содрали %d %s за постой.\r\n",cost,desc_count(cost,WHAT_MONEYu));
              send_to_char(buf,ch);
             }
          GET_GOLD(ch) -= MAX(cost - GET_BANK_GOLD(ch), 0);
          GET_BANK_GOLD(ch) = MAX(GET_BANK_GOLD(ch) - cost, 0);
          save_char(ch, NOWHERE);
         }
     }
  switch (orig_rent_code = rent.rentcode)
  {
  case RENT_RENTED:
    sprintf(buf, "%s un-renting and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRASH:
    sprintf(buf, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRYO:
    sprintf(buf, "%s un-cryo'ing and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_FORCED:
  case RENT_TIMEDOUT:
    sprintf(buf, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  default:
    sprintf(buf, "SYSERR: %s entering game with undefined rent code %d.", GET_NAME(ch), rent.rentcode);
    mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  }

  Crash_calc_objects(iplayer,FALSE);

  if (!SAVEINFO(iplayer)->rent.rentcode)
     {send_to_char("\r\n** Неизвестный код ренты **\r\n"
		   "Проблемы с восстановлением Ваших вещей из файла.\r\n"
		   "Обращайтесь за помощью к Богам.\r\n", ch);
      Crash_timer_obj(iplayer, 1000000L);
      return(1);
     }

  fsize = 0;
  if (!get_filename(GET_NAME(ch), fname, TEXT_CRASH_FILE) ||
      !(fl = fopen(fname, "r+b"))
     )
     {send_to_char("\r\n** Нет файла описания вещей **\r\n"
		   "Проблемы с восстановлением Ваших вещей из файла.\r\n"
		   "Обращайтесь за помощью к Богам.\r\n", ch);
      Crash_timer_obj(iplayer, 1000000L);
      return(1);
     }
  fseek(fl,0L,SEEK_END);
  fsize = ftell(fl);
  if (!fsize)
     {fclose(fl);
      send_to_char("\r\n** Файл описания вещей пустой **\r\n"
		   "Проблемы с восстановлением Ваших вещей из файла.\r\n"
		   "Обращайтесь за помощью к Богам.\r\n", ch);
      Crash_timer_obj(iplayer, 1000000L);
      return(1);
     }

  CREATE(readdata, char, fsize+1);
  fseek(fl,0L,SEEK_SET);
  fread(readdata,fsize,1,fl);
  if (ferror(fl) || !readdata)
     {fclose(fl);
      send_to_char("\r\n** Ошибка чтения файла описания вещей **\r\n"
		   "Проблемы с восстановлением Ваших вещей из файла.\r\n"
		   "Обращайтесь за помощью к Богам.\r\n", ch);
      log("Memory error or cann't read %s(%d)...",fname,fsize);
      free(readdata);
      Crash_timer_obj(iplayer, 1000000L);
      return(1);
     };
  fclose(fl);

  data = readdata;
  *(data+fsize) = '\0';

  for (fsize = 0, reccount = SAVEINFO(iplayer)->rent.nitems;
       reccount > 0 && *data && *data != END_CHAR;
       reccount--, fsize++
      )
      {++num_objs;
       if ((obj = read_one_object(&data, &error)) == NULL)
          {send_to_char(data,ch);
	   send_to_char("Что-то рассыпалось от длительного использования.\r\n",ch);
           continue;
          }
	
	if (GET_OBJ_VNUM(obj) != SAVEINFO(iplayer)->time[fsize].vnum)
	   {send_to_char("Нет соответствия заголовков - чтение предметов прервано.\r\n", ch);
	    extract_obj(obj);	
            break;
	   }
	
        // Check timer
	GET_OBJ_TIMER(obj) = SAVEINFO(iplayer)->time[fsize].timer;
        // if (!last_rent_check)
        //   last_rent_check = time(NULL);
        // if ((GET_OBJ_TIMER(obj) - (time(NULL) - last_rent_check) / 60) <= 0 ||
        //    error
	//   )
        //   {sprintf(buf,"%s рассыпал%s от длительного использования.\r\n",
        //            CAP(obj->PNames[0]),GET_OBJ_SUF_2(obj));
        //    send_to_char(buf,ch);
        //    extract_obj(obj);
        //    continue;
        //   }
        // Check valid class
        if (invalid_anti_class(ch,obj) || invalid_unique(ch,obj))
           {sprintf(buf,"%s рассыпал%s, как запрещенн%s для Вас.\r\n",
                    CAP(obj->PNames[0]),GET_OBJ_SUF_2(obj),GET_OBJ_SUF_3(obj));
            send_to_char(buf,ch);
            extract_obj(obj);
            continue;
           }
         obj->next_content = obj_list;
         obj_list          = obj;
	 //sprintf(buf,"You have %s at %d.\r\n",obj->PNames[0],obj->worn_on);
         //send_to_char(buf,ch);
        }
  for (obj = obj_list; obj; obj = obj2)
      {obj2 = obj->next_content;
       obj->next_content = NULL;
       if (obj->worn_on >= 0)
          {// Equipped or in inventory
           if (obj2 && obj2->worn_on < 0 && GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
              {// This is container and it is not free
               CREATE(tank, struct container_list_type, 1);
               tank->next     = tank_list;
               tank->tank     = obj;
               tank->location = 0;
               tank_list      = tank;
              }
           else
              {while (tank_list)
                     {// Clear tanks list
                      tank      = tank_list;
                      tank_list = tank->next;
                      free(tank);
                     }
	      }
           location = obj->worn_on;
           obj->worn_on = 0;
	
           auto_equip(ch, obj, location);
	   //sprintf(buf,"Attempt equip you %s at %d.\r\n",obj->PNames[0],location);
	   //send_to_char(buf,ch);
          }
       else
          {if (obj2 && obj2->worn_on < obj->worn_on && GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
              {// This is container and it is not free
	       tank_to        = tank_list;
	       CREATE(tank, struct container_list_type, 1);
               tank->next     = tank_list;
               tank->tank     = obj;
               tank->location = obj->worn_on;
               tank_list      = tank;
              }
           else
             {while ((tank_to = tank_list))
                    // Clear all tank than less or equal this object location
                    if (tank_list->location > obj->worn_on)
                       break;
                    else
                       {tank      = tank_list;
                        tank_list = tank->next;
                        free(tank);
                       }
	     }
           obj->worn_on = 0;
           if (tank_to)
              {//sprintf(buf,"Attempt put %s to %s.\r\n",obj->PNames[0],tank_to->tank->PNames[0]);
               //send_to_char(buf,ch);
	       obj_to_obj(obj,tank_to->tank);
	      }
           else
	      {//sprintf(buf,"Attempt give you %s.\r\n",obj->PNames[0]);
               //send_to_char(buf,ch);
	       obj_to_char(obj,ch);
	      }
          }
      }
  while (tank_list)
        {//Clear tanks list
         tank      = tank_list;
         tank_list = tank->next;
         free(tank);
        }
  //log("[CRASH_LOAD->AFFECT_TOTAL] Start");
  affect_total(ch);
  //log("[CRASH_LOAD->AFFECT_TOTAL] Start");
  /* Little hoarding check. -gg 3/1/98 */
  // sprintf(fname, "%s (level %d) has %d objects (max %d).",
  // 	  GET_NAME(ch), GET_LEVEL(ch), num_objs, max_obj_save);
  // mudlog(fname, NRM, MAX(GET_INVIS_LEV(ch), LVL_GOD), TRUE);

  /* turn this into a crash file by re-writing the control block */

  get_filename(GET_NAME(ch), fname, TIME_CRASH_FILE);
  if ((fl = fopen(fname, "r+b")))
     {SAVEINFO(iplayer)->rent.rentcode = RENT_CRASH;
      SAVEINFO(iplayer)->rent.time     = time(0);
      Crash_write_rentcode(ch, fl, &SAVEINFO(iplayer)->rent);
      fclose(fl);
     }
  free(readdata);

  if (orig_rent_code == RENT_RENTED ||
      orig_rent_code == RENT_CRYO
     )
     return (0);
  else
     return (1);
}


int Crash_load(struct char_data * ch)
{
 FILE  *fl;
  char   fname[MAX_STRING_LENGTH];

  if (get_filename(GET_NAME(ch), fname, TIME_CRASH_FILE) &&
      (fl = fopen(fname, "r+b"))
     )
     {fclose(fl);
      return (Crash_load_ntype(ch));
     }

  if (get_filename(GET_NAME(ch), fname, CRASH_FILE) &&
      (fl = fopen(fname, "r+b"))
     )
     {fclose(fl);
      return (Crash_load_otype(ch));
     }
  return (1);
}

#define CRASH_LENGTH   0x20000
#define CRASH_DEPTH    0x1000

int   Crashitems;
char  *Crashbufferdata
 = NULL;
char  *Crashbufferpos;

int Init_crashsave(char *name)
{int iplayer = -1;

 if ((iplayer = get_ptable_by_name(name)) < 0)
    {sprintf(buf,"[SYSERR] Bad ID for '%s'- %d.",name,iplayer);
     mudlog(buf, NRM, MAX(LVL_IMMORT, LVL_GOD), TRUE);
     return(-1);
    }
 if (!Crashbufferdata)
    CREATE(Crashbufferdata, char, CRASH_LENGTH);
 Crashitems = 0;
 Crashbufferpos = Crashbufferdata;
 *Crashbufferpos= '\0';
 return (iplayer);
}

int Done_crashsave(int iplayer)
{char  fname[MAX_STRING_LENGTH] = "";
 FILE *fl=NULL;
 // log("Done_crashsave for char ID %d, name %s", iplayer,
 //     (player_table+iplayer)->name);

 if (iplayer < 0 || !SAVEINFO(iplayer))
    return (0);

 if (get_filename((player_table+iplayer)->name,fname,TEXT_CRASH_FILE))
    {if (!(fl = fopen(fname,"w")) )
        {sprintf(buf,"[SYSERR] Store text file '%s'- MAY BE LOCKED.",fname);
         mudlog(buf, NRM, MAX(LVL_IMMORT, LVL_GOD), TRUE);
	 return (0);
	}
     fprintf(fl,"* Items file\n%s\n$\n$\n",Crashbufferdata);
     fclose(fl);
    }
 else
    {Crash_delete_file((player_table+iplayer)->name,CRASH_DELETE_OLD|CRASH_DELETE_NEW);
     return (0);
    }

 if (get_filename((player_table+iplayer)->name,fname,TIME_CRASH_FILE))
    {if (!(fl = fopen(fname,"wb")))
        {sprintf(buf,"[SYSERR] Store time file '%s' - MAY BE LOCKED.",fname);
         mudlog(buf, NRM, MAX(LVL_IMMORT, LVL_IMMORT), TRUE);
	 return (0);
	}
     fwrite(SAVEINFO(iplayer), SAVESIZE(iplayer), 1, fl);
     Crash_delete_file((player_table+iplayer)->name, CRASH_DELETE_OLD);
     fclose(fl);
    }
 else
    return (0);

 return (1);
}


int Crash_save(int iplayer, struct obj_data * obj, int location)
{
  struct obj_data *tmp;

  if (obj)
     {Crash_save(iplayer, obj->next_content, location);
      Crash_save(iplayer, obj->contains, MIN(0, location) - 1);
      if (iplayer >= 0 &&
          Crashitems < MAX_SAVED_ITEMS &&
          Crashbufferpos - Crashbufferdata + CRASH_DEPTH < CRASH_LENGTH
	 )
         {write_one_object(&Crashbufferpos,obj,location);
	  SAVEINFO(iplayer)->time[Crashitems].vnum  = GET_OBJ_VNUM(obj);
	  SAVEINFO(iplayer)->time[Crashitems].timer = GET_OBJ_TIMER(obj);
	  Crashitems++;
	 }
      for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
          GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);
     }
  return (TRUE);
}


void Crash_restore_weight(struct obj_data * obj)
{
  if (obj)
     {Crash_restore_weight(obj->contains);
      Crash_restore_weight(obj->next_content);
      if (obj->in_obj)
         GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
     }
}

/*
 * Get !RENT items from equipment to inventory and
 * extract !RENT out of worn containers.
 */
void Crash_extract_norent_eq(struct char_data *ch)
{
  int j;

  for (j = 0; j < NUM_WEARS; j++)
      {if (GET_EQ(ch, j) == NULL)
          continue;

       if (Crash_is_unrentable(GET_EQ(ch, j)))
          obj_to_char(unequip_char(ch, j), ch);
       else
          Crash_extract_norents(GET_EQ(ch, j));
      }
}

void Crash_extract_objs(struct obj_data * obj)
{ int rnum;
  if (obj)
     {Crash_extract_objs(obj->contains);
      Crash_extract_objs(obj->next_content);
      if ((rnum = real_object(GET_OBJ_VNUM(obj))) >= 0)
         obj_index[rnum].stored++;
      extract_obj(obj);
     }
}


int Crash_is_unrentable(struct obj_data * obj)
{
  if (!obj)
     return (0);

  if (IS_OBJ_STAT(obj, ITEM_NORENT) ||
      GET_OBJ_RENT(obj) < 0         ||
      GET_OBJ_RNUM(obj) <= NOTHING  ||
      GET_OBJ_TYPE(obj) == ITEM_KEY
     )
     return (1);

  return (0);
}


void Crash_extract_norents(struct obj_data * obj)
{
  if (obj)
     {Crash_extract_norents(obj->contains);
      Crash_extract_norents(obj->next_content);
      if (Crash_is_unrentable(obj))
         extract_obj(obj);
     }
}


void Crash_extract_expensive(struct obj_data * obj)
{
  struct obj_data *tobj, *max;

  max = obj;
  for (tobj = obj; tobj; tobj = tobj->next_content)
      if (GET_OBJ_RENT(tobj) > GET_OBJ_RENT(max))
         max = tobj;
  extract_obj(max);
}



void Crash_calculate_rent(struct obj_data * obj, int *cost)
{
  if (obj)
     {*cost += MAX(0, GET_OBJ_RENT(obj));
      Crash_calculate_rent(obj->contains, cost);
      Crash_calculate_rent(obj->next_content, cost);
     }
}

int Crash_calcitems(struct obj_data *obj)
{int i = 0;

 if (obj)
    {i += Crash_calcitems(obj->contains);
     i += Crash_calcitems(obj->next_content);
     i++;
    }
 return (i);
}


void Crash_crashsave(struct char_data * ch)
{
  struct save_rent_info rent;
  int    j, num=0, iplayer=-1;

  if (IS_NPC(ch))
     return;

  if (get_filename(GET_NAME(ch), buf, TIME_CRASH_FILE))
     {if ((iplayer = Init_crashsave(GET_NAME(ch))) < 0)
         {sprintf(buf,"[SYSERR] Store file '%s' - INVALID ID.",GET_NAME(ch));
          mudlog(buf, NRM, MAX(LVL_IMMORT, LVL_IMMORT), TRUE);
          return;
	 }
      for (j = 0; j < NUM_WEARS; j++)
          num += Crash_calcitems(GET_EQ(ch, j));
      num +=
 Crash_calcitems(ch->carrying);

      rent.rentcode = RENT_CRASH;
      rent.time     = time(0);
      rent.nitems   = num;

      Crash_create_timer(iplayer, num);
      SAVEINFO(iplayer)->rent = rent;
      //sprintf(buf,"Save %d items, %d bytes\r\n", num, SAVESIZE(iplayer));
      //send_to_char(buf,ch);

      for (j = 0; j < NUM_WEARS; j++)
          if (GET_EQ(ch, j))
             {Crash_save(iplayer, GET_EQ(ch, j), j + 1);
              Crash_restore_weight(GET_EQ(ch, j));
             }

      Crash_save(iplayer, ch->carrying, 0);
      Crash_restore_weight(ch->carrying);

      Done_crashsave(iplayer);
      REMOVE_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
     }
}

void Crash_ldsave(struct char_data * ch)
{
  struct save_rent_info rent;
  int    j, num=0, iplayer=-1;
  int    cost=0, cost_eq=0;


  if (IS_NPC(ch))
     return;

  if (get_filename(GET_NAME(ch), buf, TIME_CRASH_FILE))
     {if ((iplayer = Init_crashsave(GET_NAME(ch))) < 0)
         {sprintf(buf,"[SYSERR] Store file '%s' - INVALID ID.",GET_NAME(ch));
          mudlog(buf, NRM, MAX(LVL_IMMORT, LVL_IMMORT), TRUE);
          return;
	 }
      // Crash_extract_norent_eq(ch);
      // Crash_extract_norents(ch->carrying);

      cost = 0;
      Crash_calculate_rent(ch->carrying, &cost);
      num += Crash_calcitems(ch->carrying);

      cost_eq = 0;
      for (j = 0; j < NUM_WEARS; j++)
          {Crash_calculate_rent(GET_EQ(ch, j), &cost_eq);
	   num += Crash_calcitems(GET_EQ(ch, j));
	  }
      cost += cost_eq;

      Crash_create_timer(iplayer, num);
      if (!num)
         {Crash_delete_file(GET_NAME(ch),CRASH_DELETE_OLD | CRASH_DELETE_NEW);
          return;
         }

      rent.net_cost_per_diem  = cost;
      rent.rentcode           = RENT_TIMEDOUT;
      rent.time               = time(0);
      rent.gold               = GET_GOLD(ch);
      rent.account            = GET_BANK_GOLD(ch);
      rent.nitems             = num;
      SAVEINFO(iplayer)->rent = rent;

      for (j = 0; j < NUM_WEARS; j++)
          {if (GET_EQ(ch, j))
              {Crash_save(iplayer, GET_EQ(ch, j), j + 1);
               Crash_restore_weight(GET_EQ(ch, j));
               // Crash_extract_objs(GET_EQ(ch, j));
              }
          }
      Crash_save(iplayer, ch->carrying, 0);
      Crash_restore_weight(ch->carrying);
      if (!Done_crashsave(iplayer))
         {sprintf(buf,"[LD error] Cann't save items for char %s.",GET_NAME(ch));
          mudlog(buf, BRF, LVL_IMMORT, TRUE);

	  send_to_char("!!! Проблемы с записью Ваших вещей - обращайтесь к Богам !!!\r\n", ch);
         }	
      else
         {// sprintf(buf,"[LD Ok] Store %d items for char %s.",num,GET_NAME(ch));
          // mudlog(buf, BRF, LVL_IMMORT, TRUE);
         }	  	
      REMOVE_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
	
      // Crash_extract_objs(ch->carrying);
     }
  else
     {sprintf(buf,"[LD error] Cann't created descriptor for char %s.",GET_NAME(ch));
      mudlog(buf, BRF, LVL_IMMORT, TRUE);

      send_to_char("!!! Не могу создать дескриптор файла вещей - обращайтесь к Богам !!!\r\n", ch);
     }
}



void Crash_idlesave(struct char_data * ch)
{
  struct save_rent_info rent;
  int    j, num=0, iplayer=-1;
  int    cost=0, cost_eq=0;


  if (IS_NPC(ch))
     return;

  if (get_filename(GET_NAME(ch), buf, TIME_CRASH_FILE))
     {if ((iplayer = Init_crashsave(GET_NAME(ch))) < 0)
         {sprintf(buf,"[SYSERR] Store file '%s' - INVALID ID.",GET_NAME(ch));
          mudlog(buf, NRM, MAX(LVL_IMMORT, LVL_IMMORT), TRUE);
          return;
	 }
      Crash_extract_norent_eq(ch);
      Crash_extract_norents(ch->carrying);

      cost = 0;
      Crash_calculate_rent(ch->carrying, &cost);
      num += Crash_calcitems(ch->carrying);

      cost_eq = 0;
      for (j = 0; j < NUM_WEARS; j++)
          {Crash_calculate_rent(GET_EQ(ch, j), &cost_eq);
	   num += Crash_calcitems(GET_EQ(ch, j));
	  }
      cost += cost_eq;

      Crash_create_timer(iplayer, num);
      if (!num)
         {Crash_delete_file(GET_NAME(ch),CRASH_DELETE_OLD | CRASH_DELETE_NEW);
          return;
         }

      rent.net_cost_per_diem  = cost;
      rent.rentcode           = RENT_TIMEDOUT;
      rent.time               = time(0);
      rent.gold               = GET_GOLD(ch);
      rent.account            = GET_BANK_GOLD(ch);
      rent.nitems             = num;
      SAVEINFO(iplayer)->rent = rent;
      //sprintf(buf,"Save %d items, %d bytes\r\n", num, SAVESIZE(iplayer));
      //send_to_char(buf,ch);

      for (j = 0; j < NUM_WEARS; j++)
          {if (GET_EQ(ch, j))
              {Crash_save(iplayer, GET_EQ(ch, j), j + 1);
               Crash_restore_weight(GET_EQ(ch, j));
               Crash_extract_objs(GET_EQ(ch, j));
              }
          }
      Crash_save(iplayer, ch->carrying, 0);
      if (!Done_crashsave(iplayer))
         {sprintf(buf,"[IDLE ERROR] Cann't save items for char %s.",GET_NAME(ch));
          mudlog(buf, BRF, LVL_IMMORT, TRUE);

	  send_to_char("!!! Проблемы с записью Ваших вещей - обращайтесь к Богам !!!\r\n", ch);
         }	
      else
         {// sprintf(buf,"[IDLE OK] Store %d items for char %s.",num,GET_NAME(ch));
          // mudlog(buf, BRF, LVL_IMMORT, TRUE);
         }	  	
      Crash_extract_objs(ch->carrying);
      REMOVE_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);

     }
  else
     {sprintf(buf,"[IDLE ERROR] Cann't created descriptor for char %s.",GET_NAME(ch));
      mudlog(buf, BRF, LVL_IMMORT, TRUE);

      send_to_char("!!! Не могу создать дескриптор файла вещей - обращайтесь к Богам !!!\r\n", ch);
     }
}


void Crash_rentsave(struct char_data * ch, int cost)
{
  struct save_rent_info rent;
  int    j, num=0, iplayer=-1;

  if (IS_NPC(ch))
     return;

//  if (get_filename(GET_NAME(ch),buf,TIME_CRASH_FILE) &&
//      (iplayer = Init_crashsave(GET_NAME(ch))) >= 0
//     )
  if (get_filename(GET_NAME(ch),buf,TIME_CRASH_FILE))
     {if ((iplayer = Init_crashsave(GET_NAME(ch))) < 0)
         {sprintf(buf,"[SYSERR] Store file '%s' - INVALID ID.",GET_NAME(ch));
          mudlog(buf, NRM, MAX(LVL_IMMORT, LVL_IMMORT), TRUE);
          return;
	 }
      Crash_extract_norent_eq(ch);
      Crash_extract_norents(ch->carrying);

      num += Crash_calcitems(ch->carrying);
      for (j = 0; j < NUM_WEARS; j++)
	  num += Crash_calcitems(GET_EQ(ch, j));

      Crash_create_timer(iplayer, num);      	
      if (!num)
         {Crash_delete_file(GET_NAME(ch),CRASH_DELETE_OLD | CRASH_DELETE_NEW);
          return;
         }

      rent.net_cost_per_diem  = cost;
      rent.rentcode           = RENT_RENTED;
      rent.time               = time(0);
      rent.gold               = GET_GOLD(ch);
      rent.account            = GET_BANK_GOLD(ch);
      rent.nitems             = num;
      SAVEINFO(iplayer)->rent = rent;
      //sprintf(buf,"Save %d items, %d bytes\r\n", num, SAVESIZE(iplayer));
      //send_to_char(buf,ch);

      for (j = 0; j < NUM_WEARS; j++)
          if (GET_EQ(ch, j))
             {Crash_save(iplayer,GET_EQ(ch, j), j + 1);
              Crash_restore_weight(GET_EQ(ch, j));
              Crash_extract_objs(GET_EQ(ch, j));
             }
      Crash_save(iplayer, ch->carrying, 0);
      if (!Done_crashsave(iplayer))
         send_to_char("!!! Проблемы с записью Ваших вещей - обращайтесь к Богам !!!\r\n", ch);
      Crash_extract_objs(ch->carrying);
      REMOVE_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
     }
  else
     send_to_char("!!! Не могу создать дескриптор файла вещей - обращайтесь к Богам !!!\r\n", ch);
}


void Crash_cryosave(struct char_data * ch, int cost)
{
  struct save_rent_info rent;
  int    j, num=0, iplayer=-1;

  if (IS_NPC(ch))
     return;

  if (get_filename(GET_NAME(ch), buf, TIME_CRASH_FILE) &&
      (iplayer =Init_crashsave(GET_NAME(ch))) >= 0
     )
     {Crash_extract_norent_eq(ch);
      Crash_extract_norents(ch->carrying);

      num += Crash_calcitems(ch->carrying);
      for (j = 0; j < NUM_WEARS; j++)
	  num += Crash_calcitems(GET_EQ(ch, j));

      Crash_create_timer(iplayer, num);      	
      if (!num)
         {Crash_delete_file(GET_NAME(ch),CRASH_DELETE_OLD | CRASH_DELETE_NEW);
          return;
         }


      GET_GOLD(ch)            = MAX(0, GET_GOLD(ch) - cost);
      rent.rentcode           = RENT_CRYO;
      rent.time               = time(0);
      rent.gold               = GET_GOLD(ch);
      rent.account            = GET_BANK_GOLD(ch);
      rent.net_cost_per_diem  = 0;
      rent.nitems             = num;
      SAVEINFO(iplayer)->rent = rent;
      //sprintf(buf,"Save %d items, %d bytes\r\n", num, SAVESIZE(iplayer));
      //send_to_char(buf,ch);

      for (j = 0; j < NUM_WEARS; j++)
          if (GET_EQ(ch, j))
             {Crash_save(iplayer, GET_EQ(ch, j), j + 1);
              Crash_restore_weight(GET_EQ(ch, j));
              Crash_extract_objs(GET_EQ(ch, j));
             }
      Crash_save(iplayer, ch->carrying, 0);
      if (!Done_crashsave(iplayer))
         send_to_char("!!! Проблемы с записью Ваших вещей - обращайтесь к Богам !!!\r\n", ch);
      Crash_extract_objs(ch->carrying);
      SET_BIT(PLR_FLAGS(ch, PLR_CRYO), PLR_CRYO);
     }
  else
     send_to_char("!!! Не могу создать дескриптор файла вещей - обращайтесь к Богам !!!\r\n", ch);
}


/* ************************************************************************
* Routines used for the receptionist					  *
************************************************************************* */

void Crash_rent_deadline(struct char_data * ch, struct char_data * recep,
			             long cost)
{
  long rent_deadline;

  if (!cost)
     {send_to_char("Ты сможешь жить у меня до второго пришествия.\r\n",ch);
      return;
     }

  rent_deadline = ((GET_GOLD(ch) + GET_BANK_GOLD(ch)) / cost);
  sprintf(buf,
      "$n сказал$g Вам :\r\n"
      "\"Постой обойдется тебе в %ld %s.\"\r\n"
      "\"Твоих денег хватит на %ld %s.\"\r\n",
	  cost, desc_count(cost,WHAT_MONEYu),
	  rent_deadline, desc_count(rent_deadline, WHAT_DAY));
  act(buf, FALSE, recep, 0, ch, TO_VICT);
}

int Crash_report_unrentables(struct char_data * ch, struct char_data * recep,
			         struct obj_data * obj)
{
  char buf[128];
  int has_norents = 0;

  if (obj)
     {if (Crash_is_unrentable(obj))
         {has_norents = 1;
          sprintf(buf, "$n сказал$g Вам : \"Я не приму на постой %s.\"", OBJN(obj, ch, 3));
          act(buf, FALSE, recep, 0, ch, TO_VICT);
         }
      has_norents += Crash_report_unrentables(ch, recep, obj->contains);
      has_norents += Crash_report_unrentables(ch, recep, obj->next_content);
     }
  return (has_norents);
}



void Crash_report_rent(struct char_data * ch, struct char_data * recep,
		            struct obj_data * obj, int *cost, long *nitems, int display, int factor, int equip)
{
  static char buf[256];

  if (obj)
     {if (!Crash_is_unrentable(obj))
         {(*nitems)++;
          *cost += MAX(0, ((equip ? GET_OBJ_RENTEQ(obj) : GET_OBJ_RENT(obj)) * factor));
          if (display)
             {if (*nitems == 1)
                 act("$n сказал$g Вам : \"Итак :\"\r\n",
                     FALSE, recep, 0, ch, TO_VICT);
              sprintf(buf, "- %d %s за %s",
		              (equip ? GET_OBJ_RENTEQ(obj) : GET_OBJ_RENT(obj)) * factor,
		              desc_count((equip ? GET_OBJ_RENTEQ(obj) : GET_OBJ_RENT(obj)) * factor, WHAT_MONEYa),
		              OBJN(obj, ch, 3));
	          act(buf, FALSE, recep, 0, ch, TO_VICT);
             }
         }
      Crash_report_rent(ch, recep, obj->contains, cost, nitems, display, factor, FALSE);
      Crash_report_rent(ch, recep, obj->next_content, cost, nitems, display, factor, FALSE);
     }
}



int Crash_offer_rent(struct char_data * ch, struct char_data * receptionist,
		             int display, int factor, int *totalcost)
{
  char buf[MAX_INPUT_LENGTH];
  int  i, divide = 1;
  long numitems = 0, norent;

  *totalcost = 0;
  norent     = Crash_report_unrentables(ch, receptionist, ch->carrying);
  for (i = 0; i < NUM_WEARS; i++)
      norent += Crash_report_unrentables(ch, receptionist, GET_EQ(ch, i));

  if (norent)
     return (FALSE);

  *totalcost = min_rent_cost(ch) * factor;

  Crash_report_rent(ch, receptionist, ch->carrying,
                    totalcost, &numitems, display, factor, FALSE);

  for (i = 0; i < NUM_WEARS; i++)
      Crash_report_rent(ch, receptionist, GET_EQ(ch, i), totalcost, &numitems, display, factor, TRUE);

  if (!numitems)
     {act("$n сказал$g Вам : \"Но у тебя ведь ничего нет ! Просто набери \"конец\" !\"",
	      FALSE, receptionist, 0, ch, TO_VICT);
      return (FALSE);
     }
  if (numitems > max_obj_save)
     {sprintf(buf, "$n сказал$g Вам : \"Извините, но я не могу хранить больше %d предметов.\"",
	          max_obj_save);
      act(buf, FALSE, receptionist, 0, ch, TO_VICT);
      return (FALSE);
     }

  divide    = GET_LEVEL(ch) <= 15 ? 2 : 1;

  if (display)
     {if (min_rent_cost(ch))
         {sprintf(buf, "$n сказал$g Вам : \"И еще %d %s мне на чай :)\"",
                  min_rent_cost(ch) * factor,
	          desc_count(min_rent_cost(ch) * factor,WHAT_MONEYu));
          act(buf, FALSE, receptionist, 0, ch, TO_VICT);
         }
      sprintf(buf, "$n сказал$g Вам : \"В сумме это составит %d %s %s.\"",
              *totalcost,
	          desc_count(*totalcost,WHAT_MONEYu),
	          (factor == RENT_FACTOR ? "в день " : "")
	          );
      act(buf, FALSE, receptionist, 0, ch, TO_VICT);
      if (MAX(0,*totalcost/divide) > GET_GOLD(ch) + GET_BANK_GOLD(ch))
         {act("\"...которых у тебя отродясь не было.\"",
	          FALSE, receptionist, 0, ch, TO_VICT);
          return (FALSE);
         };

      *totalcost = MAX(0, *totalcost/divide);
      if (divide > 1)
         act("\"Так уж и быть, я скощу тебе половину.\"",
             FALSE, receptionist,0,ch,TO_VICT);
      if (factor == RENT_FACTOR)
         Crash_rent_deadline(ch, receptionist, *totalcost);
     }
  else
     *totalcost = MAX(0, *totalcost/divide);
  return (TRUE);
}



int gen_receptionist(struct char_data * ch, struct char_data * recep,
		         int cmd, char *arg, int mode)
{
  room_rnum save_room;
  int cost, rentshow = TRUE;

  if (!ch->desc || IS_NPC(ch))
     return (FALSE);

  if (!cmd && !number(0, 5))
     return (FALSE);

  if (!CMD_IS("offer")  && !CMD_IS("rent") &&
      !CMD_IS("постой") && !CMD_IS("предложение"))
     return (FALSE);
  if (!AWAKE(recep))
     {sprintf(buf, "%s не в состоянии говорить с Вами...\r\n", HSSH(recep));
      send_to_char(buf, ch);
      return (TRUE);
     }
  if (!CAN_SEE(recep, ch))
     {act("$n сказал$g : \"Не люблю говорить с теми, кого я не вижу !\"", FALSE, recep, 0, 0, TO_ROOM);
      return (TRUE);
     }
  if (RENTABLE(ch))
     {send_to_char("В связи с боевыми действиями эвакуация временно прекращена.\r\n",ch);
      return (TRUE);
     }
  if (FIGHTING(ch))
     {return (FALSE);
     }
  if (free_rent)
     {act("$n сказал$g Вам : \"Сегодня спим нахаляву.  Наберите просто \"конец\".\"",
	     FALSE, recep, 0, ch, TO_VICT);
      return (1);
     }
  if (CMD_IS("rent") || CMD_IS("постой"))
     {if (!Crash_offer_rent(ch, recep, rentshow, mode,&cost))
         return (TRUE);

      if (!rentshow)
         {if (mode == RENT_FACTOR)
             sprintf(buf, "$n сказал$g Вам : \"Дневной постой обойдется тебе в %d %s.\"",
                     cost, desc_count(cost, WHAT_MONEYu));
          else
          if (mode == CRYO_FACTOR)
             sprintf(buf, "$n сказал$g Вам : \"Дневной постой обойдется тебе в %d %s (за пользование холодильником :)\"",
                     cost, desc_count(cost, WHAT_MONEYu));
          act(buf, FALSE, recep, 0, ch, TO_VICT);

          if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch))
             {act("$n сказал$g Вам : '..но такой голотьбе, как ты, это не по карману.'",
	              FALSE, recep, 0, ch, TO_VICT);
              return (TRUE);
             }
          if (cost && (mode == RENT_FACTOR))
             Crash_rent_deadline(ch, recep, cost);
         }

      if (mode == RENT_FACTOR)
         {act("$n запер$q Ваши вещи в сундук и повел в тесную каморку.",
	          FALSE, recep, 0, ch, TO_VICT);
          Crash_rentsave(ch, cost);
          sprintf(buf, "%s has rented (%d/day, %ld tot.)",
                  GET_NAME(ch),  cost, GET_GOLD(ch) + GET_BANK_GOLD(ch));
         }
      else
         {/* cryo */
          act("$n запер$q Ваши вещи в сундук и повел в тесную каморку.\r\n"
	          "Белый призрак появился в комнате, обдав Вас холодом...\r\n"
	          "Вы потеряли связь с окружающими Вас...",
	          FALSE, recep, 0, ch, TO_VICT);
          Crash_cryosave(ch, cost);
          sprintf(buf, "%s has cryo-rented.", GET_NAME(ch));
          SET_BIT(PLR_FLAGS(ch, PLR_CRYO), PLR_CRYO);
         }

      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      save_room = ch->in_room;

      if (save_room == r_named_start_room)
         act("$n проводил$g $N3 мощным пинком на свободную лавку.", FALSE, recep, 0, ch, TO_ROOM);
      else
      if (save_room == r_helled_start_room || save_room == r_unreg_start_room)
         act("$n проводил$g $N3 мощным пинком на свободные нары.", FALSE, recep, 0, ch, TO_ROOM);
      else
         act("$n помог$q $N2 отойти ко сну.", FALSE, recep, 0, ch, TO_NOTVICT);
      extract_char(ch,FALSE);
      if (save_room != r_helled_start_room &&
          save_room != r_named_start_room  &&
	  save_room != r_unreg_start_room
         )
         GET_LOADROOM(ch) = GET_ROOM_VNUM(save_room);
      save_char(ch, save_room);
     }
  else
     {Crash_offer_rent(ch, recep, TRUE, mode, &cost);
      act("$N предложил$G $n2 остановиться у н$S.", FALSE, ch, 0, recep, TO_ROOM);
     }
  return (TRUE);
}


SPECIAL(receptionist)
{
  return (gen_receptionist(ch, (struct char_data *)me, cmd, argument, RENT_FACTOR));
}


SPECIAL(cryogenicist)
{
  return (gen_receptionist(ch, (struct char_data *)me, cmd, argument, CRYO_FACTOR));
}


void Crash_frac_save_all(int frac_part)
{struct descriptor_data *d;
 int    i;

 for (d = descriptor_list; d; d = d->next)
     {if ((STATE(d) == CON_PLAYING)               &&
          !IS_NPC(d->character)                   &&
	  GET_ACTIVITY(d->character) == frac_part &&
	  PLR_FLAGGED(d->character, PLR_CRASH)
	 )	
         {Crash_crashsave(d->character);
          save_char(d->character, NOWHERE);
	  REMOVE_BIT(PLR_FLAGS(d->character, PLR_CRASH), PLR_CRASH);
         }
     }
 for (i = 0; i <= top_of_p_table; i++)
     if (player_table[i].level >= 0     &&
         player_table[i].level <= 5     &&
         player_table[i].last_logon > 0 &&
	 player_table[i].last_logon +
	 60 * 60 * 24 * MAX(1,player_table[i].level) < time(0)
	)
	{log("%d=%d=%s",i,player_table[i].level,player_table[i].name);
	 if (delete_char(player_table[i].name))
	    {sprintf(buf,"%s (lev %d) auto deleted",
	             player_table[i].name, player_table[i].level);
             mudlog(buf, NRM, LVL_GOD, TRUE);
	    }
	 player_table[i].last_logon = -1;
	 player_table[i].level      = -1;
	}
}

void Crash_save_all(void)
{
  struct descriptor_data *d;
  for (d = descriptor_list; d; d = d->next)
      {if ((STATE(d) == CON_PLAYING) && !IS_NPC(d->character))
          {if (PLR_FLAGGED(d->character, PLR_CRASH))
              {Crash_crashsave(d->character);
               save_char(d->character, NOWHERE);
               REMOVE_BIT(PLR_FLAGS(d->character, PLR_CRASH), PLR_CRASH);
              }
          }
      }
}

long old_crash_save[OBJECT_SAVE_ACTIVITY] = {-1};

void Crash_frac_rent_time(int frac_part)
{int dectime = 0, c;

 if (*old_crash_save == -1)
    memset(&old_crash_save,0x00,sizeof(old_crash_save));


 if (!*(old_crash_save+frac_part))
    *(old_crash_save+frac_part) = time(NULL) - time(NULL) % 60;
 else
    {dectime = time(NULL) - *(old_crash_save+frac_part);
     if ((dectime /= 60))
        *(old_crash_save+frac_part) = time(NULL) - time(NULL) % 60;
    }
 for (c = 0; dectime && c <= top_of_p_table; c++)
     if (player_table[c].activity == frac_part &&
         player_table[c].unique   != -1        &&
         SAVEINFO(c)
        )
        Crash_timer_obj(c, dectime);
}

void Crash_rent_time(int dectime)
{int c;
 for (c = 0; c <= top_of_p_table; c++)
     if (player_table[c].unique != -1)
        Crash_timer_obj(c, dectime);
}
