// prool's recoding functions
// proolix@gmail.com www.prool.kharkov.org mud.kharkov.org

#define BUFLEN 512 // prool
#define UTF 0
#define KOI 1

extern int codetable;
extern int logging;

void utf8_to_koi(char *str_i, char *str_o);
void koi_to_utf8(char *str_i, char *str_o);
void fromkoi (char *str);
void utf8_to_win(char *str_i, char *str_o);
void win_to_utf8(char *str_i, char *str_o);
void outhex(char *str);
