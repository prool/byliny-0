/* ************************************************************************
*   File: house.c                                       Part of CircleMUD *
*  Usage: Handling of player houses                                       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "house.h"
#include "constants.h"
#include "screen.h"

extern struct room_data *world;
extern struct index_data *obj_index;
extern struct index_data *mob_index;
extern struct descriptor_data *descriptor_list;
extern const  char *Dirs[];

struct obj_data *Obj_from_store(struct obj_file_elem object, int *location);
int Obj_to_store(struct obj_data * obj, FILE * fl, int location);

struct house_control_rec house_control[MAX_HOUSES];
int num_of_houses = 0;

/* local functions */
int  House_get_filename(int vnum, char *filename);
int  House_load(room_vnum house_num);
int  House_save(struct obj_data * obj, FILE * fp, int in_room);
void House_restore_weight(struct obj_data * obj);
void House_delete_file(int house_num);
long House_create_unique(void);
int  find_house(room_vnum vnum);
void House_save_control(void);
void hcontrol_list_houses(struct char_data * ch);
void hcontrol_build_house(struct char_data * ch, char *arg);
void hcontrol_destroy_house(struct char_data * ch, char *arg);
void hcontrol_pay_house(struct char_data * ch, char *arg);
void hcontrol_set_guard(struct char_data * ch, char *arg);
ACMD(do_hcontrol);
ACMD(do_house);
ACMD(do_hchannel);
ACMD(do_whohouse);
ACMD(do_whoclan);

void House_channel(struct char_data *ch, char* msg);
void House_id_channel(struct char_data *ch, int huid, char* msg);

const char* house_rank[] =
{ "пришелец",
  "отрок",
  "вой",
  "муж",
  "гридень",
  "кметь",
  "храбр",
  "десятник",
  "боярин",
  "воевода",
  "изгнать",
  "\n"
};

const char *HCONTROL_FORMAT =
"Формат: hcontrol build[возвести] <house vnum> <exit direction> <player name> <shortname> [name]\r\n"
"        hcontrol rooms[залы] <house vnum> <room vnum> ...\r\n"
"        hcontrol destroy[разрушить] <house vnum>\r\n"
"        hcontrol pay[налог] <house vnum>\r\n"
"        hcontrol show[показать]\r\n"
"        hcontrol guard[охранник] <house vnum> <mob vnum>";

  
/* First, the basics: finding the filename; loading/saving objects */

/* Return a filename given a house vnum */
int House_get_filename(int vnum, char *filename)
{
  if (vnum == 0)
     return (0);

  sprintf(filename, LIB_HOUSE"%d.house", vnum);
  return (1);
}  


/* Load all objects for a house */
int House_load(room_vnum house_num)
{
  FILE *fl;
  char      fname[MAX_STRING_LENGTH];
  struct    obj_file_elem object;
  struct    obj_data *obj;
  room_rnum rnum;
  int       i;

  if (!House_get_filename(house_num, fname))
     return (0);
  if (house_num < 0 || house_num >= MAX_HOUSES)
     return (0);
  if (!(fl = fopen(fname, "r+b")))	/* no file found */
     return (0);
  while (!feof(fl))
        {fread(&object, sizeof(struct obj_file_elem), 1, fl);
         if (ferror(fl))
            {perror("SYSERR: Reading house file in House_load");
             fclose(fl);
             return (0);
            }
         if (!feof(fl))
            {i   = -1;
             obj = Obj_from_store(object, &i);
             if (!obj)
                continue;
             if (i < 0 || i >= MAX_HOUSE_ROOMS)
                i = 0;
             if ((rnum = real_room(house_control[house_num].vnum[i])) == NOWHERE)
                for (i = 0; i < MAX_HOUSE_ROOMS; i++)
                    if ((rnum = real_room(house_control[house_num].vnum[i])) != NOWHERE)
                       break;
             if (rnum == NOWHERE)
                extract_obj(obj);
             else
                obj_to_room(obj,rnum);
            }
        }

  fclose(fl);

  return (1);
}


/* Save all objects for a house (recursive; initial call must be followed
   by a call to House_restore_weight)  Assumes file is open already. */
int House_save(struct obj_data * obj, FILE * fp, int in_room)
{
  struct obj_data *tmp;
  int result;

  if (obj)
     {House_save(obj->contains, fp, in_room);
      House_save(obj->next_content, fp, in_room);
      result = Obj_to_store(obj, fp, in_room);
      if (!result)
         return (0);

      for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
          GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);
     }
  return (1);
}


/* restore weight of containers after House_save has changed them for saving */
void House_restore_weight(struct obj_data * obj)
{
  if (obj)
     {House_restore_weight(obj->contains);
      House_restore_weight(obj->next_content);
      if (obj->in_obj)
         GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
     }
}


/* Save all objects in a house */
void House_crashsave(room_vnum house_num)
{
  int rnum,i;
  char buf[MAX_STRING_LENGTH];
  FILE *fp;

  if (house_num < 0 || house_num >= MAX_HOUSES)
     return;
  if (!House_get_filename(house_num, buf))
     return;
  if (!(fp = fopen(buf, "wb")))
     {perror("SYSERR: Error saving house file");
      return;
     }
  for (i=0; i < MAX_HOUSE_ROOMS; i++)
      {if ((rnum = real_room(house_control[house_num].vnum[i])) == NOWHERE)
          continue;
       if (!House_save(world[rnum].contents, fp, i))
          {fclose(fp);
           return;
          }
       House_restore_weight(world[rnum].contents);
      }
  fclose(fp);
  REMOVE_BIT(ROOM_FLAGS(rnum, ROOM_HOUSE_CRASH), ROOM_HOUSE_CRASH);
}


