#ifndef _AD9851_H_
#define _AD9851_H_

#include "sys.h"

//GPIO”≥…‰
#define FQUP (PFout(3))
#define REST (PFout(5))
#define WCLK (PFout(7))
#define DATA (PFout(9))
#define D7 (DATA)

#define HIGH 1
#define LOW 0
#define REFCLK_ENABLE 1
#define REFCLK_DISABLE 0
#define POWER_UP 1
#define POWER_DOWN 0
#define __WAIT __NOP;__NOP;__NOP;__NOP;__NOP;

extern void IO_Init();
extern void AD9851_Init();
extern void Reset();
extern void ChangeParToSer();
extern void Send40bits(u32 FreqNeed,u8 REFCLK,u8 Power,u8 Phase);
extern void StableDATA();

#endif