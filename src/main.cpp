/*        Your Name & E-mail: Jyro Jimnez, jjime180@ucr.edu

*         Discussion Section: 21

*         Assignment: Custom Lab project Final Iteration

*         Exercise Description: Simple Tetris Game using an 8x8 LED matrix

*        

*         I acknowledge all content contained herein, excluding template or example code, is my own original work.

*

*         Demo Link: <URL>

*/

//Utility Libraries
#include "timerISR.h"     //Timer ISR
#include "helper.h"       //For Bit access functions
#include "periph.h"       //For ADC Utility Function
#include "spiAVR.h"       //For SPI Communication with 8x8 LED Matrix
#include "queue.h"        //For Queue implementation for Tetris Block organization
#include "LCD.h"          //LCD helper Library for output
//Library for random
#include "stdlib.h"

#define NUM_TASKS 4     //Macro for total number of tasks


/*
        Variables to handle score in LCD screen
*/

//Variable for Score
int score = 0;
char scoreOut[4] = {' ', ' ', ' ', '0'};

//Score Util Function
//function to translate int value to a string
void convertInt(int num, char scoreOut[]) {
  //temp int value to read
  int x = 0;
  //multiply by 10 initialy to work with loop
  num *= 10;
  for(int i = 0; i < 3; i++) {
    //divide by 10 to get number places
    num /= 10;
    //mod 10 to isolate number place
    x = num % 10;
    //for last number
    if(i == 2 && x >= 1) {
        scoreOut[0] = '0' + x;
    }
    //for secnd dnumber
    if(i == 1 && x >= 1) {
        scoreOut[1] = '0' + x;
    }
    //for first number
    if(i == 0) {
        scoreOut[2] = '0' + x;
    }
  }
}


/*
        Global Variables to haandle game logic
*/

//Variable for pseudo random number
int randomNum = 0;

//bool variables to know if game over or not
bool gameOver = 0;

/*
        Passive Buzzer Functions and whatnot
*/

//Chords ICR1 Values (Short num of chords for now, wanna add more later if have time)
int C = 7644;
int D = 6812;
int E = 6067;
int F = 5718;
int G = 5103;
int A = 4545;
int B = 4044;

const int size = 130;

//array to play music (Melody of Little Root Town Pokemon)
int chords[size] = {
  F, F, G, G,
  A, A, A*2, A*2, C, C, G, G,
  A, A, G, G, A, A, B, B, 
  0, 0, C, C, 0, 0, D, 0,
  A, A, G, G, A, A, C, C,
  D, D, 0, 0, E, E, 0, 0,
  D, D, 0, 0, A, A, G, G,
  F, F, E, E, F, F, A, A,
  0, 0, 0, 0, D, D, E, E,
  F, F, A*2, A*2, B, B, D, D,
  C, C, B, B, 0, B, A, A,
  F, F, A*2, A*2, B, B, D, D,
  D, D, B, B, A, A, G, G,
  F, 0, 0, 0, 0, 0, 0, 0, 
  F, F, D, D, F, F, E, E, 
  0, 0, 0, F, 0, 0, G, G,
  0, 0, C, C, B, B
};

//Counter Variables
int j = 0;
int u = 0;


//Wanna add more music and sound effects if i had more time


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


/*
        Array for Game Over
*/

unsigned long over[8] = {
  0x00000000, 
  0x7E001E00, 
  0x127E207E, 
  0x124A4042, 
  0x124A4042, 
  0x124A207E, 
  0x6C001E00, 
  0x00000000
};

/*
        Tetris Grid Arrays and Variables
*/

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
  0x00000004, 
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
  randomNum = rand();
  nextPCS.push(randomNum % 5);
  randomNum = rand();
  nextPCS.push(randomNum  % 5);
  randomNum = rand();
  nextPCS.push(randomNum  % 5);
  randomNum = rand();
  nextPCS.push(randomNum  % 5);
}

//Get the next piece after using one up
void next() {
  nextPCS.pop();
  //set RandomNum to new number
  randomNum = rand();
  //push a new piece to the queue somwhat randomly
  nextPCS.push(randomNum % 5);
}

//Setting what the next piece is with its parameters
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

//line check variables
int numLines = 0;

