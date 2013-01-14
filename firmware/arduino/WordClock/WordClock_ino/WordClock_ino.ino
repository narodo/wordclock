#include <Wire.h>
#include <stdio.h>
#include "wordclock.h"
#include "RTClib.h"

#define CLK    10
#define STROBE     12
#define DATA       11
#define OE         13
#define MIN_BTN     4
#define HR_BTN     5

#define CLK_WAIT   50 //clock wait in us
#define DEBOUNCE_WAIT 10 //wait in ms
#define ADJ_TIMEOUT 3000 //adjustment time-out in ms we wait before we go back into "clock" mode

RTC_DS1307 RTC;

void write_leds(char leds[3]){

  char i, j;
  char worker = 0;
  
  digitalWrite(CLK, LOW);
  //digitalWrite(OE, LOW);
  digitalWrite(STROBE, LOW);
  digitalWrite(DATA, LOW);
  
  for(i=0; i<3; i++){
    for(j=0; j<8; j++){
      //null other bits than the one we send out
      worker = leds[i] & (1 << j);
      if(worker)
        digitalWrite(DATA, HIGH);
      else
        digitalWrite(DATA, LOW);

      delayMicroseconds(CLK_WAIT);
      digitalWrite(CLK, HIGH);
      delayMicroseconds(CLK_WAIT);
      digitalWrite(CLK, LOW);
    
    }
  }

   digitalWrite(STROBE, HIGH);
   delayMicroseconds(100);
   digitalWrite(OE, HIGH);

}


void clear_leds(char leds[3]){
 char i;
 
 for(i=0;i<3;i++)
   leds[i]=0;

}

void clear_led_bit(char bit_num, char leds[3]){
  int array_num, bit_number;
  
  array_num = bit_num/8;
  bit_number = 1 << (bit_num %8); 
  leds[array_num] &= !bit_number;

}

void set_led_bit(char bit_num, char leds[3]){
  int array_num, bit_number;
  
  array_num = bit_num/8;
  bit_number = 1 << (bit_num %8); 
  leds[array_num] |= bit_number;

}

void set_adjust_led_hr(char hours, char leds[3]){

    set_led_bit(hour_bit_mapping[hours-1], leds);
}


void set_adjust_led_min(char minutes, char leds[3]){

  char min_tmp;
  
  clear_leds(leds);

  if(minutes == 0){
    set_led_bit(UHR, leds);
    return;
  }
    

  if(minutes < 10){
  //use hour leds for minutes 1 to 10
    set_led_bit(hour_bit_mapping[minutes-1], leds);
    return;
  }
  
  if(minutes == 10){
    set_led_bit(ZEHN, leds);
    return;
  }
    
  if(minutes < 20){
  //use hour leds and "zehn"
    set_led_bit(ZEHN, leds);
    set_led_bit(hour_bit_mapping[(minutes-10)-1], leds);
    return;
  }

   if(minutes < 30){
    //use hour leds and "halb"
    set_led_bit(ZWANZIG, leds);
    set_led_bit(hour_bit_mapping[(minutes-20)-1], leds);
    return;
  }

  if(minutes < 40){
    //use hour leds and "halb"
    set_led_bit(HALB, leds);
    set_led_bit(hour_bit_mapping[(minutes-30)-1], leds);
    return;
  }
  
  if(minutes < 60){
    //now we change the direction around and use "vor"
    set_led_bit(VOR, leds);
    min_tmp=60-minutes;
    
   if(min_tmp == 20){
      set_led_bit(ZWANZIG, leds);
      return;
    }
    if(min_tmp >= 10){
      set_led_bit(ZEHN, leds);
      if(min_tmp != 10)
        set_led_bit(hour_bit_mapping[min_tmp - 10 -1], leds);
      return;
    }
    
    if(min_tmp < 10){
      set_led_bit(hour_bit_mapping[min_tmp-1], leds);
      return;
    }
   return;
  }
}

struct time adjustTime(char min_adj, char hr_adj, char leds[3]){
  char minutes, hours;
  char button_pressed=10;
  char minute_active=0, hour_active=0;
  
  struct time adj_time;
  
  minutes = min_adj;
  hours = hr_adj;
  
  while(button_pressed > 0){
   
    //check which button was pressed
   if(digitalRead(HR_BTN)==0 || digitalRead(MIN_BTN)==0){
     delay(DEBOUNCE_WAIT); 
    
     if(!minute_active && (digitalRead(MIN_BTN)==0)){
       minute_active = 1;
       button_pressed = 10;
       if(minutes < 60)
         minutes++;
       else
         minutes=0;
       
       clear_leds(leds);
       set_adjust_led_min(minutes, leds);
       write_leds(leds);
     }
     
     if(!hour_active && (digitalRead(HR_BTN)==0)){
       hour_active = 1;
       button_pressed = 10;
       if(hours < 12)
         hours++;
       else
         hours=1;
       clear_leds(leds);
       set_adjust_led_hr(hours, leds);
       write_leds(leds);
     }
   

  }
  //reset edge trigger
  if(digitalRead(HR_BTN) == 1)
    hour_active = 0;
  if(digitalRead(MIN_BTN) == 1)
    minute_active = 0;
  
  delay(ADJ_TIMEOUT/10);
  button_pressed--;
  
  }
  
