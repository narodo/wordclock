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

#define CLK_WAIT   100 //clock wait in us
#define DEBOUNCE_WAIT 10 //wait in ms
#define ADJ_TIMEOUT 3000 //adjustment time-out in ms we wait before we go back into "clock" mode

RTC_DS1307 RTC;

void write_leds(){
 
  unsigned char i, j;
  unsigned char worker = 0;
  unsigned char debug_i;
  
  Serial.print("---0x");
  for(debug_i = 3; debug_i > 0 ; debug_i--){
      Serial.print(leds[debug_i-1], HEX);
  }
  
  digitalWrite(CLK, LOW);
  //digitalWrite(OE, LOW);
  digitalWrite(STROBE, LOW);
  digitalWrite(DATA, LOW);
  
  for(i=0; i<3; i++){
    for(j=0; j<8; j++){
      //null other bits except the one we send out
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


void clear_leds(){
 unsigned char i;
 
 for(i=0;i<3;i++)
   leds[i]=0;

}

void clear_led_bit(unsigned char bit_num){
  int array_num, bit_number;
  
  array_num = bit_num/8;
  bit_number = 1 << (bit_num %8); 
  leds[array_num] &= !bit_number;

}

void set_led_bit(unsigned char bit_num){
  int array_num, bit_number;
  
  array_num = bit_num/8;
  bit_number = 1 << (bit_num %8); 
  leds[array_num] |= bit_number;
  
}

void set_adjust_led_hr(unsigned char hours){

    set_led_bit(hour_bit_mapping[hours-1]);
}


void set_adjust_led_min(unsigned char minutes){

  unsigned char min_tmp;
  
  clear_leds();

  if(minutes == 0){
    set_led_bit(UHR);
    return;
  }
    

  if(minutes < 10){
  //use hour leds for minutes 1 to 10
    set_led_bit(hour_bit_mapping[minutes-1]);
    return;
  }
  
  if(minutes == 10){
    set_led_bit(ZEHN);
    return;
  }
    
  if(minutes < 20){
  //use hour leds and "zehn"
    set_led_bit(ZEHN);
    set_led_bit(hour_bit_mapping[(minutes-10)-1]);
    return;
  }

   if(minutes < 30){
    //use hour leds and "halb"
    set_led_bit(ZWANZIG);
    set_led_bit(hour_bit_mapping[(minutes-20)-1]);
    return;
  }

  if(minutes < 40){
    //use hour leds and "halb"
    set_led_bit(HALB);
    set_led_bit(hour_bit_mapping[(minutes-30)-1]);
    return;
  }
  
  if(minutes < 60){
    //now we change the direction around and use "vor"
    set_led_bit(VOR);
    min_tmp=60-minutes;
    
   if(min_tmp == 20){
      set_led_bit(ZWANZIG);
      return;
    }
    if(min_tmp >= 10){
      set_led_bit(ZEHN);
      if(min_tmp != 10)
        set_led_bit(hour_bit_mapping[min_tmp - 10 -1]);
      return;
    }
    
    if(min_tmp < 10){
      set_led_bit(hour_bit_mapping[min_tmp-1]);
      return;
    }
   return;
  }
}

struct time adjustTime(unsigned char min_adj, unsigned char hr_adj){
  unsigned char minutes, hours;
  unsigned char button_pressed=10;
  unsigned char minute_active=0, hour_active=0;
  
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
       
       clear_leds();
       set_adjust_led_min(minutes);
       write_leds();
     }
     
     if(!hour_active && (digitalRead(HR_BTN)==0)){
       hour_active = 1;
       button_pressed = 10;
       if(hours < 12)
         hours++;
       else
         hours=1;
       clear_leds();
       set_adjust_led_hr(hours);
       write_leds();
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


void set_time_led(unsigned char hours, unsigned char minutes){
   
  unsigned char five_minutes;
  unsigned char debug_i;

  five_minutes = minutes / 5;

  Serial.print("-:0x");
    for(debug_i = 3; debug_i > 0 ; debug_i--){
      Serial.print(leds[debug_i-1], HEX);
    }

    clear_leds();
    set_led_bit(ES);
    set_led_bit(IST);
  
    
    switch(five_minutes){
      case 0: //exact hour
        set_led_bit(hour_bit_mapping[hours-1]);
        if(hours != 1)
          set_led_bit(UHR);
        break;

      case 1:  //five past  
        set_led_bit(FUNF);
        set_led_bit(NACH);
        set_led_bit(hour_bit_mapping[hours-1]);    
        break;
    
      case 2: //ten past  
        set_led_bit(ZEHN);
        set_led_bit(NACH);
        set_led_bit(hour_bit_mapping[hours-1]);    
        break;
        
      case 3: //quater past  
        set_led_bit(VIERTEL);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0]);    
        else
          set_led_bit(hour_bit_mapping[hours]);    
        break;
      
      case 4: // zwanzig past  
        set_led_bit(ZWANZIG);
        set_led_bit(NACH);
        set_led_bit(hour_bit_mapping[hours-1]);    
        break;
      
      case 5: // fünf und zwanzig past  
        set_led_bit(FUNF);
        set_led_bit(VOR);
        set_led_bit(HALB);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0]);    
        else
          set_led_bit(hour_bit_mapping[hours]);    
        break;
        
      case 6: // halb  
        set_led_bit(HALB);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0]);    
        else
          set_led_bit(hour_bit_mapping[hours]);    
        break;

      case 7: // fünf nach halb  
        set_led_bit(FUNF);
        set_led_bit(NACH);
        set_led_bit(HALB);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0]);    
        else
          set_led_bit(hour_bit_mapping[hours]);    
        break;
      
       case 8: // zwanzig vor  
        set_led_bit(ZWANZIG);
        set_led_bit(VOR);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0]);    
        else
          set_led_bit(hour_bit_mapping[hours]);    
        break;

      case 9: // viertel vor  
        set_led_bit(VIERTEL);
        set_led_bit(VOR);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0]);    
        else
          set_led_bit(hour_bit_mapping[hours]);    
        break;

      case 10: // zehn vor  
        set_led_bit(ZEHN);
        set_led_bit(VOR);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0]);    
        else
          set_led_bit(hour_bit_mapping[hours]);    
        break;

      case 11: // fünf vor  
        set_led_bit(FUNF);
        set_led_bit(VOR);
        if(hours == 12)
          set_led_bit(hour_bit_mapping[0]);    
        else
          set_led_bit(hour_bit_mapping[hours]);    
        break;
    }
 
    Serial.print("--:0x");
    for(debug_i = 3; debug_i > 0 ; debug_i--){
      Serial.print(leds[debug_i-1], HEX);
    }
 
    
}


