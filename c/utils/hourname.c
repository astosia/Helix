#include <pebble.h>
#include "hourname.h"
#include "string.h"
const char* HOURNAME_ES[] = {
  "DOCE",
  "UNO",
  "DOS",
  "TRES",
  "CUATRO",
  "CINCO",
  "SEIS",
  "SIETE",
  "OCHO",
  "NUEVE",
  "DIEZ",
  "ONCE",
};
const char* HOURNAME_EN[] = {
  "TWELVE",
  "ONE",
  "TWO",
  "THREE",
  "FOUR",
  "FIVE",
  "SIX",
  "SEVEN",
  "EIGHT",
  "NINE",
  "TEN",
  "ELEVEN",
};
const char* HOURNAME_DE[] = {
  "ZWOLF",
  "EINS",
  "ZWEI",
  "DREI",
  "VIER",
  "FUNF",
  "SECHS",
  "SIEBEN",
  "ACHT",
  "NEUN",
  "ZEHN",
  "ELF",
};
const char* HOURNAME_FR[] = {
  "DOUZE",
  "UN",
  "DEUX",
  "TROIS",
  "QUATRE",
  "CINQ",
  "SIX",
  "SEPT",
  "HUIT",
  "NEUF",
  "DIX",
  "ONZE",
};
const char* HOURNAME_PT[] = {
  "DOZE",
  "UM",
  "DOIS",
  "TRES",
  "QUATRO",
  "CINCO",
  "SEIS",
  "SETE",
  "OITO",
  "NOVE",
  "DEZ",
  "ONZE",
};
const char* HOURNAME_IT[] = {
  "DODICI",
  "UNO",
  "DUE",
  "TRE",
  "QUATTRO",
  "CINQUE",
  "SEI",
  "SETTE",
  "OTTO",
  "NOVE",
  "DIECI",
  "UNDICI",
};
const char* HOURNAME_NU[] = {
  "12",
  "1",
  "2",
  "3",
  "4",
  "5",
  "6",
  "7",
  "8",
  "9",
  "10",
  "11",
};//end_hourname
void fetchhrname(int HN, const char* lang, char *iterhourname ){
       if (strcmp(lang,"es_ES")==0) {strcpy(iterhourname, HOURNAME_ES[HN]);}
  else if (strcmp(lang,"fr_FR")==0) {strcpy(iterhourname, HOURNAME_FR[HN]);}
  else if (strcmp(lang,"de_DE")==0) {strcpy(iterhourname, HOURNAME_DE[HN]);}
  else if (strcmp(lang,"it_IT")==0) {strcpy(iterhourname, HOURNAME_IT[HN]);}
  else if (strcmp(lang,"pt_PT")==0) {strcpy(iterhourname, HOURNAME_PT[HN]);}
  else if (strcmp(lang,"en_US")==0) {strcpy(iterhourname, HOURNAME_EN[HN]);}
  else                              {strcpy(iterhourname, HOURNAME_NU[HN]);};
}
