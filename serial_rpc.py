
import serial
import time
import yaml
import ast
from enum import Enum
import logging
import struct
import threading
from queue import Queue

MAX_RESPONSE_PACKET_QUEUE_SIZE = 10
MAX_PAYLOAD_LENGTH = 9
PACKET_SIZE = MAX_PAYLOAD_LENGTH + 3

def crc8_ccitt(data: bytes) -> int:
    if not isinstance(data, (bytes, bytearray)):
        raise TypeError("Input must be bytes or bytearray.")

    crc = 0x00  # Initial value
    poly = 0x07  # Polynomial

    for byte in data:
        crc ^= byte  # XOR byte into CRC
        for _ in range(8):  # Process each bit
            if crc & 0x80:  # If MSB is set
                crc = ((crc << 1) & 0xFF) ^ poly
            else:
                crc = (crc << 1) & 0xFF

    return crc

class serial_rpc:

    class serial_rpc_parameter:
    
        def __init__(self, parameter_group: str, parameter_set_name: str, id: int, parameter_name: str, parameter_type: str, is_a_list: bool, list_length_parameter_name = ''):
            
            self.value = 0
            self.parameter_name: str = parameter_name
            self.parameter_group: str = parameter_group
            self.parameter_set_name: str = parameter_set_name
            self.parameter_full_name: str = parameter_set_name + ":" + parameter_name
            self.parameter_type: str = parameter_type
            self.is_a_list:bool = is_a_list
            self.list_length_parameter_name = list_length_parameter_name
            self.id = id
        
        def get(self):
            return self.value
        
        def set(self, value):
            self.value = value

    def log_parameter_info(self, parameter):
        self.logger.info("-----------------------------------------------")
        self.logger.info(f"Parameter set ID: {parameter.id}")
        self.logger.info(f"Parameter group: {parameter.parameter_group}")
        self.logger.info(f"Parameter set name: {parameter.parameter_set_name}")
        self.logger.info(f"Parameter name: {parameter.parameter_name}")
        self.logger.info(f"Parameter value: {parameter.value}")
        self.logger.info("-----------------------------------------------")


    def load_parameters(self):
        
        file_handle = open(self.parameter_file, 'r')
        yaml_file_data = yaml.safe_load(file_handle)
        
        for command in yaml_file_data['commands']:
        
            command_name = command['name']
            command_id = command['id']
        
            request_parameters = command['request_parameters']
            response_parameters = command['response_parameters']
        
            for request_parameter in request_parameters:
                request_parameter_name = request_parameter['name']
                request_parameter_is_a_list = request_parameter['is_a_list']
                request_parameter_type = request_parameter['type']
                
                self.request_parameters.append(self.serial_rpc_parameter(parameter_group = 'request', parameter_set_name = command_name, parameter_name = request_parameter_name, parameter_type = request_parameter_type, id = command_id, is_a_list = request_parameter_is_a_list))

            for response_parameter in response_parameters:
                response_parameter_name = response_parameter['name']
                response_parameter_is_a_list = response_parameter['is_a_list']
                response_parameter_type = response_parameter['type']
                
                if(response_parameter_is_a_list): 
                    response_parameter_list_length_parameter = response_parameter['length_parameter']
                    self.response_parameters.append(self.serial_rpc_parameter(parameter_group = 'response', parameter_set_name = command_name, parameter_name = response_parameter_name, parameter_type = response_parameter_type, is_a_list= response_parameter_is_a_list, id = command_id, list_length_parameter_name = response_parameter_list_length_parameter))
                else:
                    self.response_parameters.append(self.serial_rpc_parameter(parameter_group = 'response', parameter_set_name = command_name, parameter_name = response_parameter_name, parameter_type = response_parameter_type, is_a_list= response_parameter_is_a_list, id = command_id))


        for parameter in self.request_parameters:
            self.log_parameter_info(parameter)

        for parameter in self.response_parameters:
            self.log_parameter_info(parameter)

    def serial_rx_task(self):
            
        while self.running:
                
            if(self.serial_port.in_waiting >= PACKET_SIZE):
                    
                rx_packet = self.serial_port.read(PACKET_SIZE)
                self.response_packet_queue.put(rx_packet)

    def __init__(self, port_name: str, baud_rate: int, parameter_file: str, timeout_ms:int = 1000):

        logging.basicConfig( level = logging.DEBUG, format="%(asctime)s [%(levelname)s] %(name)s: %(message)s", filename = "serial_rpc.log", filemode = "w" )

        self.logger = logging.getLogger('serial_rpc_logger')
        
        self.serial_port: serial.Serial = serial.Serial(port = port_name, baudrate = baud_rate, timeout = float(timeout_ms) * float(0.001))
        self.request_parameters = []
        self.response_parameters = []
        self.parameter_file: str = parameter_file
        
        self.load_parameters()

        self.running = True
        self.response_packet_queue = Queue(maxsize = MAX_RESPONSE_PACKET_QUEUE_SIZE)
        self.rx_thread = threading.Thread(target = self.serial_rx_task)
        self.rx_thread.start()
        self.rx_timeout = float(0.001) * float(timeout_ms)
        
        self.logger.debug("RPC initialized")

    def close(self):
            
        self.running = False
        self.rx_thread.join()
        self.serial_port.close()
        
    def set_request_parameter_value(self, request_name, parameter_name, value):
        
        target_parameter = None
        
        for parameter in self.request_parameters:
            if parameter.parameter_set_name == request_name and parameter.parameter_name == parameter_name:
                target_parameter = parameter
                break
        
        if target_parameter == None:
            self.logger.error(f"Request parameter {request_name}:{parameter_name} not found !")
            raise ValueError(f"Request parameter {request_name}:{parameter_name} not found !")
        
        target_parameter.set(value)

    def get_request_parameter_value(self, request_name, parameter_name):
        
        target_parameter = None
        
        for parameter in self.request_parameters:
            if parameter.parameter_set_name == request_name and parameter.parameter_name == parameter_name:
                target_parameter = parameter
                break
        
        if target_parameter == None:
            self.logger.error(f"Request parameter {request_name}:{parameter_name} not found !")
            raise ValueError(f"Request parameter {request_name}:{parameter_name} not found !")
        
        return target_parameter.get()

    def set_response_parameter_value(self, response_name, parameter_name, value):
        
        target_parameter = None
        
        for parameter in self.response_parameters:
            if parameter.parameter_set_name == response_name and parameter.parameter_name == parameter_name:
                target_parameter = parameter
                break
        
        if target_parameter == None:
            self.logger.error(f"Response parameter {response_name}:{parameter_name} not found !")
            raise ValueError(f"Response parameter {response_name}:{parameter_name} not found !")
        
        target_parameter.set(value)

    def get_response_parameter_value(self, response_name, parameter_name):
        
        target_parameter = None
        
        for parameter in self.response_parameters:
            if parameter.parameter_set_name == response_name and parameter.parameter_name == parameter_name:
                target_parameter = parameter
                break
        
        if target_parameter == None:
            self.logger.error(f"Response parameter {response_name}:{parameter_name} not found !")
            raise ValueError(f"Response parameter {response_name}:{parameter_name} not found !")
        
        return target_parameter.get()


    def form_request_payload(self, request_name: str):
        
        parameter_list = []
        
        for parameter in self.request_parameters:
            if parameter.parameter_set_name == request_name:
                parameter_list.append(parameter)
        
        payload = []
        
        for parameter in parameter_list:
            
            if parameter.is_a_list == False:
                
                if parameter.parameter_type == 'uint8' or parameter.parameter_type == 'int8':
                    payload.append( int(parameter.value) & 255 )
                
                if parameter.parameter_type == 'uint16' or parameter.parameter_type == 'int16':
                    payload.append( (int(parameter.value) >> 8) & 255 )
                    payload.append( int(parameter.value) & 255 )
                
                if parameter.parameter_type == 'uint32' or parameter.parameter_type == 'int32':
                    payload.append( ( int(parameter.value) >> 24 ) & 255 )
                    payload.append( ( int(parameter.value) >> 16 ) & 255 )
                    payload.append( ( int(parameter.value) >> 8 ) & 255 )
                    payload.append( ( int(parameter.value) ) & 255 )
                    
                if parameter.parameter_type == 'float':    
                    _value = struct.unpack('>I', struct.pack('>f', float(parameter.value)))[0]
                    payload.append( ( _value >> 24 ) & 255 )
                    payload.append( ( _value >> 16 ) & 255 )
                    payload.append( ( _value >> 8 ) & 255 )
                    payload.append( _value & 255 )                

            else:

                values = parameter.value

                for value in values:

                    if parameter.parameter_type == 'uint8' or parameter.parameter_type == 'int8':
                        payload.append( int(value) & 255 )
                
                    if parameter.parameter_type == 'uint16' or parameter.parameter_type == 'int16':
                        payload.append( (int(value) >> 8) & 255 )
                        payload.append( int(value) & 255 )
                
                    if parameter.parameter_type == 'uint32' or parameter.parameter_type == 'int32':
                        payload.append( ( int(value) >> 24 ) & 255 )
                        payload.append( ( int(value) >> 16 ) & 255 )
                        payload.append( ( int(value) >> 8 ) & 255 )
                        payload.append( ( int(value) ) & 255 )
                    
                    if parameter.parameter_type == 'float':    
                        _value = struct.unpack('>I', struct.pack('>f', float(value)))[0]
                        payload.append( ( _value >> 24 ) & 255 )
                        payload.append( ( _value >> 16 ) & 255 )
                        payload.append( ( _value >> 8 ) & 255 )
                        payload.append( _value & 255 )                

        self.logger.info(f"Request payload for request parameter {request_name} = {payload}")

        return payload

    def get_request_id(self, request_name: str):
        
        target_parameter = None
        
        for parameter in self.request_parameters:
            if parameter.parameter_set_name == request_name:
                target_parameter = parameter
        
        if target_parameter == None:
            raise ValueError(f"Request ID for request {request_name} not found !")
        
        return target_parameter.id

    def get_response_name(self, response_id: int):
        
        target_parameter = None
        
        for parameter in self.response_parameters:
            if parameter.id == response_id:
                target_parameter = parameter
        
        if target_parameter == None:
            raise ValueError(f"Response name for response ID {response_id} not found !")
        
        return target_parameter.parameter_set_name    

    def decode_response_packet(self, packet):
        
        if crc8_ccitt(packet) != 0:
            raise IOError("CRC of response packet is corrupted !")
        
        response_id = int(packet[0])
        response_payload_length = int(packet[1])
        response_payload = packet[2: (2 + response_payload_length)]
        response_name = self.get_response_name(response_id)
        
        self.logger.info(f"Response packet ID: {response_id}")
        self.logger.info(f"Response payload length: {response_payload_length}")
        self.logger.info(f"Response payload: {response_payload}")
        self.logger.info(f"Response name: {response_name}")
        
        response_parameter_list = []
        
        for parameter in self.response_parameters:
        
            if parameter.id == response_id:
            
                response_parameter_list.append(parameter)
        
        index = 0
        
        for parameter in response_parameter_list:
            
            if parameter.is_a_list == False:
                
                if parameter.parameter_type == 'uint8' or parameter.parameter_type == 'int8':
                   
                    _value = int.from_bytes(response_payload[index:index+1], byteorder = 'big', signed = (parameter.parameter_type == 'int8') )
                    index += 1
                    
                    parameter.set(_value)
                
                if parameter.parameter_type == 'uint16' or parameter.parameter_type == 'int16':
                
                    _value = int.from_bytes(response_payload[index : index+2], byteorder = 'big', signed = (parameter.parameter_type == 'int16') )
                    index += 2
                    
                    parameter.set(_value)
                    
                if parameter.parameter_type == 'uint32' or parameter.parameter_type == 'int32':

                    _value = int.from_bytes(response_payload[index : index+4], byteorder = 'big', signed = (parameter.parameter_type == 'int32') )                
                    index += 4
                    
                    parameter.set(_value)

                if parameter.parameter_type == 'float':

                    _value = int( int( response_payload[index] << 24 ) | int( response_payload[index+1] << 16 ) | int( response_payload[index+2] << 8 ) | int(response_payload[index+3]) )                
                    float_value = struct.unpack('>f', _value.to_bytes(4, 'big'))[0]
                    index += 4
                    
                    parameter.set(float_value)        

                self.logger.info(f"Setting value of {parameter.parameter_set_name} : {parameter.parameter_name} to {_value}")
            
            else:
                
                list_length = self.get_response_parameter_value(parameter.parameter_set_name, parameter.list_length_parameter_name)
                
                values = []
                
                for i in range(0, list_length):
                
                    if parameter.parameter_type == 'uint8' or parameter.parameter_type == 'int8':
                   
                        _value = int.from_bytes(response_payload[index:index+1], byteorder = 'big', signed = (parameter.parameter_type == 'int8') )
                        index += 1
                    
                        values.append(_value)
                
                    if parameter.parameter_type == 'uint16' or parameter.parameter_type == 'int16':
                
                        _value = int.from_bytes(response_payload[index : index+2], byteorder = 'big', signed = (parameter.parameter_type == 'int16') )
                        index += 2
                    
                        values.append(_value)
                    
                    if parameter.parameter_type == 'uint32' or parameter.parameter_type == 'int32':

                        _value = int.from_bytes(response_payload[index : index+4], byteorder = 'big', signed = (parameter.parameter_type == 'int32') )                
                        index += 4
                    
                        values.append(_value)

                    if parameter.parameter_type == 'float':

                        _value = int( int( response_payload[index] << 24 ) | int( response_payload[index+1] << 16 ) | int( response_payload[index+2] << 8 ) | int(response_payload[index+3]) )                
                        float_value = struct.unpack('>f', _value.to_bytes(4, 'big'))[0]
                        index += 4
                    
                        values.append(float_value)        

                parameter.set(values)

                self.logger.info(f"Setting value of {parameter.parameter_set_name} : {parameter.parameter_name} to {values}")

    def send_request(self, request_name: str):
    
        packet = []
    
        payload = self.form_request_payload(request_name)
        id = self.get_request_id(request_name)
        
        if len(payload) > MAX_PAYLOAD_LENGTH:
            raise ValueError(f"Number of bytes in the payload cannot be greater than {MAX_PAYLOAD_LENGTH}")
        
        packet.append( id )
        packet.append( len(payload) )
        
        packet = packet + payload
        
        for i in range(0, (MAX_PAYLOAD_LENGTH - len(payload))):
            packet.append(0)

        crc = crc8_ccitt(bytes(packet))
        
        packet.append(int(crc))
        
        self.logger.info(f"Sending request payload: {packet}")
        
        self.serial_port.write(packet)
        
        response_packet = self.response_packet_queue.get(timeout = self.rx_timeout)
        
        self.decode_response_packet(response_packet)
