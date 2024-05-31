#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "spiAVR.h"

#define NUM_TASKS 1 //TODO: Change to the number of tasks being used

/*
        Initialize LED Matrix
*/
void Matrix_Init() {
  //Decode Mode Off
  PORTB = SetBit(PORTB, 2, 0); //set low
  SPI_SEND(0x09);
  SPI_SEND(0x00);
  PORTB = SetBit(PORTB, 2, 1); //set high

  //Intensity Low for now
  PORTB = SetBit(PORTB, 2, 0); //set low
  SPI_SEND(0x0A);
  SPI_SEND(0x01);
  PORTB = SetBit(PORTB, 2, 1); //set high

  //Scan Limit to all
  PORTB = SetBit(PORTB, 2, 0); //set low
  SPI_SEND(0x0B);
  SPI_SEND(0x07);
  PORTB = SetBit(PORTB, 2, 1); //set high

  //Shutdown to on
  PORTB = SetBit(PORTB, 2, 0); //set low
  SPI_SEND(0x0C);
  SPI_SEND(0x01);
  PORTB = SetBit(PORTB, 2, 1); //set high

  //Display test to off for now
  PORTB = SetBit(PORTB, 2, 0); //set low
  SPI_SEND(0x0F);
  SPI_SEND(0x00);
  PORTB = SetBit(PORTB, 2, 1); //set high

  //initially turn off all leds to make sure
  for(int i = 0; i < 8; i++) {
    PORTB = SetBit(PORTB, 2, 0); //set low
    SPI_SEND(0x01 + i);
    SPI_SEND(0x00);
    PORTB = SetBit(PORTB, 2, 1); //set high
  }

  //extra no ops to pass initialization for turn off
  for(int i = 0; i < 4; i++) {
    //NO OP
    PORTB = SetBit(PORTB, 2, 0); //set low
    SPI_SEND(0x00);
    SPI_SEND(0x00);
    PORTB = SetBit(PORTB, 2, 1); //set high
  }
}

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;


//TODO: Define Periods for each task
// e.g. const unsined long TASK1_PERIOD = <PERIOD>
const unsigned long GCD_PERIOD = 10;//TODO:Set the GCD Period
const unsigned long TASK1_PERIOD = 10;

task tasks[NUM_TASKS]; // declared task array with 5 tasks

//  Passive Buzzer (Background Music) Output
enum task1_states {task1_start, playSong, end} task1_state;

void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}

//LED Matrix Tick function
int task1_tick(int state) {
    //state transitions
    switch(state) {
        case task1_start:
          state = playSong;
          break;
        case playSong:
          state = end;
          break;
        case end:
          state = end;
          break;
    }

    //state functions
    switch(state) {
        case task1_start:
          //ignore do nothing
          break;
        case playSong:
          //send first instruction (location)
          SPI_SEND(0x01);
          //send second instruction (leds)
          SPI_SEND(0xF0);
          
          //set low
          PORTB = SetBit(PORTB, 2, 0);
          //turn high
          PORTB = SetBit(PORTB, 2, 1);
          break;
        case end:
          //do nothing
          break;
    }

    //return satte
    return state;
}

int main(void) {
    //TODO: initialize all your inputs and ouputs
    DDRB = 0xFF; PORTB = 0x00; //all outputs for B

    ADC_init();     // initializes ADC
    SPI_INIT();     // Initialize SPI protocol
    Matrix_Init();  // Initialize 8x8 Led matrix

    //Task Initialization
    //LED Matrix task initialization
    tasks[0].period = TASK1_PERIOD;
    tasks[0].state = task1_start;
    tasks[0].elapsedTime = TASK1_PERIOD;
    tasks[0].TickFct = &task1_tick;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {
        
        TimerISR();
        while (!TimerFlag){}  // Wait for SM period
        TimerFlag = 0;        // Lower flag

    }
    return 0;

}