//Line Check Functions
bool checkLine() {
  //loop through all of columns and place pieces into it
  for(int i = 0; i < 8; i++) {
    if((outGrid[i] & 0x80000000) != 0x80000000) {
      return false;  //No line made
    }
  }

  //Line made cause got here
  return true;
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
const unsigned long TASK1_PERIOD = 250;    //LED Matrix
const unsigned long TASK2_PERIOD = 50;     //Buttons Task for left, right, and rotate
const unsigned long TASK3_PERIOD = 250;    //LCD for score output
const unsigned long TASK4_PERIOD = 250;    //Passive Buzzer to play a score/melody

task tasks[NUM_TASKS]; // declared task array with 5 tasks

//  LED Matrix States
enum task1_states {task1_start, drop, checkTetris, off, loss} task1_state;
//  Buttons Left, Right and Rotate State
enum task2_states {task2_start, idle, left, right, rotate, hold} task2_state;
//  LCD Task States
enum task3_states {task3_start, update, game, noGame} task3_state;
//  Passive Buzzer Task to play some music :)
enum task4_states {task4_start, playSong, pressMute, muteSong, pressPlay} task4_state;

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

          //Collsiuon check
          if(!checkDrop()) {
              //Re update output grid to old one
              numTiles--;
              //update output grid
              updateOutput();
              //New state
              //Check if it is  loss
              if(numTiles <= 3) {
                gameOver = 1; //game over flag
              }
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
          if(numTiles >= 4) {
            //Change tetris board with new pieces
            updateTetris();
            numTiles = 0;
            state = off;

            //get score for person
            //just one line so no multiplier
            if(numLines == 1) {
              score++;
            }
            // 2 lines so 2x the score
            if(numLines == 2) {
              score += 2;
            }
            // 3 lines so 5 times the score
            if(numLines == 3) {
              score += 5;
            }
            // 4 lines so 8 times the score
            if(numLines == 4) {
              score += 8;
            }

            //rest numLines
            numLines = 0;


            //go to loss if game over flag is on
            if(gameOver == 1) {
              state = loss;
            }
          }

          break;
        case off:
          state = drop;
          //reset hor position and rotation
          horPos = 8;
          rot = 0;

          //Set next piece
          next();
          setPiece();

          break;
        case loss:
          //Loss state so keep it like that for now
          if(gameOver == 1) {
            state = loss;
          }
          else {
            state = drop;
            //reset everything
            gameOver = 0;
            horPos = 0;
            rot = 0;
            numLines = 0;
            score = 0;
            //Set next piece
            next();
            setPiece();

            for(int i = 0; i < 8; i++) {
              outGrid[i] = 0x00000000;
              tetrisGrid[i] = 0;
            }
          }

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
          //Have a check if they get tetris or get a row
          if(checkLine()) {
            //increment number of lines got for score
            numLines++;
            for(int i = 0; i < 8; i++) {
              outGrid[i] = outGrid[i] << 1;
            }
          }
          
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
        case loss:
          //Output over statement
          for(int i = 0; i < 8; i++) {
            //Convert the 32bit binary to 4 8-bit binary
            x = over[i];   //i + 8 to control where we want to move piecves horizontally
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
    }

    //return satte
    return state;
}


//
// Button Left, Right and Rotate Tick Function
//

int task2_tick(int state) {
    //state transitions
    switch(state) {
        case task2_start:
          state = idle;
          break;
        case idle:
          //stay off if no input is done
            if(!((PINC>>3) & 0x01) && !((PINC>>4) & 0x01) && !((PINC>>2) & 0x01)) {
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
            //Rotate Button
            else if(((PINC>>2) & 0x01)) {
              state = rotate;
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
        case  rotate:
          state = hold;
          break;
        case hold:
          //stay in hold while holdinhg
          if(((PINC>>3) & 0x01) || ((PINC>>4) & 0x01) || ((PINC>>2) & 0x01)) {
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
        case rotate:
          rot = (rot + 1) % 4;
          setPiece();
          break;
        case hold:
          //do nothjing
          break;
    }

    //return satte
    return state;
}


//
//  LED Matrix Tick Function
//

int task3_tick(int state) {
    //state transitions
    switch(state) {
        case task3_start:
          gameOver = 0;
          state = update;
          break;
        case update:
          if(gameOver == 0) {
            state = game;
          }
          else {
            state = noGame;
            j = 0;  //counter for GAME OVER VAR
          }
          break;
        case game:
          if(gameOver == 0) {
            state = game;
          }
          else {
            state = update;
          }
          break;
        case noGame:
          if(j < 50) {
            state = noGame;
          }
          else {
            state = update;
            //reset game over
            gameOver = 0;
          }
          break;
    }

    //state functions
    switch(state) {
        case task3_start:
          //do nothing
          break;
        case update:
          if(gameOver == 0) {
            lcd_clear();
            lcd_goto_xy(0, 0);
            lcd_write_str("Player Score:   ");
          }
          else {
            lcd_clear();
            lcd_goto_xy(0, 0);
            lcd_write_str("Thx 4 Playing");
            lcd_goto_xy(1, 0);
            lcd_write_str("Game Over!");
            //Then Maybe Add a custom character at the end
          }
          break;
        case game:
          //update the score thats it
          lcd_goto_xy(1, 0);
          //output score value
          convertInt(score, scoreOut);
          lcd_write_character(scoreOut[0]);
          lcd_write_character(scoreOut[1]);
          lcd_write_character(scoreOut[2]);
          lcd_write_character(scoreOut[3]);
          lcd_write_str(" Pts");
          break;
        case noGame:
          j++;
          break;
          
    }

    //return satte
    return state;
}



//
//  Passive Buzzer Tick Function
//

//Passive BUZZER Tick function
int task4_tick(int state) {
    //state transitions
    switch(state) {
        case task4_start:
          state = playSong;
          u = 0;
          break;
        case playSong:
          //Mute Button
          if(!((PINC>>1) & 0x01)) {
            state = playSong;
          }
          else {
            state = pressMute;
          }
          break;
        case pressMute:
          //Mute Button
          if(!((PINC>>1) & 0x01)) {
            state = muteSong;
          }
          else {
            state = pressMute;
          }
          break;
        case muteSong:
          //Mute Button
          if(!((PINC>>1) & 0x01)) {
            state = muteSong;
          }
          else {
            state = pressPlay;
          }
          break;
        case pressPlay:
          //Mute Button
          if(!((PINC>>1) & 0x01)) {
            state = playSong;
          }
          else {
            state = pressPlay;
          }
          break;
    }

    //state functions
    switch(state) {
        case task4_start:
          //ignore do nothing
          break;
        case playSong:
          //play song
          TCCR1A |= (1 << WGM11) | (1 << COM1A1); //COM1A1 sets it to channel A
          TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11); //CS11 sets the prescaler to be 8
          //WGM11, WGM12, WGM13 set timer to fast pwm mode

          ICR1 = chords[u]; //pwm period depending on the chord frequency

          OCR1A = chords[u] / 10;

          //increment i
          if(u < size) {
            u++;
          }
          else {
            u = 0;
          }
          break;
        case pressMute:
          //Do Nothing
          break;
        case muteSong:
          //Mute Song Dont play Anything
          //play song
          TCCR1A |= (1 << WGM11) | (1 << COM1A1); //COM1A1 sets it to channel A
          TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11); //CS11 sets the prescaler to be 8
          //WGM11, WGM12, WGM13 set timer to fast pwm mode

          ICR1 = 0; //pwm period depending on the chord frequency

          OCR1A = 0;
          break;
        case pressPlay:
          //do nothing transition phase
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
    DDRD = 0xFF; PORTD = 0x00; //all outputs for D

    ADC_init();     // initializes ADC
    SPI_INIT();     // Initialize SPI protocol
    Matrix_Init();  // Initialize 8x8 Led matrix
    queue_init();   // Initialize queue with first 4 pieces
    setPiece();     // Set First piece 
    lcd_init();     // Initlize LCD
    
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

    //LCD Screen task initialization
    tasks[2].period = TASK3_PERIOD;
    tasks[2].state = task3_start;
    tasks[2].elapsedTime = TASK3_PERIOD;
    tasks[2].TickFct = &task3_tick;
    
    //Passive Buzzer Task Initialization
    tasks[3].period = TASK4_PERIOD;
    tasks[3].state = task4_start;
    tasks[3].elapsedTime = TASK4_PERIOD;
    tasks[3].TickFct = &task4_tick;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {
        
        TimerISR();
        while (!TimerFlag){}  // Wait for SM period
        TimerFlag = 0;        // Lower flag

    }

    return 0;

}