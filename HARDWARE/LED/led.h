#ifndef __LED_H
#define __LED_H

#include "sys.h"

#define LED0 PBout(5) // PB5£¬ºìÉ«
#define LED1 PEout(5) // PE5£¬ÂÌÉ«

void LED_Init(void);

#endif
