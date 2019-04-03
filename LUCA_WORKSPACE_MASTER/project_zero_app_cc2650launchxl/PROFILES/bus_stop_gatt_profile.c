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

/*********************************************************************
 * INCLUDES
 */
#include <string.h>

#include "bcomdef.h"
#include "osal.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"

#include "bus_stop.h"

#include <ti/mw/display/Display.h>

#include "bus_stop_gatt_profile.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define SERVAPP_NUM_ATTR_SUPPORTED        8

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Simple GATT Profile Service UUID: 0xFFF0
CONST uint8 busStopProfileServUUID[ATT_BT_UUID_SIZE] =
{
 LO_UINT16(BUS_STOP_PROFILE_SERV_UUID), HI_UINT16(BUS_STOP_PROFILE_SERV_UUID)
};

// Characteristic 1 UUID: 0xFFF1
CONST uint8 busStopProfilechar1UUID[ATT_BT_UUID_SIZE] =
{
 LO_UINT16(BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_UUID), HI_UINT16(BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_UUID)
};

// Characteristic 2 UUID: 0xFFF2
CONST uint8 busStopProfilechar2UUID[ATT_BT_UUID_SIZE] =
{
 LO_UINT16(BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT_UUID), HI_UINT16(BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT_UUID)
};


/*********************************************************************
 * EXTERNAL VARIABLES
 */

extern Display_Handle dispHandle;

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static simpleProfileCBs_t *simpleProfile_AppCBs = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// Bus Stop Profile Service attribute
static CONST gattAttrType_t busStopProfileService = { ATT_BT_UUID_SIZE, busStopProfileServUUID };

// Bus Stop Profile Characteristic 1 Properties
static uint8 busStopProfileChar1Props = GATT_PROP_READ | GATT_PROP_WRITE;
// Characteristic 1 Value
static uint8 busStopProfileChar1[BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_LEN]
                                                 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// Bus Stop Profile Characteristic 1 User Description
static uint8 busStopProfileChar1UserDesp[15] = "Client Data In";



// Bus Stop Profile Characteristic 2 Properties
static uint8 busStopProfileChar2Props = GATT_PROP_NOTIFY;
// Characteristic 4 Value
static uint8 busStopProfileChar2[BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT_LEN]
                                                  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// Bus Stop Profile Characteristic 2 Configuration Each client has its own
// instantiation of the Client Characteristic Configuration. Reads of the
// Client Characteristic Configuration only shows the configuration for
// that client and writes only affect the configuration of that client.
//  FB: That simply means many devices can 'subscribe' to the noticiaftions,
//      but they each get their own notifications, so they each need their
//      own 'structure' (or other more-correct programming term)
static gattCharCfg_t *busStopProfileChar2Config;
// Bus Stop Profile Characteristic 2 User Description
static uint8 busStopProfileChar2UserDesp[16] = "Client Data Out";


