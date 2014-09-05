/**************************************************************************
*  File: wldcmd.c                                                         *
*  Usage: contains the command_interpreter for rooms,                     *
*         room commands.                                                  *
*                                                                         *
*                                                                         *
*  $Author: galion $
*  $Date: 1996/08/05 03:27:07 $
*  $Revision: 3.12 $
**************************************************************************/

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "screen.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "db.h"

extern struct room_data *world;
extern const char *dirs[];
extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;

void die(struct char_data * ch, struct char_data * killer);
void sub_write(char *arg, char_data *ch, byte find_invis, int targets);
void send_to_zone(char *messg, int zone_rnum);
void asciiflag_conv(char *flag, void *value);
char_data *get_char_by_room(room_data *room, char *name);
room_data *get_room(char *name);
obj_data *get_obj_by_room(room_data *room, char *name);
int  check_recipe_items(struct char_data * ch, int spellnum, int spelltype, int showrecipe);
void char_dam_message(int dam, struct char_data *ch, struct char_data *victim, int attacktype, int mayflee); // prool
#define WCMD(name)  \
    void (name)(room_data *room, char *argument, int cmd, int subcmd)


struct wld_command_info 
{   char *command;
    void (*command_pointer)
           (room_data *room, char *argument, int cmd, int subcmd);
    int	subcmd;
};


/* do_wsend */
#define SCMD_WSEND        0
#define SCMD_WECHOAROUND  1



/* attaches room vnum to msg and sends it to script_log */
void wld_log(room_data *room, char *msg)
{
    char buf[MAX_INPUT_LENGTH + 100];

    void script_log(char *msg);

    sprintf(buf, "Wld (room %d): %s", room->number, msg);
    script_log(buf);
}


/* sends str to room */
void act_to_room(char *str, room_data *room)
{
    /* no one is in the room */
    if (!room->people)
	   return;

    /*
     * since you can't use act(..., TO_ROOM) for an room, send it
     * TO_ROOM and TO_CHAR for some char in the room.
     * (just dont use $n or you might get strange results)
     */
    act(str, FALSE, room->people, 0, 0, TO_ROOM);
    act(str, FALSE, room->people, 0, 0, TO_CHAR);
}



/* World commands */

/* prints the argument to all the rooms aroud the room */
WCMD(do_wasound)
{
    int  door;
 
    skip_spaces(&argument);

    if (!*argument) 
       {wld_log(room, "wasound called with no argument");
	    return;
       }
  
    for (door = 0; door < NUM_OF_DIRS; door++) 
        {struct room_direction_data *exit;
	
	     if ((exit = room->dir_option[door]) && (exit->to_room != NOWHERE) &&
	          room != &world[exit->to_room])
	        act_to_room(argument, &world[exit->to_room]);
        }
}


WCMD(do_wecho)
{
    skip_spaces(&argument);

    if (!*argument) 
	   wld_log(room, "wecho called with no args");
    else 
	   act_to_room(argument, room);
}


WCMD(do_wsend)
{
    char buf[MAX_INPUT_LENGTH], *msg;
    char_data *ch;
  
    msg = any_one_arg(argument, buf);

    if (!*buf)
       {wld_log(room, "wsend called with no args");
	    return;
       }

    skip_spaces(&msg);

    if (!*msg)
       {wld_log(room, "wsend called without a message");
	    return;
       }

    if ((ch = get_char_by_room(room, buf)))
       {if (subcmd == SCMD_WSEND)
	       sub_write(msg, ch, TRUE, TO_CHAR);
	    else 
	    if (subcmd == SCMD_WECHOAROUND)
	       sub_write(msg, ch, TRUE, TO_ROOM);
       }
    else
	   wld_log(room, "no target found for wsend");
}

static int real_zone(int number)
{
  int counter;
      
  for (counter = 0; counter <= top_of_zone_table; counter++)
      if ((number >= (zone_table[counter].number * 100)) &&
          (number <= (zone_table[counter].top)))
      return counter;
 
  return -1;
}

WCMD(do_wzoneecho)
{
    int zone;
    char zone_name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], *msg;
  
    msg = any_one_arg(argument, zone_name);
    skip_spaces(&msg);

    if (!*zone_name || !*msg)
	   wld_log(room, "wzoneecho called with too few args");
    else 
    if ((zone = real_zone(atoi(zone_name))) < 0)
	   wld_log(room, "wzoneecho called for nonexistant zone");
    else 
       {sprintf(buf, "%s\r\n", msg);
	    send_to_zone(buf, zone);
       }
}


