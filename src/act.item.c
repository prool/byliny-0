/* ************************************************************************
*   File: act.item.c                                    Part of CircleMUD *
*  Usage: object handling routines -- get/drop and container handling     *
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
#include "constants.h"
#include "dg_scripts.h"

/* extern variables */
extern room_rnum donation_room_1;
#if 0
extern room_rnum donation_room_2;  /* uncomment if needed! */
extern room_rnum donation_room_3;  /* uncomment if needed! */
#endif
extern struct obj_data *obj_proto;
extern struct room_data *world;
extern struct char_data *mob_proto;
extern const int material_value[];
extern const char *weapon_affects[];

int  preequip_char(struct char_data *ch, struct obj_data *obj, int where);
void postequip_char(struct char_data *ch, struct obj_data *obj);

/* local functions */
int can_take_obj(struct char_data * ch, struct obj_data * obj);
void get_check_money(struct char_data * ch, struct obj_data * obj);
int perform_get_from_room(struct char_data * ch, struct obj_data * obj);
void get_from_room(struct char_data * ch, char *arg, int amount);
void perform_give_gold(struct char_data * ch, struct char_data * vict, int amount);
void perform_give(struct char_data * ch, struct char_data * vict, struct obj_data * obj);
int perform_drop(struct char_data * ch, struct obj_data * obj, byte mode, const int sname, room_rnum RDR);
void perform_drop_gold(struct char_data * ch, int amount, byte mode, room_rnum RDR);
struct char_data *give_find_vict(struct char_data * ch, char *arg);
void weight_change_object(struct obj_data * obj, int weight);
void perform_put(struct char_data * ch, struct obj_data * obj, struct obj_data * cont);
void name_from_drinkcon(struct obj_data * obj);
void get_from_container(struct char_data * ch, struct obj_data * cont, char *arg, int mode, int amount);
void name_to_drinkcon(struct obj_data * obj, int type);
void wear_message(struct char_data * ch, struct obj_data * obj, int where);
void perform_wear(struct char_data * ch, struct obj_data * obj, int where);
int find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg);
void perform_get_from_container(struct char_data * ch, struct obj_data * obj, struct obj_data * cont, int mode);
void perform_remove(struct char_data * ch, int pos);
int  invalid_anti_class(struct char_data *ch, struct obj_data *obj);
ACMD(do_remove);
ACMD(do_put);
ACMD(do_get);
ACMD(do_drop);
ACMD(do_give);
ACMD(do_drink);
ACMD(do_eat);
ACMD(do_drunkoff);
ACMD(do_pour);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_grab);
ACMD(do_upgrade);

void perform_put(struct char_data * ch, struct obj_data * obj,
		      struct obj_data * cont)
{
  if (!drop_otrigger(obj, ch))
     return;

  if (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, 0))
     act("$O : $o не помещается туда.", FALSE, ch,obj,cont,TO_CHAR);
  else
  if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
     act("Невозможно положить контейнер в контейнер.",FALSE,ch,0,0,TO_CHAR);
  else
  if (IS_OBJ_STAT(obj, ITEM_NODROP))
     act("Неведомая сила помешала положить $o3 в $O3.",FALSE,ch,obj,cont,TO_CHAR);
  else
     {obj_from_char(obj);
      obj_to_obj(obj, cont);

      act("$n положил$g $o3 в $O3.", TRUE, ch, obj, cont, TO_ROOM);

       /* Yes, I realize this is strange until we have auto-equip on rent. -gg */
      if (IS_OBJ_STAT(obj, ITEM_NODROP) && !IS_OBJ_STAT(cont, ITEM_NODROP))
         {SET_BIT(GET_OBJ_EXTRA(cont, ITEM_NODROP), ITEM_NODROP);
          act("Вы почувствовали что-то странное, когда положили $o3 в $O3.", FALSE,
               ch, obj, cont, TO_CHAR);
         }
      else
         act("Вы положили $o3 в $O3.", FALSE, ch, obj, cont, TO_CHAR);
  }
}


/* The following put modes are supported by the code below:

	1) put <object> <container>
	2) put all.<object> <container>
	3) put all <container>

	<container> must be in inventory or on ground.
	all objects to be put into container must be in inventory.
*/

ACMD(do_put)
{
  char     arg1[MAX_INPUT_LENGTH];
  char     arg2[MAX_INPUT_LENGTH];
  char     arg3[MAX_INPUT_LENGTH];
  struct   obj_data *obj, *next_obj, *cont;
  struct   char_data *tmp_char;
  int      obj_dotmode, cont_dotmode, found = 0, howmany = 1, money_mode=FALSE;
  char    *theobj, *thecont, *theplace;
  int      where_bits = FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM;


  argument = two_arguments(argument, arg1, arg2);
  argument = one_argument(argument, arg3);

  if (*arg3 && is_number(arg1))
     {howmany  = atoi(arg1);
      theobj   = arg2;
      thecont  = arg3;
      theplace = argument;
     }
  else
     {theobj   = arg1;
      thecont  = arg2;
      theplace = arg3;
     }

  if (isname(theplace, "земля комната room ground"))
     where_bits = FIND_OBJ_ROOM;
  else
  if (isname(theplace, "инвентарь inventory"))
     where_bits = FIND_OBJ_INV;
  else
  if (isname(theplace, "экипировка equipment"))
     where_bits = FIND_OBJ_EQUIP;


  if (theobj &&
      (!strn_cmp("coin",theobj,4) ||
       !strn_cmp("кун",theobj,3)))
      { money_mode = TRUE;
        if (howmany <= 0)
           {send_to_char("Следует указать чиста конкретную сумму.\r\n",ch);
            return;
           }
        if (GET_GOLD(ch) < howmany)
           {send_to_char("Нет у Вас такой суммы.\r\n",ch);
            return;
           }
        obj_dotmode = FIND_INDIV;
      }
  else
     obj_dotmode  = find_all_dots(theobj);

  cont_dotmode = find_all_dots(thecont);

  if (!*theobj)
     send_to_char("Положить что и куда ?\r\n", ch);
  else
  if (cont_dotmode != FIND_INDIV)
     send_to_char("Вы можете положить вещь только в один контейнер.\r\n", ch);
  else
  if (!*thecont)
     {sprintf(buf, "Куда Вы хотите положить '%s' ?\r\n", theobj);
      send_to_char(buf, ch);
     }
  else
     {generic_find(thecont, where_bits, ch, &tmp_char, &cont);
      if (!cont)
         {sprintf(buf, "Вы не видите здесь '%s'.\r\n", thecont);
          send_to_char(buf, ch);
         }
      else
      if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
         act("В $o3 нельзя ничего положить.",FALSE, ch, cont, 0, TO_CHAR);
      else
      if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
         act("$o3 закрыт$A !", FALSE, ch, cont, 0, TO_CHAR);
      else
         {if (obj_dotmode == FIND_INDIV)
             {/* put <obj> <container> */
              if (money_mode)
                 {obj = create_money(howmany);
                  if (!obj)
                     return;
                  obj_to_char(obj,ch);
                  GET_GOLD(ch) -= howmany;
                  perform_put(ch,obj,cont);
                 }
              else
              if (!(obj = get_obj_in_list_vis(ch, theobj, ch->carrying)))
                 {sprintf(buf, "У Вас нет '%s'.\r\n", theobj);
                  send_to_char(buf, ch);
                 }
              else
              if (obj == cont)
                 send_to_char("Вам будет трудно запихнуть вещь саму в себя.\r\n", ch);
              else
                 {struct obj_data *next_obj;
                  while(obj && howmany--)
                       {next_obj = obj->next_content;
                        perform_put(ch, obj, cont);
                        obj = get_obj_in_list_vis(ch, theobj, next_obj);
                       }
                 }
             }
          else
             {for (obj = ch->carrying; obj; obj = next_obj)
                  {next_obj = obj->next_content;
                   if (obj != cont && CAN_SEE_OBJ(ch, obj) &&
                       (obj_dotmode == FIND_ALL || isname(theobj, obj->name)))
                      {found = 1;
                       perform_put(ch, obj, cont);
                      }
                  }
              if (!found)
                 {if (obj_dotmode == FIND_ALL)
                     send_to_char("Чтобы положить что-то ненужное нужно купить что-то ненужное.\r\n", ch);
                  else
                     {sprintf(buf, "Вы не видите ничего похожего на '%s'.\r\n", theobj);
                      send_to_char(buf, ch);
                     }
                 }
             }
         }
     }
}



int can_take_obj(struct char_data * ch, struct obj_data * obj)
{
  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
     {act("$p: Вы не могете нести столько вещей.", FALSE, ch, obj, 0, TO_CHAR);
      return (0);
     }
  else
  if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch))
     {act("$p: Вы не в состоянии нести еще и $S.", FALSE, ch, obj, 0, TO_CHAR);
      return (0);
     }
  else
  if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE)))
     {act("$p: Вы не можете взять $S.", FALSE, ch, obj, 0, TO_CHAR);
      return (0);
     }
  else
  if (invalid_anti_class(ch,obj))
     {act("$p: Эта вещь не предназначена для Вас !", FALSE, ch, obj, 0, TO_CHAR);
      return (0);
     }
  return (1);
}


void get_check_money(struct char_data * ch, struct obj_data * obj)
{
  int value = GET_OBJ_VAL(obj, 0);

  if (GET_OBJ_TYPE(obj) != ITEM_MONEY || value <= 0)
    return;

  obj_from_char(obj);
  extract_obj(obj);

  GET_GOLD(ch) += value;

  sprintf(buf, "Это составило %d %s.\r\n", value, desc_count(value,WHAT_MONEYu));
  send_to_char(buf, ch);
}


void perform_get_from_container(struct char_data * ch, struct obj_data * obj,
				     struct obj_data * cont, int mode)
{ if ((mode == FIND_OBJ_INV || mode == FIND_OBJ_ROOM || mode == FIND_OBJ_EQUIP) &&
      can_take_obj(ch, obj))
     {if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
          act("$o: Вы не можете нести столько вещей.", FALSE, ch, obj, 0, TO_CHAR);
      else
      if (get_otrigger(obj, ch))
         {obj_from_obj(obj);
          obj_to_char(obj, ch);
          if (obj->carried_by == ch)
             {act("Вы взяли $o3 из $O1.", FALSE, ch, obj, cont, TO_CHAR);
              act("$n взял$g $o3 из $O1.", TRUE, ch, obj, cont, TO_ROOM);
              get_check_money(ch, obj);
             }
         }
     }
}


void get_from_container(struct char_data * ch, struct obj_data * cont,
			     char *arg, int mode, int howmany)
{
  struct obj_data *obj, *next_obj;
  int obj_dotmode, found = 0;

  obj_dotmode = find_all_dots(arg);
  if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
     act("$o закрыт$A.", FALSE, ch, cont, 0, TO_CHAR);
  else
  if (obj_dotmode == FIND_INDIV)
     {if (!(obj = get_obj_in_list_vis(ch, arg, cont->contains)))
         {sprintf(buf, "Вы не видите '%s' в $o5.", arg);
          act(buf, FALSE, ch, cont, 0, TO_CHAR);
         }
      else
         {struct obj_data *obj_next;
          while(obj && howmany--)
               {
                obj_next = obj->next_content;
                perform_get_from_container(ch, obj, cont, mode);
                obj = get_obj_in_list_vis(ch, arg, obj_next);
               }
         }
     }
  else
     {
      if (obj_dotmode == FIND_ALLDOT && !*arg)
         {send_to_char("Взять что \"все\" ?\r\n", ch);
          return;
         }
      for (obj = cont->contains; obj; obj = next_obj)
          {next_obj = obj->next_content;
           if (CAN_SEE_OBJ(ch, obj) &&
	           (obj_dotmode == FIND_ALL || isname(arg, obj->name)))
	           {found = 1;
	            perform_get_from_container(ch, obj, cont, mode);
               }
          }
      if (!found)
         {if (obj_dotmode == FIND_ALL)
	         act("$o пуст$A.", FALSE, ch, cont, 0, TO_CHAR);
          else
             {sprintf(buf, "Вы не видите ничего похожего на '%s' в $o5.", arg);
	          act(buf, FALSE, ch, cont, 0, TO_CHAR);
             }
         }
     }
}


int perform_get_from_room(struct char_data * ch, struct obj_data * obj)
{
  if (can_take_obj(ch, obj) && get_otrigger(obj, ch))
     {obj_from_room(obj);
      obj_to_char(obj, ch);
      if (obj->carried_by == ch)
         {act("Вы взяли $o3.", FALSE, ch, obj, 0, TO_CHAR);
          act("$n взял$g $o3.", TRUE, ch, obj, 0, TO_ROOM);
          get_check_money(ch, obj);
          return (1);
         }
     }
  return (0);
}


void get_from_room(struct char_data * ch, char *arg, int howmany)
{
  struct obj_data *obj, *next_obj;
  int dotmode, found = 0;

  dotmode = find_all_dots(arg);

  if (dotmode == FIND_INDIV)
     {if (!(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents)))
         {sprintf(buf, "Вы не видите здесь '%s'.\r\n", arg);
          send_to_char(buf, ch);
         }
      else
         {struct obj_data *obj_next;
          while (obj && howmany--)
                {obj_next = obj->next_content;
                 perform_get_from_room(ch, obj);
                 obj = get_obj_in_list_vis(ch, arg, obj_next);
                }
         }
     }
  else
     {if (dotmode == FIND_ALLDOT && !*arg)
         {send_to_char("Взять что \"все\" ?\r\n", ch);
          return;
         }
      for (obj = world[ch->in_room].contents; obj; obj = next_obj)
          {next_obj = obj->next_content;
           if (CAN_SEE_OBJ(ch, obj) &&
	           (dotmode == FIND_ALL || isname(arg, obj->name)))
	          {found = 1;
	           perform_get_from_room(ch, obj);
              }
          }
      if (!found)
         {if (dotmode == FIND_ALL)
	         send_to_char("Похоже, здесь ничего нет.\r\n", ch);
          else
             {sprintf(buf, "Вы не нашли здесь '%s'.\r\n", arg);
	          send_to_char(buf, ch);
             }
         }
     }
}

