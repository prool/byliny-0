/* ************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


/* external declarations and prototypes **********************************/

extern struct weather_data weather_info;
extern FILE *logfile;
extern char AltToKoi[];
extern char KoiToAlt[];
extern char WinToKoi[];
extern char KoiToWin[];
extern char KoiToWin2[];
extern char AltToLat[];

#define log			basic_mud_log

/* public functions in utils.c */
char	*str_dup(const char *source);
int	str_cmp(const char *arg1, const char *arg2);
int	strn_cmp(const char *arg1, const char *arg2, int n);
void	basic_mud_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
int	touch(const char *path);
void	mudlog(const char *str, int type, int level, int file);
void	log_death_trap(struct char_data *ch);
int	number(int from, int to);
int	dice(int number, int size);
void	sprintbit(bitvector_t vektor, const char *names[], char *result);
void	sprintbits(struct new_flag flags, const char *names[], char *result, char *div);
void	sprinttype(int type, const char *names[], char *result);
int	get_line(FILE *fl, char *buf);
int	get_filename(char *orig_name, char *filename, int mode);
struct time_info_data *age(struct char_data *ch);
int	num_pc_in_room(struct room_data *room);
void	core_dump_real(const char *, int);
int   replace_str(char **string, char *pattern, char *replacement, int rep_all, int max_size);
void  format_text(char **ptr_string, int mode, struct descriptor_data *d, int maxlen);
int     check_moves(struct char_data *ch, int how_moves);
void    to_koi(char *str,int);
void    from_koi(char *str, int);
int     real_sector(int room);
#define core_dump()		core_dump_real(__FILE__, __LINE__)

/* random functions in random.c */
void circle_srandom(unsigned long initial_seed);
unsigned long circle_random(void);

/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

#define SF_EMPTY       (1 << 0)
#define SF_FOLLOWERDIE (1 << 1)
#define SF_MASTERDIE   (1 << 2)
#define SF_CHARMLOST   (1 << 3)

#define EXP_IMPL 100000000
#define MAX_AUCTION_LOT            3
#define MAX_AUCTION_TACT_BUY       5
#define MAX_AUCTION_TACT_PRESENT   (MAX_AUCTION_TACT_BUY + 3)
#define AUCTION_PULSES             30
#define CHAR_DRUNKED               10
#define CHAR_MORTALLY_DRUNKED      16

int MAX(int a, int b);
int MIN(int a, int b);
char *CAP(char *txt);

#define KtoW(c) ((ubyte)(c) < 128 ? (c) : KoiToWin[(ubyte)(c)-128])
#define KtoW2(c) ((ubyte)(c) < 128 ? (c) : KoiToWin2[(ubyte)(c)-128])
#define KtoA(c) ((ubyte)(c) < 128 ? (c) : KoiToAlt[(ubyte)(c)-128])
#define WtoK(c) ((ubyte)(c) < 128 ? (c) : WinToKoi[(ubyte)(c)-128])
#define AtoK(c) ((ubyte)(c) < 128 ? (c) : AltToKoi[(ubyte)(c)-128])
#define AtoL(c) ((ubyte)(c) < 128 ? (c) : AltToLat[(ubyte)(c)-128])

/* in magic.c */
bool	circle_follow(struct char_data *ch, struct char_data * victim);

/* in act.informative.c */
void	look_at_room(struct char_data *ch, int mode);

/* in act.movmement.c */
int	do_simple_move(struct char_data *ch, int dir, int following);
int	perform_move(struct char_data *ch, int dir, int following, int checkmob);

/* in limits.c */
int	mana_gain(struct char_data *ch);
int	hit_gain(struct char_data *ch);
int	move_gain(struct char_data *ch);
void	advance_level(struct char_data *ch);
void	set_title(struct char_data *ch, char *title);
void	gain_exp(struct char_data *ch, int gain);
void	gain_exp_regardless(struct char_data *ch, int gain);
void	gain_condition(struct char_data *ch, int condition, int value);
void	check_idling(struct char_data *ch);
void	point_update(void);
void	update_pos(struct char_data *victim);


/* various constants *****************************************************/

/* defines for mudlog() */
#define OFF	0
#define BRF	1
#define NRM	2
#define CMP	3

/* get_filename() */
#define CRASH_FILE	  0
#define ETEXT_FILE	  1
#define ALIAS_FILE	  2
#define SCRIPT_VARS_FILE  3
#define PLAYERS_FILE      4
#define PKILLERS_FILE     5
#define PQUESTS_FILE      6
#define PMKILL_FILE       7
#define TEXT_CRASH_FILE   8
#define TIME_CRASH_FILE   9

/* breadth-first searching */
#define BFS_ERROR		    -1
#define BFS_ALREADY_THERE	-2
#define BFS_NO_PATH		    -3
#define NUM_PADS             6
/*
 * XXX: These constants should be configurable. See act.informative.c
 *	and utils.c for other places to change.
 */
/* mud-life time */
#define HOURS_PER_DAY          24
#define DAYS_PER_MONTH         30
#define MONTHS_PER_YEAR        12
#define SECS_PER_PLAYER_AFFECT 2
#define TIME_KOEFF             2
#define SECS_PER_MUD_HOUR	   60
#define SECS_PER_MUD_DAY	   (HOURS_PER_DAY*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH	   (DAYS_PER_MONTH*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR	   (MONTHS_PER_YEAR*SECS_PER_MUD_MONTH)
#define NEWMOONSTART               27
#define NEWMOONSTOP                1
#define HALFMOONSTART              7
#define FULLMOONSTART              13
#define FULLMOONSTOP               15
#define LASTHALFMOONSTART          21
#define MOON_CYCLE				   28
#define WEEK_CYCLE				   7
#define POLY_WEEK_CYCLE			   9


/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN	60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)


#define GET_COMMSTATE(ch)      ((ch)->player_specials->saved.Prelimit)
#define SET_COMMSTATE(ch,val)  ((ch)->player_specials->saved.Prelimit = (val))
#define IS_IMMORTAL(ch)     (!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_IMMORT || GET_COMMSTATE(ch)))
#define IS_GOD(ch)          (!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_GOD    || GET_COMMSTATE(ch)))
#define IS_GRGOD(ch)        (!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_GRGOD  || GET_COMMSTATE(ch)))
#define IS_IMPL(ch)         (!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_IMPL   || GET_COMMSTATE(ch)))
#define IS_HIGHGOD(ch)      (IS_IMPL(ch) && (GET_GOD_FLAG(ch,GF_HIGHGOD) || GET_COMMSTATE(ch)))

#define IS_BITS(mask, bitno) (IS_SET(mask,(1 << bitno)))
#define IS_CASTER(ch)        (IS_BITS(MASK_CASTER,GET_CLASS(ch)))
#define IS_MAGE(ch)          (IS_BITS(MASK_MAGES, GET_CLASS(ch)))

/* string utils **********************************************************/


