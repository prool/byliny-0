/**************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <unistd.h> // prool
#include "prool.h" // prool

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"

extern struct descriptor_data *descriptor_list;
extern struct time_data time_info;
extern struct room_data *world;
extern struct char_data *mob_proto;

/* local functions */
struct time_info_data *real_time_passed(time_t t2, time_t t1);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void die_follower(struct char_data * ch);
void add_follower(struct char_data * ch, struct char_data * leader);
void prune_crlf(char *txt);

/* external functions */
int   attack_best(struct char_data *ch, struct char_data *victim);
char *House_name(struct char_data *ch);
char *House_rank(struct char_data *ch);
char *House_sname(struct char_data *ch);

char AltToKoi[] = {
"АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдежзийклмноп░▒▓│┤╡╢╖╕╣║╗╝╜╛┐└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀рстуфхцчшщъыьэюяЁё≥≤⌠⌡÷≈°·∙√ ²■©"};
char KoiToAlt[] = {
"дЁз©юыц╢баеъэшщч╟╠╡ТЧЗШВСРЭУЬЩЫЖм╨уЯжи╦╥╩тсх╬╫╪фгл╣П╤╧яркопйьвнЪН═║Ф╓╔ДёЕ╗╘╙╚╛╜╝╞ОЮАБЦ╕╒ЛК╖ХМИГЙ·─│√└┘■┐∙┬┴┼▀▄█▌▐÷░▒▓⌠├┌°⌡┤≤²≥≈ "};
char WinToKoi[] = {
"©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©©АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдежзийклмнопрстуфхцчшщъыьэюя"};
char KoiToWin[] = {
"-|+++++++++.....................-|+.+++++++++++++++.+++++++++++.ЧЮАЖДЕТЦУХИЙКЛМНОЪПЯРСФБЭШГЬЩЫВЗчюаждетцухийклмноъпярсфбэшгьщывз"};
char KoiToWin2[] = {
"-|+++++++++.....................-|+.+++++++++++++++.+++++++++++.ЧЮАЖДЕТЦУХИЙКЛМНОzПЯРСФБЭШГЬЩЫВЗчюаждетцухийклмноъпярсфбэшгьщывз"};
char AltToLat[]  = {
"─│┌┐└┘├┤┬┴┼▀▄█▌▐░▒▓⌠■∙√≈≤≥ ⌡°²·÷═║╒ё╓╔╕╖╗╘╙╚╛╜╝╞╟╠╡Ё╢╣╤╥╦╧╨╩╪╫╬©0abcdefghijklmnopqrstY1v23z456780ABCDEFGHIJKLMNOPQRSTY1V23Z45678"};


/* creates a random number in interval [from;to] */
int number(int from, int to)
{
  /* error checking in case people call number() incorrectly */
  if (from > to)
     {int tmp = from;
      from = to;
      to = tmp;
      log("SYSERR: number() should be called with lowest, then highest. number(%d, %d), not number(%d, %d).", from, to, to, from);
     }

  return ((circle_random() % (to - from + 1)) + from);
}


/* simulates dice roll */
int dice(int number, int size)
{
  int sum = 0;

  if (size <= 0 || number <= 0)
     return (0);

  while (number-- > 0)
        sum += ((circle_random() % size) + 1);

  return (sum);
}


int MIN(int a, int b)
{ return (a < b ? a : b);
}


int MAX(int a, int b)
{ return (a > b ? a : b);
}


char *CAP(char *txt)
{ *txt = UPPER(*txt);
  return (txt);
}


/* Create a duplicate of a string */
char *str_dup(const char *source)
{ char *new_z;

  CREATE(new_z, char, strlen(source) + 1);
  return (strcpy(new_z, source));
}


/*
 * Strips \r\n from end of string.
 */
void prune_crlf(char *txt)
{
  int i = strlen(txt) - 1;

  while (txt[i] == '\n' || txt[i] == '\r')
        txt[i--] = '\0';
}


/*
 * str_cmp: a case-insensitive version of strcmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different or we reach the end of both.
 */
int str_cmp(const char *arg1, const char *arg2)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL)
     {log("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
      return (0);
     }

  for (i = 0; arg1[i] || arg2[i]; i++)
      if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
         return (chk);	/* not equal */

  return (0);
}


/*
 * strn_cmp: a case-insensitive version of strncmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different, the end of both, or n is reached.
 */
int strn_cmp(const char *arg1, const char *arg2, int n)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL)
     {log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
      return (0);
     }

  for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
      if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
         return (chk); /* not equal */

  return (0);
}

/* log a death trap hit */
void log_death_trap(struct char_data * ch)
{
  char buf[256];

  sprintf(buf, "%s hit death trap #%d (%s)", GET_NAME(ch),
   	      GET_ROOM_VNUM(IN_ROOM(ch)), world[ch->in_room].name);
  mudlog(buf, BRF, LVL_IMMORT, TRUE);
}

/*
 * New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.
 */
void basic_mud_log(const char *format, ...)
{
  va_list args;
  int i;
  char buffer[BUFLEN], buffer_o[BUFLEN*2];
#if 1 // prool: 1 - logging, 0 - no logging
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  for (i=0;i<BUFLEN;i++) {buffer[i]=0; buffer_o[2*i]=0; buffer_o[2*i+1]=0;}

  if (logfile == NULL)
    puts("SYSERR: Using log() before stream was initialized!");
  if (format == NULL)
    format = "SYSERR: log() received a NULL format.";

  time_s[strlen(time_s) - 1] = '\0';

  fprintf(logfile, "%-15.15s :: ", time_s + 4);

  va_start(args, format);
#if 0 // prool: 0 - normal output, 0 - UTF-8 output (for cygwin)
  vfprintf(logfile, format, args);
#else
  vsnprintf(buffer, BUFLEN, format, args);
  koi_to_utf8(buffer, buffer_o);
  fprintf(logfile, buffer_o);
#endif
  va_end(args);

  fprintf(logfile, "\n");
  fflush(logfile);
#endif
}