/* Delete a house save file */
void House_delete_file(int house_num)
{
  char fname[MAX_INPUT_LENGTH];
  FILE *fl;

  if (!House_get_filename(house_num, fname))
     return;
  if (!(fl = fopen(fname, "rb")))
     {if (errno != ENOENT)
         log("SYSERR: Error deleting house file #%d. (1): %s", house_num, strerror(errno));
      return;
     }
  fclose(fl);
  if (remove(fname) < 0)
     log("SYSERR: Error deleting house file #%d. (2): %s", house_num, strerror(errno));
}


/* List all objects in a house file */
void House_listrent(struct char_data * ch, room_vnum house_num)
{
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  struct obj_data *obj;
  int i;

  if (!House_get_filename(house_num, fname))
     return;
  if (!(fl = fopen(fname, "rb")))
     {sprintf(buf, "No objects on file for house #%d.\r\n", house_num);
      send_to_char(buf, ch);
      return;
     }
  *buf = '\0';
  while (!feof(fl))
        {fread(&object, sizeof(struct obj_file_elem), 1, fl);
         if (ferror(fl))
            {fclose(fl);
             return;
            }
         if (!feof(fl) && (obj = Obj_from_store(object, &i)) != NULL)
            {sprintf(buf + strlen(buf), " [%5d] (%5dau) %s is in room %d\r\n",
	             GET_OBJ_VNUM(obj), GET_OBJ_RENT(obj),
	             obj->short_description,
	             i);
             extract_obj(obj);
            }
        }

  send_to_char(buf, ch);
  fclose(fl);
}




/******************************************************************
 *  Functions for house administration (creation, deletion, etc.  *
 *****************************************************************/

int find_house(room_vnum vnum)
{
  int i,j;

  for (i = 0; i < num_of_houses; i++)
      for (j = 0; j < MAX_HOUSE_ROOMS; j++)
          if (house_control[i].vnum[j] == vnum)
             return (i);

  return (NOWHERE);
}


long House_create_unique(void)
{int  i;
 long j;

 for(;TRUE;)
    {j = (number(1,255) << 24) + (number(1,255) << 16) + (number(1,255) << 8) + number(1,255);
     for (i=0;i < num_of_houses;i++)
         if (HOUSE_UNIQUE(i) == j)
            break;
     if (i >= num_of_houses)
        return (j);
    }
 return (0);
}

/* Save the house control information */
void House_save_control(void)
{
  FILE *fl;

  if (!(fl = fopen(HCONTROL_FILE, "wb")))
     {perror("SYSERR: Unable to open house control file.");
      return;
     }
  /* write all the house control recs in one fell swoop.  Pretty nifty, eh? */
  fwrite(house_control, sizeof(struct house_control_rec), num_of_houses, fl);

  fclose(fl);
}


/* call from boot_db - will load control recs, load objs, set atrium bits */
/* should do sanity checks on vnums & remove invalid records */
void House_boot(void)
{
  struct house_control_rec temp_house;
  room_rnum real_house, real_atrium;
  FILE *fl;
  int    j;

  memset((char *)house_control,0,sizeof(struct house_control_rec)*MAX_HOUSES);

  if (!(fl = fopen(HCONTROL_FILE, "rb")))
     {if (errno == ENOENT)
         log("   House control file '%s' does not exist.", HCONTROL_FILE);
      else
         perror("SYSERR: " HCONTROL_FILE);
      return;
     }
  while (!feof(fl) && num_of_houses < MAX_HOUSES)
        {fread(&temp_house, sizeof(struct house_control_rec), 1, fl);

         if (feof(fl))
            break;

         if (get_name_by_id(temp_house.owner) == NULL)
            continue; /* owner no longer exists -- skip */

         if ((real_house = real_room(temp_house.vnum[0])) == NOWHERE)
            continue; /* this vnum doesn't exist -- skip */

         if (find_house(temp_house.vnum[0]) != NOWHERE)
            continue; /* this vnum is already a house -- skip */

         if ((real_atrium = real_room(temp_house.atrium)) == NOWHERE)
            continue; /* house doesn't have an atrium -- skip */

        if (temp_house.exit_num < 0 || temp_house.exit_num >= NUM_OF_DIRS)
           continue; /* invalid exit num -- skip */

        if (TOROOM(real_house, temp_house.exit_num) != real_atrium)
           continue; /* exit num mismatch -- skip */

        house_control[num_of_houses] = temp_house;
        for (j = 0; j < MAX_HOUSE_ROOMS; j++)
            if ((real_house = real_room(house_control[num_of_houses].vnum[j])) != NOWHERE)
               {SET_BIT(ROOM_FLAGS(real_house, ROOM_HOUSE),   ROOM_HOUSE);
                SET_BIT(ROOM_FLAGS(real_house, ROOM_PRIVATE), ROOM_PRIVATE);
               }
        SET_BIT(ROOM_FLAGS(real_atrium, ROOM_ATRIUM), ROOM_ATRIUM);
        House_load(num_of_houses++);
       }

  fclose(fl);
  House_save_control();
}

int  House_for_uid(long uid)
{int i;
 for (i = 0; i < num_of_houses; i++)
     if (HOUSE_UNIQUE(i) == uid)
        return (i);
 return (NOWHERE);
}

int House_check_exist(long uid)
{int j;
 for (j=0;j < num_of_houses;j++)
     if (HOUSE_UNIQUE(j) == uid)
        return (TRUE);
 return (FALSE);
}

int House_check_keeper(long uid)
{room_rnum i,j;
 struct char_data *ch;

 if ((j=House_for_uid(uid)) == NOWHERE)
    return (FALSE);
 if (!house_control[j].keeper)
    return (FALSE);
 if ((i=real_room(house_control[j].atrium)) == NOWHERE)
    return (FALSE);
 for (ch = world[i].people; ch; ch = ch->next_in_room)
     if (GET_MOB_VNUM(ch) == house_control[j].keeper)
        return (TRUE);
 return (FALSE);
}

