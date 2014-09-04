/* ************************************************************************
*   File: act.social.c                                  Part of CircleMUD *
*  Usage: Functions to handle socials                                     *
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
#include "handler.h"
#include "db.h"
#include "spells.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;

void get_one_line(FILE *fl, char *buf);

/* local globals */
int top_of_socialm = -1;
int top_of_socialk = -1;
struct social_messg   *soc_mess_list = NULL;
struct social_keyword *soc_keys_list = NULL;

/* local functions */
int find_action(char* cmd);
int do_social(struct char_data *ch, char *argument);
ACMD(do_insult);

int find_action(char *cmd)
{
  int bot, top, mid, len, chk;

  bot = 0;
  top = top_of_socialk;
  len = strlen(cmd);

  if (top < 0 || !len)
    return (-1);

  for (;;) 
      {mid = (bot + top) / 2;
      
       if (bot > top)
          return (-1);
       if (!(chk = strn_cmp(cmd, soc_keys_list[mid].keyword, len)))
          {while (mid > 0 &&
                  !strn_cmp(cmd, soc_keys_list[mid-1].keyword, len))
                 mid--; 
           return (soc_keys_list[mid].social_message);
          }

       if (chk > 0)
          bot = mid + 1;
       else
          top = mid - 1;
      }
}



int do_social(struct char_data *ch, char *argument)
{
  int act_nr;
  char   social[MAX_INPUT_LENGTH];
  struct social_messg   *action;
  struct char_data *vict;

  if (!argument || !*argument) 
     return (FALSE);
     
  argument = one_argument(argument, social);
  
  if ((act_nr = find_action(social)) < 0)
     return (FALSE);
     
  action     = &soc_mess_list[act_nr];
  if (GET_POS(ch) < action->ch_min_pos || GET_POS(ch) > action->ch_max_pos)
     {send_to_char("Вам крайне неудобно это сделать.\r\n", ch);
      return (TRUE);
     } 

  if (action->char_found && argument)
     one_argument(argument, buf);
  else
     *buf = '\0';

  if (!*buf) 
     {send_to_char(action->char_no_arg, ch);
      send_to_char("\r\n", ch);
      act(action->others_no_arg, FALSE, ch, 0, 0, TO_ROOM);
      return(TRUE);
     }
  if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM))) 
     {send_to_char(action->not_found ? action->not_found :
                   "Поищите кого-нибодь более доступного для этих целей.\r\n", ch);
      send_to_char("\r\n", ch);
     } 
  else 
  if (vict == ch) 
     {send_to_char(action->char_no_arg, ch);
      send_to_char("\r\n", ch);
      act(action->others_no_arg, FALSE, ch, 0, 0, TO_ROOM);
     } 
  else 
     {if (GET_POS(vict) < action->vict_min_pos || GET_POS(vict) > action->vict_max_pos)
         act("$N2 сейчас, похоже, не до Вас.",
 	         FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
      else 
         {act(action->char_found, 0, ch, 0, vict, TO_CHAR | TO_SLEEP);
          act(action->others_found, FALSE, ch, 0, vict, TO_NOTVICT);
          act(action->vict_found, FALSE, ch, 0, vict, TO_VICT);
         }
     }
  return (TRUE);   
}



