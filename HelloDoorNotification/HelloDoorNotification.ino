/*
 *  [TODO] Documentation
 *
 *  *   Source sketches
 *  *   Operation outline
 */

#include <pins_arduino.h>
#include <WiFi.h>
#include <Arduino.h>
#include <heltec.h>

#include "credentials.h"

static const uint16_t port = 80;
static const char *const host = "maker.ifttt.com";

static unsigned long lastDebounceTime = 0;
static unsigned long debounceDelay = 100;

RTC_DATA_ATTR int buttonState;
static int newButtonState;

void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

static void setupWifi()
{

  WiFi.begin(ssid, pass);

  Serial.println();
  Serial.println();
  Serial.print("Waiting for WiFi... ");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(500);
}

static void sendButtonStatus(const int buttonStatus)
{
  Serial.print("Connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;

  if (!client.connect(host, port))
  {
    Serial.println("Connection failed.");
    Serial.println("Waiting 5 seconds before retrying...");
    delay(5000);
    return;
  }

  const char *const status = buttonStatus ? "Unlocked" : "Locked";

  String jsonObject = String("{\"value1\":\"") + status + "\"}";

  client.println(String("POST ") + resource + " HTTP/1.1");
  client.println(String("Host: ") + host);
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonObject.length());
  client.println();
  client.println(jsonObject);

  int timeout = 5 * 10; // 5 seconds
  while (!!!client.available() && (timeout-- > 0))
  {
    delay(100);
  }
  if (!!!client.available())
  {
    Serial.println("No response...");
  }
  while (client.available())
  {
    Serial.write(client.read());
  }

  Serial.println("\nclosing connection");
  client.stop();
}

static void printText(const String text)
{
  Heltec.display->clear();

  Heltec.display->drawString(0, 0, text);
  Heltec.display->display();
}

static void oledSetup()
{
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void setup()
{
  oledSetup();

  //Serial.begin(115200);
  //delay(100);

  printText("Booting");
  setupWifi();

  print_wakeup_reason();

  pinMode(KEY_BUILTIN, INPUT);
  newButtonState = digitalRead(KEY_BUILTIN);

  Serial.println(String("Button state: ") + buttonState);
  Serial.println(String("New button state: ") + newButtonState);

  printText(String("Button states (old/new): ") + String(buttonState) + " " + String(newButtonState));
  delay(1000);

  buttonState = newButtonState;

  sendButtonStatus(buttonState);
  printText("Status sent, sleeping");

  delay(1000);

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, !buttonState);

  //Go to sleep now
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void loop()
{
}
