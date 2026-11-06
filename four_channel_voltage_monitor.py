import serial_rpc
import time
import csv


def write_calibration_table(handle):

    calibration = csv.reader(open('calibration.csv', 'r'))
    header = next(calibration)
    
    for entry in calibration:
        handle.set_request_parameter_value('write_calibration_table_entry', 'channel', entry[0])
        handle.set_request_parameter_value('write_calibration_table_entry', 'index', entry[1])
        handle.set_request_parameter_value('write_calibration_table_entry', 'code', entry[3])
        handle.set_request_parameter_value('write_calibration_table_entry', 'voltage', entry[2])

        handle.send_request('write_calibration_table_entry')

def read_calibration_table_entry(handle, channel ,index):
    handle.set_request_parameter_value('read_calibration_table_entry', 'channel', channel)
    handle.set_request_parameter_value('read_calibration_table_entry', 'index', index)

    handle.send_request('read_calibration_table_entry')

    return [handle.get_response_parameter_value('read_calibration_table_entry', 'voltage'), handle.get_response_parameter_value('read_calibration_table_entry', 'code')]

def get_input_voltages(handle):

    input_voltages = []

    for i in range(0, 4):

        handle.set_request_parameter_value('get_input_voltage', 'channel', i)
        handle.send_request('get_input_voltage')
        input_voltages.append(handle.get_response_parameter_value('get_input_voltage', 'voltage'))

    return input_voltages

handle = serial_rpc.serial_rpc(port_name = 'COM19', baud_rate = 9600, parameter_file='four_channel_voltage_monitor.yaml')

handle.set_request_parameter_value('ping', 'increment', 1)

handle.send_request('ping')

print(handle.get_response_parameter_value('ping', 'count'))

samples = [0, 0, 0, 0]

print(get_input_voltages(handle))

write_calibration_table(handle)

# Calibration code

for i in range(0, 100000):

    handle.send_request('get_input_codes')

    samples[0] = handle.get_response_parameter_value('get_input_codes', 'input_0_code')
    samples[1] = handle.get_response_parameter_value('get_input_codes', 'input_1_code')
    samples[2] = handle.get_response_parameter_value('get_input_codes', 'input_2_code')
    samples[3] = handle.get_response_parameter_value('get_input_codes', 'input_3_code')

    voltages = get_input_voltages(handle)

    time.sleep(0.1)

    print(f"time = {round(0.1*i,1)} : samples: {samples}, voltages: {voltages}")

handle.close()
