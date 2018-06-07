#include <ClickEncoder.h>
#include <TimerOne.h>
#include <FastLED.h>
#include <EEPROMex.h>

#define DEBUG_OUTBUT 0
#define EEPROM_SETTINGS 1

#define ENC_HALFSTEP 0

#define MODE_COUNT 2

#define LED_PIN 4
#define COLOR_ORDER GRB
#define CHIPSET WS2811
#define NUM_ROWS 15
#define NUM_COLS 15
#define NUM_LEDS (NUM_ROWS * NUM_COLS)
#define FRAMES_PER_SECOND 60

#define PSU_MAX_MAMPS 1500 
#define button_pin 5
#define enup_pin 7
#define endown_pin 6

#define petentiometer_pin A1

CRGB leds[NUM_LEDS];

#include "ColorPalettes.h"

CRGBPalette32 current_flame_palette = flame_palette_fire;

byte matrix[NUM_ROWS][NUM_COLS];

ClickEncoder *encoder;
int16_t last, value;

int brightness = 512;

// encoder globals
int last_encoder_position;

// controls state
byte mode = 0;
int hold = 0;

// Mode specific params
// 0: flame palette
byte flame_palette = 0;
byte flame_dissipation = 70;

// 1: static lamp
byte lamp_hue = 1;
byte lamp_saturation = 200;

unsigned long time, renderTime, eepromTime;

void timerIsr()
{
    encoder->service();
    last_encoder_position = encoder->getValue();
    ClickEncoder::Button butt = encoder->getButton(); 
    if (butt != ClickEncoder::Open)
    {
        switch (butt)
        {
        case ClickEncoder::Pressed:
            break;
        case ClickEncoder::Held:
            hold = 1;
            break;
        case ClickEncoder::Released:
            hold = 0;
            break;
        case ClickEncoder::Clicked:
            mode++;
            if (mode >= MODE_COUNT)
            {
                mode = 0;
            }
            eepromTime = millis();
            break;
        case ClickEncoder::DoubleClicked:
            break;
        }
    }
}

void setup()
{
    delay(500); // sanity delay

    FastLED.setMaxPowerInVoltsAndMilliamps(5, PSU_MAX_MAMPS);   // V, mA

    FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(brightness >> 2);

#ifdef DEBUG_OUTBUT
    Serial.begin(9600);
#endif

    if (EEPROM_SETTINGS)
    {
        if (EEPROM.read(50) != 250)
        {
            EEPROM.write(50, 250);
            write_eeprom();
        }
        else
        {
            read_eeprom();
        }
    }

    pinMode(enup_pin, INPUT_PULLUP);
    pinMode(endown_pin, INPUT_PULLUP);
    pinMode(button_pin, INPUT);
    
    encoder = new ClickEncoder(enup_pin, endown_pin, button_pin);

    Timer1.initialize(1000);
    Timer1.attachInterrupt(timerIsr);

    last = -1;
}

