#include <WiFi.h>
#include <PubSubClient.h>

#define RX_PIN 16
#define TX_PIN 17
#define UART_BAUDRATE 115200

const char* ssid = "southern";
const char* password = "praha666";
const char* mqtt_server = "192.168.1.3";

uint8_t temperature = 0, humidity = 0, light = 0, thresholdsValue = 0, manualMode = 0, i = 0, bitmask1, bitmask2, bitmask3, bitmask4;
uint8_t stmToEsp[2], espToStm[2];
char tempString[3], humString[3], lightString[3];

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
void callback(char* topic, byte* message, uint16_t length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  for (i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  if (String(topic) == "esp32/ledOutput")
  {
    if (messageTemp == "on")
    {
      espToStm[0] = 1; //1 - LED state, 2 - thresholds
      espToStm[1] = 1;
      Serial2.write(espToStm, 2);
    }
    else if (messageTemp == "off")
    {
      espToStm[0] = 1; //command type: 1 - LED state
      espToStm[1] = 0;
      Serial2.write(espToStm, 2);
    }
  }
  if (String(topic) == "esp32/thresholdsOutput")
  {
    thresholdsValue = atoi(messageTemp.c_str());
    espToStm[0] = 2; //command type: 2 - thresholds
    espToStm[1] = thresholdsValue;
    Serial2.write(espToStm, 2);
  }
  if (String(topic) == "esp32/manualMode")
  {
    manualMode = atoi(messageTemp.c_str());
    espToStm[0] = 3; //command type: 3 - manual mode
    espToStm[1] = manualMode;
    Serial2.write(espToStm, 2);
  }
  if (String(topic) == "esp32/bitMask1")
  {
    bitmask1 = atoi(messageTemp.c_str());
    espToStm[0] = 4; // command type: 4 - bitmask 1
    espToStm[1] = bitmask1;
    Serial2.write(espToStm, 2);
  }
  if (String(topic) == "esp32/bitMask2")
  {
    bitmask2 = atoi(messageTemp.c_str());
    espToStm[0] = 5; // command type: 5 - bitmask 2
    espToStm[1] = bitmask2;
    Serial2.write(espToStm, 2);
  }
  if (String(topic) == "esp32/bitMask3")
  {
    bitmask3 = atoi(messageTemp.c_str());
    espToStm[0] = 6; // command type: 6 - bitmask 3
    espToStm[1] = bitmask3;
    Serial2.write(espToStm, 2);
  }
  if (String(topic) == "esp32/bitMask4")
  {
    bitmask4 = atoi(messageTemp.c_str());
    espToStm[0] = 7; // command type: 7 - bitmask 4
    espToStm[1] = bitmask4;
    Serial2.write(espToStm, 2);
  }
}
void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client"))
    {
      Serial.println("connected");
      client.subscribe("esp32/ledOutput");
      client.subscribe("esp32/thresholdsOutput");
      client.subscribe("esp32/manualMode");
      client.subscribe("esp32/bitMask1");
      client.subscribe("esp32/bitMask2");
      client.subscribe("esp32/bitMask3");
      client.subscribe("esp32/bitMask4");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(3000);
    }
  }
}
void setup()
{
  Serial.begin(UART_BAUDRATE);
  Serial2.begin(UART_BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}
void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  if (!client.loop())
  {
    client.connect("ESP32Client");
  }
  if (Serial2.available() >= 2)
  {
    Serial2.readBytes(stmToEsp, 2);
    if (stmToEsp[0] == 1)
    {
      temperature = stmToEsp[1];
      itoa(temperature, tempString, 10);
      Serial.print("Temperature: ");
      Serial.println(temperature);
      client.publish("esp32/temperature", tempString);
    }
    else if (stmToEsp[0] == 2)
    {
      humidity = stmToEsp[1];
      itoa(humidity, humString, 10);
      Serial.print("Humidity: ");
      Serial.println(humidity);
      client.publish("esp32/humidity", humString);
    }
    else if (stmToEsp[0] == 3)
    {
      light = stmToEsp[1];
      itoa(light, lightString, 10);
      Serial.print("Light: ");
      Serial.println(light);
      client.publish("esp32/light", lightString);
    }
  }
}
