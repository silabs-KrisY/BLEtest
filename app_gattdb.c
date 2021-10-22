/***************************************************************************//**
 * @file
 * @brief GATT database declaration source file.
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
#include "app_gattdb.h"

#define GATTDB_SECURITY_NONE            0x00
#define GATTDB_FLAG_NONE                0x00
#define GATTDB_ADVERTISED_SERVICE_NONE  0x00

//---------------------------------
// Services
uint8_t generic_access_service_uuid[] = { 0x00, 0x18 };
uint8_t device_information_service_uuid[] = { 0x0A, 0x18 };

service_t services[SERVICES_COUNT] = {
  {
    // GENERIC_ACCESS
    .type = sl_bt_gattdb_primary_service,
    .property = GATTDB_ADVERTISED_SERVICE_NONE,
    .uuid_len = sizeof(generic_access_service_uuid),
    .uuid = generic_access_service_uuid,
    .handle = 0xFFFF
  },
  {
    // DEVICE_INFORMATION
    .type = sl_bt_gattdb_primary_service,
    .property = GATTDB_ADVERTISED_SERVICE_NONE,
    .uuid_len = sizeof(device_information_service_uuid),
    .uuid = device_information_service_uuid,
    .handle = 0xFFFF
  }
};

//---------------------------------
// Characteristics
uint8_t device_name_characteristic_uuid[] = { 0x00, 0x2A };
char device_name_characteristic_value[] = "Silabs Example";
uint8_t appearance_characteristic_uuid[] = { 0x01, 0x2A };
uint8_t appearance_characteristic_value[] = { 0x00, 0x00 };
uint8_t manufacturer_name_string_characteristic_uuid[] = { 0x29, 0x2A };
char manufacturer_name_string_characteristic_value[] = "Silicon Labs";
uint8_t system_id_characteristic_uuid[] = { 0x23, 0x2A };
uint8_t system_id_characteristic_value[8];

characteristic_t characteristics[CHARACTERISTICS_COUNT] = {
  {
    // DEVICE_NAME
    .service = &services[GENERIC_ACCESS],
    .property = (SL_BT_GATTDB_CHARACTERISTIC_READ | SL_BT_GATTDB_CHARACTERISTIC_WRITE),
    .security = GATTDB_SECURITY_NONE,
    .flag = GATTDB_FLAG_NONE,
    .uuid_len = sizeof(device_name_characteristic_uuid),
    .uuid = device_name_characteristic_uuid,
    .value_type = sl_bt_gattdb_fixed_length_value,
    .maxlen = sizeof(device_name_characteristic_value),
    .value_len = sizeof(device_name_characteristic_value),
    .value = (uint8_t *)&device_name_characteristic_value,
    .handle = 0xFFFF
  },
  {
    // APPEARANCE
    .service = &services[GENERIC_ACCESS],
    .property = SL_BT_GATTDB_CHARACTERISTIC_READ,
    .security = GATTDB_SECURITY_NONE,
    .flag = GATTDB_FLAG_NONE,
    .uuid_len = sizeof(appearance_characteristic_uuid),
    .uuid = appearance_characteristic_uuid,
    .value_type = sl_bt_gattdb_fixed_length_value,
    .maxlen = sizeof(appearance_characteristic_value),
    .value_len = sizeof(appearance_characteristic_value),
    .value = appearance_characteristic_value,
    .handle = 0xFFFF
  },
  {
    // MANUFACTURER_NAME_STRING
    .service = &services[DEVICE_INFORMATION],
    .property = SL_BT_GATTDB_CHARACTERISTIC_READ,
    .security = GATTDB_SECURITY_NONE,
    .flag = GATTDB_FLAG_NONE,
    .uuid_len = sizeof(manufacturer_name_string_characteristic_uuid),
    .uuid = manufacturer_name_string_characteristic_uuid,
    .value_type = sl_bt_gattdb_fixed_length_value,
    .maxlen = sizeof(manufacturer_name_string_characteristic_value),
    .value_len = sizeof(manufacturer_name_string_characteristic_value),
    .value = (uint8_t *)&manufacturer_name_string_characteristic_value,
    .handle = 0xFFFF
  },
  {
    // SYSTEM_ID
    .service = &services[DEVICE_INFORMATION],
    .property = SL_BT_GATTDB_CHARACTERISTIC_READ,
    .security = GATTDB_SECURITY_NONE,
    .flag = GATTDB_FLAG_NONE,
    .uuid_len = sizeof(system_id_characteristic_uuid),
    .uuid = system_id_characteristic_uuid,
    .value_type = sl_bt_gattdb_fixed_length_value,
    .maxlen = sizeof(system_id_characteristic_value),
    .value_len = sizeof(system_id_characteristic_value),
    .value = system_id_characteristic_value,
    .handle = 0xFFFF
  }
};

//---------------------------------
// Handles adding a new service.
sl_status_t app_gattdb_add_service(uint16_t session, service_t *service)
{
  return sl_bt_gattdb_add_service(session,
                                  service->type,
                                  service->property,
                                  service->uuid_len,
                                  service->uuid,
                                  &service->handle);
}

// Handles adding a new characteristic to a service.
sl_status_t app_gattdb_add_characteristic(uint16_t session,
                                          characteristic_t *characteristic)
{
  if (characteristic->uuid_len == UUID_16_LEN) {
    sl_bt_uuid_16_t uuid;
    memcpy(uuid.data, characteristic->uuid, characteristic->uuid_len);
    return sl_bt_gattdb_add_uuid16_characteristic(session,
                                                  characteristic->service->handle,
                                                  characteristic->property,
                                                  characteristic->security,
                                                  characteristic->flag,
                                                  uuid,
                                                  characteristic->value_type,
                                                  characteristic->maxlen,
                                                  characteristic->value_len,
                                                  characteristic->value,
                                                  &characteristic->handle);
  } else if (characteristic->uuid_len == UUID_128_LEN) {
    uuid_128 uuid;
    memcpy(uuid.data, characteristic->uuid, characteristic->uuid_len);
    return sl_bt_gattdb_add_uuid128_characteristic(session,
                                                   characteristic->service->handle,
                                                   characteristic->property,
                                                   characteristic->security,
                                                   characteristic->flag,
                                                   uuid,
                                                   characteristic->value_type,
                                                   characteristic->maxlen,
                                                   characteristic->value_len,
                                                   characteristic->value,
                                                   &characteristic->handle);
  } else {
    return SL_STATUS_INVALID_PARAMETER;
  }
}