WCMD(do_wdoor)
{
    char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
    char field[MAX_INPUT_LENGTH], *value;
    room_data *rm;
    struct room_direction_data *exit;
    int dir, fd, to_room;

    const char *door_field[] = 
   {"purge",
	"description",
	"flags",
	"key",
	"name",
	"room",
	"\n"
    };


    argument = two_arguments(argument, target, direction);
    value = one_argument(argument, field);
    skip_spaces(&value);

    if (!*target || !*direction || !*field) 
       {wld_log(room, "wdoor called with too few args");
	    return;
       }
  
    if ((rm = get_room(target)) == NULL) 
       {wld_log(room, "wdoor: invalid target");
	    return;
       }
  
    if ((dir = search_block(direction, dirs, FALSE)) == -1) 
       {wld_log(room, "wdoor: invalid direction");
	    return;
       }

    if ((fd = search_block(field, door_field, FALSE)) == -1) 
       {wld_log(room, "wdoor: invalid field");
	    return;
       }

    exit = rm->dir_option[dir];

    /* purge exit */
    if (fd == 0) 
       {if (exit) 
           {if (exit->general_description)
		       free(exit->general_description);
	        if (exit->keyword)
		       free(exit->keyword);
	        free(exit);
	        rm->dir_option[dir] = NULL;
	       }
       }
    else 
       {if (!exit) 
           {CREATE(exit, struct room_direction_data, 1);
	        rm->dir_option[dir] = exit; 
	       }
	    switch (fd) 
	       {
	case 1:  /* description */
	    if (exit->general_description)
		   free(exit->general_description);
	    CREATE(exit->general_description, char, strlen(value) + 3);
	    strcpy(exit->general_description, value);
	    strcat(exit->general_description, "\r\n");
	    break;
	case 2:  /* flags       */
	    asciiflag_conv(value, &exit->exit_info);
	    break;
	case 3:  /* key         */
	    exit->key = atoi(value);
	    break;
	case 4:  /* name        */
	    if (exit->keyword)
		   free(exit->keyword);
	    CREATE(exit->keyword, char, strlen(value) + 1);
	    strcpy(exit->keyword, value);
	    break;
	case 5:  /* room        */
	    if ((to_room = real_room(atoi(value))) != NOWHERE)
		   exit->to_room = to_room;
	    else
		   wld_log(room, "wdoor: invalid door target");
	    break;
	       }
       }
}


WCMD(do_wteleport)
{
    char_data *ch, *next_ch, *horse, *charmee, *ncharmee;
    int  target, nr;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    argument = two_arguments(argument, arg1, arg2);
    skip_spaces(&argument);
  
    if (!*arg1 || !*arg2) 
       {wld_log(room, "wteleport called with too few args");
	    return;
       }

    nr     = atoi(arg2);
    target = real_room(nr);

    if (target == NOWHERE) 
       wld_log(room, "wteleport target is an invalid room");
    else 
    if (!str_cmp(arg1, "all") || !str_cmp(arg1, "все"))
       {if (nr == room->number) 
           {wld_log(room, "wteleport all target is itself");
            return;
           }
        for (ch = room->people; ch; ch = next_ch)
            {next_ch = ch->next_in_room;
             if (on_horse(ch) || has_horse(ch, TRUE))
                horse = get_horse(ch);
             else
                horse = NULL; 
             char_from_room(ch);
             char_to_room(ch, target);
             if (!str_cmp(argument,"horse") && horse)
                {if (horse == next_ch)
                    next_ch = horse->next_in_room;
                 char_from_room(horse);
                 char_to_room(horse,target);
                }
             check_horse(ch);
            }
       }
    else
       {if ((ch = get_char_by_room(room, arg1))) 
           {if (on_horse(ch) || has_horse(ch, TRUE))
               horse = get_horse(ch);
            else
               horse = NULL; 
            for (charmee = world[IN_ROOM(ch)].people; charmee; charmee = ncharmee)
             	{ncharmee = charmee->next_in_room;
	         if (IS_NPC(charmee)                && 
	             AFF_FLAGGED(charmee,AFF_CHARM) &&
		     charmee->master == ch
		    )
		    {char_from_room(charmee);
                     char_to_room(charmee,target);
		    }
	        }
            char_from_room(ch);
            char_to_room(ch, target);
            if (!str_cmp(argument,"horse") && horse)
               {char_from_room(horse);
                char_to_room(horse,target);
               }
            check_horse(ch);
	   }
	else
	   wld_log(room, "wteleport: no target found");
       }
}


