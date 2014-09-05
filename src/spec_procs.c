/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <unistd.h> // prool

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "dg_scripts.h"
#include "constants.h"

/*   external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern int guild_info[][3];

/* extern functions */
void add_follower(struct char_data * ch, struct char_data * leader);
ACMD(do_drop);
ACMD(do_gen_door);
ACMD(do_say);
int  go_track(struct char_data *ch, struct char_data *victim, int skill_no);
int  has_key(struct char_data *ch, obj_vnum key);
int  find_first_step(room_rnum src, room_rnum target, struct char_data *ch);
void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd);
int  check_recipe_items(struct char_data * ch, int spellnum, int spelltype, int extract);
SPECIAL(shop_keeper);
void ASSIGNMASTER(mob_vnum mob, SPECIAL(fname), int learn_info);

/* local functions */
void sort_spells(void);
int compare_spells(const void *x, const void *y);
char *how_good(struct char_data *ch, int percent);
void list_skills(struct char_data * ch);
void list_spells(struct char_data * ch);
int  slot_for_char(struct char_data * ch, int i);
SPECIAL(guild_mono);
SPECIAL(guild_poly);
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(guild_guard);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);
SPECIAL(bank);

#define IS_SHOPKEEPER(ch) (IS_MOB(ch) && mob_index[GET_MOB_RNUM(ch)].func == shop_keeper)

/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

int spell_sort_info[MAX_SKILLS + 1];


int compare_spells(const void *x, const void *y)
{
  int	a = *(const int *)x,
	b = *(const int *)y;

  return strcmp(spell_info[a].name, spell_info[b].name);
}

void sort_spells(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a <= MAX_SKILLS; a++)
      spell_sort_info[a] = a;

  qsort(&spell_sort_info[1], MAX_SKILLS, sizeof(int), compare_spells);
}

