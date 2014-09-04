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

/* Extern functions */
ACMD(do_tell);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
int  invalid_no_class(struct char_data *ch, struct obj_data *obj);
int  invalid_anti_class(struct char_data *ch, struct obj_data *obj);
int  invalid_unique(struct char_data *ch, struct obj_data *obj);
int  min_rent_cost(struct char_data *ch);
void name_from_drinkcon(struct obj_data * obj);
void name_to_drinkcon(struct obj_data * obj, int type);
int  delete_char(char *name);
/* local functions */
void Crash_extract_norent_eq(struct char_data *ch);
int  auto_equip(struct char_data *ch, struct obj_data *obj, int location);
int Crash_offer_rent(struct char_data * ch, struct char_data * receptionist, int display, int factor, int *totalcost);
int Crash_report_unrentables(struct char_data * ch, struct char_data * recep, struct obj_data * obj);
void Crash_report_rent(struct char_data * ch, struct char_data * recep, struct obj_data * obj, int *cost, long *nitems, int display, int factor, int equip);
struct obj_data *Obj_from_store(struct obj_file_elem object, int *location);
int Obj_to_store(struct obj_data * obj, FILE * fl, int location);
void update_obj_file(void);
int Crash_write_rentcode(struct char_data * ch, FILE * fl, struct rent_info * rent);
int gen_receptionist(struct char_data * ch, struct char_data * recep, int cmd, char *arg, int mode);
int Crash_save(struct obj_data * obj, FILE * fp, int location);
void Crash_rent_deadline(struct char_data * ch, struct char_data * recep, long cost);
void Crash_restore_weight(struct obj_data * obj);
void Crash_extract_objs(struct obj_data * obj);
int Crash_is_unrentable(struct obj_data * obj);
void Crash_extract_norents(struct obj_data * obj);
void Crash_extract_expensive(struct obj_data * obj);
void Crash_calculate_rent(struct obj_data * obj, int *cost);
void Crash_rentsave(struct char_data * ch, int cost);
void Crash_cryosave(struct char_data * ch, int cost);


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


int Crash_delete_file(char *name)
{
  char filename[50];
  FILE *fl;

  if (!get_filename(name, filename, CRASH_FILE))
     return (0);
  if (!(fl = fopen(filename, "rb")))
     {if (errno != ENOENT)	/* if it fails but NOT because of no file */
         log("SYSERR: deleting crash file %s (1): %s", filename, strerror(errno));
      return (0);
     }
  fclose(fl);

  /* if it fails, NOT because of no file */
  if (remove(filename) < 0 && errno != ENOENT)
     log("SYSERR: deleting crash file %s (2): %s", filename, strerror(errno));

  return (1);
}


int Crash_delete_crashfile(struct char_data * ch)
{
  char fname[MAX_INPUT_LENGTH];
  struct rent_info rent;
  FILE *fl;

  if (!get_filename(GET_NAME(ch), fname, CRASH_FILE))
     return (0);
  if (!(fl = fopen(fname, "rb")))
     {if (errno != ENOENT)	/* if it fails, NOT because of no file */
         log("SYSERR: checking for crash file %s (3): %s", fname, strerror(errno));
      return (0);
     }
  if (!feof(fl))
     fread(&rent, sizeof(struct rent_info), 1, fl);
  fclose(fl);

  if (rent.rentcode == RENT_CRASH)
     Crash_delete_file(GET_NAME(ch));

  return (1);
}


