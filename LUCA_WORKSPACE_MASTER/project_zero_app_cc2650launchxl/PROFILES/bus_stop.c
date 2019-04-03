/*
 * bus_stop.c
 *
 *  Created on: Jan 27, 2017
 *      Author: Fred Buhler
 *
 *      Notes:
 *          - fucntions for implementing bus stop 'mailbox'
 *          - made for use with bus_stop_gatt_profile.c
 */

#include <string.h>

#include "bcomdef.h" // for data-types

#include <ti/mw/display/Display.h>

#include "bus_stop.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <ti/drivers/PIN.h>
#include "Board.h"

/* Pin driver handles */
//static PIN_Handle ledPinHandle;

/* Global memory storage for a PIN_Config table */
//static PIN_State ledPinState;

/*
 * Initial LED pin configuration table
 *   - LEDs Board_LED0 & Board_LED1 are off.
 */
/*
PIN_Config ledPinTable[] = {
  Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
  Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
  PIN_TERMINATE
};
*/
extern Display_Handle dispHandle;

#include "osal_snv.h"
#include "gapbondmgr.h"
#include <gap.h>
#include "peripheral.h"
#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL
// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
uint8_t BSAdvertData[] =
{
  // Flags; this sets the device to use limited discoverable
  // mode (advertises for 30 seconds at a time) or general
  // discoverable mode (advertises indefinitely), depending
  // on the DEFAULT_DISCOVERY_MODE define.

  0x02,   // length of this data
  GAP_ADTYPE_FLAGS,
  DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

  9,
  GAP_ADTYPE_SIGNED_DATA,
  0x30, 0x30, 0x30, 0x30,
  0x20, 0x20, 0x20, 0x20,

  // complete name
//  17,
//  GAP_ADTYPE_LOCAL_NAME_COMPLETE,
//  'S', 'e', 'e', 'i', 'n', 'g', 'B', 'u', 's', '3', '1', '3', '0', '0', '0', '2',
  // complete name
  15,
  GAP_ADTYPE_LOCAL_NAME_COMPLETE,
  'L', 'o', 'o', 'k', 'i', 'n', 'g', 'B', 'u', 's', '.', 'c', 'o', 'm',
};


void set_bsadvdate(){
    /*
    ledPinHandle = PIN_open(&ledPinState, ledPinTable);
    if(!ledPinHandle) {
       Display_print0(dispHandle, 0, 0, "Error initializing board LED pins");
      //Log_error0("Error initializing board LED pins");
      //Task_exit();
    }*/
    uint8_t res =
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(BSAdvertData), BSAdvertData);
        Display_print1(dispHandle, 0, 0, "advert: %c", res +'0');
};


mailbox_t mailbox;
bool initialized = false;

char user_arr[user_num][usr_name_max] = {"usr", "a"};
char password_arr[user_num][pwd_max] = {"pwd", "a"};

#define login_cmd     0x01
#define logout_cmd    0x02
#define list_cmd      0x04
#define fetch_cmd     0x05
#define store_cmd     0x08
#define delete_cmd    0x09
#define fail_cmd      0x0E
#define OK_cmd        0x0F
#define cmd_ind       0x00

#define fail_to_login   0x01
#define fail_not_login  0x02
#define fail_msg_count  0x03
#define fail_msg_full   0x04
#define fail_index      0x05
#define fail_delete     0x06
#define fail_no_cmd     0x07
#define fail_no_msg_to_del 0x08

#define len_cmd         1
#define len_msg_count   4
#define len_code        1
#define len_uid         4
#define len_nid         4
#define len_rid         2
#define len_sid         2

#define shift_msg_count cmd_ind + len_cmd
#define shift_code      shift_msg_count + len_msg_count
#define shift_uid       shift_code + len_code
#define shift_nid       shift_uid + len_uid
#define shift_rid       shift_nid + len_nid
#define shift_sid       shift_rid + len_rid