char *how_good(struct char_data *ch, int percent)
{ static char out_str[128];
  if (percent < 0)
     strcpy(out_str," !������! ");
  else
  if (percent == 0)
     sprintf(out_str, " %s(�� �������)%s", CCINRM(ch, C_NRM), CCNRM(ch, C_NRM));
  else
  if (percent <= 10)
     sprintf(out_str, " %s(������)%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
  else
  if (percent <= 20)
     sprintf(out_str, " %s(����� �����)%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
  else
  if (percent <= 30)
     sprintf(out_str, " %s(�����)%s", CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
  else
  if (percent <= 40)
     sprintf(out_str, " %s(���� ��������)%s", CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
  else
  if (percent <= 50)
     sprintf(out_str, " %s(������)%s", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
  else
  if (percent <= 60)
     sprintf(out_str, " %s(���� ��������)%s", CCIYEL(ch, C_NRM), CCNRM(ch, C_NRM));
  else
  if (percent <= 70)
     sprintf(out_str, " %s(������)%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  else
  if (percent <= 80)
     sprintf(out_str, " %s(����� ������)%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  else
  if (percent <= 90)
     sprintf(out_str, " %s(�����������)%s", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  else
  if (percent <= 100)
     sprintf(out_str, " %s(����������)%s", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  else
  if (percent <= 110)
     sprintf(out_str, " %s(��������)%s", CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
  else
     sprintf(out_str, " %s(�����������)%s", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
  sprintf(out_str+strlen(out_str)," %d%%",percent);
  return out_str;
}

const char *prac_types[] =
{ "spell",
  "skill"
};

#define LEARNED_LEVEL	0	/* % known which is considered "learned" */
#define MAX_PER_PRAC	1	/* max percent gain in skill per practice */
#define MIN_PER_PRAC	2	/* min percent gain in skill per practice */
#define PRAC_TYPE	3	/* should it say 'spell' or 'skill'?	 */

/* actual prac_params are in class.c */
extern int prac_params[4][NUM_CLASSES];

#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])
#define SPLSKL(ch) (prac_types[prac_params[PRAC_TYPE][(int)GET_CLASS(ch)]])

void list_skills(struct char_data * ch)
{
  int i=0, sortpos;

  sprintf(buf, "�� �������� ���������� �������� :\r\n");

  strcpy(buf2, buf);

  for (sortpos = 1; sortpos <= MAX_SKILLS; sortpos++)
      {if (strlen(buf2) >= MAX_STRING_LENGTH - 60)
          {strcat(buf2, "**OVERFLOW**\r\n");
           break;
          }
       if (GET_SKILL(ch, sortpos))
          {if (!skill_info[sortpos].name ||
               *skill_info[sortpos].name == '!')
              continue;
           switch (sortpos)
           {case SKILL_AID:
            case SKILL_DRUNKOFF:
            case SKILL_IDENTIFY:
            case SKILL_POISONED:
            case SKILL_COURAGE:
            if (timed_by_skill(ch, sortpos))
               sprintf(buf,"[%3d] ", timed_by_skill(ch, sortpos));
            else
               sprintf(buf,"[-!-] ");
            break;
            default:
            sprintf(buf,"      ");
            }
           sprintf(buf+strlen(buf), "%-20s %s\r\n", skill_info[sortpos].name, how_good(ch, GET_SKILL(ch, sortpos)));
           strcat(buf2, buf);	/* The above, ^ should always be safe to do. */
           i++;
          }
      }
  if (!i)
     sprintf(buf2 + strlen(buf2),"��� ������.\r\n");
  page_string(ch->desc, buf2, 1);
}

void list_spells(struct char_data * ch)
{char names[MAX_SLOT][MAX_STRING_LENGTH];
 int  slots[MAX_SLOT],i,max_slot=0, slot_num, is_full, gcount=0;

 is_full = 0;
 max_slot= 0;
 for (i = 0; i < MAX_SLOT; i++)
     {*names[i] = '\0';
       slots[i] = 0;
     }
 for (i = 1; i <= MAX_SPELLS; i++)
     {if (!GET_SPELL_TYPE(ch,i))
         continue;
      if (!spell_info[i].name || *spell_info[i].name == '!')
         continue;
      if ((GET_SPELL_TYPE(ch,i) & 0xFF) == SPELL_RUNES &&
          !check_recipe_items(ch,i,SPELL_RUNES,FALSE))
         continue;
      slot_num = spell_info[i].slot_forc[(int) GET_CLASS(ch)] - 1;
      max_slot = MAX(slot_num+1,max_slot);
      slots[slot_num] += sprintf(names[slot_num]+slots[slot_num],
                                 "%s|<%c%c%c%c%c%c%c%c> %-25s|",
                                 slots[slot_num] % 80 < 10 ? "\r\n" : "  ",
                                 IS_SET(GET_SPELL_TYPE(ch,i),SPELL_KNOW)   ?  'K' : '.',
                                 IS_SET(GET_SPELL_TYPE(ch,i),SPELL_TEMP)   ?  'T' : '.',
                                 IS_SET(GET_SPELL_TYPE(ch,i),SPELL_POTION) ?  'P' : '.',
                                 IS_SET(GET_SPELL_TYPE(ch,i),SPELL_WAND)   ?  'W' : '.',
                                 IS_SET(GET_SPELL_TYPE(ch,i),SPELL_SCROLL) ?  'S' : '.',
                                 IS_SET(GET_SPELL_TYPE(ch,i),SPELL_ITEMS)  ?  'I' : '.',
                                 IS_SET(GET_SPELL_TYPE(ch,i),SPELL_RUNES)  ?  'R' : '.',
                                 '.',
                                 spell_info[i].name);
      is_full++;
     };
 gcount = sprintf(buf2+gcount,"  %s��� �������� ��������� ����� :%s",
                              CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
 if (is_full)
    {for (i = 0; i < max_slot; i++)
         {if (slots[i] != 0) gcount += sprintf(buf2+gcount,"\r\n���� %d",i+1);
          if (slots[i])
             gcount += sprintf(buf2+gcount,"%s",names[i]);
/*          else
             gcount += sprintf(buf2+gcount,"\n\r�����.");*/
         }
    }
 else
    gcount += sprintf(buf2+gcount,"\r\n� ��������� ����� ����� ��� ���������� !");
 gcount += sprintf(buf2+gcount,"\r\n");
 page_string(ch->desc, buf2, 1);
}

struct guild_learn_type
{      int skill_no;
       int spell_no;
       int level;
};

struct guild_mono_type
{      int      races;      /* bitvector */
       int      classes;    /* bitvector */
       int      religion;   /* bitvector */
       int      alignment;  /* bitvector */
       struct   guild_learn_type *learn_info;
};

struct guild_poly_type
{      int      races;      /* bitvector */
       int      classes;    /* bitvector */
       int      religion;   /* bitvector */
       int      alignment;  /* bitvector */
       int      skill_no;
       int      spell_no;
       int      level;
};

int    GUILDS_MONO_USED = 0;
int    GUILDS_POLY_USED = 0;

struct guild_mono_type *guild_mono_info = NULL;
struct guild_poly_type **guild_poly_info = NULL;

void init_guilds(void)
{ FILE *magic;
  char name[MAX_INPUT_LENGTH],
       line[256], line1[256], line2[256], line3[256],
       line4[256],line5[256], line6[256],*pos;
  int    i, spellnum, skillnum, num, type=0, lines=0, level, pgcount=0, mgcount=0;
  struct guild_poly_type *poly_guild=NULL;
  struct guild_mono_type mono_guild;

  if (!(magic = fopen(LIB_MISC"guilds.lst","r")))
     {log("Cann't open guilds list file...");
      _exit(1);
     }
  while (get_line(magic,name))
        {if (!name[0] || name[0] == ';')
            continue;
         log("<%s>",name);
         if ((lines = sscanf(name, "%s %s %s %s %s %s %s", line, line1, line2,
                             line3, line4, line5, line6)) == 0)
            continue;
         // log("%d",lines);

         if (!strn_cmp(line,"monoguild",strlen(line)) || !strn_cmp(line,"�������������",strlen(line)))
            {type = 1;
             if (lines < 5)
                {log("Bad format for monoguild header, #s #s #s #s #s need...");
                 _exit(1);
                }
             mono_guild.learn_info = NULL;
             mono_guild.races      = 0;
             mono_guild.classes    = 0;
             mono_guild.religion   = 0;
             mono_guild.alignment  = 0;
             mgcount                = 0;
             for (i=0;*(line1+i);i++)
                 if (strchr("!1xX",*(line1+i)))
                    SET_BIT(mono_guild.races,(1 << i));
             for (i=0;*(line2+i);i++)
                 if (strchr("!1xX",*(line2+i)))
                    SET_BIT(mono_guild.classes,(1 << i));
             for (i=0;*(line3+i);i++)
                 if (strchr("!1xX",*(line3+i)))
                    SET_BIT(mono_guild.religion,(1 << i));
             for (i=0;*(line4+i);i++)
                 if (strchr("!1xX",*(line4+i)))
                    SET_BIT(mono_guild.alignment,(1 << i));
            }
          else
          if (!strn_cmp(line,"polyguild",strlen(line)) || !strn_cmp(line,"���������c����",strlen(line)))
             {type       = 2;
              poly_guild = NULL;
              pgcount    = 0;
             }
          else
          if (!strn_cmp(line,"master",strlen(line)) || !strn_cmp(line,"�������",strlen(line)))
             {if ((num = atoi(line1)) == 0 || real_mobile(num) < 0)
                 {log("Cann't assign master %s in guilds.lst",line1);
                  _exit(1);
                 }

              if (!((type == 1 || type == 11) && mono_guild.learn_info) &&
                  !((type == 2 || type == 12) && poly_guild))
                 {log("Cann't define guild info for master %s",line1);
                  _exit(1);
                 }
              if (type == 1 || type == 11)
                 {if (type == 1)
                     {if (!guild_mono_info)
                         CREATE(guild_mono_info, struct guild_mono_type, GUILDS_MONO_USED+1);
                      else
                         RECREATE(guild_mono_info, struct guild_mono_type, GUILDS_MONO_USED+1);
                      log("Create mono guild for mobile %s",line1);
                      RECREATE(mono_guild.learn_info, struct guild_learn_type, mgcount+1);
                      (mono_guild.learn_info+mgcount)->skill_no = -1;
                      (mono_guild.learn_info+mgcount)->spell_no = -1;
                      (mono_guild.learn_info+mgcount)->level    = -1;
                      guild_mono_info[GUILDS_MONO_USED] = mono_guild;
                      GUILDS_MONO_USED++;
                     }
                  else
                     log("Assign mono guild for mobile %s",line1);
                  ASSIGNMASTER(num,guild_mono,GUILDS_MONO_USED);
                  type = 11;
                 }
              if (type == 2 || type == 12)
                 {if (type == 2)
                     {if (!guild_poly_info)
                         CREATE(guild_poly_info, struct guild_poly_type * , GUILDS_POLY_USED+1);
                      else
                         RECREATE(guild_poly_info, struct guild_poly_type * , GUILDS_POLY_USED+1);
                      log("Create poly guild for mobile %s",line1);
                      RECREATE(poly_guild, struct guild_poly_type, pgcount+1);
                      (poly_guild+pgcount)->skill_no = -1;
                      (poly_guild+pgcount)->spell_no = -1;
                      (poly_guild+pgcount)->level    = -1;
                      guild_poly_info[GUILDS_POLY_USED] = poly_guild;
                      GUILDS_POLY_USED++;
                     }
                  else
                     log("Assign poly guild for mobile %s",line1);
                  ASSIGNMASTER(num,guild_poly,GUILDS_POLY_USED);
                  type = 12;
                 }
             }
          else
          if (type == 1)
             {if (lines < 3)
                 {log("You need use 3 arguments for monoguild");
                  _exit(1);
                 }
              if ((spellnum = atoi(line)) == 0 || spellnum > MAX_SPELLS)
                 {if ((pos = strchr(line,'.')))
                     *pos = ' ';
                  spellnum = find_spell_num(line);
                 }
              if ((skillnum = atoi(line1)) == 0 || skillnum > MAX_SKILLS)
                 {if ((pos = strchr(line1,'.')))
                     *pos = ' ';
                  skillnum = find_skill_num(line1);
                 }
              if (skillnum <= 0 && spellnum <= 0)
                 {log("Unknown skill or spell for monoguild");
                  _exit(1);
                 }
              if ((level = atoi(line2)) == 0 || level >= LVL_IMMORT)
                 {log("Use 1-%d level for guilds",LVL_IMMORT);
                  _exit(1);
                 }
              if (!mono_guild.learn_info)
                 CREATE(mono_guild.learn_info, struct guild_learn_type, mgcount+1);
              else
                 RECREATE(mono_guild.learn_info, struct guild_learn_type, mgcount+1);
              (mono_guild.learn_info+mgcount)->spell_no = MAX(0,spellnum);
              (mono_guild.learn_info+mgcount)->skill_no = skillnum;
              (mono_guild.learn_info+mgcount)->level    = level;
              // log("->%d %d %d<-",spellnum,skillnum,level);
              mgcount++;
             }
          else

          if (type == 2)
             {if (lines < 7)
                 {log("You need use 7 arguments for poluguild");
                  _exit(1);
                 }
              if (!poly_guild)
                 CREATE(poly_guild, struct guild_poly_type, pgcount+1);
              else
                 RECREATE(poly_guild, struct guild_poly_type, pgcount+1);
              (poly_guild+pgcount)->races    = 0;
              (poly_guild+pgcount)->classes  = 0;
              (poly_guild+pgcount)->religion = 0;
              (poly_guild+pgcount)->alignment= 0;
              for (i=0;*(line+i);i++)
                  if (strchr("!1xX",*(line+i)))
                     SET_BIT((poly_guild+pgcount)->races,(1 << i));
              for (i=0;*(line1+i);i++)
                  if (strchr("!1xX",*(line1+i)))
                     SET_BIT((poly_guild+pgcount)->classes,(1 << i));
              for (i=0;*(line2+i);i++)
                  if (strchr("!1xX",*(line2+i)))
                     SET_BIT((poly_guild+pgcount)->religion,(1 << i));
              for (i=0;*(line3+i);i++)
                  if (strchr("!1xX",*(line3+i)))
                     SET_BIT((poly_guild+pgcount)->alignment,(1 << i));
              if ((spellnum = atoi(line4)) == 0 || spellnum > MAX_SPELLS)
                 {if ((pos = strchr(line4,'.')))
                     *pos = ' ';
                  spellnum = find_spell_num(line4);
                 }
              if ((skillnum = atoi(line5)) == 0 || skillnum > MAX_SKILLS)
                 {if ((pos = strchr(line5,'.')))
                     *pos = ' ';
                  skillnum = find_skill_num(line5);
                 }
              if (skillnum <= 0 && spellnum <= 0)
                 {log("Unknown skill or spell for polyguild");
                  _exit(1);
                 }
              if ((level = atoi(line6)) == 0 || level >= LVL_IMMORT)
                 {log("Use 1-%d level for guilds",LVL_IMMORT);
                  _exit(1);
                 }
              (poly_guild+pgcount)->spell_no = MAX(0,spellnum);
              (poly_guild+pgcount)->skill_no = skillnum;
              (poly_guild+pgcount)->level    = level;
              // log("->%d %d %d<-",spellnum,skillnum,level);
              pgcount++;
             }
        }
  fclose(magic);
  return;
}

#define SCMD_LEARN 1

SPECIAL(guild_mono)
{
  int skill_no, command=0, gcount=0, info_num=0, found = FALSE, sfound = FALSE, i, bits;
  struct char_data *victim = (struct char_data *) me;

  if (IS_NPC(ch))
     return (0);

  if (CMD_IS("�����") || CMD_IS("practice"))
     command = SCMD_LEARN;
  else
     return (0);

  if ((info_num = mob_index[victim->nr].stored) <= 0 || info_num > GUILDS_MONO_USED)
     {act("$N ������$G : '������, $n, � ��� � ��������.'",
          FALSE, ch, 0, victim, TO_CHAR);
      return (1);
     };
  info_num--;
  if (!IS_BITS(guild_mono_info[info_num].classes, GET_CLASS(ch)) ||
      !IS_BITS(guild_mono_info[info_num].races,   GET_RACE(ch))  ||
      !IS_BITS(guild_mono_info[info_num].religion, GET_RELIGION(ch))
     )
     {act("$N ������$g : '$n, � �� ��� �����, ��� ��.'",
          FALSE, ch, 0, victim, TO_CHAR);
      return (1);
     }

  skip_spaces(&argument);

  switch (command)
{ case SCMD_LEARN:
  if (!*argument)
     {gcount += sprintf(buf,"� ���� ������� ���� ����������:\r\n");
      for (i = 0, found = FALSE;
           (guild_mono_info[info_num].learn_info+i)->spell_no >= 0; i++)
          {if ((guild_mono_info[info_num].learn_info+i)->level > GET_LEVEL(ch))
              continue;
           // log("%d - %d",(guild_mono_info[info_num].learn_info+i)->skill_no, (guild_mono_info[info_num].learn_info+i)->spell_no);
           if ((skill_no = bits = (guild_mono_info[info_num].learn_info+i)->skill_no) > 0 &&
               (!GET_SKILL(ch,skill_no) || IS_GRGOD(ch)))
              {gcount += sprintf(buf + gcount, "- ������ %s\"%s\"\%s\r\n",
                                 CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
               found  = TRUE;
              }
           if (!(bits = -2 * bits) || bits == SPELL_TEMP)
              bits = SPELL_KNOW;
           if ((skill_no = (guild_mono_info[info_num].learn_info+i)->spell_no) &&
               (!((GET_SPELL_TYPE(ch,skill_no) & bits) == bits) || IS_GRGOD(ch)))
              {gcount += sprintf(buf + gcount, "- ����� %s\"%s\"\%s\r\n",
                                 CCCYN(ch, C_NRM), spell_name(skill_no), CCNRM(ch, C_NRM));
               found  = TRUE;
              }
          }
      if (!found)
         {act("$N ������$G : '������, ���� ������ ������ ����.'",
              FALSE, ch, 0, victim, TO_CHAR);
          return (1);
         }
      else
         {send_to_char(buf,ch);
          return (1);
         }
     }

  if (!strn_cmp(argument, "���", strlen(argument)) ||
      !strn_cmp(argument, "all", strlen(argument)))
     {for (i = 0, found = FALSE, sfound = TRUE;
           (guild_mono_info[info_num].learn_info+i)->spell_no >= 0; i++)
          {if ((guild_mono_info[info_num].learn_info+i)->level > GET_LEVEL(ch))
              continue;
           if ((skill_no = bits = (guild_mono_info[info_num].learn_info+i)->skill_no) > 0 &&
               !GET_SKILL(ch,skill_no))
              {// sprintf(buf, "$N ������$G ��� ������ %s\"%s\"\%s",
               //             CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
               // act(buf,FALSE,ch,0,victim,TO_CHAR);
               // GET_SKILL(ch,skill_no) = 10;
               sfound  = TRUE;
              }
           if (!(bits = -2*bits) || bits == SPELL_TEMP)
              bits = SPELL_KNOW;
           if ((skill_no = (guild_mono_info[info_num].learn_info+i)->spell_no) &&
               !((GET_SPELL_TYPE(ch,skill_no) & bits) == bits))
              {gcount += sprintf(buf, "$N ������$G ��� ����� %s\"%s\"\%s",
                                 CCCYN(ch, C_NRM), spell_name(skill_no), CCNRM(ch, C_NRM));
               act(buf,FALSE,ch,0,victim,TO_CHAR);
               if (IS_SET(bits, SPELL_KNOW))
                  SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_KNOW);
               if (IS_SET(bits, SPELL_ITEMS))
                  SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_ITEMS);
               if (IS_SET(bits, SPELL_RUNES))
                  SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_RUNES);
               if (IS_SET(bits, SPELL_POTION))
                  {SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_POTION);
                   GET_SKILL(ch, SKILL_CREATE_POTION) = MAX(10, GET_SKILL(ch, SKILL_CREATE_POTION));
                  }
               if (IS_SET(bits, SPELL_WAND))
                  {SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_WAND);
                   GET_SKILL(ch, SKILL_CREATE_WAND) = MAX(10, GET_SKILL(ch, SKILL_CREATE_WAND));
                  }
               if (IS_SET(bits, SPELL_SCROLL))
                  {SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_SCROLL);
                   GET_SKILL(ch, SKILL_CREATE_SCROLL) = MAX(10, GET_SKILL(ch, SKILL_CREATE_SCROLL));
                  }
               found  = TRUE;
              }
          }
      if (sfound)
         act("$N ������$G : \r\n"
              "'$n, � ���������, ��� ����� ������ ���������� ���� ���������� ������.'\r\n"
              "'������ ���� ����� ����������� ���� ������.'",
          FALSE, ch, 0, victim, TO_CHAR);
      if (!found)
         act("$N ������ ������ ��� �� ������$G.",
             FALSE, ch, 0, victim, TO_CHAR);
      return (1);
     }

  if (((skill_no = find_skill_num(argument)) > 0 && skill_no <= MAX_SKILLS))
     {for (i = 0, found = FALSE;
           (guild_mono_info[info_num].learn_info+i)->spell_no >= 0; i++)
          {if ((guild_mono_info[info_num].learn_info+i)->level > GET_LEVEL(ch))
              continue;
           if (skill_no == (guild_mono_info[info_num].learn_info+i)->skill_no)
              {if (GET_SKILL(ch,skill_no))
                  act("$N ������$g ��� : '����� ������ �� ����, �� ��� �������� ���� �������.'",
                      FALSE, ch, 0, victim, TO_CHAR);
               else
                  {sprintf(buf, "$N ������$G ��� ������ %s\"%s\"\%s",
                                CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
                   act(buf,FALSE,ch,0,victim,TO_CHAR);
                   GET_SKILL(ch,skill_no) = 10;
                  }
               found  = TRUE;
              }
          }
      if (!found)
         act("$N ������$G : '��� �� ������ ����� ������.'",
             FALSE, ch, 0, victim, TO_CHAR);
      return (1);
     }

  if (((skill_no = find_spell_num(argument)) > 0 && skill_no <= MAX_SPELLS))
     {for (i = 0, found = FALSE;
           (guild_mono_info[info_num].learn_info+i)->spell_no >= 0; i++)
          {if ((guild_mono_info[info_num].learn_info+i)->level > GET_LEVEL(ch))
              continue;
           if (skill_no == (guild_mono_info[info_num].learn_info+i)->spell_no)
              {if (!(bits = -2*(guild_mono_info[info_num].learn_info+i)->skill_no) || bits == SPELL_TEMP)
                  bits = SPELL_KNOW;
               if ((GET_SPELL_TYPE(ch,skill_no) & bits) == bits)
                  {if (IS_SET(bits, SPELL_KNOW))
                      sprintf(buf,"$N ������$G : \r\n"
                                  "'$n, ����� ��������� ��� ����������, ���� ���������� �������\r\n"
                                  "%s ����(�����) '%s' <����>%s � �������� �������� <ENTER>'.",
                                  CCCYN(ch, C_NRM), spell_name(skill_no), CCNRM(ch, C_NRM));
                   else
                   if (IS_SET(bits, SPELL_ITEMS))
                      sprintf(buf,"$N ������$G : \r\n"
                                  "'$n, ����� ��������� ��� �����, ���� ���������� �������\r\n"
                                  "%s ������� '%s' <����>%s � �������� �������� <ENTER>'.",
                                  CCCYN(ch, C_NRM), spell_name(skill_no), CCNRM(ch, C_NRM));
                   else
                   if (IS_SET(bits, SPELL_RUNES))
                      sprintf(buf,"$N ������$G : \r\n"
                                  "'$n, ����� ��������� ��� �����, ���� ���������� �������\r\n"
                                  "%s ���� '%s' <����>%s � �������� �������� <ENTER>'.",
                                  CCCYN(ch, C_NRM), spell_name(skill_no), CCNRM(ch, C_NRM));
                   else
                      strcpy(buf,"�� ��� ������ ���.");
                   act(buf,FALSE, ch, 0, victim, TO_CHAR);
                  }
               else
                  {sprintf(buf, "$N ������$G ��� ����� %s\"%s\"\%s",
                                CCCYN(ch, C_NRM), spell_name(skill_no), CCNRM(ch, C_NRM));
                   act(buf,FALSE,ch,0,victim,TO_CHAR);
                   if (IS_SET(bits, SPELL_KNOW))
                      SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_KNOW);
                   if (IS_SET(bits, SPELL_ITEMS))
                      SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_ITEMS);
                   if (IS_SET(bits, SPELL_RUNES))
                      SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_RUNES);
                   if (IS_SET(bits, SPELL_POTION))
                      {SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_POTION);
                       GET_SKILL(ch, SKILL_CREATE_POTION) = MAX(10, GET_SKILL(ch, SKILL_CREATE_POTION));
                      }
                   if (IS_SET(bits, SPELL_WAND))
                      {SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_WAND);
                       GET_SKILL(ch, SKILL_CREATE_WAND) = MAX(10, GET_SKILL(ch, SKILL_CREATE_WAND));
                      }
                   if (IS_SET(bits, SPELL_SCROLL))
                      {SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_SCROLL);
                       GET_SKILL(ch, SKILL_CREATE_SCROLL) = MAX(10, GET_SKILL(ch, SKILL_CREATE_SCROLL));
                      }
                  }
               found  = TRUE;
              }
          }
      if (!found)
         act("$N ������$G : '� � ���$G �� ���� ����� �����.'",
             FALSE, ch, 0, victim, TO_CHAR);
      return (1);
     }

  act("$N ������$G ��� : '� ������� � ������ ����� �� ����$G.'",
      FALSE, ch, 0, victim, TO_CHAR);
  return (1);
}
return (0);
}


SPECIAL(guild_poly)
{
  int skill_no, command=0, gcount=0, info_num=0, found = FALSE, sfound = FALSE, i, bits;
  struct char_data *victim = (struct char_data *) me;

  if (IS_NPC(ch))
     return (0);

  if (CMD_IS("�����") || CMD_IS("practice"))
     command = SCMD_LEARN;
  else
     return (0);

  if ((info_num = mob_index[victim->nr].stored) <= 0 || info_num > GUILDS_POLY_USED)
     {act("$N ������$G : '������, $n, � ��� � ��������.'",
          FALSE, ch, 0, victim, TO_CHAR);
      return (1);
     };
  info_num--;

  skip_spaces(&argument);

  switch (command)
{ case SCMD_LEARN:
  if (!*argument)
     {gcount += sprintf(buf,"� ���� ������� ���� ����������:\r\n");
      for (i = 0, found = FALSE;
           (guild_poly_info[info_num]+i)->spell_no >= 0; i++)
          {if ((guild_poly_info[info_num]+i)->level > GET_LEVEL(ch))
              continue;
           // log("%d - %d",(guild_poly_info[info_num]+i)->skill_no,(guild_poly_info[info_num]+i)->spell_no);
           if (!IS_BITS((guild_poly_info[info_num]+i)->classes, GET_CLASS(ch)) ||
               !IS_BITS((guild_poly_info[info_num]+i)->races,   GET_RACE(ch))  ||
               !IS_BITS((guild_poly_info[info_num]+i)->religion, GET_RELIGION(ch))
              )
              continue;

           if ((skill_no = bits = (guild_poly_info[info_num]+i)->skill_no) > 0 &&
               (!GET_SKILL(ch,skill_no) || IS_GRGOD(ch)))
              {gcount += sprintf(buf + gcount, "- ������ %s\"%s\"\%s\r\n",
                                 CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
               found  = TRUE;
              }
           if (!(bits = -2*bits) || bits == SPELL_TEMP)
              bits = SPELL_KNOW;
           if ((skill_no = (guild_poly_info[info_num]+i)->spell_no) &&
               (!((GET_SPELL_TYPE(ch,skill_no) & bits) == bits) || IS_GRGOD(ch)))
              {gcount += sprintf(buf + gcount, "- ����� %s\"%s\"\%s\r\n",
                                 CCCYN(ch, C_NRM), spell_name(skill_no), CCNRM(ch, C_NRM));
               found  = TRUE;
              }
          }
      if (!found)
         {act("$N ������$G : '������, � �� ����� ���� ������.'",
              FALSE, ch, 0, victim, TO_CHAR);
          return (1);
         }
      else
         {send_to_char(buf,ch);
          return (1);
         }
     }

  if (!strn_cmp(argument, "���", strlen(argument)) ||
      !strn_cmp(argument, "all", strlen(argument)))
     {for (i = 0, found = FALSE, sfound = FALSE;
           (guild_poly_info[info_num]+i)->spell_no >= 0; i++)
          {if ((guild_poly_info[info_num]+i)->level > GET_LEVEL(ch))
              continue;
           if (!IS_BITS((guild_poly_info[info_num]+i)->classes, GET_CLASS(ch)) ||
               !IS_BITS((guild_poly_info[info_num]+i)->races,   GET_RACE(ch))  ||
               !IS_BITS((guild_poly_info[info_num]+i)->religion, GET_RELIGION(ch))
              )
              continue;

           if ((skill_no = bits = (guild_poly_info[info_num]+i)->skill_no) > 0 &&
               !GET_SKILL(ch,skill_no))
              {// sprintf(buf, "$N ������$G ��� ������ %s\"%s\"\%s",
               //             CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
               // act(buf,FALSE,ch,0,victim,TO_CHAR);
               // GET_SKILL(ch,skill_no) = 10;
               sfound  = TRUE;
              }

           if (!(bits = -2*bits) || bits == SPELL_TEMP)
              bits = SPELL_KNOW;
           if ((skill_no = (guild_poly_info[info_num]+i)->spell_no) &&
               !((GET_SPELL_TYPE(ch,skill_no) & bits) == bits))
              {gcount += sprintf(buf, "$N ������$G ��� ����� %s\"%s\"\%s",
                                 CCCYN(ch, C_NRM), spell_name(skill_no), CCNRM(ch, C_NRM));
               act(buf,FALSE,ch,0,victim,TO_CHAR);

               if (IS_SET(bits, SPELL_KNOW))
                  SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_KNOW);
               if (IS_SET(bits, SPELL_ITEMS))
                  SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_ITEMS);
               if (IS_SET(bits, SPELL_RUNES))
                  SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_RUNES);
               if (IS_SET(bits, SPELL_POTION))
                  {SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_POTION);
                   GET_SKILL(ch, SKILL_CREATE_POTION) = MAX(10, GET_SKILL(ch, SKILL_CREATE_POTION));
                  }
               if (IS_SET(bits, SPELL_WAND))
                  {SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_WAND);
                   GET_SKILL(ch, SKILL_CREATE_WAND) = MAX(10, GET_SKILL(ch, SKILL_CREATE_WAND));
                  }
               if (IS_SET(bits, SPELL_SCROLL))
                  {SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_SCROLL);
                   GET_SKILL(ch, SKILL_CREATE_SCROLL) = MAX(10, GET_SKILL(ch, SKILL_CREATE_SCROLL));
                  }
               found  = TRUE;
              }
          }
      if (sfound)
         act("$N ������$G : \r\n"
              "'$n, � ���������, ��� ����� ������ ���������� ���� ���������� ������.'\r\n"
              "'������ ���� ����� ����������� ���� ������.'",
          FALSE, ch, 0, victim, TO_CHAR);
      if (!found)
         act("$N ������ ������ ��� �� ������$G.",
             FALSE, ch, 0, victim, TO_CHAR);
      return (1);
     }

  if (((skill_no = find_skill_num(argument)) > 0 && skill_no <= MAX_SKILLS))
     {for (i = 0, found = FALSE;
           (guild_poly_info[info_num]+i)->spell_no >= 0; i++)
          {if ((guild_poly_info[info_num]+i)->level > GET_LEVEL(ch))
              continue;
           if (!IS_BITS((guild_poly_info[info_num]+i)->classes, GET_CLASS(ch)) ||
               !IS_BITS((guild_poly_info[info_num]+i)->races,   GET_RACE(ch))  ||
               !IS_BITS((guild_poly_info[info_num]+i)->religion, GET_RELIGION(ch))
              )
              continue;
           if (skill_no == (guild_poly_info[info_num]+i)->skill_no)
              {if (GET_SKILL(ch,skill_no))
                  act("$N ������$G ��� : '����� ������ �� ����, �� ��� �������� ���� �������.'",
                      FALSE, ch, 0, victim, TO_CHAR);
               else
                  {sprintf(buf, "$N ������$G ��� ������ %s\"%s\"\%s",
                                CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
                   act(buf,FALSE,ch,0,victim,TO_CHAR);
                   GET_SKILL(ch,skill_no) = 10;
                  }
               found  = TRUE;
              }
          }
      if (!found)
         act("$N ������$G : '��� �� ������ ����� ������.'",
             FALSE, ch, 0, victim, TO_CHAR);
      return (1);
     }

  if (((skill_no = find_spell_num(argument)) > 0 && skill_no <= MAX_SPELLS))
     {for (i = 0, found = FALSE;
           (guild_poly_info[info_num]+i)->spell_no >= 0; i++)
          {if ((guild_poly_info[info_num]+i)->level > GET_LEVEL(ch))
              continue;
           if (!(bits = -2*(guild_poly_info[info_num]+i)->skill_no) || bits == SPELL_TEMP)
              bits = SPELL_KNOW;
           if (!IS_BITS((guild_poly_info[info_num]+i)->classes, GET_CLASS(ch)) ||
               !IS_BITS((guild_poly_info[info_num]+i)->races,   GET_RACE(ch))  ||
               !IS_BITS((guild_poly_info[info_num]+i)->religion,GET_RELIGION(ch))
              )
              continue;

           if (skill_no == (guild_poly_info[info_num]+i)->spell_no)
              {if ((GET_SPELL_TYPE(ch,skill_no) & bits) == bits)
                  {if (IS_SET(bits, SPELL_KNOW))
                      sprintf(buf,"$N ������$G : \r\n"
                                  "'$n, ����� ��������� ��� ����������, ���� ���������� �������\r\n"
                                  "%s ����(�����) '%s' <����>%s � �������� �������� <ENTER>'.",
                                  CCCYN(ch, C_NRM), spell_name(skill_no), CCNRM(ch, C_NRM));
                   else
                   if (IS_SET(bits, SPELL_ITEMS))
                      sprintf(buf,"$N ������$G : \r\n"
                                  "'$n, ����� ��������� ��� �����, ���� ���������� �������\r\n"
                                  "%s ������� '%s' <����>%s � �������� �������� <ENTER>'.",
                                  CCCYN(ch, C_NRM), spell_name(skill_no), CCNRM(ch, C_NRM));
                   else
                   if (IS_SET(bits, SPELL_RUNES))
                      sprintf(buf,"$N ������$G : \r\n"
                                  "'$n, ����� ��������� ��� �����, ���� ���������� �������\r\n"
                                  "%s ���� '%s' <����>%s � �������� �������� <ENTER>'.",
                                  CCCYN(ch, C_NRM), spell_name(skill_no), CCNRM(ch, C_NRM));
                   else
                      strcpy(buf,"�� ��� ������ ���.");
                   act(buf,FALSE, ch, 0, victim, TO_CHAR);
                  }
               else
                  {sprintf(buf, "$N ������$G ��� ����� %s\"%s\"\%s",
                                CCCYN(ch, C_NRM), spell_name(skill_no), CCNRM(ch, C_NRM));
                   act(buf,FALSE,ch,0,victim,TO_CHAR);
                   if (IS_SET(bits, SPELL_KNOW))
                      SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_KNOW);
                   if (IS_SET(bits, SPELL_ITEMS))
                      SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_ITEMS);
                   if (IS_SET(bits, SPELL_RUNES))
                      SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_RUNES);
                   if (IS_SET(bits, SPELL_POTION))
                      {SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_POTION);
                       GET_SKILL(ch, SKILL_CREATE_POTION) = MAX(10, GET_SKILL(ch, SKILL_CREATE_POTION));
                      }
                   if (IS_SET(bits, SPELL_WAND))
                      {SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_WAND);
                       GET_SKILL(ch, SKILL_CREATE_WAND) = MAX(10, GET_SKILL(ch, SKILL_CREATE_WAND));
                      }
                   if (IS_SET(bits, SPELL_SCROLL))
                      {SET_BIT(GET_SPELL_TYPE(ch,skill_no), SPELL_SCROLL);
                       GET_SKILL(ch, SKILL_CREATE_SCROLL) = MAX(10, GET_SKILL(ch, SKILL_CREATE_SCROLL));
                      }
                  }
               found  = TRUE;
              }
          }
      if (!found)
         act("$N ������$G : '� � ���$G �� ���� ������ ����������.'",
             FALSE, ch, 0, victim, TO_CHAR);
      return (1);
     }

  act("$N ������$G ��� : '� ������� � ������ ����� �� ����$G.'",
      FALSE, ch, 0, victim, TO_CHAR);
  return (1);
}
return (0);
}


