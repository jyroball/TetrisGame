#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "spiAVR.h"
#include "queue.h"
#include "serialATmega.h"

#define NUM_TASKS 2     //Macro for total number of tasks

//Variable for pseudo random number
int randomNum = 0;

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
  0x00000000, 
  0x00000000, 
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

//T Piece
//Positions are rotating clockwise
unsigned long tPiece[numPos][columns] = {
  //Base Position (Up T)
  {0x00000000, 
  0x00000000, 
  0x00000010, 
  0x00000018, 
  0x00000010, 
  0x00000000, 
  0x00000000, 
  0x00000000}, 
  //Position 2 (right T)
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000008, 
  0x0000001C, 
  0x00000000, 
  0x00000000, 
  0x00000000}, 
  //Position 3 (down T)
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x0000001C, 
  0x00000008, 
  0x00000000, 
  0x00000000, 
  0x00000000}, 
  //Position 4 (left T)
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000008, 
  0x00000018, 
  0x00000008, 
  0x00000000, 
  0x00000000}
};

//| Piece
//Positions are rotating clockwise
unsigned long lPiece[numPos][columns] = {
  //Base Position (UP)
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000000, 
  0x0000001E, 
  0x00000000, 
  0x00000000, 
  0x00000000}, 
  //Position 2 (RIGHT)
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000010, 
  0x00000010, 
  0x00000010, 
  0x00000010, 
  0x00000000}, 
  //Position 3 (DOWN)
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x0000001E, 
  0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000000}, 
  //Position 4 (LEFT)
  {0x00000000, 
  0x00000010, 
  0x00000010, 
  0x00000010, 
  0x00000010, 
  0x00000000, 
  0x00000000, 
  0x00000000}
};

//L Piece
//Positions are rotating clockwise
unsigned long LPiece[numPos][columns] = {
  //Base Position (UP)
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x0000001C, 
  0x00000010, 
  0x00000000, 
  0x00000000, 
  0x00000000}, 
  //Position 2 (RIGHT)
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000018, 
  0x00000008, 
  0x00000008, 
  0x00000000, 
  0x00000000}, 
  //Position 3 (DOWN)
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000002, 
  0x0000001C, 
  0x00000000, 
  0x00000000, 
  0x00000000}, 
  //Position 4 (LEFT)
  {0x00000000, 
  0x00000000, 
  0x00000010, 
  0x00000010, 
  0x00000018, 
  0x00000000, 
  0x00000000, 
  0x00000000}
};

//z Piece
//Positions are rotating clockwise
unsigned long zPiece[numPos][columns] = {
  //Base Position (UP)
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x0000000C, 
  0x00000018, 
  0x00000000, 
  0x00000000, 
  0x00000000}, 
  //Position 2 (RIGHT)
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x00000010, 
  0x00000018, 
  0x00000008, 
  0x00000000, 
  0x00000000}, 
  //Position 3 (DOWN)
  {0x00000000, 
  0x00000000, 
  0x00000000, 
  0x0000000C, 
  0x00000018, 
  0x00000000, 
  0x00000000, 
  0x00000000}, 
  //Position 4 (LEFT)
  {0x00000000, 
  0x00000000, 
  0x00000010, 
  0x00000018, 
  0x00000008, 
  0x00000000, 
  0x00000000, 
  0x00000000}
};

//
//  Next Piecves queue and utility functions
//
queue nextPCS;

//  Array for Next Piece
unsigned long NextPiece[columns] = {
    0x00000000, 
    0x00000000, 
    0x00000000, 
    0x00000000, 
    0x00000000, 
    0x00000000, 
    0x00000000, 
    0x00000000
};

//Have parameters for a piece's max range for horizontal values here to might as well
int maxHorLeft = 0;
int maxHorRight = 7;
int rot = 0;

//Initialize queue
void queue_init() {
  //Push first 4 shapes for now
  nextPCS.push(4);
  nextPCS.push(1);
  nextPCS.push(2);
  nextPCS.push(3);
}

void next() {
  nextPCS.pop();
  //set RandomNum to new number
  randomNum = randomNum * 3;
  //push a new piece to the queue somwhat randomly
  nextPCS.push(randomNum % 5);
}

