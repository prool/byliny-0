/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <unistd.h> // prool

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */



#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "handler.h"
#include "constants.h"

void _exit(int); // prool

extern int     siteok_everyone;
extern struct  spell_create_type spell_create[];
extern

/* local functions */
int  parse_class(char arg);
int  parse_race(char arg);
long find_class_bitvector(char arg);
byte saving_throws(int class_num, int type, int level);
int  thaco(int class_num, int level);
void roll_real_abils(struct char_data * ch);
void do_start(struct char_data * ch, int newbie);
int  backstab_mult(int level);
int  invalid_anti_class(struct char_data *ch, struct obj_data *obj);
int  invalid_no_class(struct char_data *ch, struct obj_data *obj);
int  level_exp(int chclass, int level);
const char *title_male(int chclass, int level);
const char *title_female(int chclass, int level);
byte extend_saving_throws(int class_num, int type, int level);

/* Names first */

const char *class_abbrevs[] =
{ "Ле",
  "Ко",
  "Та",
  "Бо",
  "На",
  "Др",
  "Ку",
  "Во",
  "Че",
  "Ви",
  "Ох",
  "Кз",
  "Кп",
  "Вл",
  "\n"
};


const char *pc_class_types[] =
{ "Лекарь",
  "Колдун",
  "Тать",
  "Богатырь",
  "Наемник",
  "Дружинник",
  "Кудесник",
  "Волшебник",
  "Чернокнижник",
  "Витязь",
  "Охотник",
  "Кузнец",
  "Купец",
  "Волхв",
  "\n"
};


/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
"\r\n"
"Выберите профессию :\r\n"
"  [Л]екарь\r\n"
"  [К]олдун\r\n"
"  [Т]ать\r\n"
"  [Б]огатырь\r\n"
"  [Н]аемник\r\n"
"  [Д]ружинник\r\n"
"  К[у]десник\r\n"
"  [В]олшебник\r\n"
"  [Ч]ернокнижник\r\n"
"  В[и]тязь\r\n"
"  [О]хотник\r\n"
"  Ку[з]нец\r\n"
"  Ку[п]ец\r\n"
"  Вол[x]в\r\n";


/* The menu for choosing a religion in interpreter.c: */
const char *religion_menu =
"\r\n"
"Какой религии Вы отдаете предпочтение :\r\n"
"  Я[з]ычество\r\n"
"  [Х]ристианство\r\n";

/* The menu for choosing a race in interpreter.c: */
const char *race_menu =
"\r\n"
"Какой РОД Вам ближе всего по духу :\r\n"
"  [С]еверяне\r\n"
"  [П]оляне\r\n"
"  [К]ривичи\r\n"
"  [В]ятичи\r\n"
"  В[е]лыняне\r\n"
"  [Д]ревляне\r\n"
;

const char *race_types[] =
{"северяне",
 "поляне",
 "кривичи",
 "вятичи",
 "велыняне",
 "древляне"
};


/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(char arg)
{
  arg = LOWER(arg);

  switch (arg)
  {case 'л': return CLASS_CLERIC;
   case 'к': return CLASS_MAGIC_USER;
   case 'т': return CLASS_THIEF;
   case 'б': return CLASS_WARRIOR;
   case 'н': return CLASS_ASSASINE;
   case 'д': return CLASS_GUARD;
	case 'у': return CLASS_DEFENDERMAGE;
   case 'в': return CLASS_CHARMMAGE;
   case 'ч': return CLASS_NECROMANCER;
   case 'и': return CLASS_PALADINE;
   case 'о': return CLASS_RANGER;
   case 'з': return CLASS_SMITH;
   case 'п': return CLASS_MERCHANT;
   case 'х': return CLASS_DRUID;
   default:  return CLASS_UNDEFINED;
  }
}


int parse_race(char arg)
{
  arg = LOWER(arg);

  switch (arg)
  {
  case 'с': return CLASS_SEVERANE;
  case 'п': return CLASS_POLANE;
  case 'к': return CLASS_KRIVICHI;
  case 'в': return CLASS_VATICHI;
  case 'е': return CLASS_VELANE;
  case 'д': return CLASS_DREVLANE;
  default:  return CLASS_UNDEFINED;
  }
}

/*
 * bitvectors (i.e., powers of two) for each class, mainly for use in
 * do_who and do_users.  Add new classes at the end so that all classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.
 */

long find_class_bitvector(char arg)
{
  arg = LOWER(arg);

  switch (arg)
  { case 'л': return (1 << CLASS_CLERIC);
    case 'к': return (1 << CLASS_MAGIC_USER);
    case 'т': return (1 << CLASS_THIEF);
	 case 'б': return (1 << CLASS_WARRIOR);
    case 'н': return (1 << CLASS_ASSASINE);
    case 'д': return (1 << CLASS_GUARD);
    case 'у': return (1 << CLASS_DEFENDERMAGE);
    case 'в': return (1 << CLASS_CHARMMAGE);
    case 'ч': return (1 << CLASS_NECROMANCER);
    case 'и': return (1 << CLASS_PALADINE);
    case 'о': return (1 << CLASS_RANGER);
    case 'з': return (1 << CLASS_SMITH);
    case 'п': return (1 << CLASS_MERCHANT);
    case 'х': return (1 << CLASS_DRUID);
    default:  return 0;
  }
}


/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 *
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 *
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

#define SPELL	0
#define SKILL	1

/* #define LEARNED_LEVEL	0  % known which is considered "learned" */
/* #define MAX_PER_PRAC		1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC		2  min percent gain in skill per practice */
/* #define PRAC_TYPE		3  should it say 'spell' or 'skill'?	*/

int prac_params[4][NUM_CLASSES] =
{ /* MAG	CLE		THE		WAR */
  {95,          95,             85,             80},/* learned level */
  {100,         100,            12,             12},/* max per prac */
  {25,          25,             0,		0,},/* min per pac */
  {SPELL,       SPELL,          SKILL,          SKILL}/* prac name */
};


/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.c if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. If you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here.
 */
int guild_info[][3] = {

/* Midgaard */
  {CLASS_MAGIC_USER,	3017,	SCMD_SOUTH},
  {CLASS_CLERIC,	    3004,	SCMD_NORTH},
  {CLASS_THIEF,		    3027,	SCMD_EAST},
  {CLASS_WARRIOR,	    3021,	SCMD_EAST},

/* Brass Dragon */
  {-999 /* all */ ,	5065,	SCMD_WEST},

/* this must go last -- add new guards above! */
  {-1, -1, -1}
};



/*
 * Saving throws for:
 * MCTW
 *   PARA, ROD, PETRI, BREATH, SPELL
 *     Levels 0-40
 *
 * Do not forget to change extern declaration in magic.c if you add to this.
 */


byte saving_throws(int class_num, int type, int level)
{ return extend_saving_throws(class_num,type,level);
}

