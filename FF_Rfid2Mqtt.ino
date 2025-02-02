#define RFID_2_MQTT_VERSION "24.12.1-1"                             // Version of this code

#include <ESP8266WiFi.h>                                            // WiFi (embedded)
#include <Arduino.h>                                                // Arduino (embedded)
#include <SPI.h>                                                    // SPI (embeded)
#include <MFRC522.h>                                                // MFRC522 module https://github.com/miguelbalboa/rfid.git
#include <AsyncMqttClient.h>                                        // Asynchronous MQTT client https://github.com/marvinroger/async-mqtt-client
#include <FF_Rfid2MqttParameters.h>                                 // Constants for this program

//  *** WiFi stuff ***
WiFiEventHandler onStationModeConnectedHandler;                     // Event handler called when WiFi is connected
WiFiEventHandler onStationModeDisconnectedHandler;                  // Event handler called when WiFi is disconnected
WiFiEventHandler onStationModeGotIPHandler;                         // Event handler called when WiFi got an IP
IPAddress ipAddress;                                                // My IP address
static char nodeName[sizeof(rootNodeName)+12+2];                    // Define this (full) node name

//  *** Asynchronous MQTT client ***
AsyncMqttClient mqttClient;                                         // Asynchronous MQTT client
bool mqttStatusReceived = false;                                    // True if we received initial status at startup
uint8_t mqttDisconnectedCount = 0;                                  // Count of successive disconnected status
#define MAX_MQTT_DISCONNECTED 15                                    // Restart ESP if more than this number of disconnected count has bee seen
static char mqttLastWillTopic[sizeof(mqttRootLastWillTopic)+12+2];
static char mqttUidTopic[sizeof(mqttRootUidTopic)+12+2];
static char mqttSetTopic[sizeof(mqttRootSetTopic)+12+2];

//  *** RFID-RC522 module ***
MFRC522 rfid(SS_PIN, RST_PIN);
unsigned long rfidLastTime = 0;                                     // Last time rfid loop ran

//  *** Syslog ***
#ifdef SYSLOG_USED
    #include <WiFiUdp.h>
    #include <Syslog.h>
    WiFiUDP udpClient;
    Syslog syslog(udpClient, SYSLOG_PROTO_IETF);
#endif

//  *** LED ***
#include <FF_LED.h>

enum ledSignalType {
    BLINKS_WAIT_BADGE = 1,                                          // Waiting for badge
    BLINKS_BADGE_INVALID,                                           // Badge is NOT recognized
    BLINKS_BADGE_VALID,                                             // Badge is recognized
    BLINKS_WAIT_FOR_SERVER,                                         // Waiting for server answer
    BLINKS_NO_MQTT,                                                 // MQTT not connected
    BLINKS_NO_WIFI                                                  // No Wifi state
};

#define BLINKS_MIN (BLINKS_WAIT_BADGE)                                 // Min value
#define BLINKS_MAX (BLINKS_BADGE_VALID)                             // Max value

FF_LED led(LED_PIN, LED_REVERTED);
unsigned long ledSetTime = 0;                                       // Last time resetTimer was set
unsigned long ledResetTime = 0;                                     // Time to wait before resetting LED
ledSignalType ledSavedSignal = BLINKS_NO_WIFI;                      // Saved LED signal

//      ### Functions ###

//  ** RFID ***

//  RFID setup
void rfidSetup(void) {
    SPI.begin();                                                    // Init SPI bus
    rfid.PCD_Init();                                                // Init MFRC522
}

//  RFID loop
void rfidLoop(void) {
    if ((millis() - rfidLastTime) > 100) {
        rfidLastTime = millis();
        if (rfid.PICC_IsNewCardPresent()) {                         // new tag is available
            if (rfid.PICC_ReadCardSerial()) {                       // NUID has been readed
                char uid[33];
                char *ptr = uid;
                for (int i = 0; i < rfid.uid.size; i++) {
                    if (i) {
                        *ptr++ = ':';
                    }
                    snprintf(ptr, 3, "%02x", rfid.uid.uidByte[i]);
                    ptr += 2;
                }
                signal(PSTR("Card UID: %s"), uid);

                setLed(BLINKS_WAIT_FOR_SERVER);                     // Wait for server answer
                uint16_t result = mqttClient.publish(mqttUidTopic, 0, false, uid);  // Publish message
                if (!result) {
                    signal(PSTR("Publish %s to %s returned %d"), uid, mqttUidTopic, result);
                }

                rfid.PICC_HaltA();                                  // halt PICC
                rfid.PCD_StopCrypto1();                             // stop encryption on PCD
            }
        }
    }
}