#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define LOWER(c)   (a_lcc(c))
#define UPPER(c)   (a_ucc(c))
#define LATIN(c)   (a_lat(c))

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')
#define IF_STR(st) ((st) ? (st) : "\0")

/* memory utils **********************************************************/


#define CREATE(result, type, number)  do {\
	if ((number) * sizeof(type) <= 0)	\
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);	\
	if (!((result) = (type *) calloc ((number), sizeof(type))))	\
		{ perror("SYSERR: malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("SYSERR: realloc failure"); abort(); } } while(0)

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      temp = head;			\
      while (temp && (temp->next != (item))) \
	 temp = temp->next;		\
      if (temp)				\
         temp->next = (item)->next;	\
   }					\


/* basic bitvector utils *************************************************/


#define IS_SET(flag,bit)  ((flag & 0x3FFFFFFF) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit & 0x3FFFFFFF))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit & 0x3FFFFFFF))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit & 0x3FFFFFFF))

/*
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'.
 */
#if 0
/* Subtle bug in the '#var', but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
	(*(((ch)->player_specials == &dummy_mob) ? (log("SYSERR: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var)	(var)
#endif

#define MOB_FLAGS(ch,flag)  (GET_FLAG((ch)->char_specials.saved.act,flag))
#define PLR_FLAGS(ch,flag)  (GET_FLAG((ch)->char_specials.saved.act,flag))
#define PRF_FLAGS(ch,flag)  (GET_FLAG((ch)->player_specials->saved.pref, flag))
#define AFF_FLAGS(ch,flag)  (GET_FLAG((ch)->char_specials.saved.affected_by, flag))
#define NPC_FLAGS(ch,flag)  (GET_FLAG((ch)->mob_specials.npc_flags, flag))
#define EXTRA_FLAGS(ch,flag)(GET_FLAG((ch)->Temporary, flag))
#define ROOM_FLAGS(loc,flag)(GET_FLAG(world[(loc)].room_flags, flag))
#define SPELL_ROUTINES(spl) (spell_info[spl].routines)

/* See http://www.circlemud.org/~greerga/todo.009 to eliminate MOB_ISNPC. */
#define IS_NPC(ch)	        (IS_SET(MOB_FLAGS(ch, MOB_ISNPC), MOB_ISNPC))
#define IS_MOB(ch)          (IS_NPC(ch) && GET_MOB_RNUM(ch) >= 0)

#define MOB_FLAGGED(ch, flag)   (IS_NPC(ch) && IS_SET(MOB_FLAGS(ch,flag), (flag)))
#define PLR_FLAGGED(ch, flag)   (!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch,flag), (flag)))
#define AFF_FLAGGED(ch, flag)   (IS_SET(AFF_FLAGS(ch,flag), (flag)))
#define PRF_FLAGGED(ch, flag)   (IS_SET(PRF_FLAGS(ch,flag), (flag)))
#define NPC_FLAGGED(ch, flag)   (IS_SET(NPC_FLAGS(ch,flag), (flag)))
#define EXTRA_FLAGGED(ch, flag) (IS_SET(EXTRA_FLAGS(ch,flag), (flag)))
#define ROOM_FLAGGED(loc, flag) (IS_SET(ROOM_FLAGS((loc),(flag)), (flag)))
#define EXIT_FLAGGED(exit, flag)     (IS_SET((exit)->exit_info, (flag)))
#define OBJVAL_FLAGGED(obj, flag)    (IS_SET(GET_OBJ_VAL((obj), 1), (flag)))
#define OBJWEAR_FLAGGED(obj, flag)   (IS_SET((obj)->obj_flags.wear_flags, (flag)))
#define OBJ_FLAGGED(obj, flag)       (IS_SET(GET_OBJ_EXTRA(obj,flag), (flag)))
#define HAS_SPELL_ROUTINE(spl, flag) (IS_SET(SPELL_ROUTINES(spl), (flag)))
#define IS_FLY(ch)                   (AFF_FLAGGED(ch,AFF_FLY))
#define SET_EXTRA(ch,skill,vict)   ({(ch)->extra_attack.used_skill = skill; \
                                     (ch)->extra_attack.victim     = vict;})
#define GET_EXTRA_SKILL(ch)          ((ch)->extra_attack.used_skill)
#define GET_EXTRA_VICTIM(ch)         ((ch)->extra_attack.victim)
#define SET_CAST(ch,snum,dch,dobj)   ({(ch)->cast_attack.spellnum  = snum; \
                                       (ch)->cast_attack.tch       = dch; \
                                       (ch)->cast_attack.tobj      = dobj;})
#define GET_CAST_SPELL(ch)         ((ch)->cast_attack.spellnum)
#define GET_CAST_CHAR(ch)          ((ch)->cast_attack.tch)
#define GET_CAST_OBJ(ch)           ((ch)->cast_attack.tobj)



/* IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED(ch, skill))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT(PLR_FLAGS(ch, flag), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch, flag), (flag))) & (flag))


/* room utils ************************************************************/


#define SECT(room)	(world[(room)].sector_type)
#define GET_ROOM_SKY(room) (world[room].weather.duration > 0 ? \
                            world[room].weather.sky : weather_info.sky)

#define IS_MOONLIGHT(room) ((GET_ROOM_SKY(room) == SKY_LIGHTNING && \
                             weather_info.moon_day >= FULLMOONSTART && \
                             weather_info.moon_day <= FULLMOONSTOP))

#define IS_TIMEDARK(room)  ((world[room].gdark > world[room].glight) || \
                            (!(world[room].light+world[room].fires+world[room].glight) && \
                              (ROOM_FLAGGED(room, ROOM_DARK) || \
                              (SECT(room) != SECT_INSIDE && \
                               SECT(room) != SECT_CITY   && \
                               ( weather_info.sunlight == SUN_SET || \
                                 weather_info.sunlight == SUN_DARK )) ) ) )

#define IS_DEFAULTDARK(room) (ROOM_FLAGGED(room, ROOM_DARK) || \
                              (SECT(room) != SECT_INSIDE && \
                               SECT(room) != SECT_CITY   && \
                               ( weather_info.sunlight == SUN_SET || \
                                 weather_info.sunlight == SUN_DARK )) )

                            
#define IS_DARK(room)      ((world[room].gdark > world[room].glight) || \
                            (!(world[room].gdark < world[room].glight) && \
                             !(world[room].light+world[room].fires) && \
                              (ROOM_FLAGGED(room, ROOM_DARK) || \
                              (SECT(room) != SECT_INSIDE && \
                               SECT(room) != SECT_CITY   && \
                               ( weather_info.sunlight == SUN_SET || \
                                 (weather_info.sunlight == SUN_DARK && \
                                  !IS_MOONLIGHT(room)) )) ) ) )

#define IS_DARKTHIS(room)      ((world[room].gdark > world[room].glight) || \
                                (!(world[room].gdark < world[room].glight) && \
                                 !(world[room].light+world[room].fires) && \
                                  (ROOM_FLAGGED(room, ROOM_DARK) || \
                                  (SECT(room) != SECT_INSIDE && \
                                   SECT(room) != SECT_CITY   && \
                                   ( weather_info.sunlight == SUN_DARK && \
                                     !IS_MOONLIGHT(room) )) ) ) )

