	  "-- Item number : [%s%d%s]\r\n"
	  "%s1%s) Синонимы : %s%s\r\n"
	  "%s2%s) Именительный (это ЧТО)             : %s%s\r\n"
	  "%s3%s) Родительный  (нету ЧЕГО)           : %s%s\r\n"
	  "%s4%s) Дательный    (прикрепить к ЧЕМУ)   : %s%s\r\n"
	  "%s5%s) Винительный  (держать ЧТО)         : %s%s\r\n"
	  "%s6%s) Творительный (вооружиться ЧЕМ)     : %s%s\r\n"
	  "%s7%s) Предложный   (писать на ЧЕМ)       : %s%s\r\n"
	  "%s8%s) Описание          :-\r\n%s%s\r\n"
	  "%s9%s) Опис.при действии :-\r\n%s%s"
	  "%sA%s) Тип предмета      :-\r\n%s%s\r\n"
	  "%sB%s) Экстрафлаги       :-\r\n%s%s"

	  cyn, OLC_NUM(d), nrm,
	  grn, nrm, yel, (obj->name && *obj->name) ? obj->name : "undefined",
	  grn, nrm, yel, (obj->PNames[0] && *obj->PNames[0]) ? obj->short_description : "undefined",
	  grn, nrm, yel, (obj->PNames[1] && *obj->PNames[1]) ? obj->short_description : "undefined",	  
	  grn, nrm, yel, (obj->PNames[2] && *obj->PNames[2]) ? obj->short_description : "undefined",
	  grn, nrm, yel, (obj->PNames[3] && *obj->PNames[3]) ? obj->short_description : "undefined",	  
	  grn, nrm, yel, (obj->PNames[4] && *obj->PNames[4]) ? obj->short_description : "undefined",
	  grn, nrm, yel, (obj->PNames[5] && *obj->PNames[5]) ? obj->short_description : "undefined",	  
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
  sprintbits(obj->obj_flags.no_flags, no_bits, buf2, ',');
  sprintf(buf,
	  "%sC%s) Wear flags  : %s%s\r\n"
	  "%sD%s) No flags    : %s%s\r\n"
	  grn,nrm,cyn,buf1,
	  grn,nrm,cyn,buf2);
	  
  sprintbit(obj->obj_flags.anti_flags, anti_flags, buf1, ',');
  sprintbits(obj->obj_flags.affects, affects_flag, buf2, ',');
  sprintf(buf,
	      "%sE%s) Anti flags  : %s%s\r\n"
	      "%sF%s) Weight      : %s%d\r\n"
	      "%sG%s) Cost        : %s%d\r\n"
	      "%sH%s) Cost/Day    : %s%d\r\n"
	      "%sI%s) Cost/Day(eq): %s%d\r\n"
	      "%sJ%s) Max value   : %s%d\r\n"
	      "%sK%s) Cur value   : %s%d\r\n"
	      "%sL%s) Material    : %s%d\r\n"
	      "%sM%s) Timer       : %s%d\r\n"
	      "%sN%s) Skill       : %s%d\r\n"
	      "%sO%s) Values      : %s%d %d %d %d\r\n"
	      "%sP%s) Affects     : %s%s\r\n"
	      "%sR%s) Applies menu\r\n"
	      "%sT%s) Extra descriptions menu\r\n"
          "%sS%s) Script      : %s%s\r\n"
	      "%sQ%s) Quit\r\n"
	      "Enter choice : ",

	      grn, nrm, cyn, buf1,
	      grn, nrm, cyn, GET_OBJ_WEIGHT(obj),
	      grn, nrm, cyn, GET_OBJ_COST(obj),
	      grn, nrm, cyn, GET_OBJ_RENT(obj),
	      grn, nrm, cyn, GET_OBJ_RENTEQ(obj),
	      grn, nrm, cyn, GET_OBJ_MAX(obj),
	      grn, nrm, cyn, GET_OBJ_CUR(obj),
	      grn, nrm, cyn, GET_OBJ_TIMER(obj),
	      grn, nrm, cyn, GET_OBJ_SKILL(obj),
	      grn, nrm, cyn, 
	      GET_OBJ_VAL(obj, 0),GET_OBJ_VAL(obj, 1),GET_OBJ_VAL(obj, 2),GET_OBJ_VAL(obj, 3),  
	      grn, nrm, grn, nrm, 
	      grn, nrm, cyn, buf2,
          grn, nrm, cyn, obj->proto_script?"Set.":"Not Set.",
          grn, nrm);
  send_to_char(buf, d->character);
  OLC_MODE(d) = OEDIT_MAIN_MENU;