WCMD(do_wforce)
{
    char_data *ch, *next_ch;
    char arg1[MAX_INPUT_LENGTH], *line;

    line = one_argument(argument, arg1);
  
    if (!*arg1 || !*line) 
       {wld_log(room, "wforce called with too few args");
	    return;
       }
  
    if (!str_cmp(arg1, "all") || !str_cmp(arg1, "все"))
       {for (ch = room->people; ch; ch = next_ch)
	        {next_ch = ch->next_in_room;
	         if (GET_LEVEL(ch)<LVL_IMMORT)
	            {command_interpreter(ch, line);
	            }
	        }
       }
    else
       {if ((ch = get_char_by_room(room, arg1)))
	       {if (GET_LEVEL(ch)<LVL_IMMORT)
	           {command_interpreter(ch, line);
	           }
	       }
	    else
	       wld_log(room, "wforce: no target found");
       }
}


/* increases the target's exp */
WCMD(do_wexp)
{
    char_data *ch;
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

    two_arguments(argument, name, amount);

    if (!*name || !*amount) 
       {wld_log(room, "wexp: too few arguments");
	    return;
       }

    if ((ch = get_char_by_room(room, name))) 
	   gain_exp(ch, atoi(amount));
    else 
       {wld_log(room, "wexp: target not found");
 	    return;
       }
}



/* purge all objects an npcs in room, or specified object or mob */
WCMD(do_wpurge)
{
    char arg[MAX_INPUT_LENGTH];
    char_data *ch, *next_ch;
    obj_data *obj, *next_obj;

    one_argument(argument, arg);
  
    if (!*arg) 
       {for (ch = room->people; ch; ch = next_ch ) 
            {next_ch = ch->next_in_room;
	         if (IS_NPC(ch))
		    extract_char(ch,FALSE);
	        }
	
	    for (obj = room->contents; obj; obj = next_obj ) 
	        {next_obj = obj->next_content;
	         extract_obj(obj);
	        }
    
	    return;
       }
  
    if (!(ch = get_char_by_room(room, arg))) 
       {if ((obj = get_obj_by_room(room, arg))) 
           {extract_obj(obj);
	       } 
	    else 
	       wld_log(room, "wpurge: bad argument");
	    return;
       }
  
    if (!IS_NPC(ch)) 
       {wld_log(room, "wpurge: purging a PC");
	    return;
       }
    
    extract_char(ch,FALSE);
}


/* loads a mobile or object into the room */
WCMD(do_wload)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int number = 0;
    char_data *mob;
    obj_data *object;
  

    two_arguments(argument, arg1, arg2);
  
    if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) 
       {wld_log(room, "wload: bad syntax");
	    return;
       }

    if (is_abbrev(arg1, "mob")) 
       {if ((mob = read_mobile(number, VIRTUAL)) == NULL) 
           {wld_log(room, "wload: bad mob vnum");
	    return;
	   }
	char_to_room(mob, real_room(room->number));
        load_mtrigger(mob);
       }
    else 
    if (is_abbrev(arg1, "obj")) 
       {if ((object = read_object(number, VIRTUAL)) == NULL) 
           {wld_log(room, "wload: bad object vnum");
	    return;
	   }
	GET_OBJ_ZONE(object) = world[real_room(room->number)].zone;
	obj_to_room(object, real_room(room->number)); 
        load_otrigger(object);
       }
    else
       wld_log(room, "wload: bad type");
}

/* increases spells & skills */
const char *skill_name(int num);
const char *spell_name(int num);
int   find_skill_num(char *name);
int   find_spell_num(char *name);



WCMD(do_wdamage) 
{
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
    int  dam = 0;
    char_data *ch;

    two_arguments(argument, name, amount);

    if (!*name || !*amount || !isdigit(*amount)) 
       {wld_log(room, "wdamage: bad syntax");
	return;
       }

    dam = atoi(amount);

    if ((ch = get_char_by_room(room, name))) 
       {if (GET_LEVEL(ch) >= LVL_IMMORT && dam > 0) 
           {send_to_char("Будучи бессмертным, Вы избежали повреждения...", ch);
	    return;
	   }
        GET_HIT(ch) -= dam;
        if (dam < 0) 
           {send_to_char("Вам почувствовали себя лучше.\r\n", ch);
            return;
           }

	update_pos(ch);
	char_dam_message(dam,ch,ch,TYPE_UNDEFINED, 0); // prool
	if (GET_POS(ch) == POS_DEAD) 
	   {if (!IS_NPC(ch)) 
	       {sprintf(buf2, "%s killed by a trap at %s", GET_NAME(ch),
	      		world[ch->in_room].name);
      	        mudlog(buf2, BRF, 0, TRUE);
    	       }
    	    die(ch, NULL);
	   }
       }
    else
       wld_log(room, "wdamage: target not found");
}