ACMD(do_mark)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  int    cont_dotmode, found = 0, mode;
  struct obj_data *cont;
  struct char_data *tmp_char;

  argument = two_arguments(argument, arg1, arg2);

  if (!*arg1)
     send_to_char("Что Вы хотите маркировать ?\r\n", ch);
  else
  if (!*arg2 || !is_number(arg2))
     send_to_char("Не указан или неверный маркер.\r\n", ch);
  else
     {cont_dotmode = find_all_dots(arg1);
      if (cont_dotmode == FIND_INDIV)
         {mode = generic_find(arg1, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &cont);
          if (!cont)
             {sprintf(buf, "У Вас нет '%s'.\r\n", arg1);
	          send_to_char(buf, ch);
             }
          cont->obj_flags.Obj_owner = atoi(arg2);
          act("Вы пометили $o3.",FALSE,ch,cont,0,TO_CHAR);
         }
      else
         {if (cont_dotmode == FIND_ALLDOT && !*arg1)
             {send_to_char("Пометить что \"все\" ?\r\n", ch);
	          return;
             }
          for (cont = ch->carrying; cont; cont = cont->next_content)
	          if (CAN_SEE_OBJ(ch, cont) &&
	              (cont_dotmode == FIND_ALL || isname(arg1, cont->name)))
	             {cont->obj_flags.Obj_owner = atoi(arg2);
	              act("Вы пометили $o3.",FALSE,ch,cont,0,TO_CHAR);
	             }
          for (cont = world[ch->in_room].contents; cont; cont = cont->next_content)
	          if (CAN_SEE_OBJ(ch, cont) &&
	              (cont_dotmode == FIND_ALL || isname(arg2, cont->name)))
	             {cont->obj_flags.Obj_owner = atoi(arg2);
	              act("Вы пометили $o3.",FALSE,ch,cont,0,TO_CHAR);
	             }
          if (!found)
             {if (cont_dotmode == FIND_ALL)
	             send_to_char("Вы не смогли найти ничего для маркировки.\r\n", ch);
	          else
	             {sprintf(buf, "Вы что-то не видите здесь '%s'.\r\n", arg1);
	              send_to_char(buf, ch);
	             }
             }
         }
     }
}


ACMD(do_get)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  char *theplace;
  int  where_bits = FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP;

  int cont_dotmode, found = 0, mode;
  struct obj_data *cont;
  struct char_data *tmp_char;

  argument = two_arguments(argument, arg1, arg2);
  argument = one_argument(argument, arg3);
  theplace = argument;

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
     send_to_char("У Вас заняты руки !\r\n", ch);
  else
  if (!*arg1)
     send_to_char("Что Вы хотите взять ?\r\n", ch);
  else
  if (!*arg2)
     get_from_room(ch, arg1, 1);
  else
  if (is_number(arg1) && !*arg3)
     get_from_room(ch, arg2, atoi(arg1));
  else
     {int amount = 1;
      if (is_number(arg1))
         {amount = atoi(arg1);
          strcpy(arg1, arg2);
          strcpy(arg2, arg3);
         }
      else
         theplace = arg3;
      if (isname(theplace, "земля комната room ground"))
         where_bits = FIND_OBJ_ROOM;
      else
      if (isname(theplace, "инвентарь inventory"))
         where_bits = FIND_OBJ_INV;
      else
      if (isname(theplace, "экипировка equipment"))
         where_bits = FIND_OBJ_EQUIP;

      cont_dotmode = find_all_dots(arg2);
      if (cont_dotmode == FIND_INDIV)
         {mode = generic_find(arg2, where_bits, ch, &tmp_char, &cont);
          if (!cont)
             {sprintf(buf, "Вы не видите '%s'.\r\n", arg2);
              send_to_char(buf, ch);
             }
          else
          if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
             act("$o - не контейнер.", FALSE, ch, cont, 0, TO_CHAR);
          else
             get_from_container(ch, cont, arg1, mode, amount);
         }
      else
         {if (cont_dotmode == FIND_ALLDOT && !*arg2)
             {send_to_char("Взять из чего \"всего\" ?\r\n", ch);
              return;
             }
          for (cont = ch->carrying;
               cont && IS_SET(where_bits, FIND_OBJ_INV);
               cont = cont->next_content
              )
              if (CAN_SEE_OBJ(ch, cont) &&
                  (cont_dotmode == FIND_ALL || isname(arg2, cont->name))
                 )
                 {if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER)
                     {found = 1;
                      get_from_container(ch, cont, arg1, FIND_OBJ_INV, amount);
                     }
                  else
                  if (cont_dotmode == FIND_ALLDOT)
                     {found = 1;
                      act("$o - не контейнер.", FALSE, ch, cont, 0, TO_CHAR);
                     }
                 }
          for (cont = world[ch->in_room].contents;
               cont && IS_SET(where_bits, FIND_OBJ_ROOM);
               cont = cont->next_content
              )
              if (CAN_SEE_OBJ(ch, cont) &&
                  (cont_dotmode == FIND_ALL || isname(arg2, cont->name))
                 )
                 {if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER)
                     {get_from_container(ch, cont, arg1, FIND_OBJ_ROOM, amount);
                      found = 1;
                     }
                  else
                  if (cont_dotmode == FIND_ALLDOT)
                     {act("$o - не контейнер.", FALSE, ch, cont, 0, TO_CHAR);
                      found = 1;
                     }
                 }
          if (!found)
             {if (cont_dotmode == FIND_ALL)
                 send_to_char("Вы не смогли найти ни одного контейнера.\r\n", ch);
              else
                 {sprintf(buf, "Вы что-то не видите здесь '%s'.\r\n", arg2);
                  send_to_char(buf, ch);
                 }
             }
         }
     }
}


void perform_drop_gold(struct char_data * ch, int amount,
		            byte mode, room_rnum RDR)
{
  struct obj_data *obj;
  if (amount <= 0)
     send_to_char("Да, похоже Вы слишком переиграли сегодня.\r\n", ch);
  else
  if (GET_GOLD(ch) < amount)
     send_to_char("У Вас нет такой суммы !\r\n", ch);
  else
     {if (mode != SCMD_JUNK)
         {WAIT_STATE(ch, PULSE_VIOLENCE);	/* to prevent coin-bombing */
          obj = create_money(amount);
          if (mode == SCMD_DONATE)
             {sprintf(buf, "Вы выбросили %d %s на ветер.\r\n", amount, desc_count(amount,WHAT_MONEYu));
              send_to_char(buf, ch);
	          act("$n выбросил$g деньги... На ветер :(", FALSE, ch, 0, 0, TO_ROOM);
	          obj_to_room(obj, RDR);
	          act("$o исчез$Q в клубах дыма !", 0, 0, obj, 0, TO_ROOM);
             }
          else
             {if (!drop_wtrigger(obj, ch))
                 {extract_obj(obj);
                  return;
                 }
	          sprintf(buf, "Вы бросили %d %s на землю.", amount, desc_count(amount,WHAT_MONEYu));
	          send_to_char(buf, ch);
	          sprintf(buf, "$n бросил$g %s на землю.", money_desc(amount,3));
	          act(buf, TRUE, ch, 0, 0, TO_ROOM);
	          obj_to_room(obj, ch->in_room);
             }
         }
      else
         {sprintf(buf, "$n пожертвовал$g %s... В подарок Богам !",
	              money_desc(amount,3));
          act(buf, FALSE, ch, 0, 0, TO_ROOM);
          sprintf(buf, "Вы пожертвовали Богам %d %s.\r\n", amount, desc_count(amount,WHAT_MONEYu));
          send_to_char(buf, ch);
         }
      GET_GOLD(ch) -= amount;
     }
}


#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? \
		      "  It vanishes in a puff of smoke!" : "")

const char *drop_op[3][3] =
{
 {"выбросить",   "выбросили",   "выбросил"},
 {"пожертвовать","пожертвовали","пожертвовал"},
 {"бросить",     "бросили",     "бросил"}
};
int perform_drop(struct char_data * ch, struct obj_data * obj,
		     byte mode, const int sname, room_rnum RDR)
{
  int value;
  if (!drop_otrigger(obj, ch))
     return 0;
  if ((mode == SCMD_DROP) && !drop_wtrigger(obj, ch))
     return 0;
  if (IS_OBJ_STAT(obj, ITEM_NODROP))
     {sprintf(buf, "Вы не можете %s $o!", drop_op[sname][0]);
      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      return (0);
     }
  sprintf(buf, "Вы %s $o3.%s", drop_op[sname][1], VANISH(mode));
  act(buf, FALSE, ch, obj, 0, TO_CHAR);
  sprintf(buf, "$n %s$g $o3.%s", drop_op[sname][2], VANISH(mode));
  act(buf, TRUE, ch, obj, 0, TO_ROOM);
  obj_from_char(obj);

  if ((mode == SCMD_DONATE) && IS_OBJ_STAT(obj, ITEM_NODONATE))
     mode = SCMD_JUNK;

  switch (mode)
  {case SCMD_DROP:
    obj_to_room(obj, ch->in_room);
    return (0);
   case SCMD_DONATE:
    obj_to_room(obj, RDR);
    act("$o растворил$U в клубах дыма !", FALSE, 0, obj, 0, TO_ROOM);
    return (0);
   case SCMD_JUNK:
    value = MAX(1, MIN(200, GET_OBJ_COST(obj) / 16));
    extract_obj(obj);
    return (value);
   default:
    log("SYSERR: Incorrect argument %d passed to perform_drop.", mode);
    break;
  }

  return (0);
}



ACMD(do_drop)
{
  struct obj_data *obj, *next_obj;
  room_rnum RDR = 0;
  byte mode = SCMD_DROP;
  int dotmode, amount = 0, multi;
  int sname;

  switch (subcmd)
      {
  case SCMD_JUNK:
    sname = 0;
    mode = SCMD_JUNK;
    break;
  case SCMD_DONATE:
    sname = 1;
    mode = SCMD_DONATE;
    switch (number(0, 2))
           {
    case 0:
      mode = SCMD_JUNK;
      break;
    case 1:
    case 2:
      RDR = real_room(donation_room_1);
      break;
/*    case 3: RDR = real_room(donation_room_2); break;
      case 4: RDR = real_room(donation_room_3); break;
*/
            }
      if (RDR == NOWHERE)
         {send_to_char("Вы не можете этого здесь сделать.\r\n", ch);
          return;
         }
      break;
  default:
    sname = 2;
    break;
  }

  argument = one_argument(argument, arg);

  if (!*arg)
     {sprintf(buf, "Что Вы хотите %s?\r\n", drop_op[sname][0]);
      send_to_char(buf, ch);
      return;
     }
  else
  if (is_number(arg))
     {multi = atoi(arg);
      one_argument(argument, arg);
      if (!str_cmp("coins", arg) || !str_cmp("coin", arg) ||
          !str_cmp("кун", arg) || !str_cmp("денег", arg))
          perform_drop_gold(ch, multi, mode, RDR);
      else
      if (multi <= 0)
         send_to_char("Не имеет смысла.\r\n", ch);
      else
      if (!*arg)
         {sprintf(buf, "%s %d чего ?\r\n", drop_op[sname][0], multi);
          send_to_char(buf, ch);
         }
      else
      if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
         {sprintf(buf, "У Вас нет ничего похожего на %s.\r\n", arg);
          send_to_char(buf, ch);
         }
      else
         {do {next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);
              amount += perform_drop(ch, obj, mode, sname, RDR);
              obj = next_obj;
             }
          while (obj && --multi);
         }
     }
  else
     {dotmode = find_all_dots(arg);
      /* Can't junk or donate all */
      if ((dotmode == FIND_ALL) &&
          (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE))
          {if (subcmd == SCMD_JUNK)
	          send_to_char("Вас с нетерпением ждут. У психиатра :)\r\n", ch);
           else
	          send_to_char("Такую жертву я принять не могу !\r\n", ch);
           return;
          }
      if (dotmode == FIND_ALL)
         {if (!ch->carrying)
	         send_to_char("А у Вас ничего и нет.\r\n", ch);
          else
	         for (obj = ch->carrying; obj; obj = next_obj)
	             {next_obj = obj->next_content;
	              amount += perform_drop(ch, obj, mode, sname, RDR);
	             }
         }
      else
      if (dotmode == FIND_ALLDOT)
         {if (!*arg)
             {sprintf(buf, "%s \"все\" какого типа предметов ?\r\n", drop_op[sname][0]);
	          send_to_char(buf, ch);
	          return;
             }
          if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
             {sprintf(buf, "У Вас нет ничего похожего на '%s'.\r\n", arg);
	          send_to_char(buf, ch);
             }
          while (obj)
             {next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);
	          amount += perform_drop(ch, obj, mode, sname, RDR);
	          obj = next_obj;
             }
         }
      else
         {if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
             {sprintf(buf, "У Вас нет '%s'.\r\n", arg);
	          send_to_char(buf, ch);
             }
          else
	         amount += perform_drop(ch, obj, mode, sname, RDR);
         }
  }

  if (amount && (subcmd == SCMD_JUNK))
     {send_to_char("Боги не обратили внимания на этот хлам.\r\n", ch);
      act("$n принес$q жертву. Но Боги были глухи к н$m !", TRUE, ch, 0, 0, TO_ROOM);
      /* GET_GOLD(ch) += amount; */
     }

}


void perform_give(struct char_data * ch, struct char_data * vict,
		       struct obj_data * obj)
{
  if (IS_OBJ_STAT(obj, ITEM_NODROP))
     {act("Вы не можете передать $o3!!", FALSE, ch, obj, 0, TO_CHAR);
      return;
     }
  if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict))
     {act("У $N1 заняты руки.", FALSE, ch, 0, vict, TO_CHAR);
      return;
     }
  if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict))
     {act("$E не может нести такой вес.", FALSE, ch, 0, vict, TO_CHAR);
      return;
     }
  if (!give_otrigger(obj, ch, vict) ||
      !receive_mtrigger(vict, ch, obj)
     )
     return;

  act("Вы дали $o3 $N2.", FALSE, ch, obj, vict, TO_CHAR);
  act("$n дал$g Вам $o3.", FALSE, ch, obj, vict, TO_VICT);
  act("$n дал$g $o3 $N2.", TRUE, ch, obj, vict, TO_NOTVICT);
  obj_from_char(obj);
  obj_to_char(obj,vict);
}

/* utility function for give */
struct char_data *give_find_vict(struct char_data * ch, char *arg)
{
  struct char_data *vict;

