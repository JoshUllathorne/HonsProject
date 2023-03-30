/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Fade an LED between low and high brightness. An interrupt handler updates
// the PWM slice's output level each time the counter wraps.

#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/time.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"

#define LED_PIN 15

#define TABLE_SIZE 512

#define A110 390
#define A 0
#define As 24
#define B 48
#define C 64
#define Cs 82
#define D 100
#define Ds 116
#define E 132
#define F 148
#define Fs 164
#define G 172
#define Gs 187


int octave = 8; // 1 = 110hz    2 = 220hz    4 = 440hz    8 = 880hz       
int notePlayed = A; 

int sinVal[] = {
0,2,4,6,8,8,10,12,
14,14,16,18,20,22,22,24,
26,28,30,30,32,34,36,36,
38,40,42,44,44,46,48,50,
50,52,54,56,58,58,60,62,
64,66,66,68,70,72,72,74,
76,78,80,80,82,84,86,86,
88,90,92,92,94,96,98,100,
100,102,104,106,106,108,110,112,
112,114,116,118,120,120,122,124,
126,126,128,130,132,132,134,136,
138,138,140,142,144,144,146,148,
150,150,152,154,156,156,158,160,
162,162,164,166,168,168,170,172,
174,174,176,178,180,180,182,184,
184,186,188,190,190,192,194,196,
196,198,200,202,202,204,206,206,
208,210,212,212,214,216,216,218,
220,222,222,224,226,226,228,230,
230,232,234,236,236,238,240,240,
242,244,244,246,248,248,250,252,
254,254,256,258,258,260,262,262,
264,266,266,268,270,270,272,274,
274,276,278,278,280,282,282,284,
286,286,288,290,290,292,292,294,
296,296,298,300,300,302,304,304,
306,306,308,310,310,312,314,314,
316,316,318,320,320,322,322,324,
326,326,328,330,330,332,332,334,
336,336,338,338,340,340,342,344,
344,346,346,348,350,350,352,352,
354,354,356,358,358,360,360,362,
362,364,364,366,368,368,370,370,
372,372,374,374,376,376,378,378,
380,380,382,384,384,386,386,388,
388,390,390,392,392,394,394,396,
396,398,398,400,400,402,402,404,
404,406,406,406,408,408,410,410,
412,412,414,414,416,416,418,418,
420,420,420,422,422,424,424,426,
426,428,428,428,430,430,432,432,
432,434,434,436,436,438,438,438,
440,440,442,442,442,444,444,446,
446,446,448,448,448,450,450,452,
452,452,454,454,454,456,456,456,
458,458,460,460,460,462,462,462,
464,464,464,466,466,466,468,468,
468,470,470,470,470,472,472,472,
474,474,474,476,476,476,476,478,
478,478,480,480,480,480,482,482,
482,482,484,484,484,484,486,486,
486,486,488,488,488,488,490,490,
490,490,492,492,492,492,492,494,
494,494,494,494,496,496,496,496,
496,498,498,498,498,498,500,500,
500,500,500,500,502,502,502,502,
502,502,502,504,504,504,504,504,
504,504,506,506,506,506,506,506,
506,506,508,508,508,508,508,508,
508,508,508,508,508,510,510,510,
510,510,510,510,510,510,510,510,
510,510,510,510,512,512,512,512,
512,512,512,512,512,512,512,512,
512,512,512,512,512,512,512,512,
};