byte extend_saving_throws(int class_num, int type, int level)
{
  switch (class_num)
  {case CLASS_MAGIC_USER:
	 switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  81;
		 case 15: return  79;
		 case 16: return  78;
		 case 17: return  75;
		 case 18: return  73;
		 case 19: return  71;
		 case 20: return  68;
		 case 21: return  65;
		 case 22: return  62;
		 case 23: return  59;
		 case 24: return  56;
		 case 25: return  52;
		 case 26: return  48;
		 case 27: return  44;
		 case 28: return  40;
		 case 29: return  35;
		 case 30: return  30;
		 default: return 	0;
		break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
		 default: return  0;
		break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  81;
		 case 15: return  79;
		 case 16: return  78;
		 case 17: return  75;
		 case 18: return  73;
		 case 19: return  71;
		 case 20: return  68;
		 case 21: return  65;
		 case 22: return  62;
		 case 23: return  59;
		 case 24: return  56;
		 case 25: return  52;
		 case 26: return  48;
		 case 27: return  44;
		 case 28: return  40;
		 case 29: return  35;
		 case 30: return  30;
		 default: return 	0;
		 break;
		}
	 case SAVING_BREATH:	// Breath weapons
		switch (level)
		{case  0: return 90;
		 case  1: return 75;
		 case  2: return 73;
		 case  3: return 71;
		 case  4: return 69;
		 case  5: return 67;
		 case  6: return 65;
		 case  7: return 63;
		 case  8: return 61;
		 case  9: return 60;
		 case 10: return 59;
		 case 11: return 57;
		 case 12: return 55;
		 case 13: return 53;
		 case 14: return 51;
		 case 15: return 50;
		 case 16: return 49;
		 case 17: return 47;
		 case 18: return 45;
		 case 19: return 43;
		 case 20: return 41;
		 case 21: return 40;
		 case 22: return 39;
       case 23: return 37;
       case 24: return 35;
       case 25: return 33;
       case 26: return 31;
       case 27: return 29;
       case 28: return 27;
		 case 29: return 25;
		 case 30: return 23;
		 default: return 0;
		 break;
		}
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 default: return  0;
		break;
		}
	 case SAVING_BASIC:  // Protect from skills
		switch (level)
		{case  0: return  100;
		 case  1: return  100;
		 case  2: return  100;
		 case  3: return  100;
		 case  4: return  100;
		 case  5: return  100;
		 case  6: return  100;
		 case  7: return  100;
		 case  8: return  100;
		 case  9: return  100;
		 case 10: return  100;
		 case 11: return  100;
		 case 12: return  100;
		 case 13: return  100;
       case 14: return  100;
       case 15: return  100;
       case 16: return  100;
       case 17: return  100;
       case 18: return  100;
       case 19: return  100;
       case 20: return  100;
		 case 21: return  100;
       case 22: return  100;
       case 23: return  100;
		 case 24: return  100;
		 case 25: return  100;
		 case 26: return  100;
       case 27: return  100;
       case 28: return  100;
       case 29: return  100;
       case 30: return  100;
       default: return  70;
 	   break;
      }
	 default:
		log("SYSERR: Invalid saving throw type.");
		break;
						}
	 break;
	case CLASS_DEFENDERMAGE:
	 switch (type) {
    case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 default: return  0;
		break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
		 default: return  0;
		break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
		 default: return 	0;
		 break;
		}
	 case SAVING_BREATH:	// Breath weapons
		switch (level)
		{case  0: return 90;
		 case  1: return 75;
		 case  2: return 73;
		 case  3: return 71;
		 case  4: return 69;
		 case  5: return 67;
		 case  6: return 65;
		 case  7: return 63;
		 case  8: return 61;
		 case  9: return 60;
		 case 10: return 59;
		 case 11: return 57;
		 case 12: return 55;
		 case 13: return 53;
		 case 14: return 51;
		 case 15: return 50;
		 case 16: return 49;
		 case 17: return 47;
		 case 18: return 45;
		 case 19: return 43;
		 case 20: return 41;
		 case 21: return 40;
		 case 22: return 39;
		 case 23: return 37;
		 case 24: return 35;
		 case 25: return 33;
		 case 26: return 31;
		 case 27: return 29;
		 case 28: return 27;
		 case 29: return 25;
		 case 30: return 23;
		 default: return 0;
		 break;
		}
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 default: return  0;
		break;
      }
    case SAVING_BASIC:  // Protect from skills
      switch (level)
      {case  0: return  100;
		 case  1: return  100;
       case  2: return  100;
       case  3: return  100;
		 case  4: return  100;
		 case  5: return  100;
       case  6: return  100;
       case  7: return  100;
       case  8: return  100;
       case  9: return  100;
       case 10: return  100;
		 case 11: return  100;
		 case 12: return  100;
		 case 13: return  100;
       case 14: return  100;
       case 15: return  100;
       case 16: return  100;
       case 17: return  100;
       case 18: return  100;
       case 19: return  100;
       case 20: return  100;
       case 21: return  100;
       case 22: return  100;
       case 23: return  100;
       case 24: return  100;
       case 25: return  100;
       case 26: return  100;
		 case 27: return  100;
       case 28: return  100;
       case 29: return  100;
       case 30: return  100;
       default: return  70;
 	   break;
		}
	 default:
		log("SYSERR: Invalid saving throw type.");
		break;
						}
	 break;
	case CLASS_CHARMMAGE:
	 switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  81;
		 case 15: return  79;
		 case 16: return  78;
		 case 17: return  75;
		 case 18: return  73;
		 case 19: return  71;
		 case 20: return  68;
		 case 21: return  65;
		 case 22: return  62;
		 case 23: return  59;
		 case 24: return  56;
		 case 25: return  52;
		 case 26: return  48;
		 case 27: return  44;
		 case 28: return  40;
		 case 29: return  35;
		 case 30: return  30;
		 default: return 	0;
		break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
		 default: return  0;
		break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 default: return 	0;
		 break;
      }
    case SAVING_BREATH:	// Breath weapons
      switch (level)
      {case  0: return 90;
		 case  1: return 75;
       case  2: return 73;
       case  3: return 71;
		 case  4: return 69;
       case  5: return 67;
       case  6: return 65;
       case  7: return 63;
       case  8: return 61;
		 case  9: return 60;
       case 10: return 59;
		 case 11: return 57;
       case 12: return 55;
       case 13: return 53;
       case 14: return 51;
       case 15: return 50;
       case 16: return 49;
       case 17: return 47;
       case 18: return 45;
       case 19: return 43;
       case 20: return 41;
		 case 21: return 40;
		 case 22: return 39;
       case 23: return 37;
       case 24: return 35;
       case 25: return 33;
       case 26: return 31;
       case 27: return 29;
       case 28: return 27;
       case 29: return 25;
       case 30: return 23;
       default: return 0;
       break;
		}
    case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  81;
		 case 15: return  79;
		 case 16: return  78;
		 case 17: return  75;
		 case 18: return  73;
		 case 19: return  71;
		 case 20: return  68;
		 case 21: return  65;
		 case 22: return  62;
		 case 23: return  59;
		 case 24: return  56;
		 case 25: return  52;
		 case 26: return  48;
		 case 27: return  44;
		 case 28: return  40;
		 case 29: return  35;
		 case 30: return  30;
		 default: return   0;
		break;
		}
	 case SAVING_BASIC:  // Protect from skills
		switch (level)
		{case  0: return  100;
		 case  1: return  100;
		 case  2: return  100;
		 case  3: return  100;
		 case  4: return  100;
		 case  5: return  100;
		 case  6: return  100;
		 case  7: return  100;
		 case  8: return  100;
		 case  9: return  100;
		 case 10: return  100;
		 case 11: return  100;
		 case 12: return  100;
		 case 13: return  100;
		 case 14: return  100;
		 case 15: return  100;
		 case 16: return  100;
		 case 17: return  100;
		 case 18: return  100;
		 case 19: return  100;
		 case 20: return  100;
		 case 21: return  100;
		 case 22: return  100;
		 case 23: return  100;
       case 24: return  100;
       case 25: return  100;
       case 26: return  100;
       case 27: return  100;
		 case 28: return  100;
       case 29: return  100;
       case 30: return  100;
       default: return  70;
 	   break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
                  }
	 break;
	case CLASS_NECROMANCER:
    switch (type) {
    case SAVING_PARA:	// Paralyzation
      switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  81;
		 case 15: return  79;
		 case 16: return  78;
		 case 17: return  75;
		 case 18: return  73;
		 case 19: return  71;
		 case 20: return  68;
		 case 21: return  65;
		 case 22: return  62;
		 case 23: return  59;
		 case 24: return  56;
		 case 25: return  52;
		 case 26: return  48;
		 case 27: return  44;
		 case 28: return  40;
		 case 29: return  35;
		 case 30: return  30;
		 default: return 	0;
		break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  81;
		 case 15: return  79;
		 case 16: return  78;
		 case 17: return  75;
		 case 18: return  73;
		 case 19: return  71;
		 case 20: return  68;
		 case 21: return  65;
		 case 22: return  62;
		 case 23: return  59;
		 case 24: return  56;
		 case 25: return  52;
		 case 26: return  48;
		 case 27: return  44;
		 case 28: return  40;
		 case 29: return  35;
		 case 30: return  30;
		 default: return  0;
		break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  81;
		 case 15: return  79;
		 case 16: return  78;
		 case 17: return  75;
		 case 18: return  73;
		 case 19: return  71;
		 case 20: return  68;
		 case 21: return  65;
		 case 22: return  62;
		 case 23: return  59;
		 case 24: return  56;
		 case 25: return  52;
		 case 26: return  48;
		 case 27: return  44;
		 case 28: return  40;
		 case 29: return  35;
		 case 30: return  30;
		 default: return 	0;
		 break;
		}
	 case SAVING_BREATH:	// Breath weapons
		switch (level)
		{case  0: return 90;
		 case  1: return 75;
		 case  2: return 73;
		 case  3: return 71;
		 case  4: return 69;
		 case  5: return 67;
		 case  6: return 65;
       case  7: return 63;
       case  8: return 61;
       case  9: return 60;
       case 10: return 59;
       case 11: return 57;
		 case 12: return 55;
       case 13: return 53;
       case 14: return 51;
       case 15: return 50;
       case 16: return 49;
       case 17: return 47;
       case 18: return 45;
       case 19: return 43;
       case 20: return 41;
       case 21: return 40;
		 case 22: return 39;
		 case 23: return 37;
       case 24: return 35;
       case 25: return 33;
       case 26: return 31;
       case 27: return 29;
       case 28: return 27;
       case 29: return 25;
       case 30: return 23;
       default: return 0;
       break;
      }
	 case SAVING_SPELL:	// Generic spells
      switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  81;
		 case 15: return  79;
		 case 16: return  78;
		 case 17: return  75;
		 case 18: return  73;
		 case 19: return  71;
		 case 20: return  68;
		 case 21: return  65;
		 case 22: return  62;
		 case 23: return  59;
		 case 24: return  56;
		 case 25: return  52;
		 case 26: return  48;
		 case 27: return  44;
		 case 28: return  40;
		 case 29: return  35;
		 case 30: return  30;
		 default: return  0;
		break;
      }
    case SAVING_BASIC:  // Protect from skills
      switch (level)
      {case  0: return  100;
		 case  1: return  100;
       case  2: return  100;
       case  3: return  100;
       case  4: return  100;
       case  5: return  100;
       case  6: return  100;
		 case  7: return  100;
       case  8: return  100;
       case  9: return  100;
       case 10: return  100;
		 case 11: return  100;
       case 12: return  100;
       case 13: return  100;
		 case 14: return  100;
       case 15: return  100;
       case 16: return  100;
       case 17: return  100;
       case 18: return  100;
       case 19: return  100;
       case 20: return  100;
       case 21: return  100;
       case 22: return  100;
       case 23: return  100;
       case 24: return  100;
       case 25: return  100;
       case 26: return  100;
       case 27: return  100;
       case 28: return  100;
		 case 29: return  100;
       case 30: return  100;
       default: return  70;
 	   break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
		break;
						}
	 break;
  case CLASS_CLERIC:
	 switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 default: return 	0;
		 break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 default: return 	0;
		 break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
		 default: return  0;
		 break;
      }
    case SAVING_BREATH:	// Breath weapons
      switch (level)
      {case  0: return 90;
		 case  1: return 80;
       case  2: return 79;
       case  3: return 78;
       case  4: return 76;
		 case  5: return 75;
       case  6: return 73;
       case  7: return 70;
       case  8: return 67;
       case  9: return 65;
       case 10: return 64;
       case 11: return 63;
       case 12: return 61;
		 case 13: return 60;
       case 14: return 59;
       case 15: return 57;
       case 16: return 56;
       case 17: return 55;
       case 18: return 54;
       case 19: return 53;
       case 20: return 52;
       case 21: return 51;
       case 22: return 50;
       case 23: return 48;
		 case 24: return 45;
		 case 25: return 44;
		 case 26: return 42;
		 case 27: return 40;
		 case 28: return 39;
		 case 29: return 38;
		 case 30: return 37;
		 default: return  0;
		 break;
		}
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
		 default: return  0;
		}
    case SAVING_BASIC:  // Protect from skills
      switch (level)
      {case  0: return  100;
		 case  1: return  100;
       case  2: return  100;
       case  3: return  100;
       case  4: return  100;
       case  5: return  100;
       case  6: return  100;
       case  7: return  100;
       case  8: return  100;
		 case  9: return  100;
       case 10: return  100;
       case 11: return  100;
       case 12: return  100;
       case 13: return  100;
       case 14: return  100;
       case 15: return  100;
       case 16: return  100;
       case 17: return  100;
       case 18: return  100;
       case 19: return  100;
       case 20: return  100;
       case 21: return  100;
       case 22: return  100;
		 case 23: return  100;
       case 24: return  100;
       case 25: return  100;
       case 26: return  100;
       case 27: return  100;
       case 28: return  100;
       case 29: return  100;
       case 30: return  100;
		 default: return  70;
 	   break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
						}
	 break;
  case CLASS_DRUID:
	 switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  83;
		 case 13: return  82;
		 case 14: return  80;
		 case 15: return  79;
		 case 16: return  76;
		 case 17: return  74;
		 case 18: return  72;
		 case 19: return  69;
		 case 20: return  66;
		 case 21: return  63;
		 case 22: return  60;
		 case 23: return  57;
		 case 24: return  53;
		 case 25: return  49;
		 case 26: return  45;
		 case 27: return  40;
		 case 28: return  35;
		 case 29: return  30;
		 case 30: return  25;
		 default: return  0;
		 break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  83;
		 case 13: return  82;
		 case 14: return  80;
		 case 15: return  79;
		 case 16: return  76;
		 case 17: return  74;
		 case 18: return  72;
		 case 19: return  69;
		 case 20: return  66;
		 case 21: return  63;
		 case 22: return  60;
		 case 23: return  57;
		 case 24: return  53;
		 case 25: return  49;
		 case 26: return  45;
		 case 27: return  40;
		 case 28: return  35;
		 case 29: return  30;
		 case 30: return  25;
		 default: return  0;
		 break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  83;
		 case 13: return  82;
		 case 14: return  80;
		 case 15: return  79;
		 case 16: return  76;
		 case 17: return  74;
		 case 18: return  72;
		 case 19: return  69;
		 case 20: return  66;
		 case 21: return  63;
		 case 22: return  60;
		 case 23: return  57;
		 case 24: return  53;
		 case 25: return  49;
		 case 26: return  45;
		 case 27: return  40;
		 case 28: return  35;
		 case 29: return  30;
		 case 30: return  25;
		 default: return 	0;
		 break;
		}
	 case SAVING_BREATH:	// Breath weapons
		switch (level)
		{case  0: return 90;
		 case  1: return 80;
       case  2: return 79;
       case  3: return 78;
       case  4: return 76;
       case  5: return 75;
       case  6: return 73;
       case  7: return 70;
       case  8: return 67;
       case  9: return 65;
       case 10: return 64;
       case 11: return 63;
       case 12: return 61;
       case 13: return 60;
       case 14: return 59;
		 case 15: return 57;
       case 16: return 56;
       case 17: return 55;
       case 18: return 54;
       case 19: return 53;
       case 20: return 52;
       case 21: return 51;
       case 22: return 50;
       case 23: return 48;
       case 24: return 45;
       case 25: return 44;
       case 26: return 42;
       case 27: return 40;
       case 28: return 39;
		 case 29: return 38;
       case 30: return 37;
       default: return  0;
       break;
      }
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  83;
		 case 13: return  82;
		 case 14: return  80;
		 case 15: return  79;
		 case 16: return  76;
		 case 17: return  74;
		 case 18: return  72;
		 case 19: return  69;
		 case 20: return  66;
		 case 21: return  63;
		 case 22: return  60;
		 case 23: return  57;
		 case 24: return  53;
		 case 25: return  49;
		 case 26: return  45;
		 case 27: return  40;
		 case 28: return  35;
		 case 29: return  30;
		 case 30: return  25;
		 default: return 	0;
      }
    case SAVING_BASIC:  // Protect from skills
      switch (level)
      {case  0: return  100;
		 case  1: return  100;
       case  2: return  100;
		 case  3: return  100;
       case  4: return  100;
       case  5: return  100;
       case  6: return  100;
       case  7: return  100;
       case  8: return  100;
       case  9: return  100;
       case 10: return  100;
		 case 11: return  100;
       case 12: return  100;
       case 13: return  100;
       case 14: return  100;
       case 15: return  100;
       case 16: return  100;
       case 17: return  100;
       case 18: return  100;
       case 19: return  100;
       case 20: return  100;
       case 21: return  100;
       case 22: return  100;
       case 23: return  100;
       case 24: return  100;
       case 25: return  100;
       case 26: return  100;
       case 27: return  100;
		 case 28: return  100;
       case 29: return  100;
       case 30: return  100;
       default: return  70;
 	   break;
		}
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
                  }
    break;
  case CLASS_THIEF:
  case CLASS_ASSASINE:
  case CLASS_MERCHANT:
	 switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
		 default: return  0;
		 break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
		 default: return  0;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
		 default: return  0;
       break;
      }
    case SAVING_BREATH:	// Breath weapons
      switch (level)
      {case  0: return 90;
		 case  1: return 80;
       case  2: return 79;
       case  3: return 78;
		 case  4: return 77;
		 case  5: return 76;
		 case  6: return 75;
		 case  7: return 74;
		 case  8: return 73;
		 case  9: return 72;
		 case 10: return 71;
		 case 11: return 70;
		 case 12: return 69;
		 case 13: return 68;
		 case 14: return 67;
		 case 15: return 66;
		 case 16: return 65;
		 case 17: return 64;
		 case 18: return 63;
		 case 19: return 62;
		 case 20: return 61;
		 case 21: return 60;
		 case 22: return 59;
		 case 23: return 58;
		 case 24: return 57;
		 case 25: return 56;
		 case 26: return 55;
		 case 27: return 54;
		 case 28: return 53;
		 case 29: return 52;
		 case 30: return 51;
		 default: return  0;
		 break;
		}
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
		 default: return  0;
		 break;
		}
	 case SAVING_BASIC:  // Protect from skills
		switch (level)
		{case  0: return  100;
		 case  1: return  99;
		 case  2: return  98;
		 case  3: return  97;
		 case  4: return  96;
		 case  5: return  95;
		 case  6: return  94;
		 case  7: return  93;
		 case  8: return  92;
		 case  9: return  91;
		 case 10: return  90;
		 case 11: return  89;
		 case 12: return  88;
		 case 13: return  87;
		 case 14: return  86;
		 case 15: return  85;
		 case 16: return  84;
		 case 17: return  83;
		 case 18: return  82;
		 case 19: return  81;
		 case 20: return  80;
		 case 21: return  79;
		 case 22: return  78;
		 case 23: return  77;
		 case 24: return  76;
		 case 25: return  75;
		 case 26: return  74;
		 case 27: return  73;
		 case 28: return  72;
		 case 29: return  71;
		 case 30: return  70;
		 default: return  50;
		break;
		}
	 default:
		log("SYSERR: Invalid saving throw type.");
		break;
						}
	 break;
  case CLASS_WARRIOR:
  case CLASS_GUARD:
  case CLASS_SMITH:
	 switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  90;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  89;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  87;
		 case 12: return  86;
		 case 13: return  85;
		 case 14: return  84;
		 case 15: return  83;
		 case 16: return  82;
		 case 17: return  80;
		 case 18: return  79;
		 case 19: return  77;
		 case 20: return  75;
		 case 21: return  74;
		 case 22: return  72;
		 case 23: return  69;
		 case 24: return  67;
		 case 25: return  65;
		 case 26: return  62;
		 case 27: return  59;
		 case 28: return  56;
		 case 29: return  53;
		 case 30: return  50;
		 case 31: return  45;
		 case 32: return  40;
		 case 33: return  35;
		 case 34: return  30;
		 case 35: return  25;
		 case 36: return  20;
		 case 37: return  15;
		 case 38: return  10;
		 case 39: return   5;
		 case 40: return   4;
		 case 41: return   3;	// Some mobiles
		 case 42: return   2;
		 case 43: return   1;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  90;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  89;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  87;
		 case 12: return  86;
		 case 13: return  85;
		 case 14: return  84;
		 case 15: return  83;
		 case 16: return  82;
		 case 17: return  80;
		 case 18: return  79;
		 case 19: return  77;
		 case 20: return  75;
		 case 21: return  74;
		 case 22: return  72;
		 case 23: return  69;
		 case 24: return  67;
		 case 25: return  65;
		 case 26: return  62;
		 case 27: return  59;
		 case 28: return  56;
		 case 29: return  53;
		 case 30: return  50;
		 case 31: return  45;
		 case 32: return  40;
		 case 33: return  35;
		 case 34: return  30;
		 case 35: return  25;
		 case 36: return  20;
		 case 37: return  15;
		 case 38: return  10;
		 case 39: return   5;
		 case 40: return   4;
		 case 41: return   3;	// Some mobiles
		 case 42: return   2;
		 case 43: return   1;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  90;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  89;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  87;
		 case 12: return  86;
		 case 13: return  85;
		 case 14: return  84;
		 case 15: return  83;
		 case 16: return  82;
		 case 17: return  80;
		 case 18: return  79;
		 case 19: return  77;
		 case 20: return  75;
		 case 21: return  74;
		 case 22: return  72;
		 case 23: return  69;
		 case 24: return  67;
		 case 25: return  65;
		 case 26: return  62;
		 case 27: return  59;
		 case 28: return  56;
		 case 29: return  53;
		 case 30: return  50;
		 case 31: return  45;
		 case 32: return  40;
		 case 33: return  35;
		 case 34: return  30;
		 case 35: return  25;
		 case 36: return  20;
		 case 37: return  15;
		 case 38: return  10;
		 case 39: return   5;
		 case 40: return   4;
		 case 41: return   3;	// Some mobiles
		 case 42: return   2;
		 case 43: return   1;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
	 case SAVING_BREATH:	// Breath weapons
		switch (level)
		{case  0: return 90;
		 case  1: return 85;
		 case  2: return 83;
		 case  3: return 82;
		 case  4: return 80;
		 case  5: return 75;
		 case  6: return 70;
		 case  7: return 65;
		 case  8: return 63;
		 case  9: return 62;
		 case 10: return 60;
		 case 11: return 55;
		 case 12: return 50;
		 case 13: return 45;
		 case 14: return 43;
		 case 15: return 42;
		 case 16: return 40;
		 case 17: return 37;
		 case 18: return 33;
		 case 19: return 30;
		 case 20: return 29;
		 case 21: return 28;
		 case 22: return 26;
		 case 23: return 25;
		 case 24: return 24;
		 case 25: return 23;
		 case 26: return 21;
		 case 27: return 20;
		 case 28: return 19;
		 case 29: return 18;
		 case 30: return 17;
		 case 31: return 16;
		 case 32: return 15;
		 case 33: return 14;
		 case 34: return 13;
		 case 35: return 12;
		 case 36: return 11;
		 case 37: return 10;
		 case 38: return  9;
		 case 39: return  8;
		 case 40: return  7;
		 case 41: return  6;
		 case 42: return  5;
		 case 43: return  4;
		 case 44: return  3;
		 case 45: return  2;
		 case 46: return  1;
		 case 47: return  0;
		 case 48: return  0;
		 case 49: return  0;
		 default: return  0;
		 break;
		}
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  90;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  89;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  87;
		 case 12: return  86;
		 case 13: return  85;
		 case 14: return  84;
		 case 15: return  83;
		 case 16: return  82;
		 case 17: return  80;
		 case 18: return  79;
		 case 19: return  77;
		 case 20: return  75;
		 case 21: return  74;
		 case 22: return  72;
		 case 23: return  69;
		 case 24: return  67;
		 case 25: return  65;
		 case 26: return  62;
		 case 27: return  59;
		 case 28: return  56;
		 case 29: return  53;
		 case 30: return  50;
		 case 31: return  45;
		 case 32: return  40;
		 case 33: return  35;
		 case 34: return  30;
		 case 35: return  25;
		 case 36: return  20;
		 case 37: return  15;
		 case 38: return  10;
		 case 39: return   5;
		 case 40: return   4;
		 case 41: return   3;	// Some mobiles
		 case 42: return   2;
		 case 43: return   1;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
      }
    case SAVING_BASIC:  // Protect from skills
      switch (level)
      {case  0: return  100;
		 case  1: return  99;
       case  2: return  97;
       case  3: return  96;
       case  4: return  95;
       case  5: return  94;
       case  6: return  92;
       case  7: return  91;
       case  8: return  89;
       case  9: return  88;
       case 10: return  86;
       case 11: return  85;
       case 12: return  84;
       case 13: return  83;
       case 14: return  81;
		 case 15: return  80;
       case 16: return  79;
		 case 17: return  77;
       case 18: return  76;
       case 19: return  75;
       case 20: return  74;
       case 21: return  72;
       case 22: return  71;
       case 23: return  70;
       case 24: return  68;
		 case 25: return  65;
       case 26: return  64;
		 case 27: return  63;
       case 28: return  62;
		 case 29: return  61;
       case 30: return  60;
       case 31: return  58;
       case 32: return  57;
       case 33: return  56;
       case 34: return  54;
       case 35: return  52;
       case 36: return  51;
       case 37: return  49;
		 case 38: return  47;
       case 39: return  46;
		 case 40: return  45;
		 case 41: return  43;
		 case 42: return  42;
		 case 43: return  41;
		 case 44: return  39;
		 case 45: return  37;
		 case 46: return  35;
		 case 47: return  34;
		 case 48: return  32;
		 case 49: return  31;
		 default: return  30;
		break;
		}
	 default:
		log("SYSERR: Invalid saving throw type.");
		break;
	 }
  case CLASS_RANGER:
  case CLASS_PALADINE:
	 switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  86;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  82;
		 case 15: return  80;
		 case 16: return  79;
		 case 17: return  77;
		 case 18: return  75;
		 case 19: return  72;
		 case 20: return  70;
		 case 21: return  67;
		 case 22: return  65;
		 case 23: return  62;
		 case 24: return  59;
		 case 25: return  55;
		 case 26: return  52;
		 case 27: return  48;
		 case 28: return  44;
		 case 29: return  39;
		 case 30: return  35;
		 default: return 	0;
		 break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  86;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  82;
		 case 15: return  80;
		 case 16: return  79;
		 case 17: return  77;
		 case 18: return  75;
		 case 19: return  72;
		 case 20: return  70;
		 case 21: return  67;
		 case 22: return  65;
		 case 23: return  62;
		 case 24: return  59;
		 case 25: return  55;
		 case 26: return  52;
		 case 27: return  48;
		 case 28: return  44;
		 case 29: return  39;
		 case 30: return  35;
		 default: return 	0;
		 break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  86;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  82;
		 case 15: return  80;
		 case 16: return  79;
		 case 17: return  77;
		 case 18: return  75;
		 case 19: return  72;
		 case 20: return  70;
		 case 21: return  67;
		 case 22: return  65;
		 case 23: return  62;
		 case 24: return  59;
		 case 25: return  55;
		 case 26: return  52;
		 case 27: return  48;
		 case 28: return  44;
		 case 29: return  39;
		 case 30: return  35;
		 default: return  0;
		 break;
		}
	 case SAVING_BREATH:	// Breath weapons
		switch (level)
		{case  0: return 90;
		 case  1: return 85;
       case  2: return 83;
       case  3: return 82;
       case  4: return 80;
       case  5: return 75;
       case  6: return 70;
       case  7: return 65;
       case  8: return 63;
       case  9: return 62;
       case 10: return 60;
       case 11: return 55;
       case 12: return 50;
       case 13: return 45;
       case 14: return 43;
       case 15: return 42;
       case 16: return 40;
       case 17: return 37;
       case 18: return 33;
		 case 19: return 30;
       case 20: return 29;
       case 21: return 28;
       case 22: return 26;
       case 23: return 25;
       case 24: return 24;
       case 25: return 23;
		 case 26: return 21;
       case 27: return 20;
       case 28: return 19;
       case 29: return 18;
		 case 30: return 17;
       default: return 0;
       break;
      }
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  86;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  82;
		 case 15: return  80;
		 case 16: return  79;
		 case 17: return  77;
		 case 18: return  75;
		 case 19: return  72;
		 case 20: return  70;
		 case 21: return  67;
		 case 22: return  65;
		 case 23: return  62;
		 case 24: return  59;
		 case 25: return  55;
		 case 26: return  52;
		 case 27: return  48;
		 case 28: return  44;
		 case 29: return  39;
		 case 30: return  35;
		 default: return 	0;
		 break;
		}
	 case SAVING_BASIC:  // Protect from skills
		switch (level)
		{case  0: return  100;
		 case  1: return  99;
		 case  2: return  97;
		 case  3: return  96;
		 case  4: return  95;
		 case  5: return  94;
       case  6: return  92;
		 case  7: return  91;
       case  8: return  89;
       case  9: return  88;
       case 10: return  86;
       case 11: return  84;
       case 12: return  82;
       case 13: return  80;
       case 14: return  78;
       case 15: return  76;
       case 16: return  74;
		 case 17: return  72;
       case 18: return  70;
       case 19: return  68;
       case 20: return  64;
       case 21: return  62;
       case 22: return  60;
       case 23: return  58;
       case 24: return  56;
       case 25: return  54;
       case 26: return  52;
       case 27: return  50;
       case 28: return  49;
       case 29: return  47;
       case 30: return  45;
       default: return  0;
       break;
      }
    default:
		log("SYSERR: Invalid saving throw type.");
      break;
    }

