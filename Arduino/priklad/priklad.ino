#include <IoTVec.h>

//Proměnné platné všude:
IoTVec zarizeni; //Věc s kterou pracujeme

static int interval = 1000;
void setup() {
  pripojVypisy();

  zarizeni.nactiUdaje();

  zarizeni.pripojSvetlo();
  zarizeni.pripojSenzor();
  zarizeni.pripojTlacitko();

  zarizeni.nastavPripojeni();
  zarizeni.pripojSe();
}

void loop() {
  if (zarizeni.jeZpravaOdeslana())
    {
        char textZpravy[256];
        
        vytvorZpravu(0,0,0,textZpravy);
        zarizeni.posliZpravu(textZpravy);
        delay(interval);
    }
    zarizeni.delejPraci();
    delay(10);
}
