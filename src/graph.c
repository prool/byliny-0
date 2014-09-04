/* ************************************************************************
*   File: graph.c                                       Part of CircleMUD *
*  Usage: various graph algorithms                                        *
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
#include "interpreter.h"
#include "constants.h" 
#include "handler.h"
#include "db.h"
#include "spells.h"


/* Externals */
ACMD(do_say);
ACMD(do_sense);
extern struct char_data *character_list;
extern const char *dirs[];
extern const char *DirsTo[];
extern struct room_data *world;
extern int track_through_doors;
extern struct char_data *mob_proto;
extern struct player_index_element *player_table;
extern int top_of_p_table;
int    attack_best(struct char_data *ch, struct char_data *vict);
/* local functions */
void bfs_enqueue(room_rnum room, int dir);
void bfs_dequeue(void);
void bfs_clear_queue(void);
int  find_first_step(room_rnum src, room_rnum target, struct char_data *ch);
ACMD(do_track);
void hunt_victim(struct char_data * ch);

struct bfs_queue_struct {
  room_rnum room;
  char dir;
  struct bfs_queue_struct *next;
};

static struct bfs_queue_struct *queue_head = 0, *queue_tail = 0;
extern const  int Reverse[];

/* Utility macros */
#define MARK(room)	(SET_BIT(ROOM_FLAGS(room, ROOM_BFS_MARK), ROOM_BFS_MARK))
#define UNMARK(room)	(REMOVE_BIT(ROOM_FLAGS(room, ROOM_BFS_MARK), ROOM_BFS_MARK))
#define IS_MARKED(room)	(ROOM_FLAGGED(room, ROOM_BFS_MARK))
#define TOROOM(x, y)	(world[(x)].dir_option[(y)]->to_room)
#define IS_CLOSED(x, y)	(EXIT_FLAGGED(world[(x)].dir_option[(y)], EX_CLOSED))

int VALID_EDGE(room_rnum x, int y)
{
  if (world[x].dir_option[y] == NULL || TOROOM(x, y) == NOWHERE)
     return 0;
  if (track_through_doors == FALSE && IS_CLOSED(x, y))
     return 0;
  if (ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK) || IS_MARKED(TOROOM(x, y)))
     return 0;

  return 1;
}

void bfs_enqueue(room_rnum room, int dir)
{
  struct bfs_queue_struct *curr;

  CREATE(curr, struct bfs_queue_struct, 1);
  curr->room = room;
  curr->dir = dir;
  curr->next = 0;

  if (queue_tail) 
     {queue_tail->next = curr;
      queue_tail = curr;
     } 
  else
     queue_head = queue_tail = curr;
}


void bfs_dequeue(void)
{
  struct bfs_queue_struct *curr;

  curr = queue_head;

  if (!(queue_head = queue_head->next))
     queue_tail = 0;
  free(curr);
}


void bfs_clear_queue(void)
{
  while (queue_head)
        bfs_dequeue();
}


/* 
 * find_first_step: given a source room and a target room, find the first
 * step on the shortest path from the source to the target.
 *
 * Intended usage: in mobile_activity, give a mob a dir to go if they're
 * tracking another mob or a PC.  Or, a 'track' skill for PCs.
 */
int find_first_step(room_rnum src, room_rnum target, struct char_data *ch)
{
  int curr_dir;
  room_rnum curr_room;

  if (src < 0 || src > top_of_world || target < 0 || target > top_of_world) 
     {log("SYSERR: Illegal value %d or %d passed to find_first_step. (%s)", src, target, __FILE__);
      return (BFS_ERROR);
     }
  if (src == target)
     return (BFS_ALREADY_THERE);
     
  /* MOB track victim in other zone */  
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_STAY_ZONE) &&
      world[src].zone != world[target].zone)
     return (BFS_ERROR); 

  /* clear marks first, some OLC systems will save the mark. */
  for (curr_room = 0; curr_room <= top_of_world; curr_room++)
      UNMARK(curr_room);

  MARK(src);

  /* first, enqueue the first steps, saving which direction we're going. */
  for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
      if (VALID_EDGE(src, curr_dir)) 
         {MARK(TOROOM(src, curr_dir));
          bfs_enqueue(TOROOM(src, curr_dir), curr_dir);
         }

  /* now, do the classic BFS. */
  while (queue_head) 
        {if (queue_head->room == target) 
            {curr_dir = queue_head->dir;
             bfs_clear_queue();
             return (curr_dir);
            } 
         else 
            {for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
	             if (VALID_EDGE(queue_head->room, curr_dir)) 
	                {MARK(TOROOM(queue_head->room, curr_dir));
	                 bfs_enqueue(TOROOM(queue_head->room, curr_dir), queue_head->dir);
	                }
             bfs_dequeue();
            }
        }

  return (BFS_NO_PATH);
}