/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t simpleProfileAttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] =
{
 // Bus Stop Profile Service
 {
  { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
  GATT_PERMIT_READ,                         /* permissions */
  0,                                        /* handle */
  (uint8 *)&busStopProfileService            /* pValue */
 },

 // Characteristic 1 Declaration
 {
  { ATT_BT_UUID_SIZE, characterUUID },
  GATT_PERMIT_READ,
  0,
  &busStopProfileChar1Props//GATT_PROP_READ | GATT_PROP_WRITE
 },

 // Characteristic Value 1
 {
  { ATT_BT_UUID_SIZE, busStopProfilechar1UUID },
  GATT_PERMIT_READ | GATT_PERMIT_WRITE,
  0,
  busStopProfileChar1//value store
 },

 // Characteristic 1 User Description
 {
  { ATT_BT_UUID_SIZE, charUserDescUUID },
  GATT_PERMIT_READ,
  0,
  busStopProfileChar1UserDesp//client data in
 },

 // Characteristic 2 Declaration
 {
  { ATT_BT_UUID_SIZE, characterUUID },
  GATT_PERMIT_READ,
  0,
  &busStopProfileChar2Props//GATT_PROP_NOTIFY
 },

 // Characteristic Value 2
 {
  { ATT_BT_UUID_SIZE, busStopProfilechar2UUID },
  0,
  0,
  busStopProfileChar2//value store
 },

 // Characteristic 2 configuration
 {
  { ATT_BT_UUID_SIZE, clientCharCfgUUID },
  GATT_PERMIT_READ | GATT_PERMIT_WRITE,
  0,
  (uint8 *)&busStopProfileChar2Config//Client Characteristic Configuration
 },

 // Characteristic 2 User Description
 {
  { ATT_BT_UUID_SIZE, charUserDescUUID },
  GATT_PERMIT_READ,
  0,
  busStopProfileChar2UserDesp//client data out
 },
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t simpleProfile_ReadAttrCB(uint16_t connHandle,
                                          gattAttribute_t *pAttr,
                                          uint8_t *pValue, uint16_t *pLen,
                                          uint16_t offset, uint16_t maxLen,
                                          uint8_t method);
static bStatus_t simpleProfile_WriteAttrCB(uint16_t connHandle,
                                           gattAttribute_t *pAttr,
                                           uint8_t *pValue, uint16_t len,
                                           uint16_t offset, uint8_t method);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// Bus Stop Profile Service Callbacks
// Note: When an operation on a characteristic requires authorization and
// pfnAuthorizeAttrCB is not defined for that characteristic's service, the
// Stack will report a status of ATT_ERR_UNLIKELY to the client.  When an
// operation on a characteristic requires authorization the Stack will call
// pfnAuthorizeAttrCB to check a client's authorization prior to calling
// pfnReadAttrCB or pfnWriteAttrCB, so no checks for authorization need to be
// made within these functions.
CONST gattServiceCBs_t simpleProfileCBs =
{
 simpleProfile_ReadAttrCB,  // Read callback function pointer
 simpleProfile_WriteAttrCB, // Write callback function pointer
 NULL                       // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SimpleProfile_AddService
 *
 * @brief   Initializes the Simple Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t SimpleProfile_AddService( uint32 services )
{
    uint8 status;

    // Allocate Client Characteristic Configuration table
    busStopProfileChar2Config = (gattCharCfg_t *)ICall_malloc( sizeof(gattCharCfg_t) *
                                                              linkDBNumConns );
    if ( busStopProfileChar2Config == NULL )
    {
        return ( bleMemAllocError );
    }

    // Initialize Client Characteristic Configuration attributes
    GATTServApp_InitCharCfg( INVALID_CONNHANDLE, busStopProfileChar2Config );

    if ( services & BUS_STOP_PROFILE_SERV_UUID )
    {
        // Register GATT attribute list and CBs with GATT Server App
        status = GATTServApp_RegisterService( simpleProfileAttrTbl,
                                              GATT_NUM_ATTRS( simpleProfileAttrTbl ),
                                              GATT_MAX_ENCRYPT_KEY_SIZE,
                                              &simpleProfileCBs );
    }
    else
    {
        status = SUCCESS;
    }

    return ( status );
}

/*********************************************************************
 * @fn      SimpleProfile_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t SimpleProfile_RegisterAppCBs( simpleProfileCBs_t *appCallbacks )
{
    if ( appCallbacks )
    {
        simpleProfile_AppCBs = appCallbacks;

        return ( SUCCESS );
    }
    else
    {
        return ( bleAlreadyInRequestedMode );
    }
}

/*********************************************************************
 * @fn      SimpleProfile_SetParameter
 *
 * @brief   Set a Simple Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to write
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SimpleProfile_SetParameter( uint8 param, uint8 len, void *value )
{
    bStatus_t ret = SUCCESS;
    switch ( param )
    {

    case BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN:
        if ( len == BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_LEN )
        {
            VOID memcpy( busStopProfileChar1, value, BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_LEN );
        }
        else
        {
            ret = bleInvalidRange;
        }
        break;

    case BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT:
        if ( len == BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT_LEN )
        {
            VOID memcpy( busStopProfileChar2, value, BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT_LEN );

            // See if Notification has been enabled
            GATTServApp_ProcessCharCfg( busStopProfileChar2Config, busStopProfileChar2, FALSE,
                                        simpleProfileAttrTbl, GATT_NUM_ATTRS( simpleProfileAttrTbl ),
                                        INVALID_TASK_ID, simpleProfile_ReadAttrCB );
        }
        else
        {
            ret = bleInvalidRange;
        }
        break;

    default:
        ret = INVALIDPARAMETER;
        break;
    }

    return ( ret );
}

/*********************************************************************
 * @fn      SimpleProfile_GetParameter
 *
 * @brief   Get a Bus Stop Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SimpleProfile_GetParameter( uint8 param, void *value )
{
    bStatus_t ret = SUCCESS;
    switch ( param )
    {

    case BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT:
        VOID memcpy( value, busStopProfileChar2, BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT_LEN );
        break;

    case BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN:
        VOID memcpy( value, busStopProfileChar1, BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_LEN );
        break;


    default:
        ret = INVALIDPARAMETER;
        break;
    }

    return ( ret );
}

/*********************************************************************
 * @fn          simpleProfile_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 * @param       method - type of read message
 *
 * @return      SUCCESS, blePending or Failure
 */
static bStatus_t simpleProfile_ReadAttrCB(uint16_t connHandle,
                                          gattAttribute_t *pAttr,
                                          uint8_t *pValue, uint16_t *pLen,
                                          uint16_t offset, uint16_t maxLen,
                                          uint8_t method)
{
    bStatus_t status = SUCCESS;

    // Make sure it's not a blob operation (no attributes in the profile are long)
    if ( offset > 0 )
    {
        return ( ATT_ERR_ATTR_NOT_LONG );
    }

    if ( pAttr->type.len == ATT_BT_UUID_SIZE )
    {
        // 16-bit UUID
        uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
        switch ( uuid )
        {
        // No need for "GATT_SERVICE_UUID" or "GATT_CLIENT_CHAR_CFG_UUID" cases;
        // gattserverapp handles those reads


        case BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_UUID:
            *pLen = BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_LEN;
            VOID memcpy( pValue, pAttr->pValue, BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_LEN );
            break;

        case BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT_UUID:
            *pLen = BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT_LEN;
            VOID memcpy( pValue, pAttr->pValue, BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT_LEN );
            break;

        default:
            // Should never get here! (characteristics 3 and 4 do not have read permissions)
            *pLen = 0;
            status = ATT_ERR_ATTR_NOT_FOUND;
            break;
        }
    }
    else
    {
        // 128-bit UUID
        *pLen = 0;
        status = ATT_ERR_INVALID_HANDLE;
    }

    return ( status );
}

/*********************************************************************
 * @fn      simpleProfile_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 * @param   method - type of write message
 *
 * @return  SUCCESS, blePending or Failure
 */
static bStatus_t simpleProfile_WriteAttrCB(uint16_t connHandle,
                                           gattAttribute_t *pAttr,
                                           uint8_t *pValue, uint16_t len,
                                           uint16_t offset, uint8_t method)
{
    bStatus_t status = SUCCESS;
    uint8 notifyApp = 0xFF;//paramID


    Display_print0(dispHandle, 0, 0, "WriteAttrCB called");
    if ( pAttr->type.len == ATT_BT_UUID_SIZE )
    {
        // 16-bit UUID
        uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
        switch ( uuid )
        {

        case BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_UUID:
            Display_print0(dispHandle, 0, 0, "  Caught char1");
            if ( offset == 0 )
            {
                if ( len > BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN_LEN )
                {
                    status = ATT_ERR_INVALID_VALUE_SIZE;
                }
            }
            else
            {
                status = ATT_ERR_ATTR_NOT_LONG;
            }
            // Write the value
            if (status == SUCCESS)
            {
                BusStop_HandleMessage(pValue, len);

                uint8 *pCurValue = (uint8 *)pAttr->pValue;
                memcpy(pCurValue, reply_msg, 20);
                notifyApp = BUS_STOP_PROFILE_CHARACTERISTIC_1_CLIENT_DATA_IN;
                Display_print0(dispHandle, 0, 0, "    memcpy good");

            }
            break;


        case BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT_UUID:
            Display_print0(dispHandle, 0, 0, "  Caught char2");
            if ( offset == 0 )
            {
                if ( len > BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT_LEN )
                {
                    status = ATT_ERR_INVALID_VALUE_SIZE;
                }
            }
            else
            {
                status = ATT_ERR_ATTR_NOT_LONG;
            }
            // Write the value
            if (status == SUCCESS)
            {
                uint8 *pCurValue = (uint8 *)pAttr->pValue;
                memcpy(pCurValue, pValue, len);
                notifyApp = BUS_STOP_PROFILE_CHARACTERISTIC_2_CLIENT_DATA_OUT;
                Display_print0(dispHandle, 0, 0, "    memcpy good");
            }
            break;



        case GATT_CLIENT_CHAR_CFG_UUID:
            status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                                     offset, GATT_CLIENT_CFG_NOTIFY );
            break;

        default:
            // Should never get here! (characteristics 2 and 4 do not have write permissions)
            status = ATT_ERR_ATTR_NOT_FOUND;
            break;
        }
    }
    else
    {
        // 128-bit UUID
        status = ATT_ERR_INVALID_HANDLE;
    }

    // If a characteristic value changed then callback function to notify application of change
    if ( (notifyApp != 0xFF ) && simpleProfile_AppCBs && simpleProfile_AppCBs->pfnSimpleProfileChange )
    {
        simpleProfile_AppCBs->pfnSimpleProfileChange( notifyApp );
        Display_print0(dispHandle, 0, 0, "      AppCB called");
    }

    return ( status );
}

/*********************************************************************
 *********************************************************************/
