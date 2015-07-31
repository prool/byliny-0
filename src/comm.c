/* ************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __COMM_C__

#include <unistd.h> // prool
#include "prool.h" // prool

#include "conf.h"
#include "sysdep.h"

#ifdef CIRCLE_MACINTOSH		/* Includes for the Macintosh */
# define SIGPIPE 13
# define SIGALRM 14
  /* GUSI headers */
# include <sys/ioctl.h>
  /* Codewarrior dependant */
# include <SIOUX.h>
# include <console.h>
#endif

#ifdef CIRCLE_WINDOWS		/* Includes for Win32 */
# ifdef __BORLANDC__
#  include <dir.h>
# else /* MSVC */
#  include <direct.h>
# endif
# include <mmsystem.h>
#endif /* CIRCLE_WINDOWS */

#ifdef CIRCLE_AMIGA		/* Includes for the Amiga */
# include <sys/ioctl.h>
# include <clib/socket_protos.h>
#endif /* CIRCLE_AMIGA */

#ifdef CIRCLE_ACORN		/* Includes for the Acorn (RiscOS) */
# include <socklib.h>
# include <inetlib.h>
# include <sys/ioctl.h>
#endif

/*
 * Note, most includes for all platforms are in sysdep.h.  The list of
 * files that is included is controlled by conf.h for that platform.
 */

#include "sys/types.h"
#include "sys/socket.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "house.h"
#include "olc.h"
#include "dg_scripts.h"
#include "screen.h"

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

/* externs */
extern struct ban_list_element *ban_list;
extern int num_invalid;
extern char *GREETINGS;
extern const char *circlemud_version;
extern int circle_restrict;
extern int mini_mud;
extern int no_rent_check;
extern FILE *player_fl;
extern FILE *pkiller_fl;
extern ush_int DFLT_PORT;
extern const char *DFLT_DIR;
extern const char *DFLT_IP;
extern const char *LOGNAME;
extern int max_playing;
extern int nameserver_is_slow;	/* see config.c */
extern int auto_save;		/* see config.c */
extern int autosave_time;	/* see config.c */

extern struct room_data *world;	/* In db.c */
extern struct time_info_data time_info;		/* In db.c */
extern char *help;
extern zone_rnum top_of_zone_table;
extern struct zone_data *zone_table;
extern const char *save_info_msg[];   /* In olc.c */
extern struct char_data *character_list;
extern struct char_data *combat_list;

/* local globals */
struct descriptor_data *descriptor_list = NULL;		/* master desc list */
struct txt_block *bufpool = 0;	/* pool of large output buffers */
int buf_largecount = 0;		/* # of large buffers which exist */
int buf_overflows = 0;		/* # of overflows of output */
int buf_switches = 0;		/* # of switches from small to large buf */
int circle_shutdown = 0;	/* clean shutdown */
int circle_reboot   = 0;	/* reboot the game after a shutdown */
int shutdown_time   = 0;    /* reboot at this time */
int no_specials = 0;		/* Suppress ass. of special routines */
int max_players = 0;		/* max descriptors available */
int tics = 0;			    /* for extern checkpointing */
int scheck = 0;		      	/* for syntax checking mode */
long last_rent_check = 0;   /* at what time checked rented time */
struct timeval null_time;	/* zero-valued time structure */
FILE *logfile = NULL;		/* Where to send the log messages. */
int dg_act_check;		/* toggle for act_trigger */
unsigned long dg_global_pulse = 0; /* number of pulses since game start */

int codetable; // prool
int logging; // prool

/* functions in this file */
RETSIGTYPE reread_wizlists(int sig);
RETSIGTYPE unrestrict_game(int sig);
RETSIGTYPE reap(int sig);
RETSIGTYPE checkpointing(int sig);
RETSIGTYPE hupsig(int sig);
ssize_t perform_socket_read(socket_t desc, char *read_point,size_t space_left);
ssize_t perform_socket_write(socket_t desc, const char *txt,size_t length);
void echo_off(struct descriptor_data *d);
void echo_on(struct descriptor_data *d);
void sanity_check(void);
void circle_sleep(struct timeval *timeout);
int get_from_q(struct txt_q *queue, char *dest, int *aliased);
void init_game(ush_int port);
void signal_setup(void);
void game_loop(socket_t mother_desc);
socket_t init_socket(ush_int port);
int new_descriptor(socket_t s);

int get_max_players(void);
int process_output(struct descriptor_data *t);
int process_input(struct descriptor_data *t);
void timediff(struct timeval *diff, struct timeval *a, struct timeval *b);
void timeadd(struct timeval *sum, struct timeval *a, struct timeval *b);
void flush_queues(struct descriptor_data *d);
void nonblock(socket_t s);
int perform_subst(struct descriptor_data *t, char *orig, char *subst);
int perform_alias(struct descriptor_data *d, char *orig);
void record_usage(void);
char *make_prompt(struct descriptor_data *point);
void check_idle_passwords(void);
void heartbeat(int pulse);
struct in_addr *get_bind_addr(void);
int parse_ip(const char *addr, struct in_addr *inaddr);
int set_sendbuf(socket_t s);
void setup_log(const char *filename, int fd);
int open_logfile(const char *filename, FILE *stderr_fp);
#if defined(POSIX)
sigfunc *my_signal(int signo, sigfunc * func);
#endif

/* extern fcnts */
void reboot_wizlists(void);
void boot_world(void);
void player_affect_update(void);	/* In spells.c */
void mobile_affect_update(void);
void mobile_activity(int activity_level);
void perform_violence(void);
void show_string(struct descriptor_data *d, char *input);
int isbanned(char *hostname);
void weather_and_time(int mode);
void redit_save_to_disk(int zone_num);
void oedit_save_to_disk(int zone_num);
void medit_save_to_disk(int zone_num);
void sedit_save_to_disk(int zone_num);
void zedit_save_to_disk(int zone_num);
void tics_update();
void hour_update();
int  real_zone(int number);
void koi_to_alt(char* str, int len);
void koi_to_win(char* str, int len);
void koi_to_winz(char* str, int len);
void Crash_rent_time(int dectime);
void Crash_ldsave(struct char_data *ch);
int  level_exp(int chclass, int level);
void flush_player_index(void);
void dupe_player_index(void);
void Crash_frac_save_all(int frac_part);
void Crash_frac_rent_time(int frac_part);
void dt_activity(void);
#ifdef __CXREF__
#undef FD_ZERO
#undef FD_SET
#undef FD_ISSET
#undef FD_CLR
#define FD_ZERO(x)
#define FD_SET(x, y) 0
#define FD_ISSET(x, y) 0
#define FD_CLR(x, y)
#endif


/***********************************************************************
*  main game loop and related stuff                                    *
***********************************************************************/

#if defined(CIRCLE_WINDOWS) || defined(CIRCLE_MACINTOSH)

/*
 * Windows doesn't have gettimeofday, so we'll simulate it.
 * The Mac doesn't have gettimeofday either.
 * Borland C++ warns: "Undefined structure 'timezone'"
 */
void gettimeofday(struct timeval *t, struct timezone *dummy)
{
#if defined(CIRCLE_WINDOWS)
  DWORD millisec = GetTickCount();
#elif defined(CIRCLE_MACINTOSH)
  unsigned long int millisec;
  millisec = (int)((float)TickCount() * 1000.0 / 60.0);
#endif

  t->tv_sec = (int) (millisec / 1000);
  t->tv_usec = (millisec % 1000) * 1000;
}

#endif	/* CIRCLE_WINDOWS || CIRCLE_MACINTOSH */


#define plant_magic(x)	do { (x)[sizeof(x) - 1] = MAGIC_NUMBER; } while (0)
#define test_magic(x)	((x)[sizeof(x) - 1])

