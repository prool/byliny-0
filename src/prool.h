// prool's recoding functions
// proolix@gmail.com www.prool.kharkov.org mud.kharkov.org

#define BUFLEN 512
#define UTF 0
#define KOI 1

#define START_ROOM 9900

extern int codetable;
extern int logging;

void utf8_to_koi(char *str_i, char *str_o);
void koi_to_utf8(char *str_i, char *str_o);
void fromkoi (char *str);
void utf8_to_win(char *str_i, char *str_o);
void win_to_utf8(char *str_i, char *str_o);
void outhex(char *str);

char *crypt_prool(char *,char *);
