/*
 * NFC read and send to 
 * MQTT server application for Redbear Duo
 */

#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include "MQTT.h"

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);

#define END_CHAR '"'
#define NFC_WRITE 0
#define NFC_READ 1

int nfcmode = NFC_READ;
String inputString = "";
String writeString = "";
boolean stringComplete = false;

// wifi network setting
char ssid[] = "XXXXX";
char password[] = "XXXXX";
void printWifiStatus();

#define COORDINATE_ENDPOINT "coordinate/"
void callback(char* topic, byte* payload, unsigned int length);
MQTT client("xxxxxx", 1883, callback);
char lastmsg[256];

void callback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;
    Serial.print("MQTT Packet:");
    Serial.print(topic);
    Serial.print("\t");
    Serial.println(p);
}

void setup(void) {
    Serial.begin(115200);
    Serial.println("NDEF Reader");
    memset(lastmsg, 0, sizeof(lastmsg));

    // connect to WiFi network
    WiFi.on();
    WiFi.setCredentials(ssid,password);
    WiFi.connect();

    while ( WiFi.connecting()) {
      // print dots while we wait to connect
      Serial.print(".");
      delay(300);
    }

    IPAddress localIP = WiFi.localIP();
    while (localIP[0] == 0) {
      localIP = WiFi.localIP();
      Serial.println("waiting for an IP address");
      delay(1000);
    }
    printWifiStatus();

    // connect to MQTT servers
    client.connect("sparkclient");
    if (client.isConnected()) {
        client.publish("outTopic/message","hello world");
    }

    nfc.begin();
}

void loop(void) {
  if (stringComplete) {
    Serial.println(inputString);
    if (inputString.startsWith("r")) {
      nfcmode = NFC_READ;
      Serial.println("NFC_MODE : READ");
    } else if (inputString.startsWith("w")) {
      nfcmode = NFC_WRITE;
      Serial.println("NFC_MODE : WRITE");
    } else {
      writeString = inputString;
      Serial.println("Write Data : " + writeString);
    }

    // clear the string:
    inputString = "";
    stringComplete = false;
  }

  if (nfcmode == NFC_READ) {
    if (nfc.tagPresent()) {
        NfcTag tag = nfc.read();
        if(tag.hasNdefMessage()) {
            NdefMessage message = tag.getNdefMessage();

            for(int i = 0; i < message.getRecordCount(); i++) {
                NdefRecord record = message.getRecord(i);
                int payloadLength = record.getPayloadLength();
                byte payload[payloadLength];

                record.getPayload(payload);

                if (payloadLength > 0) {
                    char msg[payloadLength + 1]; memset(msg, 0, sizeof(msg));
                    int charIdx = 0;
                    for (int j = 0; j < payloadLength; j++) {
                        if (payload[j] != 0) {
                            msg[charIdx++] = payload[j];
                        }
                    }

                    Serial.print(msg);
                    Serial.println();

                    if (strncmp(lastmsg, msg, charIdx) != 0) {
                        // Particle.publish(COORDINATE_ENDPOINT, msg);
                        client.publish(COORDINATE_ENDPOINT, msg);
                        sprintf(lastmsg, "%s", msg);
                    }
                }
           }
        }
    }
    delay(50);
    
  } else if (nfcmode == NFC_WRITE) {
    Serial.println("\nPlace a formatted Mifare Classic or Ultralight NFC tag on the reader.");
    if (nfc.tagPresent()) {
        NdefMessage message = NdefMessage();
        message.addUriRecord(writeString);

        bool success = nfc.write(message);
        if (success) {
          Serial.println("Success. Try reading this tag with your phone.");       
        } else {
          Serial.println("Write failed.");
        }
    }
    delay(5000);
  }

  if (client.isConnected())
    client.loop();
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == END_CHAR)
      stringComplete = true;
    else
      inputString += inChar;
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