int main(int argc, char **argv)
{
  ush_int port;
  int pos = 1;
  const char *dir;

  /* Initialize these to check for overruns later. */
  plant_magic(buf);
  plant_magic(buf1);
  plant_magic(buf2);
  plant_magic(arg);

// prool: config file
FILE *fconfig;
char string[BUFLEN];

#ifdef CYGWIN
codetable=UTF;
#else
codetable=KOI;
#endif

logging=1;

fconfig=fopen("prool.cfg","r");
if (fconfig)
	{
	//printf("prool.cfg open\n");
	while (!feof(fconfig))
		{char *pp;
		string[0]=0;
		fgets(string,BUFLEN,fconfig);
		pp=strchr(string,'\n');
		if (pp) *pp=0;
		//printf("`%s'\n", string);
		if (!strcmp(string,"test")) printf("TEST OK!\n");
		else if (!strcmp(string,"utf")) codetable=UTF;
		else if (!strcmp(string,"koi")) codetable=KOI;
		else if (!strcmp(string,"log")) logging=1;
		else if (!strcmp(string,"nolog")) logging=0;
		}
	fclose(fconfig);
	}
else
	{
	printf("prool.cfg not open\n");
	}

#ifdef CIRCLE_MACINTOSH
  /*
   * ccommand() calls the command line/io redirection dialog box from
   * Codewarriors's SIOUX library
   */
  argc = ccommand(&argv);
  /* Initialize the GUSI library calls.  */
  GUSIDefaultSetup();
#endif

  port = DFLT_PORT;
  dir = DFLT_DIR;

//  printf("prool: main start progname=%s\n",*argv); // prool
printf("Byliny-0 MUD server started\n");

  while ((pos < argc) && (*(argv[pos]) == '-')) {
    switch (*(argv[pos] + 1)) {
    case 'o':
      if (*(argv[pos] + 2))
	LOGNAME = argv[pos] + 2;
      else if (++pos < argc)
	LOGNAME = argv[pos];
      else {
	puts("SYSERR: File name to log to expected after option -o.");
	exit(1);
      }
      break;
    case 'd':
      if (*(argv[pos] + 2))
	dir = argv[pos] + 2;
      else if (++pos < argc)
	dir = argv[pos];
      else {
	puts("SYSERR: Directory arg expected after option -d.");
	exit(1);
      }
      break;
    case 'm':
      mini_mud = 1;
      no_rent_check = 1;
      puts("Running in minimized mode & with no rent check.");
      break;
    case 'c':
      scheck = 1;
      puts("Syntax check mode enabled.");
      break;
    case 'q':
      no_rent_check = 1;
      puts("Quick boot mode -- rent check supressed.");
      break;
    case 'r':
      circle_restrict = 1;
      puts("Restricting game -- no new players allowed.");
      break;
    case 's':
      no_specials = 1;
      puts("Suppressing assignment of special routines.");
      break;
    case 'h':
      /* From: Anil Mahajan <amahajan@proxicom.com> */
      printf("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n"
              "  -c             Enable syntax check mode.\n"
              "  -d <directory> Specify library directory (defaults to 'lib').\n"
              "  -h             Print this command line argument help.\n"
              "  -m             Start in mini-MUD mode.\n"
	      "  -o <file>      Write log to <file> instead of stderr.\n"
              "  -q             Quick boot (doesn't scan rent for object limits)\n"
              "  -r             Restrict MUD -- no new players allowed.\n"
              "  -s             Suppress special procedure assignments.\n",
		 argv[0]
      );
      exit(0);
    default:
      printf("SYSERR: Unknown option -%c in argument string.\n", *(argv[pos] + 1));
      break;
    }
    pos++;
  }

  if (pos < argc) {
    if (!isdigit(*argv[pos])) {
      printf("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
      exit(1);
    } else if ((port = atoi(argv[pos])) <= 1024) {
      printf("SYSERR: Illegal port number %d.\n", port);
      exit(1);
    }
  }

  /* All arguments have been parsed, try to open log file. */

  //printf("prool label 1\n");
  setup_log("log/syslog"/*LOGNAME*/, STDERR_FILENO); // prool

  /*
   * Moved here to distinguish command line options and to show up
   * in the log if stderr is redirected to a file.
   */
  log(circlemud_version);
  log(DG_SCRIPT_VERSION);
  if (chdir(dir) < 0) {
    perror("SYSERR: Fatal error changing to data directory");
    exit(1);
  }
  log("Using %s as data directory.", dir);

  if (scheck) {
    boot_world();
    log("Done.");
  } else {
    log("Running game on port %d.", port);
    init_game(port);
  }

  return (0);
}



/* Init sockets, run game, and cleanup sockets */
void init_game(ush_int port)
{
  socket_t mother_desc;

  /* We don't want to restart if we crash before we get up. */
  touch(KILLSCRIPT_FILE);
  touch("../.crash");
  circle_srandom(time(0));

  log("Finding player limit.");
  max_players = get_max_players();

  log("Opening mother connection.");
  mother_desc = init_socket(port);

  boot_db();

#if defined(CIRCLE_UNIX) || defined(CIRCLE_MACINTOSH)
  log("Signal trapping.");
  signal_setup();
#endif

  /* If we made it this far, we will be able to restart without problem. */
  remove(KILLSCRIPT_FILE);
  log("Entering game loop.");

  game_loop(mother_desc);

  Crash_save_all();

  log("Closing all sockets.");
  while (descriptor_list)
        close_socket(descriptor_list, TRUE);

  CLOSE_SOCKET(mother_desc);
  if (!USE_SINGLE_PLAYER)
     {fclose(player_fl);
      fclose(pkiller_fl);
     }
  if (circle_reboot != 2 && olc_save_list)
     { /* Don't save zones. */
      struct olc_save_info *entry, *next_entry;
      int rznum;

      for (entry = olc_save_list; entry; entry = next_entry)
          {next_entry = entry->next;
           if (entry->type < 0 || entry->type > 4)
              {sprintf(buf, "OLC: Illegal save type %d!", entry->type);
               log(buf);
              }
           else
           if ((rznum = real_zone(entry->zone * 100)) == -1)
              {sprintf(buf, "OLC: Illegal save zone %d!", entry->zone);
               log(buf);
              }
           else
           if (rznum < 0 || rznum > top_of_zone_table)
              {sprintf(buf, "OLC: Invalid real zone number %d!", rznum);
               log(buf);
              }
           else
              {sprintf(buf, "OLC: Reboot saving %s for zone %d.",
               save_info_msg[(int)entry->type], zone_table[rznum].number);
               log(buf);
               switch (entry->type)
                 {
         case OLC_SAVE_ROOM: redit_save_to_disk(rznum); break;
         case OLC_SAVE_OBJ:  oedit_save_to_disk(rznum); break;
         case OLC_SAVE_MOB:  medit_save_to_disk(rznum); break;
         case OLC_SAVE_ZONE: zedit_save_to_disk(rznum); break;
         case OLC_SAVE_SHOP: sedit_save_to_disk(rznum); break;
         default:      log("Unexpected olc_save_list->type"); break;
                  }
              }
          }
      }
  if (circle_reboot)
     {log("Rebooting.");
      exit(52);			/* what's so great about HHGTTG, anyhow? */
     }
  log("Normal termination of game.");
}



/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
socket_t init_socket(ush_int port)
{
  socket_t s;
  int      opt;
  struct   sockaddr_in sa;

#ifdef CIRCLE_WINDOWS
  {
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(1, 1);

    if (WSAStartup(wVersionRequested, &wsaData) != 0)
       {log("SYSERR: WinSock not available!");
        exit(1);
       }
    if ((wsaData.iMaxSockets - 4) < max_players) {
      max_players = wsaData.iMaxSockets - 4;
    }
    log("Max players set to %d", max_players);

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
      log("SYSERR: Error opening network connection: Winsock error #%d",
	  WSAGetLastError());
      exit(1);
    }
  }
#else
  /*
   * Should the first argument to socket() be AF_INET or PF_INET?  I don't
   * know, take your pick.  PF_INET seems to be more widely adopted, and
   * Comer (_Internetworking with TCP/IP_) even makes a point to say that
   * people erroneously use AF_INET with socket() when they should be using
   * PF_INET.  However, the man pages of some systems indicate that AF_INET
   * is correct; some such as ConvexOS even say that you can use either one.
   * All implementations I've seen define AF_INET and PF_INET to be the same
   * number anyway, so the point is (hopefully) moot.
   */

  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("SYSERR: Error creating socket");
    exit(1);
  }
#endif				/* CIRCLE_WINDOWS */

#if defined(SO_REUSEADDR) && !defined(CIRCLE_MACINTOSH)
  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0){
    perror("SYSERR: setsockopt REUSEADDR");
    exit(1);
  }
#endif

  set_sendbuf(s);

/*
 * The GUSI sockets library is derived from BSD, so it defines
 * SO_LINGER, even though setsockopt() is unimplimented.
 *	(from Dean Takemori <dean@UHHEPH.PHYS.HAWAII.EDU>)
 */
#if defined(SO_LINGER) && !defined(CIRCLE_MACINTOSH)
  {
    struct linger ld;

    ld.l_onoff = 0;
    ld.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0)
      perror("SYSERR: setsockopt SO_LINGER");	/* Not fatal I suppose. */
  }
#endif

  /* Clear the structure */
  memset((char *)&sa, 0, sizeof(sa));

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr = *(get_bind_addr());

  if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
    perror("SYSERR: bind");
    CLOSE_SOCKET(s);
    exit(1);
  }
  nonblock(s);
  listen(s, 5);
  return (s);
}


int get_max_players(void)
{
#ifndef CIRCLE_UNIX
  return (max_playing);
#else

  int max_descs = 0;
  const char *method;

/*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.  HAS_RLIMIT is defined in sysdep.h.
 */
#ifdef HAS_RLIMIT
  {
    struct rlimit limit;

    /* find the limit of file descs */
    method = "rlimit";
    if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
      perror("SYSERR: calling getrlimit");
      exit(1);
    }

    /* set the current to the maximum */
    limit.rlim_cur = limit.rlim_max;
    if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
      perror("SYSERR: calling setrlimit");
      exit(1);
    }
#ifdef RLIM_INFINITY
    if (limit.rlim_max == RLIM_INFINITY)
      max_descs = max_playing + NUM_RESERVED_DESCS;
    else
      max_descs = MIN(max_playing + NUM_RESERVED_DESCS, limit.rlim_max);
#else
    max_descs = MIN(max_playing + NUM_RESERVED_DESCS, limit.rlim_max);
#endif
  }

#elif defined (OPEN_MAX) || defined(FOPEN_MAX)
#if !defined(OPEN_MAX)
#define OPEN_MAX FOPEN_MAX
#endif
  method = "OPEN_MAX";
  max_descs = OPEN_MAX;		/* Uh oh.. rlimit didn't work, but we have
				 * OPEN_MAX */
#elif defined (_SC_OPEN_MAX)
  /*
   * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to
   * try the POSIX sysconf() function.  (See Stevens' _Advanced Programming
   * in the UNIX Environment_).
   */
  method = "POSIX sysconf";
  errno = 0;
  if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) {
    if (errno == 0)
      max_descs = max_playing + NUM_RESERVED_DESCS;
    else {
      perror("SYSERR: Error calling sysconf");
      exit(1);
    }
  }
#else
  /* if everything has failed, we'll just take a guess */
  method = "random guess";
  max_descs = max_playing + NUM_RESERVED_DESCS;
#endif

  /* now calculate max _players_ based on max descs */
  max_descs = MIN(max_playing, max_descs - NUM_RESERVED_DESCS);

  if (max_descs <= 0) {
    log("SYSERR: Non-positive max player limit!  (Set at %d using %s).",
	    max_descs, method);
    exit(1);
  }
  log("   Setting player limit to %d using %s.", max_descs, method);
  return (max_descs);
#endif /* CIRCLE_UNIX */
}