WCMD(do_wat) 
{
    char location[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int vnum = 0;    
    room_data *r2;
 
    void wld_command_interpreter(room_data *room, char *argument);

    half_chop(argument, location, arg2);

    if (!*location || !*arg2 || !isdigit(*location)) 
       {wld_log(room, "wat: bad syntax");
	    return;
       }
    vnum = atoi(location);
    if (NOWHERE == real_room(vnum)) 
       {wld_log(room, "wat: location not found");
	    return;
       }

    r2 = &world[vnum];
    wld_command_interpreter(r2, arg2);
}

WCMD(do_wskillturn)
{
    char_data *ch;
    char name[MAX_INPUT_LENGTH], skillname[MAX_INPUT_LENGTH], 
         amount[MAX_INPUT_LENGTH], *pos;
    int  skillnum=0, skilldiff=0;
    
    one_argument (two_arguments(argument, name, skillname), amount);

    if (!*name || !*skillname || !*amount) 
       {wld_log(room, "wskillturn: too few arguments");
	    return;
       }
       
    if ((pos = strchr(skillname,'.')))
       *pos = ' ';
       
    if ((skillnum = find_skill_num(skillname)) < 0 ||
        skillnum == 0 || skillnum > MAX_SKILLS)
       {wld_log(room, "wskillturn: skill not found");
        return;
       }  
       
    if (!str_cmp(amount,"set"))
       skilldiff = 1;
    else
    if (!str_cmp(amount,"clear"))
       skilldiff = 0;
    else
       {wld_log(room, "wskillturn: unknown set variable");
        return;
       }    
    
    if ((ch = get_char_by_room(room, name))) 
       {trg_skillturn(ch,skillnum,skilldiff);
       }                                      
    else 
       {wld_log(room, "wskillturn: target not found");
 	    return;
       }
}


WCMD(do_wskilladd)
{
    char_data *ch;
    char name[MAX_INPUT_LENGTH], skillname[MAX_INPUT_LENGTH], 
         amount[MAX_INPUT_LENGTH], *pos;
    int  skillnum=0, skilldiff=0;
    
    one_argument (two_arguments(argument, name, skillname), amount);

    if (!*name || !*skillname || !*amount) 
       {wld_log(room, "wskilladd: too few arguments");
	    return;
       }
       
    if ((pos = strchr(skillname,'.')))
       *pos = ' ';
       
    if ((skillnum = find_skill_num(skillname)) < 0 ||
        skillnum == 0 || skillnum > MAX_SKILLS)
       {wld_log(room, "wskilladd: skill not found");
        return;
       }  
       
    skilldiff = atoi(amount);
    
    if ((ch = get_char_by_room(room, name))) 
       {trg_skilladd(ch,skillnum,skilldiff);
       }                                      
    else 
       {wld_log(room, "wskilladd: target not found");
 	    return;
       }
}




WCMD(do_wspellturn)
{   char_data *ch;
    char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], 
         amount[MAX_INPUT_LENGTH], *pos;
    int  spellnum=0, spelldiff=0;
    
    one_argument (two_arguments(argument, name, spellname), amount);

    if (!*name || !*spellname || !*amount) 
       {wld_log(room, "wspellturn: too few arguments");
	    return;
       }
       
    if ((pos = strchr(spellname,'.')))
       *pos = ' ';
       
    if ((spellnum = find_spell_num(spellname)) < 0 ||
        spellnum == 0 || spellnum > MAX_SPELLS)
       {wld_log(room, "wspellturn: spell not found");
        return;
       }  
       
    if (!str_cmp(amount, "set"))
       spelldiff = 1;
    else
    if (!str_cmp(amount, "clear"))
       spelldiff = 0;
    else
       {wld_log(room, "wspellturn: unknown set variable");
        return;
       }       
       
    if ((ch = get_char_by_room(room, name))) 
       {trg_spellturn(ch,spellnum,spelldiff);
       }                                      
    else 
       {wld_log(room, "wspellturn: target not found");
 	    return;
       }
}