void setPiece() {
    //Know what piece the Next piece is gonna be
    switch(nextPCS.first()) {
        // Square Piece
        case 0:
            //loop through all of columns and place pieces into it
            for(int i = 0; i < 8; i++) {
                NextPiece[i] = sqrPiece[rot][i];
            }
            //set max horizontals for square piece (Make sure its <= or >= when checking)
            maxHorLeft = 11;
            maxHorRight = 5;
            break;
        // T Piece
        case 1:
            //loop through all of columns and place pieces into it
            for(int i = 0; i < 8; i++) {
                NextPiece[i] = tPiece[rot][i];
            }
            //set max horizontals for square piece (Make sure its <= or >= when checking)
            if(rot == 0) {
              maxHorLeft = 10;
              maxHorRight = 5;
            }
            if(rot == 1) {
              maxHorLeft = 11;
              maxHorRight = 5;
            }
            if(rot == 2) {
              maxHorLeft = 11;
              maxHorRight = 5;
            }
            if(rot == 3) {
              maxHorLeft = 11;
              maxHorRight = 6;
            }
            break;
        // | Piece
        case 2:
            //loop through all of columns and place pieces into it
            for(int i = 0; i < 8; i++) {
                NextPiece[i] = lPiece[rot][i];
            }
            //set max horizontals for square piece (Make sure its <= or >= when checking)
            if(rot == 0) {
              maxHorLeft = 12;
              maxHorRight = 5;
            }
            if(rot == 1) {
              maxHorLeft = 11;
              maxHorRight = 7;
            }
            if(rot == 2) {
              maxHorLeft = 11;
              maxHorRight = 4;
            }
            if(rot == 3) {
              maxHorLeft = 9;
              maxHorRight = 5;
            }
            break;
        //L Piece
        case 3:
            //loop through all of columns and place pieces into it
            for(int i = 0; i < 8; i++) {
                NextPiece[i] = LPiece[rot][i];
            }
            //set max horizontals for square piece (Make sure its <= or >= when checking)
            if(rot == 0) {
              maxHorLeft = 11;
              maxHorRight = 5;
            }
            if(rot == 1) {
              maxHorLeft = 11;
              maxHorRight = 6;
            }
            if(rot == 2) {
              maxHorLeft = 11;
              maxHorRight = 5;
            }
            if(rot == 3) {
              maxHorLeft = 10;
              maxHorRight = 5;
            }
            break;
        // z Piece
        case 4:
            //loop through all of columns and place pieces into it
            for(int i = 0; i < 8; i++) {
                NextPiece[i] = zPiece[rot][i];
            }
            //set max horizontals for square piece (Make sure its <= or >= when checking)
            if(rot == 0) {
              maxHorLeft = 11;
              maxHorRight = 5;
            }
            if(rot == 1) {
              maxHorLeft = 11;
              maxHorRight = 6;
            }
            if(rot == 2) {
              maxHorLeft = 11;
              maxHorRight = 5;
            }
            if(rot == 3) {
              maxHorLeft = 10;
              maxHorRight = 5;
            }
            break;
        default:
            //have L piece as default for now
            //loop through all of columns and place pieces into it
            for(int i = 0; i < 8; i++) {
                NextPiece[i] = LPiece[rot][i];
            }
            //set max horizontals for square piece (Make sure its <= or >= when checking)
            if(rot == 0) {
              maxHorLeft = 11;
              maxHorRight = 5;
            }
            if(rot == 1) {
              maxHorLeft = 11;
              maxHorRight = 6;
            }
            if(rot == 2) {
              maxHorLeft = 11;
              maxHorRight = 5;
            }
            if(rot == 3) {
              maxHorLeft = 10;
              maxHorRight = 5;
            }
            break;
    }
}

//
//  Functions for piece placement validations
//

//Normal droping validation
bool checkDrop() {
  //See if the dropping piece is in collision
  //Loop through all columns to output
  for(int i = 0; i < 8; i++) {
    //if output and tetrius frid have a colliusion or if bit mask does not equal 0
    if((NextPiece[(i + horPos) % 8] << (numTiles + verPos) & tetrisGrid[i]) != 0x00000000) {
      return false; //has a collision
    }
  }
  return true;  //No colliusion
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
    outGrid[i] = tetrisGrid[i] | (NextPiece[(i + horPos) % 8] << (numTiles + verPos));  //Change square piece position with changes in vertical and horizontal position
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
const unsigned long TASK1_PERIOD = 250;     //LED Matrix
const unsigned long TASK2_PERIOD = 50;     //JoyStick

task tasks[NUM_TASKS]; // declared task array with 5 tasks

//  LED Matrix States
enum task1_states {task1_start, drop, checkTetris, off} task1_state;
//  Joystick Left and Right State
enum task2_states {task2_start, idle, left, right, hold} task2_state;

int varX;

void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {           // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {         // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state);     // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                              // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                      // Increment the elapsed time by GCD_PERIOD
    randomNum++;  //INcrement random number
	}
}


