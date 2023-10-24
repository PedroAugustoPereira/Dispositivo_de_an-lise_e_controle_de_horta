#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include <Wire.h>
#include <dht.h>
// PINOS
#define pinSoilMoisture A0
#define pinSistIrrig 6
#define pinLdr A1
#define dht_pin 2
#define DHTTYPE DHT11
#define pinMotorEstender 8
#define pinMotorRecolher 9
// FIM DOS PINOS
61 RTC_DS1307 rtc;
void setup()
{
    Serial.begin(9600);
    Serial.print(F("Initializing SD card..."));
    if (!SD.begin(4))
    {
        Serial.println(F("initialization failed!"));
        while (1)
            ;
    }
    Serial.println(F("initialization done."));
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        while (1)
            delay(10);
    }
    EEPROM.write(0, 0);
    EEPROM.write(1, 0);
    pinMode(pinLdr, INPUT);
    pinMode(pinSoilMoisture, INPUT);
    pinMode(pinMotorEstender, OUTPUT);
    pinMode(pinMotorRecolher, OUTPUT);
    62 pinMode(pinSistIrrig, OUTPUT);
}
struct config_struct
{
    float solo_config;
    float umidade_ar_config;
    float temperatura_ar_config;
};
// VARIAVEIS
int minuto_atual;
int flag_minuto_atual;
int looping = 0;
String string_config;
int extend_sombrite = 0;
extern volatile unsigned long timer0_millis;
int flag_irrig = 0;
int flag_recolher = 0;
int flag_estender = 0;
63 config_struct config_data;
// FIM DE VARIAVEIS
void loop()
{
    dht dht;
    int flag_while = 0;
    int extendendo = 0;
    DateTime timer = rtc.now();
    DateTime now = rtc.now();
    int dia = now.day();
    int mes = now.month();
    int ano = now.year();
    int hora = now.hour();
    int minuto = now.minute();

    struct status_leituras
    {
        String status_solo;
        String status_luz;
        64 String string_status_solo;
    };
    struct sensores_struct
    {
        float solo_sensor;
        int umidade_ar_sensor = 0x00;
        int temperatura_ar_sensor = 0x00;
        int luz_sensor;
    };

    sensores_struct sensores_data;
    status_leituras status_comparacoes;

    dht.read11(dht_pin);

    if (EEPROM.read(0) != 0)
    {
    }
    else
    {
        File file_config = SD.open("config.txt");

        if (file_config.available())
        {
            string_config = file_config.readStringUntil('\0');

            int index_1 = string_config.indexOf('\n');
            65 config_data.solo_config = (string_config.substring(0, index_1)).toFloat();
            // segundo elemento == umidade_ar
            int index_2 = string_config.indexOf('\n', index_1 + 1);
            config_data.umidade_ar_config = (string_config.substring(index_1 + 1, index_2)).toFloat();
            // terceiro elemento == temperatura_ar
            int index_3 = string_config.indexOf('\n', index_2 + 1);
            config_data.temperatura_ar_config = (string_config.substring(index_2 + 1, index_3)).toFloat();
        }
        file_config.close();
        EEPROM.write(0, 1);
    }
    if (EEPROM.read(0) == 1)
    {
        if (!looping)
        {
            looping = 1;
        }
        else
        {
            Serial.println("loop");
            noInterrupts();
            timer0_millis = 0;
            interrupts();
            66

                while (millis() != 10000)
            {
            }
        }

        sensores_data.umidade_ar_sensor = dht.humidity;
        sensores_data.temperatura_ar_sensor = dht.temperature;
        sensores_data.solo_sensor = analogRead(pinSoilMoisture);

        sensores_data.luz_sensor = analogRead(pinLdr);
        if (sensores_data.solo_sensor >= 700)
        {
            status_comparacoes.status_solo = "Solo seco";
        }
        else if (sensores_data.solo_sensor <= 699 && sensores_data.solo_sensor >= 600)
        {
            status_comparacoes.status_solo = "Solo levemente umido";
        }
        else if (sensores_data.solo_sensor <= 599 && sensores_data.solo_sensor >= 550)
        {
            status_comparacoes.status_solo = "Solo umido";
        }
        else if (sensores_data.solo_sensor <= 549 && sensores_data.solo_sensor >= 510)
        {
            67 status_comparacoes.status_solo = "Solo muito umido";
        }
        else if (sensores_data.solo_sensor <= 509)
        {
            status_comparacoes.status_solo = "Solo enxarcado";
        }
        if (sensores_data.solo_sensor >= 730)
        {
            sensores_data.solo_sensor = 0.0;
        }
        else if (sensores_data.solo_sensor <= 460)
        {
            sensores_data.solo_sensor = 100.0;
        }
        else
        {
            // 730 - 450 = 280
            //  se = 270 = 100%
            //  se = 0 = 0%
            sensores_data.solo_sensor = ((730.0 - (float)sensores_data.solo_sensor) * 100.0) / 280.0;
        }
        if (sensores_data.solo_sensor == config_data.solo_config)
        {
            68 status_comparacoes.string_status_solo = "Umidade do solo em nível ideal";
        }
        else if (sensores_data.solo_sensor < config_data.solo_config +
                                                 (config_data.solo_config * 0.10) &&
                 sensores_data.solo_sensor >
                     config_data.solo_config - (config_data.solo_config * 0.10))
        {
            status_comparacoes.string_status_solo = "Umidade do solo em nivel estável com
                                                    margem de 10 %
                                                    ";
        }
        else if (sensores_data.solo_sensor > config_data.solo_config +
                                                 config_data.solo_config * 0.10)
        {
            status_comparacoes.string_status_solo = "Umidade do solo elevada, verifique";
        }
        else if (sensores_data.solo_sensor < config_data.solo_config -
                                                 config_data.solo_config * 0.10)
        {
            status_comparacoes.string_status_solo = "Solo seco, verifique";

            digitalWrite(pinSistIrrig, HIGH);
            Serial.println("Bomba ligou");
            flag_irrig = 0;
            if (flag_irrig == 0)
            {
                noInterrupts();
                timer0_millis = 0;
                interrupts();
                flag_irrig = 1;
            }
            while (millis() != 3000)
            {
                69
            }
            digitalWrite(pinSistIrrig, LOW);
            flag_irrig = 0;
            Serial.println("Bomba desligou");
        }
        Serial.println(sensores_data.luz_sensor);
        if (sensores_data.luz_sensor <= 200)
        {
            status_comparacoes.status_luz = "Sol muito forte";
            if (extend_sombrite == 0 && EEPROM.read(1 == 0))
            {
                digitalWrite(pinMotorEstender, HIGH);
                digitalWrite(pinMotorRecolher, LOW);
                extendendo = 1;
                Serial.println("extendendo");
                flag_estender = 0;
                if (flag_estender == 0)
                {
                    noInterrupts();
                    timer0_millis = 0;
                    interrupts();
                    70 flag_irrig = 1;
                }
                while (millis() != 4000)
                {
                }
                digitalWrite(pinMotorEstender, LOW);
                flag_estender = 0;

                extend_sombrite = 1;
                EEPROM.write(1, 1);
            }
        }
        if (sensores_data.luz_sensor > 200 && sensores_data.luz_sensor <= 800)
        {
            status_comparacoes.status_luz = "Sol moderado";
        }
        if (sensores_data.luz_sensor > 800)
        {
            status_comparacoes.status_luz = "Sol muito fraco ou noite";
        }
        if (extend_sombrite == 1 && sensores_data.luz_sensor > 200 && EEPROM.read(1) == 1)
        {
            int recolhendo;
            Serial.println("recolhendo");
            digitalWrite(pinMotorEstender, LOW);
            digitalWrite(pinMotorRecolher, HIGH);
            71 flag_recolher = 0;
            if (flag_recolher == 0)
            {
                noInterrupts();
                timer0_millis = 0;
                interrupts();
                flag_recolher = 1;
            }
            while (millis() != 400)
            {
            }
            digitalWrite(pinMotorEstender, LOW);
            flag_recolher = 0;

            extend_sombrite = 0;
            EEPROM.write(1, 0);
        }

        File statusFile = SD.open("status.txt", FILE_WRITE);
        if (statusFile)
        {
            statusFile.print(now.day());
            72 statusFile.print("/");
            statusFile.print(now.month());
            statusFile.print("/");
            statusFile.println(now.year());
            statusFile.print(now.hour());
            statusFile.print(":");
            statusFile.println(now.minute());
            statusFile.print("\tUmidade do solo: ");
            statusFile.print(sensores_data.solo_sensor);
            statusFile.println("%");
            statusFile.print(F("\tStatus:"));
            statusFile.print("\t");
            statusFile.print(status_comparacoes.status_solo);
            statusFile.print(F("\n\t"));
            statusFile.print(status_comparacoes.string_status_solo);
            statusFile.print("\n\n");
            statusFile.print(F("\tLuz"));
            statusFile.print(F("\tStatus:\n\t"));
            statusFile.print(status_comparacoes.status_luz);
            statusFile.print("\n\n");
            statusFile.print(F("\tUmidade do ar medida: "));
            statusFile.print(sensores_data.umidade_ar_sensor);
            statusFile.print("%\n");
            statusFile.print(F("\tUmidade do ar ideal: "));
            73 statusFile.print(config_data.umidade_ar_config);
            statusFile.print("%\n\n");
            statusFile.print(F("\tTemperautra do ar: "));
            statusFile.print(sensores_data.temperatura_ar_sensor);
            statusFile.print("graus\n");
            statusFile.print(F("\tTemperautra do ar ideal: "));
            statusFile.print(config_data.temperatura_ar_config);
            statusFile.print(F("graus\n\n"));
            statusFile.close();
            Serial.println("entrou em status file");
        }
        else
            Serial.println("não entrou em status file");
    }
}