/* the "touch" command, essentially. */
int touch(const char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a")))
     {log("SYSERR: %s: %s", path, strerror(errno));
      return (-1);
     }
  else
     {fclose(fl);
      return (0);
     }
}


/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void mudlog(const char *str, int type, int level, int file)
{
  char buf[MAX_STRING_LENGTH], tp;
  struct descriptor_data *i;

  if (str == NULL)
     return;	/* eh, oh well. */
  if (file)
     log(str);
  if (level < 0)
     return;

  sprintf(buf, "[ %s ]\r\n", str);

  for (i = descriptor_list; i; i = i->next)
      {if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
          continue;
       if (GET_LEVEL(i->character) < level)
          continue;
       if (PLR_FLAGGED(i->character, PLR_WRITING))
          continue;
       tp = ((PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) +
	         (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0));
       if (tp < type)
          continue;

       send_to_char(CCGRN(i->character, C_NRM), i->character);
       send_to_char(buf, i->character);
       send_to_char(CCNRM(i->character, C_NRM), i->character);
      }
}



/*
 * If you don't have a 'const' array, just cast it as such.  It's safer
 * to cast a non-const array as const than to cast a const one as non-const.
 * Doesn't really matter since this function doesn't change the array though.
 */
char *empty_string = "ничего";
void sprintbit(bitvector_t bitvector, const char *names[], char *result)
{
  long nr=0, fail=0, divider=FALSE;

  *result = '\0';

  if ((unsigned long)bitvector < (unsigned long)INT_ONE)
      ;
  else
  if ((unsigned long)bitvector < (unsigned long)INT_TWO)
     fail = 1;
  else
  if ((unsigned long)bitvector < (unsigned long)INT_THREE)
     fail = 2;
  else
     fail = 3;
  bitvector &= 0x3FFFFFFF;
  while (fail)
        {if (*names[nr] == '\n')
            fail--;
         nr++;
        }


  for (;bitvector; bitvector >>= 1)
      {if (IS_SET(bitvector, 1))
          {if (*names[nr] != '\n')
              {strcat(result, names[nr]);
	           strcat(result, ",");
	           divider = TRUE;
              }
           else
	          {strcat(result, "UNDEF");
	           strcat(result, ",");
	           divider = TRUE;
	          }
          }
       if (*names[nr] != '\n')
          nr++;
      }

  if (!*result)
     strcat(result, empty_string);
  else
  if (divider)
     *(result + strlen(result) - 1) = '\0';
}

void sprintbits(struct new_flag flags, const char *names[], char *result, char *div)
{
 sprintbit(flags.flags[0],names,result);
 strcat(result,div);
 sprintbit(flags.flags[1]|INT_ONE,names,result+strlen(result));
 strcat(result,div);
 sprintbit(flags.flags[2]|INT_TWO,names,result+strlen(result));
 strcat(result,div);
 sprintbit(flags.flags[3]|INT_THREE,names,result+strlen(result));
}



void sprinttype(int type, const char *names[], char *result)
{
  int nr = 0;

  while (type && *names[nr] != '\n')
        {type--;
         nr++;
        }

  if (*names[nr] != '\n')
     strcpy(result, names[nr]);
  else
     strcpy(result, "UNDEF");
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data *real_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY);	/* 0..34 days  */
  /* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */

  now.month = -1;
  now.year = -1;

  return (&now);
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data *mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / (SECS_PER_MUD_HOUR * TIME_KOEFF)) % HOURS_PER_DAY;	/* 0..23 hours */
  secs     -= SECS_PER_MUD_HOUR * TIME_KOEFF * now.hours;

  now.day = (secs / (SECS_PER_MUD_DAY * TIME_KOEFF)) % DAYS_PER_MONTH; /* 0..29 days  */
  secs -= SECS_PER_MUD_DAY * TIME_KOEFF * now.day;

  now.month = (secs / (SECS_PER_MUD_MONTH * TIME_KOEFF)) % MONTHS_PER_YEAR; /* 0..11 months */
  secs -= SECS_PER_MUD_MONTH * TIME_KOEFF * now.month;

  now.year = (secs / (SECS_PER_MUD_YEAR * TIME_KOEFF)); /* 0..XX? years */

  return (&now);
}



struct time_info_data *age(struct char_data * ch)
{
  static struct time_info_data player_age;

  player_age = *mud_time_passed(time(0), ch->player.time.birth);

  player_age.year += 17;	/* All players start at 17 */

  return (&player_age);
}


/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data * ch, struct char_data * victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master)
      {if (k->master == k)
          {k->master = NULL;
           return (FALSE);
          }
       if (k == ch)
          return (TRUE);
      }

  return (FALSE);
}

void make_horse(struct char_data *horse, struct char_data *ch)
{ SET_BIT(AFF_FLAGS(horse,AFF_HORSE),AFF_HORSE);
  add_follower(horse,ch);
  REMOVE_BIT(MOB_FLAGS(horse,MOB_WIMPY),MOB_WIMPY);
  REMOVE_BIT(MOB_FLAGS(horse,MOB_SENTINEL),MOB_SENTINEL);
  REMOVE_BIT(MOB_FLAGS(horse,MOB_HELPER),MOB_HELPER);
  REMOVE_BIT(MOB_FLAGS(horse,MOB_AGGRESSIVE),MOB_AGGRESSIVE);
  REMOVE_BIT(MOB_FLAGS(horse,MOB_MOUNTING),MOB_MOUNTING);
  REMOVE_BIT(AFF_FLAGS(horse,AFF_TETHERED),AFF_TETHERED);
}