//  *** Syslog ***

#ifdef SYSLOG_USED
    void syslogSetup(void) {
        //Initialize syslog server
        syslog.server(syslogServer, syslogPort);
        Serial.printf(PSTR("Syslog host %s:%d, this host %s\n"), syslogServer, syslogPort, nodeName);
        syslog.deviceHostname(nodeName);
        syslog.defaultPriority(LOG_USER + LOG_DEBUG);
    }
#endif

//  *** Message output ***

//  Signal something to the user
//      Write message on Serial and syslog
//      Input:
//          _format: Format of string to be displayed (printf format)
//          ...: relevant parameters
static void signal(const char* _format, ...) {
    // Make the message
    char msg[256];                                                  // Message buffer (message truncated if longer than this)
    va_list arguments;                                              // Variable argument list
    va_start(arguments, _format);                                   // Read arguments after _format
    vsnprintf_P(msg, sizeof(msg), _format, arguments);              // Return buffer containing requested format with given arguments
    va_end(arguments);                                              // End of argument list
    Serial.println(msg);                                            // Print on Serial
    #ifdef SYSLOG_USED
        syslog.log(msg);
    #endif
}

//  *** Wifi stuff ***

// WiFi setup
void wiFiSetup() {
    onStationModeConnectedHandler = WiFi.onStationModeConnected(&onWiFiConnected); // Declare connection callback
    onStationModeDisconnectedHandler = WiFi.onStationModeDisconnected(&onWiFiDisconnected); // Declare disconnection callback
    onStationModeGotIPHandler = WiFi.onStationModeGotIP(&onWiFiGotIp); // Declare got IP callback

    signal(PSTR("Connecting to WiFi..."));
    WiFi.setAutoReconnect(true);                                    // Set auto reconnect
    WiFi.persistent(true);                                          // Save WiFi parameters into flash
    WiFi.hostname(nodeName);                                        // Defines this module name
    WiFi.mode(WIFI_STA);                                            // We want station mode (connect to an existing SSID)
    Serial.printf(PSTR("SSID: %s, key: %s\n"), wifiSSID, wifiKey);
    WiFi.begin(wifiSSID, wifiKey);                                  // SSID to connect to
}

// Executed when WiFi is connected
static void onWiFiConnected(WiFiEventStationModeConnected data) {
    signal(PSTR("Connected to WiFi"));
}

// Executed when WiFi is disconnected
static void onWiFiDisconnected(WiFiEventStationModeDisconnected data) {
    signal(PSTR("Disconnected from WiFi"));
    setLed(BLINKS_NO_WIFI);                                         // We don't have WiFi
}

// Executed when IP address has been given
static void onWiFiGotIp(WiFiEventStationModeGotIP data) {
    ipAddress = WiFi.localIP();
    signal(PSTR("Got IP address %d.%d.%d.%d (%s)"), ipAddress[0], ipAddress[1], ipAddress[2], ipAddress[3], nodeName);
    mqttConnect();                                                  // Connect to MQTT
}

//  *** Asynchronous MQTT client ***

// MQTT setup
void mqttSetup() {
    mqttClient.setServer(mqttServer, mqttPort);                     // Set server IP (or name), and port
    mqttClient.setClientId(nodeName);                               // Set client id (= nodeName)
    mqttClient.setCredentials(mqttUsername, mqttPassword);          // Set MQTT user and password
    mqttClient.onConnect(&onMqttConnect);                           // On connect (when MQTT is connected) callback
    mqttClient.onDisconnect(&onMqttDisconnect);                     // On disconnect (when MQTT is disconnected) callback
    mqttClient.onMessage(&onMqttMessage);                           // On message (when subscribed item is received) callback
    mqttClient.setWill(mqttLastWillTopic, 1, true,                  // Last will topic
        "{\"state\":\"down\"}");

}

// MQTT loop
void mqttLoop(void) {
}

// Establish connection with MQTT server
static void mqttConnect() {
    if (!mqttClient.connected()) {                                  // If not yet connected
        setLed(BLINKS_NO_MQTT);                                     // No connected to MQTT
        mqttDisconnectedCount++;                                    // Increment disconnected count
        if (mqttDisconnectedCount > MAX_MQTT_DISCONNECTED) {
            signal(PSTR("Restarting after %d MQTT disconnected events\n"), mqttDisconnectedCount);
            delay(1000);                                            // Give time to mesage to be displayed and sent to syslog
            ESP.restart();                                          // Restart ESP
        }
        signal(PSTR("Connecting to MQTT..."));
        mqttClient.connect();                                       // Start connection
    }
}

