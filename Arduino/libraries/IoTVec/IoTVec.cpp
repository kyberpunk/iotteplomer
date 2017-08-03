#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#include "IoTVec.h"

#define DEVICE_ID "ESP8266"
#define DHT_TYPE DHT11

// Pin layout configuration
#define LED_PIN LED_BUILTIN
#define DHT_PIN 2
#define BUTT_PIN 0

// Interval time(ms) for sending message to IoT Hub
#define INTERVAL 1000

// EEPROM address configuration
#define EEPROM_SIZE 512

// SSID and SSID password's length should < 32 bytes
// http://serverfault.com/a/45509
#define SSID_LEN 32
#define PASS_LEN 32
#define CONNECTION_STRING_LEN 256

#define MESSAGE_MAX_LEN 256



bool readFromSerial(char * prompt, char * buf, int maxLen, int timeout);
static void initWifi(char* ssid, char* pass);
static void initTime();
static bool needEraseEEPROM();
static void clearParam();
static void EEPROMWrite(int addr, char *data, int size);
static int EEPROMread(int addr, char *buf);

void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback);


//#####Soukrome a pomocne funkce########################################################################################

/* Read a string whose length should in (0, lengthLimit) from Serial and save it into buf.
 *
 *        prompt   - the interact message and buf should be allocaled already and return true.
 *        buf      - a part of memory which is already allocated to save string read from serial
 *        maxLen   - the buf's len, which should be upper bound of the read-in string's length, this should > 0
 *        timeout  - If after timeout(ms), return false with nothing saved to buf.
 *                   If no timeout <= 0, this function will not return until there is something read.
 */
bool readFromSerial(char * prompt, char * buf, int maxLen, int timeout)
{
    int timer = 0, delayTime = 1000;
    String input = "";
    if(maxLen <= 0)
    {
        // nothing can be read
        return false;
    }

    Serial.println(prompt);
    while(1)
    {
        input = Serial.readString();
        int len = input.length();
        if(len > maxLen)
        {
            LogInfo("Muzes vlozit mene nez %d znaku, vlozil jsi %d znaku.\n", maxLen, len);
        }
        else if (len > 0)
        {
            // save the input into the buf
            sprintf(buf, "%s", input.c_str());
            return true;
        }

        // if timeout, return false directly
        timer += delayTime;
        if(timeout > 0 && timer >= timeout)
        {
            LogInfo("Vlozil jsi \"nic\", preskakuji...");
            return false;
        }
        // delay a time before next read
        delay(delayTime);
    }
}

static void initWifi(char* ssid, char* pass)
{
	int first = 1;
    // Attempt to connect to Wifi network:
    LogInfo("Pokousim se pripojit k Wi-Fi siti: %s", ssid);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
		
        // Get Mac Address and show it.
        // WiFi.macAddress(mac) save the mac address into a six length array, but the endian may be different. The huzzah board should
        // start from mac[0] to mac[5], but some other kinds of board run in the oppsite direction.
        uint8_t mac[6];
        WiFi.macAddress(mac);
        LogInfo("Vase zarizeni s MAC adresou %02x:%02x:%02x:%02x:%02x:%02x neni pripojeno! Zkousim se pripojit.",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ssid);
        WiFi.begin(ssid, pass);
        if (first == 0) {
			delay(10000);
		}
		else
		{
			delay(5000);
		}
		first = 0;
    }
    LogInfo("Uspesne pripojeno k Wi-Fi %s!", ssid);
}

static void initTime()
{
    time_t epochTime;
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    while (true)
    {
        epochTime = time(NULL);

        if (epochTime == 0)
        {
            LogInfo("Ziskavani casu z internetu selhalo! Zkusim to znovu za 2 vteriny.");
            delay(2000);
        }
        else
        {
            LogInfo("Cas v internetu je: %lu", epochTime);
            break;
        }
    }
}