default: // All NPC's
    switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 case 31: return 	15;
		 case 32: return 	10;
		 case 33: return   9;
		 case 34: return   8;
		 case 35: return   7;
		 case 36: return   6;
		 case 37: return   5;
		 case 38: return   4;
		 case 39: return   3;
		 case 40: return   2;
		 case 41: return   1;	// Some mobiles
		 case 42: return   0;
		 case 43: return   0;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 case 31: return 	15;
		 case 32: return 	10;
		 case 33: return   9;
		 case 34: return   8;
		 case 35: return   7;
		 case 36: return   6;
		 case 37: return   5;
		 case 38: return   4;
		 case 39: return   3;
		 case 40: return   2;
		 case 41: return   1;	// Some mobiles
		 case 42: return   0;
		 case 43: return   0;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 case 31: return 	15;
		 case 32: return 	10;
		 case 33: return   9;
		 case 34: return   8;
		 case 35: return   7;
		 case 36: return   6;
		 case 37: return   5;
		 case 38: return   4;
		 case 39: return   3;
		 case 40: return   2;
		 case 41: return   1;	// Some mobiles
		 case 42: return   0;
		 case 43: return   0;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
	 case SAVING_BREATH:	// Breath weapons
		switch (level)
		{case  0: return 90;
		 case  1: return 85;
       case  2: return 83;
       case  3: return 82;
       case  4: return 80;
       case  5: return 75;
       case  6: return 70;
       case  7: return 65;
       case  8: return 63;
       case  9: return 62;
       case 10: return 60;
       case 11: return 55;
       case 12: return 50;
       case 13: return 45;
		 case 14: return 43;
       case 15: return 42;
       case 16: return 40;
       case 17: return 37;
       case 18: return 33;
		 case 19: return 30;
       case 20: return 29;
       case 21: return 28;
		 case 22: return 26;
       case 23: return 25;
       case 24: return 24;
       case 25: return 23;
       case 26: return 21;
       case 27: return 20;
       case 28: return 19;
		 case 29: return 18;
       case 30: return 17;
       case 31: return 16;
       case 32: return 15;
       case 33: return 14;
       case 34: return 13;
       case 35: return 12;
       case 36: return 11;
       case 37: return 10;
       case 38: return  9;
       case 39: return  8;
       case 40: return  7;
       case 41: return  6;
       case 42: return  5;
       case 43: return  4;
		 case 44: return  3;
		 case 45: return  2;
		 case 46: return  1;
		 case 47: return  0;
		 case 48: return  0;
		 case 49: return  0;
		 default: return  0;
		 break;
		}
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 case 31: return 	15;
		 case 32: return 	10;
		 case 33: return   9;
		 case 34: return   8;
		 case 35: return   7;
		 case 36: return   6;
		 case 37: return   5;
		 case 38: return   4;
		 case 39: return   3;
		 case 40: return   2;
		 case 41: return   1;	// Some mobiles
		 case 42: return   0;
		 case 43: return   0;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
    case SAVING_BASIC:  // Protect from skills
      switch (level)
      {case  0: return  100;
		 case  1: return  99;
       case  2: return  97;
       case  3: return  96;
       case  4: return  95;
       case  5: return  94;
       case  6: return  92;
		 case  7: return  91;
       case  8: return  89;
       case  9: return  88;
       case 10: return  86;
       case 11: return  85;
       case 12: return  84;
       case 13: return  83;
       case 14: return  81;
       case 15: return  80;
       case 16: return  79;
       case 17: return  77;
       case 18: return  76;
       case 19: return  75;
       case 20: return  74;
       case 21: return  72;
       case 22: return  71;
       case 23: return  70;
       case 24: return  68;
		 case 25: return  65;
		 case 26: return  64;
		 case 27: return  63;
		 case 28: return  62;
		 case 29: return  61;
		 case 30: return  60;
		 case 31: return  58;
		 case 32: return  57;
		 case 33: return  56;
		 case 34: return  54;
		 case 35: return  52;
		 case 36: return  51;
		 case 37: return  49;
		 case 38: return  47;
		 case 39: return  46;
		 case 40: return  45;
		 case 41: return  43;
		 case 42: return  42;
		 case 43: return  41;
		 case 44: return  39;
		 case 45: return  37;
       case 46: return  35;
       case 47: return  34;
       case 48: return  32;
       case 49: return  31;
       default: return  30;
		 break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }
  }

  return 100;
}






