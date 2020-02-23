#ifndef _AD9850_H_
#define _AD9850_H_

#include "sys.h"

//GPIO”≥…‰
#define FQUP_9850 (PGout(8))
#define REST_9850 (PGout(6))
#define WCLK_9850 (PGout(4))
#define DATA_9850 (PGout(2))
#define D7_9850 (DATA)

#define HIGH 1
#define LOW 0
#define POWER_UP 1
#define POWER_DOWN 0
#define __WAIT (__NOP;__NOP;__NOP;__NOP;__NOP;)

void IO_AD9850_Init();
void AD9850_Init();
void Reset_9850();
void ChangeParToSer_9850();
void SendAD9850(u32 FreqNeed,u8 Power,u8 Phase);
void StableDATA_9850();

#endif