  if (!*arg)
     {send_to_char("Кому ?\r\n", ch);
      return (NULL);
     }
  else
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {send_to_char(NOPERSON, ch);
      return (NULL);
     }
  else
  if (vict == ch)
     {send_to_char("Вы переложили ЭТО из одного кармана в другой.\r\n", ch);
      return (NULL);
     }
  else
     return (vict);
}


void perform_give_gold(struct char_data * ch, struct char_data * vict,
		            int amount)
{
  if (amount <= 0)
     {send_to_char("Ха-ха-ха (3 раза)...\r\n", ch);
      return;
     }
  if (GET_GOLD(ch) < amount &&
      (IS_NPC(ch) || !IS_GOD(ch))
     )
     {send_to_char("И откуда Вы их взять собираетесь ?\r\n", ch);
      return;
     }
  send_to_char(OK, ch);
  sprintf(buf, "$n дал$g Вам %d %s.", amount, desc_count(amount, WHAT_MONEYu));
  act(buf, FALSE, ch, 0, vict, TO_VICT);
  sprintf(buf, "$n дал$g %s $N2.", money_desc(amount,3));
  act(buf, TRUE, ch, 0, vict, TO_NOTVICT);
  if (IS_NPC(ch) || !IS_GOD(ch))
     GET_GOLD(ch) -= amount;
  GET_GOLD(vict) += amount;
  bribe_mtrigger(vict, ch, amount);
}


ACMD(do_give)
{
  int amount, dotmode;
  struct char_data *vict;
  struct obj_data *obj, *next_obj;

  argument = one_argument(argument, arg);

  if (!*arg)
     send_to_char("Дать что и кому ?\r\n", ch);
  else
  if (is_number(arg))
     {amount = atoi(arg);
      argument = one_argument(argument, arg);
      if (!strn_cmp("coin",arg,4)  ||
          !strn_cmp("кун",arg,5) ||
          !str_cmp("денег",arg))
         {one_argument(argument, arg);
          if ((vict = give_find_vict(ch, arg)) != NULL)
	         perform_give_gold(ch, vict, amount);
          return;
         }
      else
      if (!*arg)
         {/* Give multiple code. */
          sprintf(buf, "Чего %d Вы хотите дать ?\r\n", amount);
          send_to_char(buf, ch);
         }
      else
      if (!(vict = give_find_vict(ch, argument)))
         {return;
         }
      else
      if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
         {sprintf(buf, "У Вас нет '%s'.\r\n", arg);
          send_to_char(buf, ch);
         }
      else
         {while (obj && amount--)
                {next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);
	             perform_give(ch, vict, obj);
	             obj = next_obj;
                }
         }
     }
  else
     {one_argument(argument, buf1);
      if (!(vict = give_find_vict(ch, buf1)))
         return;
      dotmode = find_all_dots(arg);
      if (dotmode == FIND_INDIV)
         {if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
             {sprintf(buf, "У Вас нет '%s'.\r\n", arg);
	          send_to_char(buf, ch);
             }
          else
	         perform_give(ch, vict, obj);
         }
      else
         {if (dotmode == FIND_ALLDOT && !*arg)
             {send_to_char("Дать \"все\" какого типа предметов ?\r\n", ch);
	          return;
             }
          if (!ch->carrying)
	         send_to_char("У Вас ведь ничего нет.\r\n", ch);
          else
	         for (obj = ch->carrying; obj; obj = next_obj)
	             {next_obj = obj->next_content;
	              if (CAN_SEE_OBJ(ch, obj) &&
	                  ((dotmode == FIND_ALL || isname(arg, obj->name))))
	                 perform_give(ch, vict, obj);
	             }
         }
     }
}



void weight_change_object(struct obj_data * obj, int weight)
{
  struct obj_data *tmp_obj;
  struct char_data *tmp_ch;

  if (obj->in_room != NOWHERE)
     {GET_OBJ_WEIGHT(obj) += weight;
     }
  else
  if ((tmp_ch = obj->carried_by))
     {obj_from_char(obj);
      GET_OBJ_WEIGHT(obj) += weight;
      obj_to_char(obj, tmp_ch);
     }
  else
  if ((tmp_obj = obj->in_obj))
     {obj_from_obj(obj);
      GET_OBJ_WEIGHT(obj) += weight;
      obj_to_obj(obj, tmp_obj);
     }
  else
     {log("SYSERR: Unknown attempt to subtract weight from an object.");
     }
}



void name_from_drinkcon(struct obj_data * obj)
{
  int  i,c,j=0;
  char new_name[MAX_STRING_LENGTH];

  for (i = 0; *(obj->name + i) && a_isspace(*(obj->name + i)); i++);
  for (j = 0; *(obj->name + i) && !(a_isspace(*(obj->name + i)));
       new_name[j] = *(obj->name + i), i++, j++);
  new_name[j] = '\0';
  if (*new_name)
     {if (GET_OBJ_RNUM(obj) < 0 ||
          obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
         free(obj->name);
      obj->name = str_dup(new_name);
     }

  for (i = 0; *(obj->short_description + i) && a_isspace(*(obj->short_description + i)); i++);
  for (j = 0; *(obj->short_description + i) && !(a_isspace(*(obj->short_description + i)));
       new_name[j] = *(obj->short_description + i), i++, j++);
  new_name[j] = '\0';
  if (*new_name)
     {if (GET_OBJ_RNUM(obj) < 0 ||
          obj->short_description != obj_proto[GET_OBJ_RNUM(obj)].short_description)
         free(obj->short_description);
      obj->short_description = str_dup(new_name);
     }


  for (c = 0; c < NUM_PADS; c++)
      {for (i = 0; a_isspace(*(obj->PNames[c] + i)); i++);
       for (j = 0; !a_isspace(*(obj->PNames[c] + i));
            new_name[j] = *(obj->PNames[c] + i), i++, j++ );
       new_name[j] = '\0';
       if (*new_name)
          {if (GET_OBJ_RNUM(obj) < 0 ||
               obj->PNames[c] != obj_proto[GET_OBJ_RNUM(obj)].PNames[c])
              free(obj->PNames[c]);
           obj->PNames[c] = str_dup(new_name);
          }
      }
}



void name_to_drinkcon(struct obj_data * obj, int type)
{ int  c;
  char new_name[MAX_INPUT_LENGTH];

  sprintf(new_name, "%s %s", obj->name, drinknames[type]);
  if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
     free(obj->name);
  obj->name = str_dup(new_name);

  sprintf(new_name, "%s c %s", obj->short_description, drinknames[type]);
  if (GET_OBJ_RNUM(obj) < 0 || obj->short_description != obj_proto[GET_OBJ_RNUM(obj)].short_description)
     free(obj->short_description);
  obj->short_description = str_dup(new_name);


  for (c = 0; c < NUM_PADS; c++)
      {  sprintf(new_name, "%s с %s", obj->PNames[c], drinknames[type]);
         if (GET_OBJ_RNUM(obj) < 0 ||
             obj->PNames[c] != obj_proto[GET_OBJ_RNUM(obj)].PNames[c])
            free(obj->PNames[c]);
         obj->PNames[c] = str_dup(new_name);
      }
}



ACMD(do_drink)
{ struct obj_data *temp;
  struct affected_type af;
  int amount, weight, duration;
  int on_ground = 0;

  one_argument(argument, arg);

  if (IS_NPC(ch))	/* Cannot use GET_COND() on mobs. */
     return;

  if (!*arg)
     {send_to_char("Пить из чего ?\r\n", ch);
      return;
     }

  if (FIGHTING(ch))
     {send_to_char("Не стоит отвлекаться в бою.\r\n",ch);
      return;
     }

  if (!(temp = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {if (!(temp = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents)))
         {send_to_char("Вы не смогли это найти!\r\n", ch);
          return;
         }
      else
         on_ground = 1;
     }
  if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
      (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN))
     {send_to_char("Не стоит. Козлят и так много !\r\n", ch);
      return;
     }
  if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON))
     {send_to_char("Прежде это стоит поднять.\r\n", ch);
      return;
     }

  if ((GET_COND(ch, DRUNK) > CHAR_DRUNKED) && (GET_COND(ch, THIRST) > 0))
     {// The pig is drunk
      send_to_char("Вы не смогли сделать и глотка.\r\n", ch);
      act("$n попытал$u выпить еще, но не смог$q сделать и глотка.", TRUE, ch, 0, 0, TO_ROOM);
      return;
     }

  if (!GET_OBJ_VAL(temp, 1))
     {send_to_char("Пусто.\r\n", ch);
      return;
     }

  if (subcmd == SCMD_DRINK)
     {if (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] > 0)
         amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK];
      else
         amount = number(3, 10);
     }
  else
     {amount = 1;
     }

  amount = MIN(amount, GET_OBJ_VAL(temp, 1));
  amount = MIN(amount, 24 - GET_COND(ch,THIRST));

  if (amount <= 0)
     {send_to_char("В Вас больше не лезет.\r\n", ch);
      return;
     }
  else
  if (subcmd == SCMD_DRINK)
     {sprintf(buf, "$n выпил$g %s из $o1.", drinks[GET_OBJ_VAL(temp, 2)]);
      act(buf, TRUE, ch, temp, 0, TO_ROOM);
      sprintf(buf, "Вы выпили %s из %s.\r\n",
                   drinks[GET_OBJ_VAL(temp, 2)], OBJN(temp,ch,1));
      send_to_char(buf, ch);
     }
  else
     {act("$n отхлебнул$g из $o1.", TRUE, ch, temp, 0, TO_ROOM);
      sprintf(buf, "Вы узнали вкус %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
      send_to_char(buf, ch);
     }

  /* You can't subtract more than the object weighs */
  weight = MIN(amount, GET_OBJ_WEIGHT(temp));

  if (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)
     weight_change_object(temp, -weight); /* Subtract amount */
  gain_condition(ch, DRUNK,
	             (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] * amount) / 4);

  gain_condition(ch, FULL,
	             (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][FULL] * amount) / 4);

  gain_condition(ch, THIRST,
	             (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount) / 4);

  if (GET_COND(ch, THIRST) > 20)
     send_to_char("Вы не чувствуете жажды.\r\n", ch);

  if (GET_COND(ch, FULL) > 20)
     send_to_char("Вы чувствуете приятную тяжесть в желудке.\r\n", ch);

  if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED)
     {if (GET_COND(ch,DRUNK) >= CHAR_MORTALLY_DRUNKED)
         {send_to_char("Напилися Вы пьяны, не дойти Вам до дому....\r\n", ch);
          duration = 2;
         }
      else
         {send_to_char("Приятное тепло разлилось по Вашему телу.\r\n", ch);
          duration = 2 + MAX(0, GET_COND(ch,DRUNK) - CHAR_DRUNKED);
         }
      GET_DRUNK_STATE(ch) = MAX(GET_DRUNK_STATE(ch),GET_COND(ch, DRUNK));
      if (!AFF_FLAGGED(ch, AFF_DRUNKED) && !AFF_FLAGGED(ch, AFF_ABSTINENT))
         {send_to_char("Винные пары ударили Вам в голову.\r\n", ch);
          /***** Decrease AC ******/
          af.type      = SPELL_DRUNKED;
          af.duration  = pc_duration(ch,duration,0,0,0,0);
          af.modifier  = -20;
          af.location  = APPLY_AC;
          af.bitvector = AFF_DRUNKED;
          af.battleflag= 0;
          affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
          /***** Decrease HR ******/
          af.type      = SPELL_DRUNKED;
          af.duration  = pc_duration(ch,duration,0,0,0,0);
          af.modifier  = -2;
          af.location  = APPLY_HITROLL;
          af.bitvector = AFF_DRUNKED;
          af.battleflag= 0;
          affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
          /***** Increase DR ******/
          af.type      = SPELL_DRUNKED;
          af.duration  = pc_duration(ch,duration,0,0,0,0);
          af.modifier  = (GET_LEVEL(ch) + 4) / 5;
          af.location  = APPLY_DAMROLL;
          af.bitvector = AFF_DRUNKED;
          af.battleflag= 0;
          affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
         }
     }

  if (GET_OBJ_VAL(temp, 3) && !IS_GOD(ch))
     {/* The shit was poisoned ! */
      send_to_char("Что-то вкус какой-то странный !\r\n", ch);
      act("$n поперхнул$u и закашлял$g.", TRUE, ch, 0, 0, TO_ROOM);

      af.type      = SPELL_POISON;
      af.duration  = pc_duration(ch,amount == 1 ? amount : amount * 3,0,0,0,0);
      af.modifier  = -2;
      af.location  = APPLY_STR;
      af.bitvector = AFF_POISON;
      af.battleflag= 0;
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      af.type      = SPELL_POISON;
      af.modifier  = amount * 3;
      af.location  = APPLY_POISON;
      af.bitvector = AFF_POISON;
      af.battleflag= 0;
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      ch->Poisoner = 0;
     }

  /* empty the container, and no longer poison. 999 - whole fountain */
  if (GET_OBJ_TYPE(temp)   != ITEM_FOUNTAIN ||
      GET_OBJ_VAL(temp, 1) != 999)
     GET_OBJ_VAL(temp, 1) -= amount;
  if (!GET_OBJ_VAL(temp, 1))
     {/* The last bit */
      GET_OBJ_VAL(temp, 2) = 0;
      GET_OBJ_VAL(temp, 3) = 0;
      name_from_drinkcon(temp);
     }

  return;
}


