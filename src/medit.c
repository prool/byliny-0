/************************************************************************
 * OasisOLC - medit.c						v1.5	*
 * Copyright 1996 Harvey Gilpin.					*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "spells.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "olc.h"
#include "handler.h"
#include "dg_olc.h"

/*
 * Set this to 1 for debugging logs in medit_save_internally.
 */
#if 0
#define DEBUG
#endif

/*
 * Set this to 1 as a last resort to save mobiles.
 */
#if 0
#define I_CRASH
#endif

/*-------------------------------------------------------------------*/

/*
 * External variable declarations.
 */
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct char_data *character_list;
extern mob_rnum top_of_mobt;
extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;
extern struct player_special_data dummy_mob;
extern struct attack_hit_type attack_hit_text[];
extern const char *function_bits[];
extern const char *action_bits[];
extern const char *affected_bits[];
extern const char *position_types[];
extern const char *genders[];
extern int top_shop;
extern struct shop_data *shop_index;
extern struct descriptor_data *descriptor_list;
#if defined(OASIS_MPROG)
extern const char *mobprog_types[];
#endif

void tascii(int *pointer, int num_planes, char *ascii);
int  planebit(char *str, int *plane, int *bit);

/*-------------------------------------------------------------------*/

/*
 * Handy internal macros.
 */

#define GET_NDD(mob) ((mob)->mob_specials.damnodice)
#define GET_SDD(mob) ((mob)->mob_specials.damsizedice)
#define GET_ALIAS(mob) ((mob)->player.name)
#define GET_SDESC(mob) ((mob)->player.short_descr)
#define GET_LDESC(mob) ((mob)->player.long_descr)
#define GET_DDESC(mob) ((mob)->player.description)
#define GET_ATTACK(mob) ((mob)->mob_specials.attack_type)
#define S_KEEPER(shop) ((shop)->keeper)
#if defined(OASIS_MPROG)
#define GET_MPROG(mob)		(mob_index[(mob)->nr].mobprogs)
#define GET_MPROG_TYPE(mob)	(mob_index[(mob)->nr].progtypes)
#endif

/*-------------------------------------------------------------------*/

/*
 * Function prototypes.
 */
void medit_parse(struct descriptor_data *d, char *arg);
void medit_disp_menu(struct descriptor_data *d);
void medit_setup_new(struct descriptor_data *d);
void medit_setup_existing(struct descriptor_data *d, int rmob_num);
void medit_save_internally(struct descriptor_data *d);
void medit_save_to_disk(int zone_num);
void init_mobile(struct char_data *mob);
void copy_mobile(struct char_data *tmob, struct char_data *fmob);
void medit_disp_positions(struct descriptor_data *d);
void medit_disp_mob_flags(struct descriptor_data *d);
void medit_disp_aff_flags(struct descriptor_data *d);
void medit_disp_attack_types(struct descriptor_data *d);
#if defined(OASIS_MPROG)
void medit_disp_mprog(struct descriptor_data *d);
void medit_change_mprog(struct descriptor_data *d);
const char *medit_get_mprog_type(struct mob_prog_data *mprog);
#endif

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

/*
 * Free a mobile structure that has been edited.
 * Take care of existing mobiles and their mob_proto!
 */

void medit_free_mobile(struct char_data *mob)
{ struct helper_data_type *temp;
  int i,j;
  /*
   * Non-prototyped mobile.  Also known as new mobiles.
   */
  if (!mob)
     return;
  else 
  if (GET_MOB_RNUM(mob) == -1) 
     {if (mob->player.name)
         free(mob->player.name);
      if (mob->player.title)
         free(mob->player.title);
      for (j = 0; j < NUM_PADS; j++)
          if (GET_PAD(mob, j))
             free(GET_PAD(mob,j));
      if (mob->player.short_descr)
         free(mob->player.short_descr);
      if (mob->player.long_descr)
         free(mob->player.long_descr);
      if (mob->player.description)
         free(mob->player.description);
      if (mob->mob_specials.Questor)
         free(mob->mob_specials.Questor);
     } 
  else 
  if ((i = GET_MOB_RNUM(mob)) > -1) 
     {/* Prototyped mobile. */
      if (mob->player.name && mob->player.name != mob_proto[i].player.name)
         free(mob->player.name);
      if (mob->player.title && mob->player.title != mob_proto[i].player.title)
         free(mob->player.title);
      if (mob->player.short_descr && mob->player.short_descr != mob_proto[i].player.short_descr)
         free(mob->player.short_descr);
      for (j = 0; j < NUM_PADS; j++)
         if (GET_PAD(mob,j) && (mob->player.PNames[j] != mob_proto[i].player.PNames[j]))
            free(mob->player.PNames[j]);
      if (mob->player.long_descr && mob->player.long_descr != mob_proto[i].player.long_descr)
         free(mob->player.long_descr);
      if (mob->player.description && mob->player.description != mob_proto[i].player.description)
         free(mob->player.description);
      if (mob->mob_specials.Questor && mob->mob_specials.Questor != mob_proto[i].mob_specials.Questor)
         free(mob->mob_specials.Questor);

  }
  while (mob->helpers)
        REMOVE_FROM_LIST(mob->helpers,mob->helpers,next_helper);

  while (mob->affected)
        affect_remove(mob, mob->affected);

  free(mob);
}