int House_check_free(long uid)
{room_rnum i,j,k;
 struct char_data *ch;
 if ((j=House_for_uid(uid)) == NOWHERE)
    return (FALSE);
 for (i=0;i<MAX_HOUSE_ROOMS;i++)
     if ((k = real_room(house_control[j].vnum[i])) != NOWHERE)
        for (ch = world[k].people;ch;ch=ch->next_in_room)
            if (!IS_NPC(ch) && GET_HOUSE_UID(ch) != uid)
               return (TRUE);
 return (FALSE);
}

/* "House Control" functions */




struct descriptor_data *House_find_desc(long id)
{ struct descriptor_data *d;
  for (d = descriptor_list; d; d = d->next)
      if (d->character && STATE(d) == CON_PLAYING && GET_ID(d->character) == id)
         break;
  return (d);
}


void hcontrol_add_room(struct char_data * ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH];
  room_vnum virt_house;
  room_rnum real_house;
  sh_int    house_num,house_may,j,k;
  int       change=FALSE,found;

  arg = one_argument(arg, arg1);
  if (!*arg1)
     {send_to_char(HCONTROL_FORMAT, ch);
      return;
     }

  if ((house_num = find_house(atoi(arg1))) == NOWHERE)
     {sprintf(buf,"Комната %s не входит ни в один замок.\r\n",arg1);
      send_to_char(buf,ch);
      return;
     }

  if (!IS_GRGOD(ch) && house_control[house_num].owner != GET_ID(ch))
     {send_to_char("Вы же не хозяин замка !\r\n", ch);
      return;
     }

  while (*arg)
        {skip_spaces(&arg);
         arg = one_argument(arg, arg1);
         if (!*arg1)
            continue;
         virt_house = atoi(arg1);
         if ((real_house = real_room(virt_house)) == NOWHERE)
            {sprintf(buf,"Комнаты %s не существует !\r\n",arg1);
             send_to_char(buf, ch);
             continue;
            }
         if ((house_may = real_room(virt_house)) != NOWHERE)
            {if (house_may != house_num)
                {sprintf(buf,"Комната %d принадлежит чужому замку.\r\n",virt_house);
                 send_to_char(buf,ch);
                 continue;
                }
             for (j = 1; j < MAX_HOUSE_ROOMS; j++)
                 if (house_control[house_num].vnum[j] == virt_house)
                    break;
             if (j >= MAX_HOUSE_ROOMS)
                {sprintf(buf,"Комната %d не принадлежит вашему замку.\r\n",virt_house);
                 send_to_char(buf,ch);
                 continue;
                }
             house_control[house_num].vnum[j] = 0;
             REMOVE_BIT(ROOM_FLAGS(real_house, ROOM_HOUSE), ROOM_HOUSE);
             REMOVE_BIT(ROOM_FLAGS(real_house, ROOM_PRIVATE), ROOM_PRIVATE);
             REMOVE_BIT(ROOM_FLAGS(real_house, ROOM_HOUSE_CRASH), ROOM_HOUSE_CRASH);
             for (j++; j < MAX_HOUSE_ROOMS; j++)
                 {house_control[house_num].vnum[j-1] = house_control[house_num].vnum[j];
                  house_control[house_num].vnum[j]   = 0;
                 }
             sprintf(buf,"Комната %d удалена из замка.\r\n",virt_house);
             send_to_char(buf,ch);
             change = TRUE;
            }
         else
            {for (j = 0, found = FALSE; j < MAX_HOUSE_ROOMS; j++)
                 if (house_control[house_num].vnum[j] == 0)
                    break;
                 else
                 if ((house_may = real_room(house_control[house_num].vnum[j])) != NOWHERE)
                    for (k = 0; k < NUM_OF_DIRS; k++)
                        if (TOROOM(house_may,k) == real_house)
                           found = TRUE;
             if (j >= MAX_HOUSE_ROOMS)
                {send_to_char("Слишком много комнат в замке.\r\n",ch);
                 continue;
                }
             if (!found)
                 {sprintf(buf,"Комната %d не связана с комнатами замка.",virt_house);
                  send_to_char(buf,ch);
                  continue;
                 }
             house_control[house_num].vnum[j] = virt_house;
             SET_BIT(ROOM_FLAGS(real_house, ROOM_HOUSE), ROOM_HOUSE);
             SET_BIT(ROOM_FLAGS(real_house, ROOM_PRIVATE), ROOM_PRIVATE);
             sprintf(buf,"Комната %d присоединена к замку.\r\n",virt_house);
             send_to_char(buf,ch);
             change = TRUE;
            }
        }
  if (change)
     House_save_control();
}


void hcontrol_destroy_house(struct char_data * ch, char *arg)
{
  int i, j;
  room_rnum real_atrium, real_house;

  if (!*arg)
     {send_to_char(HCONTROL_FORMAT, ch);
      return;
     }
  if ((i = find_house(atoi(arg))) == NOWHERE)
     {send_to_char("Нет такого замка.\r\n", ch);
      return;
     }
  if ((real_atrium = real_room(house_control[i].atrium)) == NOWHERE)
     log("SYSERR: House %d had invalid atrium %d!", atoi(arg),
	  house_control[i].atrium);
  else
     REMOVE_BIT(ROOM_FLAGS(real_atrium, ROOM_ATRIUM), ROOM_ATRIUM);

  for (j = 0; j < MAX_HOUSE_ROOMS; j++)
      {if ((real_house = real_room(house_control[i].vnum[j])) == NOWHERE)
          {if (house_control[i].vnum[j])
              log("SYSERR: House %d had invalid vnum %d!", atoi(arg), house_control[i].vnum[j]);
          }
       else
          { REMOVE_BIT(ROOM_FLAGS(real_house, ROOM_HOUSE), ROOM_HOUSE);
            REMOVE_BIT(ROOM_FLAGS(real_house, ROOM_HOUSE_CRASH), ROOM_HOUSE_CRASH);
            REMOVE_BIT(ROOM_FLAGS(real_house, ROOM_PRIVATE), ROOM_PRIVATE);
          }
      }
  House_delete_file(i);

  for (j = i; j < num_of_houses - 1; j++)
      house_control[j] = house_control[j + 1];

  num_of_houses--;

  send_to_char("Замок разрушен.\r\n", ch);
  House_save_control();

  /*
   * Now, reset the ROOM_ATRIUM flag on all existing houses' atriums,
   * just in case the house we just deleted shared an atrium with another
   * house.  --JE 9/19/94
   */
  for (i = 0; i < num_of_houses; i++)
      if ((real_atrium = real_room(house_control[i].atrium)) != NOWHERE)
         SET_BIT(ROOM_FLAGS(real_atrium, ROOM_ATRIUM), ROOM_ATRIUM);
}


