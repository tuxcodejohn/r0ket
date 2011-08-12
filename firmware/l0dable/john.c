#include <sysinit.h>
#include <string.h>

#include "basic/basic.h"
#include "basic/config.h"

#include "lcd/render.h"
#include "lcd/print.h"



#include "funk/mesh.h"

#include "usetable.h"

/**************************************************************************/

void ram(void) {
   	MPKT * mpkt= meshGetMessage('m');
  //  do{
        lcdClear();
        lcdPrintln("ill mesh up");
        MO_TIME_set(mpkt->pkt,424255);
    strcpy((char*)MO_BODY(mpkt->pkt),"<<</>>");
        lcdNl();
    
 
    //} while ((getInputWaitTimeout(242))==BTN_NONE);
   return;
}
