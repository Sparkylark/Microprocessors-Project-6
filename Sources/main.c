//***594 Data Bytes*** (not including top line)
#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */

static char anodes[10] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
static char cathodes[4] = {0xFE,0xFD,0xFB,0xF7};
short currentDisplay = 0;     //next 7-segment display to refresh
short currentTime[6] = {5,9,5,9,9,9}; //Holds timer data (Minute, second, hundredths of a second)
short timerMax = 0;           //timer full boolean
short sysMode = 2;            //0 = Reset, 1 = Run, 2 = Stop
int clockCnt = 24000;         //Prescaled clock cycles to pass 1 millisecond
short timerState = 0;         //current timer mode (2= <1 minute, 1= <10 minutes, and 0= <60 minutes)
short interruptCount = 0;     //count of Stopwatch interrupts
short buttonTimeCount = 0;    //count of time since the last bounce of SW5
short currentTone = 0;        //which tone (0,1,2) is currently playing - 3 means no tone
int toneCnt[3] = {24000,12000,8000};
int toneInterruptCount = 0;   //count of stopwatch interrupts for advancing the tone
short i;                        //looping variable

void main(void) {
  SetClk8();
  DDRH &= 0xFE;   //Set PH0 (SW5) to be an input
  PPSH = 0x00;    //Falling-Edge Polarity for PH
  PIEH = 0x01;    //Enable PH0 interrupt
  DDRB |= 0xFF;   //Set 7-Seg Anodes to output
  DDRP |= 0x0F;   //Set 7-Seg Cathodes to output  
  TSCR1 = 0x90;   //Set TEN and TFFCA
  TSCR2 = 0x00;   //Prescale Factor = 1
  TIOS = 0x60;    //Use Timers 5 and 6 for output compare
  TCTL1 = 0x00;   //Set PT5 to not toggle upon successful compare
  TC6 += clockCnt;//Set OC6 interrupt to go off after 1 millisecond
  TIE = 0x60;     //Enable OC6 and OC5 interrupts
  asm("cli");
  while(1);
}

void interrupt 14 StopwatchTimerHandler(){
  TC6 += clockCnt;  //timer will interrupt again after 1 millisecond
  //The following section keeps the 7-segment displays refreshed
  PTP = cathodes[currentDisplay]; //Place current cathode on PTP
  PORTB = anodes[currentTime[currentDisplay + timerState]];  //Place 7-seg representation of current digit on PTB  
  if((timerState != 1 && (currentDisplay==1 || currentDisplay==2)) || (timerState == 1 && currentDisplay==0)){
    PORTB |= 0x80;  //Activate decimals as a point(if between 1 and 10 minutes) or as a colon (otherwise)
  }
  if(currentDisplay == 3) currentDisplay=0;
  else currentDisplay++;   
  
  
  //The following section handles the stopwatch time data
  if(interruptCount < 9) interruptCount++;
  else{   //occurs every 10 interrupts (10 milliseconds)
    interruptCount = 0;
    if(sysMode == 1 && timerMax == 0){  //if in run mode, increment timer by .01 seconds
      if(currentTime[5] < 9) currentTime[5]++;
      else{
        currentTime[5] = 0;
        if(currentTime[4] < 9) currentTime[4]++;
        else{
          currentTime[4] = 0;
          if(currentTime[3] < 9) currentTime[3]++;
          else{  //the following line sets timerMax if timer is at 59 minutes and 59 seconds
            if(currentTime[2]==5 && currentTime[1]==9 && currentTime[0]==5) timerMax = 1;
            else{  
              currentTime[3] = 0;
              if(currentTime[2] < 5) currentTime[2]++;
              else{
                currentTime[2] = 0;
                if(currentTime[1] < 9) currentTime[1]++;
                else{
                  currentTime[1] = 0;
                  if(currentTime[0] < 5) currentTime[0]++;
                  else timerMax = 1;
                }
              }
            }
          }
        }
      }
    }
  }
    
  //the following section acts as a debouncer for SW5
  if(buttonTimeCount>0) buttonTimeCount--;
  
  //The following section determines the timer mode by whether the minute is less than 1 or 10
  if(currentTime[0]==0){
    if(currentTime[1]==0) timerState = 2; //if less than 1 minute
    else timerState = 1;                  //if less than 10 minutes
  }else timerState = 0;                   //otherwise
  
  //The following section keeps track of time for the tone-playing interrupt
  if(currentTone!=3) {
    if(toneInterruptCount < 999) toneInterruptCount++;
    else{
      toneInterruptCount = 0;
      currentTone++;
    }
  }
  
}

void interrupt 13 ToneTimerHandler(){
  if(currentTone!=3) TC5 += toneCnt[currentTone];
  else {
    TC5 += 65535;
    TCTL1 = 0x00;  //Stop toggling PT5 on successful compare  
  }
}

void interrupt 25 PH0Handler(){
  PIFH = 0x01;
  if(buttonTimeCount==0){
    buttonTimeCount = 199;
    if(sysMode < 2){
      sysMode++;
      if(sysMode==2){
        currentTone = 0;
        TC5 += toneCnt[0];
        TCTL1 = 0x04;   //Set PT5 to toggle upon successful compare
      }
    }
    else{           //if going to initialize
      sysMode = 0;
	  timerMax = 0;
      for(i=0; i<=5; i++){
        currentTime[i] = 0;     //set timer to 00:00.00
      }
    }     
  }
}        