int Crash_clean_file(char *name)
{
  char fname[MAX_STRING_LENGTH], filetype[20];
  struct rent_info rent;
  FILE *fl;

  if (!get_filename(name, fname, CRASH_FILE))
     return (0);
  /*
   * open for write so that permission problems will be flagged now, at boot
   * time.
   */
  if (!(fl = fopen(fname, "r+b")))
     {if (errno != ENOENT)	/* if it fails, NOT because of no file */
         log("SYSERR: OPENING OBJECT FILE %s (4): %s", fname, strerror(errno));
      return (0);
     }
  if (!feof(fl))
     fread(&rent, sizeof(struct rent_info), 1, fl);
  fclose(fl);

  if ((rent.rentcode == RENT_CRASH) ||
      (rent.rentcode == RENT_FORCED) ||
      (rent.rentcode == RENT_TIMEDOUT))
     {if (rent.time < time(0) - (crash_file_timeout * SECS_PER_REAL_DAY))
         {Crash_delete_file(name);
          switch (rent.rentcode)
          {
      case RENT_CRASH:
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
          return (1);
         }
    /* Must retrieve rented items w/in 30 days */
     }
  else
  if (rent.rentcode == RENT_RENTED)
     if (rent.time < time(0) - (rent_file_timeout * SECS_PER_REAL_DAY))
        {Crash_delete_file(name);
         log("    Deleting %s's rent file.", name);
         return (1);
        }
  return (0);
}



void update_obj_file(void)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
      if (*player_table[i].name)
         Crash_clean_file(player_table[i].name);
}


/********** THIS ROUTINE CALC ITEMS IN CHAR EQUIP ********************/
void Crash_calc_objects(char *name)
{
  FILE   *fl;
  char   fname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  int    count = 0, rnum;
  struct rent_info rent;

#ifndef USE_AUTOEQ
  return;
#endif

  if (!get_filename(name, fname, CRASH_FILE))
     return;
  if (!(fl = fopen(fname, "rb")))
     {log("RENT CALC : %s has no rent file.\r\n", name);
      return;
     }

  if (!feof(fl))
     fread(&rent, sizeof(struct rent_info), 1, fl);
  sprintf (buf,"RENT CALC for %s :", name);
  switch (rent.rentcode)
  {
  case RENT_RENTED:
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
            {fclose(fl);
             break;
            }
         if (!feof(fl))
            if (object.location != ITEM_DESTROYED &&
                (rnum = real_object(object.item_number)) >= 0)
               {count++;
                obj_index[rnum].stored++;
               }
        }
  log("%s All objects = %d", buf, count);
  fclose(fl);
}

void Crash_timer_obj(char *name, long timer_dec)
{
  FILE   *fl;
  char   fname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  struct obj_file_elem *objects = NULL;
  int    reccount = 0, count = 0, delete = 0, rnum, i;
  long   size;
  struct rent_info rent;


#ifndef USE_AUTOEQ
        return;
#endif

  if (!get_filename(name, fname, CRASH_FILE))
     return;

  if (!(fl = fopen(fname, "rb")))
     {log("RENT TIMER %s has no rent file.", name);
      return;
     }
  fseek(fl, 0L, SEEK_END);
  size = ftell(fl);
  if (size < sizeof(struct rent_info))
     {log("RENT TIMER %s - error structure of file.", name);
      fclose(fl);
      return;
     }
  rewind(fl);
  fread(&rent, sizeof(struct rent_info), 1, fl);
  size    -= sizeof(struct rent_info);
  reccount = MAX(0, MIN(500, size / sizeof(struct obj_file_elem)));
  *buf = '\0';
  if (reccount)
     {CREATE(objects, struct obj_file_elem, reccount);
      reccount = fread(objects, sizeof(struct obj_file_elem), reccount, fl);
      reccount = MIN(500,reccount);
     }
  fclose(fl);

  sprintf (buf,"TIMER CALC for %s(%d items) :", name, reccount);

  switch (rent.rentcode)
  {
  case RENT_RENTED:
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
                   delete++;
                   if (rnum >= 0)
                      {if (obj_index[rnum].stored > 0)
                          obj_index[rnum].stored--;
                       log("Player %s : item %s deleted - time outted",name,(obj_proto+rnum)->PNames[0]);
                      }
                  };
              }
          }

      if ((fl = fopen(fname,"w+b")))
         {fwrite(&rent, sizeof(struct rent_info), 1, fl);
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
     log("%s Objects-%d,  Delete-%d,  Timer-%ld", buf, count, delete, timer_dec);
  else
     log("ERROR recognised %s", buf);
}


