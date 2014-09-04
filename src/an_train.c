/* checks for skill improvement */
void check_improve( CHAR_DATA *ch, int sn, bool success, int multiplier )
{
    int chance;
    char buf[100];

    if (IS_NPC(ch))
  	   return;

    if (ch->level < skill_table[sn].skill_level[ch->class] ||  
        skill_table[sn].rating[ch->class] == 0             ||  
        ch->pcdata->learned[sn] == 0                       || 
        ch->pcdata->learned[sn] == 100)
	    return;  /* skill is not known */ 

    /* check to see if the character has a chance to learn */
    chance = 10 * int_app[get_curr_stat(ch,STAT_INT)].learn;
    chance /= (multiplier *	
               skill_table[sn].rating[ch->class] *	
               4);
    chance += ch->level;

    if (number_range(1,1000) > chance)
	   return;

    /* now that the character has a CHANCE to learn, see if they really have */	

    if (success)
       {chance = URANGE(5,100 - ch->pcdata->learned[sn], 95);
	    if (number_percent() < chance)
	       {sprintf(buf,"$CYou have become better at %s!$c",
		            skill_table[sn].name);
  	        act_color(buf,ch,NULL,NULL,TO_CHAR,POS_DEAD, CLR_GREEN);
	        ch->pcdata->learned[sn]++;
	        gain_exp(ch,2 * skill_table[sn].rating[ch->class]);
	       }
       }
    else
      {chance = URANGE(5,ch->pcdata->learned[sn]/2,30);
	   if (number_percent() < chance)
	      {sprintf(buf,
		           "$CYou learn from your mistakes, and your %s skill improves.$c",
		           skill_table[sn].name);
	       act_color(buf,ch,NULL,NULL,TO_CHAR,POS_DEAD,CLR_GREEN);
	       ch->pcdata->learned[sn] += number_range(1,3);
	       ch->pcdata->learned[sn] = UMIN(ch->pcdata->learned[sn],100);
	       gain_exp(ch,2 * skill_table[sn].rating[ch->class]);
	      }
      }
}
