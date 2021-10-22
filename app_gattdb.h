/***************************************************************************//**
 * @file
 * @brief GATT database declaration header file.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#ifndef APP_GATTDB_H
#define APP_GATTDB_H

#include "sl_bt_api.h"

#define UUID_16_LEN (2)
#define UUID_128_LEN (16)

//---------------------------------
// Structures
typedef struct service_s{
  uint8_t type;
  uint8_t property;
  size_t uuid_len;
  uint8_t *uuid;
  uint16_t handle;
} service_t;

typedef struct characteristic_s{
  service_t *service;
  uint16_t property;
  uint16_t security;
  uint8_t flag;
  size_t uuid_len;
  uint8_t *uuid;
  uint8_t value_type;
  uint16_t maxlen;
  size_t value_len;
  uint8_t *value;
  uint16_t handle;
} characteristic_t;

//---------------------------------
// Services
typedef enum {
  GENERIC_ACCESS,
  DEVICE_INFORMATION,
  SERVICES_COUNT
} service_index_t;
extern service_t services[SERVICES_COUNT];

//---------------------------------
// Characteristics
typedef enum {
  DEVICE_NAME,
  APPEARANCE,
  MANUFACTURER_NAME_STRING,
  SYSTEM_ID,
  CHARACTERISTICS_COUNT
} characteristic_index_t;
extern characteristic_t characteristics[CHARACTERISTICS_COUNT];

//---------------------------------
// Function headers
/***************************************************************************//**
 * Handles adding a new service.
 * @param[in] session The database update session ID.
 * @param[in] service The service attribute handle. Ensured to be valid in
 *   current session
 * @return SL_STATUS_OK if successful. Error code otherwise.
 ******************************************************************************/
sl_status_t app_gattdb_add_service(uint16_t session, service_t *service);

/***************************************************************************//**
 * Handles adding a new characteristic to a service.
 * @param[in] session The database update session ID.
 * @param[in] characteristic The characteristic attribute handle. Ensured to be
 *   valid in current session.
 * @return SL_STATUS_OK if successful. Error code otherwise.
 ******************************************************************************/
sl_status_t app_gattdb_add_characteristic(uint16_t session,
                                          characteristic_t *characteristic);

#endif // APP_GATTDB_H