int  on_horse(struct char_data *ch)
{ return (AFF_FLAGGED(ch, AFF_HORSE) && has_horse(ch,TRUE));
}

int  has_horse(struct char_data *ch, int same_room)
{ struct follow_type *f;

  if (IS_NPC(ch))
     return (FALSE);

  for (f = ch->followers; f; f = f->next)
      {if (IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_HORSE) &&
           (!same_room || IN_ROOM(ch) == IN_ROOM(f->follower)))
          return (TRUE);
      }
  return (FALSE);
}

struct char_data *get_horse(struct char_data *ch)
{ struct follow_type *f;

  if (IS_NPC(ch))
     return (NULL);

  for (f = ch->followers; f; f = f->next)
      {if (IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_HORSE))
          return (f->follower);
      }
  return (NULL);
}

void horse_drop(struct char_data *ch)
{ if (ch->master)
     {act("$N сбросил$G Вас со своей спины.",FALSE,ch->master,0,ch,TO_CHAR);
      REMOVE_BIT(AFF_FLAGS(ch->master, AFF_HORSE), AFF_HORSE);
      WAIT_STATE(ch->master, 3*PULSE_VIOLENCE);
      if (GET_POS(ch->master) > POS_SITTING)
         GET_POS(ch->master) = POS_SITTING;
     }
}

void check_horse(struct char_data *ch)
{ if (!IS_NPC(ch) && !has_horse(ch, TRUE))
     REMOVE_BIT(AFF_FLAGS(ch, AFF_HORSE), AFF_HORSE);
}


/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
int  stop_follower(struct char_data * ch, int mode)
{ struct char_data *master;
  struct follow_type *j, *k;
  int    i;

  //log("[Stop follower] Start function(%s->%s)",ch ? GET_NAME(ch) : "none",
  //      ch->master ? GET_NAME(ch->master) : "none");

  if (!ch->master)
     {log("SYSERR: stop_follower(%s) without master",GET_NAME(ch));
      // core_dump();
      return (FALSE);
     }

  act("Вы прекратили следовать за $N4.", FALSE, ch, 0, ch->master, TO_CHAR);
  act("$n прекратил$g следовать за $N4.", TRUE, ch, 0, ch->master, TO_NOTVICT);

  //log("[Stop follower] Stop horse");
  if (get_horse(ch->master) == ch && on_horse(ch->master))
     horse_drop(ch);
  else
     act("$n прекратил$g следовать за Вами.", TRUE, ch, 0, ch->master, TO_VICT);

  //log("[Stop follower] Remove from followers list");
  if (!ch->master->followers)
     log("[Stop follower] SYSERR: Followers absent for %s (master %s).",GET_NAME(ch),GET_NAME(ch->master));
  else
  if (ch->master->followers->follower == ch)
     {/* Head of follower-list? */
      k                     = ch->master->followers;
      if (!(ch->master->followers = k->next) &&
          !ch->master->master
	 )
         REMOVE_BIT(AFF_FLAGS(ch->master,AFF_GROUP),AFF_GROUP);
      free(k);
     }
  else
     {/* locate follower who is not head of list */
      for (k = ch->master->followers; k->next && k->next->follower != ch; k = k->next);
      if (!k->next)
         log("[Stop follower] SYSERR: Undefined %s in %s followers list.",GET_NAME(ch), GET_NAME(ch->master));
      else
         {j       = k->next;
          k->next = j->next;
          free(j);
         }
     }
  master     = ch->master;
  ch->master = NULL;

  REMOVE_BIT(AFF_FLAGS(ch, AFF_GROUP), AFF_GROUP);
  if (IS_NPC(ch) && AFF_FLAGGED(ch,AFF_HORSE))
     REMOVE_BIT(AFF_FLAGS(ch, AFF_HORSE), AFF_HORSE);

  //log("[Stop follower] Free charmee");
  if (AFF_FLAGGED(ch, AFF_CHARM)  ||
      AFF_FLAGGED(ch, AFF_HELPER) ||
      IS_SET(mode, SF_CHARMLOST)
     )
     {if (affected_by_spell(ch,SPELL_CHARM))
         affect_from_char(ch,SPELL_CHARM);
      EXTRACT_TIMER(ch) = 5;	
      REMOVE_BIT(AFF_FLAGS(ch, AFF_CHARM), AFF_CHARM);
      // log("[Stop follower] Stop fight charmee");
      if (FIGHTING(ch))
         stop_fighting(ch,TRUE);
      /*
      log("[Stop follower] Stop fight charmee opponee");
      for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next)
          {if (FIGHTING(vict) &&
               FIGHTING(vict) == ch &&
               FIGHTING(ch) != vict)
              stop_fighting(vict);
           }
       */
      //log("[Stop follower] Charmee MOB reaction");
      if (IS_NPC(ch))
         {if (MOB_FLAGGED(ch, MOB_CORPSE))
             {act("Налетевший ветер развеял $n3, не оставив и следа.",TRUE,ch,0,0,TO_ROOM);
              GET_LASTROOM(ch) = IN_ROOM(ch);
              extract_char(ch,FALSE);
              return (TRUE);
             }
          else
          if (AFF_FLAGGED(ch, AFF_HELPER))
             REMOVE_BIT(AFF_FLAGS(ch, AFF_HELPER), AFF_HELPER);
          else
             {if (master &&
                  !IS_SET(mode, SF_MASTERDIE) &&
                  IN_ROOM(ch) == IN_ROOM(master) &&
                  CAN_SEE(ch,master) &&
                  !FIGHTING(ch))
                 {if (number(1,GET_REAL_INT(ch) * 2) > GET_REAL_CHA(master))
                     {act("$n посчитал$g, что Вы заслуживаете смерти !",FALSE,ch,0,master,TO_VICT);
                      act("$n заорал$g : \"Ты долго водил$G меня за нос, но дальше так не пойдет !\""
                          "              \"Теперь только твоя смерть может искупить твой обман !!!\"",
                          TRUE, ch, 0, master, TO_NOTVICT);
                      set_fighting(ch,master);
                     }
                 }
              else
              if (master &&
                  !IS_SET(mode, SF_MASTERDIE) &&
                  CAN_SEE(ch,master) &&
                  MOB_FLAGGED(ch, MOB_MEMORY))
                 remember(ch,master);
             }
         }
     }
  //log("[Stop follower] Restore mob flags");
  if (IS_NPC(ch) && (i = GET_MOB_RNUM(ch)) >= 0)
     {MOB_FLAGS(ch,INT_ZERRO) = MOB_FLAGS(mob_proto+i,INT_ZERRO);
      MOB_FLAGS(ch,INT_ONE)   = MOB_FLAGS(mob_proto+i,INT_ONE);
      MOB_FLAGS(ch,INT_TWO)   = MOB_FLAGS(mob_proto+i,INT_TWO);
      MOB_FLAGS(ch,INT_THREE) = MOB_FLAGS(mob_proto+i,INT_THREE);
     }
   //log("[Stop follower] Stop function");
   return (FALSE);
}



