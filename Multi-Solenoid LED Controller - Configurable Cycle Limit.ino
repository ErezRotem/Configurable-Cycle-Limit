#include <Arduino.h>
#include <LiquidCrystal.h>
#include <MsTimer2.h>

void doType(bool);
void stopTyping();
void timerFired();
int getIncrementStep(int);
void refreshDisplay(bool);
byte readButton();

// Select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Define some values used by the panel and buttons
#define BUTTON_RIGHT 0
#define BUTTON_UP 1
#define BUTTON_DOWN 2
#define BUTTON_LEFT 3
#define BUTTON_SELECT 4
#define BUTTON_NONE 5

#define BUTTON_READ_INTERVAL 30
#define BUTTON_REPEAT_INTERVAL 30
#define BUTTON_REPEAT_THRESHOLD 600

//
volatile int numberOfKeys = 2;
volatile int holdDuration = 50;
volatile int keyInterval = 100;
volatile int cycleTime = 30;

#define MIN_KEYS 1
#define MAX_KEYS 4
#define MIN_HOLD 5
#define MAX_HOLD 900
#define MIN_INTERVAL 50
#define MAX_INTERVAL 900
#define MIN_CYCLETIME 1
#define MAX_CYCLETIME 3600

#define RESOLUTION 1

#define INDEX_KEYS 0
#define INDEX_HOLD 1
#define INDEX_INTERVAL 2
#define NUMBER_OF_MENU_ITEMS 3

//
volatile byte selectionIndex = INDEX_KEYS;

volatile boolean isTyping = false;

void setup()
{
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);

  lcd.begin(16, 2);
  refreshDisplay(true);

  MsTimer2::set(RESOLUTION, timerFired);
  MsTimer2::start();
}

void loop()
{
}

void doType(bool startTyping)
{
  static unsigned long startTime;
  static unsigned long cycle[MAX_KEYS];
  unsigned long lapseTime;

  if (startTyping)
  {
    lcd.setCursor(0, 0);
    lcd.print("    ");
    startTime = millis();
    lapseTime = 0;
    cycle[0] = cycle[1] = cycle[2] = cycle[3] = 0;
  }
  else
  {
    lapseTime = millis() - startTime;
  }

  if (cycleTime * 1000 >= lapseTime)
  {
    for (int i = 0; i < numberOfKeys; i++)
    {
      if (lapseTime >= (numberOfKeys * ((cycle[i] + 1) / 2) + i) * keyInterval)
      {
        cycle[i]++;
        digitalWrite(i, HIGH);
        lcd.setCursor(i, 0);
        lcd.print("*");
      }
      if (lapseTime >= (cycle[i] / 2 * numberOfKeys + i) * keyInterval + holdDuration)
      {
        cycle[i]++;
        digitalWrite(i, LOW);
        lcd.setCursor(i, 0);
        lcd.print(" ");
      }
    }
  }
  else 
  {
    stopTyping();
    isTyping = false;
    refreshDisplay(BUTTON_SELECT);
  }
}

void stopTyping()
{
  for (int i = 0; i < numberOfKeys; i++)
  {
    digitalWrite(i, LOW);
  }
}