  adj_time.minutes = minutes;
  adj_time.hours = hours;
  
  return adj_time;
}


void set_time_led(char hours, char minutes, char leds[3]){
   
  char mod_minutes, five_minutes;
  
  mod_minutes = minutes % 5;
  five_minutes = minutes / 5;
  
  //only do something for five minute increments
  if(mod_minutes==0){
  
    clear_leds(leds);
    set_led_bit(ES, leds);
    set_led_bit(IST, leds);
  
    
    switch(five_minutes){
      case 0: //exact hour
        set_led_bit(hour_bit_mapping[hours-1], leds);
        if(hours != 1)
          set_led_bit(UHR, leds);
        break;

      case 1:  //five past  
        set_led_bit(FUNF, leds);
        set_led_bit(NACH, leds);
        set_led_bit(hour_bit_mapping[hours-1], leds);    
        break;
    
      case 2: //ten past  
        set_led_bit(ZEHN, leds);
        set_led_bit(NACH, leds);
        set_led_bit(hour_bit_mapping[hours-1], leds);    
        break;
        
      case 3: //quater past  
        set_led_bit(VIERTEL, leds);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0], leds);    
        else
          set_led_bit(hour_bit_mapping[hours], leds);    
        break;
      
      case 4: // zwanzig past  
        set_led_bit(ZWANZIG, leds);
        set_led_bit(NACH, leds);
        set_led_bit(hour_bit_mapping[hours-1], leds);    
        break;
      
      case 5: // fünf und zwanzig past  
        set_led_bit(FUNF, leds);
        set_led_bit(VOR, leds);
        set_led_bit(HALB, leds);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0], leds);    
        else
          set_led_bit(hour_bit_mapping[hours], leds);    
        break;
        
      case 6: // halb  
        set_led_bit(HALB, leds);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0], leds);    
        else
          set_led_bit(hour_bit_mapping[hours], leds);    
        break;

      case 7: // fünf nach halb  
        set_led_bit(FUNF, leds);
        set_led_bit(NACH, leds);
        set_led_bit(HALB, leds);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0], leds);    
        else
          set_led_bit(hour_bit_mapping[hours], leds);    
        break;
      
       case 8: // zwanzig vor  
        set_led_bit(ZWANZIG, leds);
        set_led_bit(VOR, leds);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0], leds);    
        else
          set_led_bit(hour_bit_mapping[hours], leds);    
        break;

      case 9: // viertel vor  
        set_led_bit(VIERTEL, leds);
        set_led_bit(VOR, leds);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0], leds);    
        else
          set_led_bit(hour_bit_mapping[hours], leds);    
        break;

      case 10: // zehn vor  
        set_led_bit(ZEHN, leds);
        set_led_bit(VOR, leds);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0], leds);    
        else
          set_led_bit(hour_bit_mapping[hours], leds);    
        break;

      case 11: // fünf vor  
        set_led_bit(FUNF, leds);
        set_led_bit(VOR, leds);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0], leds);    
        else
          set_led_bit(hour_bit_mapping[hours], leds);    
        break;
    }
  }    
}


void setup () {
    Serial.begin(57600);
    Wire.begin();
    RTC.begin();
    
    pinMode(CLK, OUTPUT);
    pinMode(DATA, OUTPUT);
    pinMode(STROBE, OUTPUT);
    pinMode(OE, OUTPUT);    
    pinMode(MIN_BTN, INPUT);
    pinMode(HR_BTN, INPUT);

  if (! RTC.isrunning()) {
    //Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    //RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
}

void loop () {
  char i=0;
  char minutes=0, hours=12;
  char *min_adj, *hr_adj;
  DateTime now = RTC.now();
  struct time time_adj;
    
    /*
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    */
    
    clear_leds(time_leds);
    //make sure we put out time first time around
    minutes = now.minute();
    hours = now.hour();
    set_time_led(hours, (minutes / 5)*5, time_leds);
    write_leds(time_leds);   
  
  
    i=1;
   
    while(1){
    
  /*
      if(minutes==59){
        hours++;
        minutes=0;
      }
      else
        minutes++;
            
      if(hours == 25)
         hours = 1; 
      
      delay(500);
   */
      now=RTC.now();
      minutes = now.minute();
      hours = now.hour();
      if(hours > 12)
         hours = hours - 12;    

      set_time_led(hours, minutes, time_leds);
      write_leds(time_leds);

    //check if minute of hour button was pressed
      if((digitalRead(HR_BTN) == 0) || (digitalRead(MIN_BTN)==0)){
        time_adj = adjustTime(minutes, hours, time_leds);
        RTC.adjust(DateTime(0,0,0,time_adj.hours, time_adj.minutes, 0));
       //make sure we put out time again
        set_time_led(time_adj.hours, (time_adj.minutes / 5)*5, time_leds);
        write_leds(time_leds);      
    }

    }
}