void hcontrol_pay_house(struct char_data * ch, char *arg)
{
  int i;

  if (!*arg)
     send_to_char(HCONTROL_FORMAT, ch);
  else
  if ((i = find_house(atoi(arg))) == NOWHERE)
     send_to_char("Нет такого замка.\r\n", ch);
  else
     {sprintf(buf, "Payment for house %s collected by %s.", arg, GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);

      house_control[i].last_payment = time(0);
      House_save_control();
      send_to_char("Вы внесли великокняжеский налог за свой замок.\r\n", ch);
     }
}


/* The hcontrol command itself, used by imms to create/destroy houses */
ACMD(do_hcontrol)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  half_chop(argument, arg1, arg2);

  if (is_abbrev(arg1, "build") || is_abbrev(arg1, "возвести"))
     hcontrol_build_house(ch, arg2);
  else
  if (is_abbrev(arg1, "rooms") || is_abbrev(arg1, "залы"))
     hcontrol_add_room(ch, arg2);
  else
  if (is_abbrev(arg1, "destroy") || is_abbrev(arg1, "разрушить"))
     hcontrol_destroy_house(ch, arg2);
  else
  if (is_abbrev(arg1, "pay") || is_abbrev(arg1, "налог"))
     hcontrol_pay_house(ch, arg2);
  else
  if (is_abbrev(arg1, "show") || is_abbrev(arg1, "показать"))
     hcontrol_list_houses(ch);
  else
  if (is_abbrev(arg1, "guard") || is_abbrev(arg1, "охранник"))
     hcontrol_set_guard(ch, arg2);
  else
     send_to_char(HCONTROL_FORMAT, ch);
}


/* The house command, used by mortal house owners to assign guests */
ACMD(do_house)
{ struct descriptor_data *d;
  int i, j, id=-1, rank=RANK_GUEST;

  argument = one_argument(argument, arg);

  if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
     send_to_char("Вам стоит войти в свой замок.\r\n", ch);
  else
  if ((i = find_house(GET_ROOM_VNUM(IN_ROOM(ch)))) == NOWHERE)
     send_to_char("Хмм.. Что-то не очень похоже на замок.\r\n", ch);
  else
  if (GET_IDNUM(ch) != house_control[i].owner)
     send_to_char("Вы же не хозяин этого замка !\r\n", ch);
  else
  if (!*arg)
     House_list_guests(ch, i, FALSE);
  else
  if ((id = get_id_by_name(arg)) < 0)
     send_to_char("Неизвестный игрок.\r\n", ch);
  else
  if (id == GET_IDNUM(ch))
     send_to_char("Это же ВАШ замок!\r\n", ch);
  else
     {if (!(d=House_find_desc(id)))
         {send_to_char("Этого игрока нет в игре !\r\n",ch);
          return;
         }
         
      one_argument(argument,arg);
      if (!*arg)
         rank = RANK_GUEST;
      else
      if ((rank = search_block(arg,house_rank,FALSE)) < 0)
         {send_to_char("Укажите положение этого игрока или ИЗГНАТЬ.\r\n",ch);
	  send_to_char(" Возможные положения:\r\n",ch);
	  for (j = RANK_GUEST; j<RANK_KNIEZE; j++) {
	    send_to_char(house_rank[j],ch); 
	    send_to_char("\r\n",ch);
	  }
          return;
         }

      if (GET_HOUSE_UID(d->character) && GET_HOUSE_UID(d->character) != HOUSE_UNIQUE(i))
         {send_to_char("Этот игрок не является Вашим соратником.\r\n",ch);
          return;
         }

      if (!GET_HOUSE_UID(d->character) && rank > RANK_KNIEZE)
         {send_to_char("Этот игрок не является Вашим соратником.\r\n",ch);
          return;
         }
    
      if (rank > RANK_KNIEZE)
         {for (j = 0; j < house_control[i].num_of_guests; j++)
              if (house_control[i].guests[j] == id)
                 {for (;j < house_control[i].num_of_guests; j++)
                      house_control[i].guests[j] = house_control[i].guests[j + 1];
                  house_control[i].num_of_guests--;
                  House_save_control();
                  act("Игрок $N изгнан$G из числа Ваших соратников.",FALSE,ch,0,d->character,TO_CHAR);
           	      
                  GET_HOUSE_UID(d->character) = GET_HOUSE_RANK(d->character) = 0;
                  save_char(d->character,NOWHERE);
                  sprintf(buf,"Вас исключили из дружины '%s'.",House_name(ch));
                  act(buf,FALSE,d->character,0,ch,TO_CHAR);
                  return;
                 }
	  return;
	 }
             
      if (rank == RANK_KNIEZE)
         {send_to_char("В замке может быть лишь один КНЯЗЬ !\r\n",ch);
          return;
         }

      if (!GET_HOUSE_UID(d->character))
         {if (house_control[i].num_of_guests == MAX_GUESTS)
             {send_to_char("Слишком много соратников в замке.\r\n", ch);
              return;
             }
                  
          j = house_control[i].num_of_guests++;
          house_control[i].guests[j] = id;
          House_save_control();
         }  
      GET_HOUSE_UID(d->character)  = HOUSE_UNIQUE(i);
      GET_HOUSE_RANK(d->character) = rank;
      save_char(d->character,NOWHERE);
      sprintf(buf,"$N приписан$G к Вашей дружине, статус - %s.",house_rank[rank]);
      act(buf,FALSE,ch,0,d->character,TO_CHAR);
/*      sprintf(buf,"Вас приписали к дружине $N1, статус - %s.",house_rank[rank]);*/
      sprintf(buf,"Вас приписали к дружине '%s', статус - %s.",House_name(ch),house_rank[rank]);
      act(buf,FALSE,d->character,0,ch,TO_CHAR);
     }
}