/* THAC0 for classes and levels.  (To Hit Armor Class 0) */
int thaco(int class_num, int level)
{
  switch (class_num) {
  case CLASS_MAGIC_USER:
  case CLASS_DEFENDERMAGE:
  case CLASS_CHARMMAGE:
  case CLASS_NECROMANCER:
    switch (level) {
    case  0: return 100;
	 case  1: return  20;
    case  2: return  20;
    case  3: return  20;
	 case  4: return  19;
	 case  5: return  19;
	 case  6: return  19;
	 case  7: return  18;
	 case  8: return  18;
	 case  9: return  18;
	 case 10: return  17;
	 case 11: return  17;
	 case 12: return  17;
	 case 13: return  16;
	 case 14: return  16;
	 case 15: return  16;
	 case 16: return  15;
	 case 17: return  15;
	 case 18: return  15;
	 case 19: return  14;
    case 20: return  14;
    case 21: return  14;
    case 22: return  13;
    case 23: return  13;
    case 24: return  13;
    case 25: return  12;
    case 26: return  12;
    case 27: return  12;
    case 28: return  11;
    case 29: return  11;
    case 30: return  10;
    case 31: return  0;
    case 32: return  0;
    case 33: return  0;
    case 34: return  0;
    default:
      log("SYSERR: Missing level for mage thac0.");
    }
  case CLASS_CLERIC:
  case CLASS_DRUID:
    switch (level) {
    case  0: return 100;
	 case  1: return  20;
    case  2: return  20;
    case  3: return  20;
    case  4: return  18;
    case  5: return  18;
    case  6: return  18;
    case  7: return  16;
    case  8: return  16;
    case  9: return  16;
    case 10: return  14;
    case 11: return  14;
    case 12: return  14;
    case 13: return  12;
    case 14: return  12;
    case 15: return  12;
	 case 16: return  10;
    case 17: return  10;
    case 18: return  10;
    case 19: return   8;
    case 20: return   8;
    case 21: return   8;
	 case 22: return   6;
    case 23: return   6;
    case 24: return   6;
    case 25: return   4;
    case 26: return   4;
    case 27: return   4;
    case 28: return   2;
    case 29: return   2;
    case 30: return   1;
    case 31: return   0;
    case 32: return   0;
    case 33: return   0;
    case 34: return   0;
    default:
      log("SYSERR: Missing level for cleric thac0.");
	 }
  case CLASS_ASSASINE:
  case CLASS_THIEF:
  case CLASS_MERCHANT:
    switch (level) {
    case  0: return 100;
	 case  1: return  20;
    case  2: return  20;
    case  3: return  19;
    case  4: return  19;
    case  5: return  18;
    case  6: return  18;
    case  7: return  17;
    case  8: return  17;
    case  9: return  16;
    case 10: return  16;
    case 11: return  15;
    case 12: return  15;
    case 13: return  14;
    case 14: return  14;
    case 15: return  13;
    case 16: return  13;
    case 17: return  12;
    case 18: return  12;
    case 19: return  11;
    case 20: return  11;
    case 21: return  10;
    case 22: return  10;
    case 23: return   9;
	 case 24: return   9;
    case 25: return   8;
    case 26: return   8;
    case 27: return   7;
    case 28: return   7;
    case 29: return   6;
    case 30: return   5;
    case 31: return   0;
    case 32: return   0;
    case 33: return   0;
    case 34: return   0;
    default:
      log("SYSERR: Missing level for thief thac0.");
    }
  case CLASS_WARRIOR:
  case CLASS_GUARD:
	 switch (level) {
	 case  0: return 100;
	 case  1: return  20;
    case  2: return  19;
    case  3: return  18;
    case  4: return  17;
    case  5: return  16;
    case  6: return  15;
    case  7: return  14;
    case  8: return  14;
    case  9: return  13;
    case 10: return  12;
    case 11: return  11;
    case 12: return  10;
    case 13: return   9;
    case 14: return   8;
    case 15: return   7;
    case 16: return   6;
    case 17: return   5;
    case 18: return   4;
    case 19: return   3;
    case 20: return   2;
    case 21: return   1;
    case 22: return   1;
    case 23: return   1;
    case 24: return   1;
    case 25: return   1;
    case 26: return   1;
	 case 27: return   1;
    case 28: return   1;
    case 29: return   1;
    case 30: return   0;
    case 31: return   0;
    case 32: return   0;
    case 33: return   0;
    case 34: return   0;
    default:
      log("SYSERR: Missing level for warrior thac0.");
    }
  case CLASS_PALADINE:
  case CLASS_RANGER:
  case CLASS_SMITH:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
	 case  2: return  19;
    case  3: return  18;
    case  4: return  18;
    case  5: return  17;
    case  6: return  16;
    case  7: return  16;
    case  8: return  15;
    case  9: return  14;
    case 10: return  14;
    case 11: return  13;
    case 12: return  12;
    case 13: return  11;
    case 14: return  11;
    case 15: return  10;
    case 16: return  10;
    case 17: return   9;
    case 18: return   8;
    case 19: return   7;
    case 20: return   6;
    case 21: return   6;
    case 22: return   5;
    case 23: return   5;
    case 24: return   4;
    case 25: return   4;
    case 26: return   3;
    case 27: return   3;
    case 28: return   2;
	 case 29: return   1;
    case 30: return   0;
    case 31: return   0;
    case 32: return   0;
    case 33: return   0;
    case 34: return   0;
    default:
      log("SYSERR: Missing level for warrior thac0.");
    }

  default:
    log("SYSERR: Unknown class in thac0 chart.");
  }

  /* Will not get there unless something is wrong. */
  return 100;
}

/* AC0 for classes and levels. */
int extra_aco(int class_num, int level)
{
  switch (class_num) {
  case CLASS_MAGIC_USER:
  case CLASS_DEFENDERMAGE:
  case CLASS_CHARMMAGE:
  case CLASS_NECROMANCER:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
    case  7: return 0;
    case  8: return 0;
    case  9: return 0;
    case 10: return 0;
    case 11: return 0;
    case 12: return 0;
    case 13: return 0;
    case 14: return 0;
    case 15: return 0;
    case 16: return -1;
	 case 17: return -1;
    case 18: return -1;
    case 19: return -1;
    case 20: return -1;
    case 21: return -1;
    case 22: return -1;
    case 23: return -1;
    case 24: return -1;
    case 25: return -1;
    case 26: return -1;
    case 27: return -1;
    case 28: return -1;
    case 29: return -1;
    case 30: return -1;
    case 31: return -5;
    case 32: return -10;
    case 33: return -15;
    case 34: return -20;
    default: return 0;
	 }
  case CLASS_CLERIC:
  case CLASS_DRUID:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
    case  7: return 0;
    case  8: return 0;
    case  9: return 0;
    case 10: return 0;
    case 11: return -1;
    case 12: return -1;
    case 13: return -1;
    case 14: return -1;
    case 15: return -1;
    case 16: return -1;
    case 17: return -1;
    case 18: return -1;
    case 19: return -1;
    case 20: return -1;
	 case 21: return -2;
    case 22: return -2;
    case 23: return -2;
    case 24: return -2;
    case 25: return -2;
    case 26: return -2;
    case 27: return -2;
    case 28: return -2;
    case 29: return -2;
    case 30: return -2;
    case 31: return -5;
    case 32: return -10;
    case 33: return -15;
    case 34: return -20;
    default: return 0;
    }
  case CLASS_ASSASINE:
  case CLASS_THIEF:
  case CLASS_MERCHANT:
    switch (level) {
	 case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
    case  7: return 0;
    case  8: return 0;
    case  9: return -1;
    case 10: return -1;
    case 11: return -1;
    case 12: return -1;
    case 13: return -1;
    case 14: return -1;
    case 15: return -1;
    case 16: return -1;
    case 17: return -2;
    case 18: return -2;
    case 19: return -2;
    case 20: return -2;
    case 21: return -2;
    case 22: return -2;
    case 23: return -2;
	 case 24: return -2;
    case 25: return -3;
    case 26: return -3;
    case 27: return -3;
    case 28: return -3;
    case 29: return -3;
    case 30: return -3;
    case 31: return -5;
    case 32: return -10;
    case 33: return -15;
    case 34: return -20;
    default: return 0;
    }
  case CLASS_WARRIOR:
  case CLASS_GUARD:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
	 case  5: return 0;
    case  6: return -1;
    case  7: return -1;
    case  8: return -1;
    case  9: return -1;
    case 10: return -1;
    case 11: return -2;
    case 12: return -2;
    case 13: return -2;
    case 14: return -2;
    case 15: return -2;
    case 16: return -3;
    case 17: return -3;
    case 18: return -3;
    case 19: return -3;
    case 20: return -3;
    case 21: return -4;
    case 22: return -4;
    case 23: return -4;
    case 24: return -4;
    case 25: return -4;
    case 26: return -5;
    case 27: return -5;
    case 28: return -5;
    case 29: return -5;
    case 30: return -5;
    case 31: return -10;
    case 32: return -15;
    case 33: return -20;
    case 34: return -25;
    default: return 0;
    }
  case CLASS_PALADINE:
  case CLASS_RANGER:
  case CLASS_SMITH:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
    case  7: return 0;
    case  8: return -1;
	 case  9: return -1;
	 case 10: return -1;
	 case 11: return -1;
	 case 12: return -1;
	 case 13: return -1;
	 case 14: return -1;
	 case 15: return -2;
	 case 16: return -2;
	 case 17: return -2;
	 case 18: return -2;
	 case 19: return -2;
	 case 20: return -2;
	 case 21: return -2;
	 case 22: return -3;
	 case 23: return -3;
	 case 24: return -3;
	 case 25: return -3;
	 case 26: return -3;
	 case 27: return -3;
	 case 28: return -3;
	 case 29: return -4;
	 case 30: return -4;
	 case 31: return -5;
	 case 32: return -10;
	 case 33: return -15;
	 case 34: return -20;
	 default: return 0;
	 }
  }
  return 0;
}


