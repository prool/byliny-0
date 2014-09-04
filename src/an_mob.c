/* procedure for all mobile attacks */
void mob_hit (CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
    int chance,number;
    CHAR_DATA *vch, *vch_next;

    /* no attacks on ghosts */
    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_GHOST))
	   return;

    /* no attack by ridden mobiles except spec_casts */
    if (RIDDEN(ch) )
	{if (ch->fighting != victim) 
	    set_fighting(ch,victim); 
	 return;
	}

    one_hit(ch,victim,dt,FALSE);

    if (ch->fighting != victim)
	   return;

 
    /* Area attack -- BALLS nasty! */
 
    if (IS_SET(ch->off_flags,OFF_AREA_ATTACK))
       {for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	        {vch_next = vch->next_in_room;
	         if ((vch != victim && vch->fighting == ch))
		        one_hit(ch,vch,dt,FALSE);
	        }
       }

    if (IS_AFFECTED(ch,AFF_HASTE) || IS_SET(ch->off_flags,OFF_FAST))
	   one_hit(ch,victim,dt,FALSE);

    if (ch->fighting != victim || dt == gsn_backstab || dt == gsn_circle ||
	    dt == gsn_dual_backstab || dt == gsn_cleave || dt == gsn_ambush  || 
	    dt == gsn_vampiric_bite)
	   return;

    chance = get_skill(ch,gsn_second_attack)/2;
    if (number_percent() < chance)
       {one_hit(ch,victim,dt,FALSE);
	    if (ch->fighting != victim)
	       return;
       }

    chance = get_skill(ch,gsn_third_attack)/4;
    if (number_percent() < chance)
       {one_hit(ch,victim,dt,FALSE);
	    if (ch->fighting != victim)
	       return;
       } 

    chance = get_skill(ch,gsn_fourth_attack)/6;
    if ( number_percent( ) < chance )
       {one_hit( ch, victim, dt ,FALSE);
	    if ( ch->fighting != victim )
	       return;
       }

    /* PC waits */

    if (ch->wait > 0)
	   return;

    number = number_range(0,2);

    if (number == 1 && IS_SET(ch->act,ACT_MAGE))
       { 
	    /*  { mob_cast_mage(ch,victim); return; } */; 
       }
 

    if (number == 2 && IS_SET(ch->act,ACT_CLERIC))
       { 
	   /*  { mob_cast_cleric(ch,victim); return; } */; 
       } 

    /* now for the skills */

    number = number_range(0,7);

    switch(number) 
    {
    case (0) :
	if (IS_SET(ch->off_flags,OFF_BASH))
	    do_bash(ch,"");
	break;

    case (1) :
	if (IS_SET(ch->off_flags,OFF_BERSERK) && !IS_AFFECTED(ch,AFF_BERSERK))
	    do_berserk(ch,"");
	break;


    case (2) :
	if (IS_SET(ch->off_flags,OFF_DISARM) || 
	    (get_weapon_sn(ch) != gsn_hand_to_hand 	&& 
	     (IS_SET(ch->act,ACT_WARRIOR) || IS_SET(ch->act,ACT_THIEF))))
       do_disarm(ch,"");
	break;

    case (3) :
	if (IS_SET(ch->off_flags,OFF_KICK))
	    do_kick(ch,"");
	break;

    case (4) :
	if (IS_SET(ch->off_flags,OFF_KICK_DIRT))
	   do_dirt(ch,"");
	break;

    case (5) :
	if (IS_SET(ch->off_flags,OFF_TAIL))
	   do_tail(ch,"");
	break; 

    case (6) :
	if (IS_SET(ch->off_flags,OFF_TRIP))
	    do_trip(ch,"");
	break;
	
    case (7) :
	if (IS_SET(ch->off_flags,OFF_CRUSH))
	    do_crush(ch,"");
	break;
    }
}
