#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#include "printf.h"
#include "RF24.h"

#define MSG_BUFFER_SIZE  (50)

const char* ssid = "DESKTOP-MFTHCPL 8991";
const char* password = "riacantik";
const char* mqtt_server = "broker.emqx.io";

LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient espClient;
PubSubClient client(espClient);
RF24 radio(2, 0);

unsigned long lastMsg = 0, pompaOn = 0, pengadukOn = 0, previousMillis = 0;;
char msg[MSG_BUFFER_SIZE];
int lamaPompa = 5000, lamaPengaduk = 5000, statusPompa = 0, statusPengaduk = 0;
uint8_t address[][6] = {"1Node", "2Node"};
bool radioNumber = 0, role = false, pompaSent = 0, pengadukSent = 0, lcdTampil = 0;
float message = 0.0, payload = 0.0, value = 0;
float suhu, kelembaban;
int tds;

struct SensorData {
  float sensorSuhu;
  float sensorKelembaban;
  float sensorTDS;
  float k1;
} Sensor;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Menghubungkan");
  lcd.setCursor(0, 1);
  lcd.print("Ke WiFi......");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Terhubung...");
  delay(1000);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println();

  if ((char)payload[0] == '1') {
    kirim(1.1);
    pompaOn = millis();
    statusPompa = 1;
  } else if ((char)payload[0] == '2') {
    kirim(1.0);
  } else if ((char)payload[0] == '3') {
    kirim(2.1);
    pengadukOn = millis();
    statusPengaduk = 1;
  } else if ((char)payload[0] == '4') {
    kirim(2.0);
  }
}

void reconnect() {
  while (!client.connected()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Menghubungkan");
    lcd.setCursor(0, 1);
    lcd.print("Ke Server MQTT");
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Terhubung...");
      client.publish("outTopic", "hello world");
      client.subscribe("hidroponik_lampu");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Gagal...");
      lcd.setCursor(0, 1);
      lcd.print("Mencoba kembali");
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Wire.begin(5, 4);
  lcd.begin(16,2);
  lcd.backlight();
  lcd.home();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}
  }
  
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(sizeof(Sensor));
  radio.openWritingPipe(address[radioNumber]);
  radio.openReadingPipe(1, address[!radioNumber]);
  radio.startListening();

}

void tampil(){
  if (lcdTampil == 0){
    lcd.setCursor(0, 0);
    lcd.print("Suhu: ");
    lcd.setCursor(6, 0);
    lcd.print(suhu);
    lcd.setCursor(11, 0);
    lcd.print("  C");
    lcd.setCursor(0, 1);
    lcd.print("PPM : ");
    lcd.setCursor(6, 1);
    lcd.print(tds);
    lcd.setCursor(12, 1);
    lcd.print(" PPM");
    lcdTampil = 1;
  }
  else if (lcdTampil == 1){
    lcd.setCursor(0, 0);
    lcd.print("Kelembaban :     ");
    lcd.setCursor(0, 1);
    lcd.print(kelembaban);
    lcd.setCursor(4, 1);
    lcd.print(" %                    ");
    lcdTampil = 0;
  }
}

void kirimKeServer(float dataSensor, char *topic){
  
  tampil();
  value = dataSensor;
  snprintf (msg, MSG_BUFFER_SIZE, "%.2f", value);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(topic, msg);
}

void kirim(float perintah){
  message = perintah;
  radio.stopListening();
  unsigned long start_timer = micros();
  bool report = radio.write(&message, sizeof(float));
  unsigned long end_timer = micros();
  
  if (report) {
    Serial.print(F("Transmission successful! "));
    Serial.print(F("Time to transmit = "));
    Serial.print(end_timer - start_timer);
    Serial.print(F(" us. Sent: "));
    Serial.println(message);
  }
  else {
    Serial.println(F("Transmission failed or timed out"));
  }
}


void terima(){
  radio.startListening();
  uint8_t pipe;
  if (radio.available(&pipe)){
    uint8_t bytes = radio.getPayloadSize();
    SensorData Sensor;
    radio.read(&Sensor, sizeof(Sensor));
    Serial.print(F("Received "));
    Serial.print(bytes);
    Serial.print(F(" bytes on pipe "));
    Serial.println(pipe);
    unsigned long currentMillis = millis();
    previousMillis = currentMillis;
    suhu = Sensor.sensorSuhu;
    kelembaban = Sensor.sensorKelembaban;
    tds = Sensor.sensorTDS;
    kirimKeServer(suhu, "hidroponik_suhu");
    kirimKeServer(kelembaban, "hidroponik_kelembaban");
    kirimKeServer(tds, "hidroponik_ppm");
  }
}

void cekPompa(){
  if (statusPompa == 1){
    if (millis() - pompaOn < lamaPompa){
      if (pompaSent == 0){
        kirim(1.1);
      }
      pompaSent = 1;
    }
    else{
      kirim(1.0);
      statusPompa = 0;
      pompaSent = 0;
      kirimKeServer(2, "pompa_status");
    }
  }
}

void cekPengaduk(){
  if (statusPengaduk == 1){
    if (millis() - pengadukOn < lamaPengaduk){
      if (pengadukSent == 0){
        kirim(2.1);
      }
      pengadukSent = 1;
    }
    else{
      kirim(2.0);
      statusPengaduk = 0;
      pengadukSent = 0;
      kirimKeServer(4, "pengaduk_status");
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  terima();
  cekPengaduk();
  cekPompa();
  if (Serial.available()) {
    char c = toupper(Serial.read());
    if (c == 'T' && !role) {
      kirim(1.0);
    } else if (c == 'R' && !role) {
      kirim(1.1);
      pompaOn = millis();
      statusPompa = 1;
    } else if (c == 'G' && !role) {
      kirim(2.0);
    } else if (c == 'F' && !role) {
      kirim(2.1);
      pengadukOn = millis();
      statusPengaduk = 1;
    }
  }
}
