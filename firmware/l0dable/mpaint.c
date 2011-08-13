#include <sysinit.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "basic/basic.h"
#include "lcd/render.h"
#include "lcd/allfonts.h"
#include "basic/ecc.h"
#include "funk/nrf24l01p.h"
#include "filesystem/ff.h"
#include "filesystem/diskio.h"
#include "funk/filetransfer.h"
#include "lcd/print.h"
#include <string.h>
#include "funk/nrf24l01p.h"
#include "funk/filetransfer.h"
#include "funk/rftransfer.h"
#include "basic/basic.h"
#include "basic/xxtea.h"
#include "filesystem/ff.h"
#include "funk/rftransfer.h"
#include "funk/nrf24l01p.h"
#include <basic/basic.h>
#include <basic/random.h>
#include <core/systick/systick.h>
//#include <lcd/print.h>

#include "usetable.h"

struct NRF_CFG config = {
    .channel= 81,
    .txmac= "\x2\x2\x1\x2\x2",
    .nrmacs=1,
    .mac0=  "\x2\x2\x1\x2\x2",
    .maclen ="\x20",
};

struct pkt {
    uint8_t x, y, d;
};

void ram() {
    int x = 48, y = 32;
    bool drawing = true;
    struct pkt pkt;

    lcdFill(0);

    while(1) {
        /* Move */
        uint8_t key = getInputWaitTimeout(50);
        switch(key) {
        case BTN_UP:
            y--;
            break;
        case BTN_DOWN:
            y++;
            break;
        case BTN_LEFT:
            x--;
            break;
        case BTN_RIGHT:
            x++;
            break;
        case BTN_ENTER:
            drawing = !drawing;
            break;
        }

        x %= RESX;
        y %= RESY;

        /* Draw */
        lcdSetPixel(x, y, drawing);

        /* Send */
        pkt.x = x;
        pkt.y = y;
        pkt.d = drawing;
        nrf_config_set(&config);
        nrf_snd_pkt_crc(sizeof(pkt), (uint8_t *)&pkt);

        /* Receive */
        nrf_config_set(&config);
        int n = nrf_rcv_pkt_time(20, sizeof(pkt), (uint8_t *)&pkt);
        if (n == sizeof(pkt)) {
            lcdSetPixel(pkt.x, pkt.y, pkt.d);
        }

        lcdDisplay();
    }
}