void Crash_listrent(struct char_data * ch, char *name)
{
  FILE *fl;
  char fname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  struct obj_data *obj;
  struct rent_info rent;

  if (!get_filename(name, fname, CRASH_FILE))
     return;
  if (!(fl = fopen(fname, "rb")))
     {sprintf(buf, "%s has no rent file.\r\n", name);
      send_to_char(buf, ch);
      return;
     }
  sprintf(buf, "%s\r\n", fname);
  if (!feof(fl))
     fread(&rent, sizeof(struct rent_info), 1, fl);
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
            {fclose(fl);
             return;
            }
         if (!feof(fl))
            if (real_object(object.item_number) > -1)
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
  send_to_char(buf, ch);
  fclose(fl);
}



int Crash_write_rentcode(struct char_data * ch, FILE * fl, struct rent_info * rent)
{
  if (fwrite(rent, sizeof(struct rent_info), 1, fl) < 1)
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
struct container_list_type
{struct obj_data            *tank;
 struct container_list_type *next;
 int    location;
};
 
int Crash_load(struct char_data * ch)
{
  FILE  *fl;
  char   fname[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  struct rent_info rent;
  int    cost, orig_rent_code, num_objs = 0, j;
  float  num_of_days;
  struct obj_data *obj, *obj2, *cont_row[MAX_BAG_ROWS], *obj_list=NULL;
  /* AutoEQ addition. */
  int    location;
  struct container_list_type *tank_list = NULL, *tank;

  /* Empty all of the container lists (you never know ...) */
  for (j = 0; j < MAX_BAG_ROWS; j++)
      cont_row[j] = NULL;
  if (!get_filename(GET_NAME(ch), fname, CRASH_FILE))
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
      Crash_timer_obj(GET_NAME(ch), 1000000L);
      return (1);
     }
  if (!feof(fl))
      fread(&rent, sizeof(struct rent_info), 1, fl);
  else
      {log("SYSERR: Crash_load: %s's rent file was empty!", GET_NAME(ch));
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
         {fclose(fl);
          sprintf(buf, "%s entering game, rented equipment lost (no $).",
	              GET_NAME(ch));
          mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
          GET_GOLD(ch) = GET_BANK_GOLD(ch) = 0;
          Crash_timer_obj(GET_NAME(ch), 1000000L);
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
             fclose(fl);
	     break;
             return (1);
            }
         if (feof(fl))
            break;
         ++num_objs;
         if ((obj = Obj_from_store(object, &location)) == NULL)
            {send_to_char("Что-то рассыпалось от длительного использования.\r\n",ch);
             continue;
            }
         // Check timer
         if (last_rent_check > time(NULL))
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
            /*
         location = auto_equip(ch, obj, location);
         //
         // What to do with a new loaded item:
         //
         // If there's a list with location less than 1 below this, then its
         // container has disappeared from the file so we put the list back into
         // the character's inventory. (Equipped items are 0 here.)
         //
         // If there's a list of contents with location of 1 below this, then we
         // check if it is a container:
         //   - Yes: Get it from the character, fill it, and give it back so we
         //          have the correct weight.
         //   -  No: The container is missing so we put everything back into the
         //          character's inventory.
         //
         // For items with negative location, we check if there is already a list
         // of contents with the same location.  If so, we put it there and if not,
         // we start a new list.
         //
         // Since location for contents is < 0, the list indices are switched to
         // non-negative.
         //
         // This looks ugly, but it works.
         //
            
         if (location > 0)
            {// Equipped 
             for (j = MAX_BAG_ROWS - 1; j > 0; j--)
                 {if (cont_row[j])
                     {// No container, back to inventory.
                      for (; cont_row[j]; cont_row[j] = obj2)
                          {obj2 = cont_row[j]->next_content;
                           obj_to_char(cont_row[j], ch);
                          }
                      cont_row[j] = NULL;
                     }
                 }
             if (cont_row[0])
                {// Content list existing. 
                 if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
                    {// Remove object, fill it, equip again. 
                     obj = unequip_char(ch,(location - 1)|0x80);
                     obj->contains = NULL; // Should be NULL anyway, but just in case.
                     for (; cont_row[0]; cont_row[0] = obj2)
                         {obj2 = cont_row[0]->next_content;
                          obj_to_obj(cont_row[0], obj);
                         }
                     equip_char(ch, obj,(location - 1)|0x80|0x40);
                    }
                 else
                    {// Object isn't container, empty the list. 
                     for (; cont_row[0]; cont_row[0] = obj2)
                         {obj2 = cont_row[0]->next_content;
                          obj_to_char(cont_row[0], ch);
                         }
                     cont_row[0] = NULL;
                    }
                }
            }
         else
            {// location <= 0
             for (j = MAX_BAG_ROWS - 1; j > -location; j--)
                 {if (cont_row[j])
                     {// No container, back to inventory.
                      for (; cont_row[j]; cont_row[j] = obj2)
                          {obj2 = cont_row[j]->next_content;
                           obj_to_char(cont_row[j], ch);
                          }
                      cont_row[j] = NULL;
                     }
                 }
             if (j == -location && cont_row[j])
                {// Content list exists.
                 if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
                    {// Take the item, fill it, and give it back.
                     obj_from_char(obj);
                     obj->contains = NULL;
                     for (; cont_row[j]; cont_row[j] = obj2)
                         {obj2 = cont_row[j]->next_content;
                          obj_to_obj(cont_row[j], obj);
                         }
                     obj_to_char(obj, ch);	// Add to inventory first.
                    }
                 else
                    {// Object isn't container, empty content list.
                     for (; cont_row[j]; cont_row[j] = obj2)
                         {obj2 = cont_row[j]->next_content;
                          obj_to_char(cont_row[j], ch);
                         }
                     cont_row[j] = NULL;
                    }
                }
             if (location < 0 && location >= -MAX_BAG_ROWS)
                {//
                 // Let the object be part of the content list but put it at the
                 // list's end.  Thus having the items in the same order as before
                 // the character rented.
                 //
                 obj_from_char(obj);
                 if ((obj2 = cont_row[-location - 1]) != NULL)
                    {while (obj2->next_content)
                           obj2 = obj2->next_content;
                     obj2->next_content = obj;
                    }
                 else
                    cont_row[-location - 1] = obj;
                }
            }
            */
         obj->next_content = obj_list;
         obj_list          = obj;
         obj->worn_on      = location;
	 /* sprintf(buf,"You have %s at %d.\r\n",obj->PNames[0],location);
	    send_to_char(buf,ch); */
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
	   /* sprintf(buf,"Attempt equip you %s at %d.\r\n",obj->PNames[0],location);
	      send_to_char(buf,ch); */
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
              {/* sprintf(buf,"Attempt put %s to %s.\r\n",obj->PNames[0],tank_list->tank->PNames[0]);
                  send_to_char(buf,ch); */
	       obj_to_obj(obj,tank_list->tank);
	      }
           else
	      {/* sprintf(buf,"Attempt give you %s.\r\n",obj->PNames[0]);
                  send_to_char(buf,ch); */
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
  sprintf(fname, "%s (level %d) has %d objects (max %d).",
	      GET_NAME(ch), GET_LEVEL(ch), num_objs, max_obj_save);
  mudlog(fname, NRM, MAX(GET_INVIS_LEV(ch), LVL_GOD), TRUE);

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



int Crash_save(struct obj_data * obj, FILE * fp, int location)
{
  struct obj_data *tmp;
  int result;

  if (obj)
     {Crash_save(obj->next_content, fp, location);
      Crash_save(obj->contains, fp, MIN(0, location) - 1);
      result = Obj_to_store(obj, fp, location);

      for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
          GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

      if (!result)
         return (0);
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


void Crash_crashsave(struct char_data * ch)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch))
     return;

  if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
     return;
  if (!(fp = fopen(buf, "wb")))
     return;

  rent.rentcode = RENT_CRASH;
  rent.time     = time(0);
  if (!Crash_write_rentcode(ch, fp, &rent))
     {fclose(fp);
      return;
     }

  for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(ch, j))
         {if (!Crash_save(GET_EQ(ch, j), fp, j + 1))
             {fclose(fp);
	          return;
             }
          Crash_restore_weight(GET_EQ(ch, j));
         }

  if (!Crash_save(ch->carrying, fp, 0))
     {fclose(fp);
      return;
     }

  Crash_restore_weight(ch->carrying);

  fclose(fp);
  REMOVE_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
}


void Crash_idlesave(struct char_data * ch)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  int cost, cost_eq;
  FILE *fp;

  if (IS_NPC(ch))
     return;

  if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
     return;
  if (!(fp = fopen(buf, "wb")))
     return;

  Crash_extract_norent_eq(ch);
  Crash_extract_norents(ch->carrying);

  cost = 0;
  Crash_calculate_rent(ch->carrying, &cost);

  cost_eq = 0;
  for (j = 0; j < NUM_WEARS; j++)
      Crash_calculate_rent(GET_EQ(ch, j), &cost_eq);

  cost += cost_eq;
/*
  cost *= 2;			// forcerent cost is 2x normal rent

  if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch))
     {for (j = 0; j < NUM_WEARS; j++)	// Unequip players with low gold.
          if (GET_EQ(ch, j))
             obj_to_char(unequip_char(ch, j), ch);

      while ((cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) && ch->carrying)
            {Crash_extract_expensive(ch->carrying);
             cost = 0;
             Crash_calculate_rent(ch->carrying, &cost);
             cost *= 2;
            }
     }
*/

  if (ch->carrying == NULL)
     {for (j = 0; j < NUM_WEARS && GET_EQ(ch, j) == NULL; j++)
          // Nothing;
      if (j == NUM_WEARS)
         {// No equipment or inventory.
          fclose(fp);
          Crash_delete_file(GET_NAME(ch));
          return;
         }
     }


  rent.net_cost_per_diem = cost;
  rent.rentcode   = RENT_TIMEDOUT;
  rent.time       = time(0);
  rent.gold       = GET_GOLD(ch);
  rent.account    = GET_BANK_GOLD(ch);
  if (!Crash_write_rentcode(ch, fp, &rent))
     {fclose(fp);
      return;
     }
  for (j = 0; j < NUM_WEARS; j++)
      {if (GET_EQ(ch, j))
          {if (!Crash_save(GET_EQ(ch, j), fp, j + 1))
              {fclose(fp);
               return;
              }
           Crash_restore_weight(GET_EQ(ch, j));
           Crash_extract_objs(GET_EQ(ch, j));
          }
      }
  if (!Crash_save(ch->carrying, fp, 0))
     {fclose(fp);
      return;
     }
  fclose(fp);

  Crash_extract_objs(ch->carrying);
}


void Crash_rentsave(struct char_data * ch, int cost)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch))
     return;

  if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
     return;
  if (!(fp = fopen(buf, "wb")))
     return;

  Crash_extract_norent_eq(ch);
  Crash_extract_norents(ch->carrying);

  rent.net_cost_per_diem = cost;
  rent.rentcode          = RENT_RENTED;
  rent.time              = time(0);
  rent.gold              = GET_GOLD(ch);
  rent.account           = GET_BANK_GOLD(ch);
  if (!Crash_write_rentcode(ch, fp, &rent))
     {fclose(fp);
      return;
     }
  for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(ch, j))
         {if (!Crash_save(GET_EQ(ch,j), fp, j + 1))
             {fclose(fp);
              return;
             }
          Crash_restore_weight(GET_EQ(ch, j));
          Crash_extract_objs(GET_EQ(ch, j));
         }
 if (!Crash_save(ch->carrying, fp, 0))
    {fclose(fp);
     return;
    }
  fclose(fp);

  Crash_extract_objs(ch->carrying);
}


