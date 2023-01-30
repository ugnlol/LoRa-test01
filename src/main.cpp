#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <string>
#include <iostream>
#include <LoRa.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define NUMFLAKES     10 // Number of snowflakes in the animation example
#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

#define LoRA_SCK   5
#define LoRA_MISO 19
#define LoRA_MOSI 27
#define LoRA_CS   18
#define LoRA_RST  14
#define LoRA_IRQ  26
#define LED_pin    2
#define LoRaFrequency 868E6   // (433Mhz) use 433E6, 868E6 or 915E6 but depends on your module's chipset frequency 
#define diagnosticsMode true // use 'true' for full diadnostics information like signal strength (RSSI), signal-to-noise ratio (SNR), etc

String deviceID    = "Transponder-1";

byte   msgCount      = 0;     // count of outgoing messages
byte   localAddress  = 0xFF;  // address of this device, you casn change this to maske it filter out multiple transponders, 0xFF is a broadcast address
byte   remoteAddress = 0xFF;  // destination to send to, 0xFF is broadcast, so all transponders receive the message
long   lastSendTime  = 0;     // last send time value
int    interval      = 5000;  // interval between link resync in milli-seconds
int    transpondRate = 1000;  // rate of transponding, currently sets the LED flash timing!
int    counter       = 0;     // a simple message sent counter
bool   respond       = true;  // to respond or not

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void printToDisplay(char *text ){
  //maks 10 bokstaver pr linje
  display.clearDisplay();

  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner

  display.print(text);
  display.println();

  display.display();
}

void printToDisplay(int number){
  //maks 10 bokstaver pr linje
  display.clearDisplay();

  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner

  display.print(number);
  display.println();

  display.display();
}

void appendToDisplay(char *text ){
  display.print(text);
  display.println();
  display.display();
}

void appendToDisplay(float number ){
  display.print(number);
  display.println();
  display.display();
}


void blinkLED(byte pin, int blinkdelay) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);              // turn the LED off
  delay(blinkdelay / 2);               // wait
  digitalWrite(pin, HIGH);             // turn the LED on (HIGH is on)
  delay(blinkdelay / 2);               // wait
  digitalWrite(pin, LOW);              // turn the LED off
}


void sendMessage(String outgoing) {
  LoRa.beginPacket();                    // start packet
  LoRa.write(remoteAddress);             // add destination address
  LoRa.write(localAddress);              // add sender address
  LoRa.write(msgCount);                  // add message ID
  LoRa.write(outgoing.length());         // add payload length
  LoRa.print(outgoing);                  // add payload
  LoRa.endPacket();                      // finish packet and send it
  msgCount++;                            // increment message ID
  blinkLED(LED_pin, transpondRate/10);   // qucik flash of on-board LED to denote sending message
}


void onReceive(int packetSize) {
  if (packetSize == 0) return;           // if no data payload received, return
  respond = true;
  lastSendTime = millis();
  // read packet header bytes:
  int recipient       = LoRa.read();     // get recipient address
  byte sender         = LoRa.read();     // get sender address
  byte incomingMsgId  = LoRa.read();     // get incoming msg ID
  byte incomingLength = LoRa.read();     // get incoming msg length
  String incoming = "";
  while (LoRa.available()) {
    incoming += (char)LoRa.read();       // get incoming message
  }
  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                              // skip rest of function
  }
  // if the recipient isn't (this device or a broadcast)
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                              // skip rest of function
  }
  // if message is for this device, or a broadcast, print details:
  blinkLED(LED_pin, transpondRate);      // slow flash on-board LED to denote received message
  if (diagnosticsMode) Serial.println("Received from : 0x" + String(sender, HEX));
  if (diagnosticsMode) Serial.println("Sent to       : 0x" + String(recipient, HEX));
  if (diagnosticsMode) Serial.println("Message ID    : " + String(incomingMsgId));
  if (diagnosticsMode) Serial.println("Message length: " + String(incomingLength));
  Serial.println("Received from : " + incoming);
  Serial.println();
  if (diagnosticsMode) Serial.println("RSSI          : " + String(LoRa.packetRssi()));
  if (diagnosticsMode) Serial.println("Snr           : " + String(LoRa.packetSnr()));
  if (diagnosticsMode) Serial.println();
  if (diagnosticsMode)
  {
       // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
  }
    printToDisplay("RSSI: ");
    appendToDisplay(LoRa.packetRssi());
    appendToDisplay("SNR:");
    appendToDisplay(LoRa.packetSnr());

    // display.println("RSSI: " + String(LoRa.packetRssi()));
  }
}

String startLoRa() {
  SPI.begin(LoRA_SCK, LoRA_MISO, LoRA_MOSI, LoRA_CS);       // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(LoRA_CS, LoRA_RST, LoRA_IRQ);                // assign pins for LoRa module
  //LoRa.setSpreadingFactor(8);                             // ranges from 6-12,default 7 see API docs, this changes bandwidth used
  //LoRa.setSyncWord(0xF3);                                 // ranges from 0-0xFF, default 0x34, see API docs, enables devices only with the same word to be received
  if (!LoRa.begin(LoRaFrequency)) {                         // initialise radio on chosen frequency
    Serial.println("LoRa init failed. Check connections.");
    return "LoRa init failed...";                           // if failed, report it back
  }
  return "LoRa init succeeded.";                            // succeeded
}



void loop() {
 
  // delay(2000);
  // Serial.println("test2 test2 test2");

  //  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  // if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
  //   Serial.println(F("SSD1306 allocation failed"));
  //   for(;;); // Don't proceed, loop forever
  // }

  // char *tekstStreng = "hei hei hei!";
  // printToDisplay("hei1"); 
  // delay(500);
  // appendToDisplay("hei2"); 
  // delay(500);
  // appendToDisplay("hei3"); 
  // delay(500);
  // appendToDisplay("hei4"); 


  // delay(2000);

  if (respond) {
    String message = deviceID + " message: " + String(counter);   // form a message to send
    sendMessage(message);                                         // send the message
    Serial.println("Sending from  : " + message);
    counter++;
    respond = false;
  }

  // parse for a packet, and call onReceive with the result:
  if (millis() - lastSendTime > interval) { // Reset the link if it has gone out of sync or a missed response
    lastSendTime = millis();
    respond = true;
    delay(random(3)*1000); // Wait a random time to prevent transponder transmission collions
  }
  onReceive(LoRa.parsePacket());


}


void setup() {
  Serial.begin(9600);
  delay(1000);
  
  while (!Serial);
  Serial.println("LoRa Transponder");
  Serial.println(startLoRa());
 
}