bool loggedin = false;
uint8 loggedin_usr = 0xFF;

uint16  list_rid;
uint32  list_nid;
uint8   list_result[store_msg_max_num];
uint8   list_count;
uint32  msg_count;
uint8   waiting_num = 0;
uint8   delete_count = 0;

byte reply_msg[reply_msg_max_len] = "fail no match cmd";
int reply_len;

//big-endian
uint32 getUint(byte bytes[], uint8 len){
  uint32 res = 0;
  uint8 i = 0;
  for (i = 0; i < len; i = i + 1){
    uint32 temp = bytes[i];
    res = res + (uint32)(temp << ((len - 1 - i) * 8));
    //printf("temp:%lu res:%lu\n", temp, res);
  }
  return res;
}

void do_login(uint8 *pValue, uint8 len){
      uint8 username_len = pValue[1];
      uint8 password_len = pValue[2];

      char username[16] = {'\0','\0', '\0', '\0',
                           '\0','\0', '\0', '\0',
                           '\0','\0', '\0', '\0',
                           '\0','\0', '\0', '\0'};
      int i;
      for (i = 0; i < username_len && i + 3 < len; i = i + 1){
        username[i] = pValue[3 + i];
      }
      username[username_len] = '\0';
      char password[16] = {'\0','\0', '\0', '\0',
                           '\0','\0', '\0', '\0',
                           '\0','\0', '\0', '\0',
                           '\0','\0', '\0', '\0'};
      for (i = 0; i < password_len && 3 + username_len + i < len; i = i + 1){
        password[i] = pValue[3 + username_len + i];
      }
      password[password_len] = '\0';
      Display_print1(dispHandle, 0, 0, "login at pos:%c", '4');
      uint8_t j;
      for (j = 0; j < 2; j = j + 1){
        if (strcmp(user_arr[j], username) == 0
            && strcmp(password_arr[j], password) == 0){
          loggedin_usr = j;
        }
      }

      if (loggedin_usr == 0xFF){
        reply_msg[0] = fail_cmd;
        reply_msg[1] = fail_to_login;
      } else {
        loggedin = true;
        reply_msg[0] = OK_cmd;
        reply_msg[1] = login_cmd;
        reply_msg[2] = (msg_count & 0xFF000000) >> 24;
        reply_msg[3] = (msg_count & 0x00FF0000) >> 16;
        reply_msg[4] = (msg_count & 0x0000FF00) >> 8;
        reply_msg[5] = (msg_count & 0x000000FF);
      }
}

void do_store(uint8* pValue, uint8 len){
        uint32 recv_msg_count = getUint(pValue + shift_msg_count, 4);
//        uint32 recv_msg_count = 20;
        if (msg_count + 1 == recv_msg_count){
          uint32 recv_nid = getUint(pValue + shift_nid, len_nid);
          uint16 recv_rid = getUint(pValue + shift_rid, len_rid);
          byte recv_msg [store_msg_max_len] = {0, 0, 0, 0, 0, 0,
                                               0, 0, 0, 0, 0, 0,
                                               0, 0, 0, 0, 0};
          //printf("sift_nid:%lu nid:%lu rid:%lu\n", shift_nid, recv_nid, recv_rid);
          memcpy(recv_msg, pValue + shift_msg_count, store_msg_max_len);
          if (mailbox.num_msg_stored < store_msg_max_num){
            int j;
            for (j = 0; j < store_msg_max_num; j = j + 1){
              if (mailbox.has_msg[j] == false){
                mailbox.has_msg[j] = true;
                mailbox.msg_rid[j] = recv_rid;
                mailbox.msg_nid[j] = recv_nid;
                mailbox.num_msg_stored = mailbox.num_msg_stored + 1;
                memcpy(mailbox.msgs + store_msg_max_len * j,
                      recv_msg, store_msg_max_len);
                break;
              }
            }
            msg_count = msg_count + 1;
            reply_msg[0] = OK_cmd;
            reply_msg[1] = store_cmd; // Chingun's fix
            reply_msg[2] = (msg_count & 0xFF000000) >> 24;
            reply_msg[3] = (msg_count & 0x00FF0000) >> 16;
            reply_msg[4] = (msg_count & 0x0000FF00) >> 8;
            reply_msg[5] = (msg_count & 0x000000FF);
          } else {
            reply_msg[0] = fail_cmd;
            reply_msg[1] = fail_msg_full;
          }
        } else {
          reply_msg[0] = fail_cmd;
          reply_msg[1] = fail_msg_count;
        }
}

