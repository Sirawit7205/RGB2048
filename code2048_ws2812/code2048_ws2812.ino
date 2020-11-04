#include <FastLED.h>

//ddr, pin, port
#define BTN_DDR DDRA
#define BTN_PIN PINA
#define AXLED_DDR DDRB
#define AXLED_PORT PORTB

//fastled and axled
#define LED_COUNT 16
#define LED_DATA_PIN 9
#define AXLED 0

//buttons
#define UP 3
#define DOWN 0
#define LEFT 2
#define RIGHT 1

//board size
#define BOARD_SIZE 16
#define BOARD_WIDTH 4
#define BOARD_HEIGHT 4

//debounce
#define DEBOUNCE_TIME 200

//noise channel
#define NOISE_PIN A7

//colors
CRGB leds[LED_COUNT];
const CRGB colorDef[13] = {CRGB::Black, 
                           CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Chartreuse,
                           CRGB::Lime, CRGB::DarkGreen, CRGB::Cyan, CRGB::Blue,
                           CRGB::RoyalBlue, CRGB::Indigo, CRGB::Violet, CRGB::White};

//game variables
volatile unsigned long lastBtnMillis = 0;   //last time any button is pressed
volatile uint8_t gameover = 0;              //game over indicator
volatile uint8_t board[BOARD_HEIGHT][BOARD_WIDTH] = {};     //game board

void setup() {
  //fastled
  FastLED.addLeds<WS2812, LED_DATA_PIN, RGB>(leds, LED_COUNT);   //for FastLED 3.3.2
  FastLED.setBrightness(25);  //around 10% brightness
  
  //random seed, because at this point AXLED is basically floating
  randomSeed(analogRead(NOISE_PIN));
  
  //setup ports
  BTN_DDR &= ~((1 << UP) | (1 << DOWN) | (1 << LEFT) | (1 << RIGHT));
  AXLED_DDR |= (1 << AXLED);
  AXLED_PORT &= ~(1 << AXLED);

  //init board with two pieces
  randomPlacement();
  randomPlacement();

  //init display
  updateDisplay();
}

void loop() {
  if((((unsigned long)(millis() - lastBtnMillis)) > DEBOUNCE_TIME) && gameover == 0)
  {
    bool validSlide = false;
    
    //getting button inputs
    //then slide the board
    if((BTN_PIN & (1 << UP)) == 0)
      validSlide = slideBoard(UP);
    if((BTN_PIN & (1 << DOWN)) == 0)
      validSlide = slideBoard(DOWN);
    if((BTN_PIN & (1 << LEFT)) == 0)
      validSlide = slideBoard(LEFT);
    if((BTN_PIN & (1 << RIGHT)) == 0)
      validSlide = slideBoard(RIGHT);

    //if the slide is valid, add a piece to board
    if(validSlide)
    {
      //update the display
      updateDisplay();
      
      //place a new piece
      randomPlacement();

      //delay a bit
      delay(50);

      //update the display again
      updateDisplay();
    }

    //check gameover
    gameover = gameoverCheck();

    //if gameover, show AXLED
    if(gameover != 0)
      AXLED_PORT |= (1 << AXLED);    //turn gameover led on

    lastBtnMillis = millis();     //debounce
  }

  delay(10);  //delay a bit
}

void updateDisplay()
{
  //update what to draw
  for(int i = 0; i < BOARD_HEIGHT; i++)           //for each board position
  {
    for(int j = 0; j < BOARD_WIDTH; j++)
    {
      leds[(i * BOARD_WIDTH) + j] = colorDef[board[i][j]];
    }
  }

  //redraw display
  FastLED.show();
}

