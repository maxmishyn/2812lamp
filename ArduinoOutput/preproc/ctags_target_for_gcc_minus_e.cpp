# 1 "c:\\Users\\maxim\\Documents\\DIY\\led-lava-lamp\\2812lamp\\2812lamp.ino"
# 1 "c:\\Users\\maxim\\Documents\\DIY\\led-lava-lamp\\2812lamp\\2812lamp.ino"
# 2 "c:\\Users\\maxim\\Documents\\DIY\\led-lava-lamp\\2812lamp\\2812lamp.ino" 2
# 3 "c:\\Users\\maxim\\Documents\\DIY\\led-lava-lamp\\2812lamp\\2812lamp.ino" 2
# 4 "c:\\Users\\maxim\\Documents\\DIY\\led-lava-lamp\\2812lamp\\2812lamp.ino" 2
# 24 "c:\\Users\\maxim\\Documents\\DIY\\led-lava-lamp\\2812lamp\\2812lamp.ino"
CRGB leds[(15 * 15)];

# 27 "c:\\Users\\maxim\\Documents\\DIY\\led-lava-lamp\\2812lamp\\2812lamp.ino" 2

CRGBPalette32 current_flame_palette = flame_palette_fire;

byte matrix[15][15];

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
    delay(500); // sanity delay

    FastLED.setMaxPowerInVoltsAndMilliamps(5, 2000); // V, mA

    FastLED.addLeds<WS2811, 4, GRB>(leds, (15 * 15)).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(brightness >> 2);


    Serial.begin(9600);
    pinMode(7, 0x2);
    pinMode(6, 0x2);
    pinMode(5, 0x0);

    encoder = new ClickEncoder(7, 6, 5);

    Timer1.initialize(1000);
    Timer1.attachInterrupt(timerIsr);

    last = -1;
}

void loop()
{
    int raw = encoder->getValue();

    int newBrightness = analogRead(A1);
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
                if (mode >= 2) {
                    mode = 0;
                }
                break;
            case ClickEncoder::DoubleClicked:
                break;
        }
    }

    switch (mode) {
        case 0:
            if ((millis() - renderTime) >= (1000 / 60)) {
                Fire2012();
                PutMatrix();
                renderTime = millis();
                //            FastLED.delay(1000 / FRAMES_PER_SECOND);
            }
            break;
        case 1:
            fill_solid(leds, (15 * 15), CHSV(lamp_hue, lamp_saturation, brightness>>2));
            FastLED.show();
            break;
        }
}


void Fire2012()
{
    // Step 1.  Cool down every cell a little
    for (int j = 0; j < 15; j++)
    {
        for (int i = 0; i < 15; i++)
        {
            matrix[i][j] = qsub8(matrix[i][j], random8(0, ((flame_dissipation * 10) / 15) + 2));
        }
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int j = 0; j < 15; j++)
    {
        for (int k = 15 - 1; k > 2; k--)
        {
            matrix[k][j] = (matrix[k - 1][j] + matrix[k - 2][j] + matrix[k - 2][j]) / 3;
        }
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    for (int j = 0; j < 15; j++)
    {
        if (random8() < 130) {
            int y = random8(7);
            matrix[y][j] = qadd8(matrix[y][j], random8(160, 255));
        }
    }
}

void PutMatrix()
{
    int i = 0;
    for (int c = 0; c < 15; c++)
    {
        for (int r = 0; r < 15; r++)
        {
            if ( c % 2 ) {
                leds[i] = ColorFromPalette(current_flame_palette, matrix[r][c]);
            } else {
                leds[i] = ColorFromPalette(current_flame_palette, matrix[(15 -1)-r][c]);
            }
            i++;
        }
        //remove. debug
        i++;
    }
    FastLED.show();
}
