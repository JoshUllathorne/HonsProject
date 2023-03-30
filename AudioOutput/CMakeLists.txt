add_executable(AudioOutput
AudioOutput.c
        )

target_link_libraries(AudioOutput pico_stdlib hardware_gpio hardware_pwm hardware_adc)

# enable usb output, disable uart output
pico_enable_stdio_usb(AudioOutput 1)
pico_enable_stdio_uart(AudioOutput 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(AudioOutput)

# add url via pico_set_program_url
example_auto_set_url(AudioOutput)