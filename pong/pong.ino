#include <SPI.h>      //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include "DMD.h"      //
#include <TimerOne.h> //

#define STICK1 A0
#define STICK2 A2
#define BUTTON1 A1
#define BUTTON2 A3
#define BATSIZE 4
#define SPEAKERPIN 4

DMD dmd(1, 1);

int nbm[] = {32319, 31744, 24253, 32437, 31879, 30391, 30399, 31777, 32447, 32439};
int score1 = 0;
int score2 = 0;
int ballx = 0;
int bally = 0;
int ballu = 0;
int ballv = 0;

void ScanDMD()
{
  dmd.scanDisplayBySPI();
}

void setup()
{
  Timer1.initialize(2000);         //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
  Timer1.attachInterrupt(ScanDMD); //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()
  dmd.clearScreen(true);           //clear/init the DMD pixels held in RAM
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW); //ground for speaker
}

void loop()
{
  int speakertone = 0; //default to no tone playing
  int p1, p2;
  //read and display player 1 bat
  p1 = analogRead(STICK1);
  p1 = p1 / 60 - 1;
  if (p1 < 0)
  {
    p1 = 0;
  }
  if (p1 + BATSIZE > 16)
  {
    p1 = 16 - BATSIZE;
  }
  //read and display player 2 bat
  p2 = analogRead(STICK2);
  p2 = p2 / 60 - 1;
  if (p2 < 0)
  {
    p2 = 0;
  }
  if (p2 + BATSIZE > 16)
  {
    p2 = 16 - BATSIZE;
  }
  //display ball
  if ((ballx == 0) && (ballu == 0))
  {
    bally = p1 + BATSIZE / 2 - 1;
    if (!digitalRead(BUTTON1))
    {
      ballu = 1;
      ballv = 0;
      speakertone = 256;
    } //serve
  }
  if ((ballx == 30) && (ballu == 0))
  {
    bally = p2 + BATSIZE / 2 - 1;
    if (!digitalRead(BUTTON2))
    {
      ballu = -1;
      ballv = 0;
      speakertone = 256;
    } //serve
  }
  ballx = ballx + ballu;
  if (ballx > 30)
  {
    ballx = 0;
    ballu = 0;
    ballv = 0;
    score1 = score1 + 1;
    if (score1 == 7)
    {
      p1victory();
    }
  } //P2 has missed, P1 wins
  if (ballx < 0)
  {
    ballx = 30;
    ballu = 0;
    ballv = 0;
    score2 = score2 + 1;
    if (score2 == 7)
    {
      p2victory();
    }
  } //P1 has missed, P2 wins
  if (ballx == 29)
  { //ball is in player 2's court
    if (abs(bally - p2 - 1) < 3)
    {                         //ball is within p2's bat
      ballu = -1;             //goes back left
      ballv = bally - p2 - 1; //change ball angle
      if (ballv == 0)
      {
        ballv = random(-1, 2);
      }                  //mix it up a bit
      speakertone = 512; //hit bat
    }
  }
  if (ballx == 1)
  { //ball is in player 1's court
    if (abs(bally - p1 - 1) < 3)
    {                         //ball is within p1's bat
      ballu = 1;              //goes back right
      ballv = bally - p1 - 1; //change ball angle
      if (ballv == 0)
      {
        ballv = random(-1, 2);
      }                  //mix it up a bit
      speakertone = 512; //hit bat
    }
  }
  int ballvtemp; //to work out half steps
  if (ballx & 1)
  { //on odd steps, only step if 2
    ballvtemp = ballv / 2;
  }
  else
  { //on even steps, step if 1 or 2
    ballvtemp = 0;
    if (ballv > 0)
    {
      ballvtemp = 1;
    }
    if (ballv < 0)
    {
      ballvtemp = -1;
    }
  }
  bally = bally + ballvtemp;
  if (bally > 13)
  {
    bally = 13;
    ballv = -1;
    speakertone = 128;
  } //hit wall
  if (bally < 1)
  {
    bally = 1;
    ballv = 1;
    speakertone = 128;
  } //hit wall
  //redraw screen from scratch every frame
  dmd.clearScreen(true); //clear/init the DMD pixels held in RAM
  net();
  num(11, 0, score1);
  num(18, 0, score2);
  ball(ballx, bally);
  paddle(0, p1, BATSIZE);
  paddle(31, p2, BATSIZE);

  if (speakertone)
  {
    tone(SPEAKERPIN, speakertone);
  }
  else
  {
    noTone(SPEAKERPIN);
  } //play tone until next time
  delay(30);
}

void net()
{
  for (int i = 0; i < 16; i++)
  {
    dmd.writePixel(15 + (i % 2), i, GRAPHICS_NORMAL, 1);
  }
}

void num(int x, int y, int n)
{
  for (int i = 0; i < 15; i++)
  {
    if (nbm[n % 10] & (1 << i))
    {
      dmd.writePixel(x + (i / 5), y + (i % 5), GRAPHICS_NORMAL, 1);
    }
  }
}

void ball(int x, int y)
{ //draw 2x2 ball at x,y
  dmd.writePixel(x, y, GRAPHICS_NORMAL, 1);
  dmd.writePixel(x + 1, y, GRAPHICS_NORMAL, 1);
  dmd.writePixel(x, y + 1, GRAPHICS_NORMAL, 1);
  dmd.writePixel(x + 1, y + 1, GRAPHICS_NORMAL, 1);
}

void paddle(int x, int y, int s)
{ //draw paddle starting at x,y, extending s down
  for (int i = 0; i < s; i++)
  {
    dmd.writePixel(x, y + i, GRAPHICS_NORMAL, 1);
  }
}

void p2victory()
{
  int i;
  for (i = 0; i < 8; i++)
  {
    tone(SPEAKERPIN, i * 128);
    dmd.clearScreen(true); //clear/init the DMD pixels held in RAM
    net();
    num(11, 0, score1);
    num(18, 0, score2);
    paddle(0, 8, BATSIZE);
    paddle(31, 8, BATSIZE);
    delay(300);
    dmd.clearScreen(true); //clear/init the DMD pixels held in RAM
    net();
    num(11, 0, score1); //flash P2 score
    paddle(0, 8, BATSIZE);
    paddle(31, 8, BATSIZE);
    delay(300);
  }
  //reset game state
  score1 = 0;
  score2 = 0;
  ballx = 0;
  bally = 0;
  ballu = 0;
  ballv = 0;
}

void p1victory()
{
  int i;
  for (i = 0; i < 8; i++)
  {
    tone(SPEAKERPIN, i * 128);
    dmd.clearScreen(true); //clear/init the DMD pixels held in RAM
    net();
    num(11, 0, score1);
    num(18, 0, score2);
    paddle(0, 8, BATSIZE);
    paddle(31, 8, BATSIZE);
    delay(300);
    dmd.clearScreen(true); //clear/init the DMD pixels held in RAM
    net();
    num(18, 0, score2); //flash P1 score
    paddle(0, 8, BATSIZE);
    paddle(31, 8, BATSIZE);
    delay(300);
  }
  //reset game state
  score1 = 0;
  score2 = 0;
  ballx = 0;
  bally = 0;
  ballu = 0;
  ballv = 0;
}