int shutting_down(void)
{static int lastmessage = 0;
 int    wait;

 if (!circle_shutdown)
    return (FALSE);
 if (!shutdown_time || time(NULL) >= shutdown_time)
    return (TRUE);
 if (lastmessage == shutdown_time || lastmessage == time(NULL))
    return (FALSE);
 wait = shutdown_time - time(NULL);
 if (wait == 10 || wait == 30 || wait == 60 || wait == 120 || wait % 300 == 0)
    {if (circle_reboot)
	{
	remove ("../.crash");
        sprintf(buf, "ПЕРЕЗАГРУЗКА через ");
	}
     else
	{
	remove("../.crash");
        sprintf(buf, "ОСТАНОВКА через ");
	}
     if (wait < 60)
        sprintf(buf+strlen(buf),"%d %s.\r\n",wait,desc_count(wait,WHAT_SEC));
     else
        sprintf(buf+strlen(buf),"%d %s.\r\n",wait/60,desc_count(wait/60,WHAT_MINa));
     send_to_all(buf);
     lastmessage = time(NULL);
    }
 return (FALSE);
}
/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
void game_loop(socket_t mother_desc)
{
  fd_set input_set, output_set, exc_set, null_set;
  struct timeval last_time, opt_time, process_time, temp_time;
  struct timeval before_sleep, now, timeout;
  char   comm[MAX_INPUT_LENGTH];
  struct descriptor_data *d, *next_d;
  int    pulse = 0, missed_pulses = 0, maxdesc, aliased;

  /* initialize various time values */
  null_time.tv_sec     = 0;
  null_time.tv_usec    = 0;
  opt_time.tv_usec     = OPT_USEC;
  opt_time.tv_sec      = 0;
  FD_ZERO(&null_set);
  last_rent_check      = time(NULL);

  gettimeofday(&last_time, (struct timezone *) 0);

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  while (!shutting_down())
    {/* Sleep if we don't have any connections */
     if (descriptor_list == NULL)
        {log("No connections.  Going to sleep.");
         FD_ZERO(&input_set);
         FD_SET(mother_desc, &input_set);
         if (select(mother_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0, NULL) < 0)
            {if (errno == EINTR)
	        log("Waking up to process signal.");
	     else
	        perror("SYSERR: Select coma");
            }
         else
	    log("New connection.  Waking up.");
         gettimeofday(&last_time, (struct timezone *) 0);
        }
    /* Set up the input, output, and exception sets for select(). */
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exc_set);
    FD_SET(mother_desc, &input_set);

    maxdesc = mother_desc;
    for (d = descriptor_list; d; d = d->next)
        {
         #ifndef CIRCLE_WINDOWS
          if (d->descriptor > maxdesc)
	         maxdesc = d->descriptor;
         #endif
         FD_SET(d->descriptor, &input_set);
         FD_SET(d->descriptor, &output_set);
         FD_SET(d->descriptor, &exc_set);
        }

    /*
     * At this point, we have completed all input, output and heartbeat
     * activity from the previous iteration, so we have to put ourselves
     * to sleep until the next 0.1 second tick.  The first step is to
     * calculate how long we took processing the previous iteration.
     */

    gettimeofday(&before_sleep, (struct timezone *) 0); /* current time */
    timediff(&process_time, &before_sleep, &last_time);

    /*
     * If we were asleep for more than one pass, count missed pulses and sleep
     * until we're resynchronized with the next upcoming pulse.
     */
    if (process_time.tv_sec == 0 && process_time.tv_usec < OPT_USEC)
       {missed_pulses = 0;
       }
    else
       {missed_pulses = process_time.tv_sec * PASSES_PER_SEC;
        missed_pulses += process_time.tv_usec / OPT_USEC;
        process_time.tv_sec = 0;
        process_time.tv_usec = process_time.tv_usec % OPT_USEC;
       }

    /* Calculate the time we should wake up */
    timediff(&temp_time, &opt_time, &process_time);
    timeadd(&last_time, &before_sleep, &temp_time);

    /* Now keep sleeping until that time has come */
    gettimeofday(&now, (struct timezone *) 0);
    timediff(&timeout, &last_time, &now);

    /* Go to sleep */
    do { circle_sleep(&timeout);
         gettimeofday(&now, (struct timezone *) 0);
         timediff(&timeout, &last_time, &now);
       } while (timeout.tv_usec || timeout.tv_sec);

    /* Poll (without blocking) for new input, output, and exceptions */
    if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0)
       { perror("SYSERR: Select poll");
         return;
       }
    /* If there are new connections waiting, accept them. */
    if (FD_ISSET(mother_desc, &input_set))
       new_descriptor(mother_desc);

    /* Kick out the freaky folks in the exception set and marked for close */
    for (d = descriptor_list; d; d = next_d)
        {next_d = d->next;
         if (FD_ISSET(d->descriptor, &exc_set))
            {FD_CLR(d->descriptor, &input_set);
	     FD_CLR(d->descriptor, &output_set);
	     close_socket(d,TRUE);
            }
        }

    /* Process descriptors with input pending */
    for (d = descriptor_list; d; d = next_d)
        {next_d = d->next;
         if (FD_ISSET(d->descriptor, &input_set))
            if (process_input(d) < 0)
               close_socket(d,FALSE);
         }

    /* Process commands we just read from process_input */
    for (d = descriptor_list; d; d = next_d)
        {next_d = d->next;

      /*
       * Not combined to retain --(d->wait) behavior. -gg 2/20/98
       * If no wait state, no subtraction.  If there is a wait
       * state then 1 is subtracted. Therefore we don't go less
       * than 0 ever and don't require an 'if' bracket. -gg 2/27/99
       */
         if (d->character)
            { GET_WAIT_STATE(d->character) -= (GET_WAIT_STATE(d->character) > 0 ? 1 : 0);
              SET_COMMSTATE(d->character,0);
              if (WAITLESS(d->character) || GET_WAIT_STATE(d->character) < 0)
                 GET_WAIT_STATE(d->character) = 0;
              if (GET_WAIT_STATE(d->character))
                 continue;
            }
	
         // Шоб в меню долго не сидели !
         if  (!get_from_q(&d->input, comm, &aliased))
 	     {if (STATE(d) != CON_PLAYING          &&
	          STATE(d) != CON_DISCONNECT       &&
	          time(NULL) - d->input_time > 300 &&
                  d->character                     &&
	          !IS_GOD(d->character)
		 )
                 close_socket(d,TRUE);
              continue;
	     }

         d->input_time = time(NULL);
         if (d->character)
            {/* Reset the idle timer & pull char back from void if necessary */
	     d->character->char_specials.timer = 0;
	     if (STATE(d) == CON_PLAYING && GET_WAS_IN(d->character) != NOWHERE)
	        {if (d->character->in_room != NOWHERE)
	            char_from_room(d->character);
	         char_to_room(d->character, GET_WAS_IN(d->character));
	         GET_WAS_IN(d->character) = NOWHERE;
	         act("$n вернул$u.", TRUE, d->character, 0, 0, TO_ROOM);
	         GET_WAIT_STATE(d->character) = 1;
	        }
            }
         d->has_prompt = 0;
      /*
       * I reversed these top 2 if checks so that you can use the
       */
         if (d->showstr_count) /* Reading something w/ pager */
            show_string(d, comm);
         else
         if (d->str)
	    string_add(d, comm);
         else
         if (STATE(d) != CON_PLAYING) /* In menus, etc. */
	    nanny(d, comm);
         else
            {/* else: we're playing normally. */
	     if (aliased) 	/* To prevent recursive aliases. */
	        d->has_prompt = 1;	/* To get newline before next cmd output. */
 	     else
	     if (perform_alias(d, comm))    /* Run it through aliasing system */
	        get_from_q(&d->input, comm, &aliased);
	     command_interpreter(d->character, comm); /* Send it to interpreter */
            }
        }

    /* Send queued output out to the operating system (ultimately to user). */
    for (d = descriptor_list; d; d = next_d)
        {next_d = d->next;
         if (*(d->output) && FD_ISSET(d->descriptor, &output_set))
            {/* Output for this player is ready */
	         if (process_output(d) < 0)
	            close_socket(d,FALSE);
	         else
	            d->has_prompt = 1;
            }
        }

    /* Print prompts for other descriptors who had no other output */
    for (d = descriptor_list; d; d = d->next)
        {if (!d->has_prompt)
            {sprintf(buf,"\r\n%s",make_prompt(d)); // tyt byl prool:
             //from_koi(buf, d->keytable);
             //write_to_descriptor(d->descriptor, buf);
		SEND_TO_Q(make_prompt(d),d);
	         d->has_prompt = 1;
            }
        }

    /* Kick out folks in the CON_CLOSE or CON_DISCONNECT state */
    for (d = descriptor_list; d; d = next_d)
        {next_d = d->next;
         if (STATE(d) == CON_CLOSE || STATE(d) == CON_DISCONNECT)
            close_socket(d,FALSE);
         }

    /*
     * Now, we execute as many pulses as necessary--just one if we haven't
     * missed any pulses, or make up for lost time if we missed a few
     * pulses by sleeping for too long.
     */
    missed_pulses++;

    if (missed_pulses <= 0)
       {log("SYSERR: **BAD** MISSED_PULSES NONPOSITIVE (%d), TIME GOING BACKWARDS!!", missed_pulses);
        missed_pulses = 1;
       }

    /* If we missed more than 30 seconds worth of pulses, just do 30 secs */
    if (missed_pulses > (30 * PASSES_PER_SEC))
       {log("SYSERR: Missed %d seconds worth of pulses.", missed_pulses / PASSES_PER_SEC);
        missed_pulses = 30 * PASSES_PER_SEC;
       }

    /* Now execute the heartbeat functions */
    while (missed_pulses--)
          heartbeat(++pulse);

    // dupe_player_index();
    // _exit(0);


    /* Roll pulse over after 10 hours */
    if (pulse >= (600 * 60 * PASSES_PER_SEC))
      pulse = 0;

#ifdef CIRCLE_UNIX
    /* Update tics for deadlock protection (UNIX only) */
    tics++;
#endif
  }
}

void beat_points_update(int pulse);
#define FRAC_SAVE TRUE

