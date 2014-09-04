/************************************************************************
 *  OasisOLC - zedit.c						v1.5	*
 *									*
 *  Copyright 1996 Harvey Gilpin.					*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "olc.h"
#include "dg_scripts.h"

/*
 * Turn on zedit debugging.  Please mail log results to greerga@van.ml.org
 * This will attempt to find the problem with replacing other zedit commands
 * when you add unrelated ones.
 */
#if 0
#define DEBUG	1
#endif

/*-------------------------------------------------------------------*/

/*
 * External data structures.
 */
extern struct zone_data *zone_table;
extern struct room_data *world;
extern zone_rnum top_of_zone_table;
extern struct char_data *mob_proto;
extern struct index_data *mob_index;
extern struct obj_data *obj_proto;
extern struct index_data *obj_index;
extern char *equipment_types[];
extern char *dirs[];
extern struct index_data **trig_index;
/*-------------------------------------------------------------------*/

/*
 * Function prototypes.
 */
void zedit_disp_menu(struct descriptor_data *d);
void zedit_setup(struct descriptor_data *d, int room_num);
void add_cmd_to_list(struct reset_com **list, struct reset_com *newcmd, int pos);
void remove_cmd_from_list(struct reset_com **list, int pos);
void delete_command(struct descriptor_data *d, int pos);
int new_command(struct descriptor_data *d, int pos);
int start_change_command(struct descriptor_data *d, int pos);
void zedit_disp_comtype(struct descriptor_data *d);
void zedit_disp_arg1(struct descriptor_data *d);
void zedit_disp_arg2(struct descriptor_data *d);
void zedit_disp_arg3(struct descriptor_data *d);
void zedit_save_internally(struct descriptor_data *d);
void zedit_save_to_disk(int zone_num);
void zedit_create_index(int znum, char *type);
void zedit_new_zone(struct char_data *ch, int vzone_num);

/*-------------------------------------------------------------------*/

/*
 * Nasty internal macros to clean up the code.
 */
#define ZCMD		(zone_table[OLC_ZNUM(d)].cmd[subcmd])
#define MYCMD		(OLC_ZONE(d)->cmd[subcmd])
#define OLC_CMD(d)	(OLC_ZONE(d)->cmd[OLC_VAL(d)])

/*-------------------------------------------------------------------*/

/*
 * Utility functions.
 */

/*-------------------------------------------------------------------*/