#define IS_DARKSIDE(room)      ((world[room].gdark > world[room].glight) || \
                                (!(world[room].gdark < world[room].glight) && \
                                 !(world[room].light+world[room].fires) && \
                                  (ROOM_FLAGGED(room, ROOM_DARK) || \
                                  (SECT(room) != SECT_INSIDE && \
                                   SECT(room) != SECT_CITY   && \
                                   ( weather_info.sunlight == SUN_SET  || \
                                     weather_info.sunlight == SUN_RISE || \
                                     (weather_info.sunlight == SUN_DARK && \
                                      !IS_MOONLIGHT(room) )) ) ) )


#define IS_LIGHT(room)     (!IS_DARK(room))

#define VALID_RNUM(rnum)	((rnum) >= 0 && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) \
	((room_vnum)(VALID_RNUM(rnum) ? world[(rnum)].number : NOWHERE))
#define GET_ROOM_SPEC(room) (VALID_RNUM(room) ? world[(room)].func : NULL)

/* char utils ************************************************************/


#define IN_ROOM(ch)	((ch)->in_room)
#define GET_WAS_IN(ch)	((ch)->was_in_room)
#define GET_AGE(ch)     (age(ch)->year)
#define GET_REAL_AGE(ch) (age(ch)->year + GET_AGE_ADD(ch))

#define GET_PC_NAME(ch)	((ch)->player.name)
#define GET_NAME(ch)    (IS_NPC(ch) ? \
			 (ch)->player.short_descr : GET_PC_NAME(ch))
#define GET_HELPER(ch)  ((ch)->helpers)			
#define GET_TITLE(ch)   ((ch)->player.title)
#define GET_LEVEL(ch)   ((ch)->player.level)
#define GET_MANA_NEED(ch)     ((ch)->ManaMemNeeded)
#define GET_MANA_STORED(ch)   ((ch)->ManaMemStored)
#define INITIATIVE(ch)        ((ch)->Initiative)
#define BATTLECNTR(ch)        ((ch)->BattleCounter)
#define EXTRACT_TIMER(ch)     ((ch)->ExtractTimer)
#define CHECK_AGRO(ch)        ((ch)->CheckAggressive)
#define WAITLESS(ch)          (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE))
#define CLR_MEMORY(ch)  (memset((ch)->Memory,0,MAX_SPELLS+1))
#define FORGET_ALL(ch) ({CLR_MEMORY(ch);memset((ch)->real_abils.SplMem,0,MAX_SPELLS+1);})
#define GET_PASSWD(ch)	 ((ch)->player.passwd)
#define GET_PFILEPOS(ch) ((ch)->pfilepos)
#define IS_KILLER(ch)    ((ch)->points.pk_counter)

/**** Adding by me */
#define GET_AF_BATTLE(ch,flag) (IS_SET(GET_FLAG((ch)->BattleAffects, flag),flag))
#define SET_AF_BATTLE(ch,flag) (SET_BIT(GET_FLAG((ch)->BattleAffects,flag),flag))
#define CLR_AF_BATTLE(ch,flag) (REMOVE_BIT(GET_FLAG((ch)->BattleAffects, flag),flag))
#define NUL_AF_BATTLE(ch)      (GET_FLAG((ch)->BattleAffects, 0) = \
                                GET_FLAG((ch)->BattleAffects, INT_ONE) = \
                                GET_FLAG((ch)->BattleAffects, INT_TWO) = \
                                GET_FLAG((ch)->BattleAffects, INT_THREE) = 0)
#define GET_GLORY(ch)          ((ch)->player_specials->saved.glory)
#define GET_REMORT(ch)         ((ch)->player_specials->saved.Remorts)
#define GET_EMAIL(ch)          ((ch)->player_specials->saved.EMail)
#define GET_GOD_FLAG(ch,flag)  (IS_SET((ch)->player_specials->saved.GodsLike,flag))
#define SET_GOD_FLAG(ch,flag)  (SET_BIT((ch)->player_specials->saved.GodsLike,flag))
#define CLR_GOD_FLAG(ch,flag)  (REMOVE_BIT((ch)->player_specials->saved.GodsLike,flag))
#define GET_UNIQUE(ch)         ((ch)->player_specials->saved.unique)
#define GET_HOUSE_UID(ch)      ((ch)->player_specials->saved.HouseUID)
#define GET_HOUSE_RANK(ch)     ((ch)->player_specials->saved.HouseRank)
#define LAST_LOGON(ch)         ((ch)->player_specials->saved.LastLogon)

#define GODS_DURATION(ch)  ((ch)->player_specials->saved.GodsDuration)
#define MUTE_DURATION(ch)  ((ch)->player_specials->saved.MuteDuration)
#define FREEZE_DURATION(ch)  ((ch)->player_specials->saved.FreezeDuration)
#define HELL_DURATION(ch)  ((ch)->player_specials->saved.HellDuration)
#define NAME_DURATION(ch)  ((ch)->player_specials->saved.NameDuration)
/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))

#define POSI(val)      ((val < 50) ? ((val > 0) ? val : 1) : 50)
#define VPOSI(val,min,max)      ((val < max) ? ((val > min) ? val : min) : max)
#define GET_CLASS(ch)   ((ch)->player.chclass)
#define GET_HOME(ch)	((ch)->player.hometown)
#define GET_HEIGHT(ch)	((ch)->player.height)
#define GET_HEIGHT_ADD(ch) ((ch)->add_abils.height_add)
#define GET_REAL_HEIGHT(ch) (GET_HEIGHT(ch) + GET_HEIGHT_ADD(ch))
#define GET_WEIGHT(ch)	((ch)->player.weight)
#define GET_WEIGHT_ADD(ch) ((ch)->add_abils.weight_add)
#define GET_REAL_WEIGHT(ch) (GET_WEIGHT(ch) + GET_WEIGHT_ADD(ch))
#define GET_SEX(ch)	((ch)->player.sex)
#define GET_LOWS(ch) ((ch)->player.Lows)
#define GET_RELIGION(ch) ((ch)->player.Religion)
#define GET_RACE(ch) ((ch)->player.Race)
#define GET_SIDE(ch) ((ch)->player.Side)
#define GET_PAD(ch,i)    ((ch)->player.PNames[i])
#define GET_DRUNK_STATE(ch) ((ch)->player_specials->saved.DrunkState)

