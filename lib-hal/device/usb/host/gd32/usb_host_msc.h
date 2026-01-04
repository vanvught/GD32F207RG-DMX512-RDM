#pragma once
/**
 * usb_host_msc.h
 *
 */
/* Copyright (C) 2023-2025 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
 #include <cstdint>

#include "usbh_core.h"
#include "usb_conf.h"

/* function declarations */
/* user operation for host-mode initialization */
void usbh_user_init();
/* deinitialize user state and associated variables */
void usbh_user_deinit();
/* user operation for device attached */
void usbh_user_device_connected();
/* user operation for reset USB Device */
void usbh_user_device_reset();
/* user operation for device disconnect event */
void usbh_user_device_disconnected();
/* user operation for device over current detection event */
void usbh_user_over_current_detected();
/* user operation for detecting device speed */
void usbh_user_device_speed_detected(uint32_t device_speed);
/* user operation when device descriptor is available */
void usbh_user_device_desc_available(void*);
/* usb device is successfully assigned the address  */
void usbh_user_device_address_assigned();
/* user operation when configuration descriptor is available */
void usbh_user_configuration_descavailable(usb_desc_config* cfg_desc, usb_desc_itf* itf_desc, usb_desc_ep* ep_desc);
/* user operation when manufacturer string exists */
void usbh_user_manufacturer_string(void*);
/* user operation when product string exists */
void usbh_user_product_string(void*);
/* user operation when serialNum string exists */
void usbh_user_serialnum_string(void*);
/* user response request is displayed to ask for application jump to class */
void usbh_user_enumeration_finish();
/* user action for application state entry */
usbh_user_status usbh_user_userinput();
/* user operation when device is not supported */
void usbh_user_device_not_supported();
/* user operation when unrecovered error happens */
void usbh_user_unrecovered_error();
/* demo application for mass storage */
int usbh_usr_msc_application();

