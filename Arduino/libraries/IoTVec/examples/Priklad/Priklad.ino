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

        //Hodnoty teploměru:
        float teplota = 0;
        float vlhkost = 0;

        //Spínač:
        //když je hodnota 0, tak je spínač uvolněný
        //když je hodnota 1, tak je spínač zmáčknutý
        int spinac = 0;

        //Zde je příklad toho, jak může vypadat odesílání zprávy
        teplota = zarizeni.ctiTeplotu();
        vlhkost = zarizeni.ctiVlhkost();
        spinac = zarizeni.ctiTlacitko();

        vytvorZpravu(teplota,vlhkost,spinac,textZpravy);
        zarizeni.posliZpravu(textZpravy);
        delay(interval);
    }
    zarizeni.delejPraci();
    delay(10);
}

int posluchacPrikazu(const char *nazevPrikazu, const unsigned char *data, size_t size, unsigned char **odpoved, size_t *odpovedVelikost, void *uzivatelskaZpetnaVazba)
{
    const char *textOdpovedi = "\"Uspech\"";
    int vysledek = 200;


    if (porovnejText(nazevPrikazu, "zapni") == 0)
    {
    //Příkaz pro zapnutí světla
    zarizeni.zapniSvetlo();

    }
    else if (porovnejText(nazevPrikazu, "vypni") == 0)
    {
    //Příkaz pro vypnutí světla
    zarizeni.vypniSvetlo();

    }
    else
    {
        textOdpovedi = "\"Nenalezeno\"";
        vysledek = PRIKAZ_NEEXISTUJE;
    }
    *odpovedVelikost = strlen(textOdpovedi);
    *odpoved = (unsigned char *)malloc(*odpovedVelikost);
    strncpy((char *)(*odpoved), textOdpovedi, *odpovedVelikost);
    return vysledek;
}