void zedit_setup(struct descriptor_data *d, int room_num)
{
  struct zone_data *zone;
  int subcmd = 0, count = 0, cmd_room = -1;

  /*
   * Allocate one scratch zone structure.  
   */
  CREATE(zone, struct zone_data, 1);

  /*
   * Copy all the zone header information over.
   */
  zone->name       = str_dup(zone_table[OLC_ZNUM(d)].name);
  zone->lifespan   = zone_table[OLC_ZNUM(d)].lifespan;
  zone->top        = zone_table[OLC_ZNUM(d)].top;
  zone->reset_mode = zone_table[OLC_ZNUM(d)].reset_mode;
  /*
   * The remaining fields are used as a 'has been modified' flag  
   */
  zone->number = 0;	/* Header information has changed.	*/
  zone->age    = 0;	/* The commands have changed.		*/

  /*
   * Start the reset command list with a terminator.
   */
  CREATE(zone->cmd, struct reset_com, 1);
  zone->cmd[0].command = 'S';

  /*
   * Add all entries in zone_table that relate to this room.
   */
  while (ZCMD.command != 'S') 
  { switch (ZCMD.command) 
    {case 'M':
     case 'O':
      cmd_room = ZCMD.arg3;
      break;
     case 'D':
     case 'R':
     case 'F':
      cmd_room = ZCMD.arg1;
      break;
     case 'V':
      if (ZCMD.arg1 == WLD_TRIGGER)
        cmd_room = real_room(ZCMD.arg3);
      break;
     default:
      break;
    }
    if (cmd_room == room_num) 
       {
#if defined(DEBUG)
        log("zedit_setup called add_cmd_to_list.");
#endif
        add_cmd_to_list(&(zone->cmd), &ZCMD, count);
        count++;
       }
    subcmd++;
  }

  OLC_ZONE(d) = zone;
  /*
   * Display main menu.
   */
  zedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/*
 * Create a new zone.
 */

void zedit_new_zone(struct char_data *ch, int vzone_num)
{
  FILE *fp;
  struct zone_data *new_table;
  int i, room;

  if (vzone_num < 0) 
     {send_to_char("������������� ���� �� ��������������.\r\n", ch);
      return;
     } 
  else 
  if (vzone_num > 999) 
     {send_to_char("����������� �������������� 999 ���.\r\n", ch);
      return;
     }

  /*
   * Make sure the zone does not exist.
   */
  room = vzone_num * 100;
  for (i = 0; i <= top_of_zone_table; i++)
      if ((zone_table[i].number * 100 <= room) && (zone_table[i].top >= room)) 
         {send_to_char("����������� ���� ��� ����������.\r\n", ch);
          return;
         }

  /*
   * Create the zone file.
   */
  sprintf(buf, "%s/%d.zon", ZON_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) 
     {mudlog("SYSERR: OLC: Can't write new zone file", BRF, LVL_IMPL, TRUE);
      send_to_char("������ �������� ����� ����.\r\n", ch);
      return;
     }
  fprintf(fp, "#%d\n����� ����~\n%d 30 2\nS\n$\n", vzone_num,
						(vzone_num * 100) + 99);
  fclose(fp);

  /*
   * Create the room file.
   */
  sprintf(buf, "%s/%d.wld", WLD_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) 
     {mudlog("SYSERR: OLC: Can't write new world file", BRF, LVL_IMPL, TRUE);
      send_to_char("�� ���� ������� ���� ����� ����� ����.\r\n", ch);
      return;
     }
  fprintf(fp, "#%d\n������~\n����������.\n~\n%d 0 0\nS\n$\n",
		  vzone_num * 100, vzone_num);
  fclose(fp);

  /*
   * Create the mobile file.
   */
  sprintf(buf, "%s/%d.mob", MOB_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) 
     {mudlog("SYSERR: OLC: Can't write new mob file", BRF, LVL_IMPL, TRUE);
      send_to_char("�� ���� ������� ���� ����� ����� ����.\r\n", ch);
      return;
     }
  fprintf(fp, "$\n");
  fclose(fp);

  /*
   * Create the object file.
   */
  sprintf(buf, "%s/%d.obj", OBJ_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) 
     {mudlog("SYSERR: OLC: Can't write new obj file", BRF, LVL_IMPL, TRUE);
      send_to_char("�� ���� ������� ���� ��������� ����� ����.\r\n", ch);
      return;
     }
  fprintf(fp, "$\n");
  fclose(fp);

  /*
   * Create the shop file.
   */
  sprintf(buf, "%s/%d.shp", SHP_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) 
     {mudlog("SYSERR: OLC: Can't write new shop file", BRF, LVL_IMPL, TRUE);
      send_to_char("�� ���� ������� ���� ��������� ����� ����.\r\n", ch);
      return;
     }
  fprintf(fp, "$~\n");
  fclose(fp);

  /*
   * Create the trigger file.
   */
  sprintf(buf, "%s/%d.trg", TRG_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) 
     {mudlog("SYSERR: OLC: Can't write new trigger file", BRF, LVL_IMPL, TRUE);
      send_to_char("�� ���� ������� ���� �������� ����� ����.\r\n", ch);
      return;
     }
  fprintf(fp, "$~\n");
  fclose(fp);

  /*
   * Update index files.
   */
  zedit_create_index(vzone_num, "zon");
  zedit_create_index(vzone_num, "wld");
  zedit_create_index(vzone_num, "mob");
  zedit_create_index(vzone_num, "obj");
  zedit_create_index(vzone_num, "shp");
  zedit_create_index(vzone_num, "trg");

  /*
   * Make a new zone in memory. This was the source of all the zedit new
   * crashes reported to the CircleMUD list. It was happily overwriting
   * the stack.  This new loop by Andrew Helm fixes that problem and is
   * more understandable at the same time.
   *
   * The variable is 'top_of_zone_table_table + 2' because we need record 0
   * through top_of_zone (top_of_zone_table + 1 items) and a new one which
   * makes it top_of_zone_table + 2 elements large.
   */
  CREATE(new_table, struct zone_data, top_of_zone_table + 2);
  new_table[top_of_zone_table + 1].number = 99900;

  if (vzone_num > zone_table[top_of_zone_table].number) 
     {/*
       * We're adding to the end of the zone table, copy all of the current
       * top_of_zone_table + 1 items over and set write point to before the
       * the last record for the for() loop below.
       */
      memcpy(new_table, zone_table, (sizeof (struct zone_data) * (top_of_zone_table + 1)));
      i = top_of_zone_table + 1;
     } 
  else
    /*
     * Copy over all the zones that are before this zone.
     */
    for (i = 0; vzone_num > zone_table[i].number; i++)
        new_table[i] = zone_table[i];

  /*
   * Ok, insert the new zone here.
   */  
  new_table[i].name     = str_dup("����� ����");
  new_table[i].number   = vzone_num;
  new_table[i].top      = (vzone_num * 100) + 99;
  new_table[i].lifespan = 30;
  new_table[i].age        = 0;
  new_table[i].reset_mode = 2;
  /*
   * No zone commands, just terminate it with an 'S'
   */
  CREATE(new_table[i].cmd, struct reset_com, 1);
  new_table[i].cmd[0].command = 'S';

  /*
   * Copy remaining zones into the table one higher, unless of course we
   * are appending to the end in which case this loop will not be used.
   */
  for (; i <= top_of_zone_table; i++)
      new_table[i + 1] = zone_table[i];

  /*
   * Look Ma, no memory leak!
   */
  free(zone_table);
  zone_table = new_table;
  top_of_zone_table++;

  /*
   * Previously, creating a new zone while invisible gave you away.
   * That quirk has been fixed with the MAX() statement.
   */
  sprintf(buf, "OLC: %s creates new zone #%d", GET_NAME(ch), vzone_num);
  mudlog(buf, BRF, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE);
  send_to_char("���� ������� ���������.\r\n", ch);

  return;
}

/*-------------------------------------------------------------------*/

void zedit_create_index(int znum, char *type)
{
  FILE *newfile, *oldfile;
  char new_name[32], old_name[32], *prefix;
  int num, found = FALSE;

  switch (*type) 
  {
  case 'z':
    prefix = ZON_PREFIX;
    break;
  case 'w':
    prefix = WLD_PREFIX;
    break;
  case 'o':
    prefix = OBJ_PREFIX;
    break;
  case 'm':
    prefix = MOB_PREFIX;
    break;
  case 's':
    prefix = SHP_PREFIX;
    break;
  case 't':
    prefix = TRG_PREFIX;
    break;
  default:
    /*
     * Caller messed up  
     */
    return;
  }

  sprintf(old_name, "%s/index", prefix);
  sprintf(new_name, "%s/newindex", prefix);

  if (!(oldfile = fopen(old_name, "r"))) 
     {sprintf(buf, "SYSERR: OLC: Failed to open %s", buf);
      mudlog(buf, BRF, LVL_IMPL, TRUE);
      return;
     } 
  else 
  if (!(newfile = fopen(new_name, "w"))) 
     {sprintf(buf, "SYSERR: OLC: Failed to open %s", buf);
      mudlog(buf, BRF, LVL_IMPL, TRUE);
      return;
     }

  /*
   * Index contents must be in order: search through the old file for the
   * right place, insert the new file, then copy the rest over. 
   */
  sprintf(buf1, "%d.%s", znum, type);
  while (get_line(oldfile, buf)) 
        {if (*buf == '$') 
            {fprintf(newfile, "%s\n$\n", (!found ? buf1 : ""));
             break;
            } 
         else 
         if (!found) 
            {sscanf(buf, "%d", &num);
             if (num > znum) 
                {found = TRUE;
                 fprintf(newfile, "%s\n", buf1);
                }
            }
         fprintf(newfile, "%s\n", buf);
        }

  fclose(newfile);
  fclose(oldfile);
  /*
   * Out with the old, in with the new.
   */
  remove(old_name);
  rename(new_name, old_name);
}

/*-------------------------------------------------------------------*/

/*
 * Save all the information in the player's temporary buffer back into
 * the current zone table.
 */
void zedit_save_internally(struct descriptor_data *d)
{
  int subcmd = 0, cmd_room = -2, room_num = real_room(OLC_NUM(d));

  /*
   * Delete all entries in zone_table that relate to this room so we
   * can add all the ones we have in their place.
   */
  while (ZCMD.command != 'S') 
        {switch (ZCMD.command) 
         {
    case 'M':
    case 'O':
      cmd_room = ZCMD.arg3;
      break;
    case 'D':
    case 'R':
    case 'F':
      cmd_room = ZCMD.arg1;
      break;
    default:
      break;
         }
         if (cmd_room == room_num) 
            {
#if defined(DEBUG)
             log("zedit_save_internally called remove_cmd_from_list.");
#endif
             remove_cmd_from_list(&(zone_table[OLC_ZNUM(d)].cmd), subcmd);
            } 
         else
            subcmd++;
        }

  /*
   * Now add all the entries in the players descriptor list  
   */
  subcmd = 0;
  while (MYCMD.command != 'S') 
        {
#if defined(DEBUG)
         log("zedit_save_internally called add_cmd_to_list.");
#endif
         add_cmd_to_list(&(zone_table[OLC_ZNUM(d)].cmd), &MYCMD, subcmd);
         subcmd++;
        }

  /*
   * Finally, if zone headers have been changed, copy over  
   */
  if (OLC_ZONE(d)->number) 
     {free(zone_table[OLC_ZNUM(d)].name);
      zone_table[OLC_ZNUM(d)].name       = str_dup(OLC_ZONE(d)->name);
      zone_table[OLC_ZNUM(d)].top        = OLC_ZONE(d)->top;
      zone_table[OLC_ZNUM(d)].reset_mode = OLC_ZONE(d)->reset_mode;
      zone_table[OLC_ZNUM(d)].lifespan = OLC_ZONE(d)->lifespan;
     }
  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_ZONE);
}