SPECIAL(horse_keeper)
{ struct char_data *victim = (struct char_data *) me, *horse = NULL;

  if (IS_NPC(ch))
     return (0);

  if (!CMD_IS("������") && !CMD_IS("horse"))
     return (0);


  skip_spaces(&argument);

  if (!*argument)
     {if (has_horse(ch,FALSE))
         {act("$N �������������$U : \"$n, ����� ���� ������ ������ ? � ���� ���� ���� ��������.\"",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      sprintf(buf,"$N ������$G : \"� ������ ���� ������� �� %d %s.\"",
              HORSE_COST, desc_count(HORSE_COST,WHAT_MONEYa));
      act(buf,FALSE,ch,0,victim,TO_CHAR);
      return (TRUE);
     }

  if (!strn_cmp(argument, "������", strlen(argument)) ||
      !strn_cmp(argument, "buy", strlen(argument)))
     {if (has_horse(ch,FALSE))
         {act("$N �������$U : \"$n, �� ������, � ���� �� ���� ������.\"",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      if (GET_GOLD(ch) < HORSE_COST)
         {act("\"������ ������, �������, � ���� ��� ����� �����!\"-������$G $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      if (!(horse = read_mobile(HORSE_VNUM,VIRTUAL)))
         {act("\"������, � ���� ��� ��� ���� �������.\"-�������� ��������$Q $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      make_horse(horse,ch);
      char_to_room(horse,IN_ROOM(ch));
      sprintf(buf,"$N �������$G %s � �����$G %s ���.",GET_PAD(horse,3),HSHR(horse));
      act(buf,FALSE,ch,0,victim,TO_CHAR);
      sprintf(buf,"$N �������$G %s � �����$G %s $n2.",GET_PAD(horse,3),HSHR(horse));
      act(buf,FALSE,ch,0,victim,TO_ROOM);
      GET_GOLD(ch) -= HORSE_COST;
      SET_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
      return (TRUE);
     }


  if (!strn_cmp(argument, "�������", strlen(argument)) ||
      !strn_cmp(argument, "sell", strlen(argument)))
     {if (!has_horse(ch,TRUE))
         {act("$N �������$U : \"$n, �� �� ������� � ��� ������.\"",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      if (on_horse(ch))
         {act("\"� �� ��������� ������� ��� � �� ��������.\"-��������$U $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      if (!(horse = get_horse(ch)) || GET_MOB_VNUM(horse) != HORSE_VNUM )
         {act("\"������, ���� ������ ��� �� ��������.\"- ������$G $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      if (IN_ROOM(horse) != IN_ROOM(victim))
         {act("\"������, ���� ������ ���-�� ������.\"- ������$G $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      extract_char(horse,FALSE);
      sprintf(buf,"$N ���������$G %s � �����$G %s � ������.",GET_PAD(horse,3),HSHR(horse));
      act(buf,FALSE,ch,0,victim,TO_CHAR);
      sprintf(buf,"$N ���������$G %s � �����$G %s � ������.",GET_PAD(horse,3),HSHR(horse));
      act(buf,FALSE,ch,0,victim,TO_ROOM);
      GET_GOLD(ch) += (HORSE_COST >> 1);
      SET_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
      return (TRUE);
     }

   return (0);
}



int npc_track(struct char_data *ch)
{ struct char_data  *vict;
  memory_rec        *names;
  int               door=BFS_ERROR, msg = FALSE, i;
  struct track_data *track;

  if (GET_REAL_INT(ch) < number(15,20))
     {for (vict = character_list; vict && door == BFS_ERROR; vict = vict->next)
          {if (CAN_SEE(ch,vict) && IN_ROOM(vict) != NOWHERE)
              for (names = MEMORY(ch); names && door == BFS_ERROR; names = names->next)
                  if (GET_IDNUM(vict) == names->id &&
                      (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
                       world[IN_ROOM(ch)].zone == world[IN_ROOM(vict)].zone
                      )
                     )
                    {for (track = world[IN_ROOM(ch)].track; track && door == BFS_ERROR; track=track->next)
                         if (track->who == GET_IDNUM(vict))
                            for  (i = 0; i < NUM_OF_DIRS; i++)
                                 if (IS_SET(track->time_outgone[i],7))
                                    {door = i;
                                     // log("MOB %s track %s at dir %d", GET_NAME(ch), GET_NAME(vict), door);
                                     break;
                                    }
                     if (!msg)
                        {msg = TRUE;
                         act("$n �����$g ����������� ������ ���-�� �����.",FALSE,ch,0,0,TO_ROOM);
                        }
                    }
          }
     }
  else
     {for (vict = character_list; vict && door == BFS_ERROR; vict = vict->next)
          if (CAN_SEE(ch,vict) && IN_ROOM(vict) != NOWHERE)
             {for (names = MEMORY(ch); names && door == BFS_ERROR; names = names->next)
                  if (GET_IDNUM(vict) == names->id &&
                      (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
                       world[IN_ROOM(ch)].zone == world[IN_ROOM(vict)].zone
                      )
                     )
                    {if (!msg)
                        {msg = TRUE;
                         act("$n �����$g ����������� ������ ���-�� �����.",FALSE,ch,0,0,TO_ROOM);
                        }
                     door = go_track(ch, vict, SKILL_TRACK);
                     // log("MOB %s sense %s at dir %d", GET_NAME(ch), GET_NAME(vict), door);
                    }
             }
     }
  return (door);
}

int item_nouse(struct obj_data *obj)
{switch (GET_OBJ_TYPE(obj))
 {
 case ITEM_LIGHT  : if (GET_OBJ_VAL(obj,2) == 0)
                       return (TRUE);
                    break;
 case ITEM_SCROLL :
 case ITEM_POTION :
                    if (!GET_OBJ_VAL(obj,1) &&
                        !GET_OBJ_VAL(obj,2) &&
                        !GET_OBJ_VAL(obj,3))
                       return (TRUE);
                    break;
 case ITEM_STAFF:
 case ITEM_WAND:    if (!GET_OBJ_VAL(obj,2))
                       return (TRUE);
                    break;
 case ITEM_OTHER:
 case ITEM_TRASH:
 case ITEM_TRAP:
 case ITEM_CONTAINER:
 case ITEM_NOTE:
 case ITEM_DRINKCON:
 case ITEM_FOOD:
 case ITEM_PEN:
 case ITEM_BOAT:
 case ITEM_FOUNTAIN:
                    return (TRUE);
                    break;
 }
 return (FALSE);
}

void npc_dropunuse(struct char_data *ch)
{struct obj_data *obj, *nobj;
 for (obj = ch->carrying; obj; obj = nobj)
     {nobj = obj->next_content;
      if (item_nouse(obj))
         {act("$n ��������$g $o3.",FALSE,ch,obj,0,FALSE);
          obj_from_char(obj);
          obj_to_room(obj,IN_ROOM(ch));
         }
     }
}



int npc_scavenge(struct char_data *ch)
{int    max = 1;
 struct obj_data *obj, *best_obj, *cont, *best_cont, *cobj;

 if (IS_SHOPKEEPER(ch))
    return (FALSE);
 npc_dropunuse(ch);
 if ( world[ch->in_room].contents && number(0, GET_REAL_INT(ch)) > 10 )
    {max      = 1;
     best_obj = NULL;
     cont     = NULL;
     best_cont= NULL;
     for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
         if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
            {if (IS_CORPSE(obj) || OBJVAL_FLAGGED(obj,CONT_LOCKED))
                continue;
             for (cobj = obj->contains; cobj; cobj = cobj->next_content)
                 if (CAN_GET_OBJ(ch, cobj) &&
                     !item_nouse(cobj)     &&
                     GET_OBJ_COST(cobj) > max)
                    {cont      = obj;
                     best_cont = best_obj = cobj;
                     max       = GET_OBJ_COST(cobj);
                    }
            }
         else
         if (!IS_CORPSE(obj)         &&
             CAN_GET_OBJ(ch, obj)    &&
             GET_OBJ_COST(obj) > max &&
             !item_nouse(obj))
            {best_obj = obj;
             max = GET_OBJ_COST(obj);
            }
     if (best_obj != NULL)
        {if (best_obj != best_cont)
            {obj_from_room(best_obj);
             obj_to_char(best_obj, ch);
             act("$n ������$g $o3.", FALSE, ch, best_obj, 0, TO_ROOM);
            }
         else
            {obj_from_obj(best_obj);
             obj_to_char(best_obj, ch);
             sprintf(buf,"$n ������$g $o3 �� %s.", cont->PNames[1]);
             act(buf,FALSE,ch,best_obj,0,TO_ROOM);
            }
        }
    }
 return (max > 1);
}

int npc_loot(struct char_data *ch)
{int    max = FALSE;
 struct obj_data *obj, *loot_obj, *next_loot, *cobj, *cnext_obj;

 if (IS_SHOPKEEPER(ch))
    return (FALSE);
 npc_dropunuse(ch);
 if (world[ch->in_room].contents && number(0, GET_REAL_INT(ch)) > 10)
    {for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
         if (CAN_SEE_OBJ(ch, obj) && IS_CORPSE(obj))
            for(loot_obj = obj->contains; loot_obj; loot_obj = next_loot)
               {next_loot = loot_obj->next_content;
                if (GET_OBJ_TYPE(loot_obj) == ITEM_CONTAINER)
                   {if (IS_CORPSE(loot_obj) || OBJVAL_FLAGGED(loot_obj,CONT_LOCKED))
                       continue;
                    for (cobj = loot_obj->contains; cobj; cobj = cnext_obj)
                        {cnext_obj = cobj->next_content;
                         if (CAN_GET_OBJ(ch, cobj) &&
                             !item_nouse(cobj))
                            {sprintf(buf,"$n �������$g $o3 �� %s.",obj->PNames[1]);
                             act(buf,FALSE,ch,cobj,0,TO_ROOM);
                             obj_from_obj(cobj);
                             obj_to_char(cobj,ch);
                             max++;
                            }
                        }
                   }
                else
                if (CAN_GET_OBJ(ch, loot_obj) &&
                    !item_nouse(loot_obj))
                   {sprintf(buf,"$n �������$g $o3 �� %s.",obj->PNames[1]);
                    act(buf,FALSE,ch,loot_obj,0,TO_ROOM);
                    obj_from_obj(loot_obj);
                    obj_to_char(loot_obj,ch);
                    max++;
                   }
               }
    }
 return (max);
}

int npc_move(struct char_data *ch, int dir, int need_specials_check)
{
  /* room_rnum was_in;
     struct    follow_type *k, *next; */
  int       need_close=FALSE, need_lock=FALSE;
  int       rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };
  struct    room_direction_data *rdata = NULL;
  int       retval = FALSE;

  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch))
     return (FALSE);
  else
  if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE)
     return (FALSE);
  else
  if (ch->master && IN_ROOM(ch) == IN_ROOM(ch->master))
     {return (FALSE);
     }
  else
  if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED))
     {if (!EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR))
         return (FALSE);
      rdata     = EXIT(ch,dir);
      if (EXIT_FLAGGED(rdata, EX_LOCKED))
         {if (has_key(ch,rdata->key) ||
              (!EXIT_FLAGGED(rdata, EX_PICKPROOF) &&
               calculate_skill(ch,SKILL_PICK,100,0) >= number(0,100)))
             {do_doorcmd(ch,0,dir,SCMD_UNLOCK);
              need_lock = TRUE;
             }
          else
             {return (FALSE);
             }
         }
       if (GET_REAL_INT(ch) >= 15 || GET_DEST(ch) != NOWHERE)
          {do_doorcmd(ch,0,dir,SCMD_OPEN);
           need_close = TRUE;
          }
     }

  perform_move(ch, dir, 1, FALSE);
  /*
  if (!ch->followers)
     retval = do_simple_move(ch, dir, need_specials_check);
  else
     {was_in = ch->in_room;
      if ((retval = do_simple_move(ch, dir, need_specials_check)))
         {for (k = ch->followers; k; k = next)
              {next = k->next;
               if (FIGHTING(k->follower)    ||
                   GET_MOB_HOLD(k->follower)||
                   GET_POS(ch) <= POS_STUNNED
                  )
               continue;
               if (k->follower->in_room == was_in)
                  {if (IS_NPC(ch))
                      {if (GET_POS(k->follower) < POS_FIGHTING)
                          {act("$n ������$u �� ����.",FALSE,k->follower,0,0,TO_ROOM);
                           GET_POS(k->follower) = POS_STANDING;
                          }
                      }
                   else
                   if (GET_POS(k->follower) < POS_STANDING))
                      continue;
                   perform_move(k->follower, dir, 1, FALSE);
                  }
              }
         }
     }
  */
  if (need_close)
     {if (retval)
         do_doorcmd(ch,0,rev_dir[dir],SCMD_CLOSE);
      else
         do_doorcmd(ch,0,dir,SCMD_CLOSE);
     }

  if (need_lock)
     {if (retval)
         do_doorcmd(ch,0,rev_dir[dir],SCMD_LOCK);
      else
         do_doorcmd(ch,0,dir,SCMD_LOCK);
     }

  return (retval);
}

int has_curse(struct obj_data *obj)
{ int i;

  for (i = 0; weapon_affect[i].aff_bitvector >= 0; i++)
      {if (!IS_SET(GET_FLAG(obj->obj_flags.affects,weapon_affect[i].aff_pos),
            weapon_affect[i].aff_pos) ||
           weapon_affect[i].aff_spell <= 0)
          continue;
       if (IS_SET(spell_info[weapon_affect[i].aff_spell].routines,NPC_AFFECT_PC|NPC_DAMAGE_PC))
          return (TRUE);
      }
  return (FALSE);
}

int calculate_weapon_class(struct char_data *ch, struct obj_data *weapon)
{ int damage=0, hits=0, i;

  if (!weapon || GET_OBJ_TYPE(weapon) != ITEM_WEAPON)
     return (0);

  hits   = calculate_skill(ch,GET_OBJ_SKILL(weapon),101,0);
  damage = (GET_OBJ_VAL(weapon,1) + 1) * (GET_OBJ_VAL(weapon,2)) / 2;
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
      {if (weapon->affected[i].location == APPLY_DAMROLL)
          damage += weapon->affected[i].modifier;
       if (weapon->affected[i].location == APPLY_HITROLL)
          hits   += weapon->affected[i].modifier * 10;
      }

  if (has_curse(weapon))
     return (0);

  return (damage + (hits > 200 ? 10 : hits/20));
}

void best_weapon(struct char_data *ch, struct obj_data *sweapon, struct obj_data **dweapon)
{ if (*dweapon == NULL)
     {if (calculate_weapon_class(ch,sweapon) > 0)
         *dweapon = sweapon;
     }
  else
  if (calculate_weapon_class(ch,sweapon) > calculate_weapon_class(ch,*dweapon))
     *dweapon = sweapon;
}

void npc_wield(struct char_data *ch)
{struct obj_data *obj, *next, *right=NULL, *left=NULL, *both=NULL;

 if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
    return;

 if (GET_EQ(ch,WEAR_HOLD) && GET_OBJ_TYPE(GET_EQ(ch,WEAR_HOLD)) == ITEM_WEAPON)
    left  = GET_EQ(ch,WEAR_HOLD);
 if (GET_EQ(ch,WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(ch,WEAR_WIELD)) == ITEM_WEAPON)
    right = GET_EQ(ch,WEAR_WIELD);
 if (GET_EQ(ch,WEAR_BOTHS) && GET_OBJ_TYPE(GET_EQ(ch,WEAR_BOTHS)) == ITEM_WEAPON)
    both = GET_EQ(ch,WEAR_BOTHS);

 if (GET_REAL_INT(ch) < 15 &&
     ((left && right) || (both)))
    return;

 for (obj = ch->carrying; obj; obj = next)
     {next = obj->next_content;
      if (GET_OBJ_TYPE(obj) != ITEM_WEAPON)
         continue;
      if (CAN_WEAR(obj, ITEM_WEAR_HOLD) && OK_HELD(ch,obj))
         best_weapon(ch,obj,&left);
      else
      if (CAN_WEAR(obj, ITEM_WEAR_WIELD) && OK_WIELD(ch,obj))
         best_weapon(ch,obj,&right);
      else
      if (CAN_WEAR(obj, ITEM_WEAR_BOTHS) && OK_BOTH(ch,obj))
         best_weapon(ch,obj,&both);
     }

 // if (isname("�����",GET_NAME(ch)))
 //    log("%s - %d, %s - %d, %s - %d",
 //        both  ? both->PNames[0] : "Empty", calculate_weapon_class(ch,both),
 //        right ? right->PNames[0] : "Empty", calculate_weapon_class(ch,right),
 //        left  ? left->PNames[0] : "Empty", calculate_weapon_class(ch,left));

 if (both && calculate_weapon_class(ch,both) > calculate_weapon_class(ch,left) +
                                               calculate_weapon_class(ch,right))
    {if (both == GET_EQ(ch,WEAR_BOTHS))
        return;
     if (GET_EQ(ch,WEAR_BOTHS))
        {act("$n ���������$g ������������ $o3.",FALSE,ch,GET_EQ(ch,WEAR_BOTHS),0,TO_ROOM);
         obj_to_char(unequip_char(ch,WEAR_BOTHS), ch);
        }
     if (GET_EQ(ch,WEAR_WIELD))
        {act("$n ���������$g ������������ $o3.",FALSE,ch,GET_EQ(ch,WEAR_WIELD),0,TO_ROOM);
         obj_to_char(unequip_char(ch,WEAR_WIELD), ch);
        }
     if (GET_EQ(ch,WEAR_SHIELD))
        {act("$n ���������$g ������������ $o3.",FALSE,ch,GET_EQ(ch,WEAR_SHIELD),0,TO_ROOM);
         obj_to_char(unequip_char(ch,WEAR_SHIELD),ch);
        }
     if (GET_EQ(ch,WEAR_HOLD))
        {act("$n ���������$g ������������ $o3.",FALSE,ch,GET_EQ(ch,WEAR_HOLD),0,TO_ROOM);
         obj_to_char(unequip_char(ch,WEAR_HOLD),ch);
        }
     act("$n ����$g $o3 � ��� ����.",FALSE,ch,both,0,TO_ROOM);
     obj_from_char(both);
     equip_char(ch,both,WEAR_BOTHS);
    }
 else
    {if (left && GET_EQ(ch,WEAR_HOLD) != left)
        {if (GET_EQ(ch,WEAR_BOTHS))
            {act("$n ���������$g ������������ $o3.",FALSE,ch,GET_EQ(ch,WEAR_BOTHS),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_BOTHS), ch);
            }
         if (GET_EQ(ch,WEAR_SHIELD))
            {act("$n ���������$g ������������ $o3.",FALSE,ch,GET_EQ(ch,WEAR_SHIELD),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_SHIELD),ch);
            }
         if (GET_EQ(ch,WEAR_HOLD))
            {act("$n ���������$g ������������ $o3.",FALSE,ch,GET_EQ(ch,WEAR_HOLD),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_HOLD),ch);
            }
         act("$n ����$g $o3 � ����� ����.",FALSE,ch,left,0,TO_ROOM);
         obj_from_char(left);
         equip_char(ch,left,WEAR_HOLD);
        }
     if (right && GET_EQ(ch,WEAR_WIELD) != right)
        {if (GET_EQ(ch,WEAR_BOTHS))
            {act("$n ���������$g ������������ $o3.",FALSE,ch,GET_EQ(ch,WEAR_BOTHS),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_BOTHS), ch);
            }
         if (GET_EQ(ch,WEAR_WIELD))
            {act("$n ���������$g ������������ $o3.",FALSE,ch,GET_EQ(ch,WEAR_WIELD),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_WIELD), ch);
            }
         act("$n ����$g $o3 � ������ ����.",FALSE,ch,right,0,TO_ROOM);
         obj_from_char(right);
         equip_char(ch,right,WEAR_WIELD);
        }
    }
}

void npc_armor(struct char_data *ch)
{struct obj_data *obj, *next;
 int    where=0;

 if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
    return;

 for (obj = ch->carrying; obj; obj = next)
     {next = obj->next_content;
      if (GET_OBJ_TYPE(obj) != ITEM_ARMOR)
          continue;
      if (CAN_WEAR(obj, ITEM_WEAR_FINGER))      where = WEAR_FINGER_R;
      if (CAN_WEAR(obj, ITEM_WEAR_NECK))        where = WEAR_NECK_1;
      if (CAN_WEAR(obj, ITEM_WEAR_BODY))        where = WEAR_BODY;
      if (CAN_WEAR(obj, ITEM_WEAR_HEAD))        where = WEAR_HEAD;
      if (CAN_WEAR(obj, ITEM_WEAR_LEGS))        where = WEAR_LEGS;
      if (CAN_WEAR(obj, ITEM_WEAR_FEET))        where = WEAR_FEET;
      if (CAN_WEAR(obj, ITEM_WEAR_HANDS))       where = WEAR_HANDS;
      if (CAN_WEAR(obj, ITEM_WEAR_ARMS))        where = WEAR_ARMS;
      if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))      where = WEAR_SHIELD;
      if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))       where = WEAR_ABOUT;
      if (CAN_WEAR(obj, ITEM_WEAR_WAIST))       where = WEAR_WAIST;
      if (CAN_WEAR(obj, ITEM_WEAR_WRIST))       where = WEAR_WRIST_R;

      if (!where)
         continue;

      if ((where == WEAR_FINGER_R) ||
          (where == WEAR_NECK_1) ||
          (where == WEAR_WRIST_R))
         if (GET_EQ(ch, where))
            where++;
      if (where == WEAR_SHIELD &&
          (GET_EQ(ch,WEAR_BOTHS) || GET_EQ(ch,WEAR_HOLD)))
         continue;
      if (GET_EQ(ch, where))
         {if (GET_REAL_INT(ch) < 15)
             continue;
          if (GET_OBJ_VAL(obj,0) + GET_OBJ_VAL(obj,1) * 3 <=
              GET_OBJ_VAL(GET_EQ(ch,where),0) + GET_OBJ_VAL(GET_EQ(ch,where),1) * 3 ||
              has_curse(obj))
             continue;
          act("$n ���������$g ������������ $o3.",FALSE,ch,GET_EQ(ch,where),0,TO_ROOM);
          obj_to_char(unequip_char(ch,where),ch);
         }
      act("$n ����$g $o3.",FALSE,ch,obj,0,TO_ROOM);
      obj_from_char(obj);
      equip_char(ch,obj,where);
      break;
     }
}