//values for a quarter of a sin wave
int sinValueOld[] = {
0,0,4,4,6,6,10,10,12,12,16,16,18,18,22,22,26,26,28,28,
32,32,34,34,38,38,40,40,44,44,48,48,50,50,54,54,56,56,
60,60,62,62,66,66,68,68,72,72,76,76,78,78,82,82,84,84,
88,88,90,90,94,94,96,96,100,100,102,102,106,106,110,110,
112,112,116,116,118,118,122,122,124,124,128,128,130,130,
134,134,136,136,140,140,142,142,146,146,148,148,152,152,
154,154,158,158,160,160,164,164,166,166,170,170,172,172,
176,176,178,178,182,182,184,184,188,188,190,190,194,194,
196,196,198,198,202,202,204,204,208,208,210,210,214,214,
216,216,218,218,222,222,224,224,228,228,230,230,234,234,
236,236,238,238,242,242,244,244,246,246,250,250,252,252,
256,256,258,258,260,260,264,264,266,266,268,268,272,272,
274,274,276,276,280,280,282,282,284,284,288,288,290,290,
292,292,294,294,298,298,300,300,302,302,304,304,308,308,
310,310,312,312,314,314,318,318,320,320,322,322,324,324,
328,328,330,330,332,332,334,334,336,336,340,340,342,342,
344,344,346,346,348,348,350,350,354,354,356,356,358,358,
360,360,362,362,364,364,366,366,368,368,370,370,372,372,
376,376,378,378,380,380,382,382,384,384,386,386,388,388,
390,390,392,392,394,394,396,396,398,398,400,400,402,402,
404,404,406,406,408,408,410,410,412,412,414,414,414,414,
416,416,418,418,420,420,422,422,424,424,426,426,428,428,
430,430,430,430,432,432,434,434,436,436,438,438,440,440,
440,440,442,442,444,444,446,446,448,448,448,448,450,450,
452,452,454,454,454,454,456,456,458,458,458,458,460,460,
462,462,462,462,464,464,466,466,466,466,468,468,470,470,
470,470,472,472,474,474,474,474,476,476,476,476,478,478,
478,478,480,480,482,482,482,482,484,484,484,484,486,486,
486,486,488,488,488,488,490,490,490,490,490,490,492,492,
492,492,494,494,494,494,496,496,496,496,496,496,498,498,
498,498,498,498,500,500,500,500,500,500,502,502,502,502,
502,502,504,504,504,504,504,504,504,504,506,506,506,506,
506,506,506,506,508,508,508,508,508,508,508,508,508,508,
510,510,510,510,510,510,510,510,510,510,510,510,510,510,
512,512,512,512,512,512,512,512,512,512,512,512,512,512,
512,512,512,512,512,512,
};

#ifdef LED_PIN
void on_pwm_wrap() {
    static int fade = (TABLE_SIZE) - 1;
    static bool going_up = true;
    int newVal;
    float multiplier = 0.01f;
    int sinWave = 1;
    // Clear the interrupt flag that brought us here
    pwm_clear_irq(pwm_gpio_to_slice_num(LED_PIN));

    if (going_up) {
        ++fade;
        if (fade > (TABLE_SIZE * 2)) {
            fade = (TABLE_SIZE * 2);
            going_up = false;
        }
    } else {
        --fade;
        if (fade < 0) {
            fade = 0;
            going_up = true;
        }
    }

    if (sinWave == 1)
    {
        if (fade >= TABLE_SIZE && fade <= (2 * TABLE_SIZE) && going_up)
        {
            newVal = TABLE_SIZE + sinVal[fade - TABLE_SIZE];
            pwm_set_gpio_level(LED_PIN, newVal * multiplier);
            //printf("%d: %d\n", fade, newVal);
        } else if (fade >= TABLE_SIZE && fade < (2 * TABLE_SIZE) && !going_up)
        {
            newVal = TABLE_SIZE + sinVal[fade - TABLE_SIZE];
            pwm_set_gpio_level(LED_PIN, newVal * multiplier);
            //printf("%d: %d\n", fade, newVal);
        } else if (fade < TABLE_SIZE && fade >= 0 && !going_up)
        {
            newVal = TABLE_SIZE - sinVal[TABLE_SIZE - fade];
            pwm_set_gpio_level(LED_PIN, newVal * multiplier);
            //printf("%d: %d\n", fade, newVal);
        } else if (fade < TABLE_SIZE && fade > 0 && going_up)
        {
            newVal = TABLE_SIZE - sinVal[TABLE_SIZE - fade];
            pwm_set_gpio_level(LED_PIN, newVal * multiplier);
            //printf("%d: %d\n", fade, newVal);
        }
    } else {
        pwm_set_gpio_level(LED_PIN, fade * multiplier);
    }
}
#endif

int main() {
#ifndef LED_PIN
#warning pwm/led_fade example requires a board with a regular LED
#else
    set_sys_clock_khz(174000, true); 
    stdio_init_all();
    // Tell the LED pin that the PWM is in charge of its value.
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
    // Figure out which slice we just connected to the LED pin
    uint slice_num = pwm_gpio_to_slice_num(LED_PIN);

    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // Get some sensible defaults for the slice configuration. By default, the
    // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config config = pwm_get_default_config();
    // Set divider, reduces counter clock to sysclock/this value
    pwm_config_set_clkdiv(&config, 2.f);

    pwm_config_set_wrap(&config, ((A110 - notePlayed) / octave));

    // Load the configuration into our PWM slice, and set it running.
    pwm_init(slice_num, &config, true);

    // Everything after this point happens in the PWM interrupt handler, so we
    // can twiddle our thumbs
    while (1)
        tight_loop_contents();
#endif
}