void do_list(uint8* pValue, uint8 len){
        list_rid = getUint(pValue + 1 + 4, 2);
        list_nid = getUint(pValue + 1, 4);
        list_count = 0;
        int j = 0;
        for (j = 0; j < store_msg_max_num; j = j + 1){
          //printf("%i %i,",mailbox.has_msg[j], mailbox.msg_rid[j]);
          if (mailbox.has_msg[j] && mailbox.msg_rid[j] == list_rid
                                 && mailbox.msg_nid[j] == list_nid){
            list_result[list_count] = j;
            list_count = list_count + 1;
          }
        }
        if (list_count > 0){
          reply_msg[0] = OK_cmd;
          reply_msg[1] = list_cmd;
          reply_msg[2] = list_count;
        } else {
          reply_msg[0] = fail_cmd;
          reply_msg[1] = list_cmd;
        }
}

void do_fetch(uint8* pValue, uint8 len){
        uint8 fetch_ind = getUint(pValue + 1, 1);
        if(fetch_ind >= list_count){
          reply_msg[0] = fail_cmd;
          reply_msg[1] = fail_index;
        } else {
          reply_msg[0] = OK_cmd;
          reply_msg[1] = fetch_cmd;
          memcpy(reply_msg + 2,
                  mailbox.msgs + list_result[fetch_ind] * store_msg_max_len,
                  store_msg_max_len);
        }
}

void do_delete(uint8* pValue, uint8 len){
        Display_print0(dispHandle, 0, 0, " -Entered do_delete()");
        //pvalue is the message that was taken from the computer
        uint16   delete_rid = getUint(pValue + 1 + 4, 2);
        uint32  delete_nid = getUint(pValue + 1, 4);
        uint32 delete_msgID = getUint(pValue + 1 + 4 + 2, 4);
        uint32 delete_msgID_cmp = 0;

        delete_count = 0;
        Display_print2(dispHandle, 0, 0, "%lu %lu,", delete_rid, delete_nid);
//        printf("%lu %lu,", delete_rid, delete_nid);
        int j = 0;

        Display_print3(dispHandle, 0, 0, "\n./;Delete Request: %lu %lu %lu,", delete_rid, delete_nid, delete_msgID);

        for (j = 0; j < store_msg_max_num; j = j + 1){

            delete_msgID_cmp = mailbox.msgs[j*17];
            delete_msgID_cmp = mailbox.msgs[j*17+1] + (uint32)(delete_msgID_cmp << 2);
            delete_msgID_cmp = mailbox.msgs[j*17+2] + (uint32)(delete_msgID_cmp << 2);
            delete_msgID_cmp = mailbox.msgs[j*17+3] + (uint32)(delete_msgID_cmp << 2);

            Display_print4(dispHandle, 0, 0, "Mailbox messages: %i %lu %lu %lu,", mailbox.has_msg[j], mailbox.msg_rid[j], mailbox.msg_nid[j], delete_msgID_cmp);


//          printf("%i %lu %lu,",mailbox.has_msg[j], mailbox.msg_rid[j], mailbox.msg_nid[j]);
//          if (mailbox.has_msg[j] && mailbox.msg_rid[j] == delete_rid
//                                 && mailbox.msg_nid[j] == delete_nid){
//          deletes all the messages at a
            if (mailbox.has_msg[j] && mailbox.msg_rid[j] == delete_rid
                                 && mailbox.msg_nid[j] == delete_nid
                                 && delete_msgID_cmp == delete_msgID){
                mailbox.has_msg[j] = false;
                mailbox.num_msg_stored = mailbox.num_msg_stored - 1;
                delete_count = delete_count + 1;
            }
        }
        if (delete_count > 0){
          reply_msg[0] = OK_cmd;
          reply_msg[1] = delete_cmd;
          reply_msg[2] = delete_count;
        } else {
          reply_msg[0] = fail_cmd;
          reply_msg[1] = fail_no_msg_to_del;
        }
}

