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

  zarizeni.nastavPosluchacePrikazu(posluchacPrikazu);
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

int posluchacPrikazu(const char *nazevPrikazu, const unsigned char *data, size_t size, unsigned char **odpoved, size_t *odpovedVelikost, void *uzivatelskaZpetnaVazba)
{
    const char *textOdpovedi = "Uspech";
    int vysledek = 200;


    if (strcmp(methodName, "toggleOn") == 0)
    {
    //Příkaz pro zapnutí světla
    //SEM doplnit



    }
    else if (strcmp(methodName, "toggleOff") == 0)
    {
    //Příkaz pro vypnutí světla
    //SEM doplnit prikazy:



    }
    else
    {
        textOdpovedi = "Nenalezeno";
        vysledek = PRIKAZ_NEEXISTUJE;
    }
    *odpovedVelikost = strlen(textOdpovedi);
    *odpoved = (unsigned char *)malloc(*response_size);
    strncpy((char *)(*odpoved), textOdpovedi, *odpovedVelikost);

    return result;
}
