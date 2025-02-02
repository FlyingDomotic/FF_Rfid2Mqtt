# Send RFID card ID to MQTT / Envoie l'identifiant d'une carte RFID à MQTT

[English version and French version in the same document]

Reads a RFID card with a RC522 proximity reader and send its ID to MQTT

[Versions françaises et anglaises dans le même document]

Lit une carte RFID avec un lecteur de proximité de type RC522 et envoie son identifiant à MQTT

## What's for?/A quoi ça sert ?

A possible use case (leading to this project) is to allow access to a 3D printer only to authorized people. We're doing that letting people swipping their (door entry) badge, send badge ID though MQTT to a system, which in turn will send back access authorization (and power printer if allowed).

Une utilisation possible (qui a conduit à ce projet) est d'autoriser l'accès à une imprimante 3D seulement aux personnes habilitées. Les utilisateurs passent leur badge (d'entrée) devant le lecteur, l'identifiant est envoyé à un système par MQTT, lequel système renvoie l'autorisation ou le refus (et met l'imprimante sous tension).

## Interfaces

We're using an ESP8266 (d1mini) and a RC522 RFID card reader. Connections are as follow (ESP8266 to RC522):
On utilise un ESP8266 (d1mini) et un lecteur de badge RFID RC522. Connexions sont comme suit (ESP8266 vers RC522) :
    D2  -> RST
    D5  -> SCK
    D6  -> MISO
    D7  -> MOSI
    D8  -> SDA
    G   -> GND
    3V3 -> 3.3V

## Prerequisites/Prérequis

VSCodium (or Visual Studio Code) with PlatformIO should be installed. You may also use Arduino IDE, as long as you read platformio.ini file to get the list of required libraries.

Vous devez avoir installé VSCodium (ou Visual Studio Code) avec PlatformIO. Vous pouvez également utiliser l'IDE Arduino, à condition d'extraire la liste des librairies requises indiquée dans le fichier platformio.ini.

## Installation

Follow these steps:

1. Clone repository in folder where you want to install it
```
cd <where_ever_you_want>
git clone https://github.com/FlyingDomotic/Rfid2Mqtt Rfid2Mqtt
```
2. Make ESP connections,
3. Copy FF_Rfid2MqttParameters.h from examples folder to src folder and update it,
4. Compile and load code into ESP,
5. At restart, ESP should connect to MQTT.

If needed, you can connect on serial/USB ESP link to see debug messages (at 74880 bds).

Suivez ces étapes :

1. Clonez le dépôt GitHub dans le répertoire où vous souhaitez l'installer
```
cd <là_où_vous_voulez_l'installer>
git clone https://github.com/FlyingDomotic/Rfid2Mqtt Rfid2Mqtt
```
2. Connecter l'ESP,
3. Copier FF_Rfid2MqttParameters.h depuis le répertoire examples vers le répertoire src folder et modifiez-le,it,
4. Compiler et charger le code dans l'ESP,
5. Au redémarrage, l'ESP doit se connecter sur MQTT/

Si besoin, vous pouvez vous connecter sur le lien série/USB de l'ESP pour voir les messages de déverminage (à 74880 bds).

##Schema/Schéma

A schema of possible implementation can be found in schema.jpg.

Un schéma d'une implémentation possible est disponible dans le fichier schema.jpg.

## Settings/Paramètres

Parameters are to be edited into FF_Rfid2MqttParameters.h. Set the parameters as:
    wifiSSID[] should contain WiFi SSID to connect to,
    wifiKey[] should contain WiFi password,
    rootNodeName[] should contain ESP root ame (will be followed by ESP chip ID),
    mqttServer[] should contain MQTT server IP address or name,
    mqttPort should contain MQTT server IP port,
    mqttUsername[] should contain MQTT username,
    mqttPassword[] should contain MQTT password,
    mqttRootUidTopic[] should contain MQTT UID topic, where id are sent to,
    mqttRootSetTopic[] should contain MQTT set topic, where led state is read,
    mqttRootLastWillTopic[] should contain MQTT Last Will Topic, where MQTT status is written.

Last 3 values are root folder topics. They'll be followed by 6 hexa characters given by ESP chip ID.

If you want to use syslog, define SYSLOG_USED (else comment it):
    syslogServer[] should contain syslog server IP address or name,
    syslogPort should contain syslog IP (UDP) port.

Les paramètres sont à éditer dans le fichier FF_Rfid2MqttParameters.h, selon les consignes suivantes :
    wifiSSID[] doit contenir le SSID du WiFi où se connected,
    wifiKey[] doit contenir le mot de passe WiFi,
    rootNodeName[] doit contenir la racine du nom de l'ESP (sera suivi du numéro de chip de l'ESP),
    mqttServer[] doit contenir l'adresse IP ou le nom du serveur MQTT,
    mqttPort doit contenir le port IP du serveur MQTT,
    mqttUsername[] doit contenir le nom d'utilisateur MQTT,
    mqttPassword[] doit contenir le mot de passe MQTT,
    mqttRootUidTopic[] doit contenir le sujet MQTT où l'identifiant lu doit être écrit,
    mqttRootSetTopic[] doit contenir le sujet MQTT où l'état de la LED doit être lu,
    mqttRootLastWillTopic[] doit contenir le sujet MQTT ou les dernières volontés (LWT) doivent être écrites.

Ces 3 dernièères valeurs sont des racines. Elles seront suivies de 6 caractères hexa-décmiaux pris sur l'ID du chip de l'ESP.

Si vous souhaitez utiliser un serveur syslog, If you want to use syslog, définissez SYSLOG_USED (sinon, commentez le avec //) :
    syslogServer[] doit contenir le nom ou l'adresse IP duu serveur syslog,
    syslogPort doit contenir le port IP (UDP) du serveur syslog.

## LED signals / Signaux LED

Internal LED is used to signal module state as follow :
    LED permanently blinks quickly when module is not connected to network,
    LED flashes 3 times and wait for 1 second when module is not connected to MQTT,
    LED slowly pulses on when ready to read a card,
    LED blinks twice a second for 5 seconds (or until server answers) when waiting for server response,
    LED blinks quickly for 2 seconds if badge is invalid/unknown,
    LED stays on (with slow pulse) when read card is valid.

La LED interne est utilisée pour indiquer l'état du module comme suit:
    La LED clignote vite de façon permanente si le module n'est pas connecté au réseau,
    La LED flashe 3 fois puis s'éteind pendant une seconde si le module n'est pas conencté à MQTT,
    La LED pulse lentement quand le module est prêt à lire une carte,
    La LED clignote 2 fois par seconde pendant 5 seconde (ou jusqu'à la réponse du serveur) quand le module attend une réponse du serveur,
    La LED clignote rapidement pendant 2 secondes si la carte n'est pas valide ou inconnue,
    La LED reste allumée (avec une pulsation lente) lorsque la carte est valide.

## Set topic values /  Valeurs du sujet Set

When "mqttRootUidTopic" topic is set by module, it waits for 5 seconds for server answer, to feedback user. Server could send a response, using "mqttRootSetTopic" topic. Set it to 2 if badge is invalid or 3 if valid. When operation is done, set it to 1 to ask for next badge.

Lorsque le sujet "mqttRootUidTopic" est positionné par le module, il attend 5 secondes une réponse du serveur, pour indiquer le résultat à l'utilisateur. Le serveur peut envoyer une réponse en utilisant le sujet "mqttRootSetTopic". Definissez-le à 2 si le badge est invalide ou 3 s'il est valide. Lorsque l'opération est terminée, mettez-le à 1 pour demander un noubeau badge.
