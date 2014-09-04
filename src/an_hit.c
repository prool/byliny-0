/*
 * Hit one guy once.
 */
void one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt ,bool secondary)
{
    OBJ_DATA *wield;
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int dam;
    int diceroll;
    int sn,skill;
    int dam_type;
    bool counter;
    bool yell;
    bool result;
    OBJ_DATA *corpse;
    int sercount;

    sn = -1;
    counter = FALSE;

    if (victim->fighting == ch)
       yell = FALSE;
    else 
       yell = TRUE;

    /* just in case */
    if (victim == ch || ch == NULL || victim == NULL)
	   return;

    /* ghosts can't fight */
    if ((!IS_NPC(victim) && IS_SET(victim->act, PLR_GHOST)) ||
	    (!IS_NPC(ch) && IS_SET(ch->act, PLR_GHOST)))
	   return;

    /*
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
	   return;

    /*
     * Figure out the type of damage message.
     */
  
    if (secondary)
       wield = get_eq_char( ch, WEAR_SECOND_WIELD);
	else
       wield = get_eq_char( ch, WEAR_WIELD );

    if ( dt == TYPE_UNDEFINED )
       { dt = TYPE_HIT;
	     if ( wield != NULL && wield->item_type == ITEM_WEAPON )
	        dt += wield->value[3];
	     else 
	        dt += ch->dam_type;
       }

    if (dt < TYPE_HIT)
    	if (wield != NULL)
    	    dam_type = attack_table[wield->value[3]].damage;
    	else
    	    dam_type = attack_table[ch->dam_type].damage;
    else
    	dam_type = attack_table[dt - TYPE_HIT].damage;

    if (dam_type == -1)
	   dam_type = DAM_BASH;

    /* get the weapon skill */
    sn    = get_weapon_sn(ch);
    skill = 20 + get_weapon_skill(ch,sn);

    /*
     * Calculate to-hit-armor-class-0 versus armor.
     */
    if ( IS_NPC(ch) )
       {thac0_00 = 20;
	    thac0_32 = -4;   /* as good as a thief */ 
	    if (IS_SET(ch->act,ACT_WARRIOR))
	       thac0_32 = -10;
	    else 
	    if (IS_SET(ch->act,ACT_THIEF))
	       thac0_32 = -4;
	    else 
	    if (IS_SET(ch->act,ACT_CLERIC))
	       thac0_32 = 2;
	    else 
	    if (IS_SET(ch->act,ACT_MAGE))
	       thac0_32 = 6;
       }
    else
       {thac0_00 = class_table[ch->class].thac0_00;
	    thac0_32 = class_table[ch->class].thac0_32;
       }

    thac0  = interpolate( ch->level, thac0_00, thac0_32 );

    if (thac0 < 0)
       thac0 = thac0/2;

    if (thac0 < -5)
       thac0 = -5 + (thac0 + 5) / 2;

    thac0 -= GET_HITROLL(ch) * skill/100;
    thac0 += 5 * (100 - skill) / 100;

    if (dt == gsn_backstab)
	   thac0 -= 10 * (100 - get_skill(ch,gsn_backstab));

    if (dt == gsn_dual_backstab)
       thac0 -= 10 * (100 - get_skill(ch,gsn_dual_backstab));

    if (dt == gsn_cleave)
        thac0 -= 10 * (100 - get_skill(ch,gsn_cleave));

    if (dt == gsn_ambush)
        thac0 -= 10 * (100 - get_skill(ch,gsn_ambush));

    if (dt == gsn_vampiric_bite)
	    thac0 -= 10 * (100 - get_skill(ch,gsn_vampiric_bite));

    switch(dam_type)
    {
	case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;	break;
	case(DAM_BASH):	 victim_ac = GET_AC(victim,AC_BASH)/10;		break;
	case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;	break;
	default:	     victim_ac = GET_AC(victim,AC_EXOTIC)/10;	break;
    }; 
	
    if (victim_ac < -15)
	   victim_ac = (victim_ac + 15) / 5 - 15;
     
    if ( get_skill(victim,gsn_armor_use) > 70)
	   {check_improve(victim,gsn_armor_use,TRUE,8);
	    victim_ac -= (victim->level) / 2;
	   }

    if ( !can_see( ch, victim ) )
	   {if ( ch->level > skill_table[gsn_blind_fighting].skill_level[ch->class]
		     && number_percent() < get_skill(ch,gsn_blind_fighting) )
		   {check_improve(ch,gsn_blind_fighting,TRUE,16);
		   }
	    else 
	    victim_ac -= 4;
	   }

    if ( victim->position < POS_FIGHTING)
	   victim_ac += 4;
 
    if (victim->position < POS_RESTING)
	   victim_ac += 6;

    /*
     * The moment of excitement!
     */
    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
	;

    if ( diceroll == 0 || 
         ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
       {/* Miss. */
	    damage( ch, victim, 0, dt, dam_type, TRUE );
	    tail_chain( );
	    return;
       }

    /*
     * Hit.
     * Calc damage.
     */


    if ( IS_NPC(ch) && (!ch->pIndexData->new_format || wield == NULL))
	   if (!ch->pIndexData->new_format)
	      {dam = number_range( ch->level / 2, ch->level * 3 / 2 );
	       if ( wield != NULL )
	    	  dam += dam / 2;
	      }
	   else
	      dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE]);
    else
       {if (sn != -1)
	       check_improve(ch,sn,TRUE,5);
	    if ( wield != NULL )
	       {if (wield->pIndexData->new_format)
		       dam = dice(wield->value[1],wield->value[2]) * skill/100;
	        else
	    	   dam = number_range( wield->value[1] * skill/100, 
			  	                   wield->value[2] * skill/100);

	        if (get_eq_char(ch,WEAR_SHIELD) == NULL)  /* no shield = more */
		       dam = dam * 21/20;

            /* sharpness! */
            if (IS_WEAPON_STAT(wield,WEAPON_SHARP))
               {int percent;

                if ((percent = number_percent()) <= (skill / 8))
                    dam = 2 * dam + (dam * 2 * percent / 100);
               }
	        /* holy weapon */
            if (IS_WEAPON_STAT(wield,WEAPON_HOLY) &&
		        IS_GOOD(ch) && IS_EVIL(victim) && number_percent() < 30)
               {act_color("$C$p shines with a holy area.$c",
			              ch,wield,NULL,TO_CHAR,POS_DEAD,CLR_YELLOW);
		        act_color("$C$p shines with a holy area.$c",
			              ch,wield,NULL,TO_ROOM,POS_DEAD,CLR_YELLOW);
                dam += dam * 120 / 100;
               }
	       }
	    else 
	       {dam =number_range(1 + 4 * skill/100, 2 * ch->level/3 * skill/100);
	        if ( get_skill(ch,gsn_master_hand) > 0 )
    		   {int d;
        	    if ((d=number_percent()) <= get_skill(ch,gsn_master_hand))
        		   {check_improve(ch,gsn_master_hand,TRUE,6);
            		dam += dam * 110 /100;
		            if ( d < 20 )
			           {SET_BIT(victim->affected_by,AFF_WEAK_STUN);
	                    act_color("$CYou hit $N with a stunning force!$c",
		                          ch,NULL,victim,TO_CHAR,POS_DEAD,CLR_RED);
	                    act_color("$C$n hit you with a stunning force!$c",
		                          ch,NULL,victim,TO_VICT,POS_DEAD,CLR_RED);
	                    act_color("$C$n hits $N with a stunning force!$c",
		                          ch,NULL,victim,TO_NOTVICT,POS_DEAD,CLR_RED);
            		    check_improve(ch,gsn_master_hand,TRUE,6);
			           }
        		   }
		       }

	       }
       }

    /*
     * Bonuses.
     */    
    if ( get_skill(ch,gsn_enhanced_damage) > 0 )
       {diceroll = number_percent();
        if (diceroll <= get_skill(ch,gsn_enhanced_damage))
           {int div;
            check_improve(ch,gsn_enhanced_damage,TRUE,6);
	        div = (ch->class == CLASS_WARRIOR) ? 100 : 
			      (ch->class == CLASS_CLERIC)  ? 130 : 114;
            dam += dam * diceroll/div;
           }
       }

    if ( get_skill(ch,gsn_master_sword) > 0 && sn== gsn_sword)
       {if (number_percent() <= get_skill(ch,gsn_master_sword))
           {OBJ_DATA *katana;

            check_improve(ch,gsn_master_sword,TRUE,6);
            dam += dam * 110 /100;

	        if ( (katana = get_eq_char(ch,WEAR_WIELD)) != NULL)
		       {AFFECT_DATA *paf;

		        if ( IS_WEAPON_STAT(katana,WEAPON_KATANA) && 
		              strstr(katana->extra_descr->description,ch->name) != NULL )
		           {katana->cost++;
		            if (katana->cost > 249)
		               {paf =  affect_find(katana->affected,gsn_katana);
		                if (paf != NULL)
			               {int old_mod=paf->modifier;			 
			                paf->modifier = UMIN(paf->modifier+1,ch->level / 3);
			                ch->hitroll += paf->modifier - old_mod;
			                if (paf->next != NULL)
				               {paf->next->modifier = paf->modifier;
				                ch->damroll += paf->modifier - old_mod;
				               }
			            act("$n's katana glows blue.\n\r",ch,NULL,NULL,TO_ROOM);
			            send_to_char("Your katana glows blue.\n\r",ch);
			           }
		            katana->cost = 0;
		           }
		       }
	       }
	    else 
	    if ( (katana=get_eq_char(ch,WEAR_SECOND_WIELD)) != NULL)
		   {AFFECT_DATA *paf;

		    if ( IS_WEAPON_STAT(katana,WEAPON_KATANA) && 
		         strstr(katana->extra_descr->description,ch->name)!=NULL )
		       {katana->cost++;
		        if (katana->cost > 249)
		           {paf =  affect_find(katana->affected,gsn_katana);
		            if (paf != NULL)
			           {paf->modifier = UMIN(paf->modifier+1,ch->level / 3);
			            if (paf->next != NULL)
			 	           paf->next->modifier = paf->modifier;
			            act("$n's katana glows blue.\n\r",ch,NULL,NULL,TO_ROOM);
			            send_to_char("Your katana glows blue.\n\r",ch);
			           }
		            katana->cost = 0;
	               }
		       }
		   }
       }
    }

    if ( !IS_AWAKE(victim) )
	   dam *= 2;
    else 
    if (victim->position < POS_FIGHTING)
	   dam = dam * 3 / 2;

    sercount = number_percent();
    if (dt==gsn_backstab || dt==gsn_vampiric_bite)
	   sercount += 40;
    if ( ch->last_fight_time != -1 && !IS_IMMORTAL(ch) &&
        (current_time - ch->last_fight_time)<FIGHT_DELAY_TIME) 
       {sercount += 10;
       }
    sercount *= 2;
    if (victim->fighting == NULL && !IS_NPC(victim) &&
	    !is_safe_nomessage(victim, ch) && !is_safe_nomessage(ch,victim) &&
	    (victim->position == POS_SITTING || victim->position == POS_STANDING)
	    && dt != gsn_assassinate &&
	    (sercount <= get_skill(victim,gsn_counter) ))
       {counter = TRUE;
	    check_improve(victim,gsn_counter,TRUE,1);
	    act("$N turns your attack against you!",ch,NULL,victim,TO_CHAR);
	    act("You turn $n's attack against $m!",ch,NULL,victim,TO_VICT);
	    act("$N turns $n's attack against $m!",ch,NULL,victim,TO_NOTVICT);
	    ch->fighting = victim;
       }
    else 
    if (!victim->fighting) 
       check_improve(victim,gsn_counter,FALSE,1);

    if ( dt == gsn_backstab && wield != NULL) 
        dam = (ch->level < 50) ? (ch->level/10 + 1) * dam + ch->level : 
                                 (ch->level/10) * dam + ch->level;
    else 
    if ( dt == gsn_dual_backstab && wield != NULL)
        dam = (ch->level < 56) ? (ch->level/14 + 1) * dam + ch->level : 
                                 (ch->level/14) * dam + ch->level;
    else 
    if (dt == gsn_circle)
       dam = (ch->level/40 + 1) * dam + ch->level;
    else 
    if ( dt == gsn_vampiric_bite && IS_VAMPIRE(ch)) 
       dam = (ch->level/20 + 1) * dam + ch->level;
    else 
    if ( dt == gsn_cleave && wield != NULL)
       {if (number_percent() < URANGE(4, 5+(ch->level-victim->level),10) && !counter)
	       {act_color("Your cleave chops $N $CIN HALF!$c",
		               ch,NULL,victim,TO_CHAR,POS_RESTING,CLR_RED);
  	        act_color("$n's cleave chops you $CIN HALF!$c",
		              ch,NULL,victim,TO_VICT,POS_RESTING,CLR_RED);
 	        act_color("$n's cleave chops $N $CIN HALF!$c",
		              ch,NULL,victim,TO_NOTVICT,POS_RESTING,CLR_RED);
	        send_to_char("You have been KILLED!\n\r",victim);
	        act("$n is DEAD!",victim,NULL,NULL,TO_ROOM);
	        WAIT_STATE( ch, 2 );
	        raw_kill(victim);
	        if ( !IS_NPC(ch) && IS_NPC(victim) )
	           {corpse = get_obj_list( ch, "corpse", ch->in_room->contents ); 
		
		        if ( IS_SET(ch->act, PLR_AUTOLOOT) &&
		             corpse && corpse->contains) /* exists and not empty */
		           do_get( ch, "all corpse" );
		
		        if (IS_SET(ch->act,PLR_AUTOGOLD) &&
		            corpse && corpse->contains  && /* exists and not empty */
		            !IS_SET(ch->act,PLR_AUTOLOOT))  
		           {do_get(ch, "gold corpse");
		            do_get(ch, "silver corpse");
		           }

		if ( IS_SET(ch->act, PLR_AUTOSAC) )
		  if (IS_SET(ch->act,PLR_AUTOLOOT) && corpse
		      && corpse->contains)
		    return;  /* leave if corpse has treasure */
		  else
		    do_sacrifice( ch, "corpse" );
	      }
	    return;
	  }
	else dam = (dam * 2 + ch->level);
      }

    if (dt == gsn_assassinate)
      {
	if (number_percent() <= URANGE(10, 20+(ch->level-victim->level)*2, 50) && !counter)
	  {
	    act_color("You $C+++ASSASSINATE+++$c $N!",ch,NULL,victim,TO_CHAR,
		      POS_RESTING,CLR_RED);
	    act("$N is DEAD!",ch,NULL,victim,TO_CHAR);
	    act_color("$n $C+++ASSASSINATES+++$c $N!",ch,NULL,victim,
		      TO_NOTVICT,POS_RESTING,CLR_RED);
	    act("$N is DEAD!",ch,NULL,victim,TO_NOTVICT);
	    act_color("$n $C+++ASSASSINATES+++$c you!",ch,NULL,victim,
		      TO_VICT,POS_DEAD,CLR_RED);
	    send_to_char("You have been KILLED!\n\r",victim);
	    check_improve(ch,gsn_assassinate,TRUE,1);
	    raw_kill(victim);
	    if ( !IS_NPC(ch) && IS_NPC(victim) )
	      {
		corpse = get_obj_list( ch, "corpse", ch->in_room->contents ); 
		
		if ( IS_SET(ch->act, PLR_AUTOLOOT) &&
		    corpse && corpse->contains) /* exists and not empty */
		  do_get( ch, "all corpse" );
		
		if (IS_SET(ch->act,PLR_AUTOGOLD) &&
		    corpse && corpse->contains  && /* exists and not empty */
		    !IS_SET(ch->act,PLR_AUTOLOOT))
		  do_get(ch, "gold corpse");
		
		if ( IS_SET(ch->act, PLR_AUTOSAC) )
		  if ( IS_SET(ch->act,PLR_AUTOLOOT) && corpse
		      && corpse->contains)
		    return;  /* leave if corpse has treasure */
		  else
		    do_sacrifice( ch, "corpse" );
	      }
	    return;

	  }
	else 
	  {
	    check_improve(ch,gsn_assassinate,FALSE,1);
	    dam *= 2;
	  }
      }

	
    dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

    if (dt == gsn_ambush)
      dam *= 3;

    if (!IS_NPC(ch) && get_skill(ch,gsn_deathblow) > 1 &&
	ch->level >= skill_table[gsn_deathblow].skill_level[ch->class] )
      {
	if (number_percent() < 0.125 * get_skill(ch,gsn_deathblow))
	  {
	    act("You deliver a blow of deadly force!",ch,NULL,NULL,TO_CHAR);
	    act("$n delivers a blow of deadly force!",ch,NULL,NULL,TO_ROOM);
	    if (cabal_ok(ch,gsn_deathblow)) {
	      dam *= ((float)ch->level) / 20;
	      check_improve(ch,gsn_deathblow,TRUE,1);
	    }
	  }
	else check_improve(ch,gsn_deathblow,FALSE,3);
      }

    if ( dam <= 0 )
	dam = 1;

    if (counter)
      {
	result = damage(ch,ch,2*dam,dt,dam_type,TRUE);
	multi_hit(victim,ch,TYPE_UNDEFINED);
      }
    else result = damage( ch, victim, dam, dt, dam_type, TRUE );

    /* vampiric bite gives hp to ch from victim */
	if (dt == gsn_vampiric_bite)
	{
	 int hit_ga = UMIN( (dam / 2 ), victim->max_hit );

	 ch->hit += hit_ga;
    	 ch->hit  = UMIN( ch->hit , ch->max_hit);
	 update_pos( ch );
    send_to_char("Your health increases as you suck your victim's blood.\n\r",ch);
	}

    /* but do we have a funky weapon? */
    if (result && wield != NULL)
    {
        int dam;

        if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_POISON))
        {
            int level;
            AFFECT_DATA *poison, af;

            if ((poison = affect_find(wield->affected,gsn_poison)) == NULL)
                level = wield->level;
            else
                level = poison->level;
            if (!saves_spell(level / 2,victim,DAM_POISON))
            {
                send_to_char("You feel poison coursing through your veins.",
                    victim);
                act("$n is poisoned by the venom on $p.",
                    victim,wield,NULL,TO_ROOM);

                af.where     = TO_AFFECTS;
                af.type      = gsn_poison;
                af.level     = level * 3/4;
                af.duration  = level / 2;
                af.location  = APPLY_STR;
                af.modifier  = -1;
                af.bitvector = AFF_POISON;
                affect_join( victim, &af );
            }

            /* weaken the poison if it's temporary */
            if (poison != NULL)
            {
                poison->level = UMAX(0,poison->level - 2);
                poison->duration = UMAX(0,poison->duration - 1);
                if (poison->level == 0 || poison->duration == 0)
                    act("The poison on $p has worn off.",ch,wield,NULL,TO_CHAR);
            }
        }
        if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_VAMPIRIC))
        {
            dam = number_range(1, wield->level / 5 + 1);
            act("$p draws life from $n.",victim,wield,NULL,TO_ROOM);
            act("You feel $p drawing your life away.",
                victim,wield,NULL,TO_CHAR);
            damage(ch,victim,dam,0,DAM_NEGATIVE,FALSE);
            ch->hit += dam/2;
        }
        if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_FLAMING))
        {
            dam = number_range(1,wield->level / 4 + 1);
            act("$n is burned by $p.",victim,wield,NULL,TO_ROOM);
            act("$p sears your flesh.",victim,wield,NULL,TO_CHAR);
            fire_effect( (void *) victim,wield->level/2,dam,TARGET_CHAR);
            damage(ch,victim,dam,0,DAM_FIRE,FALSE);
        }
        if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_FROST))
        {
            dam = number_range(1,wield->level / 6 + 2);
            act("$p freezes $n.",victim,wield,NULL,TO_ROOM);
            act("The cold touch of $p surrounds you with ice.",
                victim,wield,NULL,TO_CHAR);
            cold_effect(victim,wield->level/2,dam,TARGET_CHAR);
            damage(ch,victim,dam,0,DAM_COLD,FALSE);
        }
        if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_SHOCKING))
        {
            dam = number_range(1,wield->level/5 + 2);
            act("$n is struck by lightning from $p.",victim,wield,NULL,TO_ROOM);
            act("You are shocked by $p.",victim,wield,NULL,TO_CHAR);
            shock_effect(victim,wield->level/2,dam,TARGET_CHAR);
            damage(ch,victim,dam,0,DAM_LIGHTNING,FALSE);
        }
    }

    tail_chain( );
    return;
}