void heartbeat(int pulse)
{ struct char_data *ch=NULL, *k, *next;
  static int mins_since_crashsave = 0;
  long   check_at = 0, save_start = 0;

  void process_events(void);

  dg_global_pulse++;

  //log("---------- Start heartbeat ----------");
  //log("Process events...");
    process_events();
  //log("Stop it...");

  if (!(pulse % PULSE_DG_SCRIPT))
     {//log("Triggers check...");
      script_trigger_check();
      //log("Stop it...");
     }

  if (!(pulse % (30 * PASSES_PER_SEC)))
     {//log("Sanity check...");
      sanity_check();
      //log("Stop it...");
     }

  /*** Remove after hour update
  if (!(pulse % PULSE_ZONE))
     {//log("Zone update...");
      zone_update();
     }
   ****************************/

  if (!(pulse % (40 * PASSES_PER_SEC)))		/* 40 seconds */
     {//log("Check idle password...");
      check_idle_passwords();
      //log("Stop it...");
     }
  /* Old proc
  if (!(pulse % PULSE_MOBILE))
     {// log("Mobile activity...");
      mobile_activity();
     }
   */
  //log("Mobile activity...");
  mobile_activity(pulse % PULSE_MOBILE);
  //log("Stop it...");

  if (!(pulse % (2 * PASSES_PER_SEC)))
     {//log("Death trap activity...");
      dt_activity();
      //log("Stop it...");
     }

  if (!(pulse % PULSE_VIOLENCE))
     {//log("Perform violence...");
      perform_violence();
      //log("Stop it...");
     }

  if (pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC) == (SECS_PER_MUD_HOUR - 10) * PASSES_PER_SEC)
     {//log("Tics update...");
      tics_update();
      //log("Stop it...");
     }

  if (!(pulse % (AUCTION_PULSES * PASSES_PER_SEC)))
     {//log("Auction update...");
      tact_auction();
      //log("Stop it...");
     }

  if (!(pulse % (SECS_PER_PLAYER_AFFECT * PASSES_PER_SEC)))
     {//log ("Player affect update...");
      player_affect_update();
      //log("Stop it...");
     }

  if (!(pulse % (TIME_KOEFF * SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
     {//log("Hour msg update...");
      hour_update();
      //log("Stop it...");
      //log("Weather and time...");
      weather_and_time(1);
      //log("Stop it...");
      //log("Paste mobiles...");
      paste_mobiles(-1);
      //log("Stop it...");
     }

  if (!(pulse % PULSE_ZONE))
     {//log("Zone update...");
      zone_update();
      //log("Stop it...");
     }


  if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
     {//log("Affect update...");
      mobile_affect_update();
      //log("Stop it...");
      //log("Point update...");
      point_update();
      //log("Stop it...");
      //log("Save player index...");
      if (!USE_SINGLE_PLAYER)
         {fflush(player_fl);
          fflush(pkiller_fl);
         }
      else
         flush_player_index();
      //log("Stop it...");
     }

   if (pulse == 720 && USE_SINGLE_PLAYER)
      {//log("Dupe player index...");
       dupe_player_index();
       //log("Stop it...");
      }

  //log("Beat points update...");
  if (!(pulse % PASSES_PER_SEC))
     beat_points_update(pulse / PASSES_PER_SEC);
  //  log("Stop it...");

  if (FRAC_SAVE && auto_save && !(pulse % PASSES_PER_SEC))
     {// 1 game secunde
      //log("Fractional Crash save all...");
      Crash_frac_save_all((pulse / PASSES_PER_SEC) % PLAYER_SAVE_ACTIVITY);
      //log("Stop it...");
      //log("Fractional Rent timer save all...");
      Crash_frac_rent_time((pulse / PASSES_PER_SEC) % OBJECT_SAVE_ACTIVITY);
      //log("Stop it...");
     }

  if (!FRAC_SAVE && auto_save && !(pulse % (60 * PASSES_PER_SEC)))
     {// 1 minute
      if (++mins_since_crashsave >= autosave_time)
         {mins_since_crashsave = 0;
          //log("Crash save all...");
          Crash_save_all();
          //log("Stop it...");
          check_at = time(NULL);
          if (last_rent_check > check_at)
             last_rent_check = check_at;
          if (((check_at - last_rent_check) / 60))
             {//log("Crash rent time...");
              save_start = time(NULL);
              Crash_rent_time((check_at - last_rent_check) / 60);
              //log("Saving rent timer time = %ld(s)",time(NULL) - save_start);
              last_rent_check = time(NULL) - (check_at - last_rent_check) % 60;
	      //log("Stop it...");
             }
         }
     }

  if (auto_save && !(pulse % (60 * PASSES_PER_SEC)))
     {//log("House save all...");
      House_save_all();
      //log("Stop it...");
     }

  if (!(pulse % (5 * 60 * PASSES_PER_SEC)))	/* 5 minutes */
     {//log("Record usage...");
      record_usage();
      //log("Stop it...");
     }
  //log ("Real extract char");
  for (k = character_list; k; k = next)
      {next = k->next;
       if (IN_ROOM(k) == NOWHERE || MOB_FLAGGED(k, MOB_DELETE))
          {//log("[Heartbeat] Remove from list char %s", GET_NAME(k));
           if (k == character_list)
              character_list = next;
           else
              ch->next = next;
           k->next = NULL;
           if (MOB_FLAGGED(k,MOB_FREE))
              {//log("[Heartbeat] Free char %s", GET_NAME(k));
               free_char(k);
               //log("[Heartbeat] OK free char");
              }
          }
       else
          ch = k;
      }
  //log("---------- Stop heartbeat ----------");
}


/* ******************************************************************
*  general utility stuff (for local use)                            *
****************************************************************** */

/*
 *  new code to calculate time differences, which works on systems
 *  for which tv_usec is unsigned (and thus comparisons for something
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 */
void timediff(struct timeval *rslt, struct timeval *a, struct timeval *b)
{
  if (a->tv_sec < b->tv_sec)
    *rslt = null_time;
  else if (a->tv_sec == b->tv_sec) {
    if (a->tv_usec < b->tv_usec)
      *rslt = null_time;
    else {
      rslt->tv_sec = 0;
      rslt->tv_usec = a->tv_usec - b->tv_usec;
    }
  } else {			/* a->tv_sec > b->tv_sec */
    rslt->tv_sec = a->tv_sec - b->tv_sec;
    if (a->tv_usec < b->tv_usec) {
      rslt->tv_usec = a->tv_usec + 1000000 - b->tv_usec;
      rslt->tv_sec--;
    } else
      rslt->tv_usec = a->tv_usec - b->tv_usec;
  }
}

/*
 * Add 2 time values.
 *
 * Patch sent by "d. hall" <dhall@OOI.NET> to fix 'static' usage.
 */
void timeadd(struct timeval *rslt, struct timeval *a, struct timeval *b)
{
  rslt->tv_sec = a->tv_sec + b->tv_sec;
  rslt->tv_usec = a->tv_usec + b->tv_usec;

  while (rslt->tv_usec >= 1000000) {
    rslt->tv_usec -= 1000000;
    rslt->tv_sec++;
  }
}


void record_usage(void)
{
  int sockets_connected = 0, sockets_playing = 0;
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    sockets_connected++;
    if (STATE(d) == CON_PLAYING)
      sockets_playing++;
  }

  log("nusage: %-3d sockets connected, %-3d sockets playing",
	  sockets_connected, sockets_playing);

#ifdef RUSAGE	/* Not RUSAGE_SELF because it doesn't guarantee prototype. */
  {
    struct rusage ru;

    getrusage(RUSAGE_SELF, &ru);
    log("rusage: user time: %ld sec, system time: %ld sec, max res size: %ld",
	    ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
  }
#endif

}



/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(struct descriptor_data *d)
{
  char off_string[] =
  {
    (char) IAC,
    (char) WILL,
    (char) TELOPT_ECHO,
    (char) 0
  };

  SEND_TO_Q(off_string, d);
}


/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(struct descriptor_data *d)
{
  char on_string[] =
  {
    (char) IAC,
    (char) WONT,
    (char) TELOPT_ECHO,
    (char) IAC,
    (char) WONT,
    (char) TELOPT_NAOFFD,
    (char) IAC,
    (char) WONT,
    (char) TELOPT_NAOCRD,
    (char) 0,
  };

  SEND_TO_Q(on_string, d);
}

int  posi_value(int real, int max)
{ if (real < 0)
     return (-1);
  else
  if (real >= max)
     return (10);

  return (real * 10 / MAX(max,1));
}

char *color_value(struct char_data *ch, int real, int max)
{static char color[256];
 switch (posi_value(real,max))
 {case -1 : case 0 : case 1 :
      sprintf(color,"%s",CCRED(ch,C_NRM)); break;
  case 2 : case 3 :
      sprintf(color,"%s",CCIRED(ch,C_NRM)); break;
  case 4 : case 5 :
      sprintf(color,"%s",CCIYEL(ch,C_NRM)); break;
  case 6 : case 7 : case 8 :
      sprintf(color,"%s",CCIGRN(ch,C_NRM)); break;
  default:
      sprintf(color,"%s",CCGRN(ch,C_NRM)); break;
 }
 return (color);
}

/*
char *show_state(struct char_data *ch, struct char_data *victim)
{ int ch_hp = 11;
  static char *WORD_STATE[12] =
              {"Умирает",
               "Ужасное",
               "О.Плохое",
               "Плохое",
               "Плохое",
               "Среднее",
               "Среднее",
               "Хорошее",
               "Хорошее",
               "Хорошее",
               "О.хорошее",
               "Велиполепное"};

  ch_hp = posi_value(GET_HIT(victim),GET_REAL_MAX_HIT(victim)) + 1;
  sprintf(buf, "%s[%s:%s]%s ",
          color_value(ch, GET_HIT(victim), GET_REAL_MAX_HIT(victim)),
          PERS(victim,ch,0),
          WORD_STATE[ch_hp],
          CCNRM(ch, C_NRM));
  return buf;
}
*/

char *show_state(struct char_data *ch, struct char_data *victim)
{ int ch_hp = 11;
  static char *WORD_STATE[12] =
              {"Смертельно ранен",
               "О.тяжело ранен",
               "О.тяжело ранен",
               "Тяжело ранен",
               "Тяжело ранен",
               "Ранен",
               "Ранен",
               "Ранен",
               "Легко ранен",
               "Легко ранен",
               "Слегка ранен",
               "Невредим"};

  ch_hp = posi_value(GET_HIT(victim),GET_REAL_MAX_HIT(victim)) + 1;
  sprintf(buf, "%s[%s:%s%s]%s ",
          color_value(ch, GET_HIT(victim), GET_REAL_MAX_HIT(victim)),
          PERS(victim,ch,0),
          WORD_STATE[ch_hp],GET_CH_SUF_6(victim),
          CCNRM(ch, C_NRM));
  return buf;
}





char *make_prompt(struct descriptor_data *d)
{
  static char prompt[MAX_PROMPT_LENGTH + 1];
  static char *dirs[] = {"С","В","Ю","З","^","v"};

  int ch_hp, vict_hp;
  int door;

  /* Note, prompt is truncated at MAX_PROMPT_LENGTH chars (structs.h )*/
  if (d->showstr_count)
     sprintf(prompt,
 	         "\rЛистать : <RETURN>, Q<К>онец, R<П>овтор, B<Н>азад, или номер страницы (%d/%d).",
 	         d->showstr_page, d->showstr_count);
  else
  if (d->str)
    strcpy(prompt, "] ");
  else
  if (STATE(d) == CON_PLAYING && !IS_NPC(d->character))
    {int count = 0;
     *prompt = '\0';

     // Invisibitity
     if (GET_INVIS_LEV(d->character))
        count += sprintf(prompt + count, "i%d ", GET_INVIS_LEV(d->character));

     // Hits state
     if (PRF_FLAGGED(d->character, PRF_DISPHP))
        {count += sprintf(prompt + count, "%s", color_value(d->character,GET_HIT(d->character),GET_REAL_MAX_HIT(d->character)));
         count += sprintf(prompt + count, "%dH%s ", GET_HIT(d->character), CCNRM(d->character, C_NRM));
        }
     // Moves state
     if (PRF_FLAGGED(d->character, PRF_DISPMOVE))
        {count += sprintf(prompt + count, "%s", color_value(d->character,GET_MOVE(d->character),GET_REAL_MAX_MOVE(d->character)));
         count += sprintf(prompt + count, "%dM%s ",
                  GET_MOVE(d->character), CCNRM(d->character, C_NRM));
        }

      // Expirience
      // if (PRF_FLAGGED(d->character, PRF_DISPEXP))
      //    count += sprintf(prompt + count, "%ldx ", GET_EXP(d->character));
      if (PRF_FLAGGED(d->character, PRF_DISPEXP))
         {if (IS_IMMORTAL(d->character))
             count += sprintf(prompt + count, " ??? ");
          else
             count += sprintf(prompt + count, " %ldо ",
                              level_exp(GET_CLASS(d->character), GET_LEVEL(d->character)+1) -
                              GET_EXP(d->character));
         }
      // Mem Info
      if (PRF_FLAGGED(d->character, PRF_DISPMANA))
         {if ((ch_hp = GET_MANA_NEED(d->character)))
             {door = mana_gain(d->character);
              if (door)
                 {vict_hp = GET_MANA_STORED(d->character);
                  ch_hp   = (ch_hp - vict_hp) / door +
                            ((ch_hp - vict_hp) % door > 0);
                  count += sprintf(prompt+count,"Зауч:%d ",ch_hp);
                 }
              else
                 count += sprintf(prompt+count,"Зауч:- ");
             }
          else
             count += sprintf(prompt+count,"Зауч:0 ");
         }


      if (!FIGHTING(d->character) || IN_ROOM(d->character) != IN_ROOM(FIGHTING(d->character)))
         {/* SHOW NON COMBAT INFO */

          if (PRF_FLAGGED(d->character, PRF_DISPLEVEL))
             count += sprintf(prompt + count, "%dL ",
                              GET_LEVEL(d->character));

          if (PRF_FLAGGED(d->character, PRF_DISPGOLD))
             count += sprintf(prompt + count, "%dG ", GET_GOLD(d->character));

          if (PRF_FLAGGED(d->character, PRF_DISPEXITS))
             {count += sprintf(prompt + count,"Вых:");
              for (door = 0; door < NUM_OF_DIRS; door++)
                  {if (EXIT(d->character, door) &&
                       EXIT(d->character, door)->to_room != NOWHERE &&
                       !EXIT_FLAGGED(EXIT(d->character, door), EX_HIDDEN))
                      count += EXIT_FLAGGED(EXIT(d->character, door), EX_CLOSED) ?
      	                       sprintf(prompt + count, "(%s)", dirs[door]) :
     	                       sprintf(prompt + count, "%s", dirs[door]);
                  }
             }
         }
      else
         {count += sprintf(prompt + count, "%s", show_state(d->character, d->character));
          if (FIGHTING(FIGHTING(d->character)) && FIGHTING(FIGHTING(d->character)) != d->character)
             count += sprintf(prompt + count, "%s", show_state(d->character, FIGHTING(FIGHTING(d->character))));
          count += sprintf(prompt + count, "%s", show_state(d->character, FIGHTING(d->character)));
         };
      strcat(prompt, "> ");
     }
  else
  if (STATE(d) == CON_PLAYING && IS_NPC(d->character))
     sprintf(prompt, "{%s}-> ", GET_NAME(d->character));
  else
     *prompt = '\0';

  return (prompt);
}


void write_to_q(const char *txt, struct txt_q *queue, int aliased)
{
  struct txt_block *newt;

  CREATE(newt, struct txt_block, 1);
  newt->text = str_dup(txt);
  newt->aliased = aliased;

  /* queue empty? */
  if (!queue->head) {
    newt->next = NULL;
    queue->head = queue->tail = newt;
  } else {
    queue->tail->next = newt;
    queue->tail = newt;
    newt->next = NULL;
  }
}



int get_from_q(struct txt_q *queue, char *dest, int *aliased)
{
  struct txt_block *tmp;

  /* queue empty? */
  if (!queue->head)
    return (0);

  tmp = queue->head;
  strcpy(dest, queue->head->text);
  *aliased = queue->head->aliased;
  queue->head = queue->head->next;

  free(tmp->text);
  free(tmp);

  return (1);
}



/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data *d)
{
  int dummy;

  if (d->large_outbuf) {
    d->large_outbuf->next = bufpool;
    bufpool = d->large_outbuf;
  }
  while (get_from_q(&d->input, buf2, &dummy));
}

/* Add a new string to a player's output queue */
void write_to_output(const char *txt, struct descriptor_data *t)
{
  int size;

  /* if we're in the overflow state already, ignore this new output */
  if (t->bufptr < 0)
    return;

  if ((ubyte)*txt == 255)
     {return;
     }

  size = strlen(txt);

  /* if we have enough space, just write to buffer and that's it! */
  if (t->bufspace >= size)
     {strcpy(t->output + t->bufptr, txt);
      t->bufspace -= size;
      t->bufptr += size;
      return;
     }
  /*
   * If the text is too big to fit into even a large buffer, chuck the
   * new text and switch to the overflow state.
   */
  if (size + t->bufptr > LARGE_BUFSIZE - 1)
     {t->bufptr = -1;
      buf_overflows++;
      return;
     }
  buf_switches++;

  /* if the pool has a buffer in it, grab it */
  if (bufpool != NULL)
     {t->large_outbuf = bufpool;
      bufpool = bufpool->next;
     }
  else
     {/* else create a new one */
      CREATE(t->large_outbuf, struct txt_block, 1);
      CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
      buf_largecount++;
     }

  strcpy(t->large_outbuf->text, t->output);	/* copy to big buffer */
  t->output = t->large_outbuf->text;	/* make big buffer primary */
  strcat(t->output, txt);	/* now add new text */

  /* set the pointer for the next write */
  t->bufptr = strlen(t->output);
  /* calculate how much space is left in the buffer */
  t->bufspace = LARGE_BUFSIZE - 1 - t->bufptr;
}



/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */


/*
 * get_bind_addr: Return a struct in_addr that should be used in our
 * call to bind().  If the user has specified a desired binding
 * address, we try to bind to it; otherwise, we bind to INADDR_ANY.
 * Note that inet_aton() is preferred over inet_addr() so we use it if
 * we can.  If neither is available, we always bind to INADDR_ANY.
 */

struct in_addr *get_bind_addr()
{
  static struct in_addr bind_addr;

  /* Clear the structure */
  memset((char *) &bind_addr, 0, sizeof(bind_addr));

  /* If DLFT_IP is unspecified, use INADDR_ANY */
  if (DFLT_IP == NULL) {
    bind_addr.s_addr = htonl(INADDR_ANY);
  } else {
    /* If the parsing fails, use INADDR_ANY */
    if (!parse_ip(DFLT_IP, &bind_addr)) {
      log("SYSERR: DFLT_IP of %s appears to be an invalid IP address",DFLT_IP);
      bind_addr.s_addr = htonl(INADDR_ANY);
    }
  }

  /* Put the address that we've finally decided on into the logs */
  if (bind_addr.s_addr == htonl(INADDR_ANY))
    log("Binding to all IP interfaces on this host.");
  else
    log("Binding only to IP address %s", inet_ntoa(bind_addr));

  return (&bind_addr);
}

#ifdef HAVE_INET_ATON

/*
 * inet_aton's interface is the same as parse_ip's: 0 on failure, non-0 if
 * successful
 */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
  return (inet_aton(addr, inaddr));
}

