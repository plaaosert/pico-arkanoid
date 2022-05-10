add_executable(arkanoid
        arkanoid.c
        external/fonts.c
        external/st7735.c
        external/DEV_Config.c
	external/ICM20948.c
        )

# pull in common dependencies
target_link_libraries(
  arkanoid
  hardware_pwm
  hardware_pio
  hardware_spi
  hardware_i2c
  pico_stdlib
)

pico_enable_stdio_usb(arkanoid 1)
pico_enable_stdio_uart(arkanoid 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(arkanoid)

# add url via pico_set_program_url
example_auto_set_url(arkanoid)