ACMD(do_hchannel)
{int clannum = 0;

struct descriptor_data;

 skip_spaces(&argument);
 
 if (!IS_GOD(ch) && !GET_HOUSE_UID(ch))
    {send_to_char("Вы не принадлежите ни к одной дружине.\r\n",ch);
     return;
    }
 if (!IS_GOD(ch) && !GET_HOUSE_RANK(ch))
    {send_to_char("Вы не в состоянии ничего сюда сообщить.\r\n",ch);
     return;
    }
 if (!*argument)
    {send_to_char("Что Вы хотите сообщить ?\r\n",ch);
     return;
    }
 if (IS_GOD(ch))
    {argument = one_argument(argument, arg);
     if (!(clannum = atoi(argument)))
        {send_to_char("Уточните номер дружины.\r\n", ch);
	 return;
	}
     House_id_channel(ch,clannum,argument);
    }
 else
    House_channel(ch,argument); 
}
     

/* Misc. administrative functions */


/* crash-save all the houses */
void House_save_all(void)
{
  int i;
  room_rnum real_house;

  for (i = 0; i < num_of_houses; i++)
      if ((real_house = real_room(house_control[i].vnum[0])) != NOWHERE)
         if (ROOM_FLAGGED(real_house, ROOM_HOUSE_CRASH))
	    House_crashsave(i);
}


/* note: arg passed must be house vnum, so there. 
int House_can_enter(struct char_data * ch, room_vnum house)
{ struct char_data *tch = NULL;
  int    i, j;

  if (GET_LEVEL(ch) >= LVL_GRGOD || (i = find_house(house)) == NOWHERE)
     return (1);

  switch (house_control[i].mode)
  {case HOUSE_PRIVATE:
        // Owner
        if (GET_IDNUM(ch) == house_control[i].owner)
           return (1);
        // There is no keeper in House
        if ((j=real_room(house_control[i].atrium)) != NOWHERE &&
            HOUSE_KEEPER(i)
           )
           for (tch = world[j].people; tch; tch = tch->next_in_room)
               if (IS_NPC(ch) && GET_MOB_VNUM(tch) == HOUSE_KEEPER(i))
                  break;
        if (!tch)
           return (1);
        // This is a guest
        for (j = 0; j < house_control[i].num_of_guests; j++)
            if (GET_IDNUM(ch) == house_control[i].guests[j])
	       return (1);
  }

  return (0);
} */

void House_list_guests(struct char_data *ch, int i, int quiet)
{
  int j;
  char *temp;
  char buf[MAX_STRING_LENGTH], buf2[MAX_NAME_LENGTH + 2];

  if (house_control[i].num_of_guests == 0)
     {if (!quiet)
         send_to_char("К замку никто не приписан.\r\n", ch);
      return;
     }

  strcpy(buf, "К замку приписаны : ");

  /* Avoid <UNDEF>. -gg 6/21/98 */
  for (j = 0; j < house_control[i].num_of_guests; j++)
      {if ((temp = get_name_by_id(house_control[i].guests[j])) == NULL)
          continue;
       sprintf(buf2, "%s ", temp);
       strcat(buf, CAP(buf2));
      }

  strcat(buf, "\r\n");
  send_to_char(buf, ch);
}

void House_list_rooms(struct char_data *ch, int i, int quiet)
{
  int  j, found = FALSE, rnum;
  char buf[MAX_STRING_LENGTH], buf2[MAX_NAME_LENGTH + 2];


  strcpy(buf, "Включает комнаты : ");

  /* Avoid <UNDEF>. -gg 6/21/98 */
  for (j = 1; j < MAX_HOUSE_ROOMS; j++)
      {if ((rnum = real_room(house_control[i].vnum[j])) != NOWHERE)
          {found = TRUE;
           sprintf(buf2, "%d ", house_control[i].vnum[j]);
           strcat(buf, buf2);
          }
      }
  if (!found)
     strcat(buf," Нет.");
  strcat(buf, "\r\n");
  send_to_char(buf, ch);
}

int House_major(struct char_data *ch)
{int    i;
 struct descriptor_data *d;
 
 if (!GET_HOUSE_UID(ch))
    return (FALSE);
 if (GET_HOUSE_RANK(ch) == RANK_KNIEZE)
    return (TRUE);
 if (GET_HOUSE_RANK(ch) <  RANK_CENTURION)
    return (FALSE);
 if ((i = House_for_uid(GET_HOUSE_UID(ch))) == NOWHERE)
    return (FALSE);
 for (d = descriptor_list; d; d = d->next)
     {if (!d->character                                  ||
          GET_HOUSE_UID(d->character) != HOUSE_UNIQUE(i) ||
          STATE(d) != CON_PLAYING                        ||
          GET_HOUSE_RANK(d->character) <= GET_HOUSE_RANK(ch)
         )
         continue;
      break;
     }
 return (d == NULL);
}

