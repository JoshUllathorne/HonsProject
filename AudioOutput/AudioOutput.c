#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/time.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include <math.h>

#define AUDIO_PIN 15
#define BUTTON_PIN 28

#define MAX_DUTY_CYCLE 54
#define MIN_DUTY_CYCLE 0

#define PI 3.14159265358979323846

int sinTable[MAX_DUTY_CYCLE];

int map(int x, int in_min, int in_max, int out_min, int out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// creates a quarter of a sin table to be used in lookup table
void createSinTable(int tableSize)
{
    int val;
    for (int i = 0; i < tableSize / 2 + 1; i++)
    {
        val = tableSize / 2 * sin(i * PI / (tableSize));
        sinTable[i] = (int)val;
        // printf("%d,\n", sinTable[i]);
    }
}

// pwm interrupt handler
#ifdef AUDIO_PIN
void on_pwm_wrap()
{
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN)); // clear the interrupt flag that brought us here

    static int duty = 127;         // starting value for wave
    static bool increasing = true; // tracks if the current duty value should increase or decrease during this interrupt
    static int sinVal;             // stores the corresponding sine value for the current duty cycle
    static bool released = true;
    static int wave = 0;            //0 - sine wave     1 - triangle wave       2 - square

    // creates a triangle wave between 0 - maximum duty cycle
    if (increasing)
    {
        ++duty;
        if (duty > MAX_DUTY_CYCLE)
        {
            duty = MAX_DUTY_CYCLE;
            increasing = false;
        }
    }
    else
    {
        --duty;
        if (duty < 0)
        {
            duty = 0;
            increasing = true;
        }
    }

    // if duty is between 128 and 255
    if (duty > MAX_DUTY_CYCLE / 2 && increasing)
    {
        // add 128 to corresponding sinValue (-128)
        sinVal = sinTable[duty - MAX_DUTY_CYCLE / 2] + MAX_DUTY_CYCLE / 2;
        // printf("%d: %d\n", duty, sinVal);
    }

    // if duty is between 255 and 128
    if (duty > MAX_DUTY_CYCLE / 2 && !increasing)
    {
        // mirror LUT horizontally (go through array backwards from i = 127 to i = 0) and add 128 to sinValue
        sinVal = sinTable[duty - MAX_DUTY_CYCLE / 2] + MAX_DUTY_CYCLE / 2;
        // printf("%d: %d\n", duty, sinVal);
    }

    // if duty is between 127 and 0
    if (duty <= MAX_DUTY_CYCLE / 2 && !increasing)
    {
        // mirror LUT vertically (take 127 and minus corresponding value in array)
        sinVal = MAX_DUTY_CYCLE / 2 - sinTable[MAX_DUTY_CYCLE / 2 - duty];
        // printf("%d: %d\n", duty, sinVal);
    }

    // if duty is between 0 and 127
    if (duty <= MAX_DUTY_CYCLE / 2 && increasing)
    {
        // mirror LUT horizontally and vertically (go through array backwards from i = 127 to 0 and table[i] - 127 each value)
        sinVal = MAX_DUTY_CYCLE / 2 - sinTable[MAX_DUTY_CYCLE / 2 - duty];
        // printf("%d: %d\n", duty, sinVal);
    }

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_down(BUTTON_PIN);

    if(released)
    {
        if(gpio_get(BUTTON_PIN))
        {
            released = false;
            if (wave == 2) {
                wave = 0;
            } else {
                wave++;
            }
        }
    }

    if(!gpio_get(BUTTON_PIN))
    {
        released = true;
    }

    switch (wave)
    {
        case 0: 
            pwm_set_gpio_level(AUDIO_PIN, sinVal); // outputs sin wave 
            break;
        case 1:
            pwm_set_gpio_level(AUDIO_PIN, duty); // outputs triangle wave
            break;
        case 2:
            if (duty > MAX_DUTY_CYCLE/2) {
                pwm_set_gpio_level(AUDIO_PIN, MAX_DUTY_CYCLE * 0.25f);
            } else {
                pwm_set_gpio_level(AUDIO_PIN, 0);
            }
    }

}
#endif

int main()
{
    uint16_t result; // stores the value from the ADC input

    createSinTable(MAX_DUTY_CYCLE);

    stdio_init_all();
    set_sys_clock_khz(170000, true);

    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);
    
    // initialise ADC inputs
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);

    // set up the PWM interrupt
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    pwm_config config = pwm_get_default_config();

    // set the initial frequency the interrupt is called
    pwm_config_set_clkdiv_int_frac(&config, 255, 1);
    pwm_config_set_wrap(&config, MAX_DUTY_CYCLE);
    pwm_init(slice_num, &config, true);

    pwm_set_gpio_level(AUDIO_PIN, 0);


    while (1)
    {
        // take the current ADC value, maps it to be usable and smooths it using the average value from the last 50 reads
        int numReads = 50;
        int senseSum = 0;

        for (int i = 0; i < numReads; i++)
        {
            result = map(adc_read(), 0, 4095, 255, 24);
            senseSum += result;
        }
        int avg = senseSum / numReads;

        // adjust the clock divider to the value from ADC to modify the frequency of the wave being outputted
        pwm_set_clkdiv(slice_num, avg);

        // printf("%d\t%d\n", result, avg);
    }
}
