#include "neopixel.h"

SYSTEM_THREAD(ENABLED);

const int pixelCount = 147;
const int breatheRate = 10UL;
const char* cityLocation = "Princeton";  //City for my Photon
const char* stateLocation = "NJ";     // State for my Photon
const int totalEffects = 4;

enum ClockState{
  CLOCK_MODE = 0,
  NIGHT_MODE,
  TEST_MODE,
  WATERFALL_MODE,
  DARK_MODE
};

enum WeatherState{
  SUNSHINE = 0,
  SNOW,
  PARTLY_CLOUDY,
  LIGHTNING,
  OVERCAST,
  RAIN
};

struct deviceTime{
  uint8_t Hour;
  uint8_t Minute;
};

struct colorObject{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

deviceTime sunrise = { 6, 0};
deviceTime sunset  = {18,00};

ClockState currentState = CLOCK_MODE;
ClockState lastState = DARK_MODE;

WeatherState weather_State = PARTLY_CLOUDY;

const colorObject briteWhite = {255,255,255};
const colorObject skyBlue = {50,50,255};
const colorObject allRed = {0,255,0};
const colorObject allBlue = {0,0,255};
const colorObject yellowSun = {255,255,30};
const colorObject greenGrass = {255,0,0};
const colorObject dullGray = {125,125,125};

colorObject timeColor = briteWhite;
colorObject mailColor = briteWhite;
colorObject weatherColor = allBlue;

Adafruit_NeoPixel myPixels = Adafruit_NeoPixel(pixelCount, D2, WS2812B);

const uint8_t word_IT[8] = {0,1};
const uint8_t word_IS[8] = {3,4};
const uint8_t word_FIVE[8] = {26,27,28,29};
const uint8_t word_TEN[8] = {10,11,12};
const uint8_t word_QUARTER[8] = {19,20,21,22,23,24,25};
const uint8_t word_TWENTY[8] = {13,14,15,16,17,18};
const uint8_t word_HALF[8] = {6,7,8,9};
const uint8_t word_MINUTES[8] = {30,31,32,33,34,35,36};
const uint8_t word_PAST[8] = {48,49,50,51};
const uint8_t word_TILL[8] = {37,38};

const uint8_t word_HOUR[13][8] = {
  { NULL, NULL, NULL},       //ZERO NOT USED
  { 44, 45, 46},             //ONE
  { 52, 53, 54},             //TWO
  { 39, 40, 41, 42, 43},     //THREE
  { 56, 57, 58, 59},         //FOUR
  { 61, 62, 63, 64},         //FIVE
  { 75, 76, 77},             //SIX
  { 70, 71, 72, 73, 74},     //SEVEN
  { 65, 66, 67, 68, 69},     //EIGHT
  { 78, 79, 80, 81},         //NINE
  { 82, 83, 84},             //TEN
  { 85, 86, 87, 88, 89, 90}, //ELEVEN
  { 98, 99,100,101,102,103}, //TWELVE
};

const uint8_t word_OCLOCK[8] = { 91, 92, 93, 94, 95, 96};
const uint8_t word_MESSAGE[8] = {97};

const uint8_t tempCelcius = 104;
const uint8_t tempFahrenheit = 116;
const uint8_t tempNegative = 105;
const uint8_t digit[10] = {115,106,107,108,109,110,111,112,113,114};

const uint8_t pixelRow[9][16] = {
  {0 , 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12, NULL},//
  {13,14,15,16,17,18,19,20,21,22,23,24,25, NULL},//
  {26,27,28,29,30,31,32,33,34,35,36,37,38, NULL},//
  {39,40,41,42,43,44,45,46,47,48,49,50,51, NULL},//
  {52,53,54,55,56,57,58,59,60,61,62,63,64, NULL},//
  {65,66,67,68,69,70,71,72,73,74,75,76,77, NULL},//
  {78,79,80,81,82,83,84,85,86,87,88,89,90, NULL},//
  {91,92,93,94,95,96,97,98,99,100,101,102,103, NULL},//
  {104,105,106,107,108,109,110,111,112,113,114,115,116, NULL}//
};

const uint8_t sunshine_Weather[] = {142,143,144,145,146};
const uint8_t partlyCloudy_Weather[] = {137,138,139,140,141};
const uint8_t overcast_Weather[] = {132,133,134,135,136};
const uint8_t lightning_Weather[] = {127,128,129,130,131};
const uint8_t rain_Weather[] = {122,123,124,125,126};
const uint8_t snow_Weather[] = {117,118,119,120, 121};

uint8_t lastMinute = 61U;
bool isMessage = false;

int emailCount = 0;
int lastMailCount = 999;

char responseTopic[125];
char publishString[125];

uint32_t timerToggle= 1;
uint32_t currentWeatherUpdateTime = 0;
uint32_t gmailTime = 0;
char weatherCondition[125] = "Not Yet Reported";
char currentTime[125];
int extTemp = 199;
int celciusTemp = 199;

bool startup = true;
bool isDay;
bool lastTimerCheck;
char daytimeStatus[12] = "Night Time";

void(*displayEffect[totalEffects])();

void setup()
{
  displayEffect[0] = waterFallEffect;
  displayEffect[1] = testPanel;
  displayEffect[2] = liteRandomLeds;
  displayEffect[3] = liteEveryLedRandomly;
  sprintf(publishString, "{\"my-city\": \"%s\", \"my-state\": \"%s\" }", cityLocation, stateLocation);
  int timeZoneOffset;
  EEPROM.get(0, timeZoneOffset);
  Time.zone(timeZoneOffset);
  Particle.variable("EmailCount", &emailCount, INT);
  Particle.variable("extTemp", &extTemp, INT);
  //Particle.variable("celcius", &celciusTemp , INT);
  Particle.variable("weather", weatherCondition, STRING);
  Particle.variable("time", currentTime, STRING);
  Particle.variable("dayOrNight", daytimeStatus, STRING);
  Particle.function("mode", setClockMode);
  Particle.function("power", togglePower);
  Particle.function("weather", testWeather);
  String deviceID = System.deviceID();
  deviceID.toCharArray(responseTopic,125);
  Particle.subscribe(responseTopic, webhookHandler, MY_DEVICES);
  myPixels.begin();
  myPixels.setBrightness(255);
  myPixels.clear();
  myPixels.show();
  delay(10000);
  char pushoverMessage[60];
  sprintf(pushoverMessage, "WordClock Restart at %d:%02d%s", Time.hourFormat12(), Time.minute(), Time.isAM()? "am" : "pm");
  Particle.publish("pushover", pushoverMessage, 60, PRIVATE);
  Particle.publish("current_weather", publishString, 60, PRIVATE);
}

void loop()
{
  isDay = timerEvaluate(sunrise.Hour, sunrise.Minute, sunset.Hour, sunset.Minute, Time.minute(), Time.hour(), 0);
  if (isDay != lastTimerCheck)
  {
    if(isDay)
    {
      sprintf(daytimeStatus, "Day Time");
      Particle.publish("pushover", "The sun is now up!", 60, PRIVATE);
    }
    else
    {
      sprintf(daytimeStatus, "Night Time");
      Particle.publish("pushover", "The sun has just set!", 60, PRIVATE);
    }
  }
  lastTimerCheck = isDay;
  sprintf(currentTime, "Current Time: %d:%02d%s, Sunrise: %d:%02d, Sunset: %d:%02d", Time.hourFormat12(), Time.minute(), Time.isAM()? "am" : "pm", sunrise.Hour, sunrise.Minute, sunset.Hour, sunset.Minute);
  if (millis() - gmailTime > 29 * 999UL)
  {
    Particle.publish("browerjames_gmail", publishString, 60, PRIVATE);
    gmailTime = millis();
  }
  if (millis() - currentWeatherUpdateTime > (10 * 60 * 1000UL) || startup)
  {
    startup = false;
    if (timerToggle > 1)
    {
      timerToggle = 0;
    }
    if (timerToggle == 0)
    {
      Particle.publish("current_weather", publishString, 60, PRIVATE);
    }
    else if (timerToggle == 1)
    {
      Particle.publish("sun_time", publishString, 60, PRIVATE);
    }
    currentWeatherUpdateTime = millis();
    timerToggle++;
  }
  switch (currentState){
    case (CLOCK_MODE):
      if (currentState != lastState)
      {
        myPixels.clear();
        updateTimePanel(Time.hour(), Time.minute());
      }
      if (emailCount != lastMailCount)
      {
        if (emailCount == 0)
        {
          mailColor = {0,0,0};
          isMessage = false;
          myPixels.setPixelColor(97, myPixels.Color(0, 0, 0));
          myPixels.show();
        }
        else
        {
          if (emailCount < 5)
          {
            mailColor = greenGrass;
          }
          else if (emailCount < 10)
          {
            mailColor = yellowSun;
          }
          else
          {
            mailColor = allRed;
          }
          isMessage = true;
          myPixels.setPixelColor(97, myPixels.Color(mailColor.red, mailColor.green, mailColor.blue));
          myPixels.show();
        }
      }
      {
        switch (weather_State){
          case (SUNSHINE):
            breatheWeather(sunshine_Weather, sizeof(sunshine_Weather), breatheRate, 1);
            break;
          case (SNOW):
            breatheWeather(snow_Weather, sizeof(snow_Weather), breatheRate, 1);
            break;
          case (PARTLY_CLOUDY):
            breatheWeather(partlyCloudy_Weather, sizeof(partlyCloudy_Weather), breatheRate, 1);
            break;
          case (OVERCAST):
            breatheWeather(overcast_Weather, sizeof(overcast_Weather), breatheRate, 1);
            break;
          case (LIGHTNING):
            breatheWeather(lightning_Weather, sizeof(lightning_Weather), breatheRate, 1);
            break;
          case (RAIN):
            breatheWeather(rain_Weather, sizeof(rain_Weather), breatheRate, 1);
            break;
        }
        displayTemp(extTemp, celciusTemp, 125UL, false);
        int currentTime = Time.now();
        int currentMinute = Time.minute(currentTime);
        int currentSecond = Time.second(currentTime);
        if (currentMinute != lastMinute)
        {
          uint8_t currentHour = Time.hourFormat12(currentTime);
          updateTimePanel(currentHour, currentMinute);
        }
        lastMinute = currentMinute;
      }
      break;
    case (NIGHT_MODE):
      if (currentState != lastState)
      {
        myPixels.clear();
        myPixels.show();
      }
      break;
    case (DARK_MODE):
      if (currentState != lastState)
      liteEveryLedRandomly();
      break;
    case (TEST_MODE):
      testPanel();
      break;
    case (WATERFALL_MODE):
      waterFallEffect();
      break;
  }
  lastState = currentState;
  lastMailCount = emailCount;
}

void updateTimePanel(uint8_t theHour, uint8_t theMinute)
{
  static uint8_t priorMinute = 61;
  if (theMinute != priorMinute && (priorMinute == 59 || priorMinute == 14 || priorMinute == 29 || priorMinute == 44))
  {
    uint8_t skitChoice = random(0, 4);
    displayEffect[skitChoice]();
  }
  priorMinute = theMinute;
  clearPixels();
  liteTheWord(word_IT);
  liteTheWord(word_IS);
  if (theMinute < 33)
  {
    liteTheWord(word_HOUR[theHour]);
  }
  else
  {
    if (theHour < 12)
    {
      theHour++;
    }
    else
    {
      theHour = 1;
    }
    liteTheWord(word_HOUR[theHour]);
  }
  if (theMinute < 3)
  {
    liteTheWord(word_OCLOCK);
  }
  else if (theMinute < 8)
  {
    liteTheWord(word_FIVE);
    liteTheWord(word_PAST);
  }
  else if (theMinute < 13)
  {
    liteTheWord(word_TEN);
    liteTheWord(word_MINUTES);
    liteTheWord(word_PAST);
  }
  else if (theMinute < 18)
  {
    liteTheWord(word_QUARTER);
    liteTheWord(word_PAST);
  }
  else if (theMinute < 23)
  {
    liteTheWord(word_TWENTY);
    liteTheWord(word_MINUTES);
    liteTheWord(word_PAST);
  }
  else if (theMinute < 28)
  {
    liteTheWord(word_TWENTY);
    liteTheWord(word_FIVE);
    liteTheWord(word_PAST);
  }
  else if (theMinute < 33)
  {
    liteTheWord(word_HALF);
    liteTheWord(word_PAST);
  }
  else if (theMinute < 38)
  {
    liteTheWord(word_TWENTY);
    liteTheWord(word_FIVE);
    liteTheWord(word_TILL);
  }
  else if (theMinute < 43)
  {
    liteTheWord(word_TWENTY);
    liteTheWord(word_TILL);
  }
  else if (theMinute < 48)
  {
    liteTheWord(word_QUARTER);
    liteTheWord(word_TILL);
  }
  else if (theMinute < 53)
  {
    liteTheWord(word_TEN);
    liteTheWord(word_TILL);
  }
  else if (theMinute < 58)
  {
    liteTheWord(word_FIVE);
    liteTheWord(word_TILL);
  }
  else
  {
    liteTheWord(word_OCLOCK);
  }
  myPixels.setPixelColor(97, myPixels.Color(mailColor.red, mailColor.green, mailColor.blue));
  myPixels.show();
}

void clearPixels()
{
    for(uint8_t i = 0; i < pixelCount; i++)
    {
      myPixels.setPixelColor(i, myPixels.Color(0, 0, 0));
    }
}

void liteTheWord(const uint8_t * word)
{
    for(uint8_t i = 0; i < sizeof(word)*2; i++)
    {
      if(isDay)
      {
        timeColor = briteWhite;
      }
      else
      {
        timeColor = dullGray;
      }
      myPixels.setPixelColor(word[i], myPixels.Color(timeColor.red, timeColor.green, timeColor.blue));
    }
}

void breatheWeather(const uint8_t * segment, const uint8_t numLeds, const uint32_t increment, const uint8_t step)
{
  static uint32_t lastTimeChange = 0;
  static uint8_t direction = 1;
  const static uint8_t lowLimit = 50;
  static uint8_t value = lowLimit;
  if(millis() - lastTimeChange > increment)
  {
    value +=(direction * step);
    value = constrain(value, lowLimit, 255);
    if (value <= lowLimit || value >= 255)
    {
      direction *= -1;
    }
    for(uint8_t i = 0; i < numLeds; i++)
    {
      myPixels.setPixelColor(segment[i], myPixels.Color(map(value, 0,255,0,weatherColor.red), map(value, 0,255,0,weatherColor.green), map(value, 0,255,0,weatherColor.blue)));
    }
    myPixels.show();
    lastTimeChange += increment;
  }
}

void displayTemp(const int32_t temp, const int32_t cTemp, uint32_t displayTime, bool celcius)
{
  static uint32_t lastTimeChange = 0;
  colorObject thermocolor;
  if (temp < 41)
  {
    thermocolor = {75,75,255};
  }
  else if (temp > 88)
  {
    thermocolor = {0,255,0};
  }
  else
  {
    thermocolor = {255,255,255};
  }
  uint8_t hundreds = abs(temp % 1000) / 100;
  uint8_t tens = abs(temp % 100) / 10;
  uint8_t ones = abs(temp % 10);
  uint8_t cHundreds = abs(cTemp % 1000) / 100;
  uint8_t cTens = abs(cTemp % 100) / 10;
  uint8_t cOnes = abs(cTemp % 10);
  if(millis() - lastTimeChange < displayTime )
  {
    for(uint8_t i = 104; i <= 116; i++)
    {
      myPixels.setPixelColor(i, myPixels.Color(0, 0, 0));
    }
    if (hundreds > 0)
    {
      myPixels.setPixelColor(digit[hundreds], myPixels.Color(thermocolor.red, thermocolor.green, thermocolor.blue));
    }
    else if (temp < 0)
    {
      myPixels.setPixelColor(tempNegative, myPixels.Color(thermocolor.red, thermocolor.green, thermocolor.blue));
    }
    myPixels.show();
  }
  else if(millis() - lastTimeChange < displayTime * 3 && abs(temp) > 9)
  {
    for(uint8_t i = 104; i <= 116; i++)
    {
      myPixels.setPixelColor(i, myPixels.Color(0, 0, 0));
    }
    myPixels.show();
  }
  else if(millis() - lastTimeChange < displayTime * 4 && abs(temp) > 9)
  {
    for(uint8_t i = 104; i <= 116; i++)
    {
      myPixels.setPixelColor(i, myPixels.Color(0, 0, 0));
    }
    myPixels.setPixelColor(digit[tens], myPixels.Color(thermocolor.red, thermocolor.green, thermocolor.blue));
    myPixels.show();
  }
  else if(millis() - lastTimeChange < displayTime * 5)
  {
    for(uint8_t i = 104; i <= 116; i++)
    {
      myPixels.setPixelColor(i, myPixels.Color(0, 0, 0));
    }
    myPixels.show();
  }
  else if(millis() - lastTimeChange < displayTime * 6)
  {
    for(uint8_t i = 104; i <= 116; i++)
    {
      myPixels.setPixelColor(i, myPixels.Color(0, 0, 0));
    }
    myPixels.setPixelColor(digit[ones], myPixels.Color(thermocolor.red, thermocolor.green, thermocolor.blue));
    myPixels.show();
  }
  else if(millis() - lastTimeChange < displayTime * 7)
  {
    for(uint8_t i = 104; i <= 116; i++)
    {
      myPixels.setPixelColor(i, myPixels.Color(0, 0, 0));
    }
    myPixels.show();
  }
  else if(millis() - lastTimeChange < displayTime * 8)
  {
    for(uint8_t i = 104; i <= 116; i++)
    {
      myPixels.setPixelColor(i, myPixels.Color(0, 0, 0));
    }
    myPixels.setPixelColor(tempFahrenheit, myPixels.Color(thermocolor.red, thermocolor.green, thermocolor.blue));
    myPixels.show();
  }
  else if(millis() - lastTimeChange < displayTime * 9)
  {
    for(uint8_t i = 104; i <= 116; i++)
    {
      myPixels.setPixelColor(i, myPixels.Color(0, 0, 0));
    }
    myPixels.show();
  }
  else if(millis() - lastTimeChange > 10000UL)
  {
    lastTimeChange = millis();
  }
}

void webhookHandler(const char *event, const char *data)
{
  if (strstr(event, "browerjames_gmail/0"))
  {
    gotGmailFeed(event, data);
  }
  else if(strstr(event, "current_weather"))
  {
    gotWeather(event, data);
  }
  else if (strstr(event, "sun_time"))
  {
    gotSunTime(event, data);
  }
}

void gotGmailFeed(const char *event, const char *data)
{
  String emailReturn = extractString(String(data), "<fullcount>", "</fullcount>");
  if (emailReturn != NULL)
  {
    emailCount = emailReturn.toInt(); //
  }
}

void gotWeather(const char *event, const char *data)  //  Clear~74.4~86%~NNW~0~70~  <<<<<<< return prototype
{
  char dataBuffer[125] = "";
  sprintf(dataBuffer, "%s", data);
  char* ptr = strtok(dataBuffer, "\"~");
  sprintf(weatherCondition, "%s", ptr);
  int i = 0;
  while(weatherCondition[i])
  {
    weatherCondition[i] = tolower(weatherCondition[i]);
    i++;
  }
  extTemp = atoi(strtok(NULL, "~"));
  celciusTemp = int((double(extTemp - 32) * (5.0/9.0)));
  if (strstr(weatherCondition, "snow") || strstr(weatherCondition, "ice") || strstr(weatherCondition, "freezing") || strstr(weatherCondition, "sleet"))
  {
    weatherColor = allBlue;
    weather_State = SNOW;
  }
  else if (strstr(weatherCondition, "drizzle") || strstr(weatherCondition, "rain"))
  {
    weatherColor = allBlue;
    weather_State = RAIN;
  }
  else if (strstr(weatherCondition, "hail") || strstr(weatherCondition, "thunderstorm"))
  {
    weatherColor = allBlue;
    weather_State = LIGHTNING;
  }
  else if (strstr(weatherCondition, "overcast"))
  {
    weatherColor = dullGray;
    weather_State = OVERCAST;
  }
  else if (strstr(weatherCondition, "sun") || strstr(weatherCondition, "clear"))
  {
    weatherColor = yellowSun;
    weather_State = SUNSHINE;
  }
  else
  {
    weatherColor = allBlue;
    weather_State = PARTLY_CLOUDY;
  }
}

void gotSunTime(const char * event, const char * data)
{
  deviceTime currentTime;
  String sunriseReturn = String(data);
  char sunriseBuffer[25] = "";
  sunriseReturn.toCharArray(sunriseBuffer, 25);
  sunrise.Hour = atoi(strtok(sunriseBuffer, "\"~"));
  sunrise.Minute = atoi(strtok(NULL, "~"));
  sunset.Hour = atoi(strtok(NULL, "~"));
  sunset.Minute = atoi(strtok(NULL, "~"));
  currentTime.Hour = atoi(strtok(NULL, "~"));
  currentTime.Minute = atoi(strtok(NULL, "~"));
  Time.zone(0);
  int currentOffset = getUtcOffset(Time.hour(), currentTime.Hour);
  Time.zone(currentOffset);
  int savedOffset;
  EEPROM.get(0, savedOffset);
  if (currentOffset != savedOffset)
  {
    EEPROM.put(0, currentOffset);
  }
}

int getUtcOffset(int utcHour, int localHour)  // sorry Baker Island, this won't work for you (UTC-12)
{
  if (utcHour == localHour)
  {
    return 0;
  }
  else if (utcHour > localHour)
  {
    if (utcHour - localHour >= 12)
    {
      return 24 - utcHour + localHour;
    }
    else
    {
      return localHour - utcHour;
    }
  }
  else
  {
    if (localHour - utcHour > 12)
    {
      return  localHour - 24 - utcHour;
    }
    else
    {
      return localHour - utcHour;
    }
  }
}

String extractString(String myString, const char* start, const char* end)
{
  if (myString == NULL)
  {
    return NULL;
  }
  int index = myString.indexOf(start);
  if (index < 0) {
    return NULL;
  }
  int endIdx = myString.indexOf(end);
  if (endIdx < 0) {
    return NULL;
  }
  return myString.substring(index + strlen(start), endIdx);
}

void testPanel()
{
  for(int j = 0; j < 3; j++)
  {
    for (int i = 0; i < pixelCount; i++)
    {
      if (i == 0)
      {
        myPixels.setPixelColor(pixelCount-1, myPixels.Color(0, 0, 0));
      }
      myPixels.setPixelColor(i-1, myPixels.Color(0, 0, 0));
      myPixels.setPixelColor(i, myPixels.Color(j % 3 == 0? 255: 0, j % 3 == 1? 255: 0,  j % 3 == 2? 255: 0));
      myPixels.show();
      Particle.process();
      delay(10);
    }
  }
}

void waterFallEffect()
{
  for (uint8_t pass = 0; pass < 3; pass++)
  {
    for (uint8_t row = 0; row < 9; row++)
    {
      for(uint8_t pixel = 0; pixel < 16; pixel++)
      {
        if (pixelRow[row][pixel] == NULL && (row != 0 && pixel != 0)) break;
        myPixels.setPixelColor(pixelRow[row][pixel], myPixels.Color(pass %3 == 0 ? 255 : 0, pass % 3 == 1 ? 255 : 0, pass % 3 == 2 ? 255 : 0));
      }
      myPixels.show();
      delay(50);
      for (uint8_t k = 0; k <= 116; k++)
      {
        myPixels.setPixelColor(k, myPixels.Color(0, 0, 0));
      }
    }
    for (uint8_t row = 0; row < 9; row++)
    {
      for(uint8_t pixel = 0; pixel < 16; pixel++)
      {
        if (pixelRow[row][pixel] == NULL /*&& (9 - row != 0 && pixel != 0)*/) break;
        myPixels.setPixelColor(pixelRow[9 - row][pixel], myPixels.Color(pass %3 == 0 ? 255 : 0, pass % 3 == 1 ? 255 : 0, pass % 3 == 2 ? 255 : 0));
      }
      myPixels.show();
      Particle.process();
      delay(50);
      for (uint8_t k = 0; k <= 116; k++)
      {
        myPixels.setPixelColor(k, myPixels.Color(0, 0, 0));
      }
    }
  }
}

void liteRandomLeds()
{
  for(int j = 0; j < 100; j++)
  {
    clearPixels();
    uint8_t color = random(0,3);
    myPixels.setPixelColor(random(0,pixelCount), myPixels.Color(color % 3 == 0? 255: 0, color % 3 == 1? 255: 0,  color % 3 == 2? 255: 0));
    myPixels.show();
    Particle.process();
    delay(20);
  }
}

void liteEveryLedRandomly()
{
  uint8_t randomArray[pixelCount];
  for (uint8_t i = 0; i < pixelCount; i++)
  {
    randomArray[i] = i;
  }
  for (uint8_t i = 0; i < pixelCount; i++)
  {
    randomSeed(analogRead(A0));
    uint8_t location = random(i, pixelCount);
    uint8_t value = randomArray[location];
    randomArray[location] = randomArray[i];
    randomArray[i] = value;
  }
  for (uint8_t i = 0; i < 3; i++)
  {
    for (uint8_t j = 0; j < pixelCount; j++)
    {
      myPixels.clear();
      uint8_t color = random(0,3);
      myPixels.setPixelColor(randomArray[j], myPixels.Color(color == 0? 255: 0, color == 1? 255: 0,  color == 2? 255: 0));
      myPixels.show();
      Particle.process();
      delay(25);
    }
  }
}

int setClockMode(String command)
{
  char myCommand[125] = "";
  command.toCharArray(myCommand,125);
  int i = 0;
  while(command[i])
  {
    command[i] = tolower(command[i]);
    i++;
  }
  if (strstr(command, "clock"))
  {
    currentState = CLOCK_MODE;
  }
  else if (strstr(command, "night"))
  {
    currentState = NIGHT_MODE;
  }
  else if (strstr(command, "test"))
  {
    currentState = TEST_MODE;
  }
  else if (strstr(command, "waterfall"))
  {
    currentState = WATERFALL_MODE;
  }
  else if (strstr(command, "dark"))
  {
    currentState = DARK_MODE;
  }
  return 1;
}

int togglePower(String command)
{
  int value = command.toInt();
  if (value == 0)
  {
    currentState = DARK_MODE;
  }
  else if (value == 1)
  {
    currentState = CLOCK_MODE;
  }
  else if (value == 2)
  {
    currentState = currentState == CLOCK_MODE? DARK_MODE : CLOCK_MODE;
  }
  Particle.publish("sun_time", publishString, 60, PRIVATE);
  Particle.publish("current_weather", publishString, 60, PRIVATE);
  char pushoverMessage[60] = "";
  sprintf(pushoverMessage, "WordClock is now %s", (currentState == CLOCK_MODE? "ON" : "OFF"));
  Particle.publish("pushover", pushoverMessage, 60, PRIVATE); // Spark WebHook!!!
  return currentState == CLOCK_MODE? 1: 0;
}

int testWeather(String command)
{
  char myCommand[125] = "";
  command.toCharArray(myCommand,125);
  int i = 0;
  while(command[i])
  {
    command[i] = tolower(command[i]);
    i++;
  }
  if (strstr(myCommand, "sunshine"))
  {
    weather_State = SUNSHINE;
    return 0;
  }
  else if (strstr(myCommand,"snow"))
  {
    weather_State = SNOW;
    return 0;
  }
  else if (strstr(myCommand, "partly"))
  {
    weather_State = PARTLY_CLOUDY;
    return 0;
  }
  else if (strstr(myCommand, "lightning"))
  {
    weather_State = LIGHTNING;
    return 0;
  }
  else if (strstr(myCommand, "overcast"))
  {
    weather_State = OVERCAST;
    return 0;
  }
  else
  {
    weather_State = RAIN;
    return 1;
  }
}

bool timerEvaluate(int start_Hour, int start_Minute, int end_Hour, int end_Minute, int now_Minute, int now_hour, int now_second)
{
  int start_time = tmConvert_t(Time.year(), Time.month(), Time.day(), start_Hour, start_Minute, 0);
  int end_time = tmConvert_t(Time.year(), Time.month(), Time.day(), end_Hour, end_Minute, 0);
  int now_time = tmConvert_t(Time.year(), Time.month(), Time.day(), now_hour, now_Minute, now_second);
  //
  if (start_time < end_time)
  {
    return (now_time > start_time && now_time < end_time);
  }
  else if (end_time < start_time)
  {
    return (now_time < end_time || now_time > start_time);
  }
  else
  {
    return false;
  }
}

time_t tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss)
{
  struct tm t;
  t.tm_year = YYYY-1900;
  t.tm_mon = MM - 1;
  t.tm_mday = DD;
  t.tm_hour = hh;
  t.tm_min = mm;
  t.tm_sec = ss;
  t.tm_isdst = 0;
  time_t t_of_day = mktime(&t);
  return t_of_day;
}