#elif HAVE_INET_ADDR

/* inet_addr has a different interface, so we emulate inet_aton's */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
  long ip;

  if ((ip = inet_addr(addr)) == -1) {
    return (0);
  } else {
    inaddr->s_addr = (unsigned long) ip;
    return (1);
  }
}

#else

/* If you have neither function - sorry, you can't do specific binding. */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
  log("SYSERR: warning: you're trying to set DFLT_IP but your system has no\n"
      "functions to parse IP addresses (how bizarre!)");
  return (0);
}

#endif /* INET_ATON and INET_ADDR */



/* Sets the kernel's send buffer size for the descriptor */
int set_sendbuf(socket_t s)
{
#if defined(SO_SNDBUF) && !defined(CIRCLE_MACINTOSH)
  int opt = MAX_SOCK_BUF;

  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt SNDBUF");
    return (-1);
  }

#if 0
  if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *) &opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt RCVBUF");
    return (-1);
  }
#endif

#endif

  return (0);
}

int new_descriptor(int s)
{
  socket_t desc;
  int sockets_connected = 0;
  socklen_t i;
  static int last_desc = 0;	/* last descriptor number */
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  struct hostent *from;

  /* accept the new connection */
  i = sizeof(peer);
  if ((desc = accept(s, (struct sockaddr *) &peer, &i)) == INVALID_SOCKET)
     {perror("SYSERR: accept");
      return (-1);
     }
  /* keep it from blocking */
  nonblock(desc);

  /* set the send buffer size */
  if (set_sendbuf(desc) < 0)
     {CLOSE_SOCKET(desc);
      return (0);
     }

  /* make sure we have room for it */
  for (newd = descriptor_list; newd; newd = newd->next)
      sockets_connected++;

  if (sockets_connected >= max_players)
     {write_to_descriptor(desc, "Sorry, RUS MUD is full right now... please try again later!\r\n");
      CLOSE_SOCKET(desc);
      return (0);
     }
  /* create a new descriptor */
  CREATE(newd, struct descriptor_data, 1);
  memset((char *) newd, 0, sizeof(struct descriptor_data));

  /* find the sitename */
  if (nameserver_is_slow ||
      !(from = gethostbyaddr((char *) &peer.sin_addr, sizeof(peer.sin_addr), AF_INET)))
     {/* resolution failed */
      if (!nameserver_is_slow)
         perror("SYSERR: gethostbyaddr");

      /* find the numeric site address */
      strncpy(newd->host, (char *)inet_ntoa(peer.sin_addr), HOST_LENGTH);
      *(newd->host + HOST_LENGTH) = '\0';
     }
  else
     {strncpy(newd->host, from->h_name, HOST_LENGTH);
      *(newd->host + HOST_LENGTH) = '\0';
     }

  /* determine if the site is banned */
  if (isbanned(newd->host) == BAN_ALL)
     {CLOSE_SOCKET(desc);
      sprintf(buf2, "Connection attempt denied from [%s]", newd->host);
      mudlog(buf2, CMP, LVL_GOD, TRUE);
      free(newd);
      return (0);
     }
#if 0
  /*
   * Log new connections - probably unnecessary, but you may want it.
   * Note that your immortals may wonder if they see a connection from
   * your site, but you are wizinvis upon login.
   */
  sprintf(buf2, "New connection from [%s]", newd->host);
  mudlog(buf2, CMP, LVL_GOD, FALSE);
#endif

  /* initialize descriptor data */
  newd->descriptor = desc;
  newd->idle_tics  = 0;
  newd->output     = newd->small_outbuf;
  newd->bufspace   = SMALL_BUFSIZE - 1;
  newd->input_time = time(0);
  newd->login_time = time(0);
  *newd->output    = '\0';
  newd->bufptr     = 0;
  newd->has_prompt = 1;  /* prompt is part of greetings */
  newd->keytable   = KT_SELECTMENU;
  STATE(newd)      = CON_GET_KEYTABLE;
  /*
   * This isn't exactly optimal but allows us to make a design choice.
   * Do we embed the history in descriptor_data or keep it dynamically
   * allocated and allow a user defined history size?
   */
  CREATE(newd->history, char *, HISTORY_SIZE);

  if (++last_desc == 1000)
    last_desc = 1;
  newd->desc_num = last_desc;

  /* prepend to list */
  newd->next = descriptor_list;
  descriptor_list = newd;

  SEND_TO_Q("Byliny-0. Compiled by prool (www.prool.kharkov.org)\r\n\r\n"
	    "Using keytable\r\n"
            "  0) Koi-8\r\n"
            "  1) Alt\r\n"
            "  2) Windows(JMC)\r\n"
            "  3) Windows(zMUD)\r\n"
            "  4) Windows(zMUD ver. 6+)\r\n"            
            "  5) UTF-8\r\n"            
            "Select one : ", newd);
  return (0);
}

/*
 * Send all of the output that we've accumulated for a player out to
 * the player's descriptor.
 * FIXME - This will be rewritten before 3.1, this code is dumb.
 */
int process_output(struct descriptor_data *t)
{
  char i[MAX_SOCK_BUF], o[MAX_SOCK_BUF*2], *pi, *po;
  int  result, c;

  pi = i;
  po = o;
  /* we may need this \r\n for later -- see below */
  strcpy(i, "\r\n");

  /* now, append the 'real' output */
  strcpy(i + 2, t->output);

  /* if we're in the overflow state, notify the user */
  if (t->bufptr < 0)
     strcat(i, "**OVERFLOW**\r\n");

  /* add the extra CRLF if the person isn't in compact mode */
  if (STATE(t) == CON_PLAYING &&
      t->character &&
      !IS_NPC(t->character) &&
      !PRF_FLAGGED(t->character, PRF_COMPACT))
    strcat(i + 2, "\r\n");

  /* add a prompt */
  strncat(i + 2, make_prompt(t), MAX_PROMPT_LENGTH);

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */

  for (c=0;*(pi+c);c++)
        *(pi+c) = (*(pi+c) == '_') ? ' ' : *(pi+c);

  switch(t->keytable)
  {case KT_ALT : for(;*pi;*po = KtoA(*pi), pi++, po++);
        break;
   case KT_WIN : for(;*pi;*po = KtoW(*pi), pi++, po++)
                   if (*pi == 'я')
                      *(po++) = 255;
        break;
   case KT_WINZ: for(;*pi;*po = KtoW2(*pi), pi++, po++);
        break;
   case KT_WINZ6: for(;*pi;*po = KtoW(*pi), pi++, po++)
                   if (*pi == 'я')
                      {*(po++) = 255;
                       *(po++) = 255;
                      }
        break;
   case KT_UTF8: koi_to_utf8(pi,po); // prool
	break;
   default     : for(;*pi;*po = *pi, pi++, po++);
        break;
  }
  if (t->keytable!=KT_UTF8)  *po = '\0'; // prool

  if (t->has_prompt)		/* && !t->connected) */
    {result = write_to_descriptor(t->descriptor, o);
    }
  else
    {result = write_to_descriptor(t->descriptor, o + 2);
    }

  /* handle snooping: prepend "% " and send to snooper */
  if (t->snoop_by)
     {SEND_TO_Q("% ", t->snoop_by);
      SEND_TO_Q(t->output, t->snoop_by);
      SEND_TO_Q("%%", t->snoop_by);
     }
  /*
   * if we were using a large buffer, put the large buffer on the buffer pool
   * and switch back to the small one
   */
  if (t->large_outbuf)
     {t->large_outbuf->next = bufpool;
      bufpool               = t->large_outbuf;
      t->large_outbuf       = NULL;
      t->output             = t->small_outbuf;
     }
  /* reset total bufspace back to that of a small buffer */
  t->bufspace  = SMALL_BUFSIZE - 1;
  t->bufptr    = 0;
  *(t->output) = '\0';

  return (result);
}


/*
 * perform_socket_write: takes a descriptor, a pointer to text, and a
 * text length, and tries once to send that text to the OS.  This is
 * where we stuff all the platform-dependent stuff that used to be
 * ugly #ifdef's in write_to_descriptor().
 *
 * This function must return:
 *
 * -1  If a fatal error was encountered in writing to the descriptor.
 *  0  If a transient failure was encountered (e.g. socket buffer full).
 * >0  To indicate the number of bytes successfully written, possibly
 *     fewer than the number the caller requested be written.
 *
 * Right now there are two versions of this function: one for Windows,
 * and one for all other platforms.
 */

#if defined(CIRCLE_WINDOWS)

ssize_t perform_socket_write(socket_t desc, const char *txt, size_t length)
{
  ssize_t result;

  result = send(desc, txt, length, 0);

  if (result > 0) {
    /* Write was sucessful */
    return (result);
  }

  if (result == 0) {
    /* This should never happen! */
    log("SYSERR: Huh??  write() returned 0???  Please report this!");
    return (-1);
  }

  /* result < 0: An error was encountered. */

  /* Transient error? */
  if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINTR)
    return (0);

  /* Must be a fatal error. */
  return (-1);
}

#else

#if defined(CIRCLE_ACORN)
#define write	socketwrite
#endif