/*-------------------------------------------------------------------*/

/*
 * Save all the zone_table for this zone to disk.  This function now
 * writes simple comments in the form of (<name>) to each record.  A
 * header for each field is also there.
 */
#undef  ZCMD
#define ZCMD	(zone_table[zone_num].cmd[subcmd])
void zedit_save_to_disk(int zone_num)
{
  int subcmd, arg1 = -1, arg2 = -1, arg3 = -1, arg4 = -1;
  char fname[64];
  const char *comment = NULL;
  FILE *zfile;

  sprintf(fname, "%s/%d.new", ZON_PREFIX, zone_table[zone_num].number);
  if (!(zfile = fopen(fname, "w"))) 
     {sprintf(buf, "SYSERR: OLC: zedit_save_to_disk:  Can't write zone %d.",
	          zone_table[zone_num].number);
      mudlog(buf, BRF, LVL_BUILDER, TRUE);
      return;
     }

  /*
   * Print zone header to file  
   */
  fprintf(zfile, "#%d\n" "%s~\n" "%d %d %d\n",
          zone_table[zone_num].number,
	      (zone_table[zone_num].name && *zone_table[zone_num].name)
		  ? zone_table[zone_num].name : "������������",
          zone_table[zone_num].top,
          zone_table[zone_num].lifespan,
          zone_table[zone_num].reset_mode
         );

#if defined(ZEDIT_HELP_IN_FILE)
  fprintf(zfile,"* Field #1    Field #3   Field #4  Field #5   Field #6\n"
		        "* M (Mobile)  Mob-Vnum   Wld-Max   Room-Vnum  Room-Max\n"
		        "* F (Follow)  Room-Vnum  LMob-Vnum FMob->Vnum\n"
                "* O (Object)  Obj-Vnum   Wld-Max   Room-Vnum\n"
                "* G (Give)    Obj-Vnum   Wld-Max   Unused\n"
                "* E (Equip)   Obj-Vnum   Wld-Max   EQ-Position\n"
                "* P (Put)     Obj-Vnum   Wld-Max   Target-Obj-Vnum\n"
                "* D (Door)    Room-Vnum  Door-Dir  Door-State\n"
                "* R (Remove)  Room-Vnum  Obj-Vnum  Unused\n"
                "* Q (Purge)   Mob-Vnum   Unused    Unused\n"
                "* T (Trigger) Trig-Type  Trig-Vnum Unused\n"
                "* V (Variable)Trig-Type  Context   Room#  Varname Value\n"
  );
#endif

  for (subcmd = 0; ZCMD.command != 'S'; subcmd++) 
      {arg1 = arg2 = arg3 = arg4 = -1;
       switch (ZCMD.command) 
              {
    case 'M':
      arg1    = mob_index[ZCMD.arg1].vnum;
      arg2    = ZCMD.arg2;
      arg3    = world[ZCMD.arg3].number;
      arg4    = ZCMD.arg4;
      comment = mob_proto[ZCMD.arg1].player.short_descr;
      break;
    case 'F':
      arg1    = world[ZCMD.arg1].number;
      arg2    = mob_index[ZCMD.arg2].vnum;
      arg3    = mob_index[ZCMD.arg3].vnum;
      comment = mob_proto[ZCMD.arg2].player.short_descr;
      break;
    case 'O':
      arg1    = obj_index[ZCMD.arg1].vnum;
      arg2    = ZCMD.arg2;
      arg3    = world[ZCMD.arg3].number;
      comment = obj_proto[ZCMD.arg1].short_description;
      break;
    case 'G':
      arg1    = obj_index[ZCMD.arg1].vnum;
      arg2    = ZCMD.arg2;
      arg3    = -1;
      comment = obj_proto[ZCMD.arg1].short_description;
      break;
    case 'E':
      arg1    = obj_index[ZCMD.arg1].vnum;
      arg2    = ZCMD.arg2;
      arg3    = ZCMD.arg3;
      comment = obj_proto[ZCMD.arg1].short_description;
      break;
    case 'Q':
      arg1    = mob_index[ZCMD.arg1].vnum;
      arg2    = -1;
      arg3    = -1;
      comment = mob_proto[ZCMD.arg1].player.short_descr;
      break;
    case 'P':
      arg1    = obj_index[ZCMD.arg1].vnum;
      arg2    = ZCMD.arg2;
      arg3    = obj_index[ZCMD.arg3].vnum;
      comment = obj_proto[ZCMD.arg1].short_description;
      break;
    case 'D':
      arg1    = world[ZCMD.arg1].number;
      arg2    = ZCMD.arg2;
      arg3    = ZCMD.arg3;
      comment = world[ZCMD.arg1].name;
      break;
    case 'R':
      arg1    = world[ZCMD.arg1].number;
      arg2    = obj_index[ZCMD.arg2].vnum;
      comment = obj_proto[ZCMD.arg2].short_description;
      arg3    = -1;
      break;
    case 'T':
      arg1    = ZCMD.arg1; /* trigger type */
      arg2    = ZCMD.arg2;
      break;
    case 'V':
      arg1    = ZCMD.arg1; /* trigger type */
      arg2    = ZCMD.arg2; /* context */
      break;
    case '*':
      /*
       * Invalid commands are replaced with '*' - Ignore them.
       */
      continue;
    default:
      sprintf(buf, "SYSERR: OLC: z_save_to_disk(): Unknown cmd '%c' - NOT saving", ZCMD.command);
      mudlog(buf, BRF, LVL_BUILDER, TRUE);
      continue;
              }
       if (ZCMD.command != 'V')
          fprintf(zfile, "%c %d %d %d %d %d\t(%s)\n",
                  ZCMD.command, ZCMD.if_flag, arg1, arg2, arg3, arg4, comment);
       else
          fprintf(zfile, "%c %d %d %d %d %s %s\n",
                  ZCMD.command, ZCMD.if_flag, arg1, arg2, arg3, ZCMD.sarg1,
                  ZCMD.sarg2);
      }
  fprintf(zfile, "S\n$\n");
  fclose(zfile);
  sprintf(buf2, "%s/%d.zon", ZON_PREFIX, zone_table[zone_num].number);
  /*
   * We're fubar'd if we crash between the two lines below.
   */
  remove(buf2);
  rename(fname, buf2);

  olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_ZONE);
}

