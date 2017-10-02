#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "coap.h"
#include "DHT.h"


// RGB led pins
#define RPIN 3
#define GPIN 5
#define BPIN 6

#define DHTTYPE DHT11
#define DHTPIN 2
DHT dht(DHTPIN, DHTTYPE);

byte mac[] = { 0x90, 0xA2, 0xDA, 0x03, 0x00, 0xC3 };

// UDP and CoAP class
EthernetUDP Udp;
Coap coap(Udp);

int ledvalue=0;

void setPinColor(int red, int green, int blue){
  analogWrite(RPIN, red);
  analogWrite(GPIN, green);
  analogWrite(BPIN, blue);  
}

// CoAP server endpoint URL
// PUT/GET RGB led color
// coap-client -e "x" -m put coap://(address)/led
// coap-client -m get coap://(address)/led
void callback_led(CoapPacket &packet, IPAddress ip, int port){
  
  // extract payload
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  String message(p);

  if(packet.code==3){   
    Serial.println("Received PUT request for led color.");

    if (message.equals("0")){
      setPinColor(255, 0, 0);
      ledvalue = 0;
    }else if(message.equals("1")){
      setPinColor(0, 255, 0);
      ledvalue = 1;
    }else if(message.equals("2")){
      setPinColor(0, 0, 255);  
      ledvalue = 2;
    }
  }else if(packet.code==1){
    Serial.println("Received GET request for led color.");
  }

  /*
  int i;
  for(i=0; i < (int) packet.optionnum; i++){
    Serial.print("Option:");
    Serial.println(i);
    Serial.println(packet.options[i].number);  
    Serial.println(*(packet.options[i].buffer));  
  }*/

  if(ledvalue==0)
    coap.sendResponse(ip, port, packet.messageid, "Red");
  else if(ledvalue==1)
    coap.sendResponse(ip, port, packet.messageid, "Green");
  else if(ledvalue==2)
    coap.sendResponse(ip, port, packet.messageid, "Blue");
} 


// GET dht data
void callback_dht(CoapPacket &packet, IPAddress ip, int port){
  char response[200];
  char char_t, char_h;
  
  // extract payload
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  String message(p);

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    coap.sendResponse(ip, port, packet.messageid, "Error reading data from DHT11.");
    return;
  }
  
  Serial.println(t);
  Serial.println(h);
  //dtostrf(t, 2, 2, char_t);
  //dtostrf(h, 2, 0, char_h);
  
  sprintf(response, "Temp: %d °C, humidity: %d%%", (int) t, (int) h);

  /*
  int i;
  for(i=0; i < (int) packet.optionnum; i++){
    Serial.print("Option:");
    Serial.println(i);
    Serial.println(packet.options[i].number);  
    Serial.println(*(packet.options[i].buffer));  
  }*/

  if(packet.code==1){ 
    Serial.println("Received GET request for dht sensor.");
    coap.sendResponse(ip, port, packet.messageid, response);
  }
}

void callback_wn(CoapPacket &packet, IPAddress ip, int port){
  char *response;

  Serial.println("Received request for /.well-known/core resources");

  /*
  response = coap.uri.getUris();
  Serial.println(response);
  coap.sendResponse(ip, port, packet.messageid, response);*/

  coap.sendResponse(ip, port, packet.messageid, "</.well-known/core>, </led>, </dht11>");
}


void setup() {
  //IPAddress myip;
  Serial.begin(9600);

  IPAddress myip(192, 168, 1, 2);
  Ethernet.begin(mac, myip);
  //myip = Ethernet.localIP();
  
  Serial.print("My ip: ");  
  Serial.println(myip);  

  Serial.println("Starting DHT sensor...");
  dht.begin();

  pinMode(RPIN, OUTPUT);
  pinMode(GPIN, OUTPUT);
  pinMode(BPIN, OUTPUT);  
  setPinColor(255,0,0);

  Serial.println("Starting endpoints...");
  coap.server(callback_led, "led");         
  coap.server(callback_dht, "dht11");
  coap.server(callback_wn, ".well-known/core");

  coap.start();
  Serial.println("Ready.");
}

void loop() {
  coap.loop();
}
