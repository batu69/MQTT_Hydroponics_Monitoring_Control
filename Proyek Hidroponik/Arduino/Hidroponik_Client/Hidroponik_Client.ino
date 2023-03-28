#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define pompa 7
#define pengaduk 6
#define DHTPIN 4
#define DHTTYPE DHT11

DHT_Unified dht(DHTPIN, DHTTYPE);
RF24 radio(8, 9);
float sensorDHT;
unsigned long previousMillis = 0;
uint8_t address[][6] = {"1Node", "2Node"};
bool radioNumber = 1;
bool role = false;
float payload = 0.0;

struct SensorData {
  float sensorSuhu;
  float sensorKelembaban;
  float sensorTDS;
  float k1;
} Sensor;

void setup() {
  pinMode(pompa, OUTPUT);
  pinMode(pengaduk, OUTPUT);
  digitalWrite(pompa, HIGH);
  digitalWrite(pengaduk, HIGH);
  Serial.begin(115200);

  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}
  }
  
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(sizeof(Sensor));
  radio.openWritingPipe(address[radioNumber]);
  radio.openReadingPipe(1, address[!radioNumber]);
  
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
}

void bacaDHT(){
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Suhu Udara        : "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    Sensor.sensorSuhu = event.temperature;
    
  }
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Kelembaban Udara  : "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    Sensor.sensorKelembaban = event.relative_humidity;
  }
}
void bacaTDS(){
  int tdsMentah = analogRead(A0);
  Sensor.sensorTDS = tdsMentah;
  Serial.print(F("Kadar Nutrisi Air : "));
  Serial.print(Sensor.sensorTDS);
  Serial.println(F(" PPM"));
}
void kirim(){
  radio.stopListening();
  unsigned long start_timer = micros();
  bool report = radio.write(&Sensor, sizeof(Sensor));
  unsigned long end_timer = micros();
  
  if (report) {
    Serial.print(F("Transmission successful! "));
    Serial.print(F("Time to transmit = "));
    Serial.print((end_timer - start_timer)/1000);
    Serial.println(F(" ms."));

  }
  else {
    Serial.println(F("Transmission failed or timed out"));
  }
}

void actuator(float perintah){
  Serial.println(perintah);
  if (perintah == 1.00){
    digitalWrite(pompa, HIGH);
    Serial.println("Nyala");
  }
  else if (perintah == 1.10){
    digitalWrite(pompa, LOW);
    Serial.println("Mati");
  }
  else if (perintah == 2.00){
    digitalWrite(pengaduk, HIGH);
    Serial.println("Mati");
  }
  else if (perintah == 2.10){
    digitalWrite(pengaduk, LOW);
    Serial.println("Mati");
  }
}

void terima(){
  radio.startListening();
  uint8_t pipe;
  if (radio.available(&pipe)){
    uint8_t bytes = radio.getPayloadSize();
    radio.read(&payload, bytes);
    Serial.print(F("Received "));
    Serial.print(bytes);
    Serial.print(F(" bytes on pipe "));
    Serial.print(pipe);
    Serial.print(F(": "));
    Serial.println(payload);
    actuator(payload);
  }
}

void loop() {
  terima();
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= 2000) {
    previousMillis = currentMillis;
    bacaDHT();
    bacaTDS();
    kirim();
  }
}