/*-------------------------------------------------------------------*/

/*
 * Some common code to count the number of comands in the list.
 */
int count_commands(struct reset_com *list)
{
  int count = 0;

  while (list[count].command != 'S')
        count++;

  return count;
}

/*-------------------------------------------------------------------*/

/*
 * Adds a new reset command into a list.  Takes a pointer to the list
 * so that it may play with the memory locations.
 */
void add_cmd_to_list(struct reset_com **list, struct reset_com *newcmd, int pos)
{
  int    count, i, l;
  struct reset_com *newlist;

  /*
   * Count number of commands (not including terminator).
   */
  count = count_commands(*list);

  /*
   * Value is +2 for the terminator and new field to add.
   */
  CREATE(newlist, struct reset_com, count + 2);

  /*
   * Even tighter loop to copy the old list and insert a new command.
   */
  for (i = 0, l = 0; i <= count; i++) 
      {newlist[i] = ((i == pos) ? *newcmd : (*list)[l++]);
#if defined(DEBUG)
       sprintf(buf, "add_cmd_to_list: added %c %d %d %d %d",
               newlist[i].command, newlist[i].arg1, newlist[i].arg2,
               newlist[i].arg3, newlist[i].line);
       log(buf);
#endif
      }

  /*
   * Add terminator, then insert new list.
   */
  newlist[count + 1].command = 'S';
  free(*list);
  *list = newlist;
}

/*-------------------------------------------------------------------*/

/*
 * Remove a reset command from a list.  Takes a pointer to the list
 * so that it may play with the memory locations.
 */
void remove_cmd_from_list(struct reset_com **list, int pos)
{
  int count, i, l;
  struct reset_com *newlist;

  /*
   * Count number of commands (not including terminator)  
   */
  count = count_commands(*list);

  /*
   * Value is 'count' because we didn't include the terminator above
   * but since we're deleting one thing anyway we want one less.
   */
  CREATE(newlist, struct reset_com, count);

  /*
   * Even tighter loop to copy old list and skip unwanted command.
   */
  for (i = 0, l = 0; i < count; i++) 
      {if (i != pos) 
          {
#if defined(DEBUG)
           sprintf(buf, "remove_cmd_from_list: kept %c %d %d %d %d",
	               (*list)[i].command, (*list)[i].arg1, (*list)[i].arg2,
		           (*list)[i].arg3, (*list)[i].line);
#endif
           newlist[l++] = (*list)[i];
          }
#if defined(DEBUG)
       else
          sprintf(buf, "remove_cmd_from_list: deleted %c %d %d %d %d",
		          (*list)[i].command, (*list)[i].arg1, (*list)[i].arg2,
		          (*list)[i].arg3, (*list)[i].line);
       log(buf);
#endif
      }
  /*
   * Add the terminator, then insert the new list.
   */
  newlist[count - 1].command = 'S';
  free(*list);
  *list = newlist;
}

/*-------------------------------------------------------------------*/

/*
 * Error check user input and then add new (blank) command  
 */
int new_command(struct descriptor_data *d, int pos)
{
  int    subcmd = 0;
  struct reset_com *new_com;

  /*
   * Error check to ensure users hasn't given too large an index  
   */
  while (MYCMD.command != 'S')
        subcmd++;

  if ((pos > subcmd) || (pos < 0))
     return 0;

  /*
   * Ok, let's add a new (blank) command 
   */
  CREATE(new_com, struct reset_com, 1);
  new_com->command = 'N';
#if defined(DEBUG)
  log("new_command called add_cmd_to_list.");
#endif
  add_cmd_to_list(&OLC_ZONE(d)->cmd, new_com, pos);
  return 1;
}

/*-------------------------------------------------------------------*/
/*
 * Error check user input and then remove command  
 */

void delete_command(struct descriptor_data *d, int pos)
{
  int subcmd = 0;

  /*
   * Error check to ensure users hasn't given too large an index  
   */
  while (MYCMD.command != 'S')
        subcmd++;

  if ((pos >= subcmd) || (pos < 0))
     return;

  /*
   * Ok, let's zap it  
   */
#if defined(DEBUG)
  log("delete_command called remove_cmd_from_list.");
#endif
  remove_cmd_from_list(&OLC_ZONE(d)->cmd, pos);
}

/*-------------------------------------------------------------------*/
/*
 * Error check user input and then setup change  
 */

