/************************************************************************
 * OasisOLC - oedit.c						v1.5	*
 * Copyright 1996 Harvey Gilpin.					*
 * Original author: Levork						*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "spells.h"
#include "utils.h"
#include "db.h"
#include "boards.h"
#include "shop.h"
#include "olc.h"
#include "dg_olc.h"

/*------------------------------------------------------------------------*/

/*
 * External variable declarations.
 */

extern struct obj_data *obj_proto;
extern struct index_data *obj_index;
extern struct obj_data *object_list;
extern obj_rnum top_of_objt;
extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;
extern struct shop_data *shop_index;
extern int top_shop;
extern struct attack_hit_type attack_hit_text[];
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *extra_bits[];
extern const char *drinks[];
extern const char *apply_types[];
extern const char *container_bits[];
extern const char *anti_bits[];
extern const char *no_bits[];
extern const char *weapon_affects[];
extern const char *material_name[];
extern const char *ingradient_bits[];
extern struct spell_info_type spell_info[];
extern struct board_info_type board_info[];
extern struct descriptor_data *descriptor_list;

/*------------------------------------------------------------------------*/

/*
 * Handy macros.
 */
#define S_PRODUCT(s, i) ((s)->producing[(i)])

/*------------------------------------------------------------------------*/

void oedit_disp_container_flags_menu(struct descriptor_data *d);
void oedit_disp_extradesc_menu(struct descriptor_data *d);
void oedit_disp_weapon_menu(struct descriptor_data *d);
void oedit_disp_val1_menu(struct descriptor_data *d);
void oedit_disp_val2_menu(struct descriptor_data *d);
void oedit_disp_val3_menu(struct descriptor_data *d);
void oedit_disp_val4_menu(struct descriptor_data *d);
void oedit_disp_type_menu(struct descriptor_data *d);
void oedit_disp_extra_menu(struct descriptor_data *d);
void oedit_disp_wear_menu(struct descriptor_data *d);
void oedit_disp_menu(struct descriptor_data *d);

void oedit_parse(struct descriptor_data *d, char *arg);
void oedit_disp_spells_menu(struct descriptor_data *d);
void oedit_liquid_type(struct descriptor_data *d);
void oedit_setup_new(struct descriptor_data *d);
void oedit_setup_existing(struct descriptor_data *d, int real_num);
void oedit_save_to_disk(int zone);
void oedit_save_internally(struct descriptor_data *d);

/*------------------------------------------------------------------------*\
  Utility and exported functions
\*------------------------------------------------------------------------*/