void npc_light(struct char_data *ch)
{struct obj_data *obj, *next;

 if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
    return;

 if (AFF_FLAGGED(ch, AFF_INFRAVISION))
    return;

 if ((obj=GET_EQ(ch,WEAR_LIGHT)) &&
     (GET_OBJ_VAL(obj,2) == 0 || !IS_DARK(IN_ROOM(ch))))
    {act("$n ���������$g ������������ $o3.",FALSE,ch,obj,0,TO_ROOM);
     obj_to_char(unequip_char(ch,WEAR_LIGHT),ch);
    }

 if (!GET_EQ(ch,WEAR_LIGHT) && IS_DARK(IN_ROOM(ch)))
    for (obj = ch->carrying; obj; obj = next)
        {next = obj->next_content;
         if (GET_OBJ_TYPE(obj) != ITEM_LIGHT)
             continue;
         if (GET_OBJ_VAL(obj,2) == 0)
            {act("$n ��������$g $o3.",FALSE,ch,obj,0,TO_ROOM);
             obj_from_char(obj);
             obj_to_room(obj,IN_ROOM(ch));
             continue;
            }
         act("$n �����$g ������������ $o3.",FALSE,ch,obj,0,TO_ROOM);
         obj_from_char(obj);
         equip_char(ch,obj,WEAR_LIGHT);
         return;
        }
}