ACMD(do_insult)
{
  struct char_data *victim;

  one_argument(argument, arg);

  if (*arg) 
     {if (!(victim = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
         send_to_char("А он Вас и не услышит :(\r\n", ch);
      else 
         {if (victim != ch) 
             {sprintf(buf, "Вы оскорбили %s.\r\n", GET_PAD(victim,1));
	          send_to_char(buf, ch);

	          switch (number(0, 2)) 
	                 {
	          case 0: if (GET_SEX(ch) == SEX_MALE) 
	                     {if (GET_SEX(victim) == SEX_MALE)
	                          act("$n высмеял$g Вашу манеру держать меч !", FALSE, ch, 0, victim, TO_VICT);
	                      else
                              act("$n заявил$g, что удел любой женщины - дети, кухня и церковь.", FALSE, ch, 0, victim, TO_VICT);
	                      } 
	                   else 
	                      {	/* Ch == Woman */
	                       if (GET_SEX(victim) == SEX_MALE)
	                          act("$n заявил$g Вам, что у н$s больше... (что $e имел$g в виду ?)",
		                          FALSE, ch, 0, victim, TO_VICT);
	                        else
	                           act("$n обьявил$g всем о Вашем близком родстве с Бабой-Ягой.",
		                          FALSE, ch, 0, victim, TO_VICT);
	                      }
	                   break;
	          case 1:
	                   act("$n1 чем-то не удовлетворила Ваша мама!", FALSE, ch, 0, victim, TO_VICT);
	                   break;
	          default:
	                   act("$n предложил$g Вам посетить ближайший хутор !\r\n"
	                        "$e заявил$g, что там обитают на редкость крупные бабочки.", FALSE, ch, 0, victim, TO_VICT);
	                   break;
	                 }			/* end switch */

	          act("$n оскорбил$g $N1. СМЕРТЕЛЬНО.", TRUE, ch, 0, victim, TO_NOTVICT);
             } 
          else 
             {/* ch == victim */
	          send_to_char("Вы почувствовали себя оскорбленным.\r\n", ch);
             }
         }
     } 
  else
     send_to_char("Вы уверены, что стоит оскорблять такими словами всех ?\r\n", ch);
}


void load_socials(FILE *fl)
{
  char   line[MAX_INPUT_LENGTH], *scan, next_key[MAX_INPUT_LENGTH];
  int    key = -1, message = -1, c_min_pos, c_max_pos, v_min_pos, v_max_pos, what;

  /* get the first keyword line */
  get_one_line(fl, line);
  while (*line != '$') 
     {message++;
      scan = one_word(line, next_key);
      while (*next_key) 
            {key++;
             log("Social %d '%s' - message %d",key,next_key,message);
             soc_keys_list[key].keyword        = str_dup(next_key);
             soc_keys_list[key].social_message = message;
             scan = one_word(scan, next_key);
            }

      what = 0;     
      get_one_line(fl, line);
      while (*line != '#') 
            {scan = line;
             skip_spaces(&scan);
             if (scan && *scan && *scan != ';')
                {switch (what)
                 {case 0:
                  if (sscanf(scan, " %d %d %d %d \n", &c_min_pos, &c_max_pos, &v_min_pos, &v_max_pos) < 4) 
                     {log("SYSERR: format error in %d social file near social '%s' #d #d #d #d\n", message, line);
                      exit(1);
                     }
                  soc_mess_list[message].ch_min_pos = c_min_pos;
                  soc_mess_list[message].ch_max_pos = c_max_pos;                  
                  soc_mess_list[message].vict_min_pos = v_min_pos;   
                  soc_mess_list[message].vict_max_pos = v_max_pos;                  
                  break;
                  case 1:
                  soc_mess_list[message].char_no_arg = str_dup(scan);
                  break;
                  case 2:
                  soc_mess_list[message].others_no_arg = str_dup(scan);
                  break;
                  case 3:
                  soc_mess_list[message].char_found = str_dup(scan);
                  break;
                  case 4:
                  soc_mess_list[message].others_found = str_dup(scan);
                  break;
                  case 5:
                  soc_mess_list[message].vict_found = str_dup(scan);
                  break;
                  case 6:
                  soc_mess_list[message].not_found = str_dup(scan);
                  break;
                 } 
                } 
             if (!scan || *scan != ';')
                what++;    
             get_one_line(fl, line);
            }
      /* get next keyword line (or $) */
      get_one_line(fl, line);
     }
}

char *fread_action(FILE * fl, int nr)
{
  char buf[MAX_STRING_LENGTH];

  fgets(buf, MAX_STRING_LENGTH, fl);
  if (feof(fl)) 
     {log("SYSERR: fread_action: unexpected EOF near action #%d", nr);
      exit(1);
     }
  if (*buf == '#')
     return (NULL);

  buf[strlen(buf) - 1] = '\0';
  return (str_dup(buf));
}