void oedit_setup_new(struct descriptor_data *d)
{
  CREATE(OLC_OBJ(d), struct obj_data, 1);

  clear_object(OLC_OBJ(d));
  OLC_OBJ(d)->name              = str_dup("новый предмет");
  OLC_OBJ(d)->description       = str_dup("что-то новое лежит здесь");
  OLC_OBJ(d)->short_description = str_dup("новый предмет");
  OLC_OBJ(d)->PNames[0]         = str_dup("это что");
  OLC_OBJ(d)->PNames[1]         = str_dup("нету чего");
  OLC_OBJ(d)->PNames[2]         = str_dup("привязать к чему");
  OLC_OBJ(d)->PNames[3]         = str_dup("взять что");
  OLC_OBJ(d)->PNames[4]         = str_dup("вооружиться чем");
  OLC_OBJ(d)->PNames[5]         = str_dup("говорить о чем");
  GET_OBJ_WEAR(OLC_OBJ(d))      = ITEM_WEAR_TAKE;
  OLC_VAL(d)                    = 0;
  OLC_ITEM_TYPE(d)              = OBJ_TRIGGER;
  oedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void oedit_setup_existing(struct descriptor_data *d, int real_num)
{ int    i;
  struct extra_descr_data *this, *temp, *temp2;
  struct obj_data *obj;

  /*
   * Allocate object in memory.
   */
  CREATE(obj, struct obj_data, 1);

  clear_object(obj);
  *obj = obj_proto[real_num];

  /*
   * Copy all strings over.
   */
  obj->name              = str_dup(obj_proto[real_num].name ? obj_proto[real_num].name : "нет");
  obj->short_description = str_dup(obj_proto[real_num].short_description ?
                 	       obj_proto[real_num].short_description : "неопределено");
  obj->description = str_dup(obj_proto[real_num].description ?
 		                   obj_proto[real_num].description : "неопределено");
  obj->action_description = (obj_proto[real_num].action_description ?
  		                   str_dup(obj_proto[real_num].action_description) : NULL);
  for (i = 0; i < NUM_PADS; i++)
      obj->PNames[i] = str_dup(obj_proto[real_num].PNames[i] ? 
                               obj_proto[real_num].PNames[i] : "неопределен");
  /*
   * Extra descriptions if necessary.
   */
  if (obj_proto[real_num].ex_description) 
     {CREATE(temp, struct extra_descr_data, 1);

      obj->ex_description = temp;
      for (this = obj_proto[real_num].ex_description; this; this = this->next) 
          {temp->keyword = (this->keyword && *this->keyword) ? str_dup(this->keyword) : NULL;
           temp->description = (this->description && *this->description) ?
	                           str_dup(this->description) : NULL;
           if (this->next) 
              {CREATE(temp2, struct extra_descr_data, 1);
               temp->next = temp2;
               temp = temp2;
              } 
           else
              temp->next = NULL;
          }
     }

  if (SCRIPT(obj))
     script_copy(obj, &obj_proto[real_num], OBJ_TRIGGER);

  /*
   * Attach new object to player's descriptor.
   */
  OLC_OBJ(d)       = obj;
  OLC_VAL(d)       = 0;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
  dg_olc_script_copy(d);
  oedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

#define ZCMD zone_table[zone].cmd[cmd_no]

void oedit_save_internally(struct descriptor_data *d)
{
  int    i, shop, robj_num, found = FALSE, zone, cmd_no;
  struct extra_descr_data *this, *next_one;
  struct obj_data *obj, *swap, *new_obj_proto;
  struct index_data *new_obj_index;
  struct descriptor_data *dsc;

  /*
   * Write object to internal tables.
   */
  if ((robj_num = real_object(OLC_NUM(d))) >= 0) 
     {/*
       * We need to run through each and every object currently in the
       * game to see which ones are pointing to this prototype.
       * if object is pointing to this prototype, then we need to replace it
       * with the new one.
       */
      log("[OEdit] Save object to mem %d",robj_num);
      CREATE(swap, struct obj_data, 1);

      for (obj = object_list; obj; obj = obj->next) 
          {if (obj->item_number == robj_num) 
              {*swap = *obj;
	           *obj = *OLC_OBJ(d);
	           /*
	            * Copy game-time dependent variables over.
	            */
	           obj->in_room     = swap->in_room;
	           obj->item_number = robj_num;
	           obj->carried_by  = swap->carried_by;
	           obj->worn_by     = swap->worn_by;
	           obj->worn_on     = swap->worn_on;
	           obj->in_obj      = swap->in_obj;
	           obj->contains    = swap->contains;
	           obj->next_content= swap->next_content;
	           obj->next        = swap->next;
              }
          }
      free_obj(swap);
      /*
       * It is now safe to free the old prototype and write over it.
       */
      if (obj_proto[robj_num].name)
         free(obj_proto[robj_num].name);
      if (obj_proto[robj_num].description)
         free(obj_proto[robj_num].description);
      if (obj_proto[robj_num].short_description)
         free(obj_proto[robj_num].short_description);
      if (obj_proto[robj_num].action_description)
         free(obj_proto[robj_num].action_description);
      for (i = 0; i < NUM_PADS; i++)
          if (obj_proto[robj_num].PNames[i])
             free(obj_proto[robj_num].PNames[i]); 
      if (obj_proto[robj_num].ex_description)
         for (this = obj_proto[robj_num].ex_description; this; this = next_one) 
             {next_one = this->next;
	          if (this->keyword)
	             free(this->keyword);
	          if (this->description)
	             free(this->description);
	          free(this);
             }
      obj_proto[robj_num]             = *OLC_OBJ(d);
      obj_proto[robj_num].item_number = robj_num;
      obj_proto[robj_num].proto_script = OLC_SCRIPT(d);
     } 
  else 
     {/*
      * It's a new object, we must build new tables to contain it.
      */
      log("[OEdit] Save mem new %d(%d/%d)",OLC_NUM(d),(top_of_objt+2),sizeof(struct obj_data));
      
      CREATE(new_obj_index, struct index_data, top_of_objt + 2);
      CREATE(new_obj_proto, struct obj_data, top_of_objt + 2);
      
      /*
       * Start counting through both tables.
       */
      for (i = 0; i <= top_of_objt; i++) 
          {/*
            * If we haven't found it.
            */
           if (!found) 
              {/*
	            * Check if current virtual is bigger than our virtual number.
	            */
	           if (obj_index[i].vnum > OLC_NUM(d)) 
	              {found                          = TRUE;
	               robj_num                       = i;
	               OLC_OBJ(d)->item_number        = robj_num;
	               new_obj_index[robj_num].vnum   = OLC_NUM(d);
	               new_obj_index[robj_num].number = 0;
	               new_obj_index[robj_num].func   = NULL;
	               new_obj_proto[robj_num]        = *(OLC_OBJ(d));
                   new_obj_proto[robj_num].proto_script = OLC_SCRIPT(d);
	               new_obj_proto[robj_num].in_room      = NOWHERE;
	               /*
	                * Copy over the mob that should be here.
	                */
	               new_obj_index[robj_num + 1]             = obj_index[robj_num];
	               new_obj_proto[robj_num + 1]             = obj_proto[robj_num];
	               new_obj_proto[robj_num + 1].item_number = robj_num + 1;
	              } 
	           else 
	              {/*
	                * Just copy from old to new, no number change.
	                */
	               new_obj_proto[i] = obj_proto[i];
	               new_obj_index[i] = obj_index[i];
	              }
              } 
           else 
              {/*
  	            * We HAVE already found it, therefore copy to object + 1 
	            */
	           new_obj_index[i + 1] = obj_index[i];
	           new_obj_proto[i + 1] = obj_proto[i];
	           new_obj_proto[i + 1].item_number = i + 1;
              }
          }
      if (!found) 
         {robj_num = i;
          OLC_OBJ(d)->item_number = robj_num;
          new_obj_index[robj_num].vnum = OLC_NUM(d);
          new_obj_index[robj_num].number = 0;
          new_obj_index[robj_num].func = NULL;
          new_obj_proto[robj_num] = *(OLC_OBJ(d));
          new_obj_proto[robj_num].proto_script = OLC_SCRIPT(d);
          new_obj_proto[robj_num].in_room = NOWHERE;
         }
      /*
       * Free and replace old tables.
       */
      free(obj_proto);
      free(obj_index);
      obj_proto = new_obj_proto;
      obj_index = new_obj_index;
      top_of_objt++;
      
      /*
       * Renumber live objects.
       */
      for (obj = object_list; obj; obj = obj->next)
          if (GET_OBJ_RNUM(obj) >= robj_num)
 	          GET_OBJ_RNUM(obj)++;
      
      /*
       * Renumber zone table.
       */
      for (zone = 0; zone <= top_of_zone_table; zone++)
          for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
	          switch (ZCMD.command) 
	          {case 'P':
	                if (ZCMD.arg3 >= robj_num)
	                   ZCMD.arg3++;
	           /*
	            * No break here - drop into next case.
	            */
	           case 'O':
	           case 'G':
	           case 'E':
	                if (ZCMD.arg1 >= robj_num)
	                   ZCMD.arg1++;
	                break;
	           case 'R':
	                if (ZCMD.arg2 >= robj_num)
	                   ZCMD.arg2++;
	                break;
	          }

      /*
       * Renumber notice boards.
       */
      for (i = 0; i < NUM_OF_BOARDS; i++)
          if (BOARD_RNUM(i) >= robj_num)
	         BOARD_RNUM(i) = BOARD_RNUM(i) + 1;

      /*
       * Renumber shop produce.
       */
      for (shop = 0; shop < top_shop; shop++)
          for (i = 0; SHOP_PRODUCT(shop, i) != -1; i++)
	          if (SHOP_PRODUCT(shop, i) >= robj_num)
	             SHOP_PRODUCT(shop, i)++;

      /*
       * Renumber produce in shops being edited.
       */
      for (dsc = descriptor_list; dsc; dsc = dsc->next)
          if (dsc->connected == CON_SEDIT)
	         for (i = 0; S_PRODUCT(OLC_SHOP(dsc), i) != -1; i++)
	             if (S_PRODUCT(OLC_SHOP(dsc), i) >= robj_num)
	                S_PRODUCT(OLC_SHOP(dsc), i)++;
      
     }
  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_OBJ);
}

/*------------------------------------------------------------------------*/
void tascii(int *pointer, int num_planes, char *ascii)
{int    i, c, found;

 for (i = 0, found = FALSE; i < num_planes; i++)
     {for (c = 0; c < 31; c++)
          if (*(pointer+i) & (1 << c))
             {found = TRUE;
              sprintf(ascii+strlen(ascii),"%c%d", c < 26 ? c+'a' : c-26+'A', i);
             }
     }
 if (!found)
    strcat(ascii,"0 ");
 else
    strcat(ascii," ");
}

void oedit_save_to_disk(int zone_num)
{
  int counter, counter2, realcounter;
  FILE *fp;
  struct obj_data *obj;
  struct extra_descr_data *ex_desc;

  sprintf(buf, "%s/%d.new", OBJ_PREFIX, zone_table[zone_num].number);
  if (!(fp = fopen(buf, "w+"))) 
     {mudlog("SYSERR: OLC: Cannot open objects file!", BRF, LVL_BUILDER, TRUE);
      return;
     }
  /*
   * Start running through all objects in this zone.
   */
  for (counter = zone_table[zone_num].number * 100;
       counter <= zone_table[zone_num].top; 
       counter++) 
      {if ((realcounter = real_object(counter)) >= 0) 
          {if ((obj = (obj_proto + realcounter))->action_description) 
              {strcpy(buf1, obj->action_description);
	           strip_string(buf1);
              } 
           else
	          *buf1 = '\0';
	       *buf2 = '\0';
           tascii(&GET_OBJ_AFF(obj,0),4,buf2);
           tascii(&obj->obj_flags.anti_flag.flags[0],4,buf2);
           tascii(&obj->obj_flags.no_flag.flags[0],4,buf2);
           sprintf(buf2+strlen(buf2),"\n%d ",GET_OBJ_TYPE(obj));
           tascii(&GET_OBJ_EXTRA(obj,0),4,buf2);
           tascii(&GET_OBJ_WEAR(obj),1, buf2);
           strcat(buf2,"\n");
           
           fprintf(fp,"#%d\n"
	              "%s~\n"
	              "%s~\n"
	              "%s~\n"
	              "%s~\n"
	              "%s~\n"
	              "%s~\n"
	              "%s~\n"
	              "%s~\n"
	              "%s~\n"
	              "%d %d %d %d\n"
	              "%d %d %d %d\n"
	              "%s"
	              "%d %d %d %d\n"
	              "%d %d %d %d\n",

	              GET_OBJ_VNUM(obj),
	              (obj->name && *obj->name) ? obj->name : "undefined",
	              obj->PNames[0] ? obj->PNames[0] : "что-то",
	              obj->PNames[1] ? obj->PNames[1] : "чего-то",
	              obj->PNames[2] ? obj->PNames[2] : "чему-то",
	              obj->PNames[3] ? obj->PNames[3] : "что-то",
	              obj->PNames[4] ? obj->PNames[4] : "чем-то",
	              obj->PNames[5] ? obj->PNames[5] : "о чем-то",
	              (obj->description && *obj->description) ?
			                           obj->description : "undefined",
	              buf1, 
	              GET_OBJ_SKILL(obj),GET_OBJ_MAX(obj),GET_OBJ_CUR(obj),GET_OBJ_MATER(obj),
	              GET_OBJ_SEX(obj),GET_OBJ_TIMER(obj),obj->obj_flags.Obj_spell,obj->obj_flags.Obj_level,
	              buf2,
	              GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3), 
	              GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_RENTEQ(obj)
	             );

           script_save_to_disk(fp, obj, OBJ_TRIGGER);

           /*
            * Do we have extra descriptions? 
            */
           if (obj->ex_description) 
              {/* Yes, save them too. */
	           for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) 
	               {/*
	                 * Sanity check to prevent nasty protection faults.
	                 */
	                if (!*ex_desc->keyword || !*ex_desc->description) 
	                   {mudlog("SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!", BRF, LVL_BUILDER, TRUE);
	                    continue;
	                   }
 	                strcpy(buf1, ex_desc->description);
	                strip_string(buf1);
	                fprintf(fp, "E\n"
		                    "%s~\n"
		                    "%s~\n", ex_desc->keyword, buf1);
	               }
              }
           /*
            * Do we have affects? 
            */
           for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
	           if (obj->affected[counter2].modifier)
	              fprintf(fp, "A\n"
		                  "%d %d\n", obj->affected[counter2].location,
		                             obj->affected[counter2].modifier);
          }
      }

  /*
   * Write the final line, close the file.
   */
  fprintf(fp, "$\n$\n");
  fclose(fp);
  sprintf(buf2, "%s/%d.obj", OBJ_PREFIX, zone_table[zone_num].number);
  /*
   * We're fubar'd if we crash between the two lines below.
   */
  remove(buf2);
  rename(buf, buf2);

  olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_OBJ);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * For container flags.
 */