void Crash_cryosave(struct char_data * ch, int cost)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch))
     return;

  if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
     return;
  if (!(fp = fopen(buf, "wb")))
     return;

  Crash_extract_norent_eq(ch);
  Crash_extract_norents(ch->carrying);

  GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - cost);

  rent.rentcode = RENT_CRYO;
  rent.time     = time(0);
  rent.gold     = GET_GOLD(ch);
  rent.account  = GET_BANK_GOLD(ch);
  rent.net_cost_per_diem = 0;
  if (!Crash_write_rentcode(ch, fp, &rent))
     {fclose(fp);
      return;
     }
  for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(ch, j))
         {if (!Crash_save(GET_EQ(ch, j), fp, j + 1))
             {fclose(fp);
              return;
             }
          Crash_restore_weight(GET_EQ(ch, j));
          Crash_extract_objs(GET_EQ(ch, j));
         }
  if (!Crash_save(ch->carrying, fp, 0))
     {fclose(fp);
      return;
     }
  fclose(fp);

  Crash_extract_objs(ch->carrying);
  SET_BIT(PLR_FLAGS(ch, PLR_CRYO), PLR_CRYO);
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


#define FIRST_CHAR           ('0')
#define LAST_CHAR            ('Z')
#define COMBINATIONS         ((LAST_CHAR - FIRST_CHAR + 1) * (LAST_CHAR - FIRST_CHAR + 1))
#define COMB_DIV             (COMBINATIONS / SAVE_SUBPARTS)
#define COMB_MOD             (COMBINATIONS % SAVE_SUBPARTS)
#define MIN_TO_SAVE          6
#define MIN_CONST            60
#define SAVE_SUBPARTS        (MIN_CONST * MIN_TO_SAVE)
#define FIRST_CHARS          ((FIRST_CHAR << 8) + FIRST_CHAR)
#define LAST_CHARS           ((LAST_CHAR  << 8) + LAST_CHAR)