/* DAMROLL for classes and levels. */
int extra_damroll(int class_num, int level)
{
  switch (class_num) {
  case CLASS_MAGIC_USER:
  case CLASS_DEFENDERMAGE:
  case CLASS_CHARMMAGE:
  case CLASS_NECROMANCER:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
	 case  7: return 0;
    case  8: return 0;
    case  9: return 0;
    case 10: return 0;
    case 11: return 1;
    case 12: return 1;
    case 13: return 1;
    case 14: return 1;
    case 15: return 1;
    case 16: return 1;
    case 17: return 1;
    case 18: return 1;
    case 19: return 1;
    case 20: return 1;
    case 21: return 2;
    case 22: return 2;
    case 23: return 2;
    case 24: return 2;
    case 25: return 2;
    case 26: return 2;
    case 27: return 2;
    case 28: return 2;
    case 29: return 2;
    case 30: return 2;
	 case 31: return 4;
	 case 32: return 6;
	 case 33: return 8;
	 case 34: return 10;
	 default: return 0;
    }
  case CLASS_CLERIC:
  case CLASS_DRUID:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
    case  7: return 0;
    case  8: return 0;
    case  9: return 1;
    case 10: return 1;
    case 11: return 1;
    case 12: return 1;
    case 13: return 1;
    case 14: return 1;
    case 15: return 1;
    case 16: return 1;
    case 17: return 2;
    case 18: return 2;
    case 19: return 2;
    case 20: return 2;
    case 21: return 2;
    case 22: return 2;
    case 23: return 2;
    case 24: return 2;
    case 25: return 3;
    case 26: return 3;
    case 27: return 3;
    case 28: return 3;
    case 29: return 3;
    case 30: return 3;
    case 31: return 5;
    case 32: return 7;
    case 33: return 9;
    case 34: return 10;
    default: return 0;
    }
  case CLASS_ASSASINE:
  case CLASS_THIEF:
  case CLASS_MERCHANT:
	 switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
    case  7: return 0;
    case  8: return 1;
    case  9: return 1;
    case 10: return 1;
    case 11: return 1;
    case 12: return 1;
    case 13: return 1;
    case 14: return 1;
    case 15: return 2;
    case 16: return 2;
    case 17: return 2;
    case 18: return 2;
    case 19: return 2;
    case 20: return 2;
    case 21: return 2;
    case 22: return 3;
    case 23: return 3;
    case 24: return 3;
    case 25: return 3;
    case 26: return 3;
    case 27: return 3;
    case 28: return 3;
    case 29: return 4;
    case 30: return 4;
    case 31: return 5;
    case 32: return 7;
    case 33: return 9;
    case 34: return 11;
    default: return 0;
    }
  case CLASS_WARRIOR:
  case CLASS_GUARD:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
	 case  4: return 0;
    case  5: return 0;
    case  6: return 1;
    case  7: return 1;
    case  8: return 1;
    case  9: return 1;
    case 10: return 1;
    case 11: return 2;
    case 12: return 2;
    case 13: return 2;
    case 14: return 2;
    case 15: return 2;
    case 16: return 3;
    case 17: return 3;
    case 18: return 3;
    case 19: return 3;
    case 20: return 3;
    case 21: return 4;
    case 22: return 4;
    case 23: return 4;
    case 24: return 4;
    case 25: return 4;
    case 26: return 5;
    case 27: return 5;
    case 28: return 5;
    case 29: return 5;
    case 30: return 5;
    case 31: return 10;
    case 32: return 15;
    case 33: return 20;
    case 34: return 25;
    default: return 0;
    }
  case CLASS_PALADINE:
  case CLASS_RANGER:
  case CLASS_SMITH:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
    case  7: return 1;
	 case  8: return 1;
    case  9: return 1;
    case 10: return 1;
    case 11: return 1;
    case 12: return 1;
    case 13: return 2;
    case 14: return 2;
    case 15: return 2;
    case 16: return 2;
    case 17: return 2;
    case 18: return 2;
    case 19: return 3;
    case 20: return 3;
    case 21: return 3;
    case 22: return 3;
    case 23: return 3;
    case 24: return 3;
    case 25: return 4;
    case 26: return 4;
    case 27: return 4;
    case 28: return 4;
    case 29: return 4;
    case 30: return 4;
    case 31: return 5;
    case 32: return 10;
    case 33: return 15;
    case 34: return 20;
    default: return 0;
    }
  }
  return 0;
}


/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(struct char_data * ch)
{int i;

switch (ch->player.chclass)
             {
  case CLASS_CLERIC :
  ch->real_abils.cha = 10;
  do {ch->real_abils.con   = 12+number(0,3);
      ch->real_abils.wis   = 18+number(0,5);
      ch->real_abils.intel = 18+number(0,5);
     } // 57/48 roll 13/9
  while (ch->real_abils.con+ch->real_abils.wis+ch->real_abils.intel != 57);
  do {ch->real_abils.str = 11+number(0,3);
      ch->real_abils.dex = 10+number(0,3);
     } // 92/88 roll 6/4
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex+
         ch->real_abils.wis+ch->real_abils.intel+ch->real_abils.cha != 92);
  /* ADD SPECIFIC STATS */
  ch->real_abils.wis   += 2;
  ch->real_abils.intel += 1;
  GET_HEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(150,180) : number(150,200);
  GET_WEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(120,170) : number(120,180);
  break;

  case CLASS_MAGIC_USER :
  case CLASS_DEFENDERMAGE :
  case CLASS_CHARMMAGE :
  case CLASS_NECROMANCER :
  do {ch->real_abils.str   = 10+number(0,4);
      ch->real_abils.wis   = 17+number(0,5);
      ch->real_abils.intel = 18+number(0,5);
     } // 55/45 roll 14/10
  while (ch->real_abils.str+ch->real_abils.wis+ch->real_abils.intel != 55);
  do {ch->real_abils.con = 10+number(0,3);
      ch->real_abils.dex = 9+number(0,3);
      ch->real_abils.cha = 13+number(0,3);
     } // 92/87 roll 9/5
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex+
         ch->real_abils.wis+ch->real_abils.intel+ch->real_abils.cha != 92);
  /* ADD SPECIFIC STATS */
  ch->real_abils.wis   += 1;
  ch->real_abils.intel += 2;
  GET_HEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(150,170) : number(150,180);
  GET_WEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(120,150) : number(120,180);
  break;

  case CLASS_THIEF :
  do {ch->real_abils.str = 16+number(0,3);
      ch->real_abils.con = 14+number(0,3);
      ch->real_abils.dex = 20+number(0,3);
     } // 57/50 roll 9/7
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex != 57);
  do {ch->real_abils.wis   = 9+number(0,3);
      ch->real_abils.cha   = 12+number(0,3);
      ch->real_abils.intel = 9+number(0,3);
     } // 92/87 roll 9/5
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex+
         ch->real_abils.wis+ch->real_abils.intel+ch->real_abils.cha != 92);
  /* ADD SPECIFIC STATS */
  ch->real_abils.dex += 2;
  ch->real_abils.cha += 1;
  GET_HEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(150,180) : number(150,190);
  GET_WEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(120,170) : number(120,180);
  break;

  case CLASS_WARRIOR :
  ch->real_abils.cha = 10;
  do {ch->real_abils.str = 20+number(0,4);
      ch->real_abils.dex = 8+number(0,3);
      ch->real_abils.con = 20+number(0,3);
     } // 55/48 roll 10/7
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex != 55);
  do {ch->real_abils.intel = 11+number(0,4);
      ch->real_abils.wis   = 11+number(0,4);
     } // 92/87 roll 8/5
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex+
         ch->real_abils.wis+ch->real_abils.intel+ch->real_abils.cha != 92);
  /* ADD SPECIFIC STATS */
  ch->real_abils.str += 1;
  ch->real_abils.con += 2;
  GET_HEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(165,180) : number(170,200);
  GET_WEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(160,180) : number(170,200);
  break;

  case CLASS_ASSASINE :
  ch->real_abils.cha = 12;
  do {ch->real_abils.str = 16+number(0,5);
      ch->real_abils.dex = 18+number(0,5);
      ch->real_abils.con = 14+number(0,5);
     } // 55/48 roll 15/7
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex != 55);
  do {ch->real_abils.intel = 11+number(0,3);
      ch->real_abils.wis   = 11+number(0,3);
     } // 92/87 roll 8/5
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex+
         ch->real_abils.wis+ch->real_abils.intel+ch->real_abils.cha != 92);
  /* ADD SPECIFIC STATS */
  ch->real_abils.str += 1;
  ch->real_abils.con += 1;
  ch->real_abils.dex += 1;
  GET_HEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(150,180) : number(150,200);
  GET_WEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(120,180) : number(150,200);
  break;

  case CLASS_GUARD :
  ch->real_abils.cha = 12;
  do {ch->real_abils.str = 19+number(0,3);
      ch->real_abils.dex = 13+number(0,3);
      ch->real_abils.con = 16+number(0,5);
     } // 55/48 roll 11/7
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex != 55);
  do {ch->real_abils.intel = 10+number(0,4);
      ch->real_abils.wis   = 10+number(0,4);
     } // 92/87 roll 8/5
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex+
         ch->real_abils.wis+ch->real_abils.intel+ch->real_abils.cha != 92);
  /* ADD SPECIFIC STATS */
  ch->real_abils.str += 1;
  ch->real_abils.con += 1;
  ch->real_abils.dex += 1;
  GET_HEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(150,180) : number(150,200);
  GET_WEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(140,170) : number(160,200);
  break;

  case CLASS_PALADINE :
  do {ch->real_abils.str = 18+number(0,3);
      ch->real_abils.wis = 14+number(0,4);
      ch->real_abils.con = 14+number(0,4);
     } // 53/46 roll 11/7
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.wis != 53);
  do {ch->real_abils.intel = 12+number(0,4);
      ch->real_abils.dex   = 10+number(0,3);
      ch->real_abils.cha   = 12+number(0,4);
     } // 92/87 roll 11/5
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex+
         ch->real_abils.wis+ch->real_abils.intel+ch->real_abils.cha != 92);
  /* ADD SPECIFIC STATS */
  ch->real_abils.str   += 1;
  ch->real_abils.cha   += 1;
  ch->real_abils.wis   += 1;
  GET_HEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(150,180) : number(150,200);
  GET_WEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(140,175) : number(140,190);
  break;

  case CLASS_RANGER :

  do {ch->real_abils.str = 18+number(0,6);
      ch->real_abils.dex = 14+number(0,6);
      ch->real_abils.con = 16+number(0,3);
     } // 53/46 roll 12/7
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex != 53);
  do {ch->real_abils.intel = 11+number(0,5);
      ch->real_abils.wis   = 11+number(0,5);
      ch->real_abils.cha   = 11+number(0,5)  ;
     } // 92/85 roll 10/7
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex+
         ch->real_abils.wis+ch->real_abils.intel+ch->real_abils.cha != 92);
  /* ADD SPECIFIC STATS */
  ch->real_abils.str   += 1;
  ch->real_abils.con   += 1;
  ch->real_abils.dex   += 1;
  GET_HEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(150,180) : number(150,200);
  GET_WEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(120,180) : number(120,200);
  break;

  case CLASS_SMITH :
  do {ch->real_abils.str = 18+number(0,5);
      ch->real_abils.dex = 14+number(0,3);
      ch->real_abils.con = 14+number(0,3);
     } // 53/46 roll 11/7
  while (ch->real_abils.str+ch->real_abils.dex+ch->real_abils.con != 53);
  do {ch->real_abils.intel = 10+number(0,3);
      ch->real_abils.wis   = 11+number(0,4);
      ch->real_abils.cha   = 11+number(0,4);
     } // 92/85 roll 11/7
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex+
         ch->real_abils.wis+ch->real_abils.intel+ch->real_abils.cha != 92);
  /* ADD SPECIFIC STATS */
  ch->real_abils.str   += 2;
  ch->real_abils.cha   += 1;
  GET_HEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(160,180) : number(170,200);
  GET_WEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(150,180) : number(170,200);
  break;

  case CLASS_MERCHANT :
  do {ch->real_abils.str = 18+number(0,3);
      ch->real_abils.con = 16+number(0,3);
		ch->real_abils.dex = 14+number(0,3);
     } // 55/48 roll 9/7
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex != 55);
  do {ch->real_abils.wis   = 10+number(0,3);
      ch->real_abils.cha   = 12+number(0,3);
      ch->real_abils.intel = 10+number(0,3);
     } // 92/87 roll 9/5
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex+
         ch->real_abils.wis+ch->real_abils.intel+ch->real_abils.cha != 92);
  /* ADD SPECIFIC STATS */
  ch->real_abils.con += 2;
  ch->real_abils.cha += 1;
  GET_HEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(150,170) : number(150,190);
  GET_WEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(120,180) : number(120,200);
  break;

  case CLASS_DRUID :
  ch->real_abils.cha = 12;
  do {ch->real_abils.con   = 13+number(0,4);
      ch->real_abils.wis   = 15+number(0,3);
      ch->real_abils.intel = 17+number(0,5);
     } // 53/45 roll 12/8
  while (ch->real_abils.con+ch->real_abils.wis+ch->real_abils.intel != 53);
  do {ch->real_abils.str = 14+number(0,3);
      ch->real_abils.dex = 10+number(0,2);
     } // 92/89 roll 5/3
  while (ch->real_abils.str+ch->real_abils.con+ch->real_abils.dex+
         ch->real_abils.wis+ch->real_abils.intel+ch->real_abils.cha != 92);
  /* ADD SPECIFIC STATS */
  ch->real_abils.str   += 1;
  ch->real_abils.intel += 2;
  GET_HEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(150,180) : number(150,190);
  GET_WEIGHT(ch) = GET_SEX(ch) == SEX_FEMALE ? number(120,170) : number(120,180);
  for (i = 1; i <= MAX_SPELLS; i++)
      GET_SPELL_TYPE(ch,i) = SPELL_RUNES;
  break;

  default :
    log("SYSERROR : ATTEMPT STORE ABILITIES FOR UNKNOWN CLASS (Player %s)",
        GET_NAME(ch));
             };
}


