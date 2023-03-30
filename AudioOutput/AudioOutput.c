#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/time.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include <math.h>

#define AUDIO_PIN 15

#define MAX_DUTY_CYCLE 127
#define MIN_DUTY_CYCLE 0

#define PI 3.14159265358979323846

int sinTable[MAX_DUTY_CYCLE];

int map(int x, int in_min, int in_max, int out_min, int out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void createSinTable(int tableSize)
{
    int val;
    for (int i = 0; i < tableSize / 2; i++)
    {
        val = tableSize / 2 * sin(i * PI / (tableSize));
        sinTable[i] = (int)val;
        printf("%d,\n", sinTable[i]);
    }
}

#ifdef AUDIO_PIN
void on_pwm_wrap()
{
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN)); // clear the interrupt flag that brought us here

    static int duty = 127;          //starting value for wave
    static bool increasing = true;  //
    static int sinVal;
    static uint16_t result;
    
    if (increasing) {
        ++duty;
        if (duty > MAX_DUTY_CYCLE)
        {
            duty = MAX_DUTY_CYCLE;
            increasing = false;
        }
    } else {
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
        printf("%d: %d\n", duty, sinVal);
    }

    // if duty is between 255 and 128
    if (duty > MAX_DUTY_CYCLE / 2 && !increasing)
    {
        // mirror LUT across vertical (go through array backwards from i = 127 to i = 0) and add 128 to sinValue
        sinVal = sinTable[duty - MAX_DUTY_CYCLE / 2] + MAX_DUTY_CYCLE / 2;
        printf("%d: %d\n", duty, sinVal);
    }

    // if duty is between 127 and 0
    if (duty <= MAX_DUTY_CYCLE/2 && !increasing)
    {
        // mirror LUT across horizontal (take 127 and minus corresponding value in array)
        sinVal = MAX_DUTY_CYCLE/2 - sinTable[MAX_DUTY_CYCLE/2 - duty];
        printf("%d: %d\n", duty, sinVal);
    }

    // if duty is between 0 and 127
    if (duty <= MAX_DUTY_CYCLE/2 && increasing)
    {
        // mirror LUT across vertical and horizontal (go through array backwards from i = 127 to 0 and table[i] - 127 each value)
        sinVal = MAX_DUTY_CYCLE/2 - sinTable[MAX_DUTY_CYCLE/2 - duty];
        printf("%d: %d\n", duty, sinVal);
    }

    pwm_set_gpio_level(AUDIO_PIN, sinVal);
    //result = 64 + map(adc_read(), 0, 4095, 1, 128);
}
#endif

int main()
{
    createSinTable(MAX_DUTY_CYCLE);

    stdio_init_all();
    set_sys_clock_khz(172000, true);
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);

    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    pwm_config config = pwm_get_default_config();

    pwm_config_set_clkdiv_int(&config, 4.f);
    pwm_config_set_wrap(&config, MAX_DUTY_CYCLE);
    pwm_init(slice_num, &config, true);

    pwm_set_gpio_level(AUDIO_PIN, 0);

    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);

    while (1)
    {
        tight_loop_contents();
    }
}