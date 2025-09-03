#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Wifi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define LCD_RS 22
#define LCD_E 23
#define LCD_D4 14
#define LCD_D5 15
#define LCD_D6 16
#define LCD_D7 17

#define BUZZER 21

#define BUTTON 33

const char *ssid = "SSID";
const char *password = "password";

// NOTE : Set time zone here (https://timeapi.io/api/timezone/availabletimezones)
String timeApi = "https://timeapi.io/api/time/current/zone?timeZone=Europe%2FLondon";

// NOTE : Set lat, lon and API key here (https://home.openweathermap.org/api_keys)
const char *weatherApi = "https://api.openweathermap.org/data/2.5/weather?lat={lat}&lon={lon}&appid={API key}&units=metric";

struct Alarm {
  bool silenced;
  bool weekends;
  int hour, minute;
  String name;
};

enum Page {
  TIME,
  WEATHER,
  PAGE_COUNT,
};
 
std::vector<Alarm> alarms = {
  Alarm{ .hour = 7, .name = "WAKE UP WAKE UP WAKE UP" },
  Alarm{ .hour = 22, .minute = 30, .name = "Bedtime" }
};
Alarm *currentAlarm = nullptr;
Page page = TIME;

int year, month, day, hour, minute, seconds, milliseconds;
String dayOfWeek;

String weather;
String weatherDesc;
int tempMin, tempMax, tempFeelsLike, humidity;

void updateTime();
void displayTime();
void tick();
int daysInMonth();

void updateWeather();
void resetWeatherPage();
void displayWeather();

void resetDisplayAlarm();
void checkAlarms();
void displayAlarm();

LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

void setup() {
  Serial.begin(9600);
  Serial.println("Hello, world!");

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.cursor();
  String greeting = "Hello, world!";
  for (auto c : greeting) {
    lcd.print(c);
    delay(100);
  }
  lcd.setCursor(0, 1);
  lcd.print("Connecting...");
  lcd.blink();
  lcd.noCursor();

  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT);

  WiFi.begin(ssid, password);
  Serial.printf("Connecting to network %s", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  while (dayOfWeek == nullptr) {
    updateTime();
  }
  while (weather == nullptr) {
    updateWeather();
  }

  lcd.clear();
  lcd.noBlink();
}

int prevTime = 0;
int refreshRate = 3600000, refreshTimer = 0;
int currBtn, prevBtn;

void playRingtone();

void loop() {
  refreshTimer += millis() - prevTime;
  tick();

  if (refreshTimer >= refreshRate) {
    Serial.println("Refreshing data...");
    updateWeather();
    updateTime();
    Serial.println("Data refreshed");
    refreshTimer = 0;
  }
  
  prevBtn = currBtn;
  currBtn = digitalRead(BUTTON);

  if (currentAlarm == nullptr) {
    if (currBtn && !prevBtn) {
      page = static_cast<Page>((page + 1) % PAGE_COUNT);
      lcd.clear();
      resetWeatherPage();
    }

    checkAlarms();

    switch (page) {
    case TIME:
      displayTime();
      break;
    case WEATHER:
      displayWeather();
      break;
    }
  } else {
    displayAlarm();
    playRingtone();
    if (currBtn && !prevBtn) {
      currentAlarm->silenced = true;
      noTone(BUZZER);
      currentAlarm = nullptr;
    }
  }

  prevTime = millis();
}

void updateTime() {
  Serial.println("Fetching time");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }

  HTTPClient http;
  http.begin(timeApi);

  if (http.GET() <= 0) {
    Serial.println("Failed to update time: HTTP GET failed");
    return;
  }

  String payload = http.getString();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.println("Failed to update time: deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  year = doc["year"];
  month = doc["month"];
  day = doc["day"];
  hour = doc["hour"];
  minute = doc["minute"];
  seconds = doc["seconds"];
  milliseconds = doc["milliseconds"];
  dayOfWeek = doc["dayOfWeek"].as<String>();

  Serial.println("Successfully updated time");
}

bool colonVisible = true;
int lastBlink = 0, blinkInterval = 1000;

void displayTime() {
  if (millis() - lastBlink >= blinkInterval) {
    colonVisible = !colonVisible;
    lastBlink = millis();
    lcd.clear();
  }

  lcd.setCursor(0, 0);
  lcd.print(dayOfWeek);

  lcd.setCursor(0, 1);
  if (colonVisible)
    lcd.printf("%02d:%02d %02d/%02d/%d", hour, minute, day, month, year);
  else
    lcd.printf("%02d %02d %02d/%02d/%d", hour, minute, day, month, year);

  lcd.setCursor(16 - weather.length(), 0);
  lcd.print(weather);
}

int daysInMonth() {
  int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (month == 2) {
    int leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    return leap ? 29 : 28;
  }

  return days[month - 1];
}

int prevMillis;
void tick() {
  milliseconds += millis() - prevMillis;
  
  if (milliseconds >= 1000) {
    seconds++;
    milliseconds = 0;
  }
  if (seconds >= 60) {
    minute++;
    seconds = 0;
  }
  if (minute >= 60) {
    hour++;
    minute = 0;
  }
  if (hour >= 24) {
    day++;
    hour = 0;
  }
  if (day > daysInMonth()) {
    month++;
    day = 1;
  }
  if (month > 12) {
    year++;
    month = 1;
  }

  prevMillis = millis();
}