#define GET_STR(ch)     ((ch)->real_abils.str)
#define GET_STR_ADD(ch) ((ch)->add_abils.str_add)
#define GET_REAL_STR(ch) (POSI(GET_STR(ch) + GET_STR_ADD(ch)))
#define GET_DEX(ch)     ((ch)->real_abils.dex)
#define GET_DEX_ADD(ch) ((ch)->add_abils.dex_add)
#define GET_REAL_DEX(ch) (POSI(GET_DEX(ch)+GET_DEX_ADD(ch)))
#define GET_INT(ch)     ((ch)->real_abils.intel)
#define GET_INT_ADD(ch) ((ch)->add_abils.intel_add)
#define GET_REAL_INT(ch) (POSI(GET_INT(ch) + GET_INT_ADD(ch)))
#define GET_WIS(ch)     ((ch)->real_abils.wis)
#define GET_WIS_ADD(ch) ((ch)->add_abils.wis_add)
#define GET_REAL_WIS(ch) (POSI(GET_WIS(ch) + GET_WIS_ADD(ch)))
#define GET_CON(ch)     ((ch)->real_abils.con)
#define GET_CON_ADD(ch) ((ch)->add_abils.con_add)
#define GET_REAL_CON(ch) (POSI(GET_CON(ch) + GET_CON_ADD(ch)))
#define GET_CHA(ch)     ((ch)->real_abils.cha)
#define GET_CHA_ADD(ch) ((ch)->add_abils.cha_add)
#define GET_REAL_CHA(ch) (POSI(GET_CHA(ch) + GET_CHA_ADD(ch)))
#define GET_SIZE(ch)    ((ch)->real_abils.size)
#define GET_SIZE_ADD(ch)  ((ch)->add_abils.size_add)
#define GET_REAL_SIZE(ch) (VPOSI(GET_SIZE(ch) + GET_SIZE_ADD(ch), 1, 100))
#define GET_POS_SIZE(ch)  (POSI(GET_REAL_SIZE(ch) >> 1))
#define GET_HR(ch)	      ((ch)->real_abils.hitroll)
#define GET_HR_ADD(ch)    ((ch)->add_abils.hr_add)
#define GET_REAL_HR(ch)   (VPOSI(GET_HR(ch)+GET_HR_ADD(ch), -50, 50))
#define GET_DR(ch)	      ((ch)->real_abils.damroll)
#define GET_DR_ADD(ch)    ((ch)->add_abils.dr_add)
#define GET_REAL_DR(ch)   (VPOSI(GET_DR(ch)+GET_DR_ADD(ch), -50, 50))
#define GET_AC(ch)	      ((ch)->real_abils.armor)
#define GET_AC_ADD(ch)    ((ch)->add_abils.ac_add)
#define GET_REAL_AC(ch)      (GET_AC(ch)+GET_AC_ADD(ch))
#define GET_MORALE(ch)       ((ch)->add_abils.morale_add)
#define GET_INITIATIVE(ch)   ((ch)->add_abils.initiative_add)
#define GET_POISON(ch)		 ((ch)->add_abils.poison_add)
#define GET_CAST_SUCCESS(ch) ((ch)->add_abils.cast_success)
#define GET_PRAY(ch)         ((ch)->add_abils.pray_add)

#define GET_EXP(ch)	          ((ch)->points.exp)
#define GET_HIT(ch)	          ((ch)->points.hit)
#define GET_MAX_HIT(ch)	      ((ch)->points.max_hit)
#define GET_REAL_MAX_HIT(ch)  (GET_MAX_HIT(ch) + GET_HIT_ADD(ch))
#define GET_MOVE(ch)	      ((ch)->points.move)
#define GET_MAX_MOVE(ch)      ((ch)->points.max_move)
#define GET_REAL_MAX_MOVE(ch) (GET_MAX_MOVE(ch) + GET_MOVE_ADD(ch))
#define GET_GOLD(ch)	      ((ch)->points.gold)
#define GET_BANK_GOLD(ch)     ((ch)->points.bank_gold)


#define GET_MANAREG(ch)	  ((ch)->add_abils.manareg)
#define GET_HITREG(ch)    ((ch)->add_abils.hitreg)
#define GET_MOVEREG(ch)   ((ch)->add_abils.movereg)
#define GET_ARMOUR(ch)    ((ch)->add_abils.armour)
#define GET_ABSORBE(ch)   ((ch)->add_abils.absorb)
#define GET_AGE_ADD(ch)   ((ch)->add_abils.age_add)
#define GET_HIT_ADD(ch)   ((ch)->add_abils.hit_add)
#define GET_MOVE_ADD(ch)  ((ch)->add_abils.move_add)
#define GET_SLOT(ch,i)    ((ch)->add_abils.slot_add[i])
#define GET_SAVE(ch,i)	  ((ch)->add_abils.apply_saving_throw[i])
#define GET_CASTER(ch)    ((ch)->CasterLevel)
#define GET_DAMAGE(ch)    ((ch)->DamageLevel)
#define GET_LIKES(ch)     ((ch)->mob_specials.LikeWork)

#define GET_POS(ch)	      ((ch)->char_specials.position)
#define GET_IDNUM(ch)	  ((ch)->char_specials.saved.idnum)
#define GET_ID(x)         ((x)->id)
#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)
#define FIGHTING(ch)	  ((ch)->char_specials.fighting)
#define HUNTING(ch)	      ((ch)->char_specials.hunting)
#define PROTECTING(ch)	  ((ch)->Protecting)
#define TOUCHING(ch)	  ((ch)->Touching)

#define ALIG_NEUT 0
#define ALIG_GOOD 1
#define ALIG_EVIL 2
#define ALIG_EVIL_LESS     -300
#define ALIG_GOOD_MORE     300

#define GET_ALIGNMENT(ch)     ((ch)->char_specials.saved.alignment)
#define CALC_ALIGNMENT(ch)    ((GET_ALIGNMENT(ch) <= ALIG_EVIL_LESS) ? ALIG_EVIL : \
                               (GET_ALIGNMENT(ch) >= ALIG_GOOD_MORE) ? ALIG_GOOD : ALIG_NEUT)

#define OK_GAIN_EXP(ch,victim)((IS_NPC(ch) ? \
                                (IS_NPC(victim) ? 1 : 0) : \
                                (IS_NPC(victim) && CALC_ALIGNMENT(victim) != ALIG_GOOD ? 1 : 0) \
                               ) \
                              )
#define MAX_EXP_PERCENT   80

#define GET_COND(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.conditions[(i)]))
#define GET_LOADROOM(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.load_room))
#define GET_INVIS_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.invis_level))
#define GET_WIMP_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.wimp_level))
#define GET_FREEZE_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.freeze_level))
#define GET_BAD_PWS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.bad_pws))
#define GET_TALK(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.talks[i]))
#define POOFIN(ch)              CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
#define POOFOUT(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
#define RENTABLE(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->may_rent))
#define GET_LAST_OLC_TARG(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))
#define GET_LAST_OLC_MODE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))
#define GET_ALIASES(ch)		    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define GET_LAST_TELL(ch)	    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))