int start_change_command(struct descriptor_data *d, int pos)
{
  int subcmd = 0;

  /*
   * Error check to ensure users hasn't given too large an index  
   */
  while (MYCMD.command != 'S')
        subcmd++;

  if ((pos >= subcmd) || (pos < 0))
     return 0;

  /*
   * Ok, let's get editing  
   */
  OLC_VAL(d) = pos;
  return 1;
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * the main menu 
 */
void zedit_disp_menu(struct descriptor_data *d)
{
  int subcmd = 0, room, counter = 0;

  get_char_cols(d->character);
  room = real_room(OLC_NUM(d));

  /*
   * Menu header  
   */
  sprintf(buf,
#if defined(CLEAR_SCREEN)
	  "[H[J"
#endif
	  "Room number: %s%d%s		Room zone: %s%d\r\n"
	  "%sZ%s) ��� ����    : %s%s\r\n"
	  "%sL%s) ����� ����� : %s%d minutes\r\n"
	  "%sT%s) ����.�������: %s%d\r\n"
	  "%sR%s) ��� ������� : %s%s%s\r\n"
	  "[Command list]\r\n",

	  cyn, OLC_NUM(d), nrm,
	  cyn, zone_table[OLC_ZNUM(d)].number,
	  grn, nrm, yel, OLC_ZONE(d)->name ? OLC_ZONE(d)->name : "<NONE!>",
	  grn, nrm, yel, OLC_ZONE(d)->lifespan,
	  grn, nrm, yel, OLC_ZONE(d)->top,
	  grn, nrm, yel, OLC_ZONE(d)->reset_mode ?
                    ((OLC_ZONE(d)->reset_mode == 1) ?
	                 "���� � ���� ��� �������." :
	                 "������� �������.") :
	                "������� �� ���������", nrm
	  );

  /*
   * Print the commands for this room into display buffer.
   */
  while (MYCMD.command != 'S') 
  { /*
     * Translate what the command means.
     */
    switch (MYCMD.command) 
    {case 'M':
          sprintf(buf2, "%s��������� %s [%s%d%s], Max(� ����) : %d, Max(� �������) : %d",
	              MYCMD.if_flag ? " ����� " : "",
	              mob_proto[MYCMD.arg1].player.short_descr,
	              cyn, mob_index[MYCMD.arg1].vnum, yel,
	              MYCMD.arg2,
	              MYCMD.arg4
	             );
      break;
     case 'F':
          sprintf(buf2, "%s��������� �� %s [%s%d%s] %s [%s%d%s]",
	              MYCMD.if_flag ? " ����� " : "",
	              mob_proto[MYCMD.arg2].player.short_descr,
	              cyn, mob_index[MYCMD.arg2].vnum, yel,
	              mob_proto[MYCMD.arg3].player.short_descr,
	              cyn, mob_index[MYCMD.arg3].vnum, yel
	             );
      break;
     case 'Q':
          sprintf(buf2, "%s������ ���� ����� %s [%s%d%s]",
	              MYCMD.if_flag ? " ����� " : "",
	              mob_proto[MYCMD.arg1].player.short_descr,
	              cyn, mob_index[MYCMD.arg1].vnum, yel
	             );
      break;
    case 'G':
      sprintf(buf2, "%s���� ��� %s [%s%d%s], Max : %d",
	      MYCMD.if_flag ? " ����� " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      MYCMD.arg2
	      );
      break;
    case 'O':
      sprintf(buf2, "%s��������� %s [%s%d%s], Max : %d",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      MYCMD.arg2
	      );
      break;
    case 'E':
      sprintf(buf2, "%s����������� %s [%s%d%s], %s, Max : %d",
	      MYCMD.if_flag ? " ����� " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      equipment_types[MYCMD.arg3],
	      MYCMD.arg2
	      );
      break;
    case 'P':
      sprintf(buf2, "%s��������� %s [%s%d%s] � %s [%s%d%s], Max : %d",
	      MYCMD.if_flag ? " ����� " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      obj_proto[MYCMD.arg3].short_description,
	      cyn, obj_index[MYCMD.arg3].vnum, yel,
	      MYCMD.arg2
	      );
      break;
    case 'R':
      sprintf(buf2, "%s������� %s [%s%d%s] �� �������.",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg2].short_description,
	      cyn, obj_index[MYCMD.arg2].vnum, yel
	      );
      break;
    case 'D':
      sprintf(buf2, "%s���������� ����� %s ��� %s.",
	      MYCMD.if_flag ? " then " : "",
	      dirs[MYCMD.arg2],
	      MYCMD.arg3 == 0 ? "��������" :
	      MYCMD.arg3 == 1 ? "��������" : 
	      MYCMD.arg3 == 2 ? "��������" : 
	      MYCMD.arg3 == 3 ? "�������"  : 
	      MYCMD.arg3 == 4 ? "�����"    : "���������� ���"
	      );
      break;
    case 'T':
      sprintf(buf2, "%s��������� ������ %s%s%s [%s%d%s] � %s",
        MYCMD.if_flag ? " ����� " : "",
        cyn, trig_index[real_trigger(MYCMD.arg2)]->proto->name, yel, 
        cyn, MYCMD.arg2, yel,
        ((MYCMD.arg1 == MOB_TRIGGER) ? "����" :
          ((MYCMD.arg1 == OBJ_TRIGGER) ? "��������" :
            ((MYCMD.arg1 == WLD_TRIGGER)? "�������" : "????"))));
      break;
    case 'V':
      sprintf(buf2, "%s��������� ���������� ���������� %s:%d to %s = %s",
        MYCMD.if_flag ? " then " : "",
        MYCMD.sarg1, MYCMD.arg2,
        ((MYCMD.arg1 == MOB_TRIGGER) ? "����" :
          ((MYCMD.arg1 == OBJ_TRIGGER) ? "��������" :
            ((MYCMD.arg1 == WLD_TRIGGER)? "�������" : "????"))),
        MYCMD.sarg2);
      break;
    default:
      strcpy(buf2, "<����������� �������>");
      break;
    }
    /*
     * Build the display buffer for this command  
     */
    sprintf(buf1, "%s%d - %s%s\r\n", nrm, counter++, yel, buf2);
    strcat(buf, buf1);
    subcmd++;
  }
  /*
   * Finish off menu  
   */
  sprintf(buf1,
	  "%s%d - <END OF LIST>\r\n"
	  "%sN%s) ����� �������.\r\n"
	  "%sE%s) ������������� �������.\r\n"
	  "%sD%s) ������� �������.\r\n"
	  "%sX%s) �����\r\n��� ����� : ",
	  nrm, counter, grn, nrm, grn, nrm, grn, nrm, grn, nrm
	  );

  strcat(buf, buf1);
  send_to_char(buf, d->character);

  OLC_MODE(d) = ZEDIT_MAIN_MENU;
}

/*-------------------------------------------------------------------*/

/*
 * Print the command type menu and setup response catch. 
 */