void updateWeather() {
  Serial.println("Fetching weather...");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to update weather: no connection");
    return;
  }

  HTTPClient http;
  http.begin(weatherApi);

  if (http.GET() <= 0) {
    Serial.println("Failed to update weather: HTTP GET failed");
    return;
  }

  String payload = http.getString();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.println("Failed to update time: deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  weather = doc["weather"][0]["main"].as<String>();
  weatherDesc = doc["weather"][0]["description"].as<String>();
  tempMin = doc["main"]["temp_min"];
  tempMax = doc["main"]["temp_max"];
  tempFeelsLike = doc["main"]["feels_like"];
  humidity = doc["main"]["humidity"];

  Serial.println("Successfully updated weather");
}

enum WeatherInfo {
  RANGE,
  FEELS_LIKE,
  HUMIDITY,
  WEATHERINFO_COUNT // helper value for cycling
};

WeatherInfo weatherPage = RANGE;
int lastWeatherPageUpdate = 0, weatherPageUpdateInterval = 2000;
int weatherScrollIndex = 0, lastWeatherScroll = 0, weatherScrollInterval = 300;

void resetWeatherPage() {
  weatherPage = RANGE;
  lastWeatherPageUpdate = 0;
  weatherScrollIndex = 0;
  lastWeatherScroll = 0;
}

void displayWeather() {
  if (millis() - lastWeatherPageUpdate >= weatherPageUpdateInterval) {
    lastWeatherPageUpdate = millis();

    // cycle to next enum value
    weatherPage = static_cast<WeatherInfo>((weatherPage + 1) % WEATHERINFO_COUNT);

    lcd.clear();
  }

  lcd.setCursor(0, 0);
  if (weatherDesc.length() <= 16) {
    lcd.print(weatherDesc);
  } else {
    String padded = weatherDesc + "   ";
    if (millis() - lastWeatherScroll >= weatherScrollInterval) {   
      char buf[17];
      for (int i = 0; i < 16; i++)
        buf[i] = padded[(weatherScrollIndex + i) % padded.length()]; // modulo to wrap back to start of string
      buf[16] = '\0';

      lcd.clear();
      lcd.print(buf);

      weatherScrollIndex = (weatherScrollIndex + 1) % padded.length();

      lastWeatherScroll = millis();
    }
  }

  lcd.setCursor(0, 1);

  switch (weatherPage) {
  case RANGE:
    lcd.printf("TempRange:%d-%dC", tempMin, tempMax);
    break;
  case FEELS_LIKE:
    lcd.printf("Feels like:  %dC", tempFeelsLike);
    break;
  case HUMIDITY:
    lcd.print("Humidity:");
    lcd.setCursor(16 - std::to_string(humidity).length() - 1, 1);
    lcd.printf("%d%%", humidity);
    break;
  }
}

#define D 294
#define DSHARP 311
#define E 330
#define G 392
#define A 440

struct Note {
  int freq;
  int duration; // ms
};

Note melody[] = {
  { D, 250 }, { DSHARP, 250 }, { E, 500 },
  { G, 250 }, { E, 250 }, { G, 250 }, { E, 250 },
  { E, 250 }, { DSHARP, 250 }, { D, 250 }, { A, 250 },
  { D, 250 }, { D, 500 }, { A, 250 },
};

const int melodyLength = sizeof(melody) / sizeof(Note);

int currentNote = 0;
unsigned long lastNote = 0;
bool playing = false;

void playRingtone() {
  if (!playing) {
    tone(BUZZER, melody[0].freq);   // no duration
    lastNote = millis();
    playing = true;
    currentNote = 0;
  }

  if (playing && (millis() - lastNote >= melody[currentNote].duration)) {
    currentNote++;
    if (currentNote < melodyLength) {
      tone(BUZZER, melody[currentNote].freq);  // play next note
      lastNote = millis();
    } else {
      noTone(BUZZER); // end of melody
      playing = false;
    }
  }
}

void checkAlarms() {
  for (auto &alarm : alarms) {
    if (alarm.silenced) {
      if (hour - alarm.hour >= 1)
        alarm.silenced = false;
      continue;
    }
    if (alarm.hour != hour || alarm.minute != minute)
      continue;
    currentAlarm = &alarm;
    resetDisplayAlarm();
  }
}

float lateSeconds;
int alarmScrollIndex = 0, lastAlarmScroll = 0, alarmScrollInterval = 300, lastAlarmTime = 0;

void resetDisplayAlarm() {
  Serial.println("Here");
  lateSeconds = 0;
  alarmScrollIndex = 0;
  lastAlarmScroll = 0;
  lastAlarmTime = millis();
}

void displayAlarm() {
  lateSeconds += (millis() - lastAlarmTime) / 1000.0;

  if (millis() - lastBlink >= blinkInterval) {
    colonVisible = !colonVisible;
    lastBlink = millis();
    lcd.clear();
  }

  lcd.setCursor(0, 0);
  if (currentAlarm->name.length() <= 16) {
    lcd.print(currentAlarm->name);
  } else {
    String padded = currentAlarm->name + "   ";
    if (millis() - lastAlarmScroll >= alarmScrollInterval) {
      char buf[17];

      for (int i = 0; i < 16; i++)
        buf[i] = padded[(i + alarmScrollIndex) % padded.length()];

      buf[16] = '\0';

      lcd.clear();
      lcd.print(buf);

      alarmScrollIndex++;
      lastAlarmScroll = millis();
    }
  }

  lcd.setCursor(0, 1);
  if (colonVisible)
    lcd.printf("%02d:%02d (%.0f)", hour, minute, lateSeconds);
  else
    lcd.printf("%02d %02d (%.0f)", hour, minute, lateSeconds);

  lastAlarmTime = millis();
}