ACMD(do_drunkoff)
{
  struct obj_data *obj;
  struct affected_type af[3];
  struct timed_type    timed;
  int amount, weight, prob, percent, duration;
  int on_ground = 0;

  if (IS_NPC(ch))	/* Cannot use GET_COND() on mobs. */
     return;

  if (FIGHTING(ch))
     {send_to_char("Не стоит отвлекаться в бою.\r\n",ch);
      return;
     }

  if (AFF_FLAGGED(ch, AFF_DRUNKED))
     {send_to_char("Вы хотите испортить себе весь кураж ?\r\n"
                   "Это не есть по русски !\r\n", ch);
      return;
     }

  if (!AFF_FLAGGED(ch, AFF_ABSTINENT) && GET_COND(ch,DRUNK) < CHAR_DRUNKED)
     {send_to_char("Не стоит делать этого на трезвую голову.\r\n", ch);
      return;
     }

  if (timed_by_skill(ch, SKILL_DRUNKOFF))
     {send_to_char("Вы не в состоянии так часто похмеляться.\r\n"
                   "Попросите Богов закодировать Вас.\r\n", ch);
      return;
     }

  one_argument(argument, arg);

  if (!*arg)
     {for (obj = ch->carrying; obj; obj = obj->next_content)
          if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
             break;
      if (!obj)
         {send_to_char("У Вас нет подходящего напитка для похмелья.\r\n", ch);
          return;
         }
     }
  else
  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {if (!(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents)))
         {send_to_char("Вы не смогли это найти!\r\n", ch);
          return;
         }
      else
         on_ground = 1;
     }

  if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
      (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
     {send_to_char("Этим Вы вряд-ли сможете похмелиться.\r\n", ch);
      return;
     }

  if (on_ground && (GET_OBJ_TYPE(obj) == ITEM_DRINKCON))
     {send_to_char("Прежде это стоит поднять.\r\n", ch);
      return;
     }

  if (!GET_OBJ_VAL(obj, 1))
     {send_to_char("Пусто.\r\n", ch);
      return;
     }

  switch(GET_OBJ_VAL(obj, 2))
  {
   case LIQ_BEER:
   case LIQ_WINE:
   case LIQ_ALE:
   case LIQ_QUAS:
   case LIQ_BRANDY:
   case LIQ_VODKA:
   case LIQ_BRAGA:
       break;
   default:
       send_to_char("Вспомните народную мудрость :\r\n"
                    "\"Клин вышибают клином...\"\r\n", ch);
       return;
  }

  timed.skill = SKILL_DRUNKOFF;
  timed.time  = 12;
  timed_to_char(ch, &timed);

  amount = MAX(1, GET_WEIGHT(ch) / 50);
  percent= number(1,skill_info[SKILL_DRUNKOFF].max_percent);
  if (amount > GET_OBJ_VAL(obj,1))
     percent += 50;
  prob   = train_skill(ch,SKILL_DRUNKOFF,skill_info[SKILL_DRUNKOFF].max_percent,0);
  amount = MIN(amount, GET_OBJ_VAL(obj, 1));
  weight = MIN(amount, GET_OBJ_WEIGHT(obj));
  weight_change_object(obj, -weight);	/* Subtract amount */
  GET_OBJ_VAL(obj, 1) -= amount;
  if (!GET_OBJ_VAL(obj, 1))
     {/* The last bit */
      GET_OBJ_VAL(obj, 2) = 0;
      GET_OBJ_VAL(obj, 3) = 0;
      name_from_drinkcon(obj);
     }

  if (percent > prob)
     {sprintf(buf,"Вы отхлебнули %s из $o1, но Ваша голова стала еще тяжелее...",
                  drinks[GET_OBJ_VAL(obj, 2)]);
      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      act("$n попробовал$g похмелиться, но это не пошло $m на пользу.",
               FALSE, ch, 0, 0, TO_ROOM);
      duration        = MAX(1, amount / 3);
      af[0].type      = SPELL_ABSTINENT;
      af[0].duration  = pc_duration(ch,duration,0,0,0,0);
      af[0].modifier  = 0;
      af[0].location  = APPLY_DAMROLL;
      af[0].bitvector = AFF_ABSTINENT;
      af[0].battleflag= 0;
      af[1].type      = SPELL_ABSTINENT;
      af[1].duration  = pc_duration(ch,duration,0,0,0,0);
      af[1].modifier  = 0;
      af[1].location  = APPLY_HITROLL;
      af[1].bitvector = AFF_ABSTINENT;
      af[1].battleflag= 0;
      af[2].type      = SPELL_ABSTINENT;
      af[2].duration  = pc_duration(ch,duration,0,0,0,0);
      af[2].modifier  = 0;
      af[2].location  = APPLY_AC;
      af[2].bitvector = AFF_ABSTINENT;
      af[2].battleflag= 0;
      switch (number(0, GET_SKILL(ch,SKILL_DRUNKOFF) / 20))
      {case 0:
       case 1:   af[0].modifier  = -2;
       case 2:
       case 3:   af[1].modifier  = -2;
       default:  af[2].modifier  = 10;
      }
      for (prob = 0; prob < 3; prob++)
          affect_join(ch, &af[prob], TRUE, FALSE, TRUE, FALSE);
      gain_condition(ch,DRUNK,amount);
     }
  else
     {sprintf(buf,"Вы отхлебнули %s из $o1 и почувствовали приятную легкость во всем теле...",
              drinks[GET_OBJ_VAL(obj, 2)]);
      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      act("$n похмелил$u и расцвел$g прям на глазах.",
          FALSE, ch, 0, 0, TO_ROOM);
      affect_from_char(ch, SPELL_ABSTINENT);
     }

  return;
}



ACMD(do_eat)
{
  struct obj_data *food;
  struct affected_type af;
  int amount;

  one_argument(argument, arg);

  if (IS_NPC(ch))	/* Cannot use GET_COND() on mobs. */
     return;

  if (!*arg)
     {send_to_char("Чем Вы собрались закусить ?\r\n", ch);
      return;
     }

  if (FIGHTING(ch))
     {send_to_char("Не стоит отвлекаться в бою.\r\n",ch);
      return;
     }

  if (!(food = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {sprintf(buf, "У Вас нет '%s'.\r\n", arg);
      send_to_char(buf, ch);
      return;
     }
  if (subcmd == SCMD_TASTE &&
      ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
       (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN)))
     {do_drink(ch, argument, 0, SCMD_SIP);
      return;
     }
  if (GET_OBJ_TYPE(food) != ITEM_FOOD && !IS_GOD(ch))
     {send_to_char("Это несъедобно !\r\n", ch);
      return;
     }
  if (GET_COND(ch, FULL) > 20)
     {/* Stomach full */
      send_to_char("Вы слишком сыты для этого !\r\n", ch);
      return;
     }
  if (subcmd == SCMD_EAT)
     {if (!GET_COMMSTATE(ch))
         {act("Вы съели $o3.", FALSE, ch, food, 0, TO_CHAR);
          act("$n съел$g $o3.", TRUE, ch, food, 0, TO_ROOM);
	 }
     }
  else
     {if (!GET_COMMSTATE(ch))
         {act("Вы откусили маленький кусочек от $o1.", FALSE, ch, food, 0, TO_CHAR);
          act("$n попробовал$g $o3 на вкус.", TRUE, ch, food, 0, TO_ROOM);
	 }
     }

  amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, 0) : 1);

  gain_condition(ch, FULL, amount);

  if (GET_COND(ch, FULL) > 20)
     send_to_char("Вы наелись.\r\n", ch);

  if (GET_OBJ_VAL(food, 3) && !IS_IMMORTAL(ch))
     {/* The shit was poisoned ! */
      send_to_char("Однако, какой странный вкус !\r\n", ch);
      act("$n закашлял$u и начал$g отплевываться.", FALSE, ch, 0, 0, TO_ROOM);

      af.type      = SPELL_POISON;
      af.duration  = pc_duration(ch,amount == 1 ? amount : amount * 2,0,0,0,0);
      af.modifier  = 0;
      af.location  = APPLY_STR;
      af.bitvector = AFF_POISON;
      af.battleflag= 0;
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      af.type      = SPELL_POISON;
      af.duration  = pc_duration(ch,amount == 1 ? amount : amount * 2,0,0,0,0);
      af.modifier  = amount * 3;
      af.location  = APPLY_POISON;
      af.bitvector = AFF_POISON;
      af.battleflag= 0;
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      ch->Poisoner = 0;
     }
  if (subcmd == SCMD_EAT)
     extract_obj(food);
  else
     {if (!(--GET_OBJ_VAL(food, 0)))
         {send_to_char("Вы доели все !\r\n", ch);
          extract_obj(food);
         }
     }
}


ACMD(do_pour)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *from_obj = NULL, *to_obj = NULL;
  int amount;

  two_arguments(argument, arg1, arg2);

  if (subcmd == SCMD_POUR)
     {if (!*arg1)
         {/* No arguments */
          send_to_char("Откуда переливаем ?\r\n", ch);
          return;
         }
     if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
        {send_to_char("У Вас нет этого !\r\n", ch);
         return;
        }
     if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON)
        {send_to_char("Вы не можете из этого переливать !\r\n", ch);
         return;
        }
     }
  if (subcmd == SCMD_FILL)
     {if (!*arg1)
         {/* no arguments */
          send_to_char("Что и из чего Вы хотели бы наполнить ?\r\n", ch);
          return;
         }
      if (!(to_obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
         {send_to_char("У Вас этого нет !\r\n", ch);
          return;
         }
      if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON)
         {act("Вы не можете наполнить $o3!", FALSE, ch, to_obj, 0, TO_CHAR);
          return;
         }
      if (!*arg2)
         {/* no 2nd argument */
          act("Из чего Вы планируете наполнить $o3?", FALSE, ch, to_obj, 0, TO_CHAR);
          return;
         }
      if (!(from_obj = get_obj_in_list_vis(ch, arg2, world[ch->in_room].contents)))
         {sprintf(buf, "Вы не видите здесь '%s'.\r\n", arg2);
          send_to_char(buf, ch);
          return;
         }
      if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN)
         {act("Вы не сможете ничего наполнить из $o1.", FALSE, ch, from_obj, 0, TO_CHAR);
          return;
         }
     }
  if (GET_OBJ_VAL(from_obj, 1) == 0)
     {act("Пусто.", FALSE, ch, from_obj, 0, TO_CHAR);
      return;
     }
  if (subcmd == SCMD_POUR)
     {/* pour */
      if (!*arg2)
         {send_to_char("Куда Вы хотите лить ?  На землю или во что-то ?\r\n", ch);
          return;
         }
      if (!str_cmp(arg2, "out") || !str_cmp(arg2,"земля"))
         {act("$n опустошил$g $o3.", TRUE, ch, from_obj, 0, TO_ROOM);
          act("Вы опустошили $o3.", FALSE, ch, from_obj, 0, TO_CHAR);

          weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1)); /* Empty */

          GET_OBJ_VAL(from_obj, 1) = 0;
          GET_OBJ_VAL(from_obj, 2) = 0;
          GET_OBJ_VAL(from_obj, 3) = 0;
          name_from_drinkcon(from_obj);

          return;
         }
      if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying)))
         {send_to_char("Вы не можете этого найти !\r\n", ch);
          return;
         }
      if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
          (GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)
	 )
	 {send_to_char("Вы не сможете в это налить.\r\n", ch);
          return;
         }
     }
  if (to_obj == from_obj)
     {send_to_char("Более тупого действа Вы придумать, конечно, не могли.\r\n", ch);
      return;
     }
  if ((GET_OBJ_VAL(to_obj, 1) != 0) &&
      (GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2)))
     {send_to_char("Вы станете неплохим Химиком, но не в нашей игре.\r\n", ch);
      return;
     }
  if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0)))
     {send_to_char("Там нет места.\r\n", ch);
      return;
     }
  if (subcmd == SCMD_POUR)
     {sprintf(buf, "Вы занялись переливанием %s в %s.",
	      drinks[GET_OBJ_VAL(from_obj, 2)], OBJN(to_obj,ch,3));
      send_to_char(buf, ch);
     }
  if (subcmd == SCMD_FILL)
     {act("Вы наполнили $o3 из $O1.", FALSE, ch, to_obj, from_obj, TO_CHAR);
      act("$n наполнил$g $o3 из $O1.", TRUE, ch, to_obj, from_obj, TO_ROOM);
     }
  /* New alias */
  if (GET_OBJ_VAL(to_obj, 1) == 0)
     name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));

  /* First same type liq. */
  GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);

  /* Then how much to pour */
  if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN ||
      GET_OBJ_VAL(from_obj,1) != 999)
     GET_OBJ_VAL(from_obj, 1) -= (amount =
                                  (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1)));
  else
      amount = GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1);

  GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);

  /* Then the poison boogie */
  GET_OBJ_VAL(to_obj, 3) =
    (GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));

  if (GET_OBJ_VAL(from_obj, 1) < 0)
     {/* There was too little */
      GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
      amount += GET_OBJ_VAL(from_obj, 1);
      GET_OBJ_VAL(from_obj, 1) = 0;
      GET_OBJ_VAL(from_obj, 2) = 0;
      GET_OBJ_VAL(from_obj, 3) = 0;
      name_from_drinkcon(from_obj);
     }

  /* And the weight boogie */
  if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN)
     weight_change_object(from_obj, -amount);
  weight_change_object(to_obj, amount);	/* Add weight */
}



void wear_message(struct char_data * ch, struct obj_data * obj, int where)
{
  const char *wear_messages[][2] = {
    {"$n засветил$g $o3 и взял$g во вторую руку.",
    "Вы зажгли $o3 и взяли во вторую руку."},

    {"$n0 надел$g $o3 на правый указательный палец.",
    "Вы надели $o3 на правый указательный палец."},

    {"$n0 надел$g $o3 на левый указательный палец.",
    "Вы надели $o3 на левый указательный палец."},

    {"$n0 надел$g $o3 вокруг шеи.",
    "Вы надели $o3 вокруг шеи."},

    {"$n0 надел$g $o3 на грудь.",
    "Вы надели $o3 на грудь."},

    {"$n0 надел$g $o3 на туловище.",
    "Вы надели $o3 на туловище.",},

    {"$n0 водрузил$g $o3 на голову.",
    "Вы водрузили $o3 себе на голову."},

    {"$n0 надел$g $o3 на ноги.",
    "Вы надели $o3 на ноги."},

    {"$n0 обул$g $o3.",
    "Вы обули $o3."},

    {"$n0 надел$g $o3 на кисти.",
    "Вы надели $o3 на кисти."},

    {"$n0 надел$g $o3 на руки.",
    "Вы надели $o3 на руки."},

    {"$n0 начал$g использовать $o3 как щит.",
    "Вы начали использовать $o3 как щит."},

    {"$n0 облачил$u в $o3.",
    "Вы облачились в $o3."},

    {"$n0 надел$g $o3 вокруг пояса.",
     "Вы надели $o3 вокруг пояса."},

    {"$n0 надел$g $o3 вокруг правого запястья.",
    "Вы надели $o3 вокруг правого запястья."},

    {"$n0 надел$g $o3 вокруг левого запястья.",
    "Вы надели $o3 вокруг левого запястья."},

    {"$n0 взял$g в правую руку $o3.",
    "Вы вооружились $o4."},

    {"$n0 взял$g $o3 в левую руку.",
    "Вы взяли $o3 в левую руку."},

    {"$n0 взял$g $o3 в обе руки.",
     "Вы взяли $o3 в обе руки."
    }
  };

  act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
  act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
}