void House_set_keeper(struct char_data *ch)
{int    house, guard_room;
 struct char_data* guard;

 if (!GET_HOUSE_UID(ch))
    {send_to_char("Вы не приписаны ни к одному замку !\r\n",ch);
     return;
    }

 if ((house = House_for_uid(GET_HOUSE_UID(ch))) == NOWHERE)
    {send_to_char("Ваш замок, похоже, разрушен.\r\n",ch);
     GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;
     return;
    }

 if (!house_control[house].keeper)
    {send_to_char("Охрана Вашего замка не предусмотрена.\r\n",ch);
     return;
    };

 if (!House_major(ch))
    {send_to_char("Вы не самый старший соратник в игре !\r\n",ch);
     return;
    }
 if (!House_check_free(GET_HOUSE_UID(ch)))
    {send_to_char("В замке посторонние игроки !\r\n",ch);
     return;
    }
 if (House_check_keeper(GET_HOUSE_UID(ch)))
    {send_to_char("Охрана уже выставлена !\r\n",ch);
     return;
    }
 house = House_for_uid(GET_HOUSE_UID(ch));
 if ((guard_room = real_room(house_control[house].atrium)) == NOWHERE)
    {send_to_char("Нет комнаты для охраны.\r\n",ch);
     return;
    }
 if (!(guard = read_mobile(house_control[house].keeper, VIRTUAL)))
    {send_to_char("Нет прототипа охранника.\r\n",ch);
     return;
    }
 char_to_room(guard, guard_room);
 act("Вы установили $N3 на стражу замка.",FALSE,ch,0,guard,TO_CHAR);
}

char* House_rank(struct char_data *ch)
{int house = 0;
 if (!GET_HOUSE_UID(ch))
    return (NULL);

 if ((house = House_for_uid(GET_HOUSE_UID(ch))) == NOWHERE)
    {GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;
     return (NULL);
    }
 return ((char *)house_rank[GET_HOUSE_RANK(ch)]); 
}

char* House_name(struct char_data *ch)
{int house = 0;
 if (!GET_HOUSE_UID(ch))
    return (NULL);

 if ((house = House_for_uid(GET_HOUSE_UID(ch))) == NOWHERE)
    {GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;
     return (NULL);
    }
 return (house_control[house].name);
}


char* House_sname(struct char_data *ch)
{int house = 0;
 if (!GET_HOUSE_UID(ch))
    return (NULL);

 if ((house = House_for_uid(GET_HOUSE_UID(ch))) == NOWHERE)
    {GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;
     return (NULL);
    }
 return (house_control[house].sname);
}


void House_channel(struct char_data *ch, char* msg)
{int    house;
 char   message[MAX_STRING_LENGTH];
 struct descriptor_data *d;

 if (!GET_HOUSE_UID(ch))
    return;
 if ((house = House_for_uid(GET_HOUSE_UID(ch))) == NOWHERE)
    {GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;
     return;
    }
 for (d = descriptor_list; d; d = d->next)
     {if (!d->character || d->character == ch ||
          STATE(d) != CON_PLAYING ||
          GET_HOUSE_UID(d->character) != HOUSE_UNIQUE(house))
         continue;
      sprintf(message,"%s дружине: %s'%s'.%s\r\n",
              GET_NAME(ch),CCIRED(d->character,C_NRM),msg,CCNRM(d->character,C_NRM));
      SEND_TO_Q(message,d);
     }

    sprintf(message,"Вы дружине: %s'%s'.%s\r\n",
              CCIRED(ch,C_NRM),msg,CCNRM(ch,C_NRM));
    send_to_char(message,ch);

}

void House_id_channel(struct char_data *ch, int huid, char* msg)
{char   message[MAX_STRING_LENGTH];
 struct descriptor_data *d;

 for (d = descriptor_list; d; d = d->next)
     {if (!d->character ||
          STATE(d) != CON_PLAYING ||
          huid != GET_HOUSE_UID(d->character)
	 )
         continue;
      sprintf(message,"%s ВАШЕМУ  КЛАНУ: %s'%s'%s\r\n",
              GET_NAME(ch),CCIRED(d->character,C_NRM),msg,CCNRM(d->character,C_NRM));
      SEND_TO_Q(message,d);
     }
}


ACMD(do_whohouse)
{
  House_list(ch);
}

void House_list(struct char_data *ch)
{int    house, num;
 struct descriptor_data *d;

 if (!GET_HOUSE_UID(ch))
    {send_to_char("Вы не приписаны ни к одному замку !\r\n",ch);
     return;
    }
 if ((house = House_for_uid(GET_HOUSE_UID(ch))) == NOWHERE)
    {GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;
     return;
    } 
 sprintf(buf," Ваша дружина: %s%s%s.\r\n %sСейчас в игре Ваши соратники :%s\r\n\r\n",
           CCIRED(ch,C_NRM),House_name(ch),CCNRM(ch,C_NRM),
	   CCWHT(ch,C_NRM),CCNRM(ch,C_NRM));
 for (d=descriptor_list, num=0; d; d=d->next)
     {if (!d->character               ||
          STATE(d) != CON_PLAYING     ||
          GET_HOUSE_UID(d->character) != HOUSE_UNIQUE(house)
         )
        continue;
      sprintf(buf2,"    %s\r\n", race_or_title(d->character));
      strcat(buf,buf2);
      num++;
     }
  sprintf(buf2,"\r\n Всего %d.\r\n",num);
  strcat(buf,buf2);
  send_to_char(buf,ch);
}

ACMD(do_whoclan)
{
 if (GET_HOUSE_RANK(ch) != RANK_KNIEZE) {
    send_to_char("Только воевода имеет доступ к полному списку!\r\n",ch);
    return;
 }
 House_list_all(ch);
}