#define GET_SKILL(ch, i)	((ch)->real_abils.Skills[i])
#define SET_SKILL(ch, i, pct)	((ch)->real_abils.Skills[i] = pct)
#define GET_SPELL_TYPE(ch, i)	((ch)->real_abils.SplKnw[i])
#define GET_SPELL_MEM(ch, i)	((ch)->real_abils.SplMem[i])
#define SET_SPELL(ch, i, pct)	((ch)->real_abils.SplMem[i] = pct)


#define GET_EQ(ch, i)		((ch)->equipment[i])

#define GET_MOB_SPEC(ch)	(IS_MOB(ch) ? mob_index[(ch)->nr].func : NULL)
#define GET_MOB_RNUM(mob)	((mob)->nr)
#define GET_MOB_VNUM(mob)	(IS_MOB(mob) ? \
				 mob_index[GET_MOB_RNUM(mob)].vnum : -1)

#define GET_DEFAULT_POS(ch)	((ch)->mob_specials.default_pos)
#define MEMORY(ch)		    ((ch)->mob_specials.memory)
#define GET_DEST(ch)        (((ch)->mob_specials.dest_count ? \
                              (ch)->mob_specials.dest[(ch)->mob_specials.dest_pos] : \
                              NOWHERE))
#define GET_ACTIVITY(ch)    ((ch)->mob_specials.activity)
#define GET_GOLD_NoDs(ch)   ((ch)->mob_specials.GoldNoDs)
#define GET_GOLD_SiDs(ch)   ((ch)->mob_specials.GoldSiDs)
#define GET_HORSESTATE(ch)  ((ch)->mob_specials.HorseState)
#define GET_LASTROOM(ch)    ((ch)->mob_specials.LastRoom)


#define STRENGTH_APPLY_INDEX(ch) \
        ( GET_REAL_STR(ch) )

#define CAN_CARRY_W(ch) (str_app[STRENGTH_APPLY_INDEX(ch)].carry_w)
#define CAN_CARRY_N(ch) (5 + (GET_REAL_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING && !AFF_FLAGGED(ch,AFF_SLEEP))
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))

#define IS_GOOD(ch)          (GET_ALIGNMENT(ch) >= ALIG_GOOD_MORE)
#define IS_EVIL(ch)          (GET_ALIGNMENT(ch) <= ALIG_EVIL_LESS)
#define IS_NEUTRAL(ch)       (!IS_GOOD(ch) && !IS_EVIL(ch))
#define SAME_ALIGN(ch,vict)  ((IS_GOOD(ch) && IS_GOOD(vict)) ||\
                              (IS_EVIL(ch) && IS_EVIL(vict)) ||\
			      (IS_NEUTRAL(ch) && IS_NEUTRAL(vict)))
#define GET_CH_SUF_1(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "�" :\
                          GET_SEX(ch) == SEX_MALE ? ""  :\
                          GET_SEX(ch) == SEX_FEMALE ? "�" : "�")
#define GET_CH_SUF_2(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "���" :\
                          GET_SEX(ch) == SEX_MALE ? "��"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "���" : "���")
#define GET_CH_SUF_3(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "��" :\
                          GET_SEX(ch) == SEX_MALE ? "��"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "��" : "��")
#define GET_CH_SUF_4(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "��" :\
                          GET_SEX(ch) == SEX_MALE ? ""  :\
                          GET_SEX(ch) == SEX_FEMALE ? "��" : "��")
#define GET_CH_SUF_5(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "��" :\
                          GET_SEX(ch) == SEX_MALE ? "��"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "��" : "��")
#define GET_CH_SUF_6(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "�" :\
                          GET_SEX(ch) == SEX_MALE ? ""  :\
                          GET_SEX(ch) == SEX_FEMALE ? "�" : "�")

#define GET_CH_VIS_SUF_1(ch,och) (!CAN_SEE(och,ch) ? "" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "�" :\
                          GET_SEX(ch) == SEX_MALE ? ""  :\
                          GET_SEX(ch) == SEX_FEMALE ? "�" : "�")
#define GET_CH_VIS_SUF_2(ch,och) (!CAN_SEE(och,ch) ? "��" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "���" :\
                          GET_SEX(ch) == SEX_MALE ? "��"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "���" : "���")
#define GET_CH_VIS_SUF_3(ch,och) (!CAN_SEE(och,ch) ? "��" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "��" :\
                          GET_SEX(ch) == SEX_MALE ? "��"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "��" : "��")
#define GET_CH_VIS_SUF_4(ch,och) (!CAN_SEE(och,ch) ? "" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "��" :\
                          GET_SEX(ch) == SEX_MALE ? ""  :\
                          GET_SEX(ch) == SEX_FEMALE ? "��" : "��")
#define GET_CH_VIS_SUF_5(ch,och) (!CAN_SEE(och,ch) ? "��" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "��" :\
                          GET_SEX(ch) == SEX_MALE ? "��"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "��" : "��")
#define GET_CH_VIS_SUF_6(ch,och) (!CAN_SEE(och,ch) ? "" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "�" :\
                          GET_SEX(ch) == SEX_MALE ? ""  :\
                          GET_SEX(ch) == SEX_FEMALE ? "�" : "�")


#define GET_OBJ_SEX(obj) ((obj)->obj_flags.Obj_sex)


#define GET_OBJ_SUF_1(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "�" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "�" : "�")
#define GET_OBJ_SUF_2(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "���" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? "��"  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "���" : "���")
#define GET_OBJ_SUF_3(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "��" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? "��"  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "��" : "��")
#define GET_OBJ_SUF_4(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "��" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "��" : "��")
#define GET_OBJ_SUF_5(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "��" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "��" : "��")
#define GET_OBJ_SUF_6(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "�" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "�" : "�")


#define GET_OBJ_VIS_SUF_1(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "�" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "�" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "�" : "�")
#define GET_OBJ_VIS_SUF_2(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "���" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "���" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? "��"  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "���" : "���")
#define GET_OBJ_VIS_SUF_3(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "��" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "��" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? "��"  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "��" : "��")
#define GET_OBJ_VIS_SUF_4(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "��" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "��" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "��" : "��")
#define GET_OBJ_VIS_SUF_5(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "��" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "��" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "��" : "��")
#define GET_OBJ_VIS_SUF_6(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "�" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "�" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "�" : "�")


/* These three deprecated. */
#define WAIT_STATE(ch, cycle) do { GET_WAIT_STATE(ch) = (cycle); } while(0)
#define CHECK_WAIT(ch)        ((ch)->wait > 0)
#define GET_WAIT(ch)          GET_WAIT_STATE(ch)
#define GET_MOB_HOLD(ch)      (AFF_FLAGGED((ch),AFF_HOLD) ? 1 : 0)
#define GET_MOB_SIELENCE(ch)  (AFF_FLAGGED((ch),AFF_SIELENCE) ? 1 : 0)
/* New, preferred macro. */
#define GET_WAIT_STATE(ch)    ((ch)->wait)


/* descriptor-based utils ************************************************/

/* Hrm, not many.  We should make more. -gg 3/4/99 */
#define STATE(d)	((d)->connected)


/* object utils **********************************************************/