void perform_wear(struct char_data * ch, struct obj_data * obj, int where)
{
  /*
   * ITEM_WEAR_TAKE is used for objects that do not require special bits
   * to be put into that position (e.g. you can hold any object, not just
   * an object with a HOLD bit.)
   */

  int wear_bitvectors[] =
  { ITEM_WEAR_TAKE,  ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,
    ITEM_WEAR_NECK,  ITEM_WEAR_BODY,   ITEM_WEAR_HEAD,   ITEM_WEAR_LEGS,
    ITEM_WEAR_FEET,  ITEM_WEAR_HANDS,  ITEM_WEAR_ARMS,   ITEM_WEAR_SHIELD,
    ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST,  ITEM_WEAR_WRIST,  ITEM_WEAR_WRIST,
    ITEM_WEAR_WIELD, ITEM_WEAR_TAKE,   ITEM_WEAR_BOTHS
  };

  const char *already_wearing[] =
  { "Вы уже используете свет.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "У Вас уже что-то надето на пальцах.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "У Вас уже что-то надето на шею.\r\n",
    "У Вас уже что-то надето на туловище.\r\n",
    "У Вас уже что-то надето на голову.\r\n",
    "У Вас уже что-то надето на ноги.\r\n",
    "У Вас уже что-то надето на ступни.\r\n",
    "У Вас уже что-то надето на кисти.\r\n",
    "У Вас уже что-то надето на руки.\r\n",
    "Вы уже используете щит.\r\n",
    "Вы уже облачены во что-то.\r\n",
    "У Вас уже что-то надето на пояс.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "У Вас уже что-то надето на запястья.\r\n",
    "Вы уже что-то держите в правой руке.\r\n",
    "Вы уже что-то держите в левой руке.\r\n",
    "Вы уже держите оружие в обеих руках.\r\n"
  };

  /* first, make sure that the wear position is valid. */
  if (!CAN_WEAR(obj, wear_bitvectors[where]))
     {act("Вы не можете одеть $o3 на эту часть тела.", FALSE, ch, obj, 0, TO_CHAR);
      return;
     }
  /* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
  if ( /* не может держать если есть свет или двуручник */
      (where == WEAR_HOLD && (GET_EQ(ch,WEAR_BOTHS) || GET_EQ(ch,WEAR_LIGHT) ||
                              GET_EQ(ch,WEAR_SHIELD))) ||
       /* не может вооружиться если есть двуручник */
      (where == WEAR_WIELD && GET_EQ(ch,WEAR_BOTHS)) ||
       /* не может держать щит если что-то держит или двуручник */
      (where == WEAR_SHIELD && (GET_EQ(ch,WEAR_HOLD) || GET_EQ(ch,WEAR_BOTHS))) ||
      /* не может двуручник если есть щит, свет, вооружен или держит */
      (where == WEAR_BOTHS && (GET_EQ(ch,WEAR_HOLD) || GET_EQ(ch,WEAR_LIGHT) ||
                               GET_EQ(ch,WEAR_SHIELD) || GET_EQ(ch,WEAR_WIELD))) ||
      /* не может держать свет если двуручник или держит */
      (where == WEAR_LIGHT && (GET_EQ(ch,WEAR_HOLD) || GET_EQ(ch,WEAR_BOTHS)))
     )
     {send_to_char("У Вас заняты руки.\r\n",ch);
      return;
     }

  if ((where == WEAR_FINGER_R) ||
      (where == WEAR_NECK_1) ||
      (where == WEAR_WRIST_R))
     if (GET_EQ(ch, where))
        where++;

  if (GET_EQ(ch, where))
     {send_to_char(already_wearing[where], ch);
      return;
     }
  if (!wear_otrigger(obj, ch, where))
     return;

  obj_from_char(obj);
  if (preequip_char(ch,obj,where) && obj->worn_by == ch)
     {wear_message(ch,obj,where);
      postequip_char(ch,obj);
     }
}



int find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg)
{
  int where = -1;

  /* \r to prevent explicit wearing. Don't use \n, it's end-of-array marker. */
  const char *keywords[] = {
    "\r!RESERVED!",
    "палец",
    "\r!RESERVED!",
    "шея",
    "грудь",
    "тело",
    "голова",
    "ноги",
    "ступни",
    "кисти",
    "руки",
    "щит",
    "плечи",
    "пояс",
    "запястья",
    "\r!RESERVED!",
    "\r!RESERVED!",
    "\r!RESERVED!",
    "\n"
  };

  if (!arg || !*arg)
     {
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
     }
  else
    {if (((where = search_block(arg, keywords, FALSE)) < 0) ||
         (*arg=='!'))
        {sprintf(buf, "'%s'?  Странная анатомия у этих русских !\r\n", arg);
         send_to_char(buf, ch);
         return -1;
        }
    }

  return (where);
}



ACMD(do_wear)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj;
  int where, dotmode, items_worn = 0;

  two_arguments(argument, arg1, arg2);

  if (!*arg1)
     {send_to_char("Что Вы собрались одеть ?\r\n", ch);
      return;
     }
  dotmode = find_all_dots(arg1);

  if (*arg2 && (dotmode != FIND_INDIV))
     {send_to_char("И на какую часть тела Вы желаете это одеть !\r\n", ch);
      return;
     }
  if (dotmode == FIND_ALL)
     {for (obj = ch->carrying; obj && !GET_MOB_HOLD(ch) && GET_POS(ch) > POS_SLEEPING; obj = next_obj)
          {next_obj = obj->next_content;
           if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0)
              {items_worn++;
	       perform_wear(ch, obj, where);
              }
          }
      if (!items_worn)
         send_to_char("Увы, но одеть Вам нечего.\r\n", ch);
     }
  else
  if (dotmode == FIND_ALLDOT)
     {if (!*arg1)
         {send_to_char("Надеть \"все\" чего ?\r\n", ch);
          return;
         }
      if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
         {sprintf(buf, "У Вас нет ничего похожего на '%s'.\r\n", arg1);
          send_to_char(buf, ch);
         }
      else
         while (obj && !GET_MOB_HOLD(ch) && GET_POS(ch) > POS_SLEEPING)
               {next_obj = get_obj_in_list_vis(ch, arg1, obj->next_content);
	        if ((where = find_eq_pos(ch, obj, 0)) >= 0)
	           perform_wear(ch, obj, where);
	        else
	           act("Вы не можете одеть $o3.", FALSE, ch, obj, 0, TO_CHAR);
	        obj = next_obj;
               }
    }
  else
     {if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
         {sprintf(buf, "У Вас нет ничего похожего на '%s'.\r\n", arg1);
          send_to_char(buf, ch);
         }
      else
         {if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
	     perform_wear(ch, obj, where);
          else
             if (!*arg2)
	        act("Вы не можете одеть $o3.", FALSE, ch, obj, 0, TO_CHAR);
         }
     }
}


ACMD(do_wield)
{
  struct obj_data *obj;
  int    wear;

  if (IS_NPC(ch) && !NPC_FLAGGED(ch, NPC_WIELDING))
     return;

  one_argument(argument, arg);

  if (!*arg)
     send_to_char("Вооружиться чем ?\r\n", ch);
  else
     if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
        {sprintf(buf, "Вы не видите ничего похожего на \'%s\'.\r\n", arg);
         send_to_char(buf, ch);
        }
     else
        {if (!CAN_WEAR(obj, ITEM_WEAR_WIELD) && !CAN_WEAR(obj, ITEM_WEAR_BOTHS))
            send_to_char("Вы не можете вооружиться этим.\r\n", ch);
         else
         if (GET_OBJ_TYPE(obj) != ITEM_WEAPON)
            send_to_char("Это не оружие.\r\n",ch);
         else
            { if (CAN_WEAR(obj, ITEM_WEAR_WIELD))
                 wear = WEAR_WIELD;
              else
                 wear = WEAR_BOTHS;
              /* This is too high
              if (GET_OBJ_SKILL(obj) == SKILL_BOTHHANDS ||
                  GET_OBJ_SKILL(obj) == SKILL_BOWS)
                 wear = WEAR_BOTHS;
               */
              if (wear == WEAR_WIELD && !IS_IMMORTAL(ch) && !OK_WIELD(ch,obj))
                 {act("Вам слишком тяжело держать $o3 в правой руке.",
                      FALSE,ch,obj,0,TO_CHAR);
                  if (CAN_WEAR(obj, ITEM_WEAR_BOTHS))
                     wear = WEAR_BOTHS;
                  else
                     return;
                 }
              if (wear == WEAR_BOTHS && !IS_IMMORTAL(ch) && !OK_BOTH(ch,obj))
                 {act("Вам слишком тяжело держать $o3 двумя руками.",
                      FALSE,ch,obj,0,TO_CHAR);
                  return;
                 };
              perform_wear(ch, obj, wear);
             }
        }
}

ACMD(do_grab)
{ int  where = WEAR_HOLD;
  struct obj_data *obj;
  one_argument(argument, arg);

  if (IS_NPC(ch) && !NPC_FLAGGED(ch, NPC_WIELDING))
     return;

  if (!*arg)
     send_to_char("Вы заорали : 'Держи его !!! Хватай его !!!'\r\n", ch);
  else
  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {sprintf(buf, "У Вас нет ничего похожего на '%s'.\r\n", arg);
      send_to_char(buf, ch);
     }
  else
     {if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
         perform_wear(ch, obj, WEAR_LIGHT);
      else
         {if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) &&
              GET_OBJ_TYPE(obj) != ITEM_WAND &&
              GET_OBJ_TYPE(obj) != ITEM_STAFF &&
              GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
              GET_OBJ_TYPE(obj) != ITEM_POTION
             )
	     {send_to_char("Вы не можете это держать.\r\n", ch);
	      return;
	     }
	  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
             {if (GET_OBJ_SKILL(obj) == SKILL_BOTHHANDS ||
                  GET_OBJ_SKILL(obj) == SKILL_BOWS
                 )
                 {send_to_char("Данный тип оружия держать невозможно.", ch);
                  return;
                 }
             }
          if (!IS_IMMORTAL(ch) && !OK_HELD(ch,obj))
             {act("Вам слишком тяжело держать $o3 в левой руке.",
                   FALSE,ch,obj,0,TO_CHAR);
              if (CAN_WEAR(obj, ITEM_WEAR_BOTHS))
                 {if (!OK_BOTH(ch, obj))
                     {act("Вам слишком тяжело держать $o3 двумя руками.",
                          FALSE, ch, obj, 0, TO_CHAR);
                      return;
                     }
                  else
                     where = WEAR_BOTHS;
                 }
              else
                 return;
             }
          perform_wear(ch, obj, where);
	 }
     }
}



void perform_remove(struct char_data * ch, int pos)
{
  struct obj_data *obj;

  if (!(obj = GET_EQ(ch, pos)))
    log("SYSERR: perform_remove: bad pos %d passed.", pos);
  else
  /*
  if (IS_OBJ_STAT(obj, ITEM_NODROP))
     act("Вы не можете снять $o3!", FALSE, ch, obj, 0, TO_CHAR);
  else
   */
  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
     act("$p: Вы не можете нести столько вещей!", FALSE, ch, obj, 0, TO_CHAR);
  else
     {if (!remove_otrigger(obj, ch))
         return;
      obj_to_char(unequip_char(ch, pos), ch);
      act("Вы прекратили использовать $o3.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n прекратил$g использовать $o3.", TRUE, ch, obj, 0, TO_ROOM);
     }
}



ACMD(do_remove)
{ int i, dotmode, found;

  one_argument(argument, arg);

  if (!*arg)
     {send_to_char("Снять что ?\r\n", ch);
      return;
     }
  dotmode = find_all_dots(arg);

  if (dotmode == FIND_ALL)
     {found = 0;
      for (i = 0; i < NUM_WEARS; i++)
          if (GET_EQ(ch, i))
             {perform_remove(ch, i);
	          found = 1;
             }
      if (!found)
         send_to_char("На Вас не одето предметов этого типа.\r\n", ch);
     }
  else
  if (dotmode == FIND_ALLDOT)
     {if (!*arg)
         send_to_char("Снять все вещи какого типа ?\r\n", ch);
      else
         {found = 0;
          for (i = 0; i < NUM_WEARS; i++)
	          if (GET_EQ(ch, i) && CAN_SEE_OBJ(ch, GET_EQ(ch, i)) &&
	              isname(arg, GET_EQ(ch, i)->name))
	             {perform_remove(ch, i);
	              found = 1;
	             }
          if (!found)
             {sprintf(buf, "Вы не используете ни одного '%s'.\r\n", arg);
	          send_to_char(buf, ch);
             }
         }
     }
  else
     { /* Returns object pointer but we don't need it, just true/false. */
      if (!get_object_in_equip_vis(ch, arg, ch->equipment, &i))
         {sprintf(buf, "Вы не используете '%s'.\r\n", arg);
          send_to_char(buf, ch);
         }
      else
         perform_remove(ch, i);
     }
}


