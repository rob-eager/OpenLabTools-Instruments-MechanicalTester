"""Python framework for writing tests for the OpenLabTools Mechanical Tester.

This abstracts away a lot of the implementation details of the instrument and
allows separate files defining tests to be written at a high level keeping
them simple and readable.

Python code runs on the Raspberry Pi contained in the instrument.
"""

from __future__ import print_function

__author__ = 'Rob Eager'

import matplotlib.pyplot as plt
import os
import serial
import sys
import time


class Tester:
    """Single large class for the API.

    This manages comms to the arduino and creates plots of test runs.
    """

    fans_data = [0, ]
    fans_time = [0, ]
    heat_data = [0, ]
    heat_time = [0, ]
    mist_data = [0, ]
    mist_time = [0, ]
    flap_data = [0, ]
    flap_time = [0, ]

    time_data = []
    temp_data = []
    humidity_data = []

    first_reading = False
    discard_first = False

    def __init__(self, port, interval=1):
        """Start the test.

        Create log file and connect to the arduino.
        """
        self.port = port
        self.test_name = sys.argv[0].split('/')[-1].split('.')[0]
        self.shortest_interval = interval

        self.open_log()
        self.serial_connect()
        self.start = round(time.time(), 2)

    def pause(self, seconds):
        print('pausing for {} second(s)'.format(seconds))
        time.sleep(seconds)

    def save_and_close_figures(self):
        print('saving svg')
        plt.savefig(self.file_name + '.svg', bbox_inches='tight')
        print('saving png')
        plt.savefig(self.file_name + '.png', bbox_inches='tight')
        print('closing figures')
        plt.close('all')
        print('done')

    def pre_fan(self, seconds):
        print('running fans for {} seconds to normalise readings'.format(
            seconds))
        self.dev.write('FANS 1 1\r\n')
        self.dev.write('OPEN\r\n')
        self.LEDs_on(255, 50)
        self.pause(seconds)
        self.dev.write('CLOSE\r\n')
        self.start = round(time.time(), 2)
        self.fans_data += [1, ]
        self.fans_time += [0, ]

    def cooldown_dryout(self):
        print('running fans for cooldown')
        print('opening flap for dryout')
        self.dev.write('FANS 1 1\r\n')
        self.dev.write("OPEN\r\n")

    def shutdown(self, fans=False):
        print('shutdown test')
        self.dev.write("HEAT 1\r\n")
        if fans:
            self.dev.write("FANS 1 1\r\n")
            self.dev.write("OPEN\r\n")
        else:
            self.dev.write("FANS 0 0\r\n")
            self.dev.write("CLOSE\r\n")
        self.dev.write("MIST 0\r\n")
        self.LEDs_on(0, 50)
        self.dev.close()

    def open_log(self):
        if not os.path.isdir(self.test_name):
            os.mkdir(self.test_name)
        date = time.strftime('%d_%b_%Y', time.gmtime())
        number = 1
        self.file_name = self.test_name + '/' + self.test_name
        self.file_name += '_' + date + '_Run'
        while os.path.isfile(self.file_name + str(number) + '.log'):
            number += 1
        self.file_name += str(number)
        print('opening log file:   ', self.file_name + '.log')

        self.log = open(self.file_name + '.log', 'w')

    def serial_connect(self):
        print('opening serial port:', self.port)
        self.dev = serial.Serial(self.port, 115200)
        not_ready = True
        while not_ready:
            line = self.dev.readline()
            if line[-7:] == 'ready\r\n':
                not_ready = False
        print('arduino is ready')

    def leds_on(self, value, div):
        print('setting LEDs to', value)
        self.dev.write('LED %d %d\r\n' % (value, div))

    def fans_on(self):
        print('turning fans on')
        t = round(time.time() - self.start, 2)
        self.log.write(str(t) + ' FANS 1\n')

        self.dev.write('FANS 1 1\r\n')
        self.fans_data += [1, ]
        self.fans_time += [t, ]

    def fans_off(self):
        print('turning fans off')
        t = round(time.time() - self.start, 2)
        self.log.write(str(t) + ' FANS 0\n')

        self.dev.write('FANS 0 0\r\n')
        self.fans_data += [0, ]
        self.fans_time += [t, ]

    def mist_on_time(self, seconds):
        self.dev.write('MIST %d\r\n' % seconds)
        t = round(time.time() - self.start, 2)
        self.log.write(str(t) + ' MIST ' + str(seconds) + '\n')
        print('misting for %d second(s)' % seconds)
        self.mist_data += [1, ]
        self.mist_time += [t, ]
        self.mist_data += [0, ]
        self.mist_time += [t + seconds, ]

    def heater_on(self):
        print('turning heater on')
        t = round(time.time() - self.start, 2)
        self.log.write(str(t) + ' HEAT 1\n')

        self.dev.write('HEAT 0\r\n')
        self.heat_data += [1, ]
        self.heat_time += [t, ]

    def heater_off(self):
        print('turning heater off')
        t = round(time.time() - self.start, 2)
        self.log.write(str(t) + ' HEAT 0\n')

        self.dev.write('HEAT 1\r\n')
        self.heat_data += [0, ]
        self.heat_time += [t, ]

    def flap_open_percent(self, percent):
        t = round(time.time() - self.start, 2)
        self.log.write(str(t) + ' FLAP ' + str(percent) + '\n')

        self.dev.write('FLAP %d\r\n' % self.flap_convert_percent(percent))
        self.flap_data += [percent, ]
        self.flap_time += [t, ]

    def flap_pulse_percent(self, t1, t2, percent):
        t = round(time.time() - self.start, 2)
        self.log.write(str(t + t1) + ' FLAP ' + str(percent) + '\n')
        self.log.write(str(t + t1 + t2) + ' FLAP 0\n')

        self.dev.write('PULSE {} {} {}\r\n'.format(
            t1, t2, self.flap_convert_percent(percent)))
        self.flap_data += [percent, ]
        self.flap_time += [t + t1, ]
        self.flap_data += [0, ]
        self.flap_time += [t + t1 + t2, ]

    def flap_convert_percent(self, percent):
        if percent == 100:
            return 55
        elif percent == 0:
            return 148
        else:
            p = int((percent / 100.0) * 36)
            print('p =', p, 'flap value =', 136 - p)
            return 136 - p

    def flap_open(self):
        print('turning heater off')
        t = round(time.time() - self.start, 2)
        self.log.write(str(t) + ' FLAP 100\n')

        self.dev.write('OPEN\r\n')
        self.flap_data += [1, ]
        self.flap_time += [t, ]

    def mist_and_flap(self):
        start = round(time.time() - self.start)
        self.mist_on_time(2.5)
        self.flap_pulse_percent(3, 3, 100)
        self.log_till_time(start + 20)

    def motor_get_pos(self):
        self.dev.write('GET_POS\r\n')
        no_data = True
        while no_data:
            line = self.dev.readline()
            line = line.strip('\r\n ')
            vals = line.split(' ')
            if line[:6] != 'MOTORS':
                print("-", line)
            else:
                print('motors position', vals[1])
                no_data = False

    def motor_new_pos(self, pos):
        self.dev.write('NEW_POS %d\r\n' % pos)

    def end_values(self, end):
        self.fans_data += [0, ]
        self.fans_time += [end, ]
        self.heat_data += [0, ]
        self.heat_time += [end, ]
        self.mist_data += [0, ]
        self.mist_time += [end, ]
        self.flap_data += [0, ]
        self.flap_time += [end, ]
        self.time_data += [end, ]
        self.temp_data += [self.temp_data[-1], ]
        self.humidity_data += [self.humidity_data[-1], ]

    def get_data(self):
        self.dev.write("DATA\r\n")
        no_data = True
        while no_data:
            line = self.dev.readline()
            line = line.strip('\r\n ')
            vals = line.split(' ')
            if len(vals) != 7:
                print("-", line)
            else:
                no_data = False
        if not self.first_reading:
            if self.discard_first:
                self.start = time.time()
                self.first_reading = True
            else:
                self.discard_first = True
                return
        vals = [str(round(time.time() - self.start, 2)), ] + vals
        float_vals = ()
        print("  ",)
        for val in vals:
            float_val = float(val)
            self.log.write(val + ", ")
            print(str(float_val).ljust(7),)
            float_vals += (float_val, )
        self.log.write("\n")
        print

        self.time_data += [float_vals[0], ]
        self.temp_data += [float_vals[1:-1], ]
        self.humidity_data += [float_vals[-1], ]

        return float_vals

    def log_till_time(self, seconds):
        last_read = time.time()
        while time.time() < self.start + seconds:
            if time.time() - last_read > max([2 * self.shortest_interval, 3]):
                self.get_data()
                last_read = time.time()
        print('log till time finished at', round(time.time() - self.start, 2))

    def open_graph(self):
        print('opening graph')
        self.temp_ax = plt.subplot2grid(
            (11, 1), (1, 0), rowspan=4)
        self.hum_ax = plt.subplot2grid(
            (11, 1), (5, 0), rowspan=4, sharex=self.temp_ax)
        self.signals_ax = plt.subplot2grid(
            (11, 1), (9, 0), rowspan=2, sharex=self.temp_ax)
        # plt.ion()

    def end_test(self, t):
        self.log_till_time(t - 5)
        self.dev.write("CLOSE\r\n")
        self.log.write(str(t) + ' FLAP 0\n')
        self.dev.write("HEAT 1\r\n")
        self.log.write(str(t) + ' HEAT 0\n')
        self.dev.write("FANS 0 0\r\n")
        self.log.write(str(t) + ' FANS 0\n')
        self.dev.write("MIST 0\r\n")
        self.log.write(str(t) + ' MIST 0\n')
        self.LEDs_on(0, 50)
        self.end_values(t)
        self.log.close()
        self.end_time = t

    def plot(self):
        self.open_graph()
        print("replotting")
        self.temp_ax.clear()
        self.temp_ax.plot(self.time_data, self.temp_data, linewidth=2)
        self.temp_ax.set_ylabel('Temperature ($^\circ$C)')
        lines = self.temp_ax.lines
        colors = ['purple', 'magenta', 'red', 'orange', 'grey', 'blue']
        for i in range(len(lines)):
            lines[i].set_color(colors[i])
        self.temp_ax.legend(
            iter(self.temp_ax.lines),
            ('Chamber Top', 'Chamber Bottom', 'Heater Chamber Top',
             'Heater Chamber Bottom', 'Extrernal', 'Humidity Sensor'),
            bbox_to_anchor=(0.0, 1.04, 1.0, 0.102), loc=3, ncol=3,
            mode='expand', borderaxespad=0.0)
        plt.setp(self.temp_ax.get_legend().get_texts(), fontsize='12')
        self.temp_ax.grid(True)
        self.hum_ax.clear()
        self.hum_ax.plot(self.time_data, self.humidity_data, linewidth=2)
        self.hum_ax.set_ylabel('Humidity %')
        self.hum_ax.grid(True)
        self.signals_ax.clear()
        data_sets = [(self.heat_time, self.heat_data),
                     (self.mist_time, self.mist_data),
                     (self.flap_time, self.flap_data),
                     (self.fans_time, self.fans_data)]
        bar_sets = []
        for d in data_sets:
            data = d[1]
            time = d[0]
            in_bar = False
            bars = []
            for i in range(len(data)):
                if not in_bar:
                    if data[i] != 0:
                        in_bar = True
                        start = time[i]
                else:
                    if data[i] == 0:  # end of a bar
                        bars += [(start, time[i] - start), ]
                        in_bar = False
            bar_sets += [bars, ]
        total_width = 10
        space = 2
        centre_points = [
            total_width * 0.5,
            total_width * 1.5,
            total_width * 2.5,
            total_width * 3.5]
        self.signals_ax.broken_barh(
            bar_sets[0], (3 * total_width + space / 2, total_width - space),
            edgecolor='none', facecolors='red')  # heat
        self.signals_ax.broken_barh(
            bar_sets[1], (2 * total_width + space / 2, total_width - space),
            edgecolor='none', facecolors='blue')  # mist
        self.signals_ax.broken_barh(
            bar_sets[2], (1 * total_width + space / 2, total_width - space),
            edgecolor='none', facecolors='cyan')  # flap
        self.signals_ax.broken_barh(
            bar_sets[3], (0 * total_width + space / 2, total_width - space),
            edgecolor='none', facecolors='grey')  # fans
        self.signals_ax.set_ylim(space / 2, 4 * total_width)
        self.signals_ax.set_yticks(centre_points)
        self.signals_ax.set_yticklabels(['fans', 'flap', 'mist', 'heat'])
        self.signals_ax.set_xlabel('Time (seconds)')
        self.signals_ax.grid(True)
        plt.setp(self.hum_ax.get_xticklabels(), visible=False)
        plt.setp(self.temp_ax.get_xticklabels(), visible=False)

        # Fix y ticks on temperature graph
        y_lim = self.temp_ax.get_ylim()
        av = int(y_lim[0] + (y_lim[1] - y_lim[0]) / 2 + 0.5)
        if y_lim[1] - y_lim[0] < 10:
            self.temp_ax.set_ylim(av - 5, av + 5)
        tks = self.temp_ax.get_yticks()
        half = int(len(tks) / 2)
        gap = int(tks[1]) - int(tks[0])
        if gap == 1:
            gap = 2
        self.temp_ax.yaxis.set_ticks(range(
            av - half * gap, av + half * gap + 1, gap))
        self.temp_ax.set_ylim(
            av - (half + 0.5) * gap, av + (half + 0.5) * gap)
        tks = self.temp_ax.get_yticks()
        self.temp_ax.set_ylim(
            int(tks[0]) - 0.5 * gap, int(tks[-1]) + 0.5 * gap)

        # Fix y ticks on humidity graph
        y_lim = self.hum_ax.get_ylim()
        av = int(y_lim[0] + (y_lim[1] - y_lim[0]) / 2 + 0.5)
        if y_lim[1] - y_lim[0] < 10:
            self.hum_ax.set_ylim(av - 5, av + 5)
        tks = self.hum_ax.get_yticks()
        half = int(len(tks) / 2)
        gap = int(tks[1]) - int(tks[0])
        if gap == 1:
            gap = 2
        self.hum_ax.yaxis.set_ticks(range(
            av - half * gap, av + half * gap + 1, gap))
        tks = self.hum_ax.get_yticks()
        self.hum_ax.set_ylim(
            int(tks[0]) - 0.5 * gap, int(tks[-1]) + 0.5 * gap)

        self.temp_ax.set_xlim([0, self.end_time])

        plt.tight_layout(h_pad=0.5)
        plt.show(block=False)
        plt.draw()
        plt.pause(0.001)