static bool needEraseEEPROM()
{
    char result = 'n';
    readFromSerial("Potrebujes znovu vlozit prihlasovaci udaje?(automaticky se preskoci za 5 vterin)[A/n]", &result, 1, 5000);
    if (result == 'A' || result == 'a')
    {
        clearParam();
        return true;
    }
    return false;
}

// reset the EEPROM
static void clearParam()
{
    char data[EEPROM_SIZE];
    memset(data, '\0', EEPROM_SIZE);
    EEPROMWrite(0, data, EEPROM_SIZE);
}

#define EEPROM_END 0
#define EEPROM_START 1
static void EEPROMWrite(int addr, char *data, int size)
{
    EEPROM.begin(EEPROM_SIZE);
    // write the start marker
    EEPROM.write(addr, EEPROM_START);
    addr++;
    for (int i = 0; i < size; i++)
    {
        EEPROM.write(addr, data[i]);
        addr++;
    }
    EEPROM.write(addr, EEPROM_END);
    EEPROM.commit();
    EEPROM.end();
}

// read bytes from addr util '\0'
// return the length of read out.
static int EEPROMread(int addr, char *buf)
{
    EEPROM.begin(EEPROM_SIZE);
    int count = -1;
    char c = EEPROM.read(addr);
    addr++;
    if (c != EEPROM_START)
    {
        return 0;
    }
    while (c != EEPROM_END && count < EEPROM_SIZE)
    {
        c = (char)EEPROM.read(addr);
        count++;
        addr++;
        buf[count] = c;
    }
    EEPROM.end();
    return count;
}

//#####Verejne funkce###################################################################################################

void pripojVypisy()
{
    // Start serial and initialize stdout
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    LogInfo("Vypisovani uspesne pripraveno.");
}

int porovnejText(char* prvni, char* druhy)
{
	return strcmp(prvni, druhy);
}

void vytvorZpravu(float teplota, float vlhkost, int spinac, char* vystup)
{
	float temperature = teplota;
    float humidity = vlhkost;
    float switchLevel = spinac;
    StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["deviceId"] = DEVICE_ID;
    root["messageId"] = 0;

    // NAN is not the valid json, change it to NULL
    if (std::isnan(temperature))
    {
        root["temperature"] = NULL;
    }
    else
    {
        root["temperature"] = temperature;
    }

    if (std::isnan(humidity))
    {
        root["humidity"] = NULL;
    }
    else
    {
        root["humidity"] = humidity;
    }

    
    {
        root["switchLevel"] = switchLevel; //(digitalRead(BUTT_PIN)>0)?0:1;
    }
    root.printTo(vystup, MESSAGE_MAX_LEN);
}


	IoTVec::IoTVec()
	{
		//this->dht = DHT(DHT_PIN, DHT_TYPE);
	}

	void IoTVec::pripojSvetlo()
	{
		pinMode(LED_PIN, OUTPUT);
		digitalWrite(LED_PIN, HIGH);
	}

	void IoTVec::zapniSvetlo()
	{
		LogInfo("Svetlo zapnuto!");
		digitalWrite(LED_PIN, LOW);
	}
	
	void IoTVec::vypniSvetlo()
	{
		LogInfo("Svetlo vypnuto!");
		digitalWrite(LED_PIN, HIGH);
	}
	
	void IoTVec::pripojSenzor()
	{
		this->dht.begin();
		this->roundTemp = this->dht.readTemperature();
		this->roundHum = this->dht.readHumidity();
	}
	
	float IoTVec::ctiTeplotu()
	{
		float temp = this->dht.readTemperature();
		this->roundTemp = this->roundTemp * 0.3 + temp * 0.7;
		return this->roundTemp;
	}
	
	float IoTVec::ctiVlhkost()
	{
		float hum = this->dht.readHumidity();
		this->roundHum = this->roundHum * 0.3 + hum * 0.7;
		return this->roundHum;	
	}
	
	void IoTVec::pripojTlacitko()
	{
		pinMode(BUTT_PIN, INPUT);
	}
	
	int IoTVec::ctiTlacitko()
	{
		return (digitalRead(BUTT_PIN)>0)?0:1;
	}


