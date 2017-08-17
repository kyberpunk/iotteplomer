
#ifndef IoTDevice_h
#define IoTDevice_h

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif 

#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>
#include <DHT.h>

#define DHT_TYPE DHT11

#define DHT_PIN 2

#define PRIKAZ_USPESNY 200
#define PRIKAZ_NEEXISTUJE 404


void pripojVypisy();

int porovnejText( const char *prvni, const char* druhy);

void vytvorZpravu(float teplota, float vlhkost, int spinac, char* vystup);

class IoTVec
{
  public:
    IoTVec();
    
    void pripojSvetlo();
	void zapniSvetlo();
	void vypniSvetlo();
	
	void pripojSenzor();
	float ctiTeplotu();
	float ctiVlhkost();
	
	void pripojTlacitko();
	int ctiTlacitko();
	
	void nactiUdaje();
    void nastavPripojeni();
	void pripojSe();
    bool jeZpravaOdeslana();
    void posliZpravu(char *naklad);
    void nastavPosluchacePrikazu(IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback);
    void delejPraci();
    
	private:
	char *ssid;
	char *pass;
	char *connectionString;
	bool messagePending;
	
	DHT dht{DHT_PIN, DHT_TYPE};
	float roundTemp = 0.0f;
	float roundHum = 0.0f;
	
	IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
	
};



#endif

