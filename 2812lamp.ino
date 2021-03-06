#include <ClickEncoder.h>
#include <TimerOne.h>
#include <FastLED.h>
#include <EEPROMex.h>

#include "globals.h"

#define ENC_HALFSTEP 0

#define MODE_COUNT 4

#define LED_PIN 10
#define COLOR_ORDER GRB
#define CHIPSET WS2812

#define PSU_MAX_MAMPS 2000 
#define button_pin 5
#define enup_pin 7
#define endown_pin 6

#define petentiometer_pin A1

#define turnoffTimeout 3600000*5

#include "ColorPalettes.h"
#include "TorchMode.h"

CRGBPalette32 current_flame_palette = flame_palette_fire;

ClickEncoder *encoder;
int16_t last, value;

byte brightness = 0;
int brightTimer = 1;

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

unsigned long time, renderTime, eepromTime, turnoffTime;

// enable/disable turnoff timer
byte turnoffTimer = 0;

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
            // reset torch
            if (mode == 3) {
                resetEnergy();
            }
            break;
        case ClickEncoder::DoubleClicked:
            if (turnoffTimer) {
                turnoffTimer = 0;
            } else {
                turnoffTimer = 1;
                turnoffTime = millis();
            }
            eepromTime = millis();
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

#ifdef DEBUG_OUTPUT
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

    if (turnoffTimer)
    {
        turnoffTime = millis();
    }

    last = -1;
}

void loop()
{
    if (turnoffTimer && ((millis() - turnoffTime) < turnoffTimeout))
    {
        mainLoop();
    }
    else if (turnoffTimer && ((millis() - turnoffTime) < (turnoffTimeout+10000))) {
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CRGB(0, 0, 0);
        }
        FastLED.setBrightness(0);
        FastLED.show();
    }
}

void mainLoop()
{
    int newBrightness = analogRead(petentiometer_pin) >> 2;
    if (turnoffTimer && ((millis() - turnoffTime)>(turnoffTimeout-60000))) {
        newBrightness = newBrightness * ((turnoffTimeout - millis() + turnoffTime) / 60000);
    }
    if ((brightTimer > 0) && ((millis() - brightTimer) > 10))
    {
        brightness++;
        FastLED.setBrightness(brightness);
        if (brightness>=newBrightness) {
            brightTimer = 0;
        } else {
            brightTimer = millis();
        }
    } 
    if ((newBrightness!=brightness) && (brightTimer==0)) {
        brightness = newBrightness;
        FastLED.setBrightness(brightness);
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
                            if (flame_palette == 0)
                            {
                                flame_palette = gFlamePalettesCount;
                            } else {
                                flame_palette--;
                            }
                        }
                        if (flame_palette > gFlamePalettesCount)
                        {
                            flame_palette = 0;
                        }

                        if (flame_palette > 0)
                        {
                            current_flame_palette = gFlamePalettes[flame_palette-1];
                        }                    
                    }
                }

#ifdef DEBUG_OUTPUT
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
#ifdef DEBUG_OUTPUT
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
            }
            break;
        case 1:
            fill_solid(leds, NUM_LEDS, CHSV(lamp_hue, lamp_saturation, brightness>>2));
            FastLED.show();
            break;
        case 2:
            if ((millis() - renderTime) >= 5) // 120 fps
            {
                torch();
                FastLED.show();
                renderTime = millis();
            }
            break;
        case 3:
            if ((millis() - renderTime) >= (1000 / FRAMES_PER_SECOND)) {
                IgniteFlagSparks();
                ByFlag();
                PutByMatrix();
                renderTime = millis();
            }
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

void ByFlag()
{
    for (int x = 0; x < NUM_COLS; x++)
    {
        for (int i = 0; i < NUM_ROWS; i++)
        {
            matrix[i][x] = avg8(matrix[i][x], matrix[i][normalizeX(x + 1)]);
        }
    }
}

int normalizeX(int x)
{
    if (x < 0)
    {
        return NUM_COLS - 1;
    }
    if (x >= NUM_COLS)
    {
        return 0;
    }
    return x;
}

void IgniteFlagSparks()
{
    for (int curCol = 0; curCol < NUM_COLS; curCol++)
    {
        int ignite = random8(0, 100);
        int i = random8(0, NUM_ROWS);
        if ( ignite >= 80) //>200
        {
            matrix[i][curCol] =  qadd8(matrix[i][curCol], random8(230, 255));
        }
        else if (ignite < 15) //<20
        {
            matrix[i][curCol] = qsub8(matrix[i][curCol], random8(150, 240));
        }

    }
}

void PutMatrix()
{
    int i = 0;
    int pixel;
    for (int c = 0; c < NUM_COLS; c++)
    {
        for (int r = 0; r < NUM_ROWS; r++)
        {
            if ( (c % 2) == 0 ) {
                pixel = matrix[r][c];
            } else {
                pixel = matrix[(NUM_ROWS-1)-r][c];
            }
            if (flame_palette > 0 )
            {
                leds[i] = ColorFromPalette(current_flame_palette,pixel);
            } else {
                leds[i] = nonlinearEnergy(pixel);
            }
            i++;
        }
    }
    FastLED.show();
}

void PutByMatrix()
{
    int i = 0;
    int border = (int) NUM_ROWS / 3;
    int pixel;
    for (int c = 0; c < NUM_COLS; c++)
    {
        for (int r = 0; r < NUM_ROWS; r++)
        {
            if ((c % 2) == 0)
            {
                pixel = matrix[r][c];
            }
            else
            {
                pixel = matrix[(NUM_ROWS - 1) - r][c];
            }

            if (r >= (border*2) || r < border) 
            {
                leds[i] = CRGB(pixel, pixel, pixel);
            }
            else
            {
                leds[i] = CHSV(4, 247, pixel); //CRGB(pixel,0,0)
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
    EEPROM.updateByte(5, turnoffTimer);
}
void read_eeprom()
{
    mode = EEPROM.readByte(1);
    flame_palette = EEPROM.readByte(2);
    lamp_hue = EEPROM.readByte(3);
    lamp_saturation = EEPROM.readByte(4);
    turnoffTimer = EEPROM.readByte(5);
}
void eeprom_timer()
{
    if ((eepromTime > 0) && ((millis() - eepromTime) > 3000))
    {
        eepromTime = 0;
        if (EEPROM_SETTINGS)
        {
            write_eeprom();
        }
    }
}

CRGB nonlinearEnergy( byte energy )
{
    const uint8_t energymap[32] = {0, 64, 96, 112, 128, 144, 152, 160, 168, 176, 184, 184, 192, 200, 200, 208, 208, 216, 216, 224, 224, 224, 232, 232, 232, 240, 240, 240, 240, 248, 248, 248};
    byte eb = energymap[energy >> 3];
    byte r = red_bias;
    byte g = green_bias;
    byte b = blue_bias;
    int red_energy = 180;
    int green_energy = 20; // 145;
    int blue_energy = 0;

    r = qadd8(r, (eb * red_energy) >> 8);
    g = qadd8(g, (eb * green_energy) >> 8);
    b = qadd8(b, (eb * blue_energy) >> 8);
    return CRGB(r, g, b);
}