#define GET_OBJ_ALIAS(obj)      ((obj)->short_description)
#define GET_OBJ_PNAME(obj,pad)  ((obj)->PNames[pad])
#define GET_OBJ_DESC(obj)       ((obj)->description)
#define GET_OBJ_SPELL(obj)      ((obj)->obj_flags.Obj_skill)
#define GET_OBJ_LEVEL(obj)      ((obj)->obj_flags.Obj_level)
#define GET_OBJ_AFFECTS(obj)    ((obj)->obj_flags.affects)
#define GET_OBJ_ANTI(obj)       ((obj)->obj_flags.anti_flag)
#define GET_OBJ_NO(obj)         ((obj)->obj_flags.no_flag)
#define GET_OBJ_ACT(obj)        ((obj)->action_description)
#define GET_OBJ_POS(obj)        ((obj)->obj_flags.worn_on)
#define GET_OBJ_TYPE(obj)	((obj)->obj_flags.type_flag)
#define GET_OBJ_COST(obj)	((obj)->obj_flags.cost)
#define GET_OBJ_RENT(obj)	((obj)->obj_flags.cost_per_day_off)
#define GET_OBJ_RENTEQ(obj)	((obj)->obj_flags.cost_per_day_on)
#define GET_OBJ_EXTRA(obj,flag)	(GET_FLAG((obj)->obj_flags.extra_flags,flag))
#define GET_OBJ_AFF(obj,flag)	(GET_FLAG((obj)->obj_flags.affects,flag))
#define GET_OBJ_WEAR(obj)	((obj)->obj_flags.wear_flags)
#define GET_OBJ_OWNER(obj)      ((obj)->obj_flags.Obj_owner)
#define GET_OBJ_VAL(obj, val)	((obj)->obj_flags.value[(val)])
#define GET_OBJ_WEIGHT(obj)	((obj)->obj_flags.weight)
#define GET_OBJ_TIMER(obj)	((obj)->obj_flags.Obj_timer)
#define GET_OBJ_DESTROY(obj) ((obj)->obj_flags.Obj_destroyer)
#define GET_OBJ_SKILL(obj)	((obj)->obj_flags.Obj_skill)
#define GET_OBJ_CUR(obj)    ((obj)->obj_flags.Obj_cur)
#define GET_OBJ_MAX(obj)    ((obj)->obj_flags.Obj_max)
#define GET_OBJ_MATER(obj)  ((obj)->obj_flags.Obj_mater)
#define GET_OBJ_ZONE(obj)   ((obj)->obj_flags.Obj_zone)
#define GET_OBJ_RNUM(obj) 	((obj)->item_number)
#define OBJ_GET_LASTROOM(obj) ((obj)->room_was_in)
#define GET_OBJ_VNUM(obj)	(GET_OBJ_RNUM(obj) >= 0 ? \
				 obj_index[GET_OBJ_RNUM(obj)].vnum : -1)
#define OBJ_WHERE(obj) ((obj)->worn_by    ? IN_ROOM(obj->worn_by) : \
                        (obj)->carried_by ? IN_ROOM(obj->carried_by) : (obj)->in_room)
#define IS_OBJ_STAT(obj,stat)	(IS_SET(GET_FLAG((obj)->obj_flags.extra_flags, \
                                                  stat), stat))
#define IS_OBJ_ANTI(obj,stat)	(IS_SET(GET_FLAG((obj)->obj_flags.anti_flag, \
                                                  stat), stat))
#define IS_OBJ_NO(obj,stat)	    (IS_SET(GET_FLAG((obj)->obj_flags.no_flag, \
                                                  stat), stat))
#define IS_OBJ_AFF(obj,stat)    (IS_SET(GET_FLAG((obj)->obj_flags.affects, \
                                                  stat), stat))

#define IS_CORPSE(obj)		(GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
					GET_OBJ_VAL((obj), 3) == 1)

#define GET_OBJ_SPEC(obj) ((obj)->item_number >= 0 ? \
	(obj_index[(obj)->item_number].func) : NULL)

#define CAN_WEAR(obj, part) (IS_SET((obj)->obj_flags.wear_flags, (part)))
#define GET_LOT(value) ((auction_lots+value))

/* compound utilities and other macros **********************************/

/*
 * Used to compute CircleMUD version. To see if the code running is newer
 * than 3.0pl13, you would use: #if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13)
 */
#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
	(((major) << 16) + ((minor) << 8) + (patchlevel))

#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "���": (GET_SEX(ch) == SEX_FEMALE ? "��" : "��")) :"���")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "��": (GET_SEX(ch) == SEX_FEMALE ? "���" : "���")) :"���")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "���": (GET_SEX(ch) == SEX_FEMALE ? "��" : "��")) :"���")

#define OSHR(ch) (GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch)==SEX_MALE ? "���": (GET_OBJ_SEX(ch) == SEX_FEMALE ? "��" : "��")) :"���")
#define OSSH(ch) (GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch)==SEX_MALE ? "��": (GET_OBJ_SEX(ch) == SEX_FEMALE ? "���" : "���")) :"���")
#define OMHR(ch) (GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch)==SEX_MALE ? "���": (GET_OBJ_SEX(ch) == SEX_FEMALE ? "��" : "��")) :"���")



/* Various macros building up to CAN_SEE */
#define MAY_SEE(sub,obj) (!AFF_FLAGGED((sub),AFF_BLIND) && \
                          (!IS_DARK(IN_ROOM(sub)) || AFF_FLAGGED((sub),AFF_INFRAVISION)) && \
			  (!AFF_FLAGGED((obj),AFF_INVISIBLE) || AFF_FLAGGED((sub),AFF_DETECT_INVIS)))
			  
#define MAY_ATTACK(sub)  (!AFF_FLAGGED((sub),AFF_CHARM)     && \
                          !IS_HORSE((sub))                  && \
			  !AFF_FLAGGED((sub),AFF_STOPFIGHT) && \
			  !AFF_FLAGGED((sub),AFF_HOLD)      && \
			  !MOB_FLAGGED((sub),MOB_NOFIGHT)   && \
			  GET_WAIT(sub) <= 0                && \
			  !FIGHTING(sub)                    && \
			  GET_POS(sub) >= POS_RESTING)

#define LIGHT_OK(sub)	(!AFF_FLAGGED(sub, AFF_BLIND) && \
   (IS_LIGHT((sub)->in_room) || AFF_FLAGGED((sub), AFF_INFRAVISION)))

#define INVIS_OK(sub, obj) \
 (!AFF_FLAGGED((sub), AFF_BLIND) && \
  ((!AFF_FLAGGED((obj),AFF_INVISIBLE) || \
    AFF_FLAGGED((sub),AFF_DETECT_INVIS) \
   ) && \
   ((!AFF_FLAGGED((obj), AFF_HIDE) && !AFF_FLAGGED((obj), AFF_CAMOUFLAGE)) || \
    AFF_FLAGGED((sub), AFF_SENSE_LIFE) \
   ) \
  ) \
 )

