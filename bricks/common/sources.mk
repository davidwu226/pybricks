# Ring buffer

LWRB_SRC_C = lib/lwrb/src/lwrb/lwrb.c

# Contiki

CONTIKI_SRC_C = $(addprefix lib/contiki-core/,\
	lib/list.c \
	lib/memb.c \
	lib/ringbuf.c \
	sys/autostart.c \
	sys/etimer.c \
	sys/process.c \
	sys/timer.c \
	)

# Pybricks modules

PYBRICKS_PYBRICKS_SRC_C = $(addprefix pybricks/,\
	common/pb_type_battery.c \
	common/pb_type_charger.c \
	common/pb_type_colorlight_external.c \
	common/pb_type_colorlight_internal.c \
	common/pb_type_control.c \
	common/pb_type_dcmotor.c \
	common/pb_type_imu.c \
	common/pb_type_keypad.c \
	common/pb_type_lightarray.c \
	common/pb_type_lightmatrix_fonts.c \
	common/pb_type_lightmatrix.c \
	common/pb_type_logger.c \
	common/pb_type_motor.c \
	common/pb_type_speaker.c \
	common/pb_type_system.c \
	experimental/pb_module_experimental.c \
	geometry/pb_module_geometry.c \
	geometry/pb_type_matrix.c \
	hubs/pb_module_hubs.c \
	hubs/pb_type_cityhub.c \
	hubs/pb_type_essentialhub.c \
	hubs/pb_type_technichub.c \
	hubs/pb_type_movehub.c \
	hubs/pb_type_primehub.c \
	iodevices/pb_module_iodevices.c \
	iodevices/pb_type_iodevices_analogsensor.c \
	iodevices/pb_type_iodevices_ev3devsensor.c \
	iodevices/pb_type_iodevices_i2cdevice.c \
	iodevices/pb_type_iodevices_lumpdevice.c \
	iodevices/pb_type_iodevices_lwp3device.c \
	iodevices/pb_type_iodevices_pupdevice.c \
	iodevices/pb_type_iodevices_uartdevice.c \
	media/pb_module_media.c \
	parameters/pb_type_icon.c \
	parameters/pb_module_parameters.c \
	parameters/pb_type_button.c \
	parameters/pb_type_color.c \
	parameters/pb_type_direction.c \
	parameters/pb_type_port.c \
	parameters/pb_type_side.c \
	parameters/pb_type_stop.c \
	pupdevices/pb_module_pupdevices.c \
	pupdevices/pb_type_pupdevices_colordistancesensor.c \
	pupdevices/pb_type_pupdevices_colorlightmatrix.c \
	pupdevices/pb_type_pupdevices_colorsensor.c \
	pupdevices/pb_type_pupdevices_forcesensor.c \
	pupdevices/pb_type_pupdevices_infraredsensor.c \
	pupdevices/pb_type_pupdevices_light.c \
	pupdevices/pb_type_pupdevices_pfmotor.c \
	pupdevices/pb_type_pupdevices_remote.c \
	pupdevices/pb_type_pupdevices_tiltsensor.c \
	pupdevices/pb_type_pupdevices_ultrasonicsensor.c \
	pybricks.c \
	robotics/pb_module_robotics.c \
	robotics/pb_type_drivebase.c \
	robotics/pb_type_spikebase.c \
	tools/pb_module_tools.c \
	tools/pb_type_stopwatch.c \
	util_mp/pb_obj_helper.c \
	util_mp/pb_type_enum.c \
	util_pb/pb_color_map.c \
	util_pb/pb_conversions.c \
	util_pb/pb_device_stm32.c \
	util_pb/pb_error.c \
	util_pb/pb_task.c \
	)

# Pybricks I/O library