/* Some initializations for characters, including initial skills */
void do_start(struct char_data * ch, int newbie)
{ struct obj_data *obj;

  GET_LEVEL(ch) = 1;
  GET_EXP(ch)   = 1;

  set_title(ch, NULL);
  if (newbie)
     roll_real_abils(ch);
  ch->points.max_hit = 10;

  obj = read_object(START_BOTTLE, VIRTUAL);
  if (obj)
     obj_to_char(obj,ch);
  obj = read_object(START_BREAD, VIRTUAL);
  if (obj)
     obj_to_char(obj,ch);
  obj = read_object(START_BREAD, VIRTUAL);
  if (obj)
     obj_to_char(obj,ch);
  obj = read_object(START_SCROLL, VIRTUAL);
  if (obj)
     obj_to_char(obj,ch);
  obj = read_object(START_LIGHT, VIRTUAL);
  if (obj)
     obj_to_char(obj,ch);
  SET_SKILL(ch, SKILL_DRUNKOFF, 10);

  switch (GET_CLASS(ch))
  {
  case CLASS_MAGIC_USER:
  case CLASS_DEFENDERMAGE:
  case CLASS_CHARMMAGE:
  case CLASS_NECROMANCER:
    obj = read_object(START_KNIFE,VIRTUAL);
    if (obj)
       equip_char(ch, obj, WEAR_WIELD);
    SET_SKILL(ch, SKILL_SATTACK,  10);
    break;

  case CLASS_DRUID:
    obj = read_object(START_ERUNE, VIRTUAL);
    if (obj)
       obj_to_char(obj, ch);
    obj = read_object(START_WRUNE, VIRTUAL);
    if (obj)
		 obj_to_char(obj, ch);
    obj = read_object(START_ARUNE, VIRTUAL);
    if (obj)
       obj_to_char(obj, ch);
    obj = read_object(START_FRUNE, VIRTUAL);
    if (obj)
       obj_to_char(obj, ch);

  case CLASS_CLERIC:
    obj = read_object(START_CLUB,VIRTUAL);
    if (obj)
       equip_char(ch, obj, WEAR_WIELD);
    SET_SKILL(ch, SKILL_SATTACK,  10);
    break;

  case CLASS_THIEF:
  case CLASS_ASSASINE:
  case CLASS_MERCHANT:
    obj = read_object(START_KNIFE,VIRTUAL);
    if (obj)
       equip_char(ch, obj, WEAR_WIELD);
    SET_SKILL(ch, SKILL_SATTACK,  75);
    break;

  case CLASS_WARRIOR:
    obj = read_object(START_SWORD,VIRTUAL);
    if (obj)
       equip_char(ch, obj, WEAR_WIELD);
    obj = read_object(START_ARMOR, VIRTUAL);
    if (obj)
       equip_char(ch, obj, WEAR_BODY);
    SET_SKILL(ch, SKILL_SATTACK,  95);
    SET_SKILL(ch, SKILL_HORSE,    10);
    break;

  case CLASS_GUARD:
  case CLASS_PALADINE:
  case CLASS_SMITH:
    obj = read_object(START_SWORD,VIRTUAL);
    if (obj)
       equip_char(ch, obj, WEAR_WIELD);
    obj = read_object(START_ARMOR, VIRTUAL);
    if (obj)
       equip_char(ch, obj, WEAR_BODY);
    SET_SKILL(ch, SKILL_SATTACK,  95);
	 if (GET_CLASS(ch) != CLASS_SMITH)
        SET_SKILL(ch, SKILL_HORSE, 10);
    break;

  case CLASS_RANGER:
    obj = read_object(START_BOW,VIRTUAL);
    if (obj)
       equip_char(ch, obj, WEAR_BOTHS);
    obj = read_object(START_ARMOR, VIRTUAL);
    if (obj)
       equip_char(ch, obj, WEAR_BODY);
    SET_SKILL(ch, SKILL_SATTACK,  95);
    SET_SKILL(ch, SKILL_HORSE,    10);
    break;

  }

  switch (GET_RACE(ch))
  {case CLASS_SEVERANE: break;
   case CLASS_POLANE:   break;
   case CLASS_KRIVICHI: break;
   case CLASS_VATICHI:  break;
   case CLASS_VELANE:   break;
   case CLASS_DREVLANE: break;
  }

  switch (GET_RELIGION(ch))
  { case RELIGION_POLY: break;
    case RELIGION_MONO: break;
  }

  advance_level(ch);
  sprintf(buf, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
  mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);

  GET_HIT(ch)  = GET_REAL_MAX_HIT(ch);
  GET_MOVE(ch) = GET_REAL_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL)   = 24;
  GET_COND(ch, DRUNK)  = 0;

  ch->player.time.played = 0;
  ch->player.time.logon  = time(0);

  if (siteok_everyone)
     SET_BIT(PLR_FLAGS(ch, PLR_SITEOK), PLR_SITEOK);
}



/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
void o_advance_level(struct char_data * ch)
{
  int add_hp_min, add_hp_max, add_move = 0, i;

  add_hp_min = MIN(GET_REAL_CON(ch), GET_REAL_CON(ch) + con_app[GET_REAL_CON(ch)].hitp);
  add_hp_max = MAX(GET_REAL_CON(ch), GET_REAL_CON(ch) + con_app[GET_REAL_CON(ch)].hitp);

  switch (GET_CLASS(ch))
         {
  case CLASS_MAGIC_USER:
  case CLASS_DEFENDERMAGE:
  case CLASS_CHARMMAGE:
  case CLASS_NECROMANCER:
    // Not more then BORN CON - 1
    add_hp_min = MIN(add_hp_min, GET_CON(ch) - 3);
    add_hp_max = MIN(add_hp_max, GET_CON(ch) - 1);
    add_move   = 2;
    break;

  case CLASS_CLERIC:
  case CLASS_DRUID:
    // Not more then BORN CON + 1
    add_hp_min = MIN(add_hp_min, GET_CON(ch) - 2);
    add_hp_max = MIN(add_hp_max, GET_CON(ch) + 1);
    add_move   = 2;
    break;

  case CLASS_THIEF:
  case CLASS_ASSASINE:
  case CLASS_MERCHANT:
    // Not more then BORN CON + 2
    add_hp_min = MIN(add_hp_min, GET_CON(ch) - 1);
    add_hp_max = MIN(add_hp_max, GET_CON(ch) + 2);
    add_move   = number(GET_DEX(ch)/6+1, GET_DEX(ch)/5+1);
    break;

  case CLASS_WARRIOR:
    // Not less then BORN CON
    add_hp_min = MAX(add_hp_min, GET_CON(ch));
    add_move   = number(GET_DEX(ch)/6+1, GET_DEX(ch)/5+1);
    break;

  case CLASS_GUARD:
  case CLASS_RANGER:
  case CLASS_PALADINE:
  case CLASS_SMITH:
    // Not more then BORN CON + 5
    // Not less then BORN CON - 2
    add_hp_min = MAX(add_hp_min, GET_CON(ch)-2);
    add_hp_max = MIN(add_hp_max, GET_CON(ch)+5);
    add_move   = number(GET_DEX(ch)/6+1, GET_DEX(ch)/5+1);
    break;
         }

  add_hp_min           = MIN(add_hp_min, add_hp_max);
  add_hp_min           = MAX(1, add_hp_min);
  add_hp_max           = MAX(1, add_hp_max);
  log("Add hp for %s in range %d..%d", GET_NAME(ch), add_hp_min, add_hp_max);
  ch->points.max_hit  += number(add_hp_min, add_hp_max);
  ch->points.max_move += MAX(1, add_move);

  if (IS_IMMORTAL(ch))
     {for (i = 0; i < 3; i++)
          GET_COND(ch, i) = (char) -1;
      SET_BIT(PRF_FLAGS(ch, PRF_HOLYLIGHT), PRF_HOLYLIGHT);
     }

  save_char(ch, NOWHERE);
}

void o_decrease_level(struct char_data * ch)
{
  int add_hp, add_move=0;

  add_hp     = MAX(MAX(GET_REAL_CON(ch), GET_REAL_CON(ch)+con_app[GET_REAL_CON(ch)].hitp),
                   MAX(GET_CON(ch), GET_CON(ch)+con_app[GET_CON(ch)].hitp));

  switch (GET_CLASS(ch))
         {
  case CLASS_MAGIC_USER:
  case CLASS_DEFENDERMAGE:
  case CLASS_CHARMMAGE:
  case CLASS_NECROMANCER:
    add_move = 2;
    break;

  case CLASS_CLERIC:
  case CLASS_DRUID:
    add_move = 2;
    break;

  case CLASS_THIEF:
  case CLASS_ASSASINE:
  case CLASS_MERCHANT:
    add_move = GET_DEX(ch)/5+1;
    break;

  case CLASS_WARRIOR:
  case CLASS_GUARD:
  case CLASS_PALADINE:
  case CLASS_RANGER:
  case CLASS_SMITH:
    add_move = GET_DEX(ch)/5+1;
    break;
         }

  log("Dec hp for %s set ot %d", GET_NAME(ch), add_hp);
  ch->points.max_hit  -= MIN(ch->points.max_hit, MAX(1, add_hp));
  ch->points.max_move -= MIN(ch->points.max_move,MAX(1, add_move));

  if (!IS_IMMORTAL(ch))
     REMOVE_BIT(PRF_FLAGS(ch, PRF_HOLYLIGHT), PRF_HOLYLIGHT);

  save_char(ch, NOWHERE);
}

void advance_level(struct char_data * ch)
{
  int con, add_hp = 0, add_move = 0, i;

  con    = MAX(class_app[(int)GET_CLASS(ch)].min_con, MIN(GET_CON(ch),class_app[(int)GET_CLASS(ch)].max_con));
  add_hp = class_app[(int)GET_CLASS(ch)].base_con +
           (con - class_app[(int)GET_CLASS(ch)].base_con) * class_app[(int)GET_CLASS(ch)].koef_con / 100 +
           number(1,3);

  switch (GET_CLASS(ch))
         {
  case CLASS_MAGIC_USER:
  case CLASS_DEFENDERMAGE:
  case CLASS_CHARMMAGE:
  case CLASS_NECROMANCER:
    add_move   = 2;
    break;

  case CLASS_CLERIC:
  case CLASS_DRUID:
    add_move   = 2;
    break;

  case CLASS_THIEF:
  case CLASS_ASSASINE:
  case CLASS_MERCHANT:
    add_move   = number(GET_DEX(ch)/6+1, GET_DEX(ch)/5+1);
    break;

  case CLASS_WARRIOR:
    add_move   = number(GET_DEX(ch)/6+1, GET_DEX(ch)/5+1);
    break;

  case CLASS_GUARD:
  case CLASS_RANGER:
  case CLASS_PALADINE:
  case CLASS_SMITH:
    add_move   = number(GET_DEX(ch)/6+1, GET_DEX(ch)/5+1);
    break;
         }

  log("Add hp for %s is %d", GET_NAME(ch), add_hp);
  ch->points.max_hit  += MAX(1,add_hp);
  ch->points.max_move += MAX(1,add_move);

  if (IS_IMMORTAL(ch))
     {for (i = 0; i < 3; i++)
          GET_COND(ch, i) = (char) -1;
      SET_BIT(PRF_FLAGS(ch, PRF_HOLYLIGHT), PRF_HOLYLIGHT);
     }

  save_char(ch, NOWHERE);
}

void decrease_level(struct char_data * ch)
{
  int con, add_hp, add_move=0;

  con   = MAX(class_app[(int)GET_CLASS(ch)].min_con, MIN(GET_CON(ch),class_app[(int)GET_CLASS(ch)].max_con));
  add_hp = class_app[(int)GET_CLASS(ch)].base_con +
           (con - class_app[(int)GET_CLASS(ch)].base_con) * class_app[(int)GET_CLASS(ch)].koef_con / 100 +
           3;

  switch (GET_CLASS(ch))
         {
  case CLASS_MAGIC_USER:
  case CLASS_DEFENDERMAGE:
  case CLASS_CHARMMAGE:
  case CLASS_NECROMANCER:
    add_move = 2;
    break;

  case CLASS_CLERIC:
  case CLASS_DRUID:
    add_move = 2;
    break;

  case CLASS_THIEF:
  case CLASS_ASSASINE:
  case CLASS_MERCHANT:
    add_move = GET_DEX(ch)/5+1;
    break;

  case CLASS_WARRIOR:
  case CLASS_GUARD:
  case CLASS_PALADINE:
  case CLASS_RANGER:
  case CLASS_SMITH:
    add_move = GET_DEX(ch)/5+1;
    break;
         }

  log("Dec hp for %s set ot %d", GET_NAME(ch), add_hp);
  ch->points.max_hit  -= MIN(ch->points.max_hit, MAX(1, add_hp));
  ch->points.max_move -= MIN(ch->points.max_move,MAX(1, add_move));
  GET_WIMP_LEV(ch)     = MAX(0,MIN(GET_WIMP_LEV(ch),GET_REAL_MAX_HIT(ch)/2));
  if (!IS_IMMORTAL(ch))
     REMOVE_BIT(PRF_FLAGS(ch, PRF_HOLYLIGHT), PRF_HOLYLIGHT);

  save_char(ch, NOWHERE);
}


/*
 * This simply calculates the backstab multiplier based on a character's
 * level.  This used to be an array, but was changed to be a function so
 * that it would be easier to add more levels to your MUD.  This doesn't
 * really create a big performance hit because it's not used very often.
 */
int backstab_mult(int level)
{
  if (level <= 0)
     return 1;	  /* level 0 */
  else
  if (level <= 5)
     return 2;	  /* level 1 - 7 */
  else
  if (level <= 10)
     return 3;	  /* level 8 - 13 */
  else
  if (level <= 15)
     return 4;	  /* level 14 - 20 */
  else
  if (level <= 20)
     return 5;	  /* level 21 - 28 */
  else
  if (level <= 25)
     return 6;	  /* level 21 - 28 */
  else
  if (level <= 30)
     return 7;	  /* level 21 - 28 */
  else
  if (level < LVL_GRGOD)
     return 10;	  /* all remaining mortal levels */
  else
     return 20;	  /* immortals */
}


/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */

int invalid_unique(struct char_data *ch, struct obj_data *obj)
{if (!ch             ||
     !obj            ||
     IS_NPC(ch)      ||
     IS_IMMORTAL(ch) ||
     obj->obj_flags.Obj_owner == 0 ||
     obj->obj_flags.Obj_owner == GET_UNIQUE(ch))
    return (FALSE);
 return (TRUE);
}