WCMD(do_wspelladd)
{   char_data *ch;
    char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], 
         amount[MAX_INPUT_LENGTH], *pos;
    int  spellnum=0, spelldiff=0;
    
    one_argument (two_arguments(argument, name, spellname), amount);

    if (!*name || !*spellname || !*amount) 
       {wld_log(room, "wspelladd: too few arguments");
	    return;
       }
       
    if ((pos = strchr(spellname,'.')))
       *pos = ' ';
       
    if ((spellnum = find_spell_num(spellname)) < 0 ||
        spellnum == 0 || spellnum > MAX_SPELLS)
       {wld_log(room, "wspelladd: spell not found");
        return;
       }  
       
    spelldiff = atoi(amount);
    
    if ((ch = get_char_by_room(room, name))) 
       {trg_spelladd(ch,spellnum,spelldiff);
       }                                      
    else 
       {wld_log(room, "wspelladd: target not found");
 	    return;
       }
}

WCMD(do_wspellitem)
{   char_data *ch;
    char name[MAX_INPUT_LENGTH], 
         spellname[MAX_INPUT_LENGTH], 
         type[MAX_INPUT_LENGTH], 
         turn[MAX_INPUT_LENGTH],
         *pos;
    int  spellnum=0, spelldiff=0, spell=0;
    
    two_arguments(two_arguments(argument, name, spellname), type, turn);

    if (!*name || !*spellname || !*type || !*turn) 
       {wld_log(room, "wspellitem: too few arguments");
	    return;
       }
       
    if ((pos = strchr(spellname,'.')))
       *pos = ' ';
       
    if ((spellnum = find_spell_num(spellname)) < 0 ||
        spellnum == 0 || spellnum > MAX_SPELLS)
       {wld_log(room, "wspellitem: spell not found");
        return;
       }  
       
    if (!str_cmp(type,"potion"))
       spell=SPELL_POTION;
    else
    if (!str_cmp(type,"wand"))
       spell=SPELL_WAND;
    else      
    if (!str_cmp(type,"scroll"))
       spell=SPELL_SCROLL;
    else
    if (!str_cmp(type,"items"))
       spell=SPELL_ITEMS;
    else
    if (!str_cmp(type,"runes"))
       spell=SPELL_RUNES;
    else    
       {wld_log(room, "wspellitem: type spell not found");
        return;
       } 
       
    if (!str_cmp(turn,"set"))
       spelldiff = 1;
    else
    if (!str_cmp(turn,"clear"))
       spelldiff = 0;
    else
       {wld_log(room, "wspellitem: unknown set variable");
        return;
       }  
    
    if ((ch = get_char_by_room(room, name))) 
       {trg_spellitem(ch,spellnum,spelldiff,spell);
       }                                      
    else 
       {wld_log(room, "wspellitem: target not found");
 	    return;
       }
}

const struct wld_command_info wld_cmd_info[] = {
    { "RESERVED", 0, 0 },/* this must be first -- for specprocs */
    
    { "wasound"    , do_wasound   , 0 },
    { "wdoor"      , do_wdoor     , 0 },
    { "wecho"      , do_wecho     , 0 },
    { "wechoaround", do_wsend     , SCMD_WECHOAROUND },
    { "wexp"       , do_wexp      , 0 },
    { "wforce"     , do_wforce    , 0 },
    { "wload"      , do_wload     , 0 },
    { "wpurge"     , do_wpurge    , 0 },
    { "wsend"      , do_wsend     , SCMD_WSEND },
    { "wteleport"  , do_wteleport , 0 },
    { "wzoneecho"  , do_wzoneecho , 0 },
    { "wdamage"    , do_wdamage,    0 },
    { "wat"        , do_wat,        0 },
    { "wspellturn"     , do_wspellturn,     0 },
    { "wskillturn"     , do_wskillturn,     0 },
    { "wspelladd"      , do_wspelladd,      0 },
    { "wskilladd"      , do_wskilladd,      0 },
    { "wspellitem"     , do_wspellitem,     0 },
    { "\n", 0, 0 }	/* this must be last */
};


/*
 *  This is the command interpreter used by rooms, called by script_driver.
 */
void wld_command_interpreter(room_data *room, char *argument)
{
    int cmd, length;
    char *line, arg[MAX_INPUT_LENGTH];
  
    skip_spaces(&argument);
  
    /* just drop to next line for hitting CR */
    if (!*argument)
	   return;

    line = any_one_arg(argument, arg);


    /* find the command */
    for (length = strlen(arg), cmd = 0;
	     *wld_cmd_info[cmd].command != '\n'; cmd++)
	    if (!strncmp(wld_cmd_info[cmd].command, arg, length))
	       break;
  
    if (*wld_cmd_info[cmd].command == '\n') 
       {sprintf(buf2, "Unknown world cmd: '%s'", argument);
	    wld_log(room, buf2);
       } 
    else
	   ((*wld_cmd_info[cmd].command_pointer) 
	    (room, line, cmd, wld_cmd_info[cmd].subcmd));
}

