
#define HALB 8
#define UHR  7
#define ES   16
#define IST  17
#define ZEHN 18
#define VIERTEL 19
#define FUNF    20
#define ZWANZIG 21
#define NACH    22
#define VOR     23

struct time {
  char minutes;
  char hours;
};


char time_leds[] = {'0', '0', '0'};

char hour_bit_mapping [12] =
  // map numbers to bit positions
  //#1   2  3   4    5  6   7  8  9  10 11  12
   { 9, 10, 11, 13, 14, 15, 2, 3, 4, 5, 12, 6};   
