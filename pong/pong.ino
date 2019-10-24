
#include <SPI.h>
#include "DMD.h"
#include <TimerOne.h>

// pin definitions
const int player_one_axis = A0;
const int player_one_button = A1;
const int player_two_axis = A2;
const int player_two_button = A3;
const int buzzer_pin = 4;

// gameplay definitions
const int batsize = 4;
const long update_rate = 30;

//tones and sounds while playing
const int ball_paddle_tone = 512;
const int ball_wall_tone = 128;
const int serve_tone = 256;

DMD dmd(1, 1);

const int rawNumberPixelData[] = {32319, 31744, 24253, 32437, 31879, 30391, 30399, 31777, 32447, 32439};
int playerOneScore = 0;
int playerTwoScore = 0;
int ballX = 0;
int ballY = 0;
int ballChangeX = 0;
int ballChangeY = 0;
int speakerTone = 0;


void ScanDMD()
{
  dmd.scanDisplayBySPI();
}

void setup()
{
  Serial.begin(9600);

  Timer1.initialize(2000);         //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
  Timer1.attachInterrupt(ScanDMD); //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()

  dmd.clearScreen(true); //clear/init the DMD pixels held in RAM

  pinMode(player_one_button, INPUT_PULLUP);
  pinMode(player_two_button, INPUT_PULLUP);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW); //ground for speaker
}

void loop()
{
  int speakerTone = 0; //default to no tone playing
  int p1, p2;
  //read and display player 1 bat
  p1 = analogRead(player_one_axis);
  p1 = p1 / 60 - 1;
  if (p1 < 0)
  {
    p1 = 0;
  }
  if (p1 + batsize > 16)
  {
    p1 = 16 - batsize;
  }
  //read and display player 2 bat
  p2 = analogRead(player_two_axis);
  p2 = p2 / 60 - 1;
  if (p2 < 0)
  {
    p2 = 0;
  }
  if (p2 + batsize > 16)
  {
    p2 = 16 - batsize;
  }
  //display ball
  if ((ballX == 0) && (ballChangeX == 0))
  {
    ballY = p1 + batsize / 2 - 1;
    if (!digitalRead(player_one_button))
    {
      ballChangeX = 1;
      ballChangeY = 0;
      speakerTone = serve_tone;
    } //serve
  }
  if ((ballX == 30) && (ballChangeX == 0))
  {
    ballY = p2 + batsize / 2 - 1;
    if (!digitalRead(player_two_button))
    {
      ballChangeX = -1;
      ballChangeY = 0;
      speakerTone = serve_tone;
    } //serve
  }
  ballX = ballX + ballChangeX;
  if (ballX > 30)
  {
    ballX = 0;
    ballChangeX = 0;
    ballChangeY = 0;
    playerOneScore = playerOneScore + 1;
    if (playerOneScore == 7)
    {
      playerOneVictory();
    }
  } //P2 has missed, P1 wins
  if (ballX < 0)
  {
    ballX = 30;
    ballChangeX = 0;
    ballChangeY = 0;
    playerTwoScore = playerTwoScore + 1;
    if (playerTwoScore == 7)
    {
      playerTwoVictory();
    }
  } //P1 has missed, P2 wins
  if (ballX == 29)
  { //ball is in player 2's court
    if (abs(ballY - p2 - 1) < 3)
    {                               //ball is within p2's bat
      ballChangeX = -1;             //goes back left
      ballChangeY = ballY - p2 - 1; //change ball angle
      if (ballChangeY == 0)
      {
        ballChangeY = random(-1, 2);
      }                               //mix it up a bit
      speakerTone = ball_paddle_tone; //hit bat
    }
  }
  if (ballX == 1)
  { //ball is in player 1's court
    if (abs(ballY - p1 - 1) < 3)
    {                               //ball is within p1's bat
      ballChangeX = 1;              //goes back right
      ballChangeY = ballY - p1 - 1; //change ball angle
      if (ballChangeY == 0)
      {
        ballChangeY = random(-1, 2);
      }                               //mix it up a bit
      speakerTone = ball_paddle_tone; //hit bat
    }
  }
  int ballChangeYtemp; //to work out half steps
  if (ballX & 1)
  { //on odd steps, only step if 2
    ballChangeYtemp = ballChangeY / 2;
  }
  else
  { //on even steps, step if 1 or 2
    ballChangeYtemp = 0;
    if (ballChangeY > 0)
    {
      ballChangeYtemp = 1;
    }
    if (ballChangeY < 0)
    {
      ballChangeYtemp = -1;
    }
  }
  ballY = ballY + ballChangeYtemp;
  if (ballY > 13)
  {
    ballY = 13;
    ballChangeY = -1;
    speakerTone = ball_wall_tone;
  } //hit wall
  if (ballY < 1)
  {
    ballY = 1;
    ballChangeY = 1;
    speakerTone = ball_wall_tone;
  } //hit wall
  //redraw screen from scratch every frame
  dmd.clearScreen(true); //clear/init the DMD pixels held in RAM
  drawNet();
  drawNumber(11, 0, playerOneScore);
  drawNumber(18, 0, playerTwoScore);
  drawBall(ballX, ballY);
  drawPaddle(0, p1, batsize);
  drawPaddle(31, p2, batsize);

  if (speakerTone)
  {
    tone(buzzer_pin, speakerTone);
  }
  else
  {
    noTone(buzzer_pin);
  } //play tone until next time

  delay(update_rate);
}