/* Called when a character that follows/is followed dies */
void die_follower(struct char_data * ch)
{ struct follow_type *j, *k = ch->followers;

  if (ch->master)
     stop_follower(ch, SF_FOLLOWERDIE);

  if (on_horse(ch))
     REMOVE_BIT(AFF_FLAGS(ch,AFF_HORSE), AFF_HORSE);

  for (k = ch->followers; k; k = j)
      {j = k->next;
       stop_follower(k->follower, SF_MASTERDIE);
      }
}



/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data * ch, struct char_data * leader)
{
  struct follow_type *k;

  if (ch->master)
     {log("SYSERR: add_follower(%s->%s) when master existing(%s)...",
          GET_NAME(ch),leader ? GET_NAME(leader) : "",GET_NAME(ch->master));
      // core_dump();
      return;
     }

  if (ch == leader)
     return;

  ch->master = leader;

  CREATE(k, struct follow_type, 1);

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  if (!IS_HORSE(ch))
     {act("Вы начали следовать за $N4.", FALSE, ch, 0, leader, TO_CHAR);
      //if (CAN_SEE(leader, ch))
      act("$n начал$g следовать за Вами.", TRUE, ch, 0, leader, TO_VICT);
      act("$n начал$g следовать за $N4.", TRUE, ch, 0, leader, TO_NOTVICT);
     }
}

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE * fl, char *buf)
{
  char temp[256];
  int lines = 0;

  do {fgets(temp, 256, fl);
      if (feof(fl))
         return (0);
      lines++;
     } while (*temp == '*' || *temp == '\n');

  temp[strlen(temp) - 1] = '\0';
  strcpy(buf, temp);
  return (lines);
}


int get_filename(char *orig_name, char *filename, int mode)
{
  const char *prefix, *middle, *suffix;
  char  name[64], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL)
     {log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
	 	  orig_name, filename);
      return (0);
     }

  switch (mode)
  {
  case CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case TEXT_CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = TEXT_SUF_OBJS;
    break;
  case TIME_CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = TIME_SUF_OBJS;
    break;
  case ALIAS_FILE:
    prefix = LIB_PLRALIAS;
    suffix = SUF_ALIAS;
    break;
  case PLAYERS_FILE:
    prefix = LIB_PLRS;
    suffix = SUF_PLAYER;
    break;
  case PKILLERS_FILE:
    prefix = LIB_PLRS;
    suffix = SUF_PKILLER;
    break;
  case PQUESTS_FILE:
    prefix = LIB_PLRS;
    suffix = SUF_QUESTS;
    break;
  case PMKILL_FILE:
    prefix = LIB_PLRS;
    suffix = SUF_PMKILL;
    break;
  case ETEXT_FILE:
    prefix = LIB_PLRTEXT;
    suffix = SUF_TEXT;
    break;
  case SCRIPT_VARS_FILE:
    prefix = LIB_PLRVARS;
    suffix = SUF_MEM;
    break;
  default:
    return (0);
  }

  strcpy(name, orig_name);
  for (ptr = name; *ptr; ptr++)
      *ptr = LOWER(AtoL(*ptr));

  switch (LOWER(*name))
  {
  case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
    middle = LIB_A;
    break;
  case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
    middle = LIB_F;
    break;
  case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
    middle = LIB_K;
    break;
  case 'p':  case 'q':  case 'r':  case 's':  case 't':
    middle = LIB_P;
    break;
  case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
    middle = LIB_U;
    break;
  default:
    middle = LIB_Z;
    break;
  }

  sprintf(filename, "%s%s"SLASH"%s.%s", prefix, middle, name, suffix);
  return (1);
}


int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;

  for (ch = room->people; ch != NULL; ch = ch->next_in_room)
      if (!IS_NPC(ch))
         i++;

  return (i);
}

/*
 * This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * You still want to call abort() or exit(1) for
 * non-recoverable errors, of course...
 *
 * XXX: Wonder if flushing streams includes sockets?
 */
void core_dump_real(const char *who, int line)
{
  log("SYSERR: Assertion failed at %s:%d!", who, line);

#if defined(CIRCLE_UNIX)
  /* These would be duplicated otherwise... */
  fflush(stdout);
  fflush(stderr);
  fflush(logfile);

  /*
   * Kill the child so the debugger or script doesn't think the MUD
   * crashed.  The 'autorun' script would otherwise run it again.
   */
  if (fork() == 0)
     abort();
#endif
}

void to_koi(char *str, int from)
{switch(from)
 {case KT_ALT : for(;*str;*str=AtoK(*str),str++);
           break;
  case KT_WINZ:
  case KT_WIN : for(;*str;*str=WtoK(*str),str++);
           break;
 }
}