void Crash_frac_save_all(int frac_part)
{static unsigned int sum_min=0, sum_max=0, sum_rest=0;
 int    rest,i;
 struct descriptor_data *d;
 char   a,b;
 if (!(rest = frac_part % (SAVE_SUBPARTS+1)))
    {sum_min = 0;
     sum_rest= 0;
     sum_max = FIRST_CHARS;
    }
 else
 if (rest == SAVE_SUBPARTS)
    {sum_min = sum_max + 1;
     sum_max = 0xFFFF;
    }
 else
 if (!sum_max)
    {sum_min = FIRST_CHARS + (LAST_CHARS - FIRST_CHARS) * rest / SAVE_SUBPARTS;
     sum_max = (sum_min & 0xFF00) | 0x00FF;
    }
 else
    {sum_min = sum_max + 1;
     if ((sum_min & 0x00FF) < (ubyte) FIRST_CHAR)
        sum_min = (sum_min & 0xFF00) + FIRST_CHAR;
     else
     if ((sum_min & 0x00FF) > (ubyte) LAST_CHAR)
        sum_min = ((sum_min & 0xFF00) + 0x0100 + FIRST_CHAR);
     sum_max = sum_min + COMB_DIV;
     if ((sum_rest += COMB_MOD) > SAVE_SUBPARTS)
        {sum_max++;
         sum_rest -= SAVE_SUBPARTS;
        }
     if ((sum_max & 0x00FF) > (ubyte) LAST_CHAR)
        {rest = (sum_max & 0x00FF) - (ubyte) LAST_CHAR;
         sum_max = (sum_max & 0xFF00) + 0xFF + rest;
        }
    }
// log("[FRAC] Saving players from %d to %d...",sum_min,sum_max);
 for (d = descriptor_list; d; d = d->next)
     {if ((STATE(d) == CON_PLAYING) && !IS_NPC(d->character))
         {if (PLR_FLAGGED(d->character, PLR_CRASH))
             {a = *(GET_NAME(d->character)+0);
              b = *(GET_NAME(d->character)+1);
              rest = (UPPER(AtoL(a)) << 8) + UPPER(AtoL(b));
              if (rest >= sum_min && rest <= sum_max)
                 {// log("[FRAC] Save %s (%d)",GET_NAME(d->character),rest);
                  Crash_crashsave(d->character);
                  save_char(d->character, NOWHERE);
	          REMOVE_BIT(PLR_FLAGS(d->character, PLR_CRASH), PLR_CRASH);
	         }
             }
         }
     }
 for (i = 0; i <= top_of_p_table; i++)
     if (player_table[i].level >= 0     &&
         player_table[i].level <= 5     &&
         player_table[i].last_logon > 0 &&
	 player_table[i].last_logon + 
	 60 * 60 * 24 * MAX(1,player_table[i].level) < time(0)
	)
	{if (*player_table[i].name == '!')
            continue;
         if (!delete_char(player_table[i].name))
	    *player_table[i].name = '!';
	 sprintf(buf,"%s (lev %d) auto deleted",
	         player_table[i].name, player_table[i].level);
	 mudlog(buf, NRM, LVL_GOD, TRUE);
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

long old_crash_save[SAVE_SUBPARTS] = {-1};

void Crash_frac_rent_time(int frac_part)
{static unsigned int sum_min=0, sum_max=0, sum_rest=0;
 int    c, rest, dectime=0,crest;
 char   a,b;

 if (*old_crash_save == -1)
    memset(&old_crash_save,0x00,sizeof(old_crash_save));

 if (!(rest = frac_part % (SAVE_SUBPARTS+1)))
    {sum_min = 0;
     sum_rest= 0;
     sum_max = FIRST_CHARS;
    }
 else
 if (rest == SAVE_SUBPARTS)
    {sum_min = sum_max + 1;
     sum_max = 0xFFFF;
    }
 else
 if (!sum_max)
    {sum_min = FIRST_CHARS + (LAST_CHARS - FIRST_CHARS) * rest / SAVE_SUBPARTS;
     sum_max = (sum_min & 0xFF00) | 0x00FF;
    }
 else
    {sum_min = sum_max + 1;
     if ((sum_min & 0x00FF) < (ubyte) FIRST_CHAR)
        sum_min = (sum_min & 0xFF00) + FIRST_CHAR;
     else
     if ((sum_min & 0x00FF) > (ubyte) LAST_CHAR)
        sum_min = ((sum_min & 0xFF00) + 0x0100 + FIRST_CHAR);
     sum_max = sum_min + COMB_DIV;
     if ((sum_rest += COMB_MOD) > SAVE_SUBPARTS)
        {sum_max++;
         sum_rest -= SAVE_SUBPARTS;
        }
     if ((sum_max & 0x00FF) > (ubyte) LAST_CHAR)
        {crest = (sum_max & 0x00FF) - (ubyte) LAST_CHAR;
         sum_max = (sum_max & 0xFF00) + 0xFF + crest;
        }
    }

 if (!*(old_crash_save+rest))
    *(old_crash_save+rest) = time(NULL) - time(NULL) % 60;
 else
    {dectime = time(NULL) - *(old_crash_save+rest);
     if ((dectime /= 60))
        *(old_crash_save+rest) = time(NULL) - time(NULL) % 60;
    }
// log("[FRAC] Saving rent file from %d to %d...",sum_min,sum_max);
 for (c = 0; dectime && c <= top_of_p_table; c++)
     if (player_table[c].unique != -1)
        {a = *(player_table[c].name+0);
         b = *(player_table[c].name+1);
         rest = (UPPER(AtoL(a)) << 8) + UPPER(AtoL(b));
         if (rest >= sum_min && rest <= sum_max)
            Crash_timer_obj(player_table[c].name, dectime);
        }
}

void Crash_rent_time(int dectime)
{int c;
 for (c = 0; c <= top_of_p_table; c++)
     if (player_table[c].unique != -1)
        Crash_timer_obj(player_table[c].name, dectime);
}