PBIO_SRC_C = $(addprefix lib/pbio/,\
	drv/adc/adc_stm32_hal.c \
	drv/adc/adc_stm32f0.c \
	drv/battery/battery_adc.c \
	drv/block_device/block_device_flash_stm32.c \
	drv/block_device/block_device_w25qxx_stm32.c \
	drv/bluetooth/bluetooth_btstack_control_gpio.c \
	drv/bluetooth/bluetooth_btstack_run_loop_contiki.c \
	drv/bluetooth/bluetooth_btstack_uart_block_stm32_hal.c \
	drv/bluetooth/bluetooth_btstack.c \
	drv/bluetooth/bluetooth_init_cc2564C_1.4.c \
	drv/bluetooth/bluetooth_stm32_bluenrg.c \
	drv/bluetooth/bluetooth_stm32_cc2640.c \
	drv/bluetooth/pybricks_service_server.c \
	drv/button/button_gpio.c \
	drv/button/button_resistor_ladder.c \
	drv/charger/charger_mp2639a.c \
	drv/clock/clock_stm32.c \
	drv/core.c \
	drv/counter/counter_core.c \
	drv/counter/counter_lpf2.c \
	drv/counter/counter_stm32f0_gpio_quad_enc.c \
	drv/gpio/gpio_stm32f0.c \
	drv/gpio/gpio_stm32f4.c \
	drv/gpio/gpio_stm32l4.c \
	drv/imu/imu_lsm6ds3tr_c_stm32.c \
	drv/ioport/ioport_lpf2.c \
	drv/led/led_array_pwm.c \
	drv/led/led_array.c \
	drv/led/led_core.c \
	drv/led/led_dual.c \
	drv/led/led_pwm.c \
	drv/motor_driver/motor_driver_hbridge_pwm.c \
	drv/pwm/pwm_core.c \
	drv/pwm/pwm_lp50xx_stm32.c \
	drv/pwm/pwm_stm32_tim.c \
	drv/pwm/pwm_tlc5955_stm32.c \
	drv/reset/reset_stm32.c \
	drv/resistor_ladder/resistor_ladder.c \
	drv/sound/sound_stm32_hal_dac.c \
	drv/uart/uart_stm32f0.c \
	drv/uart/uart_stm32f4_ll_irq.c \
	drv/uart/uart_stm32l4_ll_dma.c \
	drv/usb/usb_stm32.c \
	drv/watchdog/watchdog_stm32.c \
	platform/$(PBIO_PLATFORM)/platform.c \
	src/angle.c \
	src/battery.c \
	src/color/conversion.c \
	src/control.c \
	src/control_settings.c \
	src/dcmotor.c \
	src/drivebase.c \
	src/error.c \
	src/integrator.c \
	src/iodev.c \
	src/light/animation.c \
	src/light/color_light.c \
	src/light/light_matrix.c \
	src/logger.c \
	src/main.c \
	src/math.c \
	src/motor_process.c \
	src/motor/servo_settings.c \
	src/observer.c \
	src/parent.c \
	src/protocol/lwp3.c \
	src/protocol/nus.c \
	src/protocol/pybricks.c \
	src/servo.c \
	src/tacho.c \
	src/task.c \
	src/trajectory.c \
	src/uartdev.c \
	src/util.c \
	sys/battery.c \
	sys/bluetooth.c \
	sys/command.c \
	sys/core.c \
	sys/hmi.c \
	sys/io_ports.c \
	sys/light_matrix.c \
	sys/light.c \
	sys/main.c \
	sys/program_load.c \
	sys/program_stop.c \
	sys/status.c \
	sys/supervisor.c \
	)

# MicroPython math library

SRC_LIBM = $(addprefix lib/libm/,\
	acoshf.c \
	asinfacosf.c \
	asinhf.c \
	atan2f.c \
	atanf.c \
	atanhf.c \
	ef_rem_pio2.c \
	ef_sqrt.c \
	erf_lgamma.c \
	fmodf.c \
	kf_cos.c \
	kf_rem_pio2.c \
	kf_sin.c \
	kf_tan.c \
	log1pf.c \
	math.c \
	nearbyintf.c \
	sf_cos.c \
	sf_erf.c \
	sf_frexp.c \
	sf_ldexp.c \
	sf_modf.c \
	sf_sin.c \
	sf_tan.c \
	wf_lgamma.c \
	wf_tgamma.c \
	)