void from_koi(char *str, int from)
{switch(from)
 {case KT_ALT : for(;*str;*str=KtoA(*str),str++);
           break;
  case KT_WIN : for(;*str;*str=KtoW(*str),str++);
  case KT_WINZ: for(;*str;*str=KtoW2(*str),str++);
           break;
 }
}

void koi_to_win(char *str, int size)
{  for (;size > 0; *str=KtoW(*str), size--, str++);
}

void koi_to_winz(char *str, int size)
{  for (;size > 0; *str=KtoW2(*str), size--, str++);
}

void koi_to_alt(char *str, int size)
{  for (;size > 0; *str=KtoA(*str), size--, str++);
}

/* string manipulation fucntion originally by Darren Wilson */
/* (wilson@shark.cc.cc.ca.us) improved and bug fixed by Chris (zero@cnw.com) */
/* completely re-written again by M. Scott 10/15/96 (scottm@workcommn.net), */
/* substitute appearances of 'pattern' with 'replacement' in string */
/* and return the # of replacements */
int replace_str(char **string, char *pattern, char *replacement, int rep_all,
		int max_size)
{  char *replace_buffer = NULL;
   char *flow, *jetsam, temp;
   int len, i;

   if ((strlen(*string) - strlen(pattern)) + strlen(replacement) > max_size)
      return -1;

   CREATE(replace_buffer, char, max_size);
   i = 0;
   jetsam = *string;
   flow = *string;
   *replace_buffer = '\0';
   if (rep_all)
      {while ((flow = (char *)strstr(flow, pattern)) != NULL)
             {i++;
	          temp = *flow;
	          *flow = '\0';
	          if ((strlen(replace_buffer) + strlen(jetsam) +
	               strlen(replacement)) > max_size)
                 {i = -1;
	              break;
	             }
	          strcat(replace_buffer, jetsam);
	          strcat(replace_buffer, replacement);
	          *flow = temp;
	          flow += strlen(pattern);
	          jetsam = flow;
             }
       strcat(replace_buffer, jetsam);
      }
   else
      {if ((flow = (char *)strstr(*string, pattern)) != NULL)
          {i++;
	       flow += strlen(pattern);
	       len = ((char *)flow - (char *)*string) - strlen(pattern);

	       strncpy(replace_buffer, *string, len);
	       strcat(replace_buffer, replacement);
	       strcat(replace_buffer, flow);
          }
      }
   if (i == 0) return 0;
   if (i > 0)
      {RECREATE(*string, char, strlen(replace_buffer) + 3);
       strcpy(*string, replace_buffer);
      }
   free(replace_buffer);
   return i;
}


/* re-formats message type formatted char * */
/* (for strings edited with d->str) (mostly olc and mail)     */
void format_text(char **ptr_string, int mode, struct descriptor_data *d, int maxlen)
{  int total_chars, cap_next = TRUE, cap_next_next = FALSE;
   char *flow, *start = NULL, temp;
   /* warning: do not edit messages with max_str's of over this value */
   char formated[MAX_STRING_LENGTH];

   flow   = *ptr_string;
   if (!flow) return;

   if (IS_SET(mode, FORMAT_INDENT))
      {strcpy(formated, "   ");
       total_chars = 3;
      }
   else
      {*formated = '\0';
       total_chars = 0;
      }

   while (*flow != '\0')
         {while ((*flow == '\n') ||
	             (*flow == '\r') ||
	             (*flow == '\f') ||
	             (*flow == '\t') ||
	             (*flow == '\v') ||
	             (*flow == ' '))
	             flow++;

          if (*flow != '\0')
             {start = flow++;
	          while ((*flow != '\0') &&
		             (*flow != '\n') &&
		             (*flow != '\r') &&
		             (*flow != '\f') &&
		             (*flow != '\t') &&
		             (*flow != '\v') &&
		             (*flow != ' ') &&
		             (*flow != '.') &&
		             (*flow != '?') &&
		             (*flow != '!'))
		            flow++;

	          if (cap_next_next)
	             {cap_next_next = FALSE;
	              cap_next = TRUE;
	             }

	 /* this is so that if we stopped on a sentance .. we move off the sentance delim. */
	          while ((*flow == '.') ||
	                 (*flow == '!') ||
	                 (*flow == '?'))
	                {cap_next_next = TRUE;
	                 flow++;
	                }
	
	          temp = *flow;
	          *flow = '\0';

	          if ((total_chars + strlen(start) + 1) > 79)
	             {strcat(formated, "\r\n");
	              total_chars = 0;
	             }

	          if (!cap_next)
	             {if (total_chars > 0)
	                 {strcat(formated, " ");
	                  total_chars++;
	                 }
	             }
	          else
	             {cap_next = FALSE;
	              *start = UPPER(*start);
	             }

	          total_chars += strlen(start);
	          strcat(formated, start);

	          *flow = temp;
             }

          if (cap_next_next)
             {if ((total_chars + 3) > 79)
                 {strcat(formated, "\r\n");
	              total_chars = 0;
	             }
	          else
	             {strcat(formated, "  ");
	              total_chars += 2;
	             }
             }
         }
   strcat(formated, "\r\n");

   if (strlen(formated) > maxlen)
      formated[maxlen] = '\0';
   RECREATE(*ptr_string, char, MIN(maxlen, strlen(formated)+3));
   strcpy(*ptr_string, formated);
}


char *some_pads[3][17] =
{ {"дней","часов","лет","очков","минут","минут","кун","кун",
   "штук","штук","уровней","верст","верст","единиц","единиц","секунд","градусов"},
  {"день","час","год","очко","минута","минуту","куна","куну",
   "штука","штуку","уровень","верста","версту","единица","единицу","секунду","градус"},
  {"дня","часа","года","очка","минуты","минуты","куны","куны",
   "штуки","штуки","уровня","версты","версты","единицы","единицы","секунды","градуса"}
};