bool randomPlacement()
{
  uint8_t placementPos = random(BOARD_SIZE);  //where to place
  uint8_t placementVal = random(1, 3);        //what to place (2 or 4)
  uint8_t placementAttempts = 0;              //use to check if placing is impossible

  while(placementAttempts != BOARD_SIZE)
  {
    if(board[placementPos / BOARD_HEIGHT][placementPos % BOARD_WIDTH] == 0)    //empty space
    {
      board[placementPos / BOARD_HEIGHT][placementPos % BOARD_WIDTH] = placementVal;   //place success
      break; 
    }
    else                            //go to next spot
    {
      placementPos++;
      
      if(placementPos == BOARD_SIZE)
        placementPos = 0;
    }
  }

  //success: 0   failed: 1
  if(placementAttempts != BOARD_SIZE)
    return 0;
  else
    return 1;
}

bool slideBoard(uint8_t dir)
{
  bool validMovement = false;
  
  if(dir == UP)
  {
    for(int i = 1; i < BOARD_HEIGHT; i++)
    {
      for(int j = 0; j < BOARD_WIDTH; j++)
      {
        if(board[i][j] == 0)    //skip zeroes
          continue;
        
        int k = i;      //starting point
        while(k > 0)    //try moving
        {
          if(board[k - 1][j] == board[i][j])              //merge on match
          {
            board[k - 1][j]++;      //merge into another block
            board[i][j] = 0;        //clear the old block
            validMovement = true;   //any movement is ok
            break;
          }
          else if(board[k - 1][j] == 0 && k > 1)          //vacant, not edge
          {
            k--;                    //try next block
          }
          else if(board[k - 1][j] == 0 && k == 1)         //vacant, edge
          {
            board[k - 1][j] = board[i][j];  //move block
            board[i][j] = 0;                //clear the old block
            validMovement = true;   //any movement is ok
            break;
          }
          else                                            //path blocked
          {
            board[k][j] = board[i][j];      //move block
            
            if(k != i)                      //not at init position
            {
              board[i][j] = 0;              //clear the old block
              validMovement = true;         //valid only if this is not a starting point
            }
            break;
          }

          //update the display
          updateDisplay();

          //delay a bit
          delay(50);
        }
      }
    }
  }
  else if(dir == DOWN)
  {
    for(int i = BOARD_HEIGHT - 2; i >= 0; i--)
    {
      for(int j = 0; j < BOARD_WIDTH; j++)
      {
        if(board[i][j] == 0)  //skip zeroes
          continue;
        
        int k = i;                    //starting point
        while(k < BOARD_HEIGHT - 1)   //try moving
        {
          if(board[k + 1][j] == board[i][j])              //merge on match
          {
            board[k + 1][j]++;      //merge into another block
            board[i][j] = 0;        //clear the old block
            validMovement = true;   //any movement is ok
            break;
          }
          else if(board[k + 1][j] == 0 && k < BOARD_HEIGHT - 2)  //vacant, not edge
          {
            k++;                    //try next block
          }
          else if(board[k + 1][j] == 0 && k == BOARD_HEIGHT - 2) //vacant, edge
          {
            board[k + 1][j] = board[i][j];  //move block
            board[i][j] = 0;                //clear the old block
            validMovement = true;   //any movement is ok
            break;
          }
          else                                            //path blocked
          {
            board[k][j] = board[i][j];      //move block
            
            if(k != i)                      //not at init position
            {
              board[i][j] = 0;              //clear the old block
              validMovement = true;         //valid only if this is not a starting point
            }
            break;
          }

          //update the display
          updateDisplay();

          //delay a bit
          delay(50);
        }
      }
    }
  }
  else if(dir == LEFT)
  {
    for(int i = 0; i < BOARD_HEIGHT; i++)
    {
      for(int j = 1; j < BOARD_WIDTH; j++)
      {
        if(board[i][j] == 0)  //skip zeroes
          continue;
        
        int k = j;      //starting point
        while(k > 0)    //try moving
        {
          if(board[i][k - 1] == board[i][j])              //merge on match
          {
            board[i][k - 1]++;      //merge into another block
            board[i][j] = 0;        //clear the old block
            validMovement = true;   //any movement is ok
            break;
          }
          else if(board[i][k - 1] == 0 && k > 1)          //vacant, not edge
          {
            k--;                    //try next block
          }
          else if(board[i][k - 1] == 0 && k == 1)         //vacant, edge
          {
            board[i][k - 1] = board[i][j];  //move block
            board[i][j] = 0;                //clear the old block
            validMovement = true;   //any movement is ok
            break;
          }
          else                                            //path blocked
          {
            board[i][k] = board[i][j];      //move block
            
            if(k != j)                      //not at init position
            {
              board[i][j] = 0;                //clear the old block
              validMovement = true;         //valid only if this is not a starting point
            }
            break;
          }

          //update the display
          updateDisplay();

          //delay a bit
          delay(50);
        }
      }
    }
  }
  else if(dir == RIGHT)
  {
    for(int i = 0; i < BOARD_HEIGHT; i++)
    {
      for(int j = BOARD_WIDTH - 2; j >= 0; j--)
      {
        if(board[i][j] == 0)  //skip zeroes
          continue;
        
        int k = j;
        while(k < BOARD_WIDTH - 1)
        {
          if(board[i][k + 1] == board[i][j])              //merge on match
          {
            board[i][k + 1]++;      //merge into another block
            board[i][j] = 0;        //clear the old block
            validMovement = true;   //any movement is ok
            break;
          }
          else if(board[i][k + 1] == 0 && k < BOARD_WIDTH - 2)  //vacant, not edge
          {
            k++;                    //try next block
          }
          else if(board[i][k + 1] == 0 && k == BOARD_WIDTH - 2) //vacant, edge
          {
            board[i][k + 1] = board[i][j];  //move block
            board[i][j] = 0;                //clear the old block
            validMovement = true;   //any movement is ok
            break;
          }
          else                                            //path blocked
          {
            board[i][k] = board[i][j];      //move block
            
            if(k != j)                      //not at init position
            {
              board[i][j] = 0;              //clear the old block
              validMovement = true;         //valid only if this is not a starting point
            }
            break;
          }

          //update the display
          updateDisplay();

          //delay a bit
          delay(50);
        }
      }
    }
  }

  return validMovement;
}