ACMD(do_upgrade)
{
  struct obj_data *obj;
  int    weight, add_hr, add_dr, prob, percent, i;

  if (!GET_SKILL(ch, SKILL_UPGRADE))
     {send_to_char("Вы не умеете этого.",ch);
      return;
     }

  one_argument(argument, arg);

  if (!*arg)
     send_to_char("Что вы хотите заточить ?\r\n", ch);

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {sprintf(buf, "У Вас нет \'%s\'.\r\n", arg);
      send_to_char(buf, ch);
      return;
     };

  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON)
     {send_to_char("Вы можете заточить только оружие.\r\n", ch);
      return;
     }

  if (GET_OBJ_SKILL(obj) == SKILL_BOWS)
     {send_to_char("Невозможно заточить этот тип оружия.\r\n", ch);
      return;
     }

 if (OBJ_FLAGGED(obj, ITEM_MAGIC) || OBJ_FLAGGED(obj, ITEM_SHARPEN))
    {send_to_char("Вы не можете заточить этот предмет.\r\n", ch);
     return;
    }

  /* Make sure no other affections. */
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != APPLY_NONE)
         {send_to_char("Этот предмет не может быть заточен.\r\n", ch);
          return;
         }


  switch (obj->obj_flags.Obj_mater)
{
case MAT_BRONZE:
case MAT_BULAT:
case MAT_IRON:
case MAT_STEEL:
case MAT_SWORDSSTEEL:
case MAT_COLOR:
case MAT_BONE:
     act("Вы взялись точить $o3.", FALSE, ch, obj, 0, TO_CHAR);
     act("$n взял$u точить $o3.", FALSE, ch, obj, 0, TO_ROOM);
     weight = -1;
     break;
case MAT_WOOD:
case MAT_SUPERWOOD:
     act("Вы взялись стругать $o3.", FALSE, ch, obj, 0, TO_CHAR);
     act("$n взял$u стругать $o3.", FALSE, ch, obj, 0, TO_ROOM);
     weight = -1;
     break;
case MAT_SKIN:
     act("Вы взялись проклепывать $o3.", FALSE, ch, obj, 0, TO_CHAR);
     act("$n взял$u проклепывать $o3.", FALSE, ch, obj, 0, TO_ROOM);
     weight = +1;
     break;
default:
     sprintf(buf, "К сожалению, %s сделан из неподходящего материала.\r\n",
                  OBJN(obj,ch,0));
     send_to_char(buf,ch);
     return;
}

SET_BIT(GET_OBJ_EXTRA(obj, ITEM_SHARPEN), ITEM_SHARPEN);
percent = number(1,skill_info[SKILL_UPGRADE].max_percent);
prob    = train_skill(ch,SKILL_UPGRADE,skill_info[SKILL_UPGRADE].max_percent,0);

add_hr  = IS_IMMORTAL(ch) ? 10 : number(1,(GET_LEVEL(ch) + 5) / 6);
add_dr  = IS_IMMORTAL(ch) ? 5  : number(1,(GET_LEVEL(ch) + 5) / 6);
if (percent > prob || GET_GOD_FLAG(ch, GF_GODSCURSE))
   {act("Но только загубили $S.",FALSE,ch,obj,0,TO_CHAR);
    add_hr = -add_hr;
    add_dr = -add_dr;
   }

obj->affected[0].location = APPLY_HITROLL;
obj->affected[0].modifier = add_hr;

obj->affected[1].location = APPLY_DAMROLL;
obj->affected[1].modifier = add_dr;
obj->obj_flags.weight    += weight;
//obj->obj_flags.Obj_owner  = GET_UNIQUE(ch);
}


ACMD(do_armored)
{
  struct obj_data *obj;
  int    add_ac, add_armor, prob, percent, i, k_mul = 1, k_div = 1;

  if (!GET_SKILL(ch, SKILL_ARMORED))
     {send_to_char("Вы не умеете этого.",ch);
      return;
     }

  one_argument(argument, arg);

  if (!*arg)
     send_to_char("Что вы хотите укрепить ?\r\n", ch);

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {sprintf(buf, "У Вас нет \'%s\'.\r\n", arg);
      send_to_char(buf, ch);
      return;
     };

  if (GET_OBJ_TYPE(obj) != ITEM_ARMOR)
     {send_to_char("Вы можете укрепить только доспех.\r\n", ch);
      return;
     }

 if (OBJ_FLAGGED(obj, ITEM_MAGIC) || OBJ_FLAGGED(obj, ITEM_ARMORED))
    {send_to_char("Вы не можете укрепить этот предмет.\r\n", ch);
     return;
    }

  /* Make sure no other affections. */
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != APPLY_NONE)
         {send_to_char("Этот предмет не может быть укреплен.\r\n", ch);
          return;
         }

  if (OBJWEAR_FLAGGED(obj, (ITEM_WEAR_BODY | ITEM_WEAR_ABOUT)))
     {k_mul = 1;
      k_div = 1;
     }
  else
  if (OBJWEAR_FLAGGED(obj, (ITEM_WEAR_SHIELD | ITEM_WEAR_HEAD | ITEM_WEAR_ARMS | ITEM_WEAR_LEGS)))
     {k_mul = 2;
      k_div = 3;
     }
  else
  if (OBJWEAR_FLAGGED(obj, (ITEM_WEAR_HANDS | ITEM_WEAR_FEET)))
     {k_mul = 1;
      k_div = 2;
     }
  else
     {act("$o3 невозможно укрепить.",FALSE,ch,obj,0,TO_CHAR);
      return;
     }

  switch (obj->obj_flags.Obj_mater)
{
case MAT_IRON:
case MAT_STEEL:
     act("Вы принялись закалять $o3.", FALSE, ch, obj, 0, TO_CHAR);
     act("$n принял$u закалять $o3.", FALSE, ch, obj, 0, TO_ROOM);
     break;
case MAT_WOOD:
case MAT_SUPERWOOD:
     act("Вы принялись обшивать $o3 железом.", FALSE, ch, obj, 0, TO_CHAR);
     act("$n принял$u обшивать $o3 железом.", FALSE, ch, obj, 0, TO_ROOM);
     break;
case MAT_SKIN:
     act("Вы принялись проклепывать $o3.", FALSE, ch, obj, 0, TO_CHAR);
     act("$n принял$u проклепывать $o3.", FALSE, ch, obj, 0, TO_ROOM);
     break;
default:
     sprintf(buf, "К сожалению, %s сделан из неподходящего материала.\r\n",
                  OBJN(obj,ch,0));
     send_to_char(buf,ch);
     return;
}

SET_BIT(GET_OBJ_EXTRA(obj, ITEM_ARMORED), ITEM_ARMORED);
percent    = number(1,skill_info[SKILL_ARMORED].max_percent);
prob       = train_skill(ch,SKILL_ARMORED,skill_info[SKILL_ARMORED].max_percent,0);

add_ac     = IS_IMMORTAL(ch) ? -20 : -number(1,(GET_LEVEL(ch) + 4) / 5);
add_armor  = IS_IMMORTAL(ch) ?  5  :  number(1,(GET_LEVEL(ch) + 4) / 5);

if (percent > prob || GET_GOD_FLAG(ch, GF_GODSCURSE))
   {act("Но только испортили $S.",FALSE,ch,obj,0,TO_CHAR);
    add_ac    = -add_ac;
    add_armor = -add_armor;
   }
else
   {add_ac    = MIN(-1, add_ac * k_mul / k_div);
    add_armor = MAX(1, add_armor * k_mul / k_div);
   };

obj->affected[0].location = APPLY_AC;
obj->affected[0].modifier = add_ac;

obj->affected[1].location = APPLY_ARMOUR;
obj->affected[1].modifier = add_armor;

//obj->obj_flags.Obj_owner = GET_UNIQUE(ch);
}

ACMD(do_fire)
{int percent, prob;
 if (!GET_SKILL(ch, SKILL_FIRE))
    {send_to_char("Но Вы не знаете как.\r\n", ch);
     return;
    }

 if (on_horse(ch))
    {send_to_char("Верхом это будет затруднительно.\r\n",ch);
     return;
    }

 if (AFF_FLAGGED(ch, AFF_BLIND))
    {send_to_char("Вы ничего не видите !\r\n",ch);
     return;
    }


 if (world[IN_ROOM(ch)].fires)
    {send_to_char("Здесь уже горит огонь.\r\n", ch);
     return;
    }

 if (SECT(IN_ROOM(ch)) == SECT_INSIDE ||
     SECT(IN_ROOM(ch)) == SECT_CITY   ||
     SECT(IN_ROOM(ch)) == SECT_WATER_SWIM   ||
     SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM ||
     SECT(IN_ROOM(ch)) == SECT_FLYING       ||
     SECT(IN_ROOM(ch)) == SECT_UNDERWATER   ||
     SECT(IN_ROOM(ch)) == SECT_SECRET)
    {send_to_char("В этой комнате нельзя разжечь костер.\r\n", ch);
     return;
    }

 if (!check_moves(ch, FIRE_MOVES))
    return;

 percent = number(1,skill_info[SKILL_FIRE].max_percent);
 prob    = calculate_skill(ch, SKILL_FIRE, skill_info[SKILL_FIRE].max_percent, 0);
 if (percent > prob)
    {send_to_char("Вы попытались разжечь костер, но у Вас ничего не вышло.\r\n", ch);
     return;
    }
 else
    {world[IN_ROOM(ch)].fires = MAX(0, (prob - percent) / 5) + 1;
     send_to_char("Вы набрали хворосту и разожгли огонь.\n\r", ch);
     act("$n развел$g огонь.", FALSE, ch, 0, 0, TO_ROOM);
     improove_skill(ch, SKILL_FIRE, TRUE, 0);
    }
}

#define MAX_REMOVE  10
const int RemoveSpell[MAX_REMOVE] =
{SPELL_SLEEP,SPELL_POISON, SPELL_WEAKNESS, SPELL_CURSE, SPELL_PLAQUE,
 SPELL_SIELENCE, SPELL_BLINDNESS, SPELL_HAEMORRAGIA, SPELL_HOLD, 0};

ACMD(do_firstaid)
{int    percent, prob, success=FALSE, need=FALSE, count, spellnum = 0;
 struct timed_type timed;
 struct char_data *vict;

 if (!GET_SKILL(ch, SKILL_AID))
    {send_to_char("Вам следует этому научиться.\r\n", ch);
     return;
    }
 if (!IS_GOD(ch) && timed_by_skill(ch, SKILL_AID))
    {send_to_char("Так много лечить нельзя - больных не останется.\r\n", ch);
     return;
    }

 one_argument(argument, arg);

 if (!*arg)
    vict = ch;
 else
 if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
    {send_to_char("Кого Вы хотите подлечить ?\r\n", ch);
     return;
    };

 if (FIGHTING(vict))
    {act("$N сражается, $M не до Ваших телячьих нежностей.",FALSE,ch,0,vict,TO_CHAR);
     return;
    }

 percent = number(1,skill_info[SKILL_AID].max_percent);
 prob    = calculate_skill(ch,SKILL_AID,skill_info[SKILL_AID].max_percent,vict);

 if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE) ||
                        GET_GOD_FLAG(vict, GF_GODSLIKE))
    percent = prob;
 if (GET_GOD_FLAG(vict, GF_GODSCURSE) || GET_GOD_FLAG(vict, GF_GODSCURSE))
    prob = 0;
 success = (prob >= percent);
 need    = FALSE;

 if (GET_REAL_MAX_HIT(vict) &&
     (GET_HIT(vict) * 100 / GET_REAL_MAX_HIT(vict)) < 31)
    {need     = TRUE;
     if (success)
        {int dif        = GET_REAL_MAX_HIT(vict) - GET_HIT(vict);
	 int add        = MIN(dif, (dif * (prob - percent) / 100) + 1);
	 GET_HIT(vict) += add;
         update_pos(vict);
        }
    }
 count = MIN(MAX_REMOVE, MAX_REMOVE * prob / 100);

 for (percent = 0, prob = need;
      !need && percent < MAX_REMOVE && RemoveSpell[percent]; percent++
     )
     if (affected_by_spell(vict, RemoveSpell[percent]))
        {need     = TRUE;
         if (percent < count)
            {spellnum = RemoveSpell[percent];
             prob     = TRUE;
            }
        }


 if (!need)
    act("$N в лечении не нуждается.",FALSE,ch,0,vict,TO_CHAR);
 else
 if (!prob)
    act("У Вас не хватит умения вылечить $N3.",FALSE,ch,0,vict,TO_CHAR);
 else
    {improove_skill(ch, SKILL_AID, TRUE, 0);
     timed.skill = SKILL_AID;
     timed.time  = IS_IMMORTAL(ch) ? 2 : 6;
     timed_to_char(ch, &timed);
     if (vict != ch)
        {if (success)
            {act("Вы оказали первую помощь $N2.", FALSE, ch, 0, vict, TO_CHAR);
             act("$N оказал$G Вам первую помощь.", FALSE, vict, 0, ch, TO_CHAR);
             act("$n оказал$g первую помощь $N2.", TRUE, ch, 0, vict, TO_NOTVICT);
             if (spellnum)
                affect_from_char(vict,spellnum);
            }
         else
            {act("Вы безрезультатно пытались оказать первую помощь $N2.", FALSE, ch, 0, vict, TO_CHAR);
             act("$N безрезультатно пытал$U оказать Вам первую помощь.", FALSE, vict, 0, ch, TO_CHAR);
             act("$n безрезультатно пытал$u оказать первую помощь $N2.", TRUE, ch, 0, vict, TO_NOTVICT);
            }
        }
     else
        {if (success)
            {act("Вы оказали себе первую помощь.", FALSE, ch, 0, 0, TO_CHAR);
             act("$n оказал$g себе первую помощь.",FALSE, ch, 0, 0, TO_ROOM);
             if (spellnum)
                affect_from_char(vict,spellnum);
            }
         else
            {act("Вы безрезультатно пытались оказать себе первую помощь.", FALSE, ch, 0, vict, TO_CHAR);
             act("$n безрезультатно пытал$u оказать себе первую помощь.", FALSE, ch, 0, vict, TO_ROOM);
            }
        }
    }
}

#define MAX_POISON_TIME 12

ACMD(do_poisoned)
{
  struct obj_data   *obj;
  struct timed_type timed;
  int    i, apply_pos = MAX_OBJ_AFFECT;

  if (!GET_SKILL(ch, SKILL_POISONED))
     {send_to_char("Вы не умеете этого.",ch);
      return;
     }

  one_argument(argument, arg);

  if (!*arg)
     {send_to_char("Что вы хотите отравить ?\r\n", ch);
      return;
     }

  if (!IS_IMMORTAL(ch) && timed_by_skill(ch, SKILL_POISONED))
     {send_to_char("Вы рискуете отравиться сами, подождите немного.\r\n", ch);
      return;
     }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {sprintf(buf, "У Вас нет \'%s\'.\r\n", arg);
      send_to_char(buf, ch);
      return;
     };

  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON)
     {send_to_char("Вы можете нанести яд только на оружие.\r\n", ch);
      return;
     }

  /* Make sure no other affections. */
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location == APPLY_POISON)
         {send_to_char("На этот предмет уже нанесен яд.\r\n", ch);
          return;
         }
      else
      if (obj->affected[i].location == APPLY_NONE && apply_pos == MAX_OBJ_AFFECT)
         apply_pos = i;

  if (apply_pos >= MAX_OBJ_AFFECT)
     {send_to_char("Вы не можете нанести яд на этот предмет.\r\n",ch);
      return;
     }

  obj->affected[apply_pos].location = APPLY_POISON;
  obj->affected[apply_pos].modifier = MAX_POISON_TIME;

  timed.skill     = SKILL_POISONED;
  timed.time      = MAX_POISON_TIME;
  timed_to_char(ch,&timed);

  act("Вы осторожно нанесли яд на $o3.", FALSE, ch, obj, 0, TO_CHAR);
  act("$n осторожно нанес$q яд на $o3.", FALSE, ch, obj, 0, TO_ROOM);
}