int npc_battle_scavenge(struct char_data *ch)
{int    max = FALSE;
 struct obj_data *obj, *next_obj=NULL;

 if (IS_SHOPKEEPER(ch))
    return (FALSE);

 if ( world[IN_ROOM(ch)].contents && number(0, GET_REAL_INT(ch)) > 10 )
    for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj)
        {next_obj = obj->next_content;
         if (CAN_GET_OBJ(ch, obj) &&
             !has_curse(obj) &&
             (GET_OBJ_TYPE(obj) == ITEM_ARMOR ||
              GET_OBJ_TYPE(obj) == ITEM_WEAPON))
            {obj_from_room(obj);
             obj_to_char(obj, ch);
             act("$n ������$g $o3.", FALSE, ch, obj, 0, TO_ROOM);
             max = TRUE;
            }
        }
 return (max);
}


int npc_walk(struct char_data *ch)
{ int  rnum, door=BFS_ERROR;

  if (GET_DEST(ch) == NOWHERE || (rnum = real_room(GET_DEST(ch))) == NOWHERE)
     return (BFS_ERROR);

  if (IN_ROOM(ch) == rnum)
     {if (ch->mob_specials.dest_count == 1)
         return (NUM_OF_DIRS);
      if (ch->mob_specials.dest_pos == ch->mob_specials.dest_count - 1 &&
          ch->mob_specials.dest_dir >= 0)
         ch->mob_specials.dest_dir = -1;
      if (!ch->mob_specials.dest_pos && ch->mob_specials.dest_dir < 0)
         ch->mob_specials.dest_dir = 0;
      ch->mob_specials.dest_pos += ch->mob_specials.dest_dir >= 0 ? 1 : -1;
      if ((rnum = real_room(GET_DEST(ch))) == NOWHERE || rnum == IN_ROOM(ch))
         return (BFS_ERROR);
      else
         return (npc_walk(ch));
     }

  door = find_first_step(ch->in_room,real_room(GET_DEST(ch)),ch);
  // log("MOB %s walk to room %d at dir %d", GET_NAME(ch), GET_DEST(ch), door);

  return (door);
}

