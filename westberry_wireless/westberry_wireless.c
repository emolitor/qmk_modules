// Copyright 2025 emolitor (github.com/emolitor)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "connection.h"
#include "module.h"

#ifndef MD_BT_NAME
#    define MD_BT_NAME "QMK BT$"
#endif

typedef union {
    uint32_t raw;
    struct {
        uint8_t devs : 3;
    };
} confinfo_t;
confinfo_t confinfo;

void eeconfig_init_kb(void) {
    confinfo.devs = DEVS_USB;
    eeconfig_update_kb(confinfo.raw);
    eeconfig_init_user();
}

void bluetooth_init(void) {
    md_init();
    md_send_devinfo(MD_BT_NAME);
    wait_ms(10);
}

void bluetooth_task(void) {
    md_main_task();
}

bool bluetooth_is_connected(void) {
    return *md_getp_state() == MD_STATE_CONNECTED;
}

bool bluetooth_can_send_nkro(void) {
    return true;
}

uint8_t bluetooth_keyboard_leds(void) {
    if (bluetooth_is_connected()) {
        return *md_getp_indicator();
    }

    return 0;
}

void bluetooth_send_keyboard(report_keyboard_t *report) {
    if (MD_STATE_PAIRING == *md_getp_state()) {
        return;
    }
    uint8_t wls_report_kb[MD_SND_CMD_KB_LEN] = {0};

    if (report != NULL) {
        memcpy(wls_report_kb, (uint8_t *)report, sizeof(wls_report_kb));
    }
    md_send_kb(wls_report_kb);
}

void bluetooth_send_nkro(report_nkro_t *report) {
    static report_keyboard_t temp_report_keyboard                 = {0};
    uint8_t                  wls_report_nkro[MD_SND_CMD_NKRO_LEN] = {0};

    if (MD_STATE_PAIRING == *md_getp_state()) {
        return;
    }

    if (report != NULL) {
        report_nkro_t temp_report_nkro = *report;
        uint8_t       key_count        = 0;

        temp_report_keyboard.mods = temp_report_nkro.mods;
        for (uint8_t i = 0; i < NKRO_REPORT_BITS; i++) {
            key_count += __builtin_popcount(temp_report_nkro.bits[i]);
        }

        // find key up and del it.
        for (uint8_t i = 0; i < KEYBOARD_REPORT_KEYS && temp_report_keyboard.keys[i]; i++) {
            uint8_t usageid = 0x00;
            uint8_t n;

            for (uint8_t c = 0; c < key_count; c++) {
                for (n = 0; n < NKRO_REPORT_BITS && !temp_report_nkro.bits[n]; n++) {
                }
                usageid = (n << 3) | biton(temp_report_nkro.bits[n]);
#ifdef NKRO_ENABLE
                del_key_bit(&temp_report_nkro, usageid);
#endif
                if (usageid == temp_report_keyboard.keys[i]) {
                    break;
                }
            }

            if (usageid != temp_report_keyboard.keys[i]) {
                temp_report_keyboard.keys[i] = 0x00;
            }
        }

        // Use NKRO for sending when more than 6 keys are pressed
        // to solve the issue of the lack of a protocol flag in wireless mode.

        temp_report_nkro = *report;

        for (uint8_t i = 0; i < key_count; i++) {
            uint8_t usageid;
            uint8_t idx, n = 0;

            for (n = 0; n < NKRO_REPORT_BITS && !temp_report_nkro.bits[n]; n++) {
            }
            usageid = (n << 3) | biton(temp_report_nkro.bits[n]);
#ifdef NKRO_ENABLE
            del_key_bit(&temp_report_nkro, usageid);
#endif

            for (idx = 0; idx < KEYBOARD_REPORT_KEYS; idx++) {
                if (temp_report_keyboard.keys[idx] == usageid) {
                    break;
                }
                if (temp_report_keyboard.keys[idx] == 0x00) {
                    temp_report_keyboard.keys[idx] = usageid;
                    break;
                }
            }

            if (idx == KEYBOARD_REPORT_KEYS && (usageid < (MD_SND_CMD_NKRO_LEN * 8))) {
                wls_report_nkro[usageid / 8] |= 0x01 << (usageid % 8);
            }
        }
    } else {
        memset(&temp_report_keyboard, 0, sizeof(temp_report_keyboard));
    }

    bluetooth_send_keyboard(&temp_report_keyboard);
    host_keyboard_send(&temp_report_keyboard);
    md_send_nkro(wls_report_nkro);
}