char * desc_count (int how_many, int of_what)
{if (how_many < 0)
    how_many = -how_many;
 if ( (how_many % 100 >= 11 && how_many % 100 <= 14) ||
       how_many % 10 >= 5 ||
       how_many % 10 == 0)
    return some_pads[0][of_what];
 if ( how_many % 10 == 1)
    return some_pads[1][of_what];
 else
    return some_pads[2][of_what];
}

int check_moves(struct char_data *ch, int how_moves)
{
 if (IS_IMMORTAL(ch) || IS_NPC(ch))
    return (TRUE);
 if (GET_MOVE(ch) < how_moves)
    {send_to_char("Вы слишком устали.\r\n", ch);
     return (FALSE);
    }
 GET_MOVE(ch) -= how_moves;
 return (TRUE);
}

int real_sector(int room)
{ int sector = SECT(room);

  if (ROOM_FLAGGED(room,ROOM_NOWEATHER))
     return sector;
  switch (sector)
  {case SECT_INSIDE:
   case SECT_CITY:
   case SECT_FLYING:
   case SECT_UNDERWATER:
   case SECT_SECRET:
        return sector;
        break;
   case SECT_FIELD:
        if (world[room].weather.snowlevel > 20)
           return SECT_FIELD_SNOW;
        else
        if (world[room].weather.rainlevel > 20)
           return SECT_FIELD_RAIN;
        else
           return SECT_FIELD;
        break;
   case SECT_FOREST:
        if (world[room].weather.snowlevel > 20)
           return SECT_FOREST_SNOW;
        else
        if (world[room].weather.rainlevel > 20)
           return SECT_FOREST_RAIN;
        else
           return SECT_FOREST;
        break;
   case SECT_HILLS:
        if (world[room].weather.snowlevel > 20)
           return SECT_HILLS_SNOW;
        else
        if (world[room].weather.rainlevel > 20)
           return SECT_HILLS_RAIN;
        else
           return SECT_HILLS;
        break;
   case SECT_MOUNTAIN:
        if (world[room].weather.snowlevel > 20)
           return SECT_MOUNTAIN_SNOW;
        else
           return SECT_MOUNTAIN;
        break;
   case SECT_WATER_SWIM:
   case SECT_WATER_NOSWIM:
        if (world[room].weather.icelevel > 30)
           return SECT_THICK_ICE;
        else
        if (world[room].weather.icelevel > 20)
           return SECT_NORMAL_ICE;
        else
        if (world[room].weather.icelevel > 10)
           return SECT_THIN_ICE;
        else
           return sector;
        break;
  }
  return SECT_INSIDE;
}


void message_auction(char *message, struct char_data *ch)
{ struct descriptor_data *i;

  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next)
      {if (STATE(i) == CON_PLAYING &&
           (!ch || i != ch->desc) &&
           i->character &&
	   !PRF_FLAGGED(i->character, PRF_NOAUCT) &&
	   !PLR_FLAGGED(i->character, PLR_WRITING) &&
	   !ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF) &&
	   GET_POS(i->character) > POS_SLEEPING)
	  {if (COLOR_LEV(i->character) >= C_NRM)
	      send_to_char(CCIYEL(i->character,C_NRM), i->character);
           act(message, FALSE, i->character, 0, 0, TO_CHAR | TO_SLEEP);
           if (COLOR_LEV(i->character) >= C_NRM)
	      send_to_char(CCNRM(i->character,C_NRM), i->character);
          }
      }
}

struct auction_lot_type auction_lots[MAX_AUCTION_LOT] =
{ {-1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0},
  {-1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0},
  {-1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0} /*,
  {-1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0},
  {-1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0}  */
};

void clear_auction(int lot)
{ if (lot < 0 || lot >= MAX_AUCTION_LOT)
     return;
  GET_LOT(lot)->seller        = GET_LOT(lot)->buyer       = GET_LOT(lot)->prefect = NULL;
  GET_LOT(lot)->seller_unique = GET_LOT(lot)->buyer_unique= GET_LOT(lot)->prefect_unique = -1;
  GET_LOT(lot)->item      = NULL;
  GET_LOT(lot)->item_id   = -1;
}