/* perform_socket_write for all Non-Windows platforms */
ssize_t perform_socket_write(socket_t desc, const char *txt, size_t length)
{
  ssize_t result;

  result = write(desc, txt, length);

  if (result > 0) {
    /* Write was successful. */
    return (result);
  }

  if (result == 0) {
    /* This should never happen! */
    log("SYSERR: Huh??  write() returned 0???  Please report this!");
    return (-1);
  }

  /*
   * result < 0, so an error was encountered - is it transient?
   * Unfortunately, different systems use different constants to
   * indicate this.
   */

#ifdef EAGAIN		/* POSIX */
  if (errno == EAGAIN)
    return (0);
#endif

#ifdef EWOULDBLOCK	/* BSD */
  if (errno == EWOULDBLOCK)
    return (0);
#endif

#ifdef EDEADLK		/* Macintosh */
  if (errno == EDEADLK)
    return (0);
#endif

  /* Looks like the error was fatal.  Too bad. */
  return (-1);
}

#endif /* CIRCLE_WINDOWS */


/*
 * write_to_descriptor takes a descriptor, and text to write to the
 * descriptor.  It keeps calling the system-level write() until all
 * the text has been delivered to the OS, or until an error is
 * encountered.
 *
 * Returns:
 *  0  If all is well and good,
 * -1  If an error was encountered, so that the player should be cut off
 */
int write_to_descriptor(socket_t desc, const char *txt)
{
  size_t total;
  ssize_t bytes_written;

  total = strlen(txt);

  while (total > 0) {
    bytes_written = perform_socket_write(desc, txt, total);

    if (bytes_written < 0) {
      /* Fatal error.  Disconnect the player. */
      perror("SYSERR: Write to socket");
      return (-1);
    } else if (bytes_written == 0) {
      /*
       * Temporary failure -- socket buffer full.  For now we'll just
       * cut off the player, but eventually we'll stuff the unsent
       * text into a buffer and retry the write later.  JE 30 June 98.
       */
      log("WARNING: write_to_descriptor: socket write would block, about to close");
      return (-1);
    } else {
      txt += bytes_written;
      total -= bytes_written;
    }
  }

  return (0);
}


/*
 * Same information about perform_socket_write applies here. I like
 * standards, there are so many of them. -gg 6/30/98
 */
ssize_t perform_socket_read(socket_t desc, char *read_point, size_t space_left)
{
  ssize_t ret;

#if defined(CIRCLE_ACORN)
  ret = recv(desc, read_point, space_left, MSG_DONTWAIT);
#elif defined(CIRCLE_WINDOWS)
  ret = recv(desc, read_point, space_left, 0);
#else
  ret = read(desc, read_point, space_left);
#endif

  /* Read was successful. */
  if (ret > 0)
    return (ret);

  /* read() returned 0, meaning we got an EOF. */
  if (ret == 0) {
    log("WARNING: EOF on socket read (connection broken by peer)");
    return (-1);
  }

  /*
   * read returned a value < 0: there was an error
   */

#if defined(CIRCLE_WINDOWS)	/* Windows */
  if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINTR)
    return (0);
#else

#ifdef EINTR		/* Interrupted system call - various platforms */
  if (errno == EINTR)
    return (0);
#endif

#ifdef EAGAIN		/* POSIX */
  if (errno == EAGAIN)
    return (0);
#endif

#ifdef EWOULDBLOCK	/* BSD */
  if (errno == EWOULDBLOCK)
    return (0);
#endif /* EWOULDBLOCK */

#ifdef EDEADLK		/* Macintosh */
  if (errno == EDEADLK)
    return (0);
#endif

#endif /* CIRCLE_WINDOWS */

  /*
   * We don't know what happened, cut them off. This qualifies for
   * a SYSERR because we have no idea what happened at this point.
   */
  perror("SYSERR: perform_socket_read: about to lose connection");
  return (-1);
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 */
int process_input(struct descriptor_data *t)
{
  int buf_length, failed_subst;
  ssize_t bytes_read;
  size_t space_left;
  char *ptr, *read_point, *write_point, *nl_pos = NULL;
  char tmp[MAX_INPUT_LENGTH];

  /* first, find the point where we left off reading data */
  buf_length = strlen(t->inbuf);
  read_point = t->inbuf + buf_length;
  space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

  do
   {if (space_left <= 0)
       {log("WARNING: process_input: about to close connection: input overflow");
        return (-1);
       }

    bytes_read = perform_socket_read(t->descriptor, read_point, space_left);

    if (bytes_read < 0)	/* Error, disconnect them. */
       {return (-1);
       }
    else
    if (bytes_read == 0)	/* Just blocking, no problems. */
       return (0);

    /* at this point, we know we got some data from the read */
    /***** NOT PROPER
    if ((ubyte)*read_point == 255)
       {*(read_point) = '\0';
        return (0);
       }
     *****/
    *(read_point + bytes_read) = '\0';	/* terminate the string */
    /***** NOT_PROPER
    to_koi(read_point,t->keytable);
     *****/
    /* search for a newline in the data we just read */
    for (ptr = read_point; *ptr && !nl_pos; ptr++)
        if (ISNEWL(*ptr))
	       nl_pos = ptr;

    read_point += bytes_read;
    space_left -= bytes_read;

/*
 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
 * causing the MUD to hang when it encounters input not terminated by a
 * newline.  This was causing hangs at the Password: prompt, for example.
 * I attempt to compensate by always returning after the _first_ read, instead
 * of looping forever until a read returns -1.  This simulates non-blocking
 * I/O because the result is we never call read unless we know from select()
 * that data is ready (process_input is only called if select indicates that
 * this descriptor is in the read set).  JE 2/23/95.
 */
#if !defined(POSIX_NONBLOCK_BROKEN)
  } while (nl_pos == NULL);
#else
  } while (0);

  if (nl_pos == NULL)
    return (0);
#endif /* POSIX_NONBLOCK_BROKEN */

  /*
   * okay, at this point we have at least one newline in the string; now we
   * can copy the formatted data to a new array for further processing.
   */

  read_point = t->inbuf;

  while (nl_pos != NULL)
        {write_point = tmp;
         space_left = MAX_INPUT_LENGTH - 1;

         for (ptr = read_point; (space_left > 1) && (ptr < nl_pos); ptr++)
             {if (*ptr == '\b' || *ptr == 127)
                 {/* handle backspacing or delete key */
	              if (write_point > tmp)
	                 {if (*(--write_point) == '$')
	                     {write_point--;
	                      space_left += 2;
	                     }
	                  else
	                     space_left++;
	                 }
                 }
              else
              if (isascii(*ptr) && isprint(*ptr))
                 {if ((*(write_point++) = *ptr) == '$')
                     {/* copy one character */
	                  *(write_point++) = '$';	/* if it's a $, double it */
	                  space_left -= 2;
	                 }
	              else
	                 space_left--;
                 }
               else
               if ( (ubyte) *ptr > 127 )
                  {switch (t->keytable)
	               {default:
 	                 t->keytable = 0;
	                case 0:
			case KT_UTF8:
	                 *(write_point++) = *ptr;
	      	         break;
   	                case KT_ALT:
 	      	         *(write_point++) = AtoK(*ptr);
  	      	         break;
	                case KT_WIN:
	                case KT_WINZ6:
	                 *(write_point++) = WtoK(*ptr);
	                 if (*ptr == (char) 255 && *(ptr+1) == (char) 255 && ptr+1 < nl_pos)
	                    ptr++;
	                 break;
	                case KT_WINZ:
	      	         *(write_point++) = WtoK(*ptr);
	      	         break;
	      	       }
       	          space_left--;
      	         }
             }

    *write_point = '\0';

		if (t->keytable == KT_UTF8)
		{
			int i;
			char utf8_tmp[MAX_SOCK_BUF * 2 * 3];
			size_t len_i, len_o;

			len_i = strlen(tmp);

			for (i = 0; i < MAX_SOCK_BUF * 2 * 3; i++)
			{
				utf8_tmp[i] = 0;
			}
			utf8_to_koi(tmp, utf8_tmp);
			len_o = strlen(utf8_tmp);
			strncpy(tmp, utf8_tmp, MAX_INPUT_LENGTH - 1);
			space_left = space_left + len_i - len_o;
		}

    if ((space_left <= 0) && (ptr < nl_pos))
       {char buffer[MAX_INPUT_LENGTH + 64];

        sprintf(buffer, "Line too long.  Truncated to:\r\n%s\r\n", tmp);
        if (write_to_descriptor(t->descriptor, buffer) < 0)
	       return (-1);
       }
    if (t->snoop_by)
       {SEND_TO_Q("% ", t->snoop_by);
        SEND_TO_Q(tmp, t->snoop_by);
        SEND_TO_Q("\r\n", t->snoop_by);
       }
    failed_subst = 0;

    if (*tmp == '!' && !(*(tmp + 1)))	
       /* Redo last command. */
       strcpy(tmp, t->last_input);
    else
    if (*tmp == '!' && *(tmp + 1))
       {char *commandln = (tmp + 1);
        int starting_pos = t->history_pos,
	    cnt = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

        skip_spaces(&commandln);
        for (; cnt != starting_pos; cnt--)
            {if (t->history[cnt] && is_abbrev(commandln, t->history[cnt]))
                {strcpy(tmp, t->history[cnt]);
	             strcpy(t->last_input, tmp);
                 SEND_TO_Q(tmp, t); SEND_TO_Q("\r\n", t);
	             break;
	            }
             if (cnt == 0)	/* At top, loop to bottom. */
	            cnt = HISTORY_SIZE;
            }
       }
    else
    if (*tmp == '^')
       {if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
	       strcpy(t->last_input, tmp);
       }
    else
       {strcpy(t->last_input, tmp);
        if (t->history[t->history_pos])
	       free(t->history[t->history_pos]);        /* Clear the old line. */
        t->history[t->history_pos] = str_dup(tmp);	/* Save the new. */
        if (++t->history_pos >= HISTORY_SIZE)	/* Wrap to top. */
	       t->history_pos = 0;
       }

    if (!failed_subst)
       write_to_q(tmp, &t->input, 0);

    /* find the end of this line */
    while (ISNEWL(*nl_pos))
          nl_pos++;

    /* see if there's another newline in the input buffer */
    read_point = ptr = nl_pos;
    for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
        if (ISNEWL(*ptr))
	       nl_pos = ptr;
   }

  /* now move the rest of the buffer up to the beginning for the next pass */
  write_point = t->inbuf;
  while (*read_point)
    *(write_point++) = *(read_point++);
  *write_point = '\0';

  return (1);
}



/* perform substitution for the '^..^' csh-esque syntax orig is the
 * orig string, i.e. the one being modified.  subst contains the
 * substition string, i.e. "^telm^tell"
 */