void loop()
{
    int newBrightness = analogRead(petentiometer_pin);
    if (newBrightness!=brightness) {
        brightness = newBrightness;
        FastLED.setBrightness(brightness >> 2);
    }

    if ((value+last_encoder_position) != last )
    {
        value += last_encoder_position;
        last = value;
        eepromTime = millis();

        switch (mode) {
            case 0:
                if ((millis()-time)>430) {
                    time = millis();
                    if (hold)
                    {
                        flame_dissipation += last_encoder_position * 2;
                        if (flame_dissipation < 1)
                        {
                            flame_dissipation = 1;
                        }
                    }
                    else
                    {
                        if (last_encoder_position > 0)
                        {
                            flame_palette++;
                        }
                        if (last_encoder_position < 0)
                        {
                            flame_palette--;
                        }
                        if (flame_palette >= gFlamePalettesCount)
                        {
                            flame_palette = 0;
                        }
                        if (flame_palette < 0)
                        {
                            flame_palette = gFlamePalettesCount-1;
                        }
                        current_flame_palette = gFlamePalettes[flame_palette];
                    }
                }

#ifdef DEBUG_OUTBUT
                Serial.print(" last_encoder_position: ");
                Serial.print(last_encoder_position);
                Serial.print(" hold: ");
                Serial.print(hold);
                Serial.print("  Flame settings: ");
                Serial.print("mode: ");
                Serial.print(mode);
                Serial.print("; dissipation: ");
                Serial.print(flame_dissipation);
                Serial.print("; palette: ");
                //
                Serial.print(flame_palette);
                Serial.print("; brightness: ");
                Serial.println(brightness);
#endif

                break;
            case 1:
                if (hold)
                {
                    lamp_saturation += last_encoder_position * 8;
                    if (lamp_saturation < 0)
                    {
                        lamp_saturation = 1;
                    } else if (lamp_saturation>255)
                    {
                        lamp_saturation = 255;
                    }
                }
                else
                {
                    lamp_hue += last_encoder_position * 8;
                    if (lamp_hue < 0)
                    {
                        lamp_hue = 255 ;
                    }
                    else if (lamp_hue > 255)
                    {
                        lamp_hue = 0;
                    }
                }
#ifdef DEBUG_OUTBUT
                Serial.print(" last_encoder_position: ");
                Serial.print(last_encoder_position);
                Serial.print(" hold: ");
                Serial.print(hold);
                Serial.print("Lamp settings: ");
                Serial.print("mode: ");
                Serial.print(mode);
                Serial.print("; hue: ");
                Serial.print(lamp_hue);
                Serial.print("; sat: ");
                Serial.print(lamp_saturation);
                Serial.print("; brightness: ");
                Serial.println(brightness);
#endif
                break;
        }
    }

    switch (mode) {
        case 0:
            if ((millis() - renderTime) >= (1000 / FRAMES_PER_SECOND)) {
                Fire2012();
                PutMatrix();
                renderTime = millis();
                //            FastLED.delay(1000 / FRAMES_PER_SECOND);
            }
            break;
        case 1:
            fill_solid(leds, NUM_LEDS, CHSV(lamp_hue, lamp_saturation, brightness>>2));
            FastLED.show();
            break;
    }

    eeprom_timer();
}

#define SPARKING 130
void Fire2012() 
{
    // Step 1.  Cool down every cell a little
    for (int j = 0; j < NUM_COLS; j++)
    {
        for (int i = 0; i < NUM_ROWS; i++)
        {
            matrix[i][j] = qsub8(matrix[i][j], random8(0, ((flame_dissipation * 10) / NUM_ROWS) + 2));
        }
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int j = 0; j < NUM_COLS; j++)
    {
        for (int k = NUM_ROWS - 1; k > 2; k--)
        {
            matrix[k][j] = (matrix[k - 1][j] + matrix[k - 2][j] + matrix[k - 2][j]) / 3;
        }
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    for (int j = 0; j < NUM_COLS; j++)
    {
        if (random8() < SPARKING) {
            int y = random8(7);
            matrix[y][j] = qadd8(matrix[y][j], random8(160, 255));
        }
    }
}

void PutMatrix()
{
    int i = 0;
    for (int c = 0; c < NUM_COLS; c++)
    {
        for (int r = 0; r < NUM_ROWS; r++)
        {
            if ( (c % 2) == 0 ) {
                leds[i] = ColorFromPalette(current_flame_palette, matrix[r][c]);
            } else {
                leds[i] = ColorFromPalette(current_flame_palette, matrix[(NUM_ROWS-1)-r][c]);
            }
            i++;
        }
    }
    FastLED.show();
}

void write_eeprom()
{
    EEPROM.updateByte(1, mode);
    EEPROM.updateByte(2, flame_palette);
    EEPROM.updateByte(3, lamp_hue);
    EEPROM.updateByte(4, lamp_saturation);
}
void read_eeprom()
{
    mode = EEPROM.readByte(1);
    flame_palette = EEPROM.readByte(2);
    lamp_hue = EEPROM.readByte(3);
    lamp_saturation = EEPROM.readByte(4);
}
void eeprom_timer()
{
    if ((eepromTime>0) && ((millis() - eepromTime) > 3000))
    {
        eepromTime = 0;
        write_eeprom();
    }
}