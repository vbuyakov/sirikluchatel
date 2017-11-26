#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
extern "C" { // эта часть обязательна чтобы получить доступ к функции initVariant
#include "user_interface.h"
}

#define _DBG_
const char *ssid = "OranganWRT18";       // Имя WiFi
const char *password = "k316rv23";       // Пароль WiFi
const String self_token = "zzz";         // токен для минимальной безопасности связи
const String serv_token = "zzz";         // токен для минимальной безопасности связи
const String name = "IOT_lamp";          // имя выключателя, читай лампочки
const String serverIP = "192.168.0.231"; // внутренний IP WEB сервера
bool lamp_on = false;
bool can_toggle = false;
int button_state;

ESP8266WebServer server(80); // веб сервер
HTTPClient http;             // веб клиент

//const int lamp = 5; // Управляем реле через GPIO2
#define lamp 5
const int button = 10; // "Ловим" выключатель через GPIO0

// функция для пинга лампочки
void handleRoot()
{
  server.send(200, "text/plain", "Hello! I am " + name);
}

// функция для недействительных запросов
void handleNotFound()
{
  String message = "not found";
  server.send(404, "text/plain", message);
}

// Да будет свет
void turnOnLamp()
{
#ifdef _DBG_
  Serial.print("On\n");
#endif
  digitalWrite(lamp, LOW);
  lamp_on = true;
}

// Да будет тьма
void turnOffLamp()
{
#ifdef _DBG_
  Serial.print("Off\n");
#endif
  digitalWrite(lamp, HIGH);
  lamp_on = false;
}

// Отправляем серверу события ручного вкл./выкл.
void sendServer(bool state)
{
  //TODO: Какой то левый метод надо его выпилить
  http.begin("http://" + serverIP + "/iapi/setstate");
  String post = "token=" + self_token + "&state=" + (state ? "on" : "off"); // По токену сервер будет определять что это за устройство
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(post);
  http.end();
}

// Изменяем состояние лампы
void toggleLamp()
{
  if (lamp_on == true)
  {
    turnOffLamp();
    sendServer(false);
  }
  else
  {
    turnOnLamp();
    sendServer(true);
  }
}

// Получаем от сервера команду включить
void handleOn()
{
  String token = server.arg("token");
  if (serv_token != token)
  {
    String message = "access denied";
    server.send(401, "text/plain", message);
    return;
  }
  turnOnLamp();
  String message = "success";
  server.send(200, "text/plain", message);
}

// Получаем от сервера команду выключить
void handleOff()
{
  String token = server.arg("token");
  if (serv_token != token)
  {
    String message = "access denied";
    server.send(401, "text/plain", message);
    return;
  }
  turnOffLamp();
  String message = "success";
  server.send(200, "text/plain", message);
}

// Устанавливаем MAC чтобы давать одинаковый IP
void initVariant()
{
  uint8_t mac[6] = {0x00, 0xA3, 0xA0, 0x1C, 0x8C, 0x45};
  wifi_set_macaddr(STATION_IF, &mac[0]);
}

void setup(void)
{
  pinMode(lamp, OUTPUT);
  pinMode(button, INPUT_PULLUP); // Важно сделать INPUT_PULLUP
  turnOffLamp();
  WiFi.hostname(name);
  WiFi.begin(ssid, password);

#ifdef _DBG_
  Serial.begin(9600);
  Serial.println();
  Serial.printf("Connecting to %s ", ssid);
#endif
  // Ждем пока подключимся к WiFi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
#ifdef _DBG_
    Serial.print(".");
#endif
  }
#ifdef _DBG_
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif
  // Назначем функции на запросы
  server.on("/", handleRoot);
  server.on("/on", HTTP_GET, handleOn);
  server.on("/off", HTTP_GET, handleOff);
  server.onNotFound(handleNotFound);

  // Стартуем сервер
  server.begin();
}

void loop(void)
{
  server.handleClient();
  /*
  // Проверяем нажатие выключателя
  button_state = digitalRead(button);
  if (button_state == HIGH && can_toggle) {
    Serial.print("GO DOWN");
    toggleLamp();
    can_toggle = false;
    delay(500);
  } else if(button_state == LOW){
    Serial.print("GO UP");
    can_toggle = true;
  }*/
}