void medit_setup_new(struct descriptor_data *d)
{
  struct char_data *mob;

  /*
   * Allocate a scratch mobile structure.  
   */
  CREATE(mob, struct char_data, 1);

  init_mobile(mob);

  GET_MOB_RNUM(mob) = -1;
  /*
   * Set up some default strings.
   */
  GET_ALIAS(mob) = str_dup("неоконченный моб");
  GET_SDESC(mob) = str_dup("неоконченный моб");
  GET_PAD(mob,0) = str_dup("неоконченный моб");
  GET_PAD(mob,1) = str_dup("неоконченного моба");
  GET_PAD(mob,2) = str_dup("неоконченному мобу");
  GET_PAD(mob,3) = str_dup("неоконченного моба");
  GET_PAD(mob,4) = str_dup("неоконченным мобом");
  GET_PAD(mob,5) = str_dup("неоконченном мобе");
  GET_LDESC(mob) = str_dup("Неоконченный моб стоит тут.\r\n");
  GET_DDESC(mob) = str_dup("Выглядит достаточно незавершенно.\r\n");
  mob->mob_specials.Questor   = NULL;
  mob->helpers   = NULL;
  mob->pk_list   = NULL;
#if defined(OASIS_MPROG)
  OLC_MPROGL(d) = NULL;
  OLC_MPROG(d) = NULL;
#endif

  OLC_MOB(d)       = mob;
  OLC_VAL(d)       = 0;  /* Has changed flag. (It hasn't so far, we just made it.) */
  OLC_ITEM_TYPE(d) = MOB_TRIGGER;

  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void medit_setup_existing(struct descriptor_data *d, int rmob_num)
{
  struct char_data *mob;
#if defined(OASIS_MPROG)
  MPROG_DATA *temp;
  MPROG_DATA *head;
#endif

  /*
   * Allocate a scratch mobile structure. 
   */
  CREATE(mob, struct char_data, 1);

  copy_mobile(mob, mob_proto + rmob_num);

#if defined(OASIS_MPROG)
  /*
   * I think there needs to be a brace from the if statement to the #endif
   * according to the way the original patch was indented.  If this crashes,
   * try it with the braces and report to greerga@van.ml.org on if that works.
   */
  if (GET_MPROG(mob))
     CREATE(OLC_MPROGL(d), MPROG_DATA, 1);
  head = OLC_MPROGL(d);
  for (temp = GET_MPROG(mob); temp;temp = temp->next) 
      {OLC_MPROGL(d)->type = temp->type;
       OLC_MPROGL(d)->arglist = str_dup(temp->arglist);
       OLC_MPROGL(d)->comlist = str_dup(temp->comlist);
       if (temp->next) 
          {CREATE(OLC_MPROGL(d)->next, MPROG_DATA, 1);
           OLC_MPROGL(d) = OLC_MPROGL(d)->next;
          }
      }
  OLC_MPROGL(d) = head;
  OLC_MPROG(d) = OLC_MPROGL(d);
#endif

  OLC_MOB(d)       = mob;
  OLC_ITEM_TYPE(d) = MOB_TRIGGER;
  dg_olc_script_copy(d);
  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/*
 * Copy one mob struct to another.
 */
void copy_mobile(struct char_data *tmob, struct char_data *fmob)
{ int    j;
  struct trig_proto_list *proto, *fproto;
  struct helper_data_type *helper, *fhelper, *temp;

  /*
   * Free up any used strings.
   */
  if (GET_ALIAS(tmob))
     free(GET_ALIAS(tmob));
  if (GET_SDESC(tmob))
     free(GET_SDESC(tmob));
  if (GET_LDESC(tmob))
     free(GET_LDESC(tmob));
  if (GET_DDESC(tmob))
     free(GET_DDESC(tmob));
  for (j = 0; j < NUM_PADS; j++)
      if (GET_PAD(tmob, j))
         free(GET_PAD(tmob,j));
  if (tmob->mob_specials.Questor)
     free(tmob->mob_specials.Questor);
  while (tmob->helpers)
        REMOVE_FROM_LIST(tmob->helpers,tmob->helpers,next_helper);


  /* delete the old script list */
  proto = tmob->proto_script;
  while (proto) 
        {fproto = proto;
         proto  = proto->next;
         free(fproto);
        }

  /*
   * Copy mob over.
   */
  *tmob = *fmob;

  /*
   * Reallocate strings.
   */
  GET_ALIAS(tmob) = str_dup((GET_ALIAS(fmob) && *GET_ALIAS(fmob)) ? GET_ALIAS(fmob) : "неопределен");
  GET_SDESC(tmob) = str_dup((GET_SDESC(fmob) && *GET_SDESC(fmob)) ? GET_SDESC(fmob) : "неопределен");
  GET_LDESC(tmob) = str_dup((GET_LDESC(fmob) && *GET_LDESC(fmob)) ? GET_LDESC(fmob) : "неопределен");
  GET_DDESC(tmob) = str_dup((GET_DDESC(fmob) && *GET_DDESC(fmob)) ? GET_DDESC(fmob) : "неопределен");
  for (j = 0; j < NUM_PADS; j++)
      GET_PAD(tmob, j) = str_dup(GET_PAD(fmob,j) && *GET_PAD(fmob,j) ? GET_PAD(fmob,j) : "неопределен");
  tmob->mob_specials.Questor = (fmob->mob_specials.Questor && *fmob->mob_specials.Questor ? str_dup(fmob->mob_specials.Questor) : NULL);

  if (fmob->helpers)
     {fhelper = fmob->helpers;
      CREATE(helper, struct helper_data_type, 1);
      tmob->helpers = helper;
      do {helper->mob_vnum = fhelper->mob_vnum;
          fhelper          = fhelper->next_helper;
          if (fhelper)
             {CREATE(helper->next_helper, struct helper_data_type, 1);
              helper = helper->next_helper;
             }
         } while (fhelper);
     }

  /* copy the new script list */
  if (fmob->proto_script) 
     {fproto = fmob->proto_script;
      CREATE(proto, struct trig_proto_list, 1);
      tmob->proto_script = proto;
      do {proto->vnum = fproto->vnum;
          fproto = fproto->next;
          if (fproto) 
             {CREATE(proto->next, struct trig_proto_list, 1);
              proto = proto->next;
             }
         } while (fproto);
     }
}

/*-------------------------------------------------------------------*/

/*
 * Ideally, this function should be in db.c, but I'll put it here for
 * portability.
 */
void init_mobile(struct char_data *mob)
{
  clear_char(mob);

  GET_HIT(mob)         = GET_MANA_NEED(mob) = 1;
  GET_MANA_STORED(mob) = GET_MAX_MOVE(mob)  = 100;
  GET_NDD(mob)         = GET_SDD(mob)       = 1;
  GET_GOLD_NoDs(mob)   = GET_GOLD_SiDs(mob) = 0;
  GET_WEIGHT(mob)             = 200;
  GET_HEIGHT(mob)             = 198;
  GET_SIZE(mob)               = 30;

  mob->real_abils.str = mob->real_abils.intel = mob->real_abils.wis = 11;
  mob->real_abils.dex = mob->real_abils.con = mob->real_abils.cha = 11;

  SET_BIT(MOB_FLAGS(mob, MOB_ISNPC), MOB_ISNPC);
  mob->player_specials = &dummy_mob;
}

/*-------------------------------------------------------------------*/

#define ZCMD zone_table[zone].cmd[cmd_no]

/*
 * Save new/edited mob to memory.
 */
void medit_save_internally(struct descriptor_data *d)
{
  int rmob_num, found = 0, new_mob_num = 0, zone, cmd_no, shop, j;
  struct char_data *new_proto;
  struct index_data *new_index;
  struct char_data *live_mob;
  struct descriptor_data *dsc;

  /* put the script into proper position */
  OLC_MOB(d)->proto_script = OLC_SCRIPT(d);

  /*
   * Mob exists? Just update it.
   */
  if ((rmob_num = real_mobile(OLC_NUM(d))) > -1) 
     {OLC_MOB(d)->proto_script = OLC_SCRIPT(d);
      copy_mobile((mob_proto + rmob_num), OLC_MOB(d));
      /*
       * Update live mobiles.
       */
      for (live_mob = character_list; live_mob; live_mob = live_mob->next)
          if (IS_MOB(live_mob) && GET_MOB_RNUM(live_mob) == rmob_num) 
             {/*
	           * Only really need to update the strings, since these can
	           * cause protection faults.  The rest can wait till a reset/reboot.
	           */
	          GET_ALIAS(live_mob) = GET_ALIAS(mob_proto + rmob_num);
	          GET_SDESC(live_mob) = GET_SDESC(mob_proto + rmob_num);
	          GET_LDESC(live_mob) = GET_LDESC(mob_proto + rmob_num);
	          GET_DDESC(live_mob) = GET_DDESC(mob_proto + rmob_num);
	          for (j = 0; j < NUM_PADS; j++)
	              GET_PAD(live_mob,j) = GET_PAD(mob_proto + rmob_num, j);
	          live_mob->helpers                = (mob_proto + rmob_num)->helpers;
	          live_mob->mob_specials.Questor   = (mob_proto + rmob_num)->mob_specials.Questor;
             }
     }
  /*
   * Mob does not exist, we have to add it.
   */
  else 
     {
#if defined(DEBUG)
      fprintf(stderr, "top_of_mobt: %d, new top_of_mobt: %d\n", top_of_mobt, top_of_mobt + 1);
#endif

      CREATE(new_proto, struct char_data, top_of_mobt + 2);
      CREATE(new_index, struct index_data, top_of_mobt + 2);

      for (rmob_num = 0; rmob_num <= top_of_mobt; rmob_num++) 
          {if (!found) 
              {/* Is this the place? */
       	       if (mob_index[rmob_num].vnum > OLC_NUM(d)) 
       	          {/*
	                * Yep, stick it here.
	                */
	               found = TRUE;
#if defined(DEBUG)
	  fprintf(stderr, "Inserted: rmob_num: %d\n", rmob_num);
#endif
	               new_index[rmob_num].vnum = OLC_NUM(d);
	               new_index[rmob_num].number = 0;
	               new_index[rmob_num].func = NULL;
	               new_mob_num = rmob_num;
	               GET_MOB_RNUM(OLC_MOB(d)) = rmob_num;
	               copy_mobile((new_proto + rmob_num), OLC_MOB(d));
	               /*
	                * Copy the mob that should be here on top.
	                */
	               new_index[rmob_num + 1] = mob_index[rmob_num];
	               new_proto[rmob_num + 1] = mob_proto[rmob_num];
	               GET_MOB_RNUM(new_proto + rmob_num + 1) = rmob_num + 1;
	              } 
	           else 
	              {/* Nope, copy over as normal. */
	               new_index[rmob_num] = mob_index[rmob_num];
	               new_proto[rmob_num] = mob_proto[rmob_num];
	              }
              } 
           else 
              {/* We've already found it, copy the rest over. */
	           new_index[rmob_num + 1] = mob_index[rmob_num];
	           new_proto[rmob_num + 1] = mob_proto[rmob_num];
	           GET_MOB_RNUM(new_proto + rmob_num + 1) = rmob_num + 1;
              }
          }
#if defined(DEBUG)
      fprintf(stderr, "rmob_num: %d, top_of_mobt: %d, array size: 0-%d (%d)\n",
		            rmob_num, top_of_mobt, top_of_mobt + 1, top_of_mobt + 2);
#endif
      if (!found) 
         { /* Still not found, must add it to the top of the table. */
#if defined(DEBUG)
          fprintf(stderr, "Append.\n");
#endif
          new_index[rmob_num].vnum = OLC_NUM(d);
          new_index[rmob_num].number = 0;
          new_index[rmob_num].func = NULL;
          new_mob_num = rmob_num;
          GET_MOB_RNUM(OLC_MOB(d)) = rmob_num;
          copy_mobile((new_proto + rmob_num), OLC_MOB(d));
         }
    /*
     * Replace tables.
     */
#if defined(DEBUG)
      fprintf(stderr, "Attempted free.\n");
#endif
#if !defined(I_CRASH)
      free(mob_index);
      free(mob_proto);
#endif
      mob_index = new_index;
      mob_proto = new_proto;
      top_of_mobt++;
#if defined(DEBUG)
      fprintf(stderr, "Free ok.\n");
#endif

      /*
       * Update live mobile rnums.
       */
      for (live_mob = character_list; live_mob; live_mob = live_mob->next)
          if (GET_MOB_RNUM(live_mob) > new_mob_num)
	         GET_MOB_RNUM(live_mob)++;

      /*
       * Update zone table.
       */
      for (zone = 0; zone <= top_of_zone_table; zone++)
          for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
	          if (ZCMD.command == 'M')
	             {if (ZCMD.arg1 >= new_mob_num)
	                 ZCMD.arg1++;
	             }
  	          else
  	          if (ZCMD.command == 'F')
	             {if (ZCMD.arg2 >= new_mob_num)
	                 ZCMD.arg2++;
	              if (ZCMD.arg3 >= new_mob_num)
	                 ZCMD.arg3++;
	             }

      /*
       * Update shop keepers.
       */
      if (shop_index)
         for (shop = 0; shop <= top_shop; shop++)
 	         if (SHOP_KEEPER(shop) >= new_mob_num)
	            SHOP_KEEPER(shop)++;

      /*
       * Update keepers in shops being edited and other mobs being edited.
       */
      for (dsc = descriptor_list; dsc; dsc = dsc->next)
          if (dsc->connected == CON_SEDIT) 
             {if (S_KEEPER(OLC_SHOP(dsc)) >= new_mob_num)
	             S_KEEPER(OLC_SHOP(dsc))++;
             }  
          else 
          if (dsc->connected == CON_MEDIT) 
             {if (GET_MOB_RNUM(OLC_MOB(dsc)) >= new_mob_num)
	             GET_MOB_RNUM(OLC_MOB(dsc))++;
             }
     }

#if defined(OASIS_MPROG)
  GET_MPROG(OLC_MOB(d)) = OLC_MPROGL(d);
  GET_MPROG_TYPE(OLC_MOB(d)) = (OLC_MPROGL(d) ? OLC_MPROGL(d)->type : 0);
  while (OLC_MPROGL(d)) 
        {GET_MPROG_TYPE(OLC_MOB(d)) |= OLC_MPROGL(d)->type;
         OLC_MPROGL(d) = OLC_MPROGL(d)->next;
        }
#endif

 olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_MOB);
}

/*-------------------------------------------------------------------*/

/*
 * Save ALL mobiles for a zone to their .mob file, mobs are all 
 * saved in Extended format, regardless of whether they have any
 * extended fields.  Thanks to Sammy for ideas on this bit of code.
 */
void medit_save_to_disk(int zone_num)
{ struct helper_data_type *helper;
  int i, j, c, rmob_num, zone, top;
  FILE *mob_file;
  char fname[64];
  struct char_data *mob;
#if defined(OASIS_MPROG)
  MPROG_DATA *mob_prog = NULL;
#endif

  zone = zone_table[zone_num].number;
  top = zone_table[zone_num].top;

  sprintf(fname, "%s/%d.new", MOB_PREFIX, zone);
  if (!(mob_file = fopen(fname, "w"))) 
     {mudlog("SYSERR: OLC: Cannot open mob file!", BRF, LVL_BUILDER, TRUE);
      return;
     }

  /*
   * Seach the database for mobs in this zone and save them.
   */
  for (i = zone * 100; i <= top; i++) 
      {if ((rmob_num = real_mobile(i)) != -1) 
          {if (fprintf(mob_file, "#%d\n", i) < 0) 
              {mudlog("SYSERR: OLC: Cannot write mob file!\r\n", BRF, LVL_BUILDER, TRUE);
	           fclose(mob_file);
	           return;
              }
           mob = (mob_proto + rmob_num);

           /*
            * Clean up strings.
            */
           strcpy(buf1, (GET_LDESC(mob) && *GET_LDESC(mob)) ? GET_LDESC(mob) : "неопределен");
           strip_string(buf1);
           strcpy(buf2, (GET_DDESC(mob) && *GET_DDESC(mob)) ? GET_DDESC(mob) : "undefined");
           strip_string(buf2);
           

           fprintf(mob_file, 
                   "%s~\n"
			       "%s~\n"
			       "%s~\n"
			       "%s~\n"
			       "%s~\n"
			       "%s~\n"
			       "%s~\n"
			       "%s~\n"
			       "%s~\n",
	               (GET_ALIAS(mob) && *GET_ALIAS(mob)) ? GET_ALIAS(mob) : "неопределен",
	               (GET_PAD(mob,0) && *GET_PAD(mob,0)) ? GET_PAD(mob,0) : "кто",
	               (GET_PAD(mob,1) && *GET_PAD(mob,1)) ? GET_PAD(mob,1) : "кого",
	               (GET_PAD(mob,2) && *GET_PAD(mob,2)) ? GET_PAD(mob,2) : "кому",
	               (GET_PAD(mob,3) && *GET_PAD(mob,3)) ? GET_PAD(mob,3) : "кого",
	               (GET_PAD(mob,4) && *GET_PAD(mob,4)) ? GET_PAD(mob,4) : "кем",
	               (GET_PAD(mob,5) && *GET_PAD(mob,5)) ? GET_PAD(mob,5) : "о ком",
	                buf1, buf2); 
	       if (mob->mob_specials.Questor)
	          strcpy(buf1,mob->mob_specials.Questor);
	       else
	          strcpy(buf1,"");
	       strip_string(buf1); 
	       *buf2 = 0;
	       tascii(&MOB_FLAGS(mob,0),4,buf2);
	       tascii(&AFF_FLAGS(mob,0),4,buf2);
	        
	       fprintf(mob_file,
	               // "%s~\n" 
		           "%s%d E\n"
		           "%d %d %d %dd%d+%d %dd%d+%d\n"
		           "%dd%d+%d %ld\n"
		           "%d %d %d\n",
		           // buf1,
	               buf2, GET_ALIGNMENT(mob),
	               GET_LEVEL(mob),20 - GET_HR(mob),GET_AC(mob)/10,
	               GET_MANA_NEED(mob),GET_MANA_STORED(mob),GET_HIT(mob),
	               GET_NDD(mob),GET_SDD(mob),GET_DR(mob),
	               GET_GOLD_NoDs(mob),GET_GOLD_SiDs(mob),GET_GOLD(mob),GET_EXP(mob), 
	               GET_POS(mob), GET_DEFAULT_POS(mob), GET_SEX(mob)
	              );

           /*
            * Deal with Extra stats in case they are there.
            */
           if (GET_ATTACK(mob) != 0)
	          fprintf(mob_file, "BareHandAttack: %d\n", GET_ATTACK(mob));
	       for (c = 0; c < mob->mob_specials.dest_count; c++)
               fprintf(mob_file, "Destination: %d\n", mob->mob_specials.dest[c]);
           if (GET_STR(mob) != 11)
	          fprintf(mob_file, "Str: %d\n", GET_STR(mob));
           if (GET_DEX(mob) != 11)
	          fprintf(mob_file, "Dex: %d\n", GET_DEX(mob));
           if (GET_INT(mob) != 11)
	          fprintf(mob_file, "Int: %d\n", GET_INT(mob));
           if (GET_WIS(mob) != 11)
	          fprintf(mob_file, "Wis: %d\n", GET_WIS(mob));
           if (GET_CON(mob) != 11)
	          fprintf(mob_file, "Con: %d\n", GET_CON(mob));
           if (GET_CHA(mob) != 11)
	          fprintf(mob_file, "Cha: %d\n", GET_CHA(mob));
           if (GET_SIZE(mob))
              fprintf(mob_file, "Size: %d\n", GET_SIZE(mob));   
           
           if (mob->mob_specials.LikeWork)
              fprintf(mob_file, "LikeWork: %d\n", mob->mob_specials.LikeWork);
           if (mob->mob_specials.MaxFactor)
              fprintf(mob_file, "MaxFactor: %d\n", mob->mob_specials.MaxFactor);
           if (mob->mob_specials.ExtraAttack)
              fprintf(mob_file, "ExtraAttack: %d\n", mob->mob_specials.ExtraAttack);
           if (GET_CLASS(mob))
              fprintf(mob_file, "Class: %d\n", GET_CLASS(mob));
           if (GET_HEIGHT(mob))
              fprintf(mob_file, "Height: %d\n", GET_HEIGHT(mob));
           if (GET_WEIGHT(mob))
              fprintf(mob_file, "Weight: %d\n", GET_WEIGHT(mob));
           strcpy(buf1,"Special_Bitvector: ");
           tascii(&mob->mob_specials.npc_flags.flags[0],4,buf1);
           fprintf(mob_file, "%s\n", buf1);
           for (c = 1; c <= MAX_SKILLS; c++)
               {if (GET_SKILL(mob,c))
                   fprintf(mob_file, "Skill: %d %d\n", c, GET_SKILL(mob,c));
               }
           for (c = 1; c <= MAX_SPELLS; c++)
               {for (j = 1; j <= GET_SPELL_MEM(mob,c); j++)
                    fprintf(mob_file, "Spell: %d\n", c);
               }
           for (helper = GET_HELPER(mob); helper; helper = helper->next_helper)
               fprintf(mob_file, "Helper: %d\n", helper->mob_vnum);

           /*
            * XXX: Add E-mob handlers here.
            */

           fprintf(mob_file, "E\n");

           script_save_to_disk(mob_file, mob, MOB_TRIGGER);

#if defined(OASIS_MPROG)
           /*
            * Write out the MobProgs.
            */
           mob_prog = GET_MPROG(mob);
           while(mob_prog) 
                {strcpy(buf1, mob_prog->arglist);
                 strip_string(buf1);
                 strcpy(buf2, mob_prog->comlist);
                 strip_string(buf2);
                 fprintf(mob_file, "%s %s~\n%s", medit_get_mprog_type(mob_prog),
                 buf1, buf2);
	             mob_prog = mob_prog->next;
	             fprintf(mob_file, "~\n%s", (!mob_prog ? "|\n" : ""));
                }
#endif
          }
      }
  fprintf(mob_file, "$\n");
  fclose(mob_file);
  sprintf(buf2, "%s/%d.mob", MOB_PREFIX, zone);
  /*
   * We're fubar'd if we crash between the two lines below.
   */
  remove(buf2);
  rename(fname, buf2);

  olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_MOB);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * Display positions. (sitting, standing, etc)
 */
void medit_disp_positions(struct descriptor_data *d)
{
  int i;

  get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (i = 0; *position_types[i] != '\n'; i++) 
      {sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, position_types[i]);
       send_to_char(buf, d->character);
      }
  send_to_char("Выберите положение : ", d->character);
}

/*-------------------------------------------------------------------*/

#if defined(OASIS_MPROG)
/*
 * Get the type of MobProg.
 */
const char *medit_get_mprog_type(struct mob_prog_data *mprog)
{
  switch (mprog->type) 
  {
  case IN_FILE_PROG:	return ">in_file_prog";
  case ACT_PROG:	    return ">act_prog";
  case SPEECH_PROG:	    return ">speech_prog";
  case RAND_PROG:	    return ">rand_prog";
  case FIGHT_PROG:	    return ">fight_prog";
  case HITPRCNT_PROG:	return ">hitprcnt_prog";
  case DEATH_PROG:	    return ">death_prog";
  case ENTRY_PROG:	    return ">entry_prog";
  case GREET_PROG:	    return ">greet_prog";
  case ALL_GREET_PROG:	return ">all_greet_prog";
  case GIVE_PROG:	    return ">give_prog";
  case BRIBE_PROG:	    return ">bribe_prog";
  }
  return ">ERROR_PROG";
}

/*-------------------------------------------------------------------*/

/*
 * Display the MobProgs.
 */
void medit_disp_mprog(struct descriptor_data *d)
{
  struct mob_prog_data *mprog = OLC_MPROGL(d);

  OLC_MTOTAL(d) = 1;

#if defined(CLEAR_SCREEN)
  send_to_char("^[[H^[[J", d->character);
#endif
  while (mprog) 
        {sprintf(buf, "%d) %s %s\r\n", OLC_MTOTAL(d), medit_get_mprog_type(mprog),
		         (mprog->arglist ? mprog->arglist : "NONE"));
         send_to_char(buf, d->character);
         OLC_MTOTAL(d)++;
         mprog = mprog->next;
        }
  sprintf(buf,  
          "%d) Создать новую Mob Prog\r\n"
		  "%d) Очистить Mob Prog\r\n"
		  "Введите номер для редактирования [0 - выход]:  ",
		  OLC_MTOTAL(d), OLC_MTOTAL(d) + 1);
  send_to_char(buf, d->character);
  OLC_MODE(d) = MEDIT_MPROG;
}