/********************************************************
* Functions and Commands which use the above functions. *
********************************************************/
int go_track(struct char_data *ch, struct char_data *victim, int skill_no)
{int percent, dir;

 if (AFF_FLAGGED(victim, AFF_NOTRACK)) 
    {return BFS_ERROR;
    }

  /* 101 is a complete failure, no matter what the proficiency. */
  percent = number(0,skill_info[skill_no].max_percent);
  
  if (percent > calculate_skill(ch, skill_no, skill_info[skill_no].max_percent, victim)) 
     {int tries = 10;
      /* Find a random direction. :) */
      do {dir = number(0, NUM_OF_DIRS - 1);
         } while (!CAN_GO(ch, dir) && --tries);
      return dir;   
     }

  /* They passed the skill check. */
  return find_first_step(ch->in_room, victim->in_room, ch);
}

ACMD(do_sense)
{
  struct char_data *vict;
  int    dir;

  /* The character must have the track skill. */
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SENSE)) 
     {send_to_char("Но Вы не знаете как.\r\n", ch);
      return;
     }
     
  if (!check_moves(ch, SENSE_MOVES))
     return;
     
  one_argument(argument, arg);
  
  if (!*arg) 
     {send_to_char("Кого Вы хотите найти ?\r\n", ch);
      return;
     }
  /* The person can't see the victim. */
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD))) 
     {send_to_char("Такого существа нет.\r\n", ch);
      return;
     }
     
  /* We can't track the victim. */
  if (AFF_FLAGGED(vict, AFF_NOTRACK)) 
     {send_to_char("Вы не чувствуете его присутствия.\r\n", ch);
      return;
     }

  dir = go_track(ch, vict, SKILL_SENSE);

  switch (dir) 
         {
  case BFS_ERROR:
    send_to_char("Хммм.. Ваше чувство подвело Вас.\r\n", ch);
    break;
  case BFS_ALREADY_THERE:
    send_to_char("Вы же с ним в одной комнате !!!\r\n", ch);
    break;
  case BFS_NO_PATH:
    sprintf(buf, "Ваши чувства молчат.\r\n");
    send_to_char(buf, ch);
    break;
  default:	/* Success! */
    improove_skill(ch,SKILL_SENSE,TRUE,vict);
    sprintf(buf, "Чувство подсказало Вам : \"Ступай %s.\"\r\n", DirsTo[dir]);
    send_to_char(buf, ch);
    break;
         }
}

const char *track_when[] =
{"совсем свежие",
 "свежие",
 "менее полудневной давности",
 "примерно полудневной давности",
 "почти дневной давности",
 "примерно дневной давности",
 "совсем старые"
}; 

#define CALC_TRACK(ch,vict) (calculate_skill(ch,SKILL_TRACK,skill_info[SKILL_TRACK].max_percent,0))

int age_track(struct char_data *ch, int time, int calc_track)
{int when = 0;

 if (calc_track >= number(1,50))
    {if (time & 0x03)   /* 2 */
        when = 0;
     else
     if (time & 0x1F)   /* 5 */
        when = 1;
     else
     if (time & 0x3FF)  /* 10 */
        when = 2;
     else
     if (time & 0x7FFF)  /* 15 */
        when = 3;
     else
     if (time & 0xFFFFF) /* 20 */
        when = 4;
     else
     if (time & 0x3FFFFFF) /* 26 */
        when = 5;
     else
        when = 6;   
    }
 else
    when = number(0,6);   
 return (when);
}    
 

