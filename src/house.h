#define MAX_HOUSES	100
#define MAX_GUESTS	100
#define MAX_HOUSE_ROOMS 90
#define HOUSE_NAME_LEN  50
#define HOUSE_SNAME_LEN  4

#define HOUSE_PRIVATE	0

#define RANK_KNIEZE      9
#define RANK_CENTURION   7
#define RANK_VETERAN     4
#define RANK_JUNIOR      2
#define RANK_NOVICE      1
#define RANK_GUEST       0


struct house_control_rec
{  room_vnum vnum[MAX_HOUSE_ROOMS]; /* vnum of this house */
   room_vnum atrium;                /* vnum of atrium */
   char      name[HOUSE_NAME_LEN];
   char      sname[HOUSE_SNAME_LEN+1];
   sh_int    exit_num;              /* direction of house's exit */
   time_t    built_on;              /* date this house was built */
   int       mode;                  /* mode of ownership */
   long      owner;                 /* idnum of house's owner */
   int       num_of_guests;         /* how many guests in this house */
   long      guests[MAX_GUESTS];    /* idnums of house's guests */
   time_t    last_payment;	    /* date of last house payment */
   long      unique;
   long      keeper;
   long      spare0;
   long      spare1;
   long      spare2;
   long      spare3;
   long      spare4;
   long      spare5;
   long      spare6;
   long      spare7;
};

#define HOUSE_UNIQUE(house_num) (house_control[house_num].unique)
#define HOUSE_KEEPER(house_num) (house_control[house_num].keeper)



#define TOROOM(room, dir) (world[room].dir_option[dir] ? \
                           world[room].dir_option[dir]->to_room : NOWHERE)

void	House_listrent(struct char_data *ch, room_vnum vnum);
void	House_boot(void);
void	House_save_all(void);
int	House_can_enter(struct char_data *ch, room_vnum house);
void	House_crashsave(room_vnum house_num);
void	House_list_guests(struct char_data *ch, int i, int quiet);
void	House_list_rooms(struct char_data *ch, int i, int quiet);

int     House_check_exist(long uid);
int     House_check_free(long uid);

void    House_set_keeper(struct char_data *ch);
void    House_channel(struct char_data *ch, char *msg);
int     House_major(struct char_data *ch);
char*   House_name(struct char_data *ch);
char*   House_rank(struct char_data *ch);
char*   House_sname(struct char_data *ch);
void    House_list(struct char_data *ch);
void    House_list_all(struct char_data *ch);