void do_logout(){
    reply_msg[0] = OK_cmd;
    reply_msg[1] =logout_cmd;
}

/*  Expecting 'BUS_STOP_PACKET_LEN' to be 5
 *
 *
 *  Packet is 5x16 bits (5 words)
 *      _ _ _ _ _ _ _ _ | _ _ _ _ _ _ _ _ | _ _ _ _ _ _ _ _ | _ _ _ _ _ _ _ _ | _ _ _ _ _ _ _ _
 *          Word 1            Word 2            Word 3            Word 4            Word 5
 *
 *
 *      Word 1 (Headers): _ _ _ -  _ _ _ - _ _
 *                          H1       H2     H3
 *
 *      Word 2-5: Data / Message
 */
void BusStop_HandleMessage(uint8 *param, uint8 len)
{
    Display_print1(dispHandle, 0, 0, "len: %i", len);

    uint8 Header1 = 0;
    uint8 Header2 = 0;
    uint8 xTraBits = 0;
    uint8 dataIn[BUS_STOP_PACKET_LEN-1] = { 0, 0, 0, 0}; // I don't know if you can put '-1' here. Confirm with CS.

    Display_print0(dispHandle, 0, 0, " -Entered BusStop_HandleMessage()");
    if(len > BUS_STOP_PACKET_LEN)
    {
        Display_print2(dispHandle, 0, 0, " -err: packet length too long (%d > %d)",len, BUS_STOP_PACKET_LEN);
        return;
    }

    Display_print5(dispHandle, 0, 0, "Data in check: %d %d %d %d %d", param[0], param[1], param[2], param[3], param[4]);

    Header1 = (param[0] >> 5) & 0x07;
    Header2 = (param[0] >> 2) & 0x07;
    xTraBits = param[0] & 0x03;

    // There is likely a better way to do this. Confer with CS.
    dataIn[0] = param[1];
    dataIn[1] = param[2];
    dataIn[2] = param[3];
    dataIn[3] = param[4];


    Display_print1(dispHandle, 0, 0, "Header 1: %d", Header1);
    Display_print1(dispHandle, 0, 0, "Header 2: %d", Header2);
    Display_print1(dispHandle, 0, 0, "Extra Bits: %d", xTraBits);
    Display_print4(dispHandle, 0, 0, "Packet Data: %d %d %d %d %d", dataIn[0],dataIn[1],dataIn[2],dataIn[3]);

    /*************************************************************
     *
     * Code added here
     *
     *************************************************************/
    uint8 *pValue = param;
    reply_msg[0] = fail_cmd;
    reply_msg[1] = fail_no_cmd;
    uint8 ind;
    for (ind = 2; ind < reply_msg_max_len; ind = ind + 1){
        reply_msg[ind] = 0;
    }
    reply_len = 20;
    if (initialized == false){

      msg_count = 0;
      loggedin_usr = 0xFF;
      loggedin = false;
      mailbox.num_msg_stored = 0;
      uint8 i;
      for (i = 0; i < store_msg_max_num; i = i + 1){
        mailbox.has_msg[i] = false;
      }
      list_count = 0;
      initialized = true;
//      Log_info1("msg_count: %s",
//                        (IArg) msg_count);
    }

    //get command
    uint8 command = pValue[cmd_ind];
    Display_print5(dispHandle, 0, 0, "Data in check: %x %x %x %x %x", pValue[0], pValue[1], pValue[2], pValue[3], pValue[4]);
    Display_print5(dispHandle, 0, 0, "Data in check: %x %x %x %x %x", pValue[5], pValue[6], pValue[7], pValue[8], pValue[9]);
    if (command == login_cmd){
      do_login(pValue, len);
    } else if (loggedin == true){
      if (command == store_cmd){
        do_store(pValue, len);
      } else if (command == list_cmd){
        do_list(pValue, len);
      } else if (command == fetch_cmd){
        do_fetch(pValue, len);
      } else if (command == delete_cmd){
        do_delete(pValue, len);
      } else if (command == logout_cmd){
        do_logout();
      }
    }
}


  void store_msg(){
      byte received_string [reply_msg_max_len];
      memset(received_string, 0, reply_msg_max_len);
      memcpy(received_string, reply_msg, reply_len);

      // Needed to copy before log statement, as the holder array remains after
      // the pCharData message has been freed and reused for something else.
      if (received_string[0] == OK_cmd){
        if (received_string[1] == store_cmd){


            Display_print0(dispHandle, 0, 0, "START TO OSAL SNV WRITE %c");

                uint32 snv_status = osal_snv_write(mailbox_nvid, sizeof(mailbox), &mailbox);
  //              Log_info1("store mailbox after OKstored: %c",
  //                      (IArg) snv_status +'0');
                Display_print1(dispHandle, 0, 0, "store mailbox after OKstored: %c", snv_status +'0');
                osal_snv_write(msg_count_nvid, sizeof(msg_count), &msg_count);
                Display_print1(dispHandle, 0, 0, "update msg_count: %c", snv_status +'0');
 //               Log_info1("update msg_count: %c",
 //                       (IArg) snv_status +'0');
                waiting_num = waiting_num + 1;
                BSAdvertData[8] = '0' + waiting_num;  //only dealed with num < 10

                GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(BSAdvertData), BSAdvertData);

//                PIN_setOutputValue(ledPinHandle, Board_LED1, 1);
        } else if (received_string[1] == delete_cmd){

                uint32 snv_status = osal_snv_write(mailbox_nvid, sizeof(mailbox), &mailbox);
                Display_print1(dispHandle, 0, 0, "store mailbox after OK deleted", snv_status +'0');
  //              Log_info1("store mailbox after OK deleted: %c",
  //                      (IArg) snv_status +'0');
                if (mailbox.num_msg_stored == 0){
//                    PIN_setOutputValue(ledPinHandle, Board_LED1, 0);
                }
                waiting_num = waiting_num - delete_count;
                BSAdvertData[8] = '0' + waiting_num; //only dealed with num < 10
                GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(BSAdvertData), BSAdvertData);
        }
      }
  }


  void init_msgbox(){
        uint32 snv_status;
        snv_status = osal_snv_read(msg_count_nvid, sizeof(msg_count), &msg_count);
        //Log_info1("read msg_count: %c",
        //                      (IArg) snv_status +'0');
        Display_print1(dispHandle, 0, 0, "read msg_count: %c", snv_status +'0');

        if (snv_status != 0){
          initialized = false;
        } else {
          initialized = true;
        }

        snv_status = osal_snv_read(mailbox_nvid, sizeof(mailbox), &mailbox);
        //Log_info1("read mailbox: %c",
        //                      (IArg) snv_status +'0');
        Display_print1(dispHandle, 0, 0, "read mailbox: %c", snv_status +'0');

        if (initialized && mailbox.num_msg_stored > 0){
//          PIN_setOutputValue(ledPinHandle, Board_LED1, 1);
          waiting_num = mailbox.num_msg_stored;
          BSAdvertData[8] = '0' + waiting_num;  //only dealed with num < 10
          GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(BSAdvertData), BSAdvertData);
        }

  }