#define HERE(ch)  ((IS_NPC(ch) || (ch)->desc))

#define MORT_CAN_SEE(sub, obj) (HERE(obj) && \
                                INVIS_OK(sub,obj) && \
                                (IS_LIGHT((obj)->in_room) || \
                                 AFF_FLAGGED((sub), AFF_INFRAVISION) \
                                ) \
                               )

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && \
    IMM_CAN_SEE(sub, obj)))
    
/* Can subject see character "obj" without light */
#define MORT_CAN_SEE_CHAR(sub, obj) (HERE(obj) && \
                                     INVIS_OK(sub,obj) \
				    )

#define IMM_CAN_SEE_CHAR(sub, obj) \
        (MORT_CAN_SEE_CHAR(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define CAN_SEE_CHAR(sub, obj) (SELF(sub, obj) || \
        ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && \
         IMM_CAN_SEE_CHAR(sub, obj)))


/* End of CAN_SEE */


#define INVIS_OK_OBJ(sub, obj) \
  (!IS_OBJ_STAT((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/* Is anyone carrying this object and if so, are they visible? */

#define CAN_SEE_OBJ_CARRIER(sub, obj) \
  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) && \
   (!obj->worn_by    || CAN_SEE(sub, obj->worn_by)))
/*
#define MORT_CAN_SEE_OBJ(sub, obj) \
  (LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj))
 */

#define MORT_CAN_SEE_OBJ(sub, obj) \
  (INVIS_OK_OBJ(sub, obj) && !AFF_FLAGGED(sub, AFF_BLIND) && \
   (IS_LIGHT(IN_ROOM(sub)) || OBJ_FLAGGED(obj, ITEM_GLOW) || \
    (IS_CORPSE(obj) && AFF_FLAGGED(sub, AFF_INFRAVISION))))

#define CAN_SEE_OBJ(sub, obj) \
   (obj->worn_by    == sub || \
    obj->carried_by == sub || \
    (obj->in_obj && (obj->in_obj->worn_by == sub || obj->in_obj->carried_by == sub)) || \
    MORT_CAN_SEE_OBJ(sub, obj) || \
    (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))

#define OK_BOTH(ch,obj)  (GET_OBJ_WEIGHT(obj) <= \
                          str_app[STRENGTH_APPLY_INDEX(ch)].wield_w + str_app[STRENGTH_APPLY_INDEX(ch)].hold_w)

#define OK_WIELD(ch,obj) (GET_OBJ_WEIGHT(obj) <= \
                          str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)

#define OK_HELD(ch,obj)  (GET_OBJ_WEIGHT(obj) <= \
                          str_app[STRENGTH_APPLY_INDEX(ch)].hold_w)

#define GET_PAD_PERS(pad) ((pad) == 5 ? "���-��" :\
                           (pad) == 4 ? "���-��" :\
                           (pad) == 3 ? "����-��" :\
                           (pad) == 2 ? "����-��" :\
                           (pad) == 1 ? "����-��" : "���-��")

#define PERS(ch,vict,pad) (CAN_SEE(vict, ch) ? GET_PAD(ch,pad) : GET_PAD_PERS(pad))

#define OBJS(obj,vict) (CAN_SEE_OBJ((vict), (obj)) ? \
        	             (obj)->short_description  : "���-��")

#define GET_PAD_OBJ(pad)  ((pad) == 5 ? "���-��" :\
                           (pad) == 4 ? "���-��" :\
                           (pad) == 3 ? "���-��" :\
                           (pad) == 2 ? "����-��" :\
                           (pad) == 1 ? "����-��" : "���-��")


#define OBJN(obj,vict,pad) (CAN_SEE_OBJ((vict), (obj)) ? \
 	                        ((obj)->PNames[pad]) ? (obj)->PNames[pad] : (obj)->short_description \
 	                        : GET_PAD_OBJ(pad))

#define EXITDATA(room,door) ((room >= 0 && room < top_of_world) ? \
                             world[room].dir_option[door] : NULL)

#define EXIT(ch, door)  (world[(ch)->in_room].dir_option[door])

#define CAN_GO(ch, door) (EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))


#define CLASS_ABBR(ch) (IS_NPC(ch) ? "--" : class_abbrevs[(int)GET_CLASS(ch)])

#define IS_MAGIC_USER(ch)	(!IS_NPC(ch) && \
				(IS_BITS(MASK_MAGES, (int) GET_CLASS(ch))))
#define IS_CLERIC(ch)		(!IS_NPC(ch) && \
				((int) GET_CLASS(ch) == CLASS_CLERIC))
#define IS_THIEF(ch)		(!IS_NPC(ch) && \
				((int) GET_CLASS(ch) == CLASS_THIEF))
#define IS_ASSASINE(ch)		(!IS_NPC(ch) && \
				((int) GET_CLASS(ch) == CLASS_ASSASINE))
#define IS_WARRIOR(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_WARRIOR))
#define IS_PALADINE(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_PALADINE))
#define IS_RANGER(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_RANGER))
#define IS_GUARD(ch)		(!IS_NPC(ch) && \
				((int) GET_CLASS(ch) == CLASS_GUARD))
#define IS_SMITH(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_SMITH))
#define IS_MERCHANT(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MERCHANT))
#define IS_DRUID(ch)		(!IS_NPC(ch) && \
				((int) GET_CLASS(ch) == CLASS_DRUID))

#define LIKE_ROOM(ch) ((IS_CLERIC(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_CLERIC)) || \
                       (IS_MAGIC_USER(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_MAGE)) || \
                       (IS_WARRIOR(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_WARRIOR)) || \
                       (IS_THIEF(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_THIEF)) || \
                       (IS_ASSASINE(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_ASSASINE)) || \
                       (IS_GUARD(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_GUARD)) || \
                       (IS_PALADINE(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_PALADINE)) || \
                       (IS_RANGER(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_RANGER)) || \
                       (IS_SMITH(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_SMITH)) || \
                       (IS_MERCHANT(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_MERCHANT)) || \
                       (IS_DRUID(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_DRUID)))

#define OUTSIDE(ch) (!ROOM_FLAGGED((ch)->in_room, ROOM_INDOORS))
#define IS_HORSE(ch) (IS_NPC(ch) && (ch->master) && AFF_FLAGGED(ch, AFF_HORSE))
int    on_horse(struct char_data *ch);
int    has_horse(struct char_data *ch, int same_room);
struct char_data *get_horse(struct char_data *ch);
void   horse_drop(struct char_data *ch);
void   make_horse(struct char_data *horse, struct char_data *ch);
void   check_horse(struct char_data *ch);

