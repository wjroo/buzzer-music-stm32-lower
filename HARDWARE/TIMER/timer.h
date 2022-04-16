#ifndef __TIMER_H
#define __TIMER_H

#include "sys.h"

#define NOTES 1700 //NOTES¥Û‘ºUSART_REC_LEN/10°£
#define PRESCALE 41

extern u16 slot_ms;
extern u8 play_notes;

void TIM_PWM_Init(u16 arr, u16 psc);

void buzzerSound(u16 fre1, u16 fre2, u16 fre3, u16 fre4, u16 fre5, u16 fre6);

void stop(void);

void play(u16 music[NOTES][8], u16 length);

#endif