// Read parameters from EEPROM or Serial
void IoTVec::nactiUdaje()
{
    int ssidAddr = 0;
    int passAddr = ssidAddr + SSID_LEN;
    int connectionStringAddr = passAddr + SSID_LEN;

    // malloc for parameters
    this->ssid = (char *)malloc(SSID_LEN);
    this->pass = (char *)malloc(PASS_LEN);
    this->connectionString = (char *)malloc(CONNECTION_STRING_LEN);

    // try to read out the credential information, if failed, the length should be 0.
    int ssidLength = EEPROMread(ssidAddr, this->ssid);
    int passLength = EEPROMread(passAddr, this->pass);
    int connectionStringLength = EEPROMread(connectionStringAddr, this->connectionString);

    if (ssidLength > 0 && passLength > 0 && connectionStringLength > 0 && !needEraseEEPROM())
    {
        return;
    }

    // read from Serial and save to EEPROM
    readFromSerial("Vloz jméno Wi-Fi site (SSID): ", this->ssid, SSID_LEN, 0);
    EEPROMWrite(ssidAddr, this->ssid, strlen(this->ssid));

    readFromSerial("Vloz heslo Wi-Fi site: ", this->pass, PASS_LEN, 0);
    EEPROMWrite(passAddr, this->pass, strlen(this->pass));

    readFromSerial("Vloz \"pripojovaci retezec\" pro zarizeni: ", this->connectionString, CONNECTION_STRING_LEN, 0);
    EEPROMWrite(connectionStringAddr, this->connectionString, strlen(this->connectionString));
}


void IoTVec::nastavPripojeni()
{
	initWifi(this->ssid, this->pass);
	initTime();
}


void IoTVec::pripojSe()
{
	this->iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(this->connectionString, MQTT_Protocol);
    if (this->iotHubClientHandle == NULL)
    {
        LogInfo("Chyba, Nepodarilo se pripojit pomoci \"pripojovaciho retezce\".");
        while (1);
    }
}

bool IoTVec::jeZpravaOdeslana()
{
	return !(this->messagePending);
}

void IoTVec::posliZpravu(char *naklad)
{
	IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char *)naklad, strlen(naklad));
    if (messageHandle == NULL)
    {
        LogInfo("Chyba, nepodarilo se vytvorit zpravu.");
    }
    else
    {
        MAP_HANDLE properties = IoTHubMessage_Properties(messageHandle);
        //LogInfo("Posilam zpravu: %s", buffer);
        LogInfo("Posilam zpravu.");
        if (IoTHubClient_LL_SendEventAsync(this->iotHubClientHandle, messageHandle, sendCallback, &(this->messagePending)) != IOTHUB_CLIENT_OK)
        {
            LogInfo("Chyba pri posilani zpravy.");
        }
        else
        {
            this->messagePending = true;
            LogInfo("Zprava je uspesne odeslana.");
        }

        IoTHubMessage_Destroy(messageHandle);
    }
}

void IoTVec::nastavPosluchacePrikazu(IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC posluchacPrikazu)
{
	IoTHubClient_LL_SetDeviceMethodCallback(this->iotHubClientHandle, posluchacPrikazu, NULL);
}

void IoTVec::delejPraci()
{
	IoTHubClient_LL_DoWork(this->iotHubClientHandle);
}

void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
	bool *uspech = (bool*)userContextCallback;
	if (IOTHUB_CLIENT_CONFIRMATION_OK == result)
    {
        LogInfo("Zprava byla serverem uspesne prijata.");
    }
    else
    {
        LogInfo("Chyba, zprava byla serverem odmitnuta.");
    }
    *uspech = false;
}