ACMD(do_repair)
{
  struct obj_data   *obj;
  int    prob, percent=0, decay;

  if (!GET_SKILL(ch, SKILL_REPAIR))
     {send_to_char("Вы не умеете этого.\r\n",ch);
      return;
     }

  one_argument(argument, arg);

  if (!*arg)
     {send_to_char("Что Вы хотите ремонтировать ?\r\n", ch);
      return;
     }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {sprintf(buf, "У Вас нет \'%s\'.\r\n", arg);
      send_to_char(buf, ch);
      return;
     };

  if (GET_OBJ_MAX(obj) <= GET_OBJ_CUR(obj))
     {act("$o в ремонте не нуждается.",FALSE,ch,obj,0,TO_CHAR);
      return;
     }

  decay   = (GET_OBJ_MAX(obj) - GET_OBJ_CUR(obj)) / 5;
  decay   = MAX(1, MIN(decay,GET_OBJ_MAX(obj) / 10));
  if (GET_OBJ_MAX(obj) > decay)
     GET_OBJ_MAX(obj) -= decay;
  else
     GET_OBJ_MAX(obj) = 1;
  prob    = number(1,skill_info[SKILL_REPAIR].max_percent);
  percent = train_skill(ch,SKILL_REPAIR,skill_info[SKILL_REPAIR].max_percent,0);
  if (prob > percent)
     {GET_OBJ_CUR(obj) = MAX(0, GET_OBJ_CUR(obj) * percent / prob);
      if (obj->obj_flags.Obj_cur)
         {act("Вы попытались починить $o3, но сломали $S еще больше.",FALSE,ch,obj,0,TO_CHAR);
          act("$n попытал$u починить $o3, но сломал$g $S еще больше.",FALSE,ch,obj,0,TO_ROOM);
         }
      else
         {act("Вы окончательно доломали $o3.",FALSE,ch,obj,0,TO_CHAR);
          act("$n окончательно доломал$g $o3.",FALSE,ch,obj,0,TO_ROOM);
          extract_obj(obj);
         }
     }
  else
     {GET_OBJ_CUR(obj) = MIN(GET_OBJ_MAX(obj),
                             GET_OBJ_CUR(obj) * percent / prob + 1);
      act("Теперь $o0 выглядит лучше.",FALSE,ch,obj,0,TO_CHAR);
      act("$n умело починил$g $o3.",FALSE,ch,obj,0,TO_ROOM);
     }
}

const int meet_vnum[] = {320,321,322,323};

ACMD(do_makefood)
{ struct obj_data  *obj,*tobj;
  struct char_data *mob;
  int    prob, percent=0, mobn, wgt=0;

  if (!GET_SKILL(ch,SKILL_MAKEFOOD))
     {send_to_char("Вы не умеете этого.\r\n",ch);
      return;
     }

  one_argument(argument,arg);
  if (!*arg)
     {send_to_char("Что Вы хотите освежевать ?\r\n",ch);
      return;
     }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)) &&
      !(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents)))
     {sprintf(buf,"Вы не видите здесь '%s'.\r\n",arg);
      send_to_char(buf,ch);
      return;
     }

  if (!IS_CORPSE(obj) || (mobn = GET_OBJ_VAL(obj,2)) < 0)
     {act("Вы не сможете освежевать $o3.",FALSE,ch,obj,0,TO_CHAR);
      return;
     }
  mob = (mob_proto + real_mobile(mobn));
  if (!IS_IMMORTAL(ch) &&
      !(GET_CLASS(mob) == CLASS_ANIMAL) &&
      !(GET_CLASS(mob) == CLASS_BASIC_NPC) &&
      (wgt = GET_WEIGHT(mob)) < 180)
     {send_to_char("Этот труп невозможно освежевать.",ch);
      return;
     }
  prob    = number(1,skill_info[SKILL_MAKEFOOD].max_percent);
  percent = train_skill(ch,SKILL_MAKEFOOD,skill_info[SKILL_MAKEFOOD].max_percent,mob);
  if (prob > percent ||
      !(tobj = read_object(meet_vnum[number(0,MIN(3,MAX(0,(wgt-180)/5)))],VIRTUAL)))
     {act("Вы не сумели освежевать $o3.",FALSE,ch,obj,0,TO_CHAR);
      act("$n попытал$u освежевать $o3, но неудачно.",FALSE,ch,obj,0,TO_ROOM);
     }
  else
     {sprintf(buf,"$n умело вырезал$g %s из $o1.",tobj->PNames[3]);
      act(buf,FALSE,ch,obj,0,TO_ROOM);
      sprintf(buf,"Вы умело вырезали %s из $o1.",tobj->PNames[3]);
      act(buf,FALSE,ch,obj,0,TO_CHAR);
      if (obj->carried_by == ch)
         {if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
             {send_to_char("Вы не можете нести столько предметов.", ch);
              obj_to_room(tobj,IN_ROOM(ch));
             }
          else
          if (GET_OBJ_WEIGHT(tobj) + IS_CARRYING_W(ch) > CAN_CARRY_W(ch))
             {sprintf(buf,"Вас слишком тяжело нести еще и %s.",tobj->PNames[3]);
              send_to_char(buf,ch);
              obj_to_room(tobj,IN_ROOM(ch));
             }
          else
             obj_to_char(tobj,ch);
         }
      else
         obj_to_room(tobj,IN_ROOM(ch));
     }
  if (obj->carried_by)
     {obj_from_char(obj);
      obj_to_room(obj, IN_ROOM(ch));
     }
  extract_obj(obj);
}


#define MAX_ITEMS 9
#define MAX_PROTO 3
#define COAL_PROTO 	311
#define WOOD_PROTO      313
#define TETIVA_PROTO    314
#define ONE_DAY    	24*60
#define SEVEN_DAYS	7*24*60

struct create_item_type
{int obj_vnum;
 int material_bits;
 int min_weight;
 int max_weight;
 int proto[MAX_PROTO];
 int skill;
 int wear;
};

