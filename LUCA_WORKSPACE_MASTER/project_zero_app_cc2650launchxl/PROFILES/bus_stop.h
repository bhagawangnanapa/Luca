/*
 * bus_stop.h
 *
 *  Created on: Jan 18, 2017
 *      Author: Fred Buhler
 *
 *      Notes:
 *          - Values in this header file are for use in a modified 'simple_peripheral' for the CC2560 launchpad
 *          - Comments and modifications to the original 'simple_peripheral' are commented as '// FB'
 *          - Values or constants changed for this project (Bus Stop) were renamed with 'BS' in the name
 *          - Coder (FB) is an analog circuit designer with fundamental coding skills. This header file might not be the best solution.
 *
 */



#ifndef BUS_STOP_H
#define BUS_STOP_H

#include <stdbool.h>
#include <stdint.h>
//typedef uint8_t  uint8;
// uint16_t uint16;
//typedef uint32_t uint32;
typedef unsigned char byte;

//used to store message
#define   jogger_nvid       0x80
#define   msg_count_nvid    0x81
#define   mailbox_nvid      0x82
#define   store_addr    0x88
/*
#define GAP_BS_DEVICE_NAME_LEN      13              // Excluding null-terminate char - this is the length of the next line
#define GAP_BS_NAME                 "Bus Stop: 51a" // Legnth of this string should be in the line above
*/
#define GAP_BS_DEVICE_NAME_LEN      14              // Excluding null-terminate char - this is the length of the next line
#define GAP_BS_NAME                 "LookingBus.com" // Legnth of this string should be in the line above

#define BS_BROADCAST_NAME_LEN       0x15            // length of broadcast name - this is the length of the next line + 1 (Null)
#define BS_BROADCAST_NAME           'C',\
        'i',\
        't',\
        'y',\
        'B',\
        'u',\
        's',\
        ' ',\
        'V',\
        'i',\
        's',\
        'u',\
        'a',\
        'l',\
        'A',\
        's',\
        's',\
        'i',\
        's',\
        't'                                         // Broadcast name - Legnth of this string + 1 should be in the line above




#define DEVINFO_MODEL_NUMBER_LEN        9   // Length of next line
#define DEVINFO_MODEL_NUMBER_BS         "IoT_BS_1A"

#define DEVINFO_SERIAL_NUMBER_LEN       13  // Length of next line
#define DEVINFO_SERIAL_NUMBER_BS        "XXXX-XXXXX-XX"

#define DEVINFO_FIRMWARE_REV_LEN        4   // Length of next line
#define DEVINFO_FIRMWARE_REV_BS         "1.1a"

#define DEVINFO_HARDWARE_REV_LEN        3   // Length of next line
#define DEVINFO_HARDWARE_REV_BS         "0.1"

#define DEVINFO_SOFTWARE_REV_LEN        4   // Length of next line
#define DEVINFO_SOFTWARE_REV_BS         "1.0a"

#define DEVINFO_MANUFACTURER_NAME_LEN   12  // Length of next line
#define DEVINFO_MANUFACTURER_NAME_BS    "Aweslome LLC"

#define BS_PASSKEY                      201717 // Range is 0 - 999,999. Default is 0.

#define BUS_STOP_PACKET_LEN             20 // If this does not match the characteristic data length there will be errors

extern void BusStop_HandleMessage(uint8 *param, uint8 len);

#define reply_msg_max_len 20
#define msg_max 20
#define buf_max 16
#define cmd_max 10
#define usr_name_max 16
#define pwd_max 16
#define user_num 2
#define id_len 2
#define msg_count_len 4

#define store_msg_max_num 10
#define store_msg_max_len 17

typedef struct{
  uint32 msg_nid[store_msg_max_num];
  uint16 msg_rid[store_msg_max_num];
  byte msgs[store_msg_max_len * store_msg_max_num];
  uint8 num_msg_stored;
  bool has_msg[store_msg_max_num];

}mailbox_t;

extern bool initialized;
extern mailbox_t mailbox;
extern uint32 msg_count;

extern byte reply_msg[reply_msg_max_len];
extern int reply_len;

extern void set_bsadvdate();
extern void store_msg();
extern void init_msgbox();
extern uint8_t BSAdvertData[31];

#endif /* BUS_STOP_H_ */