int    same_group(struct char_data *ch, struct char_data *tch);
char  *only_title(struct char_data *ch);
char  *race_or_title(struct char_data *ch);
char  *race_or_title_enl(struct char_data *ch);
int    pc_duration(struct char_data *ch,int cnst, int level, int level_divisor, int min, int max);
void   paste_mobiles(int zone);
/* Auction functions  ****************************************************/
void message_auction(char *message, struct char_data *ch);
void clear_auction(int lot);
void sell_auction(int lot);
void check_auction(struct char_data *ch, struct obj_data *obj);
void tact_auction(void);
struct auction_lot_type *free_auction(int *lotnum);
int  obj_on_auction(struct obj_data *obj);

/* Modifier functions */
int god_spell_modifier(struct char_data *ch, int spellnum, int type, int value);
int day_spell_modifier(struct char_data *ch, int spellnum, int type, int value);
int weather_spell_modifier(struct char_data *ch, int spellnum, int type, int value);
int complex_spell_modifier(struct char_data *ch, int spellnum, int type, int value);

int god_skill_modifier(struct char_data *ch, int skillnum, int type, int value);
int day_skill_modifier(struct char_data *ch, int skillnum, int type, int value);
int weather_skill_modifier(struct char_data *ch, int skillnum, int type, int value);
int complex_skill_modifier(struct char_data *ch, int skillnum, int type, int value);

/* PADS for something ****************************************************/
char * desc_count (int how_many, int of_what);
#define WHAT_DAY      0
#define WHAT_HOUR     1
#define WHAT_YEAR     2
#define WHAT_POINT    3
#define WHAT_MINa     4
#define WHAT_MINu     5
#define WHAT_MONEYa   6
#define WHAT_MONEYu   7
#define WHAT_THINGa   8
#define WHAT_THINGu   9
#define WHAT_LEVEL    10
#define WHAT_MOVEa    11
#define WHAT_MOVEu    12
#define WHAT_ONEa     13
#define WHAT_ONEu     14
#define WHAT_SEC      15
#define WHAT_DEGREE   16



/* some awaking cases */
#define AW_HIDE       (1 << 0)
#define AW_INVIS      (1 << 1)
#define AW_CAMOUFLAGE (1 << 2)
#define AW_SNEAK      (1 << 3)

#define ACHECK_AFFECTS (1 << 0)
#define ACHECK_LIGHT   (1 << 1)
#define ACHECK_HUMMING (1 << 2)
#define ACHECK_GLOWING (1 << 3)
#define ACHECK_WEIGHT  (1 << 4)

int check_awake(struct char_data *ch, int what);
int awake_hide(struct char_data *ch);
int awake_invis(struct char_data *ch);
int awake_camouflage(struct char_data *ch);
int awake_sneak(struct char_data *ch);
int awaking(struct char_data *ch, int mode);

/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

/*
 * NOCRYPT can be defined by an implementor manually in sysdep.h.
 * CIRCLE_CRYPT is a variable that the 'configure' script
 * automatically sets when it determines whether or not the system is
 * capable of encrypting.
 */

#define PROOLCRYPT 

#if defined(NOCRYPT) || !defined(CIRCLE_CRYPT)
#define CRYPT(a,b) (a)
#else
#if defined(PROOLCRYPT)
#define CRYPT(a,b) ((char *) crypt_prool((a),(b)))
#else
#define CRYPT(a,b) ((char *) crypt((a),(b)))
#endif
#endif

#define SENDOK(ch)	(((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
		         (to_sleeping || AWAKE(ch)) && \
	                  !PLR_FLAGGED((ch), PLR_WRITING))



#define a_isspace(c) (strchr(" \f\n\r\t\v",(c)) != NULL)

extern inline char a_isascii(char c)
{
register char __res;
__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $32,%b0\n\t"
	"jae 1f\n\t"
	"xorb %%al,%%al\n\t"
	"1:"
	:"=a" (__res)
	:"0" (c));
return __res;
}

extern inline char a_isprint(char c)
{
register char __res;
__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $32,%b0\n\t"
	"jae 1f\n\t"
	"xorb %%al,%%al\n\t"
	"1:"
	:"=a" (__res)
	:"0" (c));
return __res;
}


extern inline char a_islower(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $97,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $122,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $192,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $223,%b0\n\t"
	"jbe 2f\n\t"
	"1:\txorb %%al,%%al\n\t"
	"2:"
	:"=a" (__res)
	:"0" (c));
return __res;
}

extern inline char a_isupper(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $65,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $90,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $224,%b0\n\t"
	"jb 1f\n\t"
	"jmp 2f\n\t"
	"1:\txorb %%al,%%al\n\t"
	"2:"
	:"=a" (__res)
	:"0" (c));
return __res;
}

extern inline char a_isdigit(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $48,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $57,%b0\n\t"
	"jbe 2f\n\t"
	"1:\txorb %%al,%%al\n\t"
	"2:"
	:"=a" (__res)
	:"0" (c));
return __res;
}

#if 1 // 1 = prool

inline char a_isalpha(char c)
{
if ((c>='a') && (c<='z')) return 1;
if ((c>='A') && (c<='Z')) return 1;
if ((unsigned char)c>=192) return 1; // cyrillic in koi8-r
return 0;
}
#else
extern inline char a_isalpha(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $65,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $90,%b0\n\t"
	"jbe 2f\n\t"	
	"cmpb $97,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $122,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $192,%b0\n\t"
	"jb 1f\n\t"
	"jmp 2f\n\t"
	"1:\txorb %%al,%%al\n\t"
	"2:"
	:"=a" (__res)
	:"0" (c));
return __res;
}
#endif

extern inline char a_isalnum(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $48,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $57,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $65,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $90,%b0\n\t"
	"jbe 2f\n\t"	
	"cmpb $97,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $122,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $192,%b0\n\t"
	"jb 1f\n\t"
	"jmp 2f\n\t"
	"1:\txorb %%al,%%al\n\t"
	"2:"
	:"=a" (__res)
	:"0" (c));
return __res;
}

extern inline char a_isxdigit(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $48,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $57,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $65,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $70,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $97,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $102,%b0\n\t"
	"jbe 2f\n\t"
	"1:\txorb %%al,%%al\n\t"
	"2:"
	:"=a" (__res)
	:"0" (c));
return __res;
}


extern inline char a_ucc(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $97,%b0\n\t"
	"jb 2f\n\t"
	"cmpb $122,%b0\n\t"
	"jbe 1f\n\t"
	"cmpb $192,%b0\n\t"
	"jb 2f\n\t"
	"cmpb $223,%b0\n\t"
	"ja 2f\n\t"	
	"addb $32,%%al\n\t"
	"jmp 2f\n\t"
	"1:\taddb $224,%%al\n\t"
	"2:"
	:"=a" (__res)
	:"0" (c));
return __res;
}

extern inline char a_lcc(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $65,%b0\n\t"
	"jb 2f\n\t"
	"cmpb $90,%b0\n\t"
	"jbe 1f\n\t"
	"cmpb $224,%b0\n\t"
	"jb 2f\n\t"
	"addb $224,%%al\n\t"
	"jmp 2f\n\t"
	"1:\taddb $32,%%al\n\t"
	"2:"
	:"=a" (__res)
	:"0" (c));
return __res;
}
