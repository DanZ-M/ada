/**************************************************************************/
/*!
    @file     usb.c
    @author   hathach (tinyusb.org)

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2018, Adafruit Industries (adafruit.com)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/

#ifdef SOFTDEVICE_PRESENT
#include "nrf_sdm.h"
#include "nrf_soc.h"
#endif

#include "nrf_usbd.h"
#include "tusb.h"

// TODO fully move to nrfx
enum {
    NRFX_POWER_USB_EVT_DETECTED, /**< USB power detected on the connector (plugged in). */
    NRFX_POWER_USB_EVT_REMOVED,  /**< USB power removed from the connector. */
    NRFX_POWER_USB_EVT_READY     /**< USB power regulator ready. */
};

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

/* tinyusb function that handles power event (detected, ready, removed)
 * We must call it within SD's SOC event handler, or set it as power event handler if SD is not enabled. */
extern void tusb_hal_nrf_power_event(uint32_t event);


//------------- IMPLEMENTATION -------------//
void usb_init(void)
{
  // USB power may already be ready at this time -> no event generated
  // We need to invoke the handler based on the status initially
  uint32_t usb_reg;

#ifdef SOFTDEVICE_PRESENT
  uint8_t sd_en = false;
  (void) sd_softdevice_is_enabled(&sd_en);

  if ( sd_en ) {
    sd_power_usbdetected_enable(true);
    sd_power_usbpwrrdy_enable(true);
    sd_power_usbremoved_enable(true);

    sd_power_usbregstatus_get(&usb_reg);
  }else
#endif
  {
#if 0 // TODO enable
    // Power module init
    const nrfx_power_config_t pwr_cfg = { 0 };
    nrfx_power_init(&pwr_cfg);

    // Register tusb function as USB power handler
    const nrfx_power_usbevt_config_t config = { .handler = (nrfx_power_usb_event_handler_t) tusb_hal_nrf_power_event };
    nrfx_power_usbevt_init(&config);

    nrfx_power_usbevt_enable();

    usb_reg = NRF_POWER->USBREGSTATUS;
#endif
  }

  if ( usb_reg & POWER_USBREGSTATUS_VBUSDETECT_Msk ) {
    tusb_hal_nrf_power_event(NRFX_POWER_USB_EVT_DETECTED);
  }

  if ( usb_reg & POWER_USBREGSTATUS_OUTPUTRDY_Msk ) {
    tusb_hal_nrf_power_event(NRFX_POWER_USB_EVT_READY);
  }

  // Init tusb stack
  tusb_init();
}

void usb_teardown(void)
{
  if ( NRF_USBD->ENABLE )
  {
    // Abort all transfers

    // Disable pull up
    nrf_usbd_pullup_disable();

    // Disable Interrupt
    NVIC_DisableIRQ(USBD_IRQn);

    // disable all interrupt
    NRF_USBD->INTENCLR = NRF_USBD->INTEN;

    nrf_usbd_disable();
    sd_clock_hfclk_release();

    sd_power_usbdetected_enable(false);
    sd_power_usbpwrrdy_enable(false);
    sd_power_usbremoved_enable(false);
  }
}

//--------------------------------------------------------------------+
// tinyusb callbacks
//--------------------------------------------------------------------+
void tud_mount_cb(void)
{

}

void tud_umount_cb(void)
{

}