//
//  LED Matrix Tick Function
//

//  Pipeline for LED matrix for each matrix
unsigned long x = 0x00000000; // b4 b3 b2 b1

unsigned int b1 = (x & 0xff);
unsigned int b2 = (x >> 8) & 0xff;
unsigned int b3 = (x >> 16) & 0xff;
unsigned int b4 = (x >> 24);

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
          // Initially Increment after checking drops and everything
          numTiles++;
          state = drop;
          
          //update output grid
          updateOutput();


          //have rotation check and fast drop with joystick here


          //Collsiuon check
          if(!checkDrop()) {
            //Re update output grid to old one
              numTiles--;
              //update output grid
              updateOutput();
              //New state
              numTiles = 0;
              state = checkTetris;
          }

          //Case for when its at the very last tile
          if(numTiles >= 28) {
            //change output to old one
            numTiles--;
            //update output grid
            updateOutput();
            //New state
            numTiles = 0;
            state = checkTetris;
          }
          break;
        case checkTetris:
          numTiles++;
          state = checkTetris;

          
          //Update Tetris
          if(numTiles >= 20) {
            //Change tetris board with new pieces
            updateTetris();

            numTiles = 0;
            state = off;
          }

          break;
        case off:
          state = drop;
          //reset hor position and rotation
          horPos = 8;
          rot = 0;

          next();
          setPiece();

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
        case checkTetris:
          //do nothing For now
          
          //Have a check if they get tetris or get a row
            //loop through all of Tetris or output grid
            //then outgrid[i] & 0x00000001 == 0x00000001 for one row then check again and keep looping till not there


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
// Left and Right Tick Function
//
int task2_tick(int state) {
    //state transitions
    switch(state) {
        case task2_start:
          state = idle;
          break;
        case idle:
          //stay off if no input is done
            if(!((PINC>>3) & 0x01) && !((PINC>>4) & 0x01)) {
                state = idle;
            }
            //left button is turned on but reight not
            else if(((PINC>>3) & 0x01)) {
                state = left;
            }
            //right button is turned on but left not
            else if(((PINC>>4) & 0x01)) {
                state = right;
            }
            //stay in off just in case
            else {
                state = idle;
            }
          break;
        case left:
          state = hold;
          break;
        case right:
          state = hold;
          break;
        case hold:
          //stay in hold while holdinhg
          if(((PINC>>3) & 0x01) || ((PINC>>4) & 0x01)) {
            state = hold;
          }
          //go back to off when not pressing anymore
          else {
            state = idle;
          }
          break;
    }

    //state functions
    switch(state) {
        case task2_start:
          //ignore do nothing
          break;
        case idle:
          //do nothingh
          break;
        case left:
          if(horPos < maxHorLeft) horPos++;
          break;
        case right:
          if(horPos > maxHorRight) horPos--;
          break;
        case hold:
          //do nothjing
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
    DDRC = 0x00; PORTC = 0xFF; //all inputs for C
    DDRB = 0xFF; PORTB = 0x00; //all outputs for B
    //DDRD = 0xFF; PORTD = 0x00; //all outputs for D

    ADC_init();     // initializes ADC
    SPI_INIT();     // Initialize SPI protocol
    Matrix_Init();  // Initialize 8x8 Led matrix
    queue_init();   // Initialize queue with first 4 pieces
    setPiece();     // Set First piece 
    serial_init (9600);

    
    //Task Initialization
    //LED Matrix task initialization
    tasks[0].period = TASK1_PERIOD;
    tasks[0].state = task1_start;
    tasks[0].elapsedTime = TASK1_PERIOD;
    tasks[0].TickFct = &task1_tick;

    //Joystick left and right task initialization
    tasks[1].period = TASK2_PERIOD;
    tasks[1].state = task2_start;
    tasks[1].elapsedTime = TASK2_PERIOD;
    tasks[1].TickFct = &task2_tick;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {
        
        TimerISR();
        while (!TimerFlag){}  // Wait for SM period
        TimerFlag = 0;        // Lower flag

    }

    return 0;

}