int invalid_anti_class(struct char_data *ch, struct obj_data *obj)
{ if (IS_NPC(ch) || WAITLESS(ch))
     return (FALSE);
  if (// (IS_OBJ_ANTI(obj, ITEM_ANTI_MONO)     && GET_RELIGION(ch) == RELIGION_MONO) ||
      // (IS_OBJ_ANTI(obj, ITEM_ANTI_POLY)     && GET_RELIGION(ch) == RELIGION_POLY) ||
      (IS_OBJ_ANTI(obj, ITEM_AN_MAGIC)      && IS_MAGIC_USER(ch)) ||
      (IS_OBJ_ANTI(obj, ITEM_AN_CLERIC)     && IS_CLERIC(ch))     ||
      (IS_OBJ_ANTI(obj, ITEM_AN_WARRIOR)    && IS_WARRIOR(ch))    ||
      (IS_OBJ_ANTI(obj, ITEM_AN_GUARD)      && IS_GUARD(ch))      ||
      (IS_OBJ_ANTI(obj, ITEM_AN_THIEF)      && IS_THIEF(ch))      ||
      (IS_OBJ_ANTI(obj, ITEM_AN_ASSASINE)   && IS_ASSASINE(ch))   ||
      (IS_OBJ_ANTI(obj, ITEM_AN_PALADINE)   && IS_PALADINE(ch))   ||
      (IS_OBJ_ANTI(obj, ITEM_AN_RANGER)     && IS_RANGER(ch))     ||
      (IS_OBJ_ANTI(obj, ITEM_AN_SMITH)      && IS_SMITH(ch))      ||
      (IS_OBJ_ANTI(obj, ITEM_AN_MERCHANT)   && IS_MERCHANT(ch))   ||
      (IS_OBJ_ANTI(obj, ITEM_AN_DRUID)      && IS_DRUID(ch))
     )
     return (TRUE);
  return (FALSE);
}

int invalid_no_class(struct char_data *ch, struct obj_data *obj)
{ if (IS_NPC(ch) || WAITLESS(ch))
     return (FALSE);

  if ((IS_OBJ_NO(obj, ITEM_NO_MONO)       && GET_RELIGION(ch) == RELIGION_MONO) ||
      (IS_OBJ_NO(obj, ITEM_NO_POLY)       && GET_RELIGION(ch) == RELIGION_POLY) ||
      (IS_OBJ_NO(obj, ITEM_NO_MAGIC)      && IS_MAGIC_USER(ch)) ||
      (IS_OBJ_NO(obj, ITEM_NO_CLERIC)     && IS_CLERIC(ch))     ||
      (IS_OBJ_NO(obj, ITEM_NO_WARRIOR)    && IS_WARRIOR(ch))    ||
      (IS_OBJ_NO(obj, ITEM_NO_GUARD)      && IS_GUARD(ch))      ||
      (IS_OBJ_NO(obj, ITEM_NO_THIEF)      && IS_THIEF(ch))      ||
      (IS_OBJ_NO(obj, ITEM_NO_ASSASINE)   && IS_ASSASINE(ch))   ||
      (IS_OBJ_NO(obj, ITEM_NO_PALADINE)   && IS_PALADINE(ch))   ||
      (IS_OBJ_NO(obj, ITEM_NO_RANGER)     && IS_RANGER(ch))     ||
      (IS_OBJ_NO(obj, ITEM_NO_SMITH)      && IS_SMITH(ch))      ||
      (IS_OBJ_NO(obj, ITEM_NO_MERCHANT)   && IS_MERCHANT(ch))   ||
      (IS_OBJ_NO(obj, ITEM_NO_DRUID)      && IS_DRUID(ch))      ||
      ((OBJ_FLAGGED(obj, ITEM_ARMORED) || OBJ_FLAGGED(obj, ITEM_SHARPEN)) &&
       !IS_SMITH(ch))
     )
     return (TRUE);
  return (FALSE);
}




/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */
void init_spell_levels(void)
{ FILE * magic;
  char line1[256], line2[256], line3[256], name[256];
  int  i[10], c, j, sp_num;
  if (!(magic = fopen(LIB_MISC"magic.lst","r")))
     {log("Cann't open magic list file...");
      _exit(1);
     }
  while (get_line(magic,name))
        {if (!name[0] || name[0] == ';')
            continue;
         if (sscanf(name, "%s %s %s %d %d",
                      line1, line2, line3,
                      i+1, i+2) != 5)
            {log("Bad format for magic string !\r\n"
                 "Format : <spell name (%%s %%s)> <classes (%%s)> <slot (%%d)> <level (%%d)>");
             _exit(1);
            }

         name[0] = '\0';
         strcat(name,line1);
         if (*line2 != '*')
            {*(name+strlen(name)+1) = '\0';
             *(name+strlen(name)+0) = ' ';
             strcat(name,line2);
            }

         if ((sp_num = find_spell_num(name)) < 0)
            {log("Spell '%s' not found...", name);
             _exit(1);
            }

         for (j = 0; line3[j] && j < NUM_CLASSES; j++)
             {if (!strchr("1xX!",line3[j]))
					  continue;
              if (i[2])
                 {mspell_level(sp_num, 1 << j, i[2]);
                  log("LEVEL set '%s' classes %x value %d",name,j,i[2]);
                 }
              if (i[1])
                 {mspell_slot (sp_num, 1 << j, i[1]);
                  log("SLOT set '%s' classes %x value %d",name,j,i[1]);
                 }
             }
        }
  fclose(magic);
  if (!(magic = fopen(LIB_MISC"items.lst","r")))
     {log("Cann't open items list file...");
      _exit(1);
     }
  while (get_line(magic,name))
        {if (!name[0] || name[0] == ';')
            continue;
         if (sscanf(name, "%s %s %s %d %d %d %d",
                      line1, line2, line3,
                      i, i+1, i+2, i+3) != 7)
            {log("Bad format for magic string !\r\n"
                 "Format : <spell name (%%s %%s)> <type (%%s)> <items_vnum (%%d %%d %%d %%d)>");
             _exit(1);
            }

         name[0] = '\0';
         strcat(name,line1);
         if (*line2 != '*')
            {*(name+strlen(name)+1) = '\0';
             *(name+strlen(name)+0) = ' ';
             strcat(name,line2);
            }
         if ((sp_num = find_spell_num(name)) < 0)
            {log("Spell '%s' not found...", name);
             _exit(1);
            }
         c = strlen(line3);
         if (!strn_cmp(line3,"potion",c))
            {spell_create[sp_num].potion.items[0] = i[0];
             spell_create[sp_num].potion.items[1] = i[1];
             spell_create[sp_num].potion.items[2] = i[2];
             spell_create[sp_num].potion.rnumber  = i[3];
             log("CREATE potion FOR MAGIC '%s'", spell_name(sp_num));
				}
         else
         if (!strn_cmp(line3,"wand",c))
            {spell_create[sp_num].wand.items[0] = i[0];
             spell_create[sp_num].wand.items[1] = i[1];
             spell_create[sp_num].wand.items[2] = i[2];
             spell_create[sp_num].wand.rnumber  = i[3];
             log("CREATE wand FOR MAGIC '%s'", spell_name(sp_num));
            }
         else
         if (!strn_cmp(line3,"scroll",c))
            {spell_create[sp_num].scroll.items[0] = i[0];
             spell_create[sp_num].scroll.items[1] = i[1];
             spell_create[sp_num].scroll.items[2] = i[2];
             spell_create[sp_num].scroll.rnumber  = i[3];
             log("CREATE scroll FOR MAGIC '%s'", spell_name(sp_num));
            }
         else
         if (!strn_cmp(line3,"items",c))
            {spell_create[sp_num].items.items[0] = i[0];
             spell_create[sp_num].items.items[1] = i[1];
             spell_create[sp_num].items.items[2] = i[2];
             spell_create[sp_num].items.rnumber  = i[3];
             log("CREATE items FOR MAGIC '%s'", spell_name(sp_num));
            }
         else
         if (!strn_cmp(line3,"runes",c))
            {spell_create[sp_num].runes.items[0] = i[0];
             spell_create[sp_num].runes.items[1] = i[1];
             spell_create[sp_num].runes.items[2] = i[2];
             spell_create[sp_num].runes.rnumber  = i[3];
             log("CREATE runes FOR MAGIC '%s'", spell_name(sp_num));
            }
         else
            {log("Unknown items option : %s", line3);
             _exit(1);
            }
        }
  fclose(magic);
  if (!(magic = fopen(LIB_MISC"skills.lst","r")))
     {log("Cann't open skills list file...");
      _exit(1);
     }
  while (get_line(magic,name))
        {if (!name[0] || name[0] == ';')
				continue;
         if (sscanf(name, "%s %s %d %d",
                      line1, line2,
                      i, i+1) != 4)
            {log("Bad format for skill string !\r\n"
                 "Format : <skill name (%%s %%s)>  <class (%%d)> <improove (%%d)> !");
             _exit(1);
            }
         name[0] = '\0';
         strcat(name,line1);
         if (*line2 != '*')
            {*(name+strlen(name)+1) = '\0';
             *(name+strlen(name)+0) = ' ';
             strcat(name,line2);
            }
         if ((sp_num = find_skill_num(name)) < 0)
            {log("Skill '%s' not found...", name);
             _exit(1);
            }
         if (i[0] < 0 || i[0] >= NUM_CLASSES)
            {log("Bad class type for skill \"%s\"...", skill_info[sp_num].name);
             _exit(1);
            }
         skill_info[sp_num].k_improove[i[0]] = MAX(1,i[1]);
        }
  fclose(magic);
  return;
}


void init_basic_values(void)
{ FILE * magic;
  char line[256], name[MAX_INPUT_LENGTH];
  int  i[10], c, j, mode=0, *pointer;
  if (!(magic = fopen(LIB_MISC"basic.lst","r")))
     {log("Cann't open basic values list file...");
      return;
     }
  while (get_line(magic,name))
        {if (!name[0] || name[0] == ';')
            continue;
         i[0] = i[1] = i[2] = i[3] = i[4] = i[5] = 100000;
         if (sscanf(name, "%s %d %d %d %d %d %d",
                      line,i,i+1,i+2,i+3,i+4,i+5) < 1)
            continue;
			if (!str_cmp(line,"str"))
            mode = 1;
         else
         if (!str_cmp(line,"dex_app"))
            mode = 2;
         else
         if (!str_cmp(line,"dex_skill"))
            mode = 3;
         else
         if (!str_cmp(line,"con"))
            mode = 4;
         else
         if (!str_cmp(line,"wis"))
            mode = 5;
         else
         if (!str_cmp(line,"int"))
            mode = 6;
         else
         if (!str_cmp(line,"cha"))
            mode = 7;
         else
         if (!str_cmp(line,"size"))
            mode = 8;
         else
         if (!str_cmp(line,"weapon"))
            mode = 9;
         else
         if ((c = atoi(line)) > 0 && c <= 50 && mode > 0 && mode < 10)
            {switch (mode)
             {case 1 : pointer = (int *)&(str_app[c].tohit); break;
              case 2 : pointer = (int *)&(dex_app[c].reaction); break;
              case 3 : pointer = (int *)&(dex_app_skill[c].p_pocket); break;
              case 4 : pointer = (int *)&(con_app[c].hitp); break;
              case 5 : pointer = (int *)&(wis_app[c].spell_additional); break;
              case 6 : pointer = (int *)&(int_app[c].spell_aknowlege); break;
              case 7 : pointer = (int *)&(cha_app[c].leadership); break;
              case 8 : pointer = (int *)&(size_app[c].ac); break;
              case 9 : pointer = (int *)&(weapon_app[c].shocking); break;
              default: pointer= NULL;
             }
             if (pointer)
                {//log("Mode %d - %d = %d %d %d %d %d %d",mode,c,
                 //    *i, *(i+1), *(i+2), *(i+3), *(i+4), *(i+5));
                 for (j = 0; j < 6; j++)
                     if (i[j] != 100000)
								{//log("[%d] %d <-> %d",j,*(pointer+j),*(i+j));
                         *(pointer+j) = *(i+j);
                        }
                 //getchar();
                }
            }
        }
  fclose(magic);
  return;
}


/*
 * This is the exp given to implementors -- it must always be greater
 * than the exp required for immortality, plus at least 20,000 or so.
 */