void sell_auction(int lot)
{ struct char_data *ch, *tch;
  struct obj_data  *obj;

  if (lot < 0 || lot >= MAX_AUCTION_LOT ||
      !(ch = GET_LOT(lot)->seller) || GET_UNIQUE(ch) != GET_LOT(lot)->seller_unique ||
      !(tch= GET_LOT(lot)->buyer)  || GET_UNIQUE(tch)!= GET_LOT(lot)->buyer_unique  ||
      !(obj= GET_LOT(lot)->item)   || GET_ID(obj)    != GET_LOT(lot)->item_id)
     return;

   if (obj->carried_by != ch)
      {sprintf(buf,"Аукцион : лот %d(%s) снят, ввиду смены владельца", lot,
               obj->PNames[0]);
       message_auction(buf,NULL);
       clear_auction(lot);
       return;
      }

    if (obj->contains)
       {sprintf(buf,"Опустошите %s перед продажей.\r\n",
                obj->PNames[3]);
        send_to_char(buf,ch);
        if (GET_LOT(lot)->tact >= MAX_AUCTION_TACT_PRESENT)
           {sprintf(buf,"Аукцион : лот %d(%s) снят с аукциона распорядителем торгов.", lot,
                    obj->PNames[0]);
            message_auction(buf,NULL);
            clear_auction(lot);
            return;
           }
       }

    if (GET_GOLD(tch) + GET_BANK_GOLD(tch) < GET_LOT(lot)->cost)
       {sprintf(buf,"У Вас не хватает денег на покупку %s.\r\n",
                obj->PNames[1]);
        send_to_char(buf,tch);
        sprintf(buf,"У покупателя %s не хватает денег.\r\n",
                obj->PNames[1]);
        send_to_char(buf,ch);
        sprintf(buf,"Аукцион : лот %d(%s) снят с аукциона распорядителем торгов.", lot,
                    obj->PNames[0]);
        message_auction(buf,NULL);
        clear_auction(lot);
        return;
       }

    if (IN_ROOM(ch) != IN_ROOM(tch) || !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
       {if (GET_LOT(lot)->tact >= MAX_AUCTION_TACT_PRESENT)
           {sprintf(buf,"Аукцион : лот %d(%s) снят с аукциона распорядителем торгов.", lot,
                    obj->PNames[0]);
            message_auction(buf,NULL);
            clear_auction(lot);
            return;
           }
        sprintf(buf,"Вам необходимо прибыть в комнату аукциона к $n2 для получения %s.",
                GET_LOT(lot)->item->PNames[1]);
        act(buf,FALSE,GET_LOT(lot)->seller,0,GET_LOT(lot)->buyer,TO_VICT|TO_SLEEP);
        sprintf(buf,"Вам необходимо прибыть в комнату аукциона к $N2 для получения денег за %s.",
                GET_LOT(lot)->item->PNames[3]);
        act(buf,FALSE,GET_LOT(lot)->seller,0,GET_LOT(lot)->buyer,TO_CHAR|TO_SLEEP);
        GET_LOT(lot)->tact = MAX(GET_LOT(lot)->tact,MAX_AUCTION_TACT_BUY);
        return;
       }

    sprintf(buf,"Вы продали %s с аукциона.\r\n",obj->PNames[3]);
    send_to_char(buf,ch);
    sprintf(buf,"Вы купили %s на аукционе.\r\n",obj->PNames[3]);
    send_to_char(buf,tch);
    obj_from_char(obj);
    obj_to_char(obj,tch);
    GET_BANK_GOLD(ch) += GET_LOT(lot)->cost;
    if ((GET_BANK_GOLD(tch)-= GET_LOT(lot)->cost) < 0)
       {GET_GOLD(tch)      += GET_BANK_GOLD(tch);
        GET_BANK_GOLD(tch)  = 0;
       }
    clear_auction(lot);
    return;
}

void check_auction(struct char_data *ch, struct obj_data *obj)
{int i;
 if (ch)
    {for (i = 0; i < MAX_AUCTION_LOT; i++)
         {if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
               continue;
          if (GET_LOT(i)->seller == ch || GET_LOT(i)->seller_unique == GET_UNIQUE(ch) ||
              GET_LOT(i)->buyer  == ch || GET_LOT(i)->buyer_unique  == GET_UNIQUE(ch) ||
              GET_LOT(i)->prefect== ch || GET_LOT(i)->prefect_unique== GET_UNIQUE(ch))
             {sprintf(buf,"Аукцион : лот %d(%s) снят с аукциона распорядителем.", i,
                      GET_LOT(i)->item->PNames[0]);
              message_auction(buf,ch);
              clear_auction(i);
             }
          }
    }
 else
 if (obj)
     {for (i = 0; i < MAX_AUCTION_LOT; i++)
          {if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
               continue;
           if (GET_LOT(i)->item == obj || GET_LOT(i)->item_id == GET_ID(obj))
              {sprintf(buf,"Аукцион : лот %d(%s) снят с аукциона распорядителем.", i,
                       GET_LOT(i)->item->PNames[0]);
               message_auction(buf,obj->carried_by);
               clear_auction(i);
              }
           }
      }
   else
      {for (i = 0; i < MAX_AUCTION_LOT; i++)
           {if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
               continue;
            if (GET_LOT(i)->item->carried_by != GET_LOT(i)->seller ||
                (GET_LOT(i)->buyer &&
                 (GET_GOLD(GET_LOT(i)->buyer) + GET_BANK_GOLD(GET_LOT(i)->buyer) <
                  GET_LOT(i)-> cost)))
               {sprintf(buf,"Аукцион : лот %d(%s) снят с аукциона распорядителем.", i,
                        GET_LOT(i)->item->PNames[0]);
                message_auction(buf,NULL);
                clear_auction(i);
               }
            }
      }
}

const char *tact_message[] =
{ "РАЗ !",
  "ДВА !!",
  "Т-Р-Р-Р-И !!!",
  "ЧЕТЫРЕ !!!!",
  "ПЯТЬ !!!!!",
  "\n"
};

void tact_auction(void)
{int i;

 check_auction(NULL,NULL);

 for (i = 0; i < MAX_AUCTION_LOT; i++)
     {if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
         continue;
      if (++GET_LOT(i)->tact < MAX_AUCTION_TACT_BUY)
         {sprintf(buf,"Аукцион : лот %d(%s), %d %s, %s", i, GET_LOT(i)->item->PNames[0],
                  GET_LOT(i)->cost, desc_count(GET_LOT(i)->cost, WHAT_MONEYa),
                  tact_message[GET_LOT(i)->tact]);
          message_auction(buf,NULL);
          continue;
         }
      else
      if (GET_LOT(i)->tact < MAX_AUCTION_TACT_PRESENT)
         {if (!GET_LOT(i)->buyer)
             {sprintf(buf,"Аукцион : лот %d(%s) снят распорядителем ввиду отсутствия спроса.",
                      i, GET_LOT(i)->item->PNames[0]);
              message_auction(buf,NULL);
              clear_auction(i);
              continue;
             }
          if (!GET_LOT(i)->prefect)
             {sprintf(buf,"Аукцион : лот %d(%s), %d %s, %s, ПРОДАНО.", i, GET_LOT(i)->item->PNames[0],
                      GET_LOT(i)->cost, desc_count(GET_LOT(i)->cost, WHAT_MONEYa),
                      tact_message[GET_LOT(i)->tact]);
              message_auction(buf,NULL);
              GET_LOT(i)->prefect        = GET_LOT(i)->buyer;
              GET_LOT(i)->prefect_unique = GET_LOT(i)->buyer_unique;
             }
          sell_auction(i);
         }
      else
         sell_auction(i);
     }
}

struct auction_lot_type *free_auction(int *lotnum)
{int i;
 for (i = 0; i < MAX_AUCTION_LOT; i++)
     {if (!GET_LOT(i)->seller && !GET_LOT(i)->item)
         {*lotnum = i;
          return (GET_LOT(i));
         }
     }

 return (NULL);
}

int obj_on_auction(struct obj_data *obj)
{int i;
 for (i = 0; i < MAX_AUCTION_LOT; i++)
     {if (GET_LOT(i)->item == obj && GET_LOT(i)->item_id == GET_ID(obj))
         return (TRUE);
     }

 return (FALSE);
}

char *only_title(struct char_data *ch)
{static char title[MAX_STRING_LENGTH], *hname, *hrank;
 static char clan[MAX_STRING_LENGTH];
 char   *pos;

 if ((hname = House_name(ch)) &&
     (hrank = House_rank(ch)) &&
     GET_HOUSE_RANK(ch)
    )
    {sprintf (clan, " (%s %s)",hrank,House_sname(ch));
    }
 else
    {clan[0] = 0;
    }

 if (!GET_TITLE(ch) || !*GET_TITLE(ch))
    {sprintf(title,"%s%s",GET_NAME(ch),clan);
    }
 else
 if ((pos = strchr(GET_TITLE(ch),';')))
    {*(pos++) = '\0';
     sprintf(title,"%s %s%s%s%s",pos, GET_NAME(ch),
             *GET_TITLE(ch) ? ", " : " ", GET_TITLE(ch),clan);
     *(--pos) = ';';
    }
 else
    sprintf(title,"%s, %s%s",GET_NAME(ch),GET_TITLE(ch),clan);
 return title;
}

char *race_or_title(struct char_data *ch)
{static char title[MAX_STRING_LENGTH], *hname, *hrank;
 static char clan[MAX_STRING_LENGTH];
 if ((hname = House_name(ch)) &&
     (hrank = House_rank(ch)) &&
     GET_HOUSE_RANK(ch)) {
    sprintf (clan, " (%s %s)",hrank,House_sname(ch));
 } else {
    clan[0] = 0;
 }
 if (!GET_TITLE(ch) || !*GET_TITLE(ch))
    sprintf (title,"%s %s%s",
             race_name[(int) GET_RACE(ch)][(int) GET_SEX(ch)], GET_NAME(ch), clan);
 else
    strcpy(title,only_title(ch));
 return title;
}


// не стоит убирать эту функцию - иначе и при приходе в локации и т.д. будет
// выводится с подствекой - а сие нафиг не надо...
char *race_or_title_enl(struct char_data *ch)
{static char title[MAX_STRING_LENGTH], *hname, *hrank;
 static char clan[MAX_STRING_LENGTH];
 char        name_who[MAX_STRING_LENGTH]="\0";
 char	     buftmp[MAX_STRING_LENGTH]="\0";
 char 	     *pos;
 struct PK_Memory_type *pk;
 int    count_pk=0;
 if ((hname = House_name(ch)) &&
     (hrank = House_rank(ch)) &&
     GET_HOUSE_RANK(ch)) {
    sprintf (clan, " (%s %s)",hrank,House_sname(ch));
 } else {
    clan[0] = 0;
 }
 	    *name_who='\0';
	    count_pk=0;
	    for (pk=ch->pk_list;pk;pk=pk->next)
	    {
		count_pk=count_pk+pk->pkills;
	    }
	    if (count_pk>=FifthPK) sprintf(name_who,"%s",CCIRED(ch,C_NRM));
	    if (count_pk<FifthPK) sprintf(name_who,"%s",CCIMAG(ch,C_NRM));	    
	    if (count_pk<=FourthPK) sprintf(name_who,"%s",CCIYEL(ch,C_NRM));
	    if (count_pk<=ThirdPK) sprintf(name_who,"%s",CCICYN(ch,C_NRM));
	    if (count_pk<=SecondPK) sprintf(name_who,"%s",CCIGRN(ch,C_NRM));
	    if (count_pk==0) sprintf(name_who,"%s",IS_GOD(ch) ? CCWHT(ch, C_SPR) : CCNRM(ch,C_NRM));
 	    if (PLR_FLAGGED(ch,PLR_KILLER)==PLR_KILLER)
              sprintf(buftmp, "%s (ДУШЕГУБ)%s\r",CCIRED(ch,C_NRM),CCNRM(ch,C_NRM));



 if (!GET_TITLE(ch) || !*GET_TITLE(ch))
 {
    sprintf (title,"%s%s %s%s%s",name_who,
             race_name[(int) GET_RACE(ch)][(int) GET_SEX(ch)], GET_NAME(ch), clan,QNRM);
	     strcat(title,buftmp);
 }
 else
    { 
     if ((pos = strchr(GET_TITLE(ch),';')))
     {*(pos++) = '\0';
      sprintf(title,"%s%s %s%s%s%s%s",name_who,pos, GET_NAME(ch), 
              *GET_TITLE(ch) ? ", " : " ", GET_TITLE(ch),clan,QNRM);
      strcat(title,buftmp);
      *(--pos) = ';';
     }
     else
     {
      sprintf(title,"%s%s, %s%s%s",name_who,GET_NAME(ch),GET_TITLE(ch),clan,QNRM);
      strcat(title,buftmp);
     }
    }
 return title;
}