void bluetooth_send_mouse(report_mouse_t *report) {
    if (MD_STATE_PAIRING == *md_getp_state()) {
        return;
    }

    typedef struct {
        uint8_t buttons;
        int8_t  x;
        int8_t  y;
        int8_t  z;
        int8_t  h;
    } __attribute__((packed)) wls_report_mouse_t;

    wls_report_mouse_t wls_report_mouse = {0};

    if (report != NULL) {
        wls_report_mouse.buttons = report->buttons;
        wls_report_mouse.x       = report->x;
        wls_report_mouse.y       = report->y;
        wls_report_mouse.z       = report->h;
        wls_report_mouse.h       = report->v;
    }

    md_send_mouse((uint8_t *)&wls_report_mouse);
}

void wireless_devs_change(uint8_t new_devs, bool reset) {
    if ((confinfo.devs != new_devs) || reset) {
        *md_getp_state()     = MD_STATE_DISCONNECTED;
        *md_getp_indicator() = 0;
    }

    if (new_devs == DEVS_USB) {
        connection_set_host(CONNECTION_HOST_USB);
    } else {
        connection_set_host(CONNECTION_HOST_BLUETOOTH);
        md_devs_change(new_devs, reset);
    }

    confinfo.devs = new_devs;
    eeconfig_update_kb(confinfo.raw);
}

void suspend_wakeup_init_westberry_wireless(void) {
    wireless_devs_change(confinfo.devs, false);
}

void keyboard_post_init_westberry_wireless(void) {
    confinfo.raw = eeconfig_read_kb();
    if (!confinfo.raw) {
        eeconfig_init_kb();
    }

    md_send_devctrl(MD_SND_CMD_DEVCTRL_FW_VERSION);   // get the module fw version.
    md_send_devctrl(MD_SND_CMD_DEVCTRL_SLEEP_BT_EN);  // timeout 30min to sleep in bt mode, enable
    md_send_devctrl(MD_SND_CMD_DEVCTRL_SLEEP_2G4_EN); // timeout 30min to sleep in 2.4g mode, enable
    wireless_devs_change(confinfo.devs, false);
}

bool process_record_westberry_wireless(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case LT(0, BT_PRF1): {
            if (record->tap.count && record->event.pressed) {
                wireless_devs_change(DEVS_BT1, false);
            } else if (record->event.pressed && *md_getp_state() != MD_STATE_PAIRING) {
                wireless_devs_change(DEVS_BT1, true);
            }
            return false;
        }
        case LT(0, BT_PRF2): {
            if (record->tap.count && record->event.pressed) {
                wireless_devs_change(DEVS_BT2, false);
            } else if (record->event.pressed && *md_getp_state() != MD_STATE_PAIRING) {
                wireless_devs_change(DEVS_BT2, true);
            }
            return false;
        }
        case LT(0, BT_PRF3): {
            if (record->tap.count && record->event.pressed) {
                wireless_devs_change(DEVS_BT3, false);
            } else if (record->event.pressed && *md_getp_state() != MD_STATE_PAIRING) {
                wireless_devs_change(DEVS_BT3, true);
            }
            return false;
        }
        case LT(0, BT_PRF4): {
            if (record->tap.count && record->event.pressed) {
                wireless_devs_change(DEVS_BT4, false);
            } else if (record->event.pressed && *md_getp_state() != MD_STATE_PAIRING) {
                wireless_devs_change(DEVS_BT4, true);
            }
            return false;
        }
        case LT(0, BT_PRF5): {
            if (record->tap.count && record->event.pressed) {
                wireless_devs_change(DEVS_BT5, false);
            } else if (record->event.pressed && *md_getp_state() != MD_STATE_PAIRING) {
                wireless_devs_change(DEVS_BT5, true);
            }
            return false;
        }
        case LT(0, OU_2P4G): {
            if (record->tap.count && record->event.pressed) {
                wireless_devs_change(DEVS_2G4, false);
            } else if (record->event.pressed && *md_getp_state() != MD_STATE_PAIRING) {
                wireless_devs_change(DEVS_2G4, true);
            }
            return false;
        }
    }

    return true;
}
