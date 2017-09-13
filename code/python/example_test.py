import framework

serial_port = '/dev/ttyACM0'
end_time    = 20

test = framework.tester(serial_port)

test.pre_fan(5)
test.log_till_time(5)
test.heater_on()
test.LEDs_on(255, 50)

test.log_till_time(10)
test.mist_on_time(2)
test.flap_pulse_percent(2, 2, 100)

test.motor_new_pos(-50)
test.pause(2)
test.motor_get_pos()
test.motor_new_pos(50)
test.pause(2)

test.end_test(end_time)
test.cooldown_dryout()

test.plot()
test.pause(10)
test.save_and_close_figures()
test.shutdown()