/* Function to return the exp required for each class/level */
int level_exp(int chclass, int level)
{
  if (level > LVL_IMPL || level < 0)
     {log("SYSERR: Requesting exp for invalid level %d!", level);
      return 0;
     }

  /*
   * Gods have exp close to EXP_MAX.  This statement should never have to
   * changed, regardless of how many mortal or immortal levels exist.
   */
   if (level > LVL_IMMORT)
      {return EXP_IMPL - ((LVL_IMPL - level) * 1000);
      }

  /* Exp required for normal mortals is below */

  switch (chclass) {

    case CLASS_MAGIC_USER:
    case CLASS_DEFENDERMAGE:
    case CLASS_CHARMMAGE:
    case CLASS_NECROMANCER:
    switch (level)
    { case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5000;
      case  4: return 10000;
      case  5: return 20000;
      case  6: return 40000;
      case  7: return 80000;
      case  8: return 160000;
      case  9: return 260000;
      case 10: return 380000;
      case 11: return 520000;
      case 12: return 680000;
      case 13: return 860000;
      case 14: return 1080000;
      case 15: return 1330000;
      case 16: return 1630000;
      case 17: return 1980000;
      case 18: return 2380000;
      case 19: return 2830000;
      case 20: return 3580000;
      case 21: return 4580000;
      case 22: return 5830000;
      case 23: return 7330000;
      case 24: return 9080000;
      case 25: return 11080000;
      case 26: return 15000000;
      case 27: return 22000000;
      case 28: return 33000000;
      case 29: return 47000000;
      case 30: return 64000000;
      /* add new levels here */
      case LVL_IMMORT: return 94000000;
    }
    break;

    case CLASS_CLERIC:
    case CLASS_DRUID:
    switch (level)
    { case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5000;
      case  4: return 10000;
      case  5: return 20000;
      case  6: return 40000;
      case  7: return 80000;
      case  8: return 160000;
      case  9: return 260000;
      case 10: return 380000;
      case 11: return 520000;
      case 12: return 680000;
      case 13: return 860000;
      case 14: return 1080000;
      case 15: return 1330000;
      case 16: return 1630000;
      case 17: return 1980000;
      case 18: return 2380000;
      case 19: return 2830000;
      case 20: return 3580000;
      case 21: return 4580000;
      case 22: return 5830000;
      case 23: return 7330000;
      case 24: return 9080000;
      case 25: return 11080000;
      case 26: return 15000000;
      case 27: return 22000000;
      case 28: return 33000000;
      case 29: return 47000000;
      case 30: return 64000000;
      /* add new levels here */
      case LVL_IMMORT: return 87000000;
    }
    break;

    case CLASS_THIEF:
    case CLASS_ASSASINE:
    case CLASS_MERCHANT:
    switch (level)
    { case  0: return 0;
      case  1: return 1;
      case  2: return 1500;
      case  3: return 3000;
      case  4: return 6000;
      case  5: return 12000;
      case  6: return 24000;
      case  7: return 48000;
      case  8: return 100000;
      case  9: return 170000;
      case 10: return 260000;
      case 11: return 370000;
      case 12: return 500000;
      case 13: return 650000;
      case 14: return 850000;
      case 15: return 1040000;
      case 16: return 1280000;
      case 17: return 1570000;
      case 18: return 1910000;
      case 19: return 2300000;
      case 20: return 2790000;
      case 21: return 3780000;
      case 22: return 5070000;
      case 23: return 6560000;
      case 24: return 8250000;
      case 25: return 10240000;
      case 26: return 13000000;
      case 27: return 20000000;
      case 28: return 30000000;
      case 29: return 43000000;
      case 30: return 59000000;
      /* add new levels here */
      case LVL_IMMORT: return 79000000;
    }
    break;

    case CLASS_WARRIOR:
    case CLASS_GUARD:
    case CLASS_PALADINE:
    case CLASS_RANGER:
    case CLASS_SMITH:
    switch (level)
    { case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 140000;
      case  9: return 220000;
      case 10: return 320000;
      case 11: return 440000;
      case 12: return 580000;
      case 13: return 740000;
      case 14: return 950000;
      case 15: return 1150000;
      case 16: return 1400000;
      case 17: return 1700000;
      case 18: return 2050000;
      case 19: return 2450000;
      case 20: return 2950000;
      case 21: return 3950000;
      case 22: return 5250000;
      case 23: return 6750000;
      case 24: return 8450000;
      case 25: return 10350000;
      case 26: return 14000000;
      case 27: return 21000000;
      case 28: return 31000000;
      case 29: return 44000000;
      case 30: return 64000000;
      /* add new levels here */
      case LVL_IMMORT: return 79000000;
    }
    break;
  }

  /*
   * This statement should never be reached if the exp tables in this function
   * are set up properly.  If you see exp of 123456 then the tables above are
   * incomplete -- so, complete them!
   */
  log("SYSERR: XP tables not set up correctly in class.c!");
  return 123456;
}


/*
 * Default titles of male characters.
 */
const char *title_male(int chclass, int level)
{
  if (level < 0 || level > LVL_IMPL)
     return "усталый путник";
  if (level == LVL_IMPL)
     return "дитя Творца";

  return ("\0");

  switch (chclass)
  { case CLASS_MAGIC_USER:
    case CLASS_DEFENDERMAGE:
	 case CLASS_CHARMMAGE:
    case CLASS_NECROMANCER:
    switch (level)
    { case  1: return "the Apprentice of Magic";
      case  2: return "the Spell Student";
      case  3: return "the Scholar of Magic";
      case  4: return "the Delver in Spells";
      case  5: return "the Medium of Magic";
      case  6: return "the Scribe of Magic";
      case  7: return "the Seer";
      case  8: return "the Sage";
      case  9: return "the Illusionist";
      case 10: return "the Abjurer";
      case 11: return "the Invoker";
      case 12: return "the Enchanter";
      case 13: return "the Conjurer";
      case 14: return "the Magician";
      case 15: return "the Creator";
      case 16: return "the Savant";
      case 17: return "the Magus";
      case 18: return "the Wizard";
      case 19: return "the Warlock";
      case 20: return "the Sorcerer";
      case 21: return "the Necromancer";
      case 22: return "the Thaumaturge";
      case 23: return "the Student of the Occult";
      case 24: return "the Disciple of the Uncanny";
      case 25: return "the Minor Elemental";
      case 26: return "the Greater Elemental";
      case 27: return "the Crafter of Magics";
      case 28: return "the Shaman";
      case 29: return "the Keeper of Talismans";
      case 30: return "the Archmage";
      case LVL_IMMORT: return "the Immortal Warlock";
      case LVL_GOD: return "the Avatar of Magic";
      case LVL_GRGOD: return "the God of Magic";
      default: return "the Mage";
    }
    break;

    case CLASS_CLERIC:
    case CLASS_DRUID:
    switch (level)
    { case  1: return "the Believer";
      case  2: return "the Attendant";
      case  3: return "the Acolyte";
      case  4: return "the Novice";
      case  5: return "the Missionary";
      case  6: return "the Adept";
      case  7: return "the Deacon";
      case  8: return "the Vicar";
      case  9: return "the Priest";
      case 10: return "the Minister";
      case 11: return "the Canon";
      case 12: return "the Levite";
      case 13: return "the Curate";
      case 14: return "the Monk";
      case 15: return "the Healer";
      case 16: return "the Chaplain";
      case 17: return "the Expositor";
      case 18: return "the Bishop";
      case 19: return "the Arch Bishop";
      case 20: return "the Patriarch";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Cardinal";
      case LVL_GOD: return "the Inquisitor";
      case LVL_GRGOD: return "the God of good and evil";
      default: return "the Cleric";
    }
    break;

    case CLASS_THIEF:
    case CLASS_ASSASINE:
    case CLASS_MERCHANT:
    switch (level)
    { case  1: return "the Pilferer";
      case  2: return "the Footpad";
      case  3: return "the Filcher";
      case  4: return "the Pick-Pocket";
      case  5: return "the Sneak";
      case  6: return "the Pincher";
      case  7: return "the Cut-Purse";
      case  8: return "the Snatcher";
      case  9: return "the Sharper";
      case 10: return "the Rogue";
      case 11: return "the Robber";
      case 12: return "the Magsman";
      case 13: return "the Highwayman";
      case 14: return "the Burglar";
      case 15: return "the Thief";
      case 16: return "the Knifer";
      case 17: return "the Quick-Blade";
      case 18: return "the Killer";
      case 19: return "the Brigand";
      case 20: return "the Cut-Throat";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Assasin";
      case LVL_GOD: return "the Demi God of thieves";
      case LVL_GRGOD: return "the God of thieves and tradesmen";
      default: return "the Thief";
    }
    break;

    case CLASS_WARRIOR:
    case CLASS_GUARD:
    case CLASS_PALADINE:
    case CLASS_RANGER:
    case CLASS_SMITH:
    switch(level)
    { case  1: return "the Swordpupil";
      case  2: return "the Recruit";
      case  3: return "the Sentry";
      case  4: return "the Fighter";
      case  5: return "the Soldier";
      case  6: return "the Warrior";
      case  7: return "the Veteran";
      case  8: return "the Swordsman";
      case  9: return "the Fencer";
      case 10: return "the Combatant";
      case 11: return "the Hero";
      case 12: return "the Myrmidon";
      case 13: return "the Swashbuckler";
      case 14: return "the Mercenary";
      case 15: return "the Swordmaster";
      case 16: return "the Lieutenant";
      case 17: return "the Champion";
      case 18: return "the Dragoon";
      case 19: return "the Cavalier";
      case 20: return "the Knight";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Warlord";
      case LVL_GOD: return "the Extirpator";
      case LVL_GRGOD: return "the God of war";
      default: return "the Warrior";
    }
    break;
  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}


/*
 * Default titles of female characters.
 */
const char *title_female(int chclass, int level)
{
  if (level < 0 || level > LVL_IMPL)
     return "усталая путница";
  if (level == LVL_IMPL)
     return "дочь Солнца";

  return ("\0");
  switch (chclass)
  { case CLASS_MAGIC_USER:
    case CLASS_DEFENDERMAGE:
    case CLASS_CHARMMAGE:
    case CLASS_NECROMANCER:
    switch (level) {
      case  1: return "the Apprentice of Magic";
      case  2: return "the Spell Student";
      case  3: return "the Scholar of Magic";
      case  4: return "the Delveress in Spells";
      case  5: return "the Medium of Magic";
      case  6: return "the Scribess of Magic";
      case  7: return "the Seeress";
      case  8: return "the Sage";
      case  9: return "the Illusionist";
      case 10: return "the Abjuress";
      case 11: return "the Invoker";
      case 12: return "the Enchantress";
      case 13: return "the Conjuress";
      case 14: return "the Witch";
      case 15: return "the Creator";
      case 16: return "the Savant";
      case 17: return "the Craftess";
      case 18: return "the Wizard";
      case 19: return "the War Witch";
      case 20: return "the Sorceress";
      case 21: return "the Necromancress";
      case 22: return "the Thaumaturgess";
      case 23: return "the Student of the Occult";
      case 24: return "the Disciple of the Uncanny";
      case 25: return "the Minor Elementress";
      case 26: return "the Greater Elementress";
      case 27: return "the Crafter of Magics";
      case 28: return "Shaman";
      case 29: return "the Keeper of Talismans";
      case 30: return "Archwitch";
      case LVL_IMMORT: return "the Immortal Enchantress";
      case LVL_GOD: return "the Empress of Magic";
      case LVL_GRGOD: return "the Goddess of Magic";
      default: return "the Witch";
    }
    break;

    case CLASS_CLERIC:
    case CLASS_DRUID:
    switch (level) {
      case  1: return "the Believer";
      case  2: return "the Attendant";
      case  3: return "the Acolyte";
      case  4: return "the Novice";
      case  5: return "the Missionary";
      case  6: return "the Adept";
      case  7: return "the Deaconess";
      case  8: return "the Vicaress";
      case  9: return "the Priestess";
      case 10: return "the Lady Minister";
      case 11: return "the Canon";
      case 12: return "the Levitess";
      case 13: return "the Curess";
      case 14: return "the Nunne";
      case 15: return "the Healess";
      case 16: return "the Chaplain";
      case 17: return "the Expositress";
      case 18: return "the Bishop";
      case 19: return "the Arch Lady of the Church";
      case 20: return "the Matriarch";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Priestess";
      case LVL_GOD: return "the Inquisitress";
      case LVL_GRGOD: return "the Goddess of good and evil";
      default: return "the Cleric";
    }
    break;

    case CLASS_THIEF:
    case CLASS_ASSASINE:
    case CLASS_MERCHANT:
    switch (level) {
      case  1: return "the Pilferess";
      case  2: return "the Footpad";
      case  3: return "the Filcheress";
      case  4: return "the Pick-Pocket";
      case  5: return "the Sneak";
      case  6: return "the Pincheress";
      case  7: return "the Cut-Purse";
      case  8: return "the Snatcheress";
      case  9: return "the Sharpress";
      case 10: return "the Rogue";
      case 11: return "the Robber";
      case 12: return "the Magswoman";
      case 13: return "the Highwaywoman";
      case 14: return "the Burglaress";
      case 15: return "the Thief";
      case 16: return "the Knifer";
      case 17: return "the Quick-Blade";
      case 18: return "the Murderess";
      case 19: return "the Brigand";
      case 20: return "the Cut-Throat";
      case 34: return "the Implementress";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Assasin";
      case LVL_GOD: return "the Demi Goddess of thieves";
      case LVL_GRGOD: return "the Goddess of thieves and tradesmen";
      default: return "the Thief";
    }
    break;

    case CLASS_WARRIOR:
    case CLASS_GUARD:
    case CLASS_PALADINE:
    case CLASS_RANGER:
    case CLASS_SMITH:
    switch(level) {
      case  1: return "the Swordpupil";
      case  2: return "the Recruit";
      case  3: return "the Sentress";
      case  4: return "the Fighter";
      case  5: return "the Soldier";
      case  6: return "the Warrior";
      case  7: return "the Veteran";
      case  8: return "the Swordswoman";
      case  9: return "the Fenceress";
      case 10: return "the Combatess";
      case 11: return "the Heroine";
      case 12: return "the Myrmidon";
      case 13: return "the Swashbuckleress";
      case 14: return "the Mercenaress";
      case 15: return "the Swordmistress";
      case 16: return "the Lieutenant";
      case 17: return "the Lady Champion";
      case 18: return "the Lady Dragoon";
      case 19: return "the Cavalier";
      case 20: return "the Lady Knight";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Lady of War";
      case LVL_GOD: return "the Queen of Destruction";
      case LVL_GRGOD: return "the Goddess of war";
      default: return "the Warrior";
    }
    break;
  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}