void do_npc_steal(struct char_data * ch, struct char_data * victim)
{ struct obj_data *obj, *best = NULL;
  int    gold;

  if (IS_NPC(victim) || IS_SHOPKEEPER(ch) || FIGHTING(victim))
     return;

  if (GET_LEVEL(victim) >= LVL_IMMORT)
     return;
     
  if (!CAN_SEE(ch,victim))
     return;
     
  if (AWAKE(victim) &&
      (number(0, MAX(0,GET_LEVEL(ch) - int_app[GET_REAL_INT(victim)].observation)) == 0))
     {act("�� ���������� ���� $n1 � ����� �������.", FALSE, ch, 0, victim, TO_VICT);
      act("$n �����$u ��������� $N1.", TRUE, ch, 0, victim, TO_NOTVICT);
     }
  else
     {/* Steal some gold coins          */
      gold = (int) ((GET_GOLD(victim) * number(1, 10)) / 100);
      if (gold > 0)
         {GET_GOLD(ch)     += gold;
          GET_GOLD(victim) -= gold;
         }
      /* Steal something from equipment */
      if (calculate_skill(ch,SKILL_STEAL,100,victim) >=
          number(1,100) - (AWAKE(victim) ? 100 : 0))
         {for (obj = victim->carrying; obj; obj = obj->next_content)
              if (CAN_SEE_OBJ(ch,obj) &&
	          (!best || GET_OBJ_COST(obj) > GET_OBJ_COST(best))
		 )
                 best = obj;
          if (best)
             {obj_from_char(best);
              obj_to_char(best,ch);
             }
         }
     }
}

