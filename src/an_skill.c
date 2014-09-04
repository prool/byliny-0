/* for returning skill information */
int get_skill(CHAR_DATA *ch, int sn)
{
    int skill;

    if (sn == -1) /* shorthand for level based skills */
       {skill = ch->level * 5 / 2;
       }
    else 
    if (sn < -1 || sn > MAX_SKILL)
       {bug("Bad sn %d in get_skill.",sn);
	    skill = 0;
       }
    else 
    if (!IS_NPC(ch))
       {if (ch->level < skill_table[sn].skill_level[ch->class])
	       skill = 0;
	    else
	       skill = ch->pcdata->learned[sn];
       }
    else /* mobiles */
       {if (skill_table[sn].spell_fun != spell_null)
	       skill = 40 + 2 * ch->level;
	    else 
	    if (sn == gsn_sneak || sn == gsn_hide)
	       skill = ch->level * 2 + 20;
        else 
        if ((sn == gsn_dodge && IS_SET(ch->off_flags,OFF_DODGE)) ||       
            (sn == gsn_parry && IS_SET(ch->off_flags,OFF_PARRY)))
	       skill = ch->level * 2;
 	    else 
 	    if (sn == gsn_shield_block)
	       skill = 10 + 2 * ch->level;
	    else 
	    if (sn == gsn_second_attack  && 
	        (IS_SET(ch->act,ACT_WARRIOR) || IS_SET(ch->act,ACT_THIEF)))
	       skill = 10 + 3 * ch->level;
	    else 
	    if (sn == gsn_third_attack && IS_SET(ch->act,ACT_WARRIOR))
	       skill = 4 * ch->level - 40;
	    else 
	    if (sn == gsn_fourth_attack && IS_SET(ch->act,ACT_WARRIOR))
	       skill = 4 * ch->level - 60;
	    else 
	    if (sn == gsn_hand_to_hand)
	       skill = 40 + 2 * ch->level;
 	    else 
 	    if (sn == gsn_trip && IS_SET(ch->off_flags,OFF_TRIP))
	       skill = 10 + 3 * ch->level;
 	    else 
 	    if (sn == gsn_bash && IS_SET(ch->off_flags,OFF_BASH))
	       skill = 10 + 3 * ch->level;
	    else 
	    if (sn == gsn_disarm  &&  
	        (IS_SET(ch->off_flags,OFF_DISARM)  ||   
	         IS_SET(ch->act,ACT_WARRIOR)       ||
	         IS_SET(ch->act,ACT_THIEF)))
	       skill = 20 + 3 * ch->level;
	    else 
	    if (sn == gsn_grip &&  
	        (IS_SET(ch->act,ACT_WARRIOR) || IS_SET(ch->act,ACT_THIEF)))
	       skill = ch->level;
	    else 
	    if (sn == gsn_berserk && IS_SET(ch->off_flags,OFF_BERSERK))
	       skill = 3 * ch->level;
	    else 
	    if (sn == gsn_kick)
	       skill = 10 + 3 * ch->level;
	    else 
	    if (sn == gsn_backstab && IS_SET(ch->act,ACT_THIEF))
	       skill = 20 + 2 * ch->level;
  	    else 
  	    if (sn == gsn_rescue)
	       skill = 40 + ch->level; 
	    else 
	    if (sn == gsn_recall)
	       skill = 40 + ch->level;
	    else 
	    if (sn == gsn_sword	|| sn == gsn_dagger	|| sn == gsn_spear ||  
	        sn == gsn_mace	||  sn == gsn_axe	||  sn == gsn_flail||  
	        sn == gsn_whip	||  sn == gsn_polearm ||  sn == gsn_bow||  
	        sn == gsn_arrow	||  sn == gsn_lance)
   	       skill = 40 + 5 * ch->level / 2;
	    else 
	       skill = 0;
       }

    if (ch->daze > 0)
       {if (skill_table[sn].spell_fun != spell_null)
	       skill /= 2;
	    else
	       skill = 2 * skill / 3;
       }

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]  > 10 )
	   skill = 9 * skill / 10;

    return URANGE(0,skill,100);
}

/* for returning weapon information */
int get_weapon_sn(CHAR_DATA *ch)
{
    OBJ_DATA *wield;
    int sn;

    wield = get_eq_char( ch, WEAR_WIELD );
    if (wield == NULL || wield->item_type != ITEM_WEAPON)
       sn = gsn_hand_to_hand;
    else 
       switch (wield->value[0])
    {
        default :               sn = -1;                break;
        case(WEAPON_SWORD):     sn = gsn_sword;         break;
        case(WEAPON_DAGGER):    sn = gsn_dagger;        break;
        case(WEAPON_SPEAR):     sn = gsn_spear;         break;
        case(WEAPON_MACE):      sn = gsn_mace;          break;
        case(WEAPON_AXE):       sn = gsn_axe;           break;
        case(WEAPON_FLAIL):     sn = gsn_flail;         break;
        case(WEAPON_WHIP):      sn = gsn_whip;          break;
        case(WEAPON_POLEARM):   sn = gsn_polearm;       break;
        case(WEAPON_BOW):   	sn = gsn_bow;       	break;
        case(WEAPON_ARROW):   	sn = gsn_arrow;       	break;
        case(WEAPON_LANCE):   	sn = gsn_lance;       	break;
   }
   return sn;
}

int get_weapon_skill(CHAR_DATA *ch, int sn)
{
     int skill;

     /* -1 is exotic */
    if (IS_NPC(ch))
       {if (sn == -1)
	       skill = 3 * ch->level;
	    else 
	    if (sn == gsn_hand_to_hand)
	       skill = 40 + 2 * ch->level;
	    else 
	       skill = 40 + 5 * ch->level / 2;
       }
    else
       {if (sn == -1)
	       skill = 3 * ch->level;
	    else
	       skill = ch->pcdata->learned[sn];
       }

    return URANGE(0,skill,100);
} 