ACMD(do_track)
{ struct char_data  *vict = NULL;
  struct track_data *track;
  int    found = FALSE, c, calc_track = 0,track_t,i;
  char   name[MAX_INPUT_LENGTH];

  /* The character must have the track skill. */
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TRACK)) 
     {send_to_char("Но Вы не знаете как.\r\n", ch);
      return;
     }
     
  if (!check_moves(ch, TRACK_MOVES))
     return;
  
  calc_track = CALC_TRACK(ch,NULL);
  act("Похоже, $n кого-то выслеживает.",FALSE,ch,0,0,TO_ROOM);
  one_argument(argument, arg);
  
  /* No argument - show all */
  if (!*arg) 
     {for (track = world[IN_ROOM(ch)].track; track; track = track->next)
          {*name = '\0';
           if (IS_SET(track->track_info, TRACK_NPC))
              strcpy(name,GET_NAME(mob_proto + track->who));
           else
              for (c = 0; c <= top_of_p_table; c++)
                  {if (player_table[c].id == track->who)
                      {strcpy(name,player_table[c].name);
                       break;
                      }
                  }    
                     
           if (*name && calc_track > number(1,40))
              {CAP(name);
               for (track_t = i = 0; i < NUM_OF_DIRS; i++)
                   {track_t |= track->time_outgone[i];
                    track_t |= track->time_income[i];
                   }
               sprintf(buf,"%s : следы %s.\r\n",name,track_when[age_track(ch,track_t,calc_track)]);
               send_to_char(buf,ch);
               found = TRUE;
              } 
          } 
      if (!found)
         send_to_char("Вы не видите ничьих следов.\r\n", ch);
      return;
     }
             
  if ((vict = get_char_vis(ch,arg,FIND_CHAR_ROOM)))
     {act("Вы же в одной комнате с $N4!",FALSE,ch,0,vict,TO_CHAR);
      return;
     }
  
  /* found victim */
  for (track = world[IN_ROOM(ch)].track; track; track = track->next)
      {*name = '\0';
       if (IS_SET(track->track_info, TRACK_NPC))
          strcpy(name,GET_NAME(mob_proto + track->who));
       else
          for (c = 0; c <= top_of_p_table; c++)
              if (player_table[c].id == track->who)
                 {strcpy(name,player_table[c].name);
                  break;
                 }
       if (*name && isname(arg,name))
          break;
       else
          *name = '\0';
      }
          
 if (calc_track < number(1,40) || !*name || ROOM_FLAGGED(IN_ROOM(ch),ROOM_NOTRACK))
    {send_to_char("Вы не видите похожих следов.\r\n", ch);
     return;
    }
     
 ch->track_dirs       = 0; 
 CAP(name);
 sprintf(buf,"%s:\r\n",name);
 
 for (c = 0; c < NUM_OF_DIRS; c++)
     {if ((track  && track->time_income[c] && calc_track >= number(0,skill_info[SKILL_TRACK].max_percent)) ||
          (!track && calc_track  < number(0,skill_info[SKILL_TRACK].max_percent))
         )
         {found = TRUE;
          sprintf(buf + strlen(buf), "- %s следы ведут %s\r\n", 
	          track_when[age_track(ch,track ? track->time_income[c] : (1 << number(0,25)),calc_track)],
		  DirsFrom[Reverse[c]]
		 );
         }
      if ((track  && track->time_outgone[c] && calc_track >= number(0,skill_info[SKILL_TRACK].max_percent)) ||
          (!track && calc_track  < number(0,skill_info[SKILL_TRACK].max_percent))
         )
         {found = TRUE;
          SET_BIT(ch->track_dirs, 1 << c);
          sprintf(buf + strlen(buf), "- %s следы ведут %s\r\n", 
	          track_when[age_track(ch,track ? track->time_outgone[c] : (1 << number(0,25)),calc_track)],
		  DirsTo[c]
		 );
         }
     }
 
 if (!found)
    {sprintf(buf,"След неожиданно оборвался.\r\n");
    }
 send_to_char(buf, ch);   
 return;    
}



