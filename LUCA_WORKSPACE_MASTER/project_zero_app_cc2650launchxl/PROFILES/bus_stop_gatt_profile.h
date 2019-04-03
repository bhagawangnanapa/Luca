/*

 Author: Fred Buhler
 Date: 1/20/2017
 Development base: Texas Instruments CC2650 Launchpad
 Reference: ble_sdk_2_02_01_18, simple_gatt_profile.h

 Purpose: Implement a GATT profile to be programmed into a CC2650/CC2640 uC.
    Profile is for a Peripheral (GATT Client) with two characteristics.
    One Characteristic is for sending data to the 'mailbox' (phone to mailbox).
    Second Characteristic is a notification sent from the mailbox back to the phone.

 Profile Characteristics:
    1) BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN     Write access to client
    2) BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT    Client Notification, for Server subscription



 Use of Texas Instruments hardware abstraction / original disclaimer
 ******************************************************************************

 Copyright (c) 2010-2016, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 Release Name: ble_sdk_2_02_01_18
 Release Date: 2016-10-26 15:20:04
 *****************************************************************************/



#ifndef BUS_STOP_GATT_PROFILE_H
#define BUS_STOP_GATT_PROFILE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Profile Parameters ID
#define BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN        0  // R uint8 - Profile CHARACTERISTICic 1 value
#define BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT       1  // W uint8 - Profile CHARACTERISTICic 2 value



// Simple Profile Service UUID
// FB: This is the default from 'simple_gatt_profile.h'.
//      It can be changed, but be sure to stay within the IEEE BLE standars
#define BUS_STOP_PROFILE_SERV_UUID               0xFFF0

// Key Pressed UUID
// FB: These are the defaults from 'simple_gatt_profile.h'.
//      They can be changed, but be sure to stay within the IEEE BLE standards
#define BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_UUID       0xFFF1
#define BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT_UUID      0xFFF2



// Simple Keys Profile Services bit fields
// FB: This trivially gets bit-wise ANDed with 0xFFFFFFFF in an 'if' statement in SimpleProfile_AddService().
//      Thus, is really only meaningful if there is more than one 'service' - which there isn't.
//      As a hardware designer, I'm simply not confident enough to remove it.
//      That is left as future work for a dedicated software developer.
#define BUS_STOP_PROFILE_SERVICE               0x00000001

// FB: If these change, the initilization in the .c file also needs to be updated. Example,
//      'static uint8 handShakeProfilechar1[BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_LEN] = { 0, 0, 0, 0, 0 };'
//      & in #ifndef FEATURE_OAD_ONCHIP in simple_peripheral.c
#define BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_LEN        20
#define BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT_LEN       20

/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */

// Callback when a characteristic value has changed
typedef void (*simpleProfileChange_t)(uint8 paramID);

typedef struct
{
    simpleProfileChange_t pfnSimpleProfileChange; // Called when characteristic value changes
} simpleProfileCBs_t;



/*********************************************************************
 * API FUNCTIONS
 */


/*
 * SimpleProfile_AddService- Initializes the Simple GATT Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */

extern bStatus_t SimpleProfile_AddService(uint32 services);

/*
 * SimpleProfile_RegisterAppCBs - Registers the application callback function.
 *                    Only call this function once.
 *
 *    appCallbacks - pointer to application callbacks.
 */
extern bStatus_t SimpleProfile_RegisterAppCBs(simpleProfileCBs_t *appCallbacks);

/*
 * SimpleProfile_SetParameter - Set a Simple GATT Profile parameter.
 *
 *    param - Profile parameter ID
 *    len - length of data to right
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
extern bStatus_t SimpleProfile_SetParameter(uint8 param, uint8 len,

                                            void *value);

/*
 * SimpleProfile_GetParameter - Get a Simple GATT Profile parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
extern bStatus_t SimpleProfile_GetParameter(uint8 param, void *value);


/*********************************************************************
 *********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SIMPLEGATTPROFILE_H */