struct create_item_type created_item [] =
{ {300,0x7E,15,40,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {301,0x7E,12,40,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {302,0x7E,8 ,25,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {303,0x7E,5 ,12,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {304,0x7E,10,35,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {305,0   ,8 ,15,{0,0,0},0,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {306,0   ,8 ,20,{0,0,0},0,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {307,0x3A,10,20,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_BODY)},
  {308,0x3A,4,10,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_ARMS)},
  {309,0x3A,6,12,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_LEGS)},
  {310,0x3A,4,10,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_HEAD)},
  {312,0,4,40,   {WOOD_PROTO,TETIVA_PROTO,0},SKILL_CREATEBOW,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS)}
};

const char *create_item_name  [] =
{"шелепуга",
 "меч",
 "сабля",
 "нож",
 "топор",
 "плеть",
 "дубина",
 "кольчуга",
 "наручи",
 "поножи",
 "шлем",
 "лук",
 "\n"
};

void go_create_weapon(struct char_data *ch, struct obj_data *obj, int obj_type, int skill)
{struct obj_data *tobj;
 char   *to_char=NULL, *to_room=NULL;
 int    prob,percent,ndice,sdice,weight;

 if (obj_type == 5 ||
     obj_type == 6
    )
    {weight  = number(created_item[obj_type].min_weight, created_item[obj_type].max_weight);
     percent = 100;
     prob    = 100;
    }
 else
    {if (!obj)
        return;
     skill = created_item[obj_type].skill;
     percent= number(1,skill_info[skill].max_percent);
     prob   = train_skill(ch,skill,skill_info[skill].max_percent,0);
     weight = MIN(GET_OBJ_WEIGHT(obj) - 2, GET_OBJ_WEIGHT(obj) * prob / percent);
    }

 if (weight < created_item[obj_type].min_weight)
    send_to_char("У Вас не хватило материала.\r\n",ch);
 else
 if (prob * 5 < percent)
    send_to_char("У Вас ничего не получилось.\r\n",ch);
 else
 if (!(tobj = read_object(created_item[obj_type].obj_vnum,VIRTUAL)))
    send_to_char("Образец был невозвратимо утерян.\r\n",ch);
 else
    {GET_OBJ_WEIGHT(tobj)      = MIN(weight, created_item[obj_type].max_weight);
     tobj->obj_flags.Obj_owner = GET_UNIQUE(ch);
     switch (obj_type)
     {case 0: /* smith weapon */
      case 1:
      case 2:
      case 3:
      case 4:
      case 11:
        GET_OBJ_TIMER(tobj) = MAX(ONE_DAY, MIN(SEVEN_DAYS, SEVEN_DAYS * prob / percent));
        GET_OBJ_MATER(tobj) = GET_OBJ_MATER(obj);
        GET_OBJ_MAX(tobj)   = MAX(50, MIN(300, 300 * prob / percent));
        GET_OBJ_CUR(tobj)   = GET_OBJ_MAX(tobj);
        percent= number(1,skill_info[skill].max_percent);
        prob   = calculate_skill(ch,skill,skill_info[skill].max_percent,0);
        ndice  = MAX(2,MIN(4,prob/percent));
        ndice += GET_OBJ_WEIGHT(tobj) / 15;
        percent= number(1,skill_info[skill].max_percent);
        prob   = calculate_skill(ch,skill,skill_info[skill].max_percent,0);
        sdice  = MAX(2,MIN(4,prob/percent));
        sdice += GET_OBJ_WEIGHT(tobj) / 15;
        GET_OBJ_VAL(tobj,1) = ndice;
        GET_OBJ_VAL(tobj,2) = sdice;
        SET_BIT(tobj->obj_flags.wear_flags,created_item[obj_type].wear);
        if (skill != SKILL_CREATEBOW)
           {if (GET_OBJ_WEIGHT(tobj) < 14 && percent * 4 > prob)
               SET_BIT(tobj->obj_flags.wear_flags,ITEM_WEAR_HOLD);
            to_room = "$n выковал$g $o3.";
            to_char = "Вы выковали $o3.";
           }
        else
           {to_room = "$n смастерил$g $o3.";
            to_char = "Вы смастерили $o3.";
           }
        break;
      case 5: /* mages weapon */
      case 6:
        GET_OBJ_TIMER(tobj) = ONE_DAY;
        GET_OBJ_MAX(tobj)   = 50;
        GET_OBJ_CUR(tobj)   = 50;
        ndice  = MAX(2,MIN(4,GET_LEVEL(ch)/number(6,8)));
        ndice += (GET_OBJ_WEIGHT(tobj) / 15);
        sdice  = MAX(2,MIN(5,GET_LEVEL(ch)/number(4,5)));
        sdice += (GET_OBJ_WEIGHT(tobj) / 15);
        GET_OBJ_VAL(tobj,1) = ndice;
        GET_OBJ_VAL(tobj,2) = sdice;
        SET_BIT(GET_OBJ_EXTRA(tobj, ITEM_NORENT), ITEM_NORENT);
        SET_BIT(GET_OBJ_EXTRA(tobj, ITEM_DECAY),  ITEM_DECAY);
        SET_BIT(GET_OBJ_EXTRA(tobj, ITEM_NOSELL), ITEM_NOSELL);
        SET_BIT(tobj->obj_flags.wear_flags,created_item[obj_type].wear);
        to_room = "$n создал$g $o3.";
        to_char = "Вы создали $o3.";
        break;
     case 7: /* smith armor */
     case 8:
     case 9:
     case 10:
        GET_OBJ_TIMER(tobj) = MAX(ONE_DAY, MIN(SEVEN_DAYS, SEVEN_DAYS *  prob / percent));
        GET_OBJ_MATER(tobj) = GET_OBJ_MATER(obj);
        GET_OBJ_MAX(tobj)   = MAX(50, MIN(300, 300 * prob / percent));
        GET_OBJ_CUR(tobj)   = GET_OBJ_MAX(tobj);
        percent= number(1,skill_info[skill].max_percent);
        prob   = calculate_skill(ch,skill,skill_info[skill].max_percent,0);
        ndice  = MAX(2,MIN((105-material_value[GET_OBJ_MATER(tobj)])/10,prob / percent));
        percent= number(1,skill_info[skill].max_percent);
        prob   = calculate_skill(ch,skill,skill_info[skill].max_percent,0);
        sdice  = MAX(1,MIN((105-material_value[GET_OBJ_MATER(tobj)])/15,prob / percent));
        GET_OBJ_VAL(tobj,1) = ndice;
        GET_OBJ_VAL(tobj,2) = sdice;
        SET_BIT(tobj->obj_flags.wear_flags,created_item[obj_type].wear);
        to_room = "$n выковал$g $o3.";
        to_char = "Вы выковали $o3.";
        break;
       }
       if (to_char)
          act(to_char,FALSE,ch,tobj,0,TO_CHAR);
       if (to_room)
          act(to_room,FALSE,ch,tobj,0,TO_ROOM);

       if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
          {send_to_char("Вы не сможете унести столько предметов.\r\n", ch);
           obj_to_room(tobj, IN_ROOM(ch));
          }
       else
       if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(tobj) > CAN_CARRY_W(ch))
          {send_to_char("Вы не сможете унести такой вес.\r\n", ch);
           obj_to_room(tobj, IN_ROOM(ch));
          }
       else
           obj_to_char(tobj,ch);
    }
 if (obj)
    {obj_from_char(obj);
     extract_obj(obj);
    }
}


ACMD(do_transform_weapon)
{ char   arg1[MAX_INPUT_LENGTH];
  char   arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj=NULL, *coal, *tmp_list[MAX_ITEMS+1], *proto[MAX_PROTO];
  int    obj_type,i,count=0,found,rnum;

  if (IS_NPC(ch) || !GET_SKILL(ch, subcmd))
     {send_to_char("Вас этому никто не научил.\r\n",ch);
      return;
     }

  argument = one_argument(argument, arg1);
  if (!*arg1)
     {switch (subcmd)
      {case SKILL_TRANSFORMWEAPON:
            send_to_char("Во что Вы хотите перековать ?\r\n",ch);
            break;
       case SKILL_CREATEBOW:
            send_to_char("Что Вы хотите смастерить ?\r\n",ch);
            break;
      }
      return;
     }
  if ((obj_type = search_block(arg1, create_item_name, FALSE)) == -1)
     {switch (subcmd)
      {case SKILL_TRANSFORMWEAPON:
            send_to_char("Перековать можно в :\r\n",ch);
            break;
       case SKILL_CREATEBOW:
            send_to_char("Смастерить можно :\r\n",ch);
            break;
      }
      for (obj_type = 0; *create_item_name[obj_type] != '\n'; obj_type++)
          if (created_item[obj_type].skill == subcmd)
             {sprintf(buf,"- %s\r\n",create_item_name[obj_type]);
              send_to_char(buf,ch);
             }
      return;
     }
  if (created_item[obj_type].skill != subcmd)
     {switch (subcmd)
      {case SKILL_TRANSFORMWEAPON :
            send_to_char("Данный предмет выковать нельзя.\r\n",ch);
            break;
       case SKILL_CREATEBOW:
            send_to_char("Данный предмет смастерить нельзя.\r\n",ch);
            break;
      }
      return;
     }

  for (i=0;i<MAX_ITEMS;tmp_list[i++] = NULL);
  for (i=0;i<MAX_PROTO;proto[i++] = NULL);

  do {argument = one_argument(argument, arg2);
      if (!*arg2)
         break;
      if (!(obj = get_obj_in_list_vis(ch,arg2,ch->carrying)))
         {sprintf(buf,"У Вас нет '%s'.\r\n",arg2);
          send_to_char(buf,ch);
         }
      else
      if (obj->contains)
         act("В $o5 что-то лежит.",FALSE,ch,obj,0,TO_CHAR);
      else
      if (GET_OBJ_TYPE(obj) == ITEM_INGRADIENT)
         {for (i = 0, found = FALSE; i < MAX_PROTO; i++)
              if (GET_OBJ_VAL(obj,1) == created_item[obj_type].proto[i])
                 {if (proto[i])
                      found = TRUE;
                  else
                     {proto[i] = obj;
                      found    = FALSE;
                      break;
                     }
                 }
          if (i >= MAX_PROTO)
             {if (found)
                 act("Похоже, Вы уже что-то используете вместо $o1.",FALSE,ch,obj,0,TO_CHAR);
              else
                 act("Похоже, $o не подойдет для этого.",FALSE,ch,obj,0,TO_CHAR);
             }
         }
      else
      if (created_item[obj_type].material_bits &&
          !IS_SET(created_item[obj_type].material_bits, (1 << GET_OBJ_MATER(obj))))
         act("$o сделан$G из неподходящего материала.",FALSE,ch,obj,0,TO_CHAR);
      else
      if (obj->contains)
         act("В $o5 что-то лежит.",FALSE,ch,obj,0,TO_CHAR);
      else
      if (count >= MAX_ITEMS)
         {switch (subcmd)
          {case SKILL_TRANSFORMWEAPON:
                send_to_char("Столько предметов Вы не сумеете перековать.\r\n",ch);
                break;
           case SKILL_CREATEBOW:
                send_to_char("Слишком много предметов.\r\n",ch);
                break;
          }
          return;
         }
      else
         {switch (subcmd)
          {case SKILL_TRANSFORMWEAPON:
                if (count && GET_OBJ_MATER(tmp_list[count-1]) != GET_OBJ_MATER(obj))
                   {send_to_char("Разные материалы !\r\n",ch);
                    return;
                   }
                tmp_list[count++] = obj;
                break;
           case SKILL_CREATEBOW:
                act("$o не предназначен$G для этих целей.",FALSE,ch,obj,0,TO_CHAR);
                break;
          }
         }
     } while (TRUE);

  switch (subcmd)
  {case SKILL_TRANSFORMWEAPON:
        if (!count)
           {send_to_char("Вам нечего перековывать.\r\n",ch);
            return;
           }

        if (!IS_IMMORTAL(ch))
           {if (!ROOM_FLAGGED(IN_ROOM(ch),ROOM_SMITH))
               {send_to_char("Вам нужно попасть в кузницу для этого.\r\n",ch);
                return;
               }
            for (coal = ch->carrying; coal; coal = coal->next_content)
                if (GET_OBJ_TYPE(coal) == ITEM_INGRADIENT)
                   {for (i = 0; i < MAX_PROTO; i++)
                        if (proto[i] == coal)
                           break;
                        else
                        if (!proto[i] && GET_OBJ_VAL(coal,1) == created_item[obj_type].proto[i])
                           {proto[i] = coal;
                            break;
                           }
                   }
            for (i = 0, found = TRUE; i < MAX_PROTO; i++)
                if (created_item[obj_type].proto[i] && !proto[i])
                   {if ((rnum = real_object(created_item[obj_type].proto[i])) < 0)
                       act("У Вас нет необходимого инградиента.",FALSE,ch,0,0,TO_CHAR);
                    else
                       act("У Вас не хватает $o1 для этого.",FALSE,ch,obj_proto+rnum,0,TO_CHAR);
                    found = FALSE;
                   }
            if (!found)
               return;
           }
        for (i = 1; i < count; i++)
            {GET_OBJ_WEIGHT(tmp_list[0]) += GET_OBJ_WEIGHT(tmp_list[i]);
             extract_obj(tmp_list[i]);
            }
        for (i = 0; i < MAX_PROTO; i++)
            {if (proto[i])
                extract_obj(proto[i]);
            }
        go_create_weapon(ch,tmp_list[0],obj_type,SKILL_TRANSFORMWEAPON);
        break;
    case SKILL_CREATEBOW:
         for (coal = ch->carrying; coal; coal = coal->next_content)
             if (GET_OBJ_TYPE(coal) == ITEM_INGRADIENT)
                {for (i = 0; i < MAX_PROTO; i++)
                      if (proto[i] == coal)
                         break;
                        else
                      if (!proto[i] && GET_OBJ_VAL(coal,1) == created_item[obj_type].proto[i])
                         {proto[i] = coal;
                          break;
                         }
               }
         for (i = 0, found = TRUE; i < MAX_PROTO; i++)
             if (created_item[obj_type].proto[i] && !proto[i])
                {if ((rnum = real_object(created_item[obj_type].proto[i])) < 0)
                       act("У Вас нет необходимого инградиента.",FALSE,ch,0,0,TO_CHAR);
                    else
                       act("У Вас не хватает $o1 для этого.",FALSE,ch,obj_proto+rnum,0,TO_CHAR);
                    found = FALSE;
                   }
        if (!found)
           return;
        for (i = 1; i < MAX_PROTO; i++)
            if (proto[i])
               {GET_OBJ_WEIGHT(proto[0]) += GET_OBJ_WEIGHT(proto[i]);
                extract_obj(proto[i]);
               }
        go_create_weapon(ch,proto[0],obj_type,SKILL_CREATEBOW);
        break;
 }
}

int ext_search_block(char *arg, const char **list, int exact)
{
  register int i, l, j, o;

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
      *(arg + l) = LOWER(*(arg + l));

  if (exact)
     { for (i = j = 0, o = 1; j != 1; i++)
           if (**(list + i) == '\n')
              {o = 1;
               switch (j)
               {case 0          : j = INT_ONE; break;
                case INT_ONE    : j = INT_TWO; break;
                case INT_TWO    : j = INT_THREE; break;
                default         : j = 1; break;
               }
              }
           else
           if (!str_cmp(arg, *(list + i)))
              return (j | o);
           else
              o <<= 1;
     }
  else
     {if (!l)
         l = 1;	/* Avoid "" to match the first available
				 * string */
      for (i = j = 0, o = 1; j != 1; i++)
           if (**(list + i) == '\n')
              {o = 1;
               switch (j)
               {case 0          : j = INT_ONE; break;
                case INT_ONE    : j = INT_TWO; break;
                case INT_TWO    : j = INT_THREE; break;
                default         : j = 1; break;
               }
              }
           else
           if (!strn_cmp(arg, *(list + i), l))
              return (j | o);
           else
              o <<= 1;
     }

  return (0);
}



ACMD(do_cheat)
{
  struct obj_data *obj;
  int    add_ac, j, i;
  char   field[MAX_INPUT_LENGTH], subfield[MAX_INPUT_LENGTH];

  argument = one_argument(argument, arg);

  if (!GET_COMMSTATE(ch))
     {send_to_char("Чаво ?\r\n", ch);
      return;
     }

  if (!*arg)
     {send_to_char("Чего читим ?\r\n", ch);
      return;
     }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {sprintf(buf, "У Вас нет \'%s\'.\r\n", arg);
      send_to_char(buf, ch);
      return;
     };

  *field = *subfield = 0;
  argument = one_argument(argument, field);
  argument = one_argument(argument, subfield);

  if (!*field)
     {send_to_char("Не указано поле !\r\n",ch);
      return;
     }

  if (!strn_cmp(field,"поле0",5))
     {send_to_char("Поле 0 : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("Требуется числовое значение.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_VAL(obj,0) = i;
     }
  else
  if (!strn_cmp(field,"поле1",5))
     {send_to_char("Поле 1 : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("Требуется числовое значение.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_VAL(obj,1) = i;
     }
  else
  if (!strn_cmp(field,"поле2",5))
     {send_to_char("Поле 2 : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("Требуется числовое значение.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_VAL(obj,2) = i;
     }
  else
  if (!strn_cmp(field,"поле3",5))
     {send_to_char("Поле 3 : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("Требуется числовое значение.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_VAL(obj,3) = i;
     }
  else
  if (!strn_cmp(field,"вес",3))
     {send_to_char("Вес : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("Требуется числовое значение.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      obj_from_char(obj);
      GET_OBJ_WEIGHT(obj) = i;
      obj_to_char(obj,ch);
     }
  else
  if (!strn_cmp(field,"таймер",6))
     {send_to_char("Таймер : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("Требуется числовое значение.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_TIMER(obj) = i;
     }
  else
  if (!strn_cmp(field,"макс",4))
     {send_to_char("Максимальная прочность : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("Требуется числовое значение.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_MAX(obj) = i;
     }
  else
  if (!strn_cmp(field,"проч",4))
     {send_to_char("Прочность : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("Требуется числовое значение.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_CUR(obj) = i;
     }
  else
  if (!strn_cmp(field,"матер",5))
     {send_to_char("Материал : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("Требуется числовое значение.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_MATER(obj) = i;
     }
  else
  if ((i = ext_search_block(field, extra_bits, FALSE)))
     {send_to_char("Экстрабит : ", ch);
      if (!*subfield)
         TOGGLE_BIT(GET_OBJ_EXTRA(obj,i),i);
      else
      if (!str_cmp(subfield,"да") || !str_cmp(subfield,"нет"))
         SET_BIT(GET_OBJ_EXTRA(obj, i),i);
      else
         REMOVE_BIT(GET_OBJ_EXTRA(obj, i),i);
      sprintf(buf,"%x %s !\r\n",i,IS_SET(GET_OBJ_EXTRA(obj,i),i) ? "On" : "Off");
      send_to_char(buf,ch);
     }
  else
  if ((i = ext_search_block(field, weapon_affects, FALSE)))
     {send_to_char("Аффект : ", ch);
      if (!*subfield)
         TOGGLE_BIT(GET_OBJ_AFF(obj,i),i);
      else
      if (!str_cmp(subfield,"да") || !str_cmp(subfield,"нет"))
         SET_BIT(GET_OBJ_AFF(obj, i),i);
      else
         REMOVE_BIT(GET_OBJ_AFF(obj, i),i);
      sprintf(buf,"%x %s !\r\n",i,IS_SET(GET_OBJ_AFF(obj,i),i) ? "On" : "Off");
      send_to_char(buf,ch);
     }
  else
  if ((i = search_block(field, wear_bits, FALSE)) >= 0)
     {send_to_char("Одеть : ", ch);
      if (!*subfield)
         TOGGLE_BIT(GET_OBJ_WEAR(obj),(1 << i));
      else
      if (!str_cmp(subfield,"да") || !str_cmp(subfield,"нет"))
         SET_BIT(GET_OBJ_WEAR(obj),(1 << i));
      else
         REMOVE_BIT(GET_OBJ_WEAR(obj),(1 << i));
      sprintf(buf,"%s %s !\r\n",wear_bits[i],IS_SET(GET_OBJ_WEAR(obj),(1 << i)) ? "On" : "Off");
      send_to_char(buf,ch);
     }
  else
  if ((i = search_block(field, apply_types, FALSE)) >= 0)
     {send_to_char("Добавляет : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&add_ac) != 1)
         {send_to_char("Требуется числовое значение.\r\n",ch);
          return;
         }
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
          if (obj->affected[j].location == i || !obj->affected[j].location)
             break;
      if (j >= MAX_OBJ_AFFECT)
         {send_to_char("Нет свободного места !\r\n",ch);
          return;
         }
      if (!add_ac)
         {i = APPLY_NONE;
          for (; j + 1 < MAX_OBJ_AFFECT; j++)
              {obj->affected[j].location = obj->affected[j+1].location;
               obj->affected[j].modifier = obj->affected[j+1].modifier;
	      }
	 }	
      obj->affected[j].location = i;
      obj->affected[j].modifier = add_ac;
      sprintf(buf,"%s на %d !\r\n",apply_types[i],add_ac);
      send_to_char(buf,ch);
     }
  else
  if (!strn_cmp(field,"запись",6))
     {REMOVE_BIT(GET_OBJ_EXTRA(obj, ITEM_NODONATE), ITEM_NODONATE);
      GET_OBJ_OWNER(obj) = 0;
      send_to_char("Изменения сделаны постоянными.\r\n",ch);
      return;
     }
  else
     {sprintf(buf,"Ничего не заломано(%s) !\r\n", field);
      send_to_char(buf,ch);
      return;
     }

  SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NODONATE), ITEM_NODONATE);
  GET_OBJ_OWNER(obj) = GET_UNIQUE(ch);
}