#include <RadioLib.h>
#include "LoRaBoards.h"
#include <WiFi.h>
#include <HTTPClient.h>
#if defined(USING_SX1280)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           2400.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   13
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             203.125
#endif
SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif
// zastava koja ukazuje da je paket primljen
static volatile bool receivedFlag = false;
static String rssi = "0dBm";
static String snr = "0dB";
static String payload = "0";
const char ssid[] = "";  //ime Wi-Fi mreže
const char password[] = "";    //šifra Wi-Fi mreže
String serverUrl = ""; // ip adresa + php ime fajla
String message;
// ova funkcija se poziva kada je modul primio paket
void setFlag(void)
{
    // paket je stigao postavljamo zastavicu
    receivedFlag = true;
}
void setup()
{
    setupBoards();

    // Kada se uredjaj uključi, neophodno je kašnjenje
    delay(1500);

#ifdef  RADIO_TCXO_ENABLE
    pinMode(RADIO_TCXO_ENABLE, OUTPUT);
    digitalWrite(RADIO_TCXO_ENABLE, HIGH);
#endif

    // inicijalizacija radija podrazumevanim podešavanjima
    int state = radio.begin();

    printResult(state == RADIOLIB_ERR_NONE);

    Serial.print(F("Radio Initializing ... "));
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }

    // Funkcija koja se poziva, kada je paket primljen
    radio.setPacketReceivedAction(setFlag);

    // Postavljanje noseće vrekvencije.
    if (radio.setFrequency(CONFIG_RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));
        while (true);
    }

    // Postavljanje propusnog opsega
    if (radio.setBandwidth(CONFIG_RADIO_BW) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("Selected bandwidth is invalid for this module!"));
        while (true);
    }


    // Postavljanje faktora širenja
    if (radio.setSpreadingFactor(12) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println(F("Selected spreading factor is invalid for this module!"));
        while (true);
    }

    // Postavljanje kodnog odnosa
    if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Selected coding rate is invalid for this module!"));
        while (true);
    }

    //Postavljanje sinhronizovane reči
    if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
        Serial.println(F("Unable to set sync word!"));
        while (true);
    }

    //Postavljanje izlazne snage
    if (radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));
        while (true);
    }
    // Postavlja dužinu preambule za LoRa ili FSK modem.
    if (radio.setPreambleLength(16) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
        Serial.println(F("Selected preamble length is invalid for this module!"));
        while (true);
    }

    // Omogućava ili onemogućava CRC proveru primljenih paketa.
    if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Serial.println(F("Selected CRC is invalid for this module!"));
        while (true);
    }

    delay(1000);

    // Osluškuje LoRa pakete
    Serial.print(F("Radio Starting to listen ... "));
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
    }
    WiFi.begin(ssid, password);
    Serial.print("Povezivanje na WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi povezan!");
 
}
void loop()
{
    if (receivedFlag) {
        receivedFlag = false;

        // Čitanje primljenih podataka
        int state = radio.readData(payload);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.print("Primljeni podaci: ");
            Serial.println(payload);

            // Slanje podataka na server ako je WiFi povezan
            if (WiFi.status() == WL_CONNECTED) {
                HTTPClient http;
                http.begin(serverUrl);
                http.addHeader("Content-Type", "application/x-www-form-urlencoded");
                String postData = "data=" + payload;
                int httpResponseCode = http.POST(postData);

                if (httpResponseCode > 0) {
                    Serial.printf("Podaci poslati, kod: %d\n", httpResponseCode);
                } else {
                    Serial.printf("Greška pri slanju, kod: %d\n", httpResponseCode);
                }
                http.end();
            }
        } else {
            Serial.println("Greška pri čitanju podataka!");
        }

        // Ponovo postavi modul u režim slušanja
        radio.startReceive();
    }

    // Dodatno kašnjenje između provera prijema
    delay(10000);
}