void zedit_disp_comtype(struct descriptor_data *d)
{
  get_char_cols(d->character);
  sprintf(buf,
#if defined(CLEAR_SCREEN)
	"[H[J"
#endif
	"%sM%s) �������� ���� � �������         %sO%s) �������� ������� � �������\r\n"
	"%sE%s) ����������� ����                %sG%s) ���� ������� ����r\n"
	"%sP%s) ��������� ������� � ���������   %sD%s) ���������� �����\r\n"
	"%sR%s) ������� ������� �� �������      %sQ%s) ������� ���� ����� ������� ����\r\n"
	"%sF%s) ������� ������� ��������������\r\n"
    "%sT%s) ��������� ������                %sV%s) ���������� ���������� ����������\r\n"
	"������� ��� ������� : ",
	grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm,
	grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm
	     );
  send_to_char(buf, d->character);
  OLC_MODE(d) = ZEDIT_COMMAND_TYPE;
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg1 and set
 * up the input catch clause  
 */
void zedit_disp_arg1(struct descriptor_data *d)
{
  switch (OLC_CMD(d).command) 
  {
  case 'M':
  case 'Q':
    send_to_char("������� ����� ���� : ", d->character);
    OLC_MODE(d) = ZEDIT_ARG1;
    break;
  case 'O':
  case 'E':
  case 'P':
  case 'G':
    send_to_char("������� ����� �������� : ", d->character);
    OLC_MODE(d) = ZEDIT_ARG1;
    break;
  case 'D':
  case 'R':
  case 'F':
    /*
     * Arg1 for these is the room number, skip to arg2  
     */
    OLC_CMD(d).arg1 = real_room(OLC_NUM(d));
    zedit_disp_arg2(d);
    break;
  case 'T':
  case 'V':
    send_to_char("������� ��� ������� (0:���, 1:�������, 2:�������) :", d->character);
    OLC_MODE(d) = ZEDIT_ARG1;
    break;
  default:
    /*
     * We should never get here  .
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_disp_arg1(): Help!", BRF, LVL_BUILDER, TRUE);
    send_to_char("Oops...\r\n", d->character);
    return;
  }
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg2 and set
 * up the input catch clause.
 */
void zedit_disp_arg2(struct descriptor_data *d)
{
  int i = 0;

  switch (OLC_CMD(d).command) 
  {case 'M':
   case 'O':
   case 'E':
   case 'P':
   case 'G':
    send_to_char("������� ������������ ���������� � ���� : ", d->character);
    break;
   case 'D':
    while (*dirs[i] != '\n') 
          {sprintf(buf, "%d) Exit %s.\r\n", i, dirs[i]);
           send_to_char(buf, d->character);
           i++;
          }
    send_to_char("������� ���������� ������ : ", d->character);
    break;
  case 'R':
    send_to_char("������� ����� �������� : ", d->character);
    break;
  case 'F':
    send_to_char("������� ����� ����-������ : ", d->character);
    break;
  case 'T':
    send_to_char("������� ����� ������� : ", d->character);
    break;
  case 'V':
    send_to_char("���������� �������� (0 - ��� ���������) : ", d->character);
    break;
  case 'Q':
  default:
    /*
     * We should never get here, but just in case...
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_disp_arg2(): Help!", BRF, LVL_BUILDER, TRUE);
    send_to_char("�������...\r\n", d->character);
    return;
  }
  OLC_MODE(d) = ZEDIT_ARG2;
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg3 and set
 * up the input catch clause.
 */
void zedit_disp_arg3(struct descriptor_data *d)
{
  int i = 0;

  switch (OLC_CMD(d).command) 
  {case 'E':
    while (*equipment_types[i] != '\n') 
          {sprintf(buf, "%2d) %26.26s %2d) %26.26s\r\n", i,
                   equipment_types[i], i + 1, (*equipment_types[i + 1] != '\n') ?
                   equipment_types[i + 1] : "");
           send_to_char(buf, d->character);
           if (*equipment_types[i + 1] != '\n')
              i += 2;
           else
              break;
          }
    send_to_char("�������� ������� : ", d->character);
    break;
  case 'P':
    send_to_char("Vnum ���������� : ", d->character);
    break;
  case 'D':
    send_to_char("0)  ����� �������\r\n"
                 "1)  ����� �������\r\n"
                 "2)  ����� �������\r\n"
                 "3)  ����� �����\r\n"
                 "4)  ����� �����\r\n"
                 "������� ��������� �����(������) : ", d->character);
    break;
  case 'V':
    send_to_char("������� ��� ���������� ����������: ", d->character);
    OLC_MODE(d) = ZEDIT_SARG1;
    return;
  case 'M':
    send_to_char("������� ������������ ���������� � ������� : ", d->character);
    break;
  case 'F':
    send_to_char("������� ����� ����-������������� : ", d->character);
    break;
  case 'Q':
  case 'O':
  case 'R':
  case 'G':
  default:
    /*
     * We should never get here, just in case.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_disp_arg3(): Help!", BRF, LVL_BUILDER, TRUE);
    send_to_char("�������...\r\n", d->character);
    return;
  }
  OLC_MODE(d) = ZEDIT_ARG3;
}

/**************************************************************************
  The GARGANTAUN event handler
 **************************************************************************/

void zedit_parse(struct descriptor_data *d, char *arg)
{
  int pos, i = 0;

  switch (OLC_MODE(d)) 
  {
/*-------------------------------------------------------------------*/
  case ZEDIT_CONFIRM_SAVESTRING:
    switch (*arg) 
    {case 'y': case 'Y': case '�': case '�':
      /*
       * Save the zone in memory, hiding invisible people.
       */
      send_to_char("�������� ���� � ������.\r\n", d->character);
      zedit_save_internally(d);
      sprintf(buf, "OLC: %s edits zone info for room %d.",
              GET_NAME(d->character), OLC_NUM(d));
      mudlog(buf, CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE);
      /* FALL THROUGH */
    case 'n': case 'N': case '�': case '�':
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      send_to_char("�������� �����!\r\n", d->character);
      send_to_char("�� ������� ��������� ���� � ������ ? : ", d->character);
      break;
    }
    break;
   /* End of ZEDIT_CONFIRM_SAVESTRING */

/*-------------------------------------------------------------------*/
  case ZEDIT_MAIN_MENU:
    switch (*arg) 
    {case 'x': case 'X':
      if (OLC_ZONE(d)->age || OLC_ZONE(d)->number) 
         {send_to_char("�� ������� ��������� ��������� ���� � ������ ? (y/n) : ", d->character);
          OLC_MODE(d) = ZEDIT_CONFIRM_SAVESTRING;
         } 
      else 
         {send_to_char("�� ���� ���������.\r\n", d->character);
          cleanup_olc(d, CLEANUP_ALL);
         }
      break;
     case 'n': case 'N':
      /*
       * New entry.
       */
      send_to_char("�������� ����� ����� ������� ? : ", d->character);
      OLC_MODE(d) = ZEDIT_NEW_ENTRY;
      break;
     case 'e': case 'E':
      /*
       * Change an entry.
       */
      send_to_char("����� ������� �� ������ �������� ? : ", d->character);
      OLC_MODE(d) = ZEDIT_CHANGE_ENTRY;
      break;
     case 'd': case 'D':
      /*
       * Delete an entry.
       */
      send_to_char("����� ������� �� ������ ������� ? : ", d->character);
      OLC_MODE(d) = ZEDIT_DELETE_ENTRY;
      break;
    case 'z': case 'Z':
      /*
       * Edit zone name.
       */
      send_to_char("������� ����� ��� ���� : ", d->character);
      OLC_MODE(d) = ZEDIT_ZONE_NAME;
      break;
    case 't': case 'T':
      /*
       * Edit top of zone.
       */
      if (GET_LEVEL(d->character) < LVL_IMPL)
         zedit_disp_menu(d);
      else 
         {send_to_char("������� ����� ������� ������� ����.\r\n"
                       "�������, ��� ������ ������ ���� ����� ���������*100+99 : ", d->character);
          OLC_MODE(d) = ZEDIT_ZONE_TOP;
         }
      break;
    case 'l': case 'L':
      /*
       * Edit zone lifespan.
       */
      send_to_char("������� ����� ����� ����� ���� : ", d->character);
      OLC_MODE(d) = ZEDIT_ZONE_LIFE;
      break;
    case 'r': case 'R':
      /*
       * Edit zone reset mode.
       */
      send_to_char("\r\n"
		   "0) ������� �� �������\r\n"
		   "1) �������, ���� � ���� ��� �������\r\n"
		   "2) ������� �������\r\n"
		   "�������� ��� ������� ���� : ", d->character);
      OLC_MODE(d) = ZEDIT_ZONE_RESET;
      break;
    default:
      zedit_disp_menu(d);
      break;
    }
    break;
    /* End of ZEDIT_MAIN_MENU */

/*-------------------------------------------------------------------*/
  case ZEDIT_NEW_ENTRY:
    /*
     * Get the line number and insert the new line.
     */
    pos = atoi(arg);
    if (isdigit(*arg) && new_command(d, pos)) 
       {if (start_change_command(d, pos)) 
           {zedit_disp_comtype(d);
            OLC_ZONE(d)->age = 1;
           }
       } 
    else
       zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_DELETE_ENTRY:
    /*
     * Get the line number and delete the line.
     */
    pos = atoi(arg);
    if (isdigit(*arg)) 
       {delete_command(d, pos);
        OLC_ZONE(d)->age = 1;
       }
    zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_CHANGE_ENTRY:
    /*
     * Parse the input for which line to edit, and goto next quiz.
     */
    pos = atoi(arg);
    if (isdigit(*arg) && start_change_command(d, pos)) 
       {zedit_disp_comtype(d);
        OLC_ZONE(d)->age = 1;
       } 
    else
       zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_COMMAND_TYPE:
    /*
     * Parse the input for which type of command this is, and goto next
     * quiz.
     */
    OLC_CMD(d).command = toupper(*arg);
    if (!OLC_CMD(d).command || (strchr("MFQOPEDGRTV", OLC_CMD(d).command) == NULL)) 
       {send_to_char("�������� �����, ��������� : ", d->character);
       } 
    else 
       {if (OLC_VAL(d)) 
           {/* If there was a previous command. */
            if (OLC_CMD(d).command == 'T' || OLC_CMD(d).command == 'V') 
               {OLC_CMD(d).if_flag = 1;
                zedit_disp_arg1(d);
               } 
            else 
               {send_to_char("������� �� ��� ������� �� ��������� ���������� ���������� ? (y/n)\r\n", d->character);
                OLC_MODE(d) = ZEDIT_IF_FLAG;
               }
           } 
        else 
           {/* 'if-flag' not appropriate. */
	        OLC_CMD(d).if_flag = 0;
	        zedit_disp_arg1(d);
           }
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_IF_FLAG:
    /*
     * Parse the input for the if flag, and goto next quiz.
     */
    switch (*arg) 
    {case 'y': case 'Y': case '�': case '�':
      OLC_CMD(d).if_flag = 1;
      break;
     case 'n': case 'N': case '�': case '�':
      OLC_CMD(d).if_flag = 0;
      break;
     default:
      send_to_char("��������� ���� : ", d->character);
      return;
    }
    zedit_disp_arg1(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ARG1:
    /*
     * Parse the input for arg1, and goto next quiz.
     */
    if (!isdigit(*arg)) 
       {send_to_char("������ ���� �������� ��������, ��������� : ", d->character);
        return;
       }
    switch (OLC_CMD(d).command) 
    {case 'M':
     case 'Q':
       if ((pos = real_mobile(atoi(arg))) >= 0) 
          {OLC_CMD(d).arg1 = pos;
           if (OLC_CMD(d).command == 'Q')
              zedit_disp_menu(d);
           else
              zedit_disp_arg2(d);
          } 
       else
	      send_to_char("��� ������ ����, ��������� : ", d->character);
       break;
     case 'O':
     case 'P':
     case 'E':
     case 'G':
       if ((pos = real_object(atoi(arg))) >= 0) 
          {OLC_CMD(d).arg1 = pos;
	       zedit_disp_arg2(d);
          } 
       else
	      send_to_char("��� ������ ��������, ��������� : ", d->character);
      break;
     case 'T':
     case 'V':
       if (atoi(arg)<MOB_TRIGGER || atoi(arg)>WLD_TRIGGER)
          send_to_char("�������� �����.", d->character);
       else 
          {OLC_CMD(d).arg1 = atoi(arg);
           zedit_disp_arg2(d);
          }
      break;
    case 'F':
    case 'D':
    case 'R':
    default:
      /*
       * We should never get here.
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_parse(): case ARG1: Ack!", BRF, LVL_BUILDER, TRUE);
      send_to_char("�������...\r\n", d->character);
      break;
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ARG2:
    /*
     * Parse the input for arg2, and goto next quiz.
     */
    if (!isdigit(*arg)) {
      send_to_char("Must be a numeric value, try again : ", d->character);
      return;
    }
    switch (OLC_CMD(d).command) 
    {case 'M':
     case 'O':
      OLC_CMD(d).arg2 = atoi(arg);
      OLC_CMD(d).arg3 = real_room(OLC_NUM(d));
      if (OLC_CMD(d).command == 'M')
         zedit_disp_arg3(d);
      else
         zedit_disp_menu(d);
      break;
      
    case 'F':
      if ((pos = real_mobile(atoi(arg))) >= 0) 
         {OLC_CMD(d).arg2 = pos;
	      zedit_disp_arg3(d);
         } 
      else
	     send_to_char("��� ������ ����, ��������� ���� : ", d->character);
      break;
      
    case 'G':
      OLC_CMD(d).arg2 = atoi(arg);
      zedit_disp_menu(d);
      break;
      
    case 'P':
    case 'E':
      OLC_CMD(d).arg2 = atoi(arg);
      zedit_disp_arg3(d);
      break;
      
    case 'V':
      OLC_CMD(d).arg2 = atoi(arg);
      if (OLC_CMD(d).arg1 == WLD_TRIGGER)
         OLC_CMD(d).arg3 = OLC_NUM(d);
      send_to_char("������� ���������� ��� : ", d->character);
      OLC_MODE(d) = ZEDIT_SARG1;
      break;
      
    case 'D':
      pos = atoi(arg);
      /*
       * Count directions.
       */
      while (*dirs[i] != '\n')
            i++;
      if ((pos < 0) || (pos > i))
         send_to_char("��������� ���� : ", d->character);
      else 
         {OLC_CMD(d).arg2 = pos;
          zedit_disp_arg3(d);
         }
      break;
      
    case 'R':
      if ((pos = real_object(atoi(arg))) >= 0) 
         {OLC_CMD(d).arg2 = pos;
	      zedit_disp_menu(d);
         } 
      else
	     send_to_char("��� ������ ��������, ��������� ���� : ", d->character);
      break;
      
    case 'T':
      if (real_trigger(atoi(arg)) >= 0) 
         {OLC_CMD(d).arg2 = atoi(arg);
          zedit_disp_menu(d);
         } 
      else
         send_to_char("��� ������ �������, ��������� ���� : ", d->character);
      break;
      
    default:
      /*
       * We should never get here, but just in case...
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_parse(): case ARG2: Ack!", BRF, LVL_BUILDER, TRUE);
      send_to_char("�������...\r\n", d->character);
      break;
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ARG3:
    /*
     * Parse the input for arg3, and go back to main menu.
     */
    if (!isdigit(*arg) && OLC_CMD(d).command != 'V') 
       {send_to_char("������ ���� �������� ��������, ��������� ���� : ", d->character);
        return;
       }
    switch (OLC_CMD(d).command) 
    {case 'M':
      OLC_CMD(d).arg4 = atoi(arg);
      zedit_disp_menu(d);
      break;
      
     case 'F':
      if ((pos = real_mobile(atoi(arg))) >= 0) 
         {OLC_CMD(d).arg3 = pos;
          zedit_disp_menu(d);
         } 
      else
	     send_to_char("��� ������ ����, ��������� ���� : ", d->character);
      break;

     case 'E':
      pos = atoi(arg);
      /*
       * Count number of wear positions.  We could use NUM_WEARS, this is
       * more reliable.
       */
      while (*equipment_types[i] != '\n')
	        i++;
      if ((pos < 0) || (pos > i))
         send_to_char("��������� ���� : ", d->character);
      else 
         {OLC_CMD(d).arg3 = pos;
	      zedit_disp_menu(d);
         }
      break;
      
    case 'P':
      if ((pos = real_object(atoi(arg))) >= 0) 
         {OLC_CMD(d).arg3 = pos;
          zedit_disp_menu(d);
         } 
      else
	     send_to_char("��� ������ ��������, ��������� ���� : ", d->character);
      break;
      
    case 'D':
      pos = atoi(arg);
      if ((pos < 0) || (pos > 4))
         send_to_char("��������� ���� : ", d->character);
      else 
         {OLC_CMD(d).arg3 = pos;
	      zedit_disp_menu(d);
         }
      break;
      
    case 'T':
    case 'V':
    case 'Q':
    case 'O':
    case 'G':
    case 'R':
    default:
      /*
       * We should never get here, but just in case...
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_parse(): case ARG3: Ack!", BRF, LVL_BUILDER, TRUE);
      send_to_char("�������...\r\n", d->character);
      break;
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_SARG1:
    if (strlen(arg)) 
       {OLC_CMD(d).sarg1 = str_dup(arg);
        OLC_MODE(d) = ZEDIT_SARG2;
        send_to_char("������� �������� ���������� ���������� : ", d->character);
       } 
    else
       send_to_char("�� ������ ������� ��� ���������� : ", d->character);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_SARG2:
    if (strlen(arg)) 
       {OLC_CMD(d).sarg2 = str_dup(arg);
        zedit_disp_menu(d);
       } 
    else
       send_to_char("�� ������ ������� �������� ���������� :", d->character);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_NAME:
    /*
     * Add new name and return to main menu.
     */
    if (OLC_ZONE(d)->name)
       free(OLC_ZONE(d)->name);
    else
      log("SYSERR: OLC: ZEDIT_ZONE_NAME: no name to free!");
    OLC_ZONE(d)->name   = str_dup(arg);
    OLC_ZONE(d)->number = 1;
    zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_RESET:
    /*
     * Parse and add new reset_mode and return to main menu.
     */
    pos = atoi(arg);
    if (!isdigit(*arg) || (pos < 0) || (pos > 2))
       send_to_char("��������� ���� (0-2) : ", d->character);
    else 
       {OLC_ZONE(d)->reset_mode = pos;
        OLC_ZONE(d)->number = 1;
        zedit_disp_menu(d);
       }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_LIFE:
    /*
     * Parse and add new lifespan and return to main menu.
     */
    pos = atoi(arg);
    if (!isdigit(*arg) || (pos < 0) || (pos > 240))
       send_to_char("��������� ���� (0-240) : ", d->character);
    else 
       {OLC_ZONE(d)->lifespan = pos;
        OLC_ZONE(d)->number = 1;
        zedit_disp_menu(d);
       }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_TOP:
    /*
     * Parse and add new top room in zone and return to main menu.
     */
    if (OLC_ZNUM(d) == top_of_zone_table)
       OLC_ZONE(d)->top = MAX(OLC_ZNUM(d) * 100, MIN(99900, atoi(arg)));
    else
       OLC_ZONE(d)->top = MAX(OLC_ZNUM(d) * 100, MIN(zone_table[OLC_ZNUM(d) + 1].number * 100, atoi(arg)));
    OLC_ZONE(d)->top = (OLC_ZONE(d)->top / 100) * 100 + 99;
    zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  default:
    /*
     * We should never get here, but just in case...
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_parse(): Reached default case!", BRF, LVL_BUILDER, TRUE);
    send_to_char("�������...\r\n", d->character);
    break;
  }
}

/*
 * End of parse_zedit()  
 */