void timerFired()
{
  static unsigned long lastButtonPressed = 0;
  static unsigned long lastButtonReleased = 0;
  static unsigned long lastButtonSent = 0;
  byte button = BUTTON_NONE;
  unsigned long timeRead;

  timeRead = millis();
  if (timeRead % BUTTON_READ_INTERVAL == 0)
  {
    button = readButton();
    if (button != BUTTON_NONE)
    {
      if (lastButtonPressed <= lastButtonReleased)
      {
        lastButtonPressed = timeRead;
      }
    }
    else
    {
      if (lastButtonPressed >= lastButtonReleased)
      {
        lastButtonReleased = timeRead;
      }
    }

    if (lastButtonPressed > lastButtonReleased)
    {
      if ((lastButtonSent < lastButtonPressed && lastButtonPressed - lastButtonReleased >= BUTTON_READ_INTERVAL) || (lastButtonSent >= lastButtonPressed && timeRead - lastButtonPressed > BUTTON_REPEAT_THRESHOLD && timeRead - lastButtonSent >= BUTTON_REPEAT_INTERVAL))
      {
        lastButtonSent = timeRead;
      }
      else
      {
        button = BUTTON_NONE;
      }
    }
  }

  switch (button)
  {
  case BUTTON_SELECT:
    if (isTyping)
    {
      stopTyping();
      isTyping = false;
    }
    else
    {
      isTyping = true;
    }
    break;
  case BUTTON_RIGHT:
    selectionIndex++;
    selectionIndex %= NUMBER_OF_MENU_ITEMS;
    break;
  case BUTTON_LEFT:
    if (selectionIndex > 0)
    {
      selectionIndex--;
    }
    else
    {
      selectionIndex = NUMBER_OF_MENU_ITEMS - 1;
    }
    break;
  case BUTTON_UP:
    switch (selectionIndex)
    {
    case INDEX_KEYS:
      cycleTime++;
      cycleTime = min(cycleTime, MAX_CYCLETIME);
      break;
    case INDEX_HOLD:
      holdDuration += getIncrementStep(holdDuration);
      holdDuration = min(holdDuration, MAX_HOLD);
      break;
    case INDEX_INTERVAL:
      keyInterval += getIncrementStep(keyInterval);
      keyInterval = min(keyInterval, MAX_INTERVAL);
      break;
    }
    break;
  case BUTTON_DOWN:
    switch (selectionIndex)
    {
    case INDEX_KEYS:
      cycleTime--;
      cycleTime = max(cycleTime, MIN_CYCLETIME);
      break;
    case INDEX_HOLD:
      holdDuration -= getIncrementStep(holdDuration - 1);
      holdDuration = max(holdDuration, MIN_HOLD);
      break;
    case INDEX_INTERVAL:
      keyInterval -= getIncrementStep(keyInterval - 1);
      keyInterval = max(keyInterval, MIN_HOLD);
      break;
    }
    break;
  }

  if (button != BUTTON_NONE)
  {
    refreshDisplay(button == BUTTON_SELECT);
  }
  if (isTyping)
  {
    doType(button != BUTTON_NONE);
  }
}

int getIncrementStep(int value)
{
  int step;
  if (value < 100)
  {
    step = 1;
  }
  else if (value < 300)
  {
    step = 10;
  }
  else
  {
    step = 20;
  }
  return step;
}

void refreshDisplay(bool init)
{
  char output[17];
  unsigned char selectedItem[3];

  if (init)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Time Hold Intval");
  }

  for (int i = 0; i < NUMBER_OF_MENU_ITEMS; i++)
  {
    if (selectionIndex == i)
    {
      selectedItem[i] = '>';
    }
    else
    {
      selectedItem[i] = ' ';
    }
  }
  sprintf(output, "%c%3d %c%3d %c%5d", selectedItem[INDEX_KEYS], cycleTime, selectedItem[INDEX_HOLD], holdDuration, selectedItem[INDEX_INTERVAL], keyInterval);
  lcd.setCursor(0, 1);
  lcd.print(output);
}

// read the buttons
byte readButton()
{
  byte button = BUTTON_NONE;

  int adc = 0;
  adc = analogRead(0); // read the value from the sensor

  // The analog readout will become close to these values when a button is pressed
  //   Right:    0
  //   Up:     144
  //   Down:   329
  //   Left:   504
  //   Select: 741
  if (adc < 50)
  {
    button = BUTTON_RIGHT;
  }
  else if (adc < 250)
  {
    button = BUTTON_UP;
  }
  else if (adc < 450)
  {
    button = BUTTON_DOWN;
  }
  else if (adc < 650)
  {
    button = BUTTON_LEFT;
  }
  else if (adc < 850)
  {
    button = BUTTON_SELECT;
  }

  return button;
}