void npc_steal(struct char_data *ch)
{
  struct char_data *cons;

  if (GET_POS(ch) != POS_STANDING || IS_SHOPKEEPER(ch) || FIGHTING(ch))
     return;

  for (cons = world[ch->in_room].people; cons; cons = cons->next_in_room)
      if (!IS_NPC(cons) && !IS_IMMORTAL(cons) && (number(0, GET_REAL_INT(ch)) > 10))
         {do_npc_steal(ch, cons);
          return;
         }
  return;
}

#define ZONE(ch)  (GET_MOB_VNUM(ch) / 100)
#define GROUP(ch) ((GET_MOB_VNUM(ch) % 100) / 10)

void npc_group(struct char_data *ch)
{struct char_data *vict, *leader=NULL;
 int    zone = ZONE(ch), group = GROUP(ch), members = 0;

 if (GET_DEST(ch) == NOWHERE)
    return;

 if (ch->master &&
     IN_ROOM(ch) == IN_ROOM(ch->master)
    )
    leader = ch->master;

 if (!ch->master)
    leader = ch;

 if (leader &&
     (AFF_FLAGGED(leader, AFF_CHARM) ||
      GET_POS(leader) < POS_SLEEPING
     )
    )
    leader = NULL;

 // Find leader
 for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
     {if (!IS_NPC(vict)                  ||
          GET_DEST(vict) != GET_DEST(ch) ||
          zone != ZONE(vict)             ||
	  group != GROUP(vict)           ||
          AFF_FLAGGED(vict, AFF_CHARM)   ||
	  GET_POS(vict) < POS_SLEEPING
	 )
        continue;
      members++;
      if (!leader || GET_REAL_INT(vict) > GET_REAL_INT(leader))
         {leader = vict;
	 }
     }

 if (members <= 1)
    {if (ch->master)
        stop_follower(ch, SF_EMPTY);
     return;
    }

if (leader->master)
   {stop_follower(leader, SF_EMPTY);
   }

 // Assign leader
 for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
     {if (!IS_NPC(vict)                  ||
          GET_DEST(vict) != GET_DEST(ch) ||
          zone != ZONE(vict)             ||
	  group != GROUP(vict)           ||
          AFF_FLAGGED(vict, AFF_CHARM)   ||
	  GET_POS(vict) < POS_SLEEPING
	 )
         continue;
      if (vict == leader)
         {SET_BIT(AFF_FLAGS(vict,AFF_GROUP), AFF_GROUP);
          continue;
         }
      if (!vict->master)
         add_follower(vict,leader);
      else
      if (vict->master != leader)
         {stop_follower(vict,SF_EMPTY);
          add_follower(vict,leader);
         }
      SET_BIT(AFF_FLAGS(vict,AFF_GROUP), AFF_GROUP);
     }

}

void npc_groupbattle(struct char_data *ch)
{struct follow_type *k;
 struct char_data   *tch, *helper;

 if (!IS_NPC(ch) ||
     !FIGHTING(ch) ||
     AFF_FLAGGED(ch,AFF_CHARM) ||
     !ch->master ||
     !ch->followers)
    return;

 k   = ch->master ? ch->master->followers : ch->followers;
 tch = ch->master ? ch->master : ch;
 for (;k; (k = tch ? k : k->next), tch = NULL)
     {helper = tch ? tch : k->follower;
      if (IN_ROOM(ch) == IN_ROOM(helper) &&
          !FIGHTING(helper) &&
          !IS_NPC(helper) &&
          GET_POS(helper) > POS_STUNNED)
         {GET_POS(helper) = POS_STANDING;
          set_fighting(helper, FIGHTING(ch));
          act("$n �������$u �� $N3.",FALSE,helper,0,ch,TO_ROOM);
         }
     }
}


SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents)
      {act("$p ��������$U � ���� !", FALSE, 0, k, 0, TO_ROOM);
       extract_obj(k);
      }

  if (!CMD_IS("drop") || !CMD_IS("�������"))
     return (0);

  do_drop(ch, argument, cmd, 0);

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents)
      {act("$p ��������$U � ���� !", FALSE, 0, k, 0, TO_ROOM);
       value += MAX(1, MIN(1, GET_OBJ_COST(k) / 10));
       extract_obj(k);
      }

  if (value)
     {send_to_char("���� ������� ���� ������.\r\n", ch);
      act("$n ������$y ������.", TRUE, ch, 0, 0, TO_ROOM);
      if (GET_LEVEL(ch) < 3)
         gain_exp(ch, value);
      else
         GET_GOLD(ch) += value;
     }
  return (1);
}