void hunt_victim(struct char_data * ch)
{
  int dir;
  byte found;
  struct char_data *tmp;

  if (!ch || !HUNTING(ch) || FIGHTING(ch))
     return;

  /* make sure the char still exists */
  for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
      if (HUNTING(ch) == tmp && IN_ROOM(tmp) != NOWHERE)
         found = TRUE;

  if (!found) 
     {do_say(ch, "О, Боги!  Моя жертва ускользнула!!", 0, 0);
      HUNTING(ch) = NULL;
      return;
     }
     
  if ((dir = find_first_step(ch->in_room, HUNTING(ch)->in_room, ch)) < 0) 
     {sprintf(buf, "Какая жалость, %s сбежал%s!", 
                   HMHR(HUNTING(ch)), GET_CH_SUF_1(HUNTING(ch)));
      do_say(ch, buf, 0, 0);
      HUNTING(ch) = NULL;
     } 
  else 
     {perform_move(ch, dir, 1, FALSE);
      if (ch->in_room == HUNTING(ch)->in_room)
         attack_best(ch, HUNTING(ch));
     }
}

ACMD(do_hidetrack)
{struct track_data *track[NUM_OF_DIRS+1], *temp;
 int    percent, prob, i, croom, found = FALSE, dir, rdir;

 if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_HIDETRACK))
    {send_to_char("Но Вы не знаете как.\r\n", ch);
     return;
    }
    
 croom = IN_ROOM(ch);

 for (dir = 0; dir < NUM_OF_DIRS; dir++)
     {track[dir] = NULL;
      rdir       = Reverse[dir];
      if (EXITDATA(croom,dir)                         && 
          EXITDATA(EXITDATA(croom,dir)->to_room,rdir) &&
          EXITDATA(EXITDATA(croom,dir)->to_room,rdir)->to_room == croom)
         {for (temp = world[EXITDATA(croom,dir)->to_room].track; temp; temp = temp->next)
              if (!IS_SET(temp->track_info, TRACK_NPC) && 
                  GET_IDNUM(ch) == temp->who           &&
                  !IS_SET(temp->track_info, TRACK_HIDE)&&
                  IS_SET(temp->time_outgone[rdir], 3)
                 )
                 {found      = TRUE;
                  track[dir] = temp;
                  break;
                 }
         }
     }

 track[NUM_OF_DIRS] = NULL;
 for (temp = world[IN_ROOM(ch)].track; temp; temp = temp->next)
     if (!IS_SET(temp->track_info, TRACK_NPC) && 
         GET_IDNUM(ch) == temp->who           &&
         !IS_SET(temp->track_info, TRACK_HIDE)        
        )
        {found              = TRUE;
         track[NUM_OF_DIRS] = temp;
         break;
        }
 
 if (!found)
    {send_to_char("Вы не видите своих следов.\r\n",ch);
     return;
    }
 if (!check_moves(ch,HIDETRACK_MOVES))
    return;
 percent = number(1,skill_info[SKILL_HIDETRACK].max_percent);
 prob    = calculate_skill(ch,SKILL_HIDETRACK,skill_info[SKILL_HIDETRACK].max_percent,0);
 if (percent > prob)
    {send_to_char("Вы безуспешно попытались замести свои следы.\r\n",ch);
     if (!number(0,25-timed_by_skill(ch,SKILL_HIDETRACK) ? 0 : 15))
        improove_skill(ch,SKILL_HIDETRACK,FALSE,0);
    }
 else
    {send_to_char("Вы успешно замели свои следы.\r\n",ch);
     if (!number(0,25-timed_by_skill(ch,SKILL_HIDETRACK) ? 0 : 15))
        improove_skill(ch,SKILL_HIDETRACK,TRUE,0);
     prob -= percent;
     for (i = 0; i <= NUM_OF_DIRS; i++)
         if (track[i])
            {if (i < NUM_OF_DIRS)
               track[i]->time_outgone[Reverse[i]] <<= MIN(31,prob);
             else
                for (rdir = 0; rdir < NUM_OF_DIRS; rdir++)
                    {track[i]->time_income[rdir]  <<= MIN(31,prob);
                     track[i]->time_outgone[rdir] <<= MIN(31,prob);
                    }
	     //sprintf(buf,"Заметены следы %d\r\n",i);
	     //send_to_char(buf,ch);
            }
    }

 for (i = 0; i <= NUM_OF_DIRS; i++)
     if (track[i])
        SET_BIT(track[i]->track_info, TRACK_HIDE);
}