/*-------------------------------------------------------------------*/

/*
 * Change the MobProgs.
 */
void medit_change_mprog(struct descriptor_data *d)
{
#if defined(CLEAR_SCREEN)
  send_to_char("^[[H^[[J", d->character);
#endif
  sprintf(buf,  
          "1) Type: %s\r\n"
		  "2) Args: %s\r\n"
		  "3) Commands:\r\n%s\r\n\r\n"
		  "Введите номер для редактирования [0 - выход]: ",
	medit_get_mprog_type(OLC_MPROG(d)),
	(OLC_MPROG(d)->arglist ? OLC_MPROG(d)->arglist : "NONE"),
	(OLC_MPROG(d)->comlist ? OLC_MPROG(d)->comlist : "NONE"));

  send_to_char(buf, d->character);
  OLC_MODE(d) = MEDIT_CHANGE_MPROG;
}

/*-------------------------------------------------------------------*/

/*
 * Change the MobProg type.
 */
void medit_disp_mprog_types(struct descriptor_data *d)
{
  int i;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("^[[H^[[J", d->character);
#endif

  for (i = 0; i < NUM_PROGS-1; i++) 
      {sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, mobprog_types[i]);
       send_to_char(buf, d->character);
      }
  send_to_char("Введите тип mob prog : ", d->character);
  OLC_MODE(d) = MEDIT_MPROG_TYPE;
}
#endif

/*-------------------------------------------------------------------*/

/*
 * Display the gender of the mobile.
 */
void medit_disp_sex(struct descriptor_data *d)
{
  int i;

  get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (i = 0; i < NUM_GENDERS; i++) 
      {sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, genders[i]);
       send_to_char(buf, d->character);
      }
  send_to_char("Выберите пол : ", d->character);
}

/*-------------------------------------------------------------------*/

/*
 * Display attack types menu.
 */
void medit_disp_attack_types(struct descriptor_data *d)
{
  int i;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (i = 0; i < NUM_ATTACK_TYPES; i++) 
      {sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, attack_hit_text[i].singular);
       send_to_char(buf, d->character);
      }
  send_to_char("Выберите тип удара : ", d->character);
}