void House_list_all(struct char_data *ch)
{ sh_int    house_num;


 if (!GET_HOUSE_UID(ch))
    {send_to_char("Вы не приписаны ни к одному замку !\r\n",ch);
     return;
 }

  if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE)) {
     send_to_char("Вам стоит войти в свой замок.\r\n", ch);
     return;
  }

  house_num = find_house(GET_ROOM_VNUM(IN_ROOM(ch)));
  if (house_num == NOWHERE)
     {sprintf(buf,"Хмм.. Что-то не очень похоже на замок.\r\n");
      send_to_char(buf,ch);
      return;
     }

  if (house_control[house_num].owner != GET_ID(ch))
     {send_to_char("Вы же не хозяин замка !\r\n", ch);
      return;
     }

 sprintf(buf," Ваша дружина: %s%s%s.\r\n %sПолный список Ваших соратников :%s\r\n\r\n",
           CCIRED(ch,C_NRM),House_name(ch),CCNRM(ch,C_NRM),
	   CCWHT(ch,C_NRM),CCNRM(ch,C_NRM));
 send_to_char(buf,ch);
 House_list_guests(ch, house_num, TRUE);


}

/*------*/
ACMD(do_listclan)
{
 int j,k,l;
 char *temp;

 send_to_char("В игре зарегистрированы следующие дружины:\r\n",ch);
 send_to_char("     #                 Глава Название\r\n\r\n",ch);
 for (j = 0, k=0 ; j < num_of_houses ; j++) {
  if ((temp = get_name_by_id(house_control[j].owner)) == NULL)
    continue;
  k++;
  l = j+1;
  CAP(temp);
  sprintf(buf," %5d %5s %15s %s\r\n",l,house_control[j].sname,
                temp,house_control[j].name);
  send_to_char(buf,ch);
 }
  sprintf(buf,"\r\n Всего - %d\r\n",k);
  send_to_char(buf,ch);

}

int House_can_enter(struct char_data * ch, room_vnum house)
{ struct char_data *mobs;
  int    i, j;

  if (GET_LEVEL(ch) >= LVL_GRGOD || (i = find_house(house)) == NOWHERE)
     return (1);

  switch (house_control[i].mode)
  {case HOUSE_PRIVATE:
	/* Всем остальным, если нет моба-охранника, то пускать */
        j = 0;
        for (mobs = world[real_room(house_control[i].atrium)].people; mobs; mobs = mobs->next_in_room)
	 if (house_control[i].keeper == GET_MOB_VNUM(mobs))
	  j++;
	if (j == 0)
           return (1);
        /* Если флаг, что ПК недавно, то не пускать */
	if (RENTABLE(ch)) {
	   send_to_char("Пускай сначала кровь с тебя стечет, а потом входи сколько угодно.\r\n",ch);
           return (0);
	}
        // Owner
        if (GET_IDNUM(ch) == house_control[i].owner)
           return (1);
        // There is no keeper in House
        /* if ((j=real_room(house_control[i].atrium)) != NOWHERE &&
            HOUSE_KEEPER(i)
           )
           for (tch = world[j].people; tch; tch = tch->next_in_room)
               if (IS_NPC(ch) && GET_MOB_VNUM(tch) == HOUSE_KEEPER(i))
                  break;
        if (!tch)
           return (1); */
        // This is a guest
        for (j = 0; j < house_control[i].num_of_guests; j++)
            if (GET_IDNUM(ch) == house_control[i].guests[j])
	       return (1);
  }

  return (0);
}

void hcontrol_set_guard(struct char_data * ch, char *arg)
{
 int    i, guard_vnum;
 char arg1[MAX_INPUT_LENGTH];

 arg = one_argument(arg, arg1);

 if (!*arg)
     {send_to_char(HCONTROL_FORMAT, ch);
      return;
     }

 if ((i = find_house(atoi(arg1))) == NOWHERE)
     {send_to_char("Нет такого замка.\r\n", ch);
      return;
     }

 guard_vnum = atoi(arg);

 house_control[i].keeper = guard_vnum;
 sprintf(arg1, "Вы установили моба %ld на стражу замка %s", 
          house_control[i].keeper, house_control[i].sname);
 act(arg1, FALSE, ch, 0, 0, TO_CHAR);
 House_save_control();
}

/*const char *HCONTROL_FORMAT =
"Формат: hcontrol build[возвести] <house vnum> <exit direction> <player name> <shortname> [name]\r\n"
"        hcontrol rooms[залы] <house vnum> <room vnum> ...\r\n"
"        hcontrol destroy[разрушить] <house vnum>\r\n"
"        hcontrol pay[налог] <house vnum>\r\n"
"        hcontrol show[показать]\r\n"
"        hcontrol guard[охранник] <house vnum> <mob vnum>"; */

void hcontrol_list_houses(struct char_data * ch)
{
  int i;
  char *timestr, *temp;
  char built_on[128], last_pay[128], own_name[128];

  if (!num_of_houses)
     {send_to_char("Родовые замки не определены.\r\n", ch);
      return;
     }
  strcpy(buf, "Комната  Вход    Возведен    Игроки  Князь        Налог внесен\r\n");
  strcat(buf, "-------  ------  ----------  ------  ------------ ------------\r\n");
  send_to_char(buf, ch);

  for (i = 0; i < num_of_houses; i++)
      {/* Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98 */
       if ((temp = get_name_by_id(house_control[i].owner)) == NULL)
          continue;

       if (house_control[i].built_on)
          {timestr = asctime(localtime(&(house_control[i].built_on)));
           *(timestr + 10) = '\0';
           strcpy(built_on, timestr);
          }
       else
          strcpy(built_on, " Вечен ");

       if (house_control[i].last_payment)
          {timestr = asctime(localtime(&(house_control[i].last_payment)));
           *(timestr + 10) = '\0';
           strcpy(last_pay, timestr);
          }
       else
          strcpy(last_pay, "Нет");

       /* Now we need a copy of the owner's name to capitalize. -gg 6/21/98 */
       strcpy(own_name, temp);

       sprintf(buf, "%7d %7d  %-10s    %2d    %-12s %s\r\n",
	       house_control[i].vnum[0], house_control[i].atrium, built_on,
	       house_control[i].num_of_guests, CAP(own_name), last_pay);

       send_to_char(buf, ch);
       House_list_guests(ch, i, TRUE);
       House_list_rooms(ch, i, TRUE);
       sprintf(buf,"Моб охранник: %ld\r\n", house_control[i].keeper);
       send_to_char(buf, ch);
      }
}