int perform_subst(struct descriptor_data *t, char *orig, char *subst)
{
  char newsub[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first string (the one
   * to be replaced
   */
  first = subst + 1;

  /* now find the second '^' */
  if (!(second = strchr(first, '^'))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return (1);
  }
  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return (1);
  }
  /* now, we construct the new string for output. */

  /* first, everything in the original, up to the string to be replaced */
  strncpy(newsub, orig, (strpos - orig));
  newsub[(strpos - orig)] = '\0';

  /* now, the replacement string */
  strncat(newsub, second, (MAX_INPUT_LENGTH - strlen(newsub) - 1));

  /* now, if there's anything left in the original after the string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(newsub, strpos + strlen(first), (MAX_INPUT_LENGTH - strlen(newsub) - 1));

  /* terminate the string in case of an overflow from strncat */
  newsub[MAX_INPUT_LENGTH - 1] = '\0';
  strcpy(subst, newsub);

  return (0);
}



void close_socket(struct descriptor_data *d, int direct)
{
  char buf[128];
  struct descriptor_data *temp;

  if (!direct && d->character && RENTABLE(d->character))
     return;

  REMOVE_FROM_LIST(d, descriptor_list, next);
  CLOSE_SOCKET(d->descriptor);
  flush_queues(d);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = NULL;

  if (d->snoop_by)
     {SEND_TO_Q("Your victim is no longer among us.\r\n", d->snoop_by);
      d->snoop_by->snooping = NULL;
     }
  /*. Kill any OLC stuff .*/
  switch(d->connected)
  { case CON_OEDIT:
    case CON_REDIT:
    case CON_ZEDIT:
    case CON_MEDIT:
    case CON_SEDIT:
    case CON_TRIGEDIT:
      cleanup_olc(d, CLEANUP_ALL);
    default:
      break;
  }

  if (d->character)
    {/*
      * Plug memory leak, from Eric Green.
      */
     if (!IS_NPC(d->character) && PLR_FLAGGED(d->character, PLR_MAILING) && d->str)
        {if (*(d->str))
            free(*(d->str));
         free(d->str);
        }
     if (STATE(d) == CON_PLAYING || STATE(d) == CON_DISCONNECT)
        {act("$n потерял$g связь.", TRUE, d->character, 0, 0, TO_ROOM);
         if (!IS_NPC(d->character))
            {save_char(d->character, NOWHERE);
	     Crash_ldsave(d->character);
	     sprintf(buf, "Closing link to: %s.", GET_NAME(d->character));
	     mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
            }
         d->character->desc = NULL;
        }
     else
        {sprintf(buf, "Losing player: %s.",
                 GET_NAME(d->character) ? GET_NAME(d->character) : "<null>");
         mudlog(buf, CMP, LVL_IMMORT, TRUE);
         free_char(d->character);
        }
    }
 else
    mudlog("Losing descriptor without char.", CMP, LVL_IMMORT, TRUE);

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && d->original->desc)
     d->original->desc = NULL;

  /* Clear the command history. */
  if (d->history)
     {int cnt;
      for (cnt = 0; cnt < HISTORY_SIZE; cnt++)
          if (d->history[cnt])
             free(d->history[cnt]);
      free(d->history);
     }

  if (d->showstr_head)
     free(d->showstr_head);
  if (d->showstr_count)
     free(d->showstr_vector);

  free(d);
}



void check_idle_passwords(void)
{
  struct descriptor_data *d, *next_d;

  for (d = descriptor_list; d; d = next_d)
      {next_d = d->next;
       if (STATE(d) != CON_PASSWORD && STATE(d) != CON_GET_NAME && STATE(d) != CON_GET_KEYTABLE)
          continue;
       if (!d->idle_tics)
          {d->idle_tics++;
           continue;
          }
       else
          {echo_on(d);
           SEND_TO_Q("\r\nTimed out... goodbye.\r\n", d);
           STATE(d) = CON_CLOSE;
          }
      }
}



/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

#if defined(CIRCLE_WINDOWS)

void nonblock(socket_t s)
{
  unsigned long val = 1;
  ioctlsocket(s, FIONBIO, &val);
}

#elif defined(CIRCLE_AMIGA)

void nonblock(socket_t s)
{
  long val = 1;
  IoctlSocket(s, FIONBIO, &val);
}

#elif defined(CIRCLE_ACORN)

void nonblock(socket_t s)
{
  int val = 1;
  socket_ioctl(s, FIONBIO, &val);
}

#elif defined(CIRCLE_VMS)