void oedit_disp_container_flags_menu(struct descriptor_data *d)
{
  get_char_cols(d->character);
  sprintbit(GET_OBJ_VAL(OLC_OBJ(d), 1), container_bits, buf1);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  sprintf(buf,
	  "%s1%s) Закрываем\r\n"
	  "%s2%s) Запираем\r\n"
	  "%s3%s) Закрыт\r\n"
	  "%s4%s) Заперт\r\n"
	  "Флаги контейнера: %s%s%s\r\n"
	  "Выберите флаг, 0 - выход : ",
	  grn, nrm, grn, nrm, grn, nrm, grn, nrm, cyn, buf1, nrm);
  send_to_char(buf, d->character);
}

/*
 * For extra descriptions.
 */
void oedit_disp_extradesc_menu(struct descriptor_data *d)
{
  struct extra_descr_data *extra_desc = OLC_DESC(d);

  strcpy(buf1, !extra_desc->next ? "<Not set>\r\n" : "Set.");

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  sprintf(buf,
	  "Меню экстрадескрипторов\r\n"
	  "%s1%s) Ключ: %s%s\r\n"
	  "%s2%s) Описание:\r\n%s%s\r\n"
	  "%s3%s) Следующий дескриптор: %s\r\n"
	  "%s0%s) Выход\r\n"
	  "Ваш выбор : ",

     grn, nrm, yel, (extra_desc->keyword && *extra_desc->keyword) ? extra_desc->keyword : "<NONE>",
	 grn, nrm, yel, (extra_desc->description && *extra_desc->description) ? extra_desc->description : "<NONE>",
	 grn, nrm, buf1, grn, nrm);
  send_to_char(buf, d->character);
  OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}

/*
 * Ask for *which* apply to edit.
 */
void oedit_disp_prompt_apply_menu(struct descriptor_data *d)
{
  int counter;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0; counter < MAX_OBJ_AFFECT; counter++) 
      {if (OLC_OBJ(d)->affected[counter].modifier) 
          {sprinttype(OLC_OBJ(d)->affected[counter].location, apply_types, buf2);
           sprintf(buf, " %s%d%s) %+d to %s\r\n", grn, counter + 1, nrm,
	                    OLC_OBJ(d)->affected[counter].modifier, buf2);
           send_to_char(buf, d->character);
          } 
       else 
          {sprintf(buf, " %s%d%s) Ничего.\r\n", grn, counter + 1, nrm);
           send_to_char(buf, d->character);
          }
      }
  send_to_char("\r\nВыберите изменяемый аффект (0 - выход) : ", d->character);
  OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

/*
 * Ask for liquid type.
 */
void oedit_liquid_type(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0; counter < NUM_LIQ_TYPES; counter++) 
      {sprintf(buf, " %s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
	           drinks[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  sprintf(buf, "\r\n%sВыберите тип жидкости : ", nrm);
  send_to_char(buf, d->character);
  OLC_MODE(d) = OEDIT_VALUE_3;
}

/*
 * The actual apply to set.
 */
void oedit_disp_apply_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0; counter < NUM_APPLIES; counter++) 
      {sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		       apply_types[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  send_to_char("\r\nЧто добавляем (0 - выход) : ", d->character);
  OLC_MODE(d) = OEDIT_APPLY;
}

/*
 * Weapon type.
 */
void oedit_disp_weapon_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0; counter < NUM_ATTACK_TYPES; counter++) 
      {sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		       attack_hit_text[counter].singular,
		       !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  send_to_char("\r\nВыберите тип удара (0 - выход): ", d->character);
}

/*
 * Spell type.
 */
void oedit_disp_spells_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0; counter < NUM_SPELLS; counter++) 
      {sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
		       spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  sprintf(buf, "\r\n%sВыберите магию (0 - выход) : ", nrm);
  send_to_char(buf, d->character);
}

/*
 * Object value #1
 */