void drawNet()
{
  for (int i = 0; i < 16; i++)
  {
    dmd.writePixel(15 + (i % 2), i, GRAPHICS_NORMAL, 1);
  }
}

void drawNumber(int x, int y, int n)
{
  for (int i = 0; i < 15; i++)
  {
    if (rawNumberPixelData[n % 10] & (1 << i))
    {
      dmd.writePixel(x + (i / 5), y + (i % 5), GRAPHICS_NORMAL, 1);
    }
  }
}

void drawBall(int x, int y)
{ //draw 2x2 ball at x,y
  dmd.writePixel(x, y, GRAPHICS_NORMAL, 1);
  dmd.writePixel(x + 1, y, GRAPHICS_NORMAL, 1);
  dmd.writePixel(x, y + 1, GRAPHICS_NORMAL, 1);
  dmd.writePixel(x + 1, y + 1, GRAPHICS_NORMAL, 1);
}

void drawPaddle(int x, int y, int s)
{ //draw paddle starting at x,y, extending s down
  for (int i = 0; i < s; i++)
  {
    dmd.writePixel(x, y + i, GRAPHICS_NORMAL, 1);
  }
}

void playerTwoVictory()
{
  int i;
  for (i = 0; i < 8; i++)
  {
    tone(buzzer_pin, i * 128);
    dmd.clearScreen(true); //clear/init the DMD pixels held in RAM
    drawNet();
    drawNumber(11, 0, playerOneScore);
    drawNumber(18, 0, playerTwoScore);
    drawPaddle(0, 8, batsize);
    drawPaddle(31, 8, batsize);
    delay(300);
    dmd.clearScreen(true); //clear/init the DMD pixels held in RAM
    drawNet();
    drawNumber(11, 0, playerOneScore); //flash P2 score
    drawPaddle(0, 8, batsize);
    drawPaddle(31, 8, batsize);
    delay(300);
  }
  //reset game state
  playerOneScore = 0;
  playerTwoScore = 0;
  ballX = 0;
  ballY = 0;
  ballChangeX = 0;
  ballChangeY = 0;
}

void playerOneVictory()
{
  int i;
  for (i = 0; i < 8; i++)
  {
    tone(buzzer_pin, i * 128);
    dmd.clearScreen(true); //clear/init the DMD pixels held in RAM
    drawNet();
    drawNumber(11, 0, playerOneScore);
    drawNumber(18, 0, playerTwoScore);
    drawPaddle(0, 8, batsize);
    drawPaddle(31, 8, batsize);
    delay(300);
    dmd.clearScreen(true); //clear/init the DMD pixels held in RAM
    drawNet();
    drawNumber(18, 0, playerTwoScore); //flash P1 score
    drawPaddle(0, 8, batsize);
    drawPaddle(31, 8, batsize);
    delay(300);
  }
  //reset game state
  playerOneScore = 0;
  playerTwoScore = 0;
  ballX = 0;
  ballY = 0;
  ballChangeX = 0;
  ballChangeY = 0;
}
