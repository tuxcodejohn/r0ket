#include <sysinit.h>

#include "core/cpu/cpu.h"
#include "core/pmu/pmu.h"

#include "basic/basic.h"
#include "lcd/render.h"
#include "filesystem/ff.h"


#ifdef CRP1
#define CRP_VALUE 0x12345678  // CRP1
#endif

#ifdef CRP2
#define CRP_VALUE 0x87654321  // CRP2
#endif

#ifdef CRP3
#define CRP_VALUE 0x43218765  // CRP3
#endif

#ifdef NO_ISP
#define CRP_VALUE 0x4e697370  // NO_ISP
#endif

#ifndef CRP_VALUE
#define CRP_VALUE 0x0  // ANY non-magic value disables CRP
#endif

__attribute__ ((used, section("crp"))) const uint32_t the_crp=CRP_VALUE;

/**************************************************************************/

void wrapper(void);

int main(void) {
    // Configure cpu and mandatory peripherals
    cpuInit();                                // Configure the CPU
// we do it later
//    systickInit(CFG_SYSTICK_DELAY_IN_MS);     // Start systick timer
// cpuInit already calls this
//    gpioInit();                               // Enable GPIO 
    pmuInit();                                // Configure power management
    adcInit();                                // Config adc pins to save power

    // initialise basic badge functions
    rbInit();

    fsInit();
  
    lcdInit(); // display

    lcdFill(0);
    lcdDisplay();

	wrapper(); // see module/ subdirectory
}
