#include <Wire.h>
#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <Adafruit_ADS1015.h>
#include <DallasTemperature.h>
#include <Adafruit_NeoPixel.h>
#include <BlynkSimpleEsp8266.h>

#define PIN 2
#define pHPin A0
#define NB_LED 28
#define Offset 0.00
#define ArrayLenth 40
#define waterFlowPin 4
#define ONE_WIRE_BUS 0
#define BLYNK_PRINT Serial
#define printInterval 10000
#define samplingInterval 20

char ssid[] = "Livebox-f69d";
char pass[] = "7F22C17585A3E830F18899CE23";
char auth[] = "ed88a5c8923b491cab8d7cc93f4c6277";

BlynkTimer timer;
Adafruit_ADS1115 ads(0x48);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NB_LED, PIN, NEO_GRB + NEO_KHZ800);

uint8_t couleur = 0;
int16_t moistureValue, adc1, adc2, adc3;
int pHPinValue, pHArrayIndex, pHArray[ArrayLenth];
static float pHValue, voltage;
volatile int tr;
int waterFlow;
float tempC;

void setup()
{
    Serial.begin(9600);
    ads.begin();
    Blynk.begin(auth, ssid, pass);
    sensors.begin();
    strip.begin();

    strip.setBrightness(50);
    strip.show();

    pinMode(waterFlowPin, INPUT);
    attachInterrupt(4, rpm, RISING);

    timer.setInterval(10000L, pulse);
    timer.setInterval(1000L, laPoste);
}

void loop()
{
    Blynk.run();
    timer.run();
    moistureValue = ads.readADC_SingleEnded(0);
    adc1 = ads.readADC_SingleEnded(1);
    adc2 = ads.readADC_SingleEnded(2);
    adc3 = ads.readADC_SingleEnded(3);
    moistureValue = moistureValue / 142;
    pH();
}

//----------Send data----------//
void laPoste()
{
    Blynk.virtualWrite(V4, pHValue);
    Blynk.virtualWrite(V5, moistureValue);
    Blynk.virtualWrite(V7, waterFlow);
    Blynk.virtualWrite(V2, tempC);
    _couleur();
    temp();
}

//----------pH----------//
void pH()
{
    static uint32_t samplingTime = millis(), printTime = millis();

    if (millis() - samplingTime > samplingInterval)
    {
        pHArray[pHArrayIndex++] = analogRead(pHPin);
        if (pHArrayIndex == ArrayLenth)
        {
            pHArrayIndex = 0;
        }
        // Il faut une régulation à 5V stable pour pas fausser les valeurs
        voltage = avergearray(pHArray, ArrayLenth) *5.0 / 1024.0;
        //voltage = 5 - voltage;
        pHValue = 3.5 * voltage + Offset;
        samplingTime = millis();
    }
}

double avergearray(int *arr, int number)
{
    int i;
    int max, min;
    double avg;
    long amount = 0;
    if (number <= 0)
    {
        Serial.println("Erreur dans les nombres de l'array!/n");
        return 0;
    }
    if (number < 5)
    { //less than 5, calculated directly statistics
        for (i = 0; i < number; i++)
        {
            amount += arr[i];
        }
        avg = amount / number;
        return avg;
    }
    else
    {
        if (arr[0] < arr[1])
        {
            min = arr[0];
            max = arr[1];
        }
        else
        {
            min = arr[1];
            max = arr[0];
        }
        for (i = 2; i < number; i++)
        {
            if (arr[i] < min)
            {
                amount += min; //arr<min
                min = arr[i];
            }
            else
            {
                if (arr[i] > max)
                {
                    amount += max; //arr>max
                    max = arr[i];
                }
                else
                {
                    amount += arr[i]; //min<=arr<=max
                }
            } //if
        }     //for
        avg = (double)amount / (number - 2);
    } //if
    return avg;
}

//----------Color----------//
BLYNK_WRITE(V1)
{
    couleur = param.asInt();
}

void _couleur()
{
    switch (couleur)
    {
    case 1:
        colorWipe(strip.Color(255, 0, 255), 10); // Opti
        break;
    case 2:
        colorWipe(strip.Color(0, 0, 255), 10); // Blue
        break;
    case 3:
        colorWipe(strip.Color(255, 0, 0), 10); // Red
        break;
    default:
        colorWipe(strip.Color(0, 0, 255), 10); // Opti default
        break;
    }
}

//----------Brightness----------//
BLYNK_WRITE(V3)
{
    strip.setBrightness(param.asInt());
}

//----------Apply color all LED----------//
void colorWipe(uint32_t c, uint8_t wait)
{
    for (uint16_t i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, c);
        strip.show();
        delay(wait);
    }
}

//----------WaterFlow----------//
void pulse()
{
    tr = 0;
    sei();
    delay (1000);
    cli();
    waterFlow = (tr * 60 / 7.5);
}

void rpm ()
{
    tr++;
}

void temp()
{
    sensors.requestTemperatures();
    tempC = sensors.getTempCByIndex(0);
}