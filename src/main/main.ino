#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "secrets.h"
#include <ESP8266WiFi.h>
#include <RCSwitch.h>

/************ Constants ************/

#define LED 2

/************ Global State ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
// You should absolutely use the WiFiClientSecure here if your
// board supports it.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and
// login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME,
                          AIO_USERNAME, AIO_KEY);

RCSwitch sender = RCSwitch();

/****************************** Feeds ***************************************/

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Subscribe zap =
    Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/zap");

void setup() {
  pinMode(LED, OUTPUT);

  Serial.begin(115200);

  // Turn LED off until MQTT is connected.
  digitalWrite(LED, HIGH);

  // Check what the pin mapping for your board is.
  // For mine this is D0.
  sender.enableTransmit(16);
  sender.setProtocol(1);
  sender.setPulseLength(187);

  // Connect to WiFi access point.
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  mqtt.subscribe(&zap);
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the
  // MQTT_connect function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &zap) {
      Serial.print(F("Zap: "));
      const char *val = (char *)zap.lastread;
      Serial.println(val);

      sender.sendTriState(val);
    }
  }

  // ping the server to keep the mqtt connection alive
  if (!mqtt.ping()) {
    mqtt.disconnect();
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  // connect will return 0 for connected
  while ((ret = mqtt.connect()) != 0) {
    digitalWrite(LED, HIGH);
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000); // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1)
        ;
    }
  }
  digitalWrite(LED, LOW);
  Serial.println("MQTT Connected!");
}