/*-------------------------------------------------------------------*/
void medit_disp_helpers(struct descriptor_data *d)
{
  int    columns = 0;
  struct helper_data_type *helper;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  send_to_char("Установлены мобы-помощники :\r\n",d->character);
  for (helper = OLC_MOB(d)->helpers; helper; helper = helper->next_helper)
      {sprintf(buf, "%s%6d%s %s", grn, helper->mob_vnum, nrm,
		       !(++columns % 6) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  if (!columns)
     {sprintf(buf,"%sНЕТ%s\r\n",cyn,nrm);
      send_to_char(buf,d->character);
     }
  send_to_char("Укажите vnum моба-помощника (0 - конец) : ", d->character);
}

void medit_disp_skills(struct descriptor_data *d)
{
  int    columns = 0, counter;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 1; counter <= MAX_SKILLS; counter++)
      {if (!skill_info[counter].name || *skill_info[counter].name == '!')
          continue;
       if (GET_SKILL(OLC_MOB(d),counter))
          sprintf(buf1,"%s[%3d]%s",cyn,GET_SKILL(OLC_MOB(d),counter),nrm);
       else
          strcpy(buf1, "     ");
       sprintf(buf, "%s%3d%s) %25s%s%s", grn, counter, nrm,
               skill_info[counter].name, buf1,
		       !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  send_to_char("\r\nУкажите номер и уровень владения умением (0 - конец) : ", d->character);
}

void medit_disp_spells(struct descriptor_data *d)
{
  int    columns = 0, counter;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 1; counter <= MAX_SPELLS; counter++)
      {if (!spell_info[counter].name || *spell_info[counter].name == '!')
          continue;
       if (GET_SPELL_MEM(OLC_MOB(d),counter))
          sprintf(buf1,"%s[%3d]%s",cyn,GET_SPELL_MEM(OLC_MOB(d),counter),nrm);
       else
          strcpy(buf1, "     ");
       sprintf(buf, "%s%3d%s) %25s%s%s", grn, counter, nrm,
               spell_info[counter].name, buf1,
		       !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  send_to_char("\r\nУкажите номер и количество заклинаний (0 - конец) : ", d->character);
}



/*
 * Display mob-flags menu.
 */
void medit_disp_mob_flags(struct descriptor_data *d)
{
  int  columns = 0, plane=0, counter;
  char c;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0, c = 'a'-1; plane < NUM_PLANES; counter++) 
      {if (*action_bits[counter] == '\n')
          {plane++;
           c = 'a'-1;
           continue;
          }
       else
       if (c == 'z')
          c = 'A';
       else
          c++;
       sprintf(buf, "%s%c%d%s) %-20.20s %s", grn, c, plane, nrm,
		       action_bits[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  sprintbits(OLC_MOB(d)->char_specials.saved.act, action_bits, buf1, ",");
  sprintf(buf, "\r\nТекущие флаги : %s%s%s\r\nВыберите флаг (0 - выход) : ",
		  cyn, buf1, nrm);
  send_to_char(buf, d->character);
}

void medit_disp_npc_flags(struct descriptor_data *d)
{
  int  columns = 0, plane=0, counter;
  char c;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0, c = 'a'-1; plane < NUM_PLANES; counter++) 
      {if (*function_bits[counter] == '\n')
          {plane++;
           c = 'a'-1;
           continue;
          }
       else
       if (c == 'z')
          c = 'A';
       else
          c++;
       sprintf(buf, "%s%c%d%s) %-20.20s %s", grn, c, plane, nrm,
		       function_bits[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  sprintbits(OLC_MOB(d)->mob_specials.npc_flags, function_bits, buf1, ",");
  sprintf(buf, "\r\nТекущие флаги : %s%s%s\r\nВыберите флаг (0 - выход) : ",
		  cyn, buf1, nrm);
  send_to_char(buf, d->character);
}


/*-------------------------------------------------------------------*/

/*
 * Display affection flags menu.
 */
void medit_disp_aff_flags(struct descriptor_data *d)
{
  int  columns = 0, plane = 0, counter;
  char c;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0, c = 'a'-1; plane < NUM_PLANES; counter++) 
      {if (*affected_bits[counter] == '\n')
          {plane++;
           c = 'a'-1;
           continue;
          }
       else
       if (c == 'z')
          c = 'A';
       else
          c++;
       
       sprintf(buf, "%s%c%d%s) %-20.20s %s", grn, c, plane, nrm,
		       affected_bits[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  sprintbits(OLC_MOB(d)->char_specials.saved.affected_by, affected_bits, buf1, ",");
  sprintf(buf, "\r\nCurrent flags   : %s%s%s\r\nEnter aff flags (0 to quit) : ",
			  cyn, buf1, nrm);
  send_to_char(buf, d->character);
}

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void medit_disp_menu(struct descriptor_data *d)
{ int    i;
  struct char_data *mob;

  mob = OLC_MOB(d);
  get_char_cols(d->character);

  sprintf(buf,
#if defined(CLEAR_SCREEN)
"[H[J"
#endif
	  "-- МОБ:  [%s%d%s]\r\n"
	  "%s1%s) Пол: %s%-7.7s%s"
	  "%s2%s) Синонимы: %s%s\r\n"
	  "%s3%s) Именительный (это кто)         : %s%s\r\n"	  
	  "%s4%s) Родительный (нет кого)         : %s%s\r\n"
	  "%s5%s) Дательный  (дать кому)         : %s%s\r\n"	  
	  "%s6%s) Винительный (ударить кого)     : %s%s\r\n"
	  "%s7%s) Творительный (сражаться с кем) : %s%s\r\n"	  
	  "%s8%s) Предложный (ехать на ком)      : %s%s\r\n"
	  "%s9%s) Короткое :-\r\n%s%s"
	  "%sA%s) Детальное:-\r\n%s%s"
      "%sB%s) Уровень:     [%s%4d%s],%sC%s) Наклонности:  [%s%4d%s]\r\n"
      "%sD%s) Хитролы:     [%s%4d%s],%sE%s) Дамролы:      [%s%4d%s]\r\n"
      "%sF%s) NumDamDice:  [%s%4d%s],%sG%s) SizeDamDice:  [%s%4d%s]\r\n"
	  "%sH%s) Num HP Dice: [%s%4d%s],%sI%s) Size HP Dice: [%s%4d%s],  %sJ%s) HP Bonus: [%s%5d%s]\r\n"
	  "%sK%s) Armor Class: [%s%4d%s],%sL%s) Опыт:         [%s%9ld%s]\r\n"
	  "%sM%s) Gold:  [%s%4d%s],%sN%s) NumGoldDice:  [%s%4d%s],%sO%s) SizeGoldDice: [%s%4d%s]\r\n",
	  cyn, OLC_NUM(d), nrm,
	  grn, nrm, yel, genders[(int)GET_SEX(mob)], nrm,
	  grn, nrm, yel, GET_ALIAS(mob),
	  grn, nrm, yel, GET_PAD(mob,0),
	  grn, nrm, yel, GET_PAD(mob,1),
	  grn, nrm, yel, GET_PAD(mob,2),
	  grn, nrm, yel, GET_PAD(mob,3),
	  grn, nrm, yel, GET_PAD(mob,4),
	  grn, nrm, yel, GET_PAD(mob,5),
	  grn, nrm, yel, GET_LDESC(mob),
	  grn, nrm, yel, GET_DDESC(mob),
	  grn, nrm, cyn, GET_LEVEL(mob), nrm,
	  grn, nrm, cyn, GET_ALIGNMENT(mob), nrm,
	  grn, nrm, cyn, GET_HR(mob), nrm,
	  grn, nrm, cyn, GET_DR(mob), nrm,
	  grn, nrm, cyn, GET_NDD(mob), nrm,
	  grn, nrm, cyn, GET_SDD(mob), nrm,
	  grn, nrm, cyn, GET_MANA_NEED(mob), nrm,
	  grn, nrm, cyn, GET_MANA_STORED(mob), nrm,
	  grn, nrm, cyn, GET_HIT(mob), nrm,
	  grn, nrm, cyn, GET_AC(mob), nrm,
	  grn, nrm, cyn, GET_EXP(mob), nrm,
	  grn, nrm, cyn, GET_GOLD(mob), nrm,
	  grn, nrm, cyn, GET_GOLD_NoDs(mob), nrm,
	  grn, nrm, cyn, GET_GOLD_SiDs(mob), nrm
	  );
  send_to_char(buf, d->character);

  sprintbits(mob->char_specials.saved.act, action_bits, buf1, ",");
  sprintbits(mob->char_specials.saved.affected_by, affected_bits, buf2, ",");
  sprintf(buf,
	  "%sP%s) Положение     : %s%s\r\n"
	  "%sR%s) По умолчанию  : %s%s\r\n"
	  "%sT%s) Тип атаки     : %s%s\r\n"
	  "%sU%s) MOB Flags     : %s%s\r\n"
	  "%sV%s) AFF Flags     : %s%s\r\n",
	  grn, nrm, yel, position_types[(int)GET_POS(mob)],
	  grn, nrm, yel, position_types[(int)GET_DEFAULT_POS(mob)],
	  grn, nrm, yel, attack_hit_text[GET_ATTACK(mob)].singular,
	  grn, nrm, cyn, buf1,
	  grn, nrm, cyn, buf2);
  send_to_char(buf, d->character);
  
  sprintbits(mob->mob_specials.npc_flags, function_bits, buf1, ",");
  *buf2 = '\0';
  if (GET_DEST(mob) == NOWHERE)
     strcpy(buf2,"-1,");
  else
     for (i = 0; i < mob->mob_specials.dest_count; i++)
         sprintf(buf2+strlen(buf2),"%d,",mob->mob_specials.dest[i]);
  *(buf2+strlen(buf2)-1) = '\0';
  sprintf(buf,
      "%sW%s) NPC flags : %s%s\r\n"
#if defined(OASIS_MPROG)
	  "%sX%s) Mob Progs : %s%s\r\n"
#endif
      "%sY%s) Destination: %s%s\r\n"
      "%sZ%s) Helpers    : %s%s\r\n"
      "%sА%s) Skills     : \r\n"
      "%sБ%s) Spells     : \r\n"
      "%sВ%s) Сила : [%s%4d%s],%sГ%s) Ловк : [%s%4d%s],%sД%s) Тело : [%s%4d%s]\r\n"
      "%sЕ%s) Мудр : [%s%4d%s],%sЖ%s) Ум   : [%s%4d%s],%sЗ%s) Обая : [%s%4d%s]\r\n"
      "%sИ%s) Рост : [%s%4d%s],%sК%s) Вес  : [%s%4d%s],%sЛ%s) Разм : [%s%4d%s]\r\n"
      "%sМ%s) Экстраатаки: [%s%4d%s]\r\n"      
      "%sS%s) Script     : %s%s\r\n"
	  "%sQ%s) Quit\r\n"
	  "Ваш выбор : ",
	  grn, nrm, cyn, buf1,
#if defined(OASIS_MPROG)
	  grn, nrm, cyn, (OLC_MPROGL(d) ? "Set." : "Not Set."),
#endif
      grn, nrm, cyn, buf2,
      grn, nrm, cyn, mob->helpers ? "Yes" : "No",
      grn, nrm,
      grn, nrm,
      grn, nrm, cyn, GET_STR(mob), nrm,
      grn, nrm, cyn, GET_DEX(mob), nrm,      
      grn, nrm, cyn, GET_CON(mob), nrm,
      grn, nrm, cyn, GET_WIS(mob), nrm,      
      grn, nrm, cyn, GET_INT(mob), nrm,
      grn, nrm, cyn, GET_CHA(mob), nrm,      
      grn, nrm, cyn, GET_HEIGHT(mob), nrm,
      grn, nrm, cyn, GET_WEIGHT(mob), nrm,
      grn, nrm, cyn, GET_SIZE(mob), nrm,
      grn, nrm, cyn, mob->mob_specials.ExtraAttack, nrm,      
      grn, nrm, cyn, mob->proto_script ? "Set." : "Not Set.",
	  grn, nrm
	  );
  send_to_char(buf, d->character);

  OLC_MODE(d) = MEDIT_MAIN_MENU;
}

/************************************************************************
 *			The GARGANTAUN event handler			*
 ************************************************************************/

void medit_parse(struct descriptor_data *d, char *arg)
{ struct helper_data_type *helper, *temp;
  int i, number, plane, bit;

  if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE) 
     {if (!*arg || (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1]))))) 
         {send_to_char("Это числовое поле, повторите ввод : ", d->character);
          return;
         }
     }
  switch (OLC_MODE(d)) 
  {
/*-------------------------------------------------------------------*/
   case MEDIT_CONFIRM_SAVESTRING:
    /*
     * Ensure mob has MOB_ISNPC set or things will go pair shaped.
     */
    SET_BIT(MOB_FLAGS(OLC_MOB(d), MOB_ISNPC), MOB_ISNPC);
    switch (*arg) 
    {case 'y': case 'Y': case 'д': case 'Д' :
      /*
       * Save the mob in memory and to disk.
       */
      send_to_char("Saving mobile to memory.\r\n", d->character);
      medit_save_internally(d);
      sprintf(buf, "OLC: %s edits mob %d", GET_NAME(d->character), OLC_NUM(d));
      mudlog(buf, CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE);
      /* FALL THROUGH */
     case 'n':  case 'N': case 'н': case 'Н':
      cleanup_olc(d, CLEANUP_ALL);
      return;
     default:
      send_to_char("Неверный выбор!\r\n", d->character);
      send_to_char("Вы хотите сохранить моба ? : ", d->character);
      return;
    }
    break;

/*-------------------------------------------------------------------*/
  case MEDIT_MAIN_MENU:
    i = 0;
    switch (*arg) 
    {case 'q': case 'Q':
      if (OLC_VAL(d)) 
         {/* Anything been changed? */
	      send_to_char("Вы желаете сохранить изменения моба ? (y/n) : ", d->character);
	      OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
         } 
      else
	     cleanup_olc(d, CLEANUP_ALL);
      return;
     case '1':
      OLC_MODE(d) = MEDIT_SEX;
      medit_disp_sex(d);
      return;
    case '2':
      OLC_MODE(d) = MEDIT_ALIAS;
      i--;
      break;
    case '3':
      OLC_MODE(d) = MEDIT_PAD0;
      i--;
      break;
    case '4':
      OLC_MODE(d) = MEDIT_PAD1;
      i--;
      break;
    case '5':
      OLC_MODE(d) = MEDIT_PAD2;
      i--;
      break;
    case '6':
      OLC_MODE(d) = MEDIT_PAD3;
      i--;
      break;
    case '7':
      OLC_MODE(d) = MEDIT_PAD4;
      i--;
      break;
    case '8':
      OLC_MODE(d) = MEDIT_PAD5;
      i--;
      break;
    case '9':
      OLC_MODE(d) = MEDIT_L_DESC;
      i--;
      break;
    case 'a': case 'A':
      OLC_MODE(d) = MEDIT_D_DESC;
      SEND_TO_Q("Введите описание моба: (/s сохранить /h помощь)\r\n\r\n", d);
      d->backstr = NULL;
      if (OLC_MOB(d)->player.description) 
         {SEND_TO_Q(OLC_MOB(d)->player.description, d);
	      d->backstr = str_dup(OLC_MOB(d)->player.description);
         }
      d->str = &OLC_MOB(d)->player.description;
      d->max_str = MAX_MOB_DESC;
      d->mail_to = 0;
      OLC_VAL(d) = 1;
      return;
    case 'b': case 'B':
      OLC_MODE(d) = MEDIT_LEVEL;
      i++;
      break;
    case 'c': case 'C':
      OLC_MODE(d) = MEDIT_ALIGNMENT;
      i++;
      break;
    case 'd': case 'D':
      OLC_MODE(d) = MEDIT_HITROLL;
      i++;
      break;
    case 'e': case 'E':
      OLC_MODE(d) = MEDIT_DAMROLL;
      i++;
      break;
    case 'f': case 'F':
      OLC_MODE(d) = MEDIT_NDD;
      i++;
      break;
    case 'g': case 'G':
      OLC_MODE(d) = MEDIT_SDD;
      i++;
      break;
    case 'h': case 'H':
      OLC_MODE(d) = MEDIT_NUM_HP_DICE;
      i++;
      break;
    case 'i': case 'I':
      OLC_MODE(d) = MEDIT_SIZE_HP_DICE;
      i++;
      break;
    case 'j': case 'J':
      OLC_MODE(d) = MEDIT_ADD_HP;
      i++;
      break;
    case 'k': case 'K':
      OLC_MODE(d) = MEDIT_AC;
      i++;
      break;
    case 'l': case 'L':
      OLC_MODE(d) = MEDIT_EXP;
      i++;
      break;
    case 'm': case 'M':
      OLC_MODE(d) = MEDIT_GOLD;
      i++;
      break;
    case 'n': case 'N':
      OLC_MODE(d) = MEDIT_GOLD_DICE;
      i++;
      break;
    case 'o': case 'O':
      OLC_MODE(d) = MEDIT_GOLD_SIZE;
      i++;
      break;
    case 'p': case 'P':
      OLC_MODE(d) = MEDIT_POS;
      medit_disp_positions(d);
      return;
    case 'r': case 'R':
      OLC_MODE(d) = MEDIT_DEFAULT_POS;
      medit_disp_positions(d);
      return;
    case 't': case 'T':
      OLC_MODE(d) = MEDIT_ATTACK;
      medit_disp_attack_types(d);
      return;
    case 'u': case 'U':
      OLC_MODE(d) = MEDIT_MOB_FLAGS;
      medit_disp_mob_flags(d);
      return;
    case 'v': case 'V':
      OLC_MODE(d) = MEDIT_AFF_FLAGS;
      medit_disp_aff_flags(d);
      return;
    case 'w': case 'W':
      OLC_MODE(d) = MEDIT_NPC_FLAGS;
      medit_disp_npc_flags(d);
      return;
      
#if defined(OASIS_MPROG)
    case 'x':  case 'X':
      OLC_MODE(d) = MEDIT_MPROG;
      medit_disp_mprog(d);
      return;
#endif
    case 's': case 'S':
      OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
      dg_script_menu(d);
      return;
    case 'y': case 'Y':
      OLC_MODE(d) = MEDIT_DESTINATION;
      i++;
      break;
    case 'z': case 'Z':
      OLC_MODE(d) = MEDIT_HELPERS;
      medit_disp_helpers(d);
      return;
    case 'а': case 'А':
      OLC_MODE(d) = MEDIT_SKILLS;
      medit_disp_skills(d);
      return;
    case 'б': case 'Б':
      OLC_MODE(d) = MEDIT_SPELLS;
      medit_disp_spells(d);
      return;
    case 'в': case 'В':
      OLC_MODE(d) = MEDIT_STR;
      i++;
      break;
    case 'г': case 'Г':
      OLC_MODE(d) = MEDIT_DEX;
      i++;
      break;
    case 'д': case 'Д':
      OLC_MODE(d) = MEDIT_CON;
      i++;
      break;
    case 'е': case 'Е':
      OLC_MODE(d) = MEDIT_WIS;
      i++;
      break;
    case 'ж': case 'Ж':
      OLC_MODE(d) = MEDIT_INT;
      i++;
      break;
    case 'з': case 'З':
      OLC_MODE(d) = MEDIT_CHA;
      i++;
      break;
    case 'и': case 'И':
      OLC_MODE(d) = MEDIT_HEIGHT;
      i++;
      break;
    case 'к': case 'К':
      OLC_MODE(d) = MEDIT_WEIGHT;
      i++;
      break;
    case 'л': case 'Л':
      OLC_MODE(d) = MEDIT_SIZE;
      i++;
      break;
    case 'м': case 'М':
      OLC_MODE(d) = MEDIT_EXTRA;
      i++;
      break;
      
    default:
      medit_disp_menu(d);
      return;
    }
    if (i != 0) 
       {send_to_char(i == 1  ? "\r\nВведите новое значение : "   :
		             i == -1 ? "\r\nВведите новый текст :\r\n] " :
			                   "\r\nОпаньки...:\r\n", d->character);
        return;
       }
    break;


/*-------------------------------------------------------------------*/
  case OLC_SCRIPT_EDIT:
    if (dg_script_edit_parse(d, arg)) 
       return;
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_ALIAS:
    if (GET_ALIAS(OLC_MOB(d)))
       free(GET_ALIAS(OLC_MOB(d)));
    GET_ALIAS(OLC_MOB(d)) = str_dup((arg && *arg) ? arg : "неопределен");
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_PAD0:
    if (GET_PAD(OLC_MOB(d),0))
       free(GET_PAD(OLC_MOB(d),0));
    GET_PAD(OLC_MOB(d),0) = str_dup((arg && *arg) ? arg : "кто-то");
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_PAD1:
    if (GET_PAD(OLC_MOB(d),1))
       free(GET_PAD(OLC_MOB(d),1));
    GET_PAD(OLC_MOB(d),1) = str_dup((arg && *arg) ? arg : "кого-то");
    GET_NAME(OLC_MOB(d))  = str_dup(GET_PAD(OLC_MOB(d),1));
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_PAD2:
    if (GET_PAD(OLC_MOB(d),2))
       free(GET_PAD(OLC_MOB(d),2));
    GET_PAD(OLC_MOB(d),2) = str_dup((arg && *arg) ? arg : "кому-то");
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_PAD3:
    if (GET_PAD(OLC_MOB(d),3))
       free(GET_PAD(OLC_MOB(d),3));
    GET_PAD(OLC_MOB(d),3) = str_dup((arg && *arg) ? arg : "кого-то");
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_PAD4:
    if (GET_PAD(OLC_MOB(d),4))
       free(GET_PAD(OLC_MOB(d),4));
    GET_PAD(OLC_MOB(d),4) = str_dup((arg && *arg) ? arg : "кем-то");
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_PAD5:
    if (GET_PAD(OLC_MOB(d),5))
       free(GET_PAD(OLC_MOB(d),5));
    GET_PAD(OLC_MOB(d),5) = str_dup((arg && *arg) ? arg : "о ком-то");
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_L_DESC:
      if (GET_LDESC(OLC_MOB(d)))
         free(GET_LDESC(OLC_MOB(d)));
      if (arg && *arg) 
         {strcpy(buf, arg);
          strcat(buf, "\r\n");
          GET_LDESC(OLC_MOB(d)) = str_dup(buf);
         } 
      else
         GET_LDESC(OLC_MOB(d)) = str_dup("неопределен");

    break;
/*-------------------------------------------------------------------*/
  case MEDIT_D_DESC:
    /*
     * We should never get here.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: medit_parse(): Reached D_DESC case!",
			BRF, LVL_BUILDER, TRUE);
    send_to_char("Опаньки...\r\n", d->character);
    break;
/*-------------------------------------------------------------------*/
#if defined(OASIS_MPROG)
  case MEDIT_MPROG_COMLIST:
    /*
     * We should never get here, but if we do, bail out.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: medit_parse(): Reached MPROG_COMLIST case!",
			BRF, LVL_BUILDER, TRUE);
    break;
#endif
/*-------------------------------------------------------------------*/
  case MEDIT_MOB_FLAGS:
    number = planebit(arg,&plane,&bit);
    if (number < 0) 
       {medit_disp_mob_flags(d);
        return;
       } 
    else 
    if (number == 0)
       break;
    else 
       {TOGGLE_BIT(OLC_MOB(d)->char_specials.saved.act.flags[plane], 1 << (bit));
        medit_disp_mob_flags(d);
        return;
       }

/*-------------------------------------------------------------------*/
  case MEDIT_NPC_FLAGS:
    number = planebit(arg,&plane,&bit);
    if (number < 0) 
       {medit_disp_npc_flags(d);
        return;
       } 
    else 
    if (number == 0)
       break;
    else 
       {TOGGLE_BIT(OLC_MOB(d)->mob_specials.npc_flags.flags[plane], 1 << (bit));
        medit_disp_npc_flags(d);
        return;
       }
/*-------------------------------------------------------------------*/

  case MEDIT_AFF_FLAGS:
    number = planebit(arg,&plane,&bit);
    if (number < 0) 
       {medit_disp_aff_flags(d);
        return;
       } 
    else 
    if (number == 0)
       break;
    else 
       {TOGGLE_BIT(OLC_MOB(d)->char_specials.saved.affected_by.flags[plane], 1 << (bit));
        medit_disp_aff_flags(d);
        return;
       }
/*-------------------------------------------------------------------*/
#if defined(OASIS_MPROG)
  case MEDIT_MPROG:
    if ((i = atoi(arg)) == 0)
       medit_disp_menu(d);
    else 
    if (i == OLC_MTOTAL(d)) 
       {struct mob_prog_data *temp;
        CREATE(temp, struct mob_prog_data, 1);
        temp->next = OLC_MPROGL(d);
        temp->type = -1;
        temp->arglist = NULL;
        temp->comlist = NULL;
        OLC_MPROG(d) = temp;
        OLC_MPROGL(d) = temp;
        OLC_MODE(d) = MEDIT_CHANGE_MPROG;
        medit_change_mprog (d);
       } 
    else 
    if (i < OLC_MTOTAL(d)) 
       {struct mob_prog_data *temp;
        int x = 1;
        for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
            x++;
        OLC_MPROG(d) = temp;
        OLC_MODE(d) = MEDIT_CHANGE_MPROG;
        medit_change_mprog (d);
       } 
    else 
    if (i == OLC_MTOTAL(d) + 1) 
       {send_to_char("Какого моба Вы хотите очистить ? ", d->character);
        OLC_MODE(d) = MEDIT_PURGE_MPROG;
       }
    else
       medit_disp_menu(d);
    return;

  case MEDIT_PURGE_MPROG:
    if ((i = atoi(arg)) > 0 && i < OLC_MTOTAL(d)) 
       {struct mob_prog_data *temp;
        int x = 1;

        for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
            x++;
        OLC_MPROG(d) = temp;
        REMOVE_FROM_LIST(OLC_MPROG(d), OLC_MPROGL(d), next);
        free(OLC_MPROG(d)->arglist);
        free(OLC_MPROG(d)->comlist);
        free(OLC_MPROG(d));
        OLC_MPROG(d) = NULL;
        OLC_VAL(d) = 1;
       }
    medit_disp_mprog(d);
    return;

  case MEDIT_CHANGE_MPROG: 
       {if ((i = atoi(arg)) == 1)
           medit_disp_mprog_types(d);
        else 
        if (i == 2) 
           {send_to_char ("Введите новый список аргументов: ", d->character);
            OLC_MODE(d) = MEDIT_MPROG_ARGS;
           } 
        else 
        if (i == 3) 
           {send_to_char("Введите новую mob prog команду:\r\n", d->character);
            /*
             * Pass control to modify.c for typing.
             */
            OLC_MODE(d) = MEDIT_MPROG_COMLIST;
            d->backstr = NULL;
            if (OLC_MPROG(d)->comlist) 
               {SEND_TO_Q(OLC_MPROG(d)->comlist, d);
                d->backstr = str_dup(OLC_MPROG(d)->comlist);
               }
            d->str = &OLC_MPROG(d)->comlist;
            d->max_str = MAX_STRING_LENGTH;
            d->mail_to = 0;
            OLC_VAL(d) = 1;
           }
        else
           medit_disp_mprog(d);
    return;
#endif

/*-------------------------------------------------------------------*/

/*
 * Numerical responses.
 */

#if defined(OASIS_MPROG)
/*
  David Klasinc suggests for MEDIT_MPROG_TYPE:
    switch (atoi(arg)) {
      case 0: OLC_MPROG(d)->type = 0; break;
      case 1: OLC_MPROG(d)->type = 1; break;
      case 2: OLC_MPROG(d)->type = 2; break;
      case 3: OLC_MPROG(d)->type = 4; break;
      case 4: OLC_MPROG(d)->type = 8; break;
      case 5: OLC_MPROG(d)->type = 16; break;
      case 6: OLC_MPROG(d)->type = 32; break;
      case 7: OLC_MPROG(d)->type = 64; break;
      case 8: OLC_MPROG(d)->type = 128; break;
      case 9: OLC_MPROG(d)->type = 256; break;
      case 10: OLC_MPROG(d)->type = 512; break;
      case 11: OLC_MPROG(d)->type = 1024; break;
      default: OLC_MPROG(d)->type = -1; break;
    }
*/

  case MEDIT_MPROG_TYPE:
    OLC_MPROG(d)->type = (1 << MAX(0, MIN(atoi(arg), NUM_PROGS - 1)));
    OLC_VAL(d) = 1;
    medit_change_mprog(d);
    return;

  case MEDIT_MPROG_ARGS:
    OLC_MPROG(d)->arglist = str_dup(arg);
    OLC_VAL(d) = 1;
    medit_change_mprog(d);
    return;
#endif

  case MEDIT_SEX:
    GET_SEX(OLC_MOB(d)) = MAX(0, MIN(NUM_GENDERS - 1, atoi(arg)));
    break;

  case MEDIT_HITROLL:
    GET_HR(OLC_MOB(d)) = MAX(0, MIN(50, atoi(arg)));
    break;

  case MEDIT_DAMROLL:
    GET_DR(OLC_MOB(d)) = MAX(0, MIN(50, atoi(arg)));
    break;

  case MEDIT_NDD:
    GET_NDD(OLC_MOB(d)) = MAX(0, MIN(30, atoi(arg)));
    break;

  case MEDIT_SDD:
    GET_SDD(OLC_MOB(d)) = MAX(0, MIN(127, atoi(arg)));
    break;

  case MEDIT_NUM_HP_DICE:
    GET_MANA_NEED(OLC_MOB(d)) = MAX(0, MIN(30, atoi(arg)));
    break;

  case MEDIT_SIZE_HP_DICE:
    GET_MANA_STORED(OLC_MOB(d)) = MAX(0, MIN(1000, atoi(arg)));
    break;

  case MEDIT_ADD_HP:
    GET_HIT(OLC_MOB(d)) = MAX(0, MIN(30000, atoi(arg)));
    break;

  case MEDIT_AC:
    GET_AC(OLC_MOB(d)) = MAX(-200, MIN(200, atoi(arg)));
    break;

  case MEDIT_EXP:
    GET_EXP(OLC_MOB(d)) = MAX(0, atoi(arg));
    break;

  case MEDIT_GOLD:
    GET_GOLD(OLC_MOB(d)) = MAX(0, atoi(arg));
    break;

  case MEDIT_GOLD_DICE:
    GET_GOLD_NoDs(OLC_MOB(d)) = MAX(0, atoi(arg));
    break;

  case MEDIT_GOLD_SIZE:
    GET_GOLD_SiDs(OLC_MOB(d)) = MAX(0, atoi(arg));
    break;

  case MEDIT_POS:
    GET_POS(OLC_MOB(d)) = MAX(0, MIN(NUM_POSITIONS - 1, atoi(arg)));
    break;

  case MEDIT_DEFAULT_POS:
    GET_DEFAULT_POS(OLC_MOB(d)) = MAX(0, MIN(NUM_POSITIONS - 1, atoi(arg)));
    break;

  case MEDIT_ATTACK:
    GET_ATTACK(OLC_MOB(d)) = MAX(0, MIN(NUM_ATTACK_TYPES - 1, atoi(arg)));
    break;

  case MEDIT_LEVEL:
    GET_LEVEL(OLC_MOB(d)) = MAX(1, MIN(50, atoi(arg)));
    break;

  case MEDIT_ALIGNMENT:
    GET_ALIGNMENT(OLC_MOB(d)) = MAX(-1000, MIN(1000, atoi(arg)));
    break;

  case MEDIT_DESTINATION:
    number = atoi(arg);
    if ((plane = real_room(number)) < 0)
       send_to_char("Нет такой комнаты.\r\n",d->character);
    else
       {for (plane = 0; plane < OLC_MOB(d)->mob_specials.dest_count; plane++)
            if (number == OLC_MOB(d)->mob_specials.dest[plane])
               {OLC_MOB(d)->mob_specials.dest_count--; 
                for (;plane < OLC_MOB(d)->mob_specials.dest_count; plane++)
                    OLC_MOB(d)->mob_specials.dest[plane] = OLC_MOB(d)->mob_specials.dest[plane+1];
                OLC_MOB(d)->mob_specials.dest[plane] = 0;
                plane++;
                break;
               }
         if (plane == OLC_MOB(d)->mob_specials.dest_count && plane < MAX_DEST)
            {OLC_MOB(d)->mob_specials.dest_count++;
             OLC_MOB(d)->mob_specials.dest[plane] = number;
            }
      }
    break;
    
  case MEDIT_HELPERS:
    number = atoi(arg);
    if (number == 0)
       break;
    if ((plane = real_mobile(number)) < 0)
       send_to_char("Нет такого моба.",d->character);
    else
       {for (helper = OLC_MOB(d)->helpers; helper; helper = helper->next_helper)
            if (helper->mob_vnum == number)
               break;
        if (helper)
           {REMOVE_FROM_LIST(helper, OLC_MOB(d)->helpers, next_helper);
           }
        else
           {CREATE(helper, struct helper_data_type, 1);
            helper->mob_vnum    = number;
            helper->next_helper = OLC_MOB(d)->helpers;
            OLC_MOB(d)->helpers = helper;
           }
       }
    medit_disp_helpers(d);
    return;    

  case MEDIT_SKILLS:
    number = atoi(arg);
    if (number == 0)
       break;
    if (number > MAX_SKILLS || !skill_info[number].name || 
        *skill_info[number].name == '!')
       send_to_char("Неизвестное умение.\r\n",d->character);
    else
    if (GET_SKILL(OLC_MOB(d),number))
       GET_SKILL(OLC_MOB(d),number) = 0;
    else
    if (sscanf(arg,"%d %d",&plane,&bit) < 2)
       send_to_char("Не указан уровень владения умением.\r\n",d->character);
    else
       GET_SKILL(OLC_MOB(d),number) = MIN(200,MAX(0,bit));
    medit_disp_skills(d);
    return;    

  case MEDIT_SPELLS:
    number = atoi(arg);
    if (number == 0)
       break;
    if (number > MAX_SPELLS || !spell_info[number].name || 
        *spell_info[number].name == '!')
       send_to_char("Неизвестное заклинание.\r\n",d->character);
    else
    if (sscanf(arg,"%d %d",&plane,&bit) < 2)
       send_to_char("Не указано количество заклинаний.\r\n",d->character);
    else
       GET_SPELL_MEM(OLC_MOB(d),number) = MIN(200,MAX(0,bit));
    medit_disp_spells(d);
    return;    

  case MEDIT_STR:
    GET_STR(OLC_MOB(d)) = MIN(50,MAX(1,atoi(arg)));
    break; 

  case MEDIT_DEX:
    GET_DEX(OLC_MOB(d)) = MIN(50,MAX(1,atoi(arg)));
    break; 

  case MEDIT_CON:
    GET_CON(OLC_MOB(d)) = MIN(50,MAX(1,atoi(arg)));
    break; 

  case MEDIT_WIS:
    GET_WIS(OLC_MOB(d)) = MIN(50,MAX(1,atoi(arg)));
    break; 

  case MEDIT_INT:
    GET_INT(OLC_MOB(d)) = MIN(50,MAX(1,atoi(arg)));
    break; 

  case MEDIT_CHA:
    GET_CHA(OLC_MOB(d)) = MIN(50,MAX(1,atoi(arg)));
    break; 

  case MEDIT_WEIGHT:
    GET_WEIGHT(OLC_MOB(d)) = MIN(200,MAX(50,atoi(arg)));
    break; 

  case MEDIT_HEIGHT:
    GET_HEIGHT(OLC_MOB(d)) = MIN(200,MAX(50,atoi(arg)));
    break; 

  case MEDIT_SIZE:
    GET_SIZE(OLC_MOB(d)) = MIN(100,MAX(1,atoi(arg)));
    break; 

  case MEDIT_EXTRA:
    OLC_MOB(d)->mob_specials.ExtraAttack = MIN(5,MAX(0,atoi(arg)));
    break; 

/*-------------------------------------------------------------------*/
  default:
    /*
     * We should never get here.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: medit_parse(): Reached default case!", BRF, LVL_BUILDER, TRUE);
    send_to_char("Oops...\r\n", d->character);
    break;

  }
/*-------------------------------------------------------------------*/

/*
 * END OF CASE 
 * If we get here, we have probably changed something, and now want to
 * return to main menu.  Use OLC_VAL as a 'has changed' flag  
 */

  OLC_VAL(d) = 1;
  medit_disp_menu(d);
}
/*
 * End of medit_parse(), thank god.
 */