SPECIAL(mayor)
{
  const char open_path[] =
	"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
  const char close_path[] =
	"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

  static const char *path = NULL;
  static int index;
  static bool move = FALSE;

  if (!move)
     {if (time_info.hours == 6)
         {move = TRUE;
          path = open_path;
          index = 0;
         }
      else
      if (time_info.hours == 20)
         {move = TRUE;
          path = close_path;
          index = 0;
         }
     }
  if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) ||
      (GET_POS(ch) == POS_FIGHTING))
     return (FALSE);

  switch (path[index])
  {
  case '0':
  case '1':
  case '2':
  case '3':
    perform_move(ch, path[index] - '0', 1, FALSE);
    break;

  case 'W':
    GET_POS(ch) = POS_STANDING;
    act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    GET_POS(ch) = POS_SLEEPING;
    act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'a':
    act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'b':
    act("$n says 'What a view!  I must get something done about that dump!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'c':
    act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'd':
    act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'e':
    act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'E':
    act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'O':
    do_gen_door(ch, "gate", 0, SCMD_UNLOCK);
    do_gen_door(ch, "gate", 0, SCMD_OPEN);
    break;

  case 'C':
    do_gen_door(ch, "gate", 0, SCMD_CLOSE);
    do_gen_door(ch, "gate", 0, SCMD_LOCK);
    break;

  case '.':
    move = FALSE;
    break;

  }

  index++;
  return (FALSE);
}


/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */



SPECIAL(snake)
{
  if (cmd)
     return (FALSE);

  if (GET_POS(ch) != POS_FIGHTING)
     return (FALSE);

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(0, 42 - GET_LEVEL(ch)) == 0))
     {act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
      act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
      call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
      return (TRUE);
     }
  return (FALSE);
}


SPECIAL(thief)
{
  struct char_data *cons;

  if (cmd)
    return (FALSE);

  if (GET_POS(ch) != POS_STANDING)
     return (FALSE);

  for (cons = world[ch->in_room].people; cons; cons = cons->next_in_room)
      if (!IS_NPC(cons) && (GET_LEVEL(cons) < LVL_IMMORT) && (!number(0, 4)))
         {do_npc_steal(ch, cons);
          return (TRUE);
         }
  return (FALSE);
}


SPECIAL(magic_user)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
     return (FALSE);

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
      if (FIGHTING(vict) == ch && !number(0, 4))
         break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
     vict = FIGHTING(ch);

  /* Hm...didn't pick anyone...I'll wait a round. */
  if (vict == NULL)
     return (TRUE);

  if ((GET_LEVEL(ch) > 13) && (number(0, 10) == 0))
     cast_spell(ch, vict, NULL, SPELL_SLEEP);

  if ((GET_LEVEL(ch) > 7) && (number(0, 8) == 0))
     cast_spell(ch, vict, NULL, SPELL_BLINDNESS);

  if ((GET_LEVEL(ch) > 12) && (number(0, 12) == 0))
     {if (IS_EVIL(ch))
         cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN);
      else
      if (IS_GOOD(ch))
         cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
     }
  if (number(0, 4))
    return (TRUE);

  switch (GET_LEVEL(ch))
  {
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
    break;
  case 6:
  case 7:
    cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
    break;
  case 8:
  case 9:
    cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
    break;
  case 10:
  case 11:
    cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
    break;
  case 12:
  case 13:
    cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
    break;
  case 14:
  case 15:
  case 16:
  case 17:
    cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
    break;
  default:
    cast_spell(ch, vict, NULL, SPELL_FIREBALL);
    break;
  }
  return (TRUE);

}


/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

SPECIAL(guild_guard)
{
  int i;
  struct char_data *guard = (struct char_data *) me;
  const char *buf =  "�������� ��������� ���, ��������� ������.\r\n";
  const char *buf2 = "�������� ��������� $n, ��������� $m ������.";

  if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND) || AFF_FLAGGED(guard, AFF_HOLD))
     return (FALSE);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
     return (FALSE);

  for (i = 0; guild_info[i][0] != -1; i++)
      {if ((IS_NPC(ch) || GET_CLASS(ch) != guild_info[i][0]) &&
 	        GET_ROOM_VNUM(IN_ROOM(ch)) == guild_info[i][1] &&
	        cmd == guild_info[i][2])
	      {send_to_char(buf, ch);
           act(buf2, FALSE, ch, 0, 0, TO_ROOM);
           return (TRUE);
          }
      }

  return (FALSE);
}



SPECIAL(puff)
{
  if (cmd)
    return (0);

  switch (number(0, 60))
  {
  case 0:
    do_say(ch, "My god!  It's full of stars!", 0, 0);
    return (1);
  case 1:
    do_say(ch, "How'd all those fish get up here?", 0, 0);
    return (1);
  case 2:
    do_say(ch, "I'm a very female dragon.", 0, 0);
    return (1);
  case 3:
    do_say(ch, "I've got a peaceful, easy feeling.", 0, 0);
    return (1);
  default:
    return (0);
  }
}



SPECIAL(fido)
{

  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content)
      {if (IS_CORPSE(i))
          {act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
           for (temp = i->contains; temp; temp = next_obj)
               {next_obj = temp->next_content;
  	            obj_from_obj(temp);
	            obj_to_room(temp, ch->in_room);
               }
           extract_obj(i);
           return (TRUE);
          }
      }
  return (FALSE);
}



SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
     return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content)
      {if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
          continue;
      if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
          continue;
      act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
      obj_from_room(i);
      obj_to_char(i, ch);
      return (TRUE);
     }

  return (FALSE);
}


SPECIAL(cityguard)
{
  struct char_data *tch, *evil;
  int max_evil;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  max_evil = 1000;
  evil = 0;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_KILLER))
          {act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
           hit(ch, tch, TYPE_UNDEFINED, 1);
           return (TRUE);
          }
      }

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_THIEF))
          {act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
           hit(ch, tch, TYPE_UNDEFINED, 1);
           return (TRUE);
          }
      }

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {if (CAN_SEE(ch, tch) && FIGHTING(tch))
          {if ((GET_ALIGNMENT(tch) < max_evil) &&
	           (IS_NPC(tch) || IS_NPC(FIGHTING(tch))))
	          {max_evil = GET_ALIGNMENT(tch);
	           evil = tch;
              }
      }
  }

  if (evil && (GET_ALIGNMENT(FIGHTING(evil)) >= 0))
     {act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, evil, TYPE_UNDEFINED, 1);
      return (TRUE);
     }
  return (FALSE);
}


#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH], pet_name[256];
  room_rnum pet_room;
  struct char_data *pet;

  pet_room = ch->in_room + 1;

  if (CMD_IS("list"))
     {send_to_char("Available pets are:\r\n", ch);
      for (pet = world[pet_room].people; pet; pet = pet->next_in_room)
          {sprintf(buf, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
           send_to_char(buf, ch);
          }
      return (TRUE);
     }
  else
  if (CMD_IS("buy"))
     {two_arguments(argument, buf, pet_name);

      if (!(pet = get_char_room(buf, pet_room)))
         {send_to_char("There is no such pet!\r\n", ch);
          return (TRUE);
         }
      if (GET_GOLD(ch) < PET_PRICE(pet))
         {send_to_char("You don't have enough gold!\r\n", ch);
          return (TRUE);
         }
      GET_GOLD(ch) -= PET_PRICE(pet);

      pet = read_mobile(GET_MOB_RNUM(pet), REAL);
      GET_EXP(pet) = 0;
      SET_BIT(AFF_FLAGS(pet, AFF_CHARM), AFF_CHARM);

      if (*pet_name)
         {sprintf(buf, "%s %s", pet->player.name, pet_name);
          /* free(pet->player.name); don't free the prototype! */
          pet->player.name = str_dup(buf);

          sprintf(buf, "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
	      pet->player.description, pet_name);
          /* free(pet->player.description); don't free the prototype! */
          pet->player.description = str_dup(buf);
         }
      char_to_room(pet, ch->in_room);
      add_follower(pet, ch);
      load_mtrigger(pet);

      /* Be certain that pets can't get/carry/use/wield/wear items */
      IS_CARRYING_W(pet) = 1000;
      IS_CARRYING_N(pet) = 100;

      send_to_char("May you enjoy your pet.\r\n", ch);
      act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

      return (1);
     }
  /* All commands except list and buy */
  return (0);
}



/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */


SPECIAL(bank)
{
  int amount;

  if (CMD_IS("balance") || CMD_IS("������") || CMD_IS("������"))
     {if (GET_BANK_GOLD(ch) > 0)
         sprintf(buf, "� ��� �� ����� %ld %s.\r\n",
	             GET_BANK_GOLD(ch), desc_count(GET_BANK_GOLD(ch),WHAT_MONEYa));
      else
         sprintf(buf, "� ��� ��� �����.\r\n");
      send_to_char(buf, ch);
      return (1);
     }
  else
  if (CMD_IS("deposit") || CMD_IS("�������") || CMD_IS("�����"))
     {if ((amount = atoi(argument)) <= 0)
         {send_to_char("������� �� ������ ������� ?\r\n", ch);
          return (1);
         }
      if (GET_GOLD(ch) < amount)
         {send_to_char("� ����� ����� �� ������ ������ ������� !\r\n", ch);
          return (1);
         }
      GET_GOLD(ch) -= amount;
      GET_BANK_GOLD(ch) += amount;
      sprintf(buf, "�� ������� %d %s.\r\n", amount, desc_count(amount,WHAT_MONEYu));
      send_to_char(buf, ch);
      act("$n ��������$g ���������� ��������.", TRUE, ch, 0, FALSE, TO_ROOM);
      return (1);
     }
  else
  if (CMD_IS("withdraw") || CMD_IS("��������"))
     {if ((amount = atoi(argument)) <= 0)
         {send_to_char("�������� ���������� �����, ������� �� ������ �������� ?\r\n", ch);
          return (1);
         }
      if (GET_BANK_GOLD(ch) < amount)
         {send_to_char("�� �� �������� ������� ����� �� ������ !\r\n", ch);
          return (1);
         }
      GET_GOLD(ch) += amount;
      GET_BANK_GOLD(ch) -= amount;
      sprintf(buf, "�� ����� %d %s.\r\n", amount, desc_count(amount,WHAT_MONEYu));
      send_to_char(buf, ch);
      act("$n ��������$g ���������� ��������.", TRUE, ch, 0, FALSE, TO_ROOM);
      return (1);
     }
  else
    return (0);
}
