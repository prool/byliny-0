/* ************************************************************************
*   File: handler.h                                     Part of CircleMUD *
*  Usage: header file: prototypes of handling and utility functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* handling the affected-structures */
void	affect_total(struct char_data *ch);
void	affect_modify(struct char_data *ch, byte loc, sbyte mod, bitvector_t bitv, bool add);
void	affect_to_char(struct char_data *ch, struct affected_type *af);
void	affect_remove(struct char_data *ch, struct affected_type *af);
void	affect_from_char(struct char_data *ch, int type);
bool	affected_by_spell(struct char_data *ch, int type);
void	affect_join(struct char_data *ch, struct affected_type *af,
                    bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void	timed_to_char(struct char_data *ch, struct timed_type *timed);
void	timed_from_char(struct char_data *ch, struct timed_type *timed);
int	    timed_by_skill(struct char_data *ch, int skill);


/* utility */
char  *money_desc(int amount,int padis);
struct obj_data *create_money(int amount);
int	   isname(const char *str, const char *namelist);
char  *fname(const char *namelist);
int	   get_number(char **name);

/* ******** objects *********** */

void	obj_to_char(struct obj_data *object, struct char_data *ch);
void	obj_from_char(struct obj_data *object);

void	equip_char(struct char_data *ch, struct obj_data *obj, int pos);
struct obj_data *unequip_char(struct char_data *ch, int pos);
int	invalid_align(struct char_data *ch, struct obj_data *obj);

struct obj_data *get_obj_in_list(char *name, struct obj_data *list);
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list);
struct obj_data *get_obj(char *name);
struct obj_data *get_obj_num(obj_rnum nr);

void	obj_to_room(struct obj_data *object, room_rnum room);
void	obj_from_room(struct obj_data *object);
void	obj_to_obj(struct obj_data *obj, struct obj_data *obj_to);
void	obj_from_obj(struct obj_data *obj);
void	object_list_new_owner(struct obj_data *list, struct char_data *ch);

void	extract_obj(struct obj_data *obj);

/* ******* characters ********* */

struct  char_data *get_char_room(char *name, room_rnum room);
struct  char_data *get_char_num(mob_rnum nr);
struct  char_data *get_char(char *name);

void	char_from_room(struct char_data *ch);
void	char_to_room(struct char_data *ch, room_rnum room);
void	extract_char(struct char_data *ch, int clear_objs);

void    set_quested(struct char_data *ch, int quest);
int     get_quested(struct char_data *ch, int quest);
void    inc_kill_vnum(struct char_data *ch, int vnum, int incvalue);
int     get_kill_vnum(struct char_data *ch, int vnum);

/* find if character can see */
struct char_data *get_char_room_vis(struct char_data *ch, char *name);
struct char_data *get_player_vis(struct char_data *ch, char *name, int inroom);

struct char_data *get_char_vis(struct char_data *ch, char *name, int where);
struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, 
struct obj_data *list);
struct obj_data *get_obj_vis(struct char_data *ch, char *name);
struct obj_data *get_object_in_equip_vis(struct char_data *ch,
char  *arg, struct obj_data *equipment[], int *j);


/* find all dots */

int	find_all_dots(char *arg);

#define FIND_INDIV	0
#define FIND_ALL	1
#define FIND_ALLDOT	2


/* Generic Find */

int	generic_find(char *arg, bitvector_t bitvector, struct char_data *ch,
		struct char_data **tar_ch, struct obj_data **tar_obj);

#define FIND_CHAR_ROOM     (1 << 0)
#define FIND_CHAR_WORLD    (1 << 1)
#define FIND_OBJ_INV       (1 << 2)
#define FIND_OBJ_ROOM      (1 << 3)
#define FIND_OBJ_WORLD     (1 << 4)
#define FIND_OBJ_EQUIP     (1 << 5)

#define CRASH_DELETE_OLD   (1 << 0)
#define CRASH_DELETE_NEW   (1 << 1)

/* prototypes from crash save system */

int	    Crash_get_filename(char *orig_name, char *filename);
int	    Crash_delete_file(char *name, int mask);
int	    Crash_delete_crashfile(struct char_data *ch);
int	    Crash_clean_file(char *name);
void	    Crash_listrent(struct char_data *ch, char *name);
int	    Crash_load(struct char_data *ch);
void	    Crash_crashsave(struct char_data *ch);
void	    Crash_idlesave(struct char_data *ch);
void	    Crash_save_all(void);

/* prototypes from fight.c */
void	set_fighting(struct char_data *ch, struct char_data *victim);
void	stop_fighting(struct char_data *ch, int switch_others);
int 	stop_follower(struct char_data *ch, int mode);
void	hit(struct char_data *ch, struct char_data *victim, int type, int weapon);
void	forget(struct char_data *ch, struct char_data *victim);
void	remember(struct char_data *ch, struct char_data *victim);
int	damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype, int mayflee);
int	skill_message(int dam, struct char_data *ch, struct char_data *vict,
	              int attacktype);
int     calculate_skill (struct char_data * ch,int skill_no,int max_value,struct char_data * vict);
int     train_skill     (struct char_data * ch,int skill_no,int max_value,struct char_data * vict);
void    improove_skill  (struct char_data * ch,int skill_no,int success,struct char_data * vict);