void oedit_disp_val1_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_1;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
  {case ITEM_LIGHT:
    /*
     * values 0 and 1 are unused.. jump to 2 
     */
    oedit_disp_val3_menu(d);
    break;
  case ITEM_SCROLL:
  case ITEM_WAND:
  case ITEM_STAFF:
  case ITEM_POTION:
    send_to_char("Уровень заклинания : ", d->character);
    break;
  case ITEM_WEAPON:
    /*
     * This doesn't seem to be used if I remembe right.
     */
    send_to_char("Модификатор попадания : ", d->character);
    break;
  case ITEM_ARMOR:
    send_to_char("Изменяет АС на : ", d->character);
    break;
  case ITEM_CONTAINER:
    send_to_char("Максимально вместимый вес : ", d->character);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    send_to_char("Количество глотков : ", d->character);
    break;
  case ITEM_FOOD:
    send_to_char("На сколько часов насыщает : ", d->character);
    break;
  case ITEM_MONEY:
    send_to_char("Сколько кун содержит : ", d->character);
    break;
  case ITEM_NOTE:
    /*
     * This is supposed to be language, but it's unused.
     */
    break;
  case ITEM_BOOK:
    send_to_char("Введите уровень изучения : ", d->character);
    break;
  case ITEM_INGRADIENT:
    send_to_char("Первый байт - лаг после применения в сек, 5 бит - уровень : ", d->character);
    break;
  default:
    oedit_disp_menu(d);
  }
}

/*
 * Object value #2
 */
void oedit_disp_val2_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_2;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
  {case ITEM_SCROLL:
   case ITEM_POTION:
        oedit_disp_spells_menu(d);
        break;
  case ITEM_WAND:
  case ITEM_STAFF:
       send_to_char("Количество зарядов : ", d->character);
       break;
  case ITEM_WEAPON:
       send_to_char("Количество бросков кубика : ", d->character);
       break;
  case ITEM_ARMOR:
       send_to_char("Изменяет броню на : ",d->character);
       break;
  case ITEM_FOOD:
    /*
     * Values 2 and 3 are unused, jump to 4...Odd.
     */
       oedit_disp_val4_menu(d);
       break;
  case ITEM_CONTAINER:
    /*
     * These are flags, needs a bit of special handling.
     */
    oedit_disp_container_flags_menu(d);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    send_to_char("Начальное количество глотков : ", d->character);
    break;
  case ITEM_BOOK:
    send_to_char("Введите номер заклинания : ",d->character);
    break;
  case ITEM_INGRADIENT:
    send_to_char("Виртуальный номер прототипа  : ",d->character);
    break;
  default:
    oedit_disp_menu(d);
  }
}

/*
 * Object value #3
 */
void oedit_disp_val3_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_3;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_LIGHT:
    send_to_char("Длительность горения (0 = погасла, -1 - вечный свет) : ", d->character);
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    send_to_char("Осталось зарядов : ", d->character);
    break;
  case ITEM_WEAPON:
    send_to_char("Количество граней кубика : ", d->character);
    break;
  case ITEM_CONTAINER:
    send_to_char("Vnum ключа для контейнера (-1 - нет ключа) : ", d->character);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    oedit_liquid_type(d);
    break;
  case ITEM_INGRADIENT:
    send_to_char("Сколько раз можно использовать : ", d->character);
    break;
  default:
    oedit_disp_menu(d);
  }
}

/*
 * Object value #4
 */
void oedit_disp_val4_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_4;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_SCROLL:
  case ITEM_POTION:
  case ITEM_WAND:
  case ITEM_STAFF:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_WEAPON:
    oedit_disp_weapon_menu(d);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
  case ITEM_FOOD:
    send_to_char("Отравлено (0 = не отравлено) : ", d->character);
    break;
  default:
    oedit_disp_menu(d);
  }
}

/*
 * Object type.
 */
