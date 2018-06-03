#include <ClickEncoder.h>
#include <TimerOne.h>
#include <FastLED.h>

#define ENC_HALFSTEP 0

#define MODE_COUNT 2

#define LED_PIN 4
#define COLOR_ORDER GRB
#define CHIPSET WS2811
#define NUM_ROWS 15
#define NUM_COLS 2
#define NUM_LEDS (NUM_ROWS * NUM_COLS)
#define FRAMES_PER_SECOND 60

#define button_pin 5
#define enup_pin 7
#define endown_pin 6

#define petentiometer_pin A1

CRGB leds[NUM_LEDS];

DEFINE_GRADIENT_PALETTE(flame_palette_fire){
    0, 0, 0, 0,          //black
    168, 255, 0, 0,      //red
    224, 255, 250, 0,    //bright yellow
    255, 255, 255, 240}; //full white
DEFINE_GRADIENT_PALETTE(flame_palette_blue){
    0, 0, 0, 0, //black
    90, 9, 9, 91,
    172, 10, 29, 157,
    210, 3, 59, 180,
    255, 166, 194, 252};
DEFINE_GRADIENT_PALETTE(flame_palette_hotwarm){
    0, 0, 0, 0, //black
    40, 25, 0, 154,
    80, 235, 46, 166,
    127, 231, 5, 71,
    170, 246, 150, 12,
    211, 205, 85, 46,
    255, 255, 255,255}; //full white
DEFINE_GRADIENT_PALETTE(flame_palette_green){
    0, 0, 0, 0, //black
    127, 23, 94, 0,
    220, 62, 210, 10,  //bright yellow
    255, 22, 253, 98}; //full white
DEFINE_GRADIENT_PALETTE(flame_palette_violette){
    0, 0, 0, 0, //black
    85, 101, 3, 91,
    164, 190, 8, 250,   //bright yellow
    255, 250, 70, 250}; //full white

CRGBPalette32 current_flame_palette = flame_palette_fire;

byte matrix[NUM_ROWS][NUM_COLS];

ClickEncoder *encoder;
int16_t last, value;

int brightness = 512;

// controls state
int mode = 0;
int hold = 0;

// Mode specific params
// 0: flame palette
int flame_palette = 0;
int flame_dissipation = 70;

// 1: static lamp
int lamp_hue = 1;
int lamp_saturation = 200;

unsigned long time, renderTime;

void timerIsr()
{
    encoder->service();
}

void setup()
{
    delay(3000); // sanity delay
    FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(brightness >> 2);

    Serial.begin(9600);
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
    int raw = encoder->getValue();

    int newBrightness = analogRead(petentiometer_pin);
    if (newBrightness!=brightness) {
        brightness = newBrightness;
        FastLED.setBrightness(brightness >> 2);
    }

    if ((value+raw) != last )
    {
        value += raw;
        last = value;
        switch (mode) {
            case 0:
                if ((millis()-time)>430) {
                    time = millis();
                    if (hold)
                    {
                        flame_dissipation += raw * 2;
                        if (flame_dissipation < 1)
                        {
                            flame_dissipation = 1;
                        }
                    }
                    else
                    {
                        if (raw > 0)
                        {
                            flame_palette++;
                        }
                        if (raw < 0)
                        {
                            flame_palette--;
                        }
                        if (flame_palette > 4)
                        {
                            flame_palette = 0;
                        }
                        if (flame_palette < 0)
                        {
                            flame_palette = 4;
                        }
                        switch (flame_palette)
                        {
                        case 0:
                            current_flame_palette = flame_palette_fire;
                            break;
                        case 1:
                            current_flame_palette = flame_palette_blue;
                            break;
                        case 2:
                            current_flame_palette = flame_palette_hotwarm;
                            break;
                        case 3:
                            current_flame_palette = flame_palette_green;
                            break;
                        case 4:
                            current_flame_palette = flame_palette_violette;
                            break;
                        }
                    }
                }
                Serial.print(" RAW: ");
                Serial.print(raw);
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
                break;
            case 1:
                if (hold)
                {
                    lamp_saturation += raw * 2;
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
                    lamp_hue += raw * 2;
                    if (lamp_hue < 0)
                    {
                        lamp_hue = 255 ;
                    }
                    else if (lamp_hue > 255)
                    {
                        lamp_hue = 0;
                    }
                }
                Serial.print(" RAW: ");
                Serial.print(raw);
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

                break;
        }
        
    }

    ClickEncoder::Button b = encoder->getButton();
    if (b != ClickEncoder::Open)
    {
        switch (b) {
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
                if (mode >= MODE_COUNT) {
                    mode = 0;
                }
                break;
            case ClickEncoder::DoubleClicked:
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
            leds[i] = ColorFromPalette(current_flame_palette, matrix[r][c]);
            i++;
        }
        //remove. debug
        i++;
    }
    FastLED.show();
}