void setup () {
    Serial.begin(76800);
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
  unsigned char i=0;
  unsigned char minutes=0, hours=12;
  unsigned char mod_minutes=0;
  static unsigned char done = 0;
  unsigned char *min_adj, *hr_adj;
  DateTime now = RTC.now();
  struct time time_adj;
   
    //delay(500); 
    
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.println(now.second(), DEC);
       
    
    clear_leds();
    //make sure we put out time first time around
    minutes = now.minute();
    hours = now.hour();
    set_time_led(hours, (minutes / 5)*5);
    write_leds();   
  
  
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
     
     if(minutes==5)
        minutes=6;
     if(minutes==0)
        minutes=5;
    
    delay(500);
   */
      now=RTC.now();
      minutes = now.minute();
      hours = now.hour();
          
      if(hours > 12)
         hours = hours - 12;    

      mod_minutes = minutes % 5;
    
    //only do something for five minute increments
    if((mod_minutes == 0) && (done == 0)){
  
      Serial.println(" ");
      Serial.print("Time: ");
      Serial.print(hours, DEC);
      Serial.print(':');
      Serial.print(minutes, DEC);
      Serial.print("--");
      done = 1;

      set_time_led(hours, minutes);
      write_leds();
      
    } else if (mod_minutes > 0)
        done = 0;

    //check if minute of hour button was pressed
      if((digitalRead(HR_BTN) == 0) || (digitalRead(MIN_BTN)==0)){
        time_adj = adjustTime(minutes, hours);
        RTC.adjust(DateTime(0,0,0,time_adj.hours, time_adj.minutes, 0));
       //make sure we put out time again
        set_time_led(time_adj.hours, (time_adj.minutes / 5)*5);
        write_leds();      
    }

    }
}
