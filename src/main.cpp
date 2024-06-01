#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "spiAVR.h"
#include "queue.h"

#define NUM_TASKS 1     //Macro for total number of tasks

//TOOK OUT PASSIVE BUZZER CODE FOR NOW TO TEST TETRIS BOARD

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

//Global Variables
short numTiles = 0;         //How many tiles a block has traversed (can't be more than 24)
short horPos = 8;           //Variable for horizontal position multiplier (-1 is right and + 1 is left)
short verPos = 0;           //Variable for vertical position multiplier
const short columns = 8;    //num of columns in the tetris grid

//  Array for Tetris Grid
unsigned long tetrisGrid[columns] = {
  0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000000
};

//  Array for Output Grid
unsigned long outGrid[columns] = {
  0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000018, 
  0x00000018, 
  0x00000000, 
  0x00000000, 
  0x00000000
};

//
//  Arrays for different pieces
//
//Variables for different pieces
short numPieces = 5;    //Total number of pieces
const short numPos = 4; //Total Number of rotation positions

//Square Piece
//Positions are all the same cause its a square
unsigned long sqrPiece[numPos][columns] = {
  //Base Position
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000018, 
  0x00000018, 
  0x00000000, 
  0x00000000, 
  0x00000000}, 
  //Position 2
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000018, 
  0x00000018, 
  0x00000000, 
  0x00000000, 
  0x00000000}, 
  //Position 3
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000018, 
  0x00000018, 
  0x00000000, 
  0x00000000, 
  0x00000000}, 
  //Position 4
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000018, 
  0x00000018, 
  0x00000000, 
  0x00000000, 
  0x00000000}
};


//
//  Next Piecves queue and utility functions
//


//REWRITE QUEUE TO HAVE THE PIECES AND THEIR POSITIONS SO JUST HANDLE THESE


queue nextPCS;

//Initialize queue
void queue_init() {
  //Push first 4 shapes for now
  nextPCS.push(0);
  nextPCS.push(1);
  nextPCS.push(2);
  nextPCS.push(3);
}

void next() {
  nextPCS.pop();
  //push a new piece to the queue somwhat randomly
  nextPCS.push(((nextPCS.first() + 3) * 3 + 3) % 5);
}



//
//  Functions for piece placement validations
//

//Normal droping validation
bool checkDrop() {



}

//Horizontal movement validation
bool checkSide() {

}

//rotating validation (rot + rotMul) % numRot then keep changing rotMul
bool checkRotate() {

}



//
//  Output Grid and Tetris Grid Update functions
//

//Call after having all checks done
void updateOutput() {
  //loop through all of columns and place pieces into it
  for(int i = 0; i < 8; i++) {
    outGrid[i] = tetrisGrid[i] | (sqrPiece[0][(i + horPos) % 8] << (numTiles + verPos));  //Change square piece position with changes in vertical and horizontal position
  }
}

//Update actual tetris board to keep track of settled pieces
void updateTetris() {
  //loop through all of columns and place pieces into it
  for(int i = 0; i < 8; i++) {
    tetrisGrid[i] = outGrid[i];
  }
}



//
//  Task Functions
//

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		      //Task's current state
	unsigned long period; 		    //Task period
	unsigned long elapsedTime; 	  //Time elapsed since last task tick
	int (*TickFct)(int); 		      //Task tick function
} task;

//Define Periods for each task
const unsigned long GCD_PERIOD = 50;       //GCD Period for tasks
const unsigned long TASK1_PERIOD = 250;

task tasks[NUM_TASKS]; // declared task array with 5 tasks

//  LED Matrix States
enum task1_states {task1_start, drop, hold, off} task1_state;

void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {           // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {         // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state);     // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                              // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                      // Increment the elapsed time by GCD_PERIOD
	}
}


//
//  LED Matrix Tick Function
//

/*
// Use as template for handling the led matrix using 32 bits for each led
unsigned long x = 0xAA08FF01; // b4 b3 b2 b1

unsigned int b1 = (x & 0xff);
unsigned int b2 = (x >> 8) & 0xff;
unsigned int b3 = (x >> 16) & 0xff;
unsigned int b4 = (x >> 24);
*/

unsigned long x = 0x00000000; // b4 b3 b2 b1

unsigned int b1 = (x & 0xff);
unsigned int b2 = (x >> 8) & 0xff;
unsigned int b3 = (x >> 16) & 0xff;
unsigned int b4 = (x >> 24);

int j = 0;


//Validation functions for checking piece



//LED Matrix Tick function
int task1_tick(int state) {
    //state transitions
    switch(state) {
        case task1_start:
          state = drop;
          //initialize output grid
          updateOutput();
          break;
        case drop:
          
          //Have checks too know if stay on this state or next

          //REMMEVBER TO DO THIS AND MAYVBE DRAW IT OUT FIRST

          
          

          //Increment after checking drops and everything
          numTiles++;
          state = drop;
          
          //update output grid
          updateOutput();

          //Case for when its at the very last tile
          if(numTiles >= 28) {

            //TEMPORARY SOLUITION

            numTiles--;
            //update output grid
            updateOutput();

            numTiles = 0;
            state = hold;
          }
          break;
        case hold:
          numTiles++;
          state = hold;

          //wait for now i guess

          if(numTiles >= 20) {
            //Change tetris board with new pieces
            updateTetris();

            numTiles = 0;
            state = off;
          }
          break;
        case off:
          state = drop;
          break;
    }

    //state functions
    switch(state) {
        case task1_start:
          //ignore do nothing
          break;
        case drop:
          //Loop through all columns to output
          for(int i = 0; i < 8; i++) {
            //Convert the 32bit binary to 4 8-bit binary
            x = outGrid[i];   //i + 8 to control where we want to move piecves horizontally
            b1 = (x & 0xff);
            b2 = (x >> 8) & 0xff;
            b3 = (x >> 16) & 0xff;
            b4 = (x >> 24);

            //send stuff to SPI
            PORTB = SetBit(PORTB, 2, 0); //set low
            //matrix 4
            SPI_SEND(0x01 + i);
            SPI_SEND(b4);
            //matrix 3
            SPI_SEND(0x01 + i);
            SPI_SEND(b3);
            //matrix 2
            SPI_SEND(0x01 + i);
            SPI_SEND(b2);
            //matrix 1
            SPI_SEND(0x01 + i);
            SPI_SEND(b1);
            PORTB = SetBit(PORTB, 2, 1); //set high
          }
          
          break;
        case hold:
          //do nothing
          break;
        case off:
          for(int i = 0; i < 8; i++) {
            PORTB = SetBit(PORTB, 2, 0); //set low
            SPI_SEND(0x01 + i);
            SPI_SEND(0x00);
            SPI_SEND(0x01 + i);
            SPI_SEND(0x00);
            SPI_SEND(0x01 + i);
            SPI_SEND(0x00);
            SPI_SEND(0x01 + i);
            SPI_SEND(0x00);
            PORTB = SetBit(PORTB, 2, 1); //set high
          }

          break;
    }

    //return satte
    return state;
}



//
//  Main Function
//

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