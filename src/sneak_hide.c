ACMD(do_sneak)
{
  struct affected_type af;
  byte percent;

  if (!GET_SKILL(ch, SKILL_SNEAK)) 
     {send_to_char("Но Вы не знаете как.\r\n", ch);
      return;
     }
     
  send_to_char("Хорошо, Вы попытаетесь двигаться бесшумно.\r\n", ch);
  
  if (AFF_FLAGGED(ch, AFF_SNEAK))
      affect_from_char(ch, SKILL_SNEAK);
      
  if (awake_others(ch) &&
      !(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
     {send_to_char("Ваша экипировка выдала Вас.\r\n",ch); 
      return;
     } 

  percent = number(1, 101);	/* 101% is a complete failure */

  if (percent > calc_skill(ch, SKILL_SNEAK, 101, NULL) + 
                dex_app_skill[GET_REAL_DEX(ch)].sneak)
     return;

  af.type       = SKILL_SNEAK;
  af.duration   = GET_LEVEL(ch);
  af.modifier   = 0;
  af.location   = APPLY_NONE;
  af.bitvector  = AFF_SNEAK;
  affect_to_char(ch, &af);
}



ACMD(do_hide)
{
  byte percent;

  if (!GET_SKILL(ch, SKILL_HIDE)) 
     {send_to_char("Но Вы не знаете как.\r\n", ch);
      return;
     }

  send_to_char("Вы попытались спрятаться.\r\n", ch);

  if (AFF_FLAGGED(ch, AFF_HIDE))
     REMOVE_BIT(AFF_FLAGS(ch, AFF_HIDE), AFF_HIDE);
     
  if (awake_others(ch) &&
      !(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
     {send_to_char("Ваша экипировка выдала Вас.\r\n",ch); 
      return;
     } 

  percent = number(1, 101);	/* 101% is a complete failure */
  if (percent > calc_skill(ch, SKILL_HIDE, 101, NULL) + 
                dex_app_skill[GET_REAL_DEX(ch)].hide)
     return;
  SET_BIT(AFF_FLAGS(ch, AFF_HIDE), AFF_HIDE);
}