void oedit_disp_type_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0; counter < NUM_ITEM_TYPES; counter++) 
      {sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		       item_types[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  send_to_char("\r\nВыберите тип предмета : ", d->character);
}

/*
 * Object extra flags.
 */
void oedit_disp_extra_menu(struct descriptor_data *d)
{
  int  counter, columns = 0, plane = 0;
  char c;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0, c = 'a'-1; plane < NUM_PLANES; counter++) 
      {if (*extra_bits[counter] == '\n')
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
		       extra_bits[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  sprintbits(OLC_OBJ(d)->obj_flags.extra_flags, extra_bits, buf1, ",");
  sprintf(buf, "\r\nЭкстрафлаги: %s%s%s\r\n"
	           "Выберите экстрафлаг (0 - выход) : ",
	           cyn, buf1, nrm);
  send_to_char(buf, d->character);
}

void oedit_disp_anti_menu(struct descriptor_data *d)
{
  int  counter, columns = 0, plane = 0;
  char c;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0, c = 'a'-1; plane < NUM_PLANES; counter++) 
      {if (*anti_bits[counter] == '\n')
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
		       anti_bits[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  sprintbits(OLC_OBJ(d)->obj_flags.anti_flag, anti_bits, buf1, ",");
  sprintf(buf, "\r\nПредмет запрещен для : %s%s%s\r\n"
	           "Выберите флаг запрета (0 - выход) : ",
	           cyn, buf1, nrm);
  send_to_char(buf, d->character);
}

void oedit_disp_no_menu(struct descriptor_data *d)
{
  int  counter, columns = 0, plane = 0;
  char c;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0, c = 'a'-1; plane < NUM_PLANES; counter++) 
      {if (*no_bits[counter] == '\n')
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
		       no_bits[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  sprintbits(OLC_OBJ(d)->obj_flags.no_flag, no_bits, buf1, ",");
  sprintf(buf, "\r\nПредмет неудобен для : %s%s%s\r\n"
	           "Выберите флаг неудобств (0 - выход) : ",
	           cyn, buf1, nrm);
  send_to_char(buf, d->character);
}

void oedit_disp_affects_menu(struct descriptor_data *d)
{
  int  counter, columns = 0, plane = 0;
  char c;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0, c = 'a'-1; plane < NUM_PLANES; counter++) 
      {if (*weapon_affects[counter] == '\n')
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
		       weapon_affects[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  sprintbits(OLC_OBJ(d)->obj_flags.affects, weapon_affects, buf1, ",");
  sprintf(buf, "\r\nНакладываемые аффекты : %s%s%s\r\n"
	           "Выберите аффект (0 - выход) : ",
	           cyn, buf1, nrm);
  send_to_char(buf, d->character);
}

/*
 * Object wear flags.
 */
void oedit_disp_wear_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0; counter < NUM_ITEM_WEARS; counter++) 
      {sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
		       wear_bits[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  sprintbit(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, buf1);
  sprintf(buf, "\r\nМожет быть одет : %s%s%s\r\n"
	           "Выберите позицию (0 - выход) : ", cyn, buf1, nrm);
  send_to_char(buf, d->character);
}

void oedit_disp_mater_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0; counter < 32 && *material_name[counter] != '\n'; counter++) 
      {sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
		       material_name[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  sprintf(buf, "\r\nСделан из : %s%s%s\r\n"
	           "Выберите материал (0 - выход) : ", cyn, material_name[GET_OBJ_MATER(OLC_OBJ(d))], nrm);
  send_to_char(buf, d->character);
}

void oedit_disp_ingradient_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0; counter < 32 && *ingradient_bits[counter] != '\n'; counter++) 
      {sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
		       ingradient_bits[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  sprintbit(GET_OBJ_SKILL(OLC_OBJ(d)), ingradient_bits, buf1);
  sprintf(buf, "\r\nТип инградиента : %s%s%s\r\n"
	           "Дополните тип (0 - выход) : ", cyn, buf1, nrm);
  send_to_char(buf, d->character);
}

const char *wskill_bits[] =
{ "палицы и дубины(141)",
  "секиры(142)",
  "длинные лезвия(143)",
  "короткие лезвия(144)",
  "иное(145)", 
  "двуручники(146)",
  "проникающее(147)",
  "копья и рогатины(148)",
  "луки(154)",
  "\n"
};


void oedit_disp_skills_menu(struct descriptor_data *d)
{
  int counter, columns = 0;
  
  if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_INGRADIENT)
     {oedit_disp_ingradient_menu(d);
      return;
     }
  
  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0; counter < 32 && *wskill_bits[counter] != '\n'; counter++) 
      {sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
		       wskill_bits[counter], !(++columns % 2) ? "\r\n" : "");
       send_to_char(buf, d->character);
      }
  sprintf(buf, "Тренируемое умение : %s%d%s"
               "Выберите умение (0 - выход) : ", cyn, GET_OBJ_SKILL(OLC_OBJ(d)), nrm);
  send_to_char(buf, d->character);
}


/*
 * Display main menu.
 */
void oedit_disp_menu(struct descriptor_data *d)
{
  struct obj_data *obj;

  obj = OLC_OBJ(d);
  get_char_cols(d->character);

  /*
   * Build buffers for first part of menu.
   */
  sprinttype(GET_OBJ_TYPE(obj), item_types, buf1);
  sprintbits(obj->obj_flags.extra_flags, extra_bits, buf2, ",");

  /*
   * Build first half of menu.
   */
  sprintf(buf,
#if defined(CLEAR_SCREEN)
	  "[H[J"
#endif
	  "-- Предмет : [%s%d%s]\r\n"
	  "%s1%s) Синонимы : %s%s\r\n"
	  "%s2%s) Именительный (это ЧТО)             : %s%s\r\n"
	  "%s3%s) Родительный  (нету ЧЕГО)           : %s%s\r\n"
	  "%s4%s) Дательный    (прикрепить к ЧЕМУ)   : %s%s\r\n"
	  "%s5%s) Винительный  (держать ЧТО)         : %s%s\r\n"
	  "%s6%s) Творительный (вооружиться ЧЕМ)     : %s%s\r\n"
	  "%s7%s) Предложный   (писать на ЧЕМ)       : %s%s\r\n"
	  "%s8%s) Описание          :-\r\n%s%s\r\n"
	  "%s9%s) Опис.при действии :-\r\n%s%s\r\n"
	  "%sA%s) Тип предмета      :-\r\n%s%s\r\n"
	  "%sB%s) Экстрафлаги       :-\r\n%s%s\r\n",

	  cyn, OLC_NUM(d), nrm,
	  grn, nrm, yel, (obj->name && *obj->name) ? obj->name : "undefined",
	  grn, nrm, yel, (obj->PNames[0] && *obj->PNames[0]) ? obj->PNames[0] : "undefined",
	  grn, nrm, yel, (obj->PNames[1] && *obj->PNames[1]) ? obj->PNames[1] : "undefined",	  
	  grn, nrm, yel, (obj->PNames[2] && *obj->PNames[2]) ? obj->PNames[2] : "undefined",
	  grn, nrm, yel, (obj->PNames[3] && *obj->PNames[3]) ? obj->PNames[3] : "undefined",	  
	  grn, nrm, yel, (obj->PNames[4] && *obj->PNames[4]) ? obj->PNames[4] : "undefined",
	  grn, nrm, yel, (obj->PNames[5] && *obj->PNames[5]) ? obj->PNames[5] : "undefined",	  
	  grn, nrm, yel, (obj->description && *obj->description) ? obj->description : "undefined",
	  grn, nrm, yel, (obj->action_description && *obj->action_description) ? obj->action_description : "<not set>\r\n",
	  grn, nrm, cyn, buf1,
	  grn, nrm, cyn, buf2
	  );
  /*
   * Send first half.
   */
  send_to_char(buf, d->character);

  sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf1);
  sprintbits(obj->obj_flags.no_flag, no_bits, buf2, ",");
  sprintf(buf,
	  "%sC%s) Wear flags  : %s%s\r\n"
	  "%sD%s) No flags    : %s%s\r\n",
	  grn,nrm,cyn,buf1,
	  grn,nrm,cyn,buf2);
  send_to_char(buf, d->character);
	  
  sprintbits(obj->obj_flags.anti_flag, anti_bits, buf1, ",");
  sprintbits(obj->obj_flags.affects, weapon_affects, buf2, ",");
  sprintf(buf,
	      "%sE%s) Anti flags  : %s%s\r\n"
	      "%sF%s) Weight      : %s%8d   %sG%s) Cost        : %s%d\r\n"
	      "%sH%s) Cost/Day    : %s%8d   %sI%s) Cost/Day(eq): %s%d\r\n"
	      "%sJ%s) Max value   : %s%8d   %sK%s) Cur value   : %s%d\r\n"
	      "%sL%s) Material    : %s%s\r\n"
	      "%sM%s) Timer       : %s%8d   %sN%s) Skill       : %s%d\r\n"
	      "%sO%s) Values      : %s%d %d %d %d\r\n"
	      "%sP%s) Affects     : %s%s\r\n"
	      "%sR%s) Applies menu\r\n"
	      "%sT%s) Extra descriptions menu\r\n"
              "%sS%s) Script      : %s%s\r\n"
	      "%sU%s) Sex         : %s%d\r\n"
	      "%sQ%s) Quit\r\n"
	      "Ваш выбор : ",

	      grn, nrm, cyn, buf1,
	      grn, nrm, cyn, GET_OBJ_WEIGHT(obj),
	      grn, nrm, cyn, GET_OBJ_COST(obj),
	      grn, nrm, cyn, GET_OBJ_RENT(obj),
	      grn, nrm, cyn, GET_OBJ_RENTEQ(obj),
	      grn, nrm, cyn, GET_OBJ_MAX(obj),
	      grn, nrm, cyn, GET_OBJ_CUR(obj),
	      grn, nrm, cyn, material_name[GET_OBJ_MATER(obj)],
	      grn, nrm, cyn, GET_OBJ_TIMER(obj),
	      grn, nrm, cyn, GET_OBJ_SKILL(obj),
	      grn, nrm, cyn, 
	      GET_OBJ_VAL(obj, 0),GET_OBJ_VAL(obj, 1),GET_OBJ_VAL(obj, 2),GET_OBJ_VAL(obj, 3),  
	      grn, nrm, grn, buf2, 
	      grn, nrm, 
	      grn, nrm,
              grn, nrm, cyn, obj->proto_script?"Set.":"Not Set.",
	      grn, nrm, cyn, GET_OBJ_SEX(obj),
              grn, nrm);
  send_to_char(buf, d->character);
  OLC_MODE(d) = OEDIT_MAIN_MENU;
}

/***************************************************************************
 main loop (of sorts).. basically interpreter throws all input to here
 ***************************************************************************/
int planebit(char *str, int *plane, int *bit)
{if (!str || !*str)
    return (-1);
 if (*str == '0')
    return (0);
 if (*str >= 'a' && *str <= 'z')
    *bit = (*(str) - 'a');
 else
 if (*str >= 'A' && *str <= 'D')
    *bit = (*(str) - 'A' + 26);
 else
    return (-1);
    
 if (*(str+1) >= '0' && *(str+1) <= '3')
    *plane = (*(str+1) - '0');
 else
    return (-1);
 return (1);
}

void oedit_parse(struct descriptor_data *d, char *arg)
{
  int number, max_val, min_val, plane, bit;

  switch (OLC_MODE(d)) 
  {case OEDIT_CONFIRM_SAVESTRING:
    switch (*arg) 
    {case 'y': case 'Y': case 'д' : case 'Д' :
      send_to_char("Saving object to memory.\r\n", d->character);
      oedit_save_internally(d);
      sprintf(buf, "OLC: %s edits obj %d", GET_NAME(d->character), OLC_NUM(d));
      mudlog(buf, CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE);
      cleanup_olc(d, CLEANUP_STRUCTS);
      return;
     case 'n': case 'N': case 'н' : case 'Н' :
      /*
       * Cleanup all.
       */
      cleanup_olc(d, CLEANUP_ALL);
      return;
     default:
      send_to_char("Неверный выбор !\r\n", d->character);
      send_to_char("Вы хотите сохранить этот предмет ?\r\n", d->character);
      return;
    }

  case OEDIT_MAIN_MENU:
    /*
     * Throw us out to whichever edit mode based on user input.
     */
    switch (*arg) 
    {case 'q': case 'Q': case 'x' : case 'X' :
      if (OLC_VAL(d)) 
         {/* Something has been modified. */
	      send_to_char("Вы хотите сохранить этот предмет ? : ", d->character);
          OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
         } 
      else
	     cleanup_olc(d, CLEANUP_ALL);
      return;
     case '1':
      send_to_char("Введите синонимы : ", d->character);
      OLC_MODE(d) = OEDIT_EDIT_NAMELIST;
      break;
     case '2':
      send_to_char("Именительный падеж [это ЧТО] : ", d->character);
      OLC_MODE(d) = OEDIT_PAD0;
      break;
     case '3':
      send_to_char("Родительный падеж [нет ЧЕГО] : ", d->character);
      OLC_MODE(d) = OEDIT_PAD1;
      break;
     case '4':
      send_to_char("Дательный падеж [прикрепить к ЧЕМУ] : ", d->character);
      OLC_MODE(d) = OEDIT_PAD2;
      break;
     case '5':
      send_to_char("Винительный падеж [держать ЧТО] : ", d->character);
      OLC_MODE(d) = OEDIT_PAD3;
      break;
     case '6':
      send_to_char("Творительный падеж [вооружиться ЧЕМ] : ", d->character);
      OLC_MODE(d) = OEDIT_PAD4;
      break;
     case '7':
      send_to_char("Предложный падеж [писать на ЧЕМ] : ", d->character);
      OLC_MODE(d) = OEDIT_PAD5;
      break;
    case '8':
      send_to_char("Введите длинное описание :-\r\n| ", d->character);
      OLC_MODE(d) = OEDIT_LONGDESC;
      break;
    case '9':
      OLC_MODE(d) = OEDIT_ACTDESC;
      SEND_TO_Q("Введите описание при применении: (/s сохранить /h помощь)\r\n\r\n", d);
      d->backstr = NULL;
      if (OLC_OBJ(d)->action_description) 
         {SEND_TO_Q(OLC_OBJ(d)->action_description, d);
	      d->backstr = str_dup(OLC_OBJ(d)->action_description);
         }
      d->str = &OLC_OBJ(d)->action_description;
      d->max_str = MAX_MESSAGE_LENGTH;
      d->mail_to = 0;
      OLC_VAL(d) = 1;
      break;
    case 'a': case 'A' :
      oedit_disp_type_menu(d);
      OLC_MODE(d) = OEDIT_TYPE;
      break;
    case 'b': case 'B' :
      oedit_disp_extra_menu(d);
      OLC_MODE(d) = OEDIT_EXTRAS;
      break;
    case 'c': case 'C' :
      oedit_disp_wear_menu(d);
      OLC_MODE(d) = OEDIT_WEAR;
      break;
    case 'd': case 'D' :
      oedit_disp_no_menu(d);
      OLC_MODE(d) = OEDIT_NO;
      break;
    case 'e': case 'E' :
      oedit_disp_anti_menu(d);
      OLC_MODE(d) = OEDIT_ANTI;
      break;
    case 'f': case 'F' :
      send_to_char("Вес предмета : ", d->character);
      OLC_MODE(d) = OEDIT_WEIGHT;
      break;
    case 'g': case 'G':
      send_to_char("Цена предмета : ", d->character);
      OLC_MODE(d) = OEDIT_COST;
      break;
    case 'h': case 'H':
      send_to_char("Рента предмета (в инвентаре) : ", d->character);
      OLC_MODE(d) = OEDIT_COSTPERDAY;
      break;
    case 'i': case 'I':
      send_to_char("Рента предмета (в экипировке) : ", d->character);
      OLC_MODE(d) = OEDIT_COSTPERDAYEQ;
      break;
    case 'j': case 'J':
      send_to_char("Максимальная прочность : ", d->character);
      OLC_MODE(d) = OEDIT_MAXVALUE;
      break;
    case 'k': case 'K':
      send_to_char("Текущая прочность : ", d->character);
      OLC_MODE(d) = OEDIT_CURVALUE;
      break;
    case 'l': case 'L' :
      oedit_disp_mater_menu(d);
      OLC_MODE(d) = OEDIT_MATER;
      break;
    case 'm': case 'M':
      send_to_char("Таймер (в тиках) : ", d->character);
      OLC_MODE(d) = OEDIT_TIMER;
      break;
    case 'n': case 'N' :
      if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_WEAPON ||
          GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_INGRADIENT)
         {oedit_disp_skills_menu(d);
          OLC_MODE(d) = OEDIT_SKILL;
         }
      break;
    case 'o': case 'O':
      /*
       * Clear any old values  
       */
      GET_OBJ_VAL(OLC_OBJ(d), 0) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 1) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 2) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 3) = 0;
      oedit_disp_val1_menu(d);
      break;
    case 'p': case 'P' :
      oedit_disp_affects_menu(d);
      OLC_MODE(d) = OEDIT_AFFECTS;
      break;
    case 'r': case 'R':
      oedit_disp_prompt_apply_menu(d);
      break;
    case 't': case 'T':
      /*
       * If extra descriptions don't exist.
       */
      if (!OLC_OBJ(d)->ex_description) 
         {CREATE(OLC_OBJ(d)->ex_description, struct extra_descr_data, 1);
	      OLC_OBJ(d)->ex_description->next = NULL;
         }
      OLC_DESC(d) = OLC_OBJ(d)->ex_description;
      oedit_disp_extradesc_menu(d);
      break;
    case 's': case 'S':
      OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
      dg_script_menu(d);
      return;
    case 'u': case 'U':
      send_to_char("Пол : ", d->character);
      OLC_MODE(d) = OEDIT_SEXVALUE;
      break;
      
    default:
      oedit_disp_menu(d);
      break;
    }
    return;			
    /*
	 * end of OEDIT_MAIN_MENU 
	 */

  case OLC_SCRIPT_EDIT:
    if (dg_script_edit_parse(d, arg)) return;
    break;

  case OEDIT_EDIT_NAMELIST:
    if (OLC_OBJ(d)->name)
       free(OLC_OBJ(d)->name);
    OLC_OBJ(d)->name = str_dup((arg && *arg) ? arg : "undefined");
    break;

  case OEDIT_PAD0:
    if (OLC_OBJ(d)->PNames[0])
       free(OLC_OBJ(d)->PNames[0]);
    if (OLC_OBJ(d)->short_description)
       free(OLC_OBJ(d)->short_description);
    OLC_OBJ(d)->short_description = str_dup((arg && *arg) ? arg : "что-то");
    OLC_OBJ(d)->PNames[0]         = str_dup((arg && *arg) ? arg : "что-то");
    break;

  case OEDIT_PAD1:
    if (OLC_OBJ(d)->PNames[1])
       free(OLC_OBJ(d)->PNames[1]);
    OLC_OBJ(d)->PNames[1]         = str_dup((arg && *arg) ? arg : "-чего-то");
    break;

  case OEDIT_PAD2:
    if (OLC_OBJ(d)->PNames[2])
       free(OLC_OBJ(d)->PNames[2]);
    OLC_OBJ(d)->PNames[2]         = str_dup((arg && *arg) ? arg : "-чему-то");
    break;

  case OEDIT_PAD3:
    if (OLC_OBJ(d)->PNames[3])
       free(OLC_OBJ(d)->PNames[3]);
    OLC_OBJ(d)->PNames[3]         = str_dup((arg && *arg) ? arg : "-что-то");
    break;

  case OEDIT_PAD4:
    if (OLC_OBJ(d)->PNames[4])
       free(OLC_OBJ(d)->PNames[4]);
    OLC_OBJ(d)->PNames[4]         = str_dup((arg && *arg) ? arg : "-чем-то");
    break;

  case OEDIT_PAD5:
    if (OLC_OBJ(d)->PNames[5])
       free(OLC_OBJ(d)->PNames[5]);
    OLC_OBJ(d)->PNames[5]         = str_dup((arg && *arg) ? arg : "-чем-то");
    break;

  case OEDIT_LONGDESC:
    if (OLC_OBJ(d)->description)
       free(OLC_OBJ(d)->description);
    OLC_OBJ(d)->description = str_dup((arg && *arg) ? arg : "неопределено");
    break;

  case OEDIT_TYPE:
    number = atoi(arg);
    if ((number < 1) || (number >= NUM_ITEM_TYPES)) 
       {send_to_char("Invalid choice, try again : ", d->character);
        return;
       } 
    else
       GET_OBJ_TYPE(OLC_OBJ(d)) = number;
    break;

  case OEDIT_EXTRAS:
    number = planebit(arg,&plane,&bit);
    if (number < 0) 
       {oedit_disp_extra_menu(d);
        return;
       } 
    else 
    if (number == 0)
       break;
    else 
       {TOGGLE_BIT(OLC_OBJ(d)->obj_flags.extra_flags.flags[plane], 1 << (bit));
        oedit_disp_extra_menu(d);
        return;
       }

  case OEDIT_WEAR:
    number = atoi(arg);
    if ((number < 0) || (number > NUM_ITEM_WEARS)) 
       { send_to_char("Неверный выбор !\r\n", d->character);
         oedit_disp_wear_menu(d);
         return;
       } 
    else 
    if (number == 0)	/* Quit. */
       break;
    else 
       {TOGGLE_BIT(GET_OBJ_WEAR(OLC_OBJ(d)), 1 << (number - 1));
        oedit_disp_wear_menu(d);
        return;
       }
    
  case OEDIT_NO:
    number = planebit(arg,&plane,&bit);
    if (number < 0) 
       {oedit_disp_no_menu(d);
        return;
       } 
    else 
    if (number == 0)
       break;
    else 
       {TOGGLE_BIT(OLC_OBJ(d)->obj_flags.no_flag.flags[plane], 1 << (bit));
        oedit_disp_no_menu(d);
        return;
       }

  case OEDIT_ANTI:
    number = planebit(arg,&plane,&bit);
    if (number < 0) 
       {oedit_disp_anti_menu(d);
        return;
       } 
    else 
    if (number == 0)
       break;
    else 
       {TOGGLE_BIT(OLC_OBJ(d)->obj_flags.anti_flag.flags[plane], 1 << (bit));
        oedit_disp_anti_menu(d);
        return;
       }


  case OEDIT_WEIGHT:
    GET_OBJ_WEIGHT(OLC_OBJ(d)) = atoi(arg);
    break;

  case OEDIT_COST:
    GET_OBJ_COST(OLC_OBJ(d)) = atoi(arg);
    break;

  case OEDIT_COSTPERDAY:
    GET_OBJ_RENT(OLC_OBJ(d)) = atoi(arg);
    break;

  case OEDIT_MAXVALUE:
    GET_OBJ_MAX(OLC_OBJ(d)) = atoi(arg);
    break;

  case OEDIT_CURVALUE:
    GET_OBJ_CUR(OLC_OBJ(d)) = atoi(arg);
    break;

  case OEDIT_SEXVALUE:
    if ((number = atoi(arg)) >= 0 && number < 4)
       GET_OBJ_SEX(OLC_OBJ(d)) = number;
    else
       {send_to_char("Пол (0-3) : ",d->character);
        return;
       }
    break;
    
  case OEDIT_MATER:
    number = atoi(arg);
    if (number < 0 || number > NUM_MATERIALS)
       {oedit_disp_mater_menu(d);
        return;
       }
    else
    if (number > 0)
       GET_OBJ_MATER(OLC_OBJ(d)) = number - 1;
    break;

  case OEDIT_COSTPERDAYEQ:
    GET_OBJ_RENTEQ(OLC_OBJ(d)) = atoi(arg);
    break;

  case OEDIT_TIMER:
    GET_OBJ_TIMER(OLC_OBJ(d)) = atoi(arg);
    break;
    
  case OEDIT_SKILL:
    number = atoi(arg);
    if (number < 0)
       {oedit_disp_skills_menu(d);
        return;
       }
    else
    if (number == 0)
       break;
    else
    if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_INGRADIENT)
       {TOGGLE_BIT(GET_OBJ_SKILL(OLC_OBJ(d)), 1 << (number-1));
        oedit_disp_skills_menu(d);
        return;
       }
    else
    if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_WEAPON)
       {switch (number)
        {case 1: number = 141; break;
         case 2: number = 142; break;
         case 3: number = 143; break;
         case 4: number = 144; break;
         case 5: number = 145; break;
         case 6: number = 146; break;
         case 7: number = 147; break;
         case 8: number = 148; break;
         case 9: number = 154; break;
         default: oedit_disp_skills_menu(d);
                  return;
        }
        GET_OBJ_SKILL(OLC_OBJ(d)) = number;
        oedit_disp_skills_menu(d);
        return;
       }
    break;
    
  case OEDIT_VALUE_1:
    /*
     * Lucky, I don't need to check any of these for out of range values.
     * Hmm, I'm not so sure - Rv  
     */
    GET_OBJ_VAL(OLC_OBJ(d), 0) = atoi(arg);
    /*
     * proceed to menu 2 
     */
    oedit_disp_val2_menu(d);
    return;
  case OEDIT_VALUE_2:
    /*
     * Here, I do need to check for out of range values.
     */
    number = atoi(arg);
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
    {case ITEM_SCROLL:
     case ITEM_POTION:
          if (number < 0 || number >= NUM_SPELLS)
	         oedit_disp_val2_menu(d);
          else 
             {GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
	          oedit_disp_val3_menu(d);
             }
          break;
     case ITEM_CONTAINER:
          /*
           * Needs some special handling since we are dealing with flag values
           * here.
           */
           if (number < 0 || number > 4)
	          oedit_disp_container_flags_menu(d);
           else 
           if (number != 0) 
              {TOGGLE_BIT(GET_OBJ_VAL(OLC_OBJ(d), 1), 1 << (number - 1));
               OLC_VAL(d) = 1;
	           oedit_disp_val2_menu(d);
              } 
           else
	          oedit_disp_val3_menu(d);
           break;

     default:
           GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
           oedit_disp_val3_menu(d);
    }
    return;

  case OEDIT_VALUE_3:
    number = atoi(arg);
    /*
     * Quick'n'easy error checking.
     */
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
    {case ITEM_SCROLL:
     case ITEM_POTION:
          min_val = 0;
          max_val = NUM_SPELLS - 1;
          break;
     case ITEM_WEAPON:
         min_val = 1;
         max_val = 50;
     case ITEM_WAND:
     case ITEM_STAFF:
         min_val = 0;
         max_val = 20;
         break;
     case ITEM_DRINKCON:
     case ITEM_FOUNTAIN:
         min_val = 0;
         max_val = NUM_LIQ_TYPES - 1;
         break;
     default:
         min_val = -32000;
         max_val = 32000;
    }
    GET_OBJ_VAL(OLC_OBJ(d), 2) = MAX(min_val, MIN(number, max_val));
    oedit_disp_val4_menu(d);
    return;

  case OEDIT_VALUE_4:
    number = atoi(arg);
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
    {case ITEM_SCROLL:
     case ITEM_POTION:
      min_val = 0;
      max_val = NUM_SPELLS - 1;
      break;
     case ITEM_WAND:
     case ITEM_STAFF:
      min_val = 1;
      max_val = NUM_SPELLS - 1;
      break;
     case ITEM_WEAPON:
      min_val = 0;
      max_val = NUM_ATTACK_TYPES - 1;
      break;
     default:
      min_val = -32000;
      max_val = 32000;
      break;
    }
    GET_OBJ_VAL(OLC_OBJ(d), 3) = MAX(min_val, MIN(number, max_val));
    break;
    
  case OEDIT_AFFECTS:
    number = planebit(arg,&plane,&bit);
    if (number < 0) 
       {oedit_disp_affects_menu(d);
        return;
       } 
    else 
    if (number == 0)
       break;
    else 
       {TOGGLE_BIT(OLC_OBJ(d)->obj_flags.affects.flags[plane], 1 << (bit));
        oedit_disp_affects_menu(d);
        return;
       }

  case OEDIT_PROMPT_APPLY:
    if ((number = atoi(arg)) == 0)
      break;
    else 
    if (number < 0 || number > MAX_OBJ_AFFECT) 
       {oedit_disp_prompt_apply_menu(d);
        return;
       }
    OLC_VAL(d) = number - 1;
    OLC_MODE(d) = OEDIT_APPLY;
    oedit_disp_apply_menu(d);
    return;

  case OEDIT_APPLY:
    if ((number = atoi(arg)) == 0) 
       {OLC_OBJ(d)->affected[OLC_VAL(d)].location = 0;
        OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = 0;
        oedit_disp_prompt_apply_menu(d);
       } 
    else 
    if (number < 0 || number >= NUM_APPLIES)
       oedit_disp_apply_menu(d);
    else 
       {OLC_OBJ(d)->affected[OLC_VAL(d)].location = number;
        send_to_char("Modifier : ", d->character);
        OLC_MODE(d) = OEDIT_APPLYMOD;
       }
    return;

  case OEDIT_APPLYMOD:
    OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = atoi(arg);
    oedit_disp_prompt_apply_menu(d);
    return;

  case OEDIT_EXTRADESC_KEY:
    if (OLC_DESC(d)->keyword)
       free(OLC_DESC(d)->keyword);
    OLC_DESC(d)->keyword = str_dup((arg && *arg) ? arg : "undefined");
    oedit_disp_extradesc_menu(d);
    return;

  case OEDIT_EXTRADESC_MENU:
    switch ((number = atoi(arg))) 
    {case 0:
          if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) 
             {struct extra_descr_data **tmp_desc;

	          if (OLC_DESC(d)->keyword)
	             free(OLC_DESC(d)->keyword);
	          if (OLC_DESC(d)->description)
	             free(OLC_DESC(d)->description);

	           /*
	            * Clean up pointers  
	            */
	          for (tmp_desc = &(OLC_OBJ(d)->ex_description); *tmp_desc;
	               tmp_desc = &((*tmp_desc)->next)) 
	              {if (*tmp_desc == OLC_DESC(d)) 
	                  {*tmp_desc = NULL;
	                   break;
	                  }
	              }
	          free(OLC_DESC(d));
             }
          break;

     case 1:
          OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
          send_to_char("Enter keywords, separated by spaces :-\r\n| ", d->character);
          return;

     case 2:
          OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION;
          SEND_TO_Q("Enter the extra description: (/s saves /h for help)\r\n\r\n", d);
          d->backstr = NULL;
          if (OLC_DESC(d)->description) 
             {SEND_TO_Q(OLC_DESC(d)->description, d);
	          d->backstr = str_dup(OLC_DESC(d)->description);
             }
          d->str = &OLC_DESC(d)->description;
          d->max_str = MAX_MESSAGE_LENGTH;
          d->mail_to = 0;
          OLC_VAL(d) = 1;
          return;

     case 3:
         /*
          * Only go to the next description if this one is finished.
          */
         if (OLC_DESC(d)->keyword && OLC_DESC(d)->description) 
            {struct extra_descr_data *new_extra;

	         if (OLC_DESC(d)->next)
	            OLC_DESC(d) = OLC_DESC(d)->next;
	         else 
	            {/* Make new extra description and attach at end. */
	             CREATE(new_extra, struct extra_descr_data, 1);
	             OLC_DESC(d)->next = new_extra;
	             OLC_DESC(d) = OLC_DESC(d)->next;
	            }
            }
         /*
          * No break - drop into default case.
          */
     default:
         oedit_disp_extradesc_menu(d);
         return;
    }
    break;
  default:
    mudlog("SYSERR: OLC: Reached default case in oedit_parse()!", BRF, LVL_BUILDER, TRUE);
    send_to_char("Oops...\r\n", d->character);
    break;
  }

  /*
   * If we get here, we have changed something.  
   */
  OLC_VAL(d) = 1;
  oedit_disp_menu(d);
}
