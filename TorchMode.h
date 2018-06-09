// Slightly modified version of the fire pattern from MessageTorch by Lukas Zeller:
// https://github.com/plan44/messagetorch

// The MIT License (MIT)

// Copyright (c) 2014 Lukas Zeller

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// torch parameters

#include "globals.h"
#include <FastLED.h>

byte flame_min = 100; // 0..255
byte flame_max = 220; // 0..255

byte random_spark_probability = 2; // 0..100
byte spark_min = 200;              // 0..255
byte spark_max = 255;              // 0..255

byte spark_tfr = 40;      // 0..256 how much energy is transferred up for a spark per cycle
uint16_t spark_cap = 200; // 0..255: spark cells: how much energy is retained from previous cycle

uint16_t up_rad = 40;   // up radiation
uint16_t side_rad = 35; // sidewards radiation
uint16_t heat_cap = 0;  // 0..255: passive cells: how much energy is retained from previous cycle

byte red_bg = 0;
byte green_bg = 0;
byte blue_bg = 0;
byte red_bias = 10;
byte green_bias = 0;
byte blue_bias = 0;
int red_energy = 180;
int green_energy = 20; // 145;
int blue_energy = 0;

byte upside_down = 0; // if set, flame (or rather: drop) animation is upside down. Text remains as-is

// torch mode
// ==========

byte nextEnergy[NUM_ROWS][NUM_COLS]; // next energy level
byte energyMode[NUM_ROWS][NUM_COLS]; // mode how energy is calculated for this point

enum
{
    torch_passive = 0,    // just environment, glow from nearby radiation
    torch_nop = 1,        // no processing
    torch_spark = 2,      // slowly looses energy, moves up
    torch_spark_temp = 3, // a spark still getting energy from the level below
};

inline void reduce(byte &aByte, byte aAmount, byte aMin = 0)
{
    int r = aByte - aAmount;
    if (r < aMin)
        aByte = aMin;
    else
        aByte = (byte)r;
}

inline void increase(byte &aByte, byte aAmount, byte aMax = 255)
{
    int r = aByte + aAmount;
    if (r > aMax)
        aByte = aMax;
    else
        aByte = (byte)r;
}

uint16_t random2(uint16_t aMinOrMax, uint16_t aMax = 0)
{
    if (aMax == 0)
    {
        aMax = aMinOrMax;
        aMinOrMax = 0;
    }
    uint32_t r = aMinOrMax;
    aMax = aMax - aMinOrMax + 1;
    r += rand() % aMax;
    return r;
}

void resetEnergy( )
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        byte y = i / NUM_COLS;
        byte x = i - (NUM_COLS*y);
        matrix[y][x] = 0; 
        nextEnergy[y][x] = 0;
        energyMode[y][x] = torch_passive;
    }
}

void calcNextEnergy()
{
    for (byte x = 0; x < NUM_COLS; x++)
    {
        for (byte y = 0; y < NUM_ROWS; y++)  
        {
            byte e = matrix[y][x]; 
            byte m = energyMode[y][x];
            switch (m)
            {
            case torch_spark:
            {
                // loose transfer up energy as long as the is any
                reduce(e, spark_tfr);
                // cell above is temp spark, sucking up energy from this cell until empty
                if (y < NUM_ROWS - 1)
                {
                    energyMode[y+1][x] = torch_spark_temp;
                }
                break;
            }
            case torch_spark_temp:
            {
                // just getting some energy from below
                byte e2 =  matrix[y-1][x]; //currentEnergy[i - NUM_COLS];
                if (e2 < spark_tfr)
                {
                    // cell below is exhausted, becomes passive
                    energyMode[y-1][x] = torch_passive;
                    // gobble up rest of energy
                    increase(e, e2);
                    // loose some overall energy
                    e = ((int)e * spark_cap) >> 8;
                    // this cell becomes active spark
                    energyMode[y][x] = torch_spark;
                }
                else
                {
                    increase(e, spark_tfr);
                }
                break;
            }
            case torch_passive:
            {
                e = ((int)e * heat_cap) >> 8;
//                 increase(e, ((((int)currentEnergy[i - 1] + (int)currentEnergy[i + 1]) * side_rad) >> 9) + (((int)currentEnergy[i - NUM_COLS] * up_rad) >> 8));
                increase(e, ((((int)matrix[y][x-1] + (int)matrix[y][x+1]) * side_rad) >> 9) + (((int)matrix[y-1][x] * up_rad) >> 8));
            }
            default:
                break;
            }
            //nextEnergy[i++] = e;
/*            if ((x+1)<NUM_COLS) {
                nextEnergy[y][x+1] = e;
            } else if ((y+1) < NUM_ROWS ) {
                nextEnergy[y+1][0] = e;
            }
 */           nextEnergy[y][x] = e;
        }
    }
}

const uint8_t energymap[32] = {0, 64, 96, 112, 128, 144, 152, 160, 168, 176, 184, 184, 192, 200, 200, 208, 208, 216, 216, 224, 224, 224, 232, 232, 232, 240, 240, 240, 240, 248, 248, 248};

void calcNextColors( )
{
    int ei = 0; // index in led
    int ee = 0;
    for (byte x = 0; x < NUM_COLS; x++)
    {
        for (byte y = 0; y < NUM_ROWS; y++)
        {
            byte yi; // index into energy calculation buffer
            
            if (upside_down)
                yi = (NUM_ROWS-1) - y;
            else
                yi = y;

            if ((x % 2) == 0)
            {
                ee = ei;
            } else {
                ee = ei - y + (NUM_ROWS - 1) - y;
            }

            uint16_t e = nextEnergy[yi][x];
            matrix[yi][x] = e; // currentEnergy[ei] = e;

            if (e > 250)
                leds[ee] = CRGB(170, 170, e); // blueish extra-bright spark
            else
            {
                if (e > 0)
                {
                    // energy to brightness is non-linear
                    byte eb = energymap[e >> 3];
                    byte r = red_bias;
                    byte g = green_bias;
                    byte b = blue_bias;
                    increase(r, (eb * red_energy) >> 8);
                    increase(g, (eb * green_energy) >> 8);
                    increase(b, (eb * blue_energy) >> 8);
                    leds[ee] = CRGB(r, g, b);
                }
                else
                {
                    // background, no energy
                    leds[ee] = CRGB(red_bg, green_bg, blue_bg);
                }
            }

            ei++;
        }
    }

}

void injectRandom()
{
    // random flame energy at bottom row
    for (byte x = 0; x < NUM_COLS; x++)
    {
//        currentEnergy[i] = random2(flame_min, flame_max);
        matrix[0][x] = random2(flame_min, flame_max);
        energyMode[0][x] = torch_nop;
    }
    // random sparks at second row
    for (byte x = 0; x < NUM_COLS; x++)
    {
        if (energyMode[1][x] != torch_spark && random2(100) < random_spark_probability)
        {
            // currentEnergy[i] = random2(spark_min, spark_max);
            matrix[1][x] = random2(spark_min, spark_max);
            energyMode[1][x] = torch_spark;
        }
    }
}

uint16_t torch( )
{
    injectRandom();
    calcNextEnergy();
    calcNextColors();



        return 1;
}