void hcontrol_build_house(struct char_data * ch, char *arg)
{ struct descriptor_data *d;
  char arg1[MAX_INPUT_LENGTH];
  char sn[HOUSE_SNAME_LEN+1];
  struct house_control_rec temp_house;
  room_vnum virt_house, virt_atrium;
  room_rnum real_house, real_atrium;
  sh_int exit_num;
  long owner;
  int i;

  if (num_of_houses >= MAX_HOUSES)
     {send_to_char("Слишком много родовых замков.\r\n", ch);
      return;
     }

  /* first arg: house's vnum */
  arg = one_argument(arg, arg1);
  if (!*arg1)
     {send_to_char(HCONTROL_FORMAT, ch);
      return;
     }
  virt_house = atoi(arg1);
  if ((real_house = real_room(virt_house)) == NOWHERE)
     {send_to_char("Нет такой комнаты.\r\n", ch);
      return;
     }
  if ((find_house(virt_house)) != NOWHERE)
     {send_to_char("Эта комната уже занята под замок.\r\n", ch);
      return;
     }

  /* second arg: direction of house's exit */
  arg = one_argument(arg, arg1);
  if (!*arg1)
     {send_to_char(HCONTROL_FORMAT, ch);
      return;
     }
  if ((exit_num = search_block(arg1, dirs, FALSE)) < 0 &&
      (exit_num = search_block(arg1, Dirs, FALSE)) < 0 )
     {sprintf(buf, "'%s' не является верным направлением.\r\n", arg1);
      send_to_char(buf, ch);
      return;
     }
  if (TOROOM(real_house, exit_num) == NOWHERE)
     {sprintf(buf, "Не существует выхода на %s из комнаты %d.\r\n", Dirs[exit_num],
              virt_house);
      send_to_char(buf, ch);
      return;
     }

  real_atrium = TOROOM(real_house, exit_num);
  virt_atrium = GET_ROOM_VNUM(real_atrium);

  if (TOROOM(real_atrium, rev_dir[exit_num]) != real_house)
     {send_to_char("Выход из замка не ведет обратно в замок.\r\n", ch);
      return;
     }

  /* third arg: player's name */
  arg = one_argument(arg, arg1);
  if (!*arg1)
     {send_to_char(HCONTROL_FORMAT, ch);
      return;
     }
  if ((owner = get_id_by_name(arg1)) < 0)
     {sprintf(buf, "Не существует игрока '%s'.\r\n", arg1);
      send_to_char(buf, ch);
      return;
     }

  if (!(d = House_find_desc(owner)))
     {send_to_char("Создание замка допустимо только для играющего персонажа.\r\n",ch);
      return;
     }

  if (GET_HOUSE_UID(d->character)) {
      sprintf(buf,"Игрок %s приписан к уже существующему замку !",GET_NAME(d->character)); 
      send_to_char(buf,ch);
      return;
     }

  /* 4th arg: short clan name */
  arg = one_argument(arg, arg1);
  if (!*arg1)
     {send_to_char(HCONTROL_FORMAT, ch);
      return;
     }
  if (strlen(arg1)>HOUSE_SNAME_LEN) {
     send_to_char("Краткое название клана не должно быть такое большое.\r\n",ch);
     return;
  }
  strcpy(sn,arg1);

  skip_spaces(&arg);

  if (strlen(arg)>HOUSE_NAME_LEN-1) {
     send_to_char("Полное название клана не должно быть такое большое.\r\n",ch);
     return;
  }

  memset(&temp_house, sizeof(struct house_control_rec), 0);
  temp_house.mode          = HOUSE_PRIVATE;
  temp_house.vnum[0]       = virt_house;
  sprintf(temp_house.name,"%s",arg);
  for (i = 0; sn[i] != 0 ; i++)
   sn[i] = UPPER(sn[i]);
  sprintf(temp_house.sname,"%s",sn);
  temp_house.atrium        = virt_atrium;
  temp_house.exit_num      = exit_num;
  temp_house.built_on      = time(0);
  temp_house.last_payment  = 0;
  temp_house.owner         = owner;
  temp_house.num_of_guests = 0;
  temp_house.keeper = 0;
  temp_house.unique        = House_create_unique();

  house_control[num_of_houses] = temp_house;

  SET_BIT(ROOM_FLAGS(real_house, ROOM_HOUSE), ROOM_HOUSE);
  SET_BIT(ROOM_FLAGS(real_house, ROOM_PRIVATE), ROOM_PRIVATE);
  SET_BIT(ROOM_FLAGS(real_atrium, ROOM_ATRIUM), ROOM_ATRIUM);
  House_crashsave(num_of_houses++);

  send_to_char("Замок возведен !\r\n", ch);
  House_save_control();

  GET_HOUSE_UID (d->character) = temp_house.unique;
  GET_HOUSE_RANK(d->character) = RANK_KNIEZE;
  SEND_TO_Q("Вы стали хозяином нового замка. Добро пожаловать !\r\n",d);
  save_char(d->character, NOWHERE);
}
