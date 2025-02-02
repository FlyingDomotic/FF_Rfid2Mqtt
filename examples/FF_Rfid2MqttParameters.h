// Here are settings you have to change to adapt module to your specific needs
// Voici les paramètres que vous devez changer pour adapter le module à vos besoins

//  *** Options ***
#define SS_PIN  D8                                                  // ESP8266 pin where SS is conencted to
                                                                    // Pinoche de l'ESP8266 où la pinoche SS du lecteur est connectée
#define RST_PIN D2                                                  // ESP8266 pin where RST is connected to
                                                                    // Pinoche de l'ESP8266 où la pinoche RST du lecteur est connectée
#define LED_PIN D4                                                  // Pin where LED is connected to
                                                                    // Pinoche ou la LED est connectée
#define LED_REVERTED true                                           // Is LED reverted (ON at LOW level)?
                                                                    // La LED est-elle inversée (allumée à l'état LOW (bas)) ?
//  *** Wifi stuff ***
//  *** Connection WiFi ***
char wifiSSID[] = "MySSID";                                         // SSID to connected to
                                                                    // SSID auquel se connecter
char wifiKey[] = "MyPassword";                                      // SSID password
                                                                    // Mot de passe associé au SSID
char rootNodeName[] = "Rfid2Mqtt";                                  // Root ESP node name
                                                                    // Racine du nom du module ESP

//  *** MQTT client ***
//  *** Client MQTT ***
char mqttServer[] = "192.168.1.1";                                  // MQTT server IP address or name
                                                                    // Adresse IP ou nom du serveur MQTT
uint16_t mqttPort = 1883;                                           // MQTT IP port
                                                                    // Port IP du serveur MQTT
char mqttUsername[] = "MyMqttUsername";                             // MQTT username
                                                                    // Utilisateur MQTT
char mqttPassword[] = "MyMqttPassword";                             // MQTT password
                                                                    // Mot de passe MQTT
char mqttRootUidTopic[] = "Rfid2Mqtt/uid";                          // MQTT UID topic, where id are sent to
                                                                    // Sujet MQTT où les id lus sont envoyés
char mqttRootSetTopic[] = "Rfid2Mqtt/set";                          // MQTT set topic, where led state is read
                                                                    // Sujet MQTT où les id lus sont envoyés
char mqttRootLastWillTopic[] = "Rfid2Mqtt/LWT";                     // MQTT Last Will Topic, where MQTT status is written
                                                                    // Sujet MQTT où le statut MQTT est écrit
//  *** Syslog client ***
//  *** Client Syslog ***

#define SYSLOG_USED                                                 // Comment this line if you don't want to use syslog
                                                                    // Commentez la ligne si vous ne voulez pas utiliser syslog
#ifdef SYSLOG_USED
    char syslogServer[] = "192.168.1.1";                            // Syslog server IP address or name
                                                                    // Adresse IP ou nom du serveur Syslog
    uint16_t syslogPort = 514;                                      // Syslog IP (UDP) port
                                                                    // Port IP du serveur Syslog
#endif