//Executed when MQTT is connected
static void onMqttConnect(bool sessionPresent) {
    signal(PSTR("MQTT connected"));
    uint16_t result = mqttClient.publish(mqttLastWillTopic, 0, true,// Last will topic
        "{\"state\":\"up\",\"version\":\"" RFID_2_MQTT_VERSION "\"}");
    if (!result) {
        signal(PSTR("Publish to %s returned %d\n"), mqttLastWillTopic, result);
    }
    mqttClient.subscribe(mqttSetTopic, 0);                          // Subscribe to set topic
    setLed(BLINKS_WAIT_BADGE);                                      // Waiting for badge
}

//Executed when MQTT is disconnected
static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    signal(PSTR("MQTT disconnected"));
    if (ledSavedSignal != BLINKS_NO_WIFI) {
        setLed(BLINKS_NO_MQTT);                                     // Waiting for badge
    }
}

// Executed when a subscribed message is received
//  Input:
//      topic: topic of received message
//      payload: content of message (WARNING: without ending numm character, use len)
//      properties: MQTT properties associated with thos message
//      len: payload length
//      index: index of this message (for long messages)
//      total: total message count (for long messages)
static void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    char message[len+1];                                            // Allocate size for message plus null ending character
    strncpy(message, payload, len);                                 // Copy message on given len
    message[len] = 0;                                               // Add zero at end
    int value = atoi(message);
    signal(PSTR("Received: %s, retain:%d"), message, properties.retain);
    if (!properties.retain) {
        if (value >= BLINKS_MIN and value <= BLINKS_MAX) {
            setLed((ledSignalType) value);
        } else {
            signal(PSTR("Illegal value received: %d"), value);
        }
    }
}

//  *** LED ***

//  Set LED display  mode
void setLed(ledSignalType ledValue){
    switch (ledValue) {
        case BLINKS_NO_WIFI:
            led.setBlink(255, 50, 50, 0);
            ledSavedSignal = ledValue;
            break;
        case BLINKS_NO_MQTT:
            led.setBlink(3, 50, 50, 1000);
            ledSavedSignal = ledValue;
            break;
        case BLINKS_WAIT_FOR_SERVER:
            led.setBlink(1, 500, 500, 0);
            ledResetTime = 5000;
            ledSetTime = millis();
            break;
        case BLINKS_WAIT_BADGE:
            led.setPulse(true, 3, 3, 2000, 0, 200);
            ledSavedSignal = ledValue;
            break;
        case BLINKS_BADGE_INVALID:
            led.setBlink(1, 50, 50, 0);
            ledResetTime = 2000;
            ledSetTime = millis();
            break;
        case BLINKS_BADGE_VALID:
            ledSavedSignal = ledValue;
            led.setPulse(false, 3, 3, 5000);
            break;
    }
}

//  LED Loop
void ledLoop(void) {
    if (ledResetTime) {                                             // Are we in LED reset mode?
        if ((millis() - ledSetTime) > ledResetTime) {               // Is reset time expired?
            ledResetTime = 0;                                       // Clear reset
            setLed(ledSavedSignal);                                 // Set last saved mode
        }
    }
    led.loop();
}

//  *** Setup routine ***
void setup() {
    // Load some strings personalized with this chip ID.
    snprintf(nodeName, sizeof(nodeName), "%s-%06x", rootNodeName, ESP.getChipId());
    snprintf(mqttLastWillTopic, sizeof(mqttLastWillTopic), "%s/%06x", mqttRootLastWillTopic, ESP.getChipId());
    snprintf(mqttUidTopic, sizeof(mqttUidTopic), "%s/%06x", mqttRootUidTopic, ESP.getChipId());
    snprintf(mqttSetTopic, sizeof(mqttSetTopic), "%s/%06x", mqttRootSetTopic, ESP.getChipId());

    led.begin();                                                    // LED setup
    setLed(BLINKS_NO_WIFI);                                         // We don't have WiFi
    // Start consle
    Serial.begin(74880);
    Serial.println();
    Serial.printf("Starting %s V%s\n", nodeName, RFID_2_MQTT_VERSION);
    #ifdef SYSLOG_USED
        syslogSetup();                                              // Syslog setup
    #endif
    mqttSetup();                                                    // MQTT setup
    wiFiSetup();                                                    // WiFi setup
    rfidSetup();                                                    // Card reader setup
    signal(PSTR("Started"));
}

//  *** Main routine ***
void loop() {
    ledLoop();
    rfidLoop();
    mqttLoop();
}