uint8_t gameoverCheck()
{
  //return 0: nothing
  //return 1: no more moves
  //return 2: reach 2048

  //default to no more moves
  //override if there's movable parts or 0 or 2048
  uint8_t returnVal = 1;

  //check blank spaces and 2048 first
  for(int i = 0; i < BOARD_HEIGHT; i++)
  {
    for(int j = 0; j < BOARD_WIDTH; j++)
    {
      if(board[i][j] == 12)         //case 2048
      {
        returnVal = 2;
        break;                      //if there is 2048 we don't have to check further
      }
      else if(board[i][j] == 0)     //case 0
      {
        returnVal = 0;
      }
    }

    //if there is 2048 we don't have to check further
    if(returnVal == 2)
      break;
  }

  //if returnVal is still 1, we have to check for valid moves
  if(returnVal == 1)
  {
    for(int i = 0; i < BOARD_HEIGHT; i++)
    {
      for(int j = 0; j < BOARD_WIDTH; j++)
      {
        if(i != 0 && board[i][j] == board[i - 1][j])
          returnVal = 0;
        else if(i != BOARD_HEIGHT - 1 && board[i][j] == board[i + 1][j])
          returnVal = 0;
        else if(j !=0 && board[i][j] == board[i][j - 1])
          returnVal = 0;
        else if(j != BOARD_WIDTH - 1 && board[i][j] == board[i][j + 1])
          returnVal = 0;

        //if returnVal is now 0, we can break
        if(returnVal == 0)
          break;
      }
      
      //if returnVal is now 0, we can break
      if(returnVal == 0)
        break;
    }
  }

  return returnVal;
}