void nonblock(socket_t s)
{
  int val = 1;

  if (ioctl(s, FIONBIO, &val) < 0) {
    perror("SYSERR: Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}

#elif defined(CIRCLE_UNIX) || defined(CIRCLE_OS2) || defined(CIRCLE_MACINTOSH)

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void nonblock(socket_t s)
{
  int flags;

  flags = fcntl(s, F_GETFL, 0);
  flags |= O_NONBLOCK;
  if (fcntl(s, F_SETFL, flags) < 0) {
    perror("SYSERR: Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}

#endif  /* CIRCLE_UNIX || CIRCLE_OS2 || CIRCLE_MACINTOSH */


/* ******************************************************************
*  signal-handling functions (formerly signals.c).  UNIX only.      *
****************************************************************** */

#if defined(CIRCLE_UNIX) || defined(CIRCLE_MACINTOSH)

RETSIGTYPE reread_wizlists(int sig)
{
  mudlog("Signal received - rereading wizlists.", CMP, LVL_IMMORT, TRUE);
  reboot_wizlists();
}


RETSIGTYPE unrestrict_game(int sig)
{
  mudlog("Received SIGUSR2 - completely unrestricting game (emergent)",
	 BRF, LVL_IMMORT, TRUE);
  ban_list = NULL;
  circle_restrict = 0;
  num_invalid = 0;
}

#ifdef CIRCLE_UNIX

/* clean up our zombie kids to avoid defunct processes */
RETSIGTYPE reap(int sig)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);

  my_signal(SIGCHLD, reap);
}

RETSIGTYPE checkpointing(int sig)
{
  if (!tics)
     {log("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite loop suspected)");
      abort();
     }
  else
    tics = 0;
}

RETSIGTYPE hupsig(int sig)
{
  log("SYSERR: Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
  printf("SYSERR: Received SIGHUP, SIGINT, or SIGTERM.  Shutting down\n"); // prool
  exit(1);			/* perhaps something more elegant should
				 * substituted */
}

#endif	/* CIRCLE_UNIX */

/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

#ifndef POSIX
#define my_signal(signo, func) signal(signo, func)
#else
sigfunc *my_signal(int signo, sigfunc * func)
{
  struct sigaction act, oact;

  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
#ifdef SA_INTERRUPT
  act.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

  if (sigaction(signo, &act, &oact) < 0)
    return (SIG_ERR);

  return (oact.sa_handler);
}
#endif				/* POSIX */


void signal_setup(void)
{
#ifndef CIRCLE_MACINTOSH
  struct itimerval itime;
  struct timeval interval;

  /* user signal 1: reread wizlists.  Used by autowiz system. */
  my_signal(SIGUSR1, reread_wizlists);

  /*
   * user signal 2: unrestrict game.  Used for emergencies if you lock
   * yourself out of the MUD somehow.  (Duh...)
   */
  my_signal(SIGUSR2, unrestrict_game);

  /*
   * set up the deadlock-protection so that the MUD aborts itself if it gets
   * caught in an infinite loop for more than 3 minutes.
   */
  interval.tv_sec = 180;
  interval.tv_usec = 0;
  itime.it_interval = interval;
  itime.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &itime, NULL);
  my_signal(SIGVTALRM, checkpointing);

  /* just to be on the safe side: */
  my_signal(SIGHUP, hupsig);
  my_signal(SIGCHLD, reap);
#endif /* CIRCLE_MACINTOSH */
  my_signal(SIGINT, hupsig);
  my_signal(SIGTERM, hupsig);
  my_signal(SIGPIPE, SIG_IGN);
  my_signal(SIGALRM, SIG_IGN);
}

#endif	/* CIRCLE_UNIX || CIRCLE_MACINTOSH */

/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */
void send_stat_char(struct char_data *ch)
{char fline[256];
 sprintf(fline,"%d[%d]HP %d[%d]Mv %dG %dL ",
         GET_HIT(ch),GET_REAL_MAX_HIT(ch),
         GET_MOVE(ch),GET_REAL_MAX_MOVE(ch),
         GET_GOLD(ch),GET_LEVEL(ch));
 SEND_TO_Q(fline, ch->desc);
}

void send_to_char(const char *messg, struct char_data *ch)
{if (ch->desc && messg)
     SEND_TO_Q(messg, ch->desc);
}


void send_to_all(const char *messg)
{
  struct descriptor_data *i;

  if (messg == NULL)
    return;

  for (i = descriptor_list; i; i = i->next)
    if (STATE(i) == CON_PLAYING)
      SEND_TO_Q(messg, i);
}


void send_to_outdoor(const char *messg, int control)
{ int    room;
  struct descriptor_data *i;

  if (!messg || !*messg)
     return;

  for (i = descriptor_list; i; i = i->next)
      {if (STATE(i) != CON_PLAYING || i->character == NULL)
          continue;
       if (!AWAKE(i->character)    || !OUTSIDE(i->character))
          continue;
       room = IN_ROOM(i->character);
       if (!control                        ||
           (IS_SET(control, SUN_CONTROL)  &&
	    room       != NOWHERE         &&
	    SECT(room) != SECT_UNDERWATER &&
	    !AFF_FLAGGED(i->character, AFF_BLIND)
	   )                               ||
	   (IS_SET(control, WEATHER_CONTROL)    &&
	    room       != NOWHERE               &&
	    SECT(room) != SECT_UNDERWATER       &&
	    !ROOM_FLAGGED(room, ROOM_NOWEATHER) &&
            world[IN_ROOM(i->character)].weather.duration <= 0
	   )
	  )
          SEND_TO_Q(messg, i);
      }
}


void send_to_gods(const char *messg)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
     return;

  for (i = descriptor_list; i; i = i->next)
      {if (STATE(i) != CON_PLAYING || i->character == NULL)
          continue;
       if (!IS_GOD(i->character))
          continue;
       SEND_TO_Q(messg, i);
      }
}


void send_to_room(const char *messg, room_rnum room, int to_awake)
{
  struct char_data *i;

  if (messg == NULL)
     return;

  for (i = world[room].people; i; i = i->next_in_room)
      if (i->desc && !IS_NPC(i) && (!to_awake || AWAKE(i)))
         SEND_TO_Q(messg, i->desc);
}



const char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == NULL) i = ACTNULL; else i = (expression);


/* higher-level communication: the act() function */
void perform_act(const char *orig, struct char_data *ch, struct obj_data *obj,
		 const void *vict_obj, const struct char_data *to)
{
  const  char *i = NULL;
  char   lbuf[MAX_STRING_LENGTH], *buf;
  ubyte  padis;
  int    stopbyte;
  struct char_data *dg_victim = NULL;
  struct obj_data *dg_target = NULL;
  char  *dg_arg = NULL;

  buf = lbuf;

  for (stopbyte=0;stopbyte < MAX_STRING_LENGTH;stopbyte++)
      {if (*orig == '$')
          {switch (*(++orig))
           {case 'n':
                 if (*(orig+1) < '0' || *(orig+1) > '5')
                    i = PERS(ch, to, 0);
                 else
                    {padis = *(++orig) - '0';
                     i = PERS(ch, to, padis);
                    }
                 break;
            case 'N':
                 if (*(orig+1) < '0' || *(orig+1) > '5')
                    {CHECK_NULL(vict_obj, PERS((const struct char_data *) vict_obj,to,0));
                    }
                 else
                    {padis = *(++orig) - '0';
                     CHECK_NULL(vict_obj, PERS((const struct char_data *) vict_obj,to,padis));
                    }
                 dg_victim = (struct char_data *) vict_obj;
                 break;
	
            case 'm':
                 i = HMHR(ch);
                 break;
            case 'M':
                 if (vict_obj)
                    i = HMHR((const struct char_data *) vict_obj);
                 else
                    CHECK_NULL(obj,OMHR(obj));
                 dg_victim = (struct char_data *) vict_obj;
                 break;
	
            case 's':
                 i = HSHR(ch);
                 break;
            case 'S':
                 if (vict_obj)
                    i = HSHR((const struct char_data *) vict_obj);
                 else
                    CHECK_NULL(obj,OSHR(obj));
                 dg_victim = (struct char_data *) vict_obj;	
                 break;

            case 'e':
                 i = HSSH(ch);
                 break;
            case 'E':
                 if (vict_obj)
                    i = HSSH((const struct char_data *) vict_obj);
                 else
                    CHECK_NULL(obj,OSSH(obj));
                 dg_victim = (struct char_data *) vict_obj;	
                 break;
	
            case 'o':
                 if (*(orig+1) < '0' || *(orig+1) > '5')
                    {CHECK_NULL(obj, OBJN(obj, to, 0));
                    }
                 else
                    {padis = *(++orig) - '0';
                     CHECK_NULL(obj, OBJN(obj, to, padis > 5 ? 0 : padis));
                    }	
                 break;
            case 'O':
                 if (*(orig+1) < '0' || *(orig+1) > '5')
                    {CHECK_NULL(vict_obj, OBJN((const struct obj_data *) vict_obj,to,0));
                    }
                 else
                    {padis = *(++orig) - '0';
                     CHECK_NULL(vict_obj, OBJN((const struct obj_data *) vict_obj,to,padis > 5 ? 0 : padis)); 	
                    }
                 dg_victim = (struct char_data *) vict_obj; 	
                 break;
	
            case 'p':
                 CHECK_NULL(obj, OBJS(obj, to));
                 break;
            case 'P':
                 CHECK_NULL(vict_obj, OBJS((const struct obj_data *) vict_obj, to));
                 dg_victim = (struct char_data *) vict_obj;	
                 break;

            case 'T':
                 CHECK_NULL(vict_obj, (const char *) vict_obj);
                 break;
	
            case 'F':
                 CHECK_NULL(vict_obj, fname((const char *) vict_obj));
                 break;
	
            case '$':
                 i = "$";
                 break;
	
            case 'a':
                 i = GET_CH_VIS_SUF_6(ch,to);
                 break;
            case 'A':
                 if (vict_obj)
                    i = GET_CH_VIS_SUF_6((const struct char_data *) vict_obj, to);
                 else
                    CHECK_NULL(obj,GET_OBJ_VIS_SUF_6(obj,to));
                 dg_victim = (struct char_data *) vict_obj;
                 break;

            case 'g':
                 i = GET_CH_VIS_SUF_1(ch,to);
                 break;
            case 'G':
                 if (vict_obj)
                    i = GET_CH_VIS_SUF_1((const struct char_data *) vict_obj,to);
                 else
                    CHECK_NULL(obj,GET_OBJ_VIS_SUF_1(obj,to));
                 dg_victim = (struct char_data *) vict_obj;
                 break;

            case 'y':
                 i = GET_CH_VIS_SUF_5(ch,to);
                 break;
            case 'Y':
                 if (vict_obj)
                    i = GET_CH_VIS_SUF_5((const struct char_data *) vict_obj,to);
                 else
                    CHECK_NULL(obj,GET_OBJ_VIS_SUF_5(obj,to));
                 dg_victim = (struct char_data *) vict_obj;
                 break;

            case 'u':
                 i = GET_CH_VIS_SUF_2(ch,to);
                 break;
            case 'U':
                 if (vict_obj)
                    i = GET_CH_VIS_SUF_2((const struct char_data *) vict_obj,to);
                 else
                    CHECK_NULL(obj,GET_OBJ_VIS_SUF_2(obj,to));
                 dg_victim = (struct char_data *) vict_obj;
                 break;

            case 'w':
                 i = GET_CH_VIS_SUF_3(ch,to);
                 break;
            case 'W':
                 if (vict_obj)
                    i = GET_CH_VIS_SUF_3((const struct char_data *) vict_obj,to);
                 else
                    CHECK_NULL(obj,GET_OBJ_VIS_SUF_3(obj,to));
                 dg_victim = (struct char_data *) vict_obj;
                 break;

            case 'q':
                 i = GET_CH_VIS_SUF_4(ch,to);
                 break;
            case 'Q':
                 if (vict_obj)
                    i = GET_CH_VIS_SUF_4((const struct char_data *) vict_obj,to);
                 else
                    CHECK_NULL(obj,GET_OBJ_VIS_SUF_4(obj,to));
                 dg_victim = (struct char_data *) vict_obj;
                 break;

            default:
                 log("SYSERR: Illegal $-code to act(): %c", *orig);
                 log("SYSERR: %s", orig);
                 i = "";
                 break;
           }
          while ((*buf = *(i++)))
                buf++;
          orig++;
          }
       else
       if ( *orig == '\\')
          {if ( *(orig+1) == 'r')
              {*(buf++) = '\r';
               orig += 2;
              }
           else
           if ( *(orig+1) == 'n')
              {*(buf++) = '\n';
               orig += 2;
              }
           else
              *(buf++) = *(orig++);
          }
       else
       if (!(*(buf++) = *(orig++)))
          break;
      }

  *(--buf) = '\r';
  *(++buf) = '\n';
  *(++buf) = '\0';

  if (to->desc)
     SEND_TO_Q(CAP(lbuf), to->desc);

  if ((IS_NPC(to) && dg_act_check) && (to != ch))
     act_mtrigger(to, lbuf, ch, dg_victim, obj, dg_target, dg_arg);
}

/* moved this to utils.h --- mah */
#ifndef SENDOK
#define SENDOK(ch)	((ch)->desc && (to_sleeping || AWAKE(ch)) && \
			(IS_NPC(ch) || !PLR_FLAGGED((ch), PLR_WRITING)))
#endif

void act(const char *str, int hide_invisible, struct char_data *ch,
	 struct obj_data *obj, const void *vict_obj, int type)
{
  const struct char_data *to;
  int   to_sleeping, check_deaf, stopcount;

  if (!str || !*str)
    return;

  if (!(dg_act_check = !(type & DG_NO_TRIG)))
     type &= ~DG_NO_TRIG;

  /*
   * Warning: the following TO_SLEEP code is a hack.
   *
   * I wanted to be able to tell act to deliver a message regardless of sleep
   * without adding an additional argument.  TO_SLEEP is 128 (a single bit
   * high up).  It's ONLY legal to combine TO_SLEEP with one other TO_x
   * command.  It's not legal to combine TO_x's with each other otherwise.
   * TO_SLEEP only works because its value "happens to be" a single bit;
   * do not change it to something else.  In short, it is a hack.
   */

  /* check if TO_SLEEP is there, and remove it if it is. */
  if ((to_sleeping = (type & TO_SLEEP)))
     type &= ~TO_SLEEP;
  if ((check_deaf = (type & CHECK_DEAF)))
     type &= ~CHECK_DEAF;

  if (type == TO_CHAR)
     {if (ch && SENDOK(ch) && IN_ROOM(ch) != NOWHERE)
         perform_act(str, ch, obj, vict_obj, ch);
      return;
     }

  if (type == TO_VICT)
     {if ((to = (const struct char_data *) vict_obj) != NULL && SENDOK(to) && IN_ROOM(to) != NOWHERE)
         perform_act(str, ch, obj, vict_obj, to);
      return;
     }
  /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

  if (ch && ch->in_room != NOWHERE)
     to = world[ch->in_room].people;
  else
  if (obj && obj->in_room != NOWHERE)
     to = world[obj->in_room].people;
  else
     {log("SYSERR: no valid target to act()!");
      return;
     }

  for (stopcount = 0; to && stopcount < 1000; to = to->next_in_room, stopcount++)
      {if (!SENDOK(to) || (to == ch))
          continue;
       if (hide_invisible && ch && !CAN_SEE(to, ch))
          continue;
       if (type != TO_ROOM && to == vict_obj)
          continue;
       if (!IS_NPC(to) && check_deaf && PRF_FLAGGED(to, PRF_DEAF))
          continue;
       perform_act(str, ch, obj, vict_obj, to);
      }
}

/*
 * This function is called every 30 seconds from heartbeat().  It checks
 * the four global buffers in CircleMUD to ensure that no one has written
 * past their bounds.  If our check digit is not there (and the position
 * doesn't have a NUL which may result from snprintf) then we gripe that
 * someone has overwritten our buffer.  This could cause a false positive
 * if someone uses the buffer as a non-terminated character array but that
 * is not likely. -gg
 */
void sanity_check(void)
{
  int ok = TRUE;

  /*
   * If any line is false, 'ok' will become false also.
   */
  ok &= (test_magic(buf)  == MAGIC_NUMBER || test_magic(buf)  == '\0');
  ok &= (test_magic(buf1) == MAGIC_NUMBER || test_magic(buf1) == '\0');
  ok &= (test_magic(buf2) == MAGIC_NUMBER || test_magic(buf2) == '\0');
  ok &= (test_magic(arg)  == MAGIC_NUMBER || test_magic(arg)  == '\0');

  /*
   * This isn't exactly the safest thing to do (referencing known bad memory)
   * but we're doomed to crash eventually, might as well try to get something
   * useful before we go down. -gg
   * However, lets fix the problem so we don't spam the logs. -gg 11/24/98
   */
  if (!ok) {
    log("SYSERR: *** Buffer overflow! ***\n"
	"buf: %s\nbuf1: %s\nbuf2: %s\narg: %s", buf, buf1, buf2, arg);

    plant_magic(buf);
    plant_magic(buf1);
    plant_magic(buf2);
    plant_magic(arg);
  }

#if 0
  log("Statistics: buf=%d buf1=%d buf2=%d arg=%d",
	strlen(buf), strlen(buf1), strlen(buf2), strlen(arg));
#endif
}

/* Prefer the file over the descriptor. */
void setup_log(const char *filename, int fd)
{
  FILE *s_fp;

#if 0 // prool
#if defined(__MWERKS__) || defined(__GNUC__) // tyt byl prool
  s_fp = stderr;
#else
  if ((s_fp = fdopen(STDERR_FILENO, "w")) == NULL) {
    puts("SYSERR: Error opening stderr, trying stdout.");

    if ((s_fp = fdopen(STDOUT_FILENO, "w")) == NULL) {
      puts("SYSERR: Error opening stdout, trying a file.");

      /* If we don't have a file, try a default. */
      if (filename == NULL || *filename == '\0')
        filename = "log/syslog";
    }
  }
#endif

  if (filename == NULL || *filename == '\0') {
    /* No filename, set us up with the descriptor we just opened. */
    logfile = s_fp;
    //puts("Using file descriptor for logging."); // prool
    return;
  }
#else
s_fp=NULL; // prool
#endif // prool
//printf("filename=`%s'\n",filename); // prool

  /* We honor the default filename first. */
  if (open_logfile(filename, s_fp))
    return;

  /* Well, that failed but we want it logged to a file so try a default. */
  if (open_logfile("log/syslog", s_fp))
    return;

  /* Ok, one last shot at a file. */
  if (open_logfile("syslog", s_fp))
    return;

  /* Erp, that didn't work either, just die. */
  puts("SYSERR: Couldn't open anything to log to, giving up.");
  exit(1);
}

int open_logfile(const char *filename, FILE *stderr_fp)
{
  if (stderr_fp)	/* freopen() the descriptor. */ // tyt byl prool
    {logfile = freopen(filename, "w", stderr_fp); /*printf("prool label 2\n");*/}
  else
    {logfile = fopen(filename, "w");  /*printf("prool label 2A\n");*/}

  if (logfile) {
    printf("Using log file '%s'%s.\n",
		filename, stderr_fp ? " with redirection" : "");
    return (TRUE);
  }

  printf("SYSERR: Error opening file '%s': %s\n", filename, strerror(errno));
  return (FALSE);
}

/*
 * This may not be pretty but it keeps game_loop() neater than if it was inline.
 */
#if defined(CIRCLE_WINDOWS)

void circle_sleep(struct timeval *timeout)
{
  Sleep(timeout->tv_sec * 1000 + timeout->tv_usec / 1000);
}

#else

void circle_sleep(struct timeval *timeout)
{
  if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, timeout) < 0) {
    if (errno != EINTR) {
      perror("SYSERR: Select sleep");
      exit(1);
    }
  }
}

#endif /* CIRCLE_WINDOWS */
