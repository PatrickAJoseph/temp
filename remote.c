
#include "uart.h"
#include "remote.h"
#include "acquisition.h"
#include "calibration.h"

#include <string.h>

#define REMOTE_PACKET_PAYLOAD_SIZE      9
#define REMOTE_PACKET_SIZE              (REMOTE_PACKET_PAYLOAD_SIZE + 3)

#define REMOTE_CALLBACK_DEFINE(__function__)        \
    uint8_t __function__(uint8_t* request, int request_size, uint8_t* response)

static const uint8_t CRC_TABLE[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
    0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
    0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
    0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
    0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
    0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
    0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
    0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
    0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
    0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
    0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
    0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
    0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
    0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
    0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
    0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
    0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

uint8_t remote_crc8ccitt(uint8_t * data, int size)
{
    uint8_t val = 0;

    uint8_t * pos = (uint8_t *) data;
    uint8_t * end = pos + size;

    while (pos < end) {
        val = CRC_TABLE[val ^ *pos];
        pos++;
    }

    return val;
}

uint8_t request_packet[REMOTE_PACKET_SIZE] = {0};
uint8_t response_packet[REMOTE_PACKET_SIZE] = {0};
volatile uint8_t request_rx_byte_index = 0;
volatile uint8_t response_pending = 0;

typedef uint8_t (*remote_callback_t)(uint8_t* request, int request_size, uint8_t* response);

const remote_callback_t remote_callback_table[];

static void remote_callback(uint8_t rx_byte, void* args)
{
    (void)args;

    if(response_pending == 0)
    {
        request_packet[request_rx_byte_index] = rx_byte;
        request_rx_byte_index++;

        if(request_rx_byte_index >= REMOTE_PACKET_SIZE)
        {
            response_pending = 1;
            request_rx_byte_index = 0;
        }
    }
}

void remote_init()
{
    request_rx_byte_index = 0;
    response_pending = 0;

    memset(request_packet, 0, REMOTE_PACKET_SIZE);
    memset(response_packet, 0, REMOTE_PACKET_SIZE);

    uart_set_rx_callback(remote_callback, 0);
}

void remote_process()
{
    uint8_t crc;

    if(response_pending)
    {
        memset(response_packet, 0, REMOTE_PACKET_SIZE);

        crc = remote_crc8ccitt(request_packet, REMOTE_PACKET_SIZE);

        if(crc)
        {
            __no_operation();
        }
        else
        {
            response_packet[0] = request_packet[0];

            response_packet[1] = remote_callback_table[response_packet[0]]( &request_packet[2],
                                                                            request_packet[1],
                                                                            &response_packet[2] );

            response_packet[REMOTE_PACKET_SIZE - 1] = remote_crc8ccitt(response_packet, REMOTE_PACKET_SIZE - 1);

            uart_send(response_packet, REMOTE_PACKET_SIZE);
        }

        memset(request_packet, 0, REMOTE_PACKET_SIZE);
        response_pending = 0;
    }
}

/***************************** Application specific code *************************/

static REMOTE_CALLBACK_DEFINE(ping);
static REMOTE_CALLBACK_DEFINE(get_input_codes);
static REMOTE_CALLBACK_DEFINE(read_calibration_table_entry);
static REMOTE_CALLBACK_DEFINE(write_calibration_table_entry);
static REMOTE_CALLBACK_DEFINE(read_input_voltage);

const remote_callback_t remote_callback_table[] = {
      ping,
      get_input_codes,
      read_calibration_table_entry,
      write_calibration_table_entry,
      read_input_voltage,
};

uint8_t ping_counter;

static REMOTE_CALLBACK_DEFINE(ping)
{
    *response = ping_counter++;
    return 1;
}

static REMOTE_CALLBACK_DEFINE(get_input_codes)
{
    uint16_t input;
    uint8_t index;

    for( index = 0 ; index < 4 ; index++ )
    {
        input = (uint16_t)acquisition_get_differential_input_code(index);
        response[index << 1] = ((input >> 8) & 0xFF);
        response[(index << 1) + 1] = (input & 0xFF);
    }

    return 8;
}

static REMOTE_CALLBACK_DEFINE(read_calibration_table_entry)
{
    uint8_t channel = 0;
    uint16_t index = 0;
    int16_t code = 0;
    int32_t voltage = 0;

    channel = request[0];
    index = (((uint16_t)request[1] << 8) | (uint16_t)request[2]);

    calibration_read(channel, index, &code, &voltage);

    response[0] = (uint8_t)(((uint16_t)code) >> 8);
    response[1] = (uint8_t)(((uint16_t)code) >> 0);

    response[2] = (uint8_t)(((uint32_t)voltage) >> 24);
    response[3] = (uint8_t)(((uint32_t)voltage) >> 16);
    response[4] = (uint8_t)(((uint32_t)voltage) >> 8);
    response[5] = (uint8_t)(((uint32_t)voltage));

   return 6;
}

static REMOTE_CALLBACK_DEFINE(write_calibration_table_entry)
{
    uint8_t channel = 0;
    uint16_t index = 0;
    int16_t code = 0;
    int32_t voltage = 0;

    channel = request[0];
    index = (((uint16_t)request[1] << 8) | (uint16_t)request[2]);
    code = (int16_t)(((uint16_t)request[3] << 8) | (uint16_t)request[4]);
    voltage = (int32_t)(((uint32_t)request[5] << 24) | ((uint32_t)request[6] << 16) | ((uint32_t)request[7] << 8) | ((uint32_t)request[8] << 0));

    calibration_write(channel, index, code, voltage);

    response[0] = 0;

    return 1;
}

static REMOTE_CALLBACK_DEFINE(read_input_voltage)
{
    int32_t voltage = acquisition_get_input_voltage(request[0]);

    response[0] = (uint8_t)(((uint32_t)voltage) >> 24);
    response[1] = (uint8_t)(((uint32_t)voltage) >> 16);
    response[2] = (uint8_t)(((uint32_t)voltage) >> 8);
    response[3] = (uint8_t)(((uint32_t)voltage));

    return 4;
}
