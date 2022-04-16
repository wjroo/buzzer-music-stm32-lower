#include "timer.h"
#include "delay.h"
#include "usart.h"

/*

关于“STM32用一个定时器输出多路不同频率及占空比的PWM（输出比较模式）”

我们使用STM32输出PWM时会使用定时器的PWM输出模式来进行生成，但是这样子生成PWM是有局限的，
它只能生成四路频率相同的PWM，当你设定了TIMx_PSC（预分频寄存器）和TIMx_ARR（自动重装载寄存器），
这时PWM的频率就被定下来了，为系统的时钟/TIMx_PSC+1/TIMx_ARR+1，你可以通过改变各个通道的CCR寄存器来改变占空比。
但是如果我们想生成多路不同频率的PWM的话，使用这个方法只能使用多个定时器了，
这样对于定时器资源较少的板子无疑是不可取的。32定时器有一个输出比较的模式，可以生成多路不同频率及占空比的PWM。

注意！！！
使用输出比较的方法可以在使用1个定时器的情况下有效的生成两路不同频率及占空比的PWM，
它对比PWM输出模式的缺点肯定就是它会有中断的处理，如果生成的PWM频率较高时它会频繁的进入比较中断，
这可能会给单片机带来较大的负担，但是在输出较低频率的PWM时，这种方法还是很好用的。
————————————————
版权声明：本文为CSDN博主「爱吃肉的大高个」的原创文章，遵循 CC 4.0 BY-SA 版权协议，转载请附上原文出处链接及本声明。
原文链接：https://blog.csdn.net/a568713197/article/details/89070265

*/

/*
arr：自动重装值
psc：时钟预分频数
*/
void TIM_PWM_Init(u16 arr, u16 psc) {
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /*使能时钟*/

    //使能定时器时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4 | RCC_APB1Periph_TIM5, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 | RCC_APB2Periph_TIM8, ENABLE);
    //使能GPIO外设和AFIO复用功能模块时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE); //TIM3_CH2->PC7、TIM8_CH1->PC6
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE); //TIM2_CH3->PB10
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE); //TIM4_CH1->PD12
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE); //TIM5_CH4->PA3、TIM1_CH1N->PA7(互补PWM输出：TIM1_CH1->PA8)

    /*引脚配置*/
    /*
        精英板 IO引脚分配表.xlsx
        Elite STM32F1_V1.4_SCH开发板原理图.pdf
        STM32中文参考手册_V10.pdf
            P 118 8.3.7 定时器复用功能重映射
            P 218 13.3.10 PWM（高级定时器1、8）
            P 269 14.3.9 PWM 模式（通用定时器2、3、4、5）

            P 184 12.3.3 DAC 数据格式
            P 149 10.3.7 DMA请求映像
    */

    //定时器重映射
    GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, ENABLE);    //Timer3全部重映射  TIM3_CH2->PC7
    GPIO_PinRemapConfig(GPIO_FullRemap_TIM2, ENABLE);    //Timer2全部重映射  TIM2_CH3->PB10
    GPIO_PinRemapConfig(GPIO_Remap_TIM4, ENABLE);         //Timer4重映射  TIM4_CH1->PD12
    //Timer5不用重映射  TIM5_CH4->PA3
    GPIO_PinRemapConfig(GPIO_PartialRemap_TIM1, ENABLE); //Timer1部分重映射，TIM1_CH1N->PA7(互补PWM输出：TIM1_CH1->PA8)
    //Timer8无重映射  TIM8_CH1->PC6

    //设置引脚为复用输出功能
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure); //TIM3_CH2->PC7

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOB, &GPIO_InitStructure); //TIM2_CH3->PB10

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_Init(GPIOD, &GPIO_InitStructure); //TIM4_CH1->PD12

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_Init(GPIOA, &GPIO_InitStructure); //TIM5_CH4->PA3

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8;
    GPIO_Init(GPIOA, &GPIO_InitStructure); //TIM1_CH1N->PA7(互补PWM输出：TIM1_CH1->PA8)

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_Init(GPIOC, &GPIO_InitStructure); //TIM8_CH1->PC6

    /*定时器计时配置*/

    //预分频器可将计数器的时钟频率按1-65536之间的任意值分频，分频后提供给计数器，作为计数器的时钟
    TIM_TimeBaseStructure.TIM_Period = arr;                        //自动重装值
    TIM_TimeBaseStructure.TIM_Prescaler = psc;                    //预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;                //设置时钟分割
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM向上计数模式

    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
    TIM_TimeBaseInit(TIM8, &TIM_TimeBaseStructure);

    /*定时器PWM通道配置*/

    //初始化PWM模式
    //110：PWM模式1－ 在向上计数时，一旦TIMx_CNT<TIMx_CCR1时通道1为有效电平，否则为无效电平；
    //在向下计数时，一旦TIMx_CNT>TIMx_CCR1时通道1为无效电平(OC1REF=0)，否则为有效电平(OC1REF=1)。
    //111：PWM模式2－ 在向上计数时，一旦TIMx_CNT<TIMx_CCR1时通道1为无效电平，否则为有效电平；
    //在向下计数时，一旦TIMx_CNT>TIMx_CCR1时通道1为有效电平，否则为无效电平。
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //选择定时器模式:TIM脉冲宽度调制模式2

    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
    TIM_OCInitStructure.TIM_Pulse = 0;                              //设置占空比？
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;      //输出极性（有效电平）

    TIM_OC2Init(TIM3, &TIM_OCInitStructure); //TIM3_CH2->PC7
    TIM_OC3Init(TIM2, &TIM_OCInitStructure); //TIM2_CH3->PB10
    TIM_OC1Init(TIM4, &TIM_OCInitStructure); //TIM4_CH1->PD12
    TIM_OC4Init(TIM5, &TIM_OCInitStructure); //TIM5_CH4->PA3
    TIM_OC1Init(TIM1, &TIM_OCInitStructure); //TIM1_CH1N->PA7(互补PWM输出：TIM1_CH1->PA8)
    TIM_OC1Init(TIM8, &TIM_OCInitStructure); //TIM8_CH1->PC6

    //使能TIMx在CCR2上的预装载寄存器
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM5, TIM_OCPreload_Enable);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC1PreloadConfig(TIM8, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_ARRPreloadConfig(TIM2, ENABLE);
    TIM_ARRPreloadConfig(TIM4, ENABLE);
    TIM_ARRPreloadConfig(TIM5, ENABLE);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_ARRPreloadConfig(TIM8, ENABLE);

    TIM_CCxNCmd(TIM1, TIM_Channel_1, TIM_CCxN_Enable); //TIM1_CH1N：默认无互补输出
    TIM_CtrlPWMOutputs(TIM1, ENABLE);                   //这句是高级定时器才有，输出PWM必须打开
    TIM_CtrlPWMOutputs(TIM8, ENABLE);

    /*运行定时器*/

    TIM_Cmd(TIM3, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
    TIM_Cmd(TIM5, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
    TIM_Cmd(TIM8, ENABLE);
}

/*
fre：频率27~4186Hz
*/
void buzzerSound(u16 fre1, u16 fre2, u16 fre3, u16 fre4, u16 fre5, u16 fre6) {
    u16 temp;

    //TIM3_CH2
    if (fre1 < 27 || (fre1 > 20000)) {
        TIM_SetCompare2(TIM3, 0); //停止蜂鸣器
        return;                      //频率具有优先级
    }
    temp = (72000000 / PRESCALE / fre1) - 1;
    TIM3->ARR = temp;                  //自动重装载值
    TIM_SetCompare2(TIM3, temp >> 1); //PWM占空比50%

    //TIM2_CH3
    if (fre2 < 27 || (fre2 > 20000)) {
        TIM_SetCompare3(TIM2, 0);
        return;
    }
    temp = (72000000 / PRESCALE / fre2) - 1;
    TIM2->ARR = temp;
    TIM_SetCompare3(TIM2, temp >> 1);

    //TIM4_CH1
    if (fre3 < 27 || (fre3 > 20000)) {
        TIM_SetCompare1(TIM4, 0);
        return;
    }
    temp = (72000000 / PRESCALE / fre3) - 1;
    TIM4->ARR = temp;
    TIM_SetCompare1(TIM4, temp >> 1);

    //TIM5_CH4
    if (fre4 < 27 || (fre4 > 20000)) {
        TIM_SetCompare4(TIM5, 0);
        return;
    }
    temp = (72000000 / PRESCALE / fre4) - 1;
    TIM5->ARR = temp;
    TIM_SetCompare4(TIM5, temp >> 1);

    //TIM1_CH1N
    if (fre5 < 27 || (fre5 > 20000)) {
        TIM_SetCompare1(TIM1, 0);
        return;
    }
    temp = (72000000 / PRESCALE / fre5) - 1;
    TIM1->ARR = temp;
    TIM_SetCompare1(TIM1, temp >> 1);

    //TIM8_CH1
    if (fre6 < 27 || (fre6 > 20000)) {
        TIM_SetCompare1(TIM8, 0);
        return;
    }
    temp = (72000000 / PRESCALE / fre6) - 1;
    TIM8->ARR = temp;
    TIM_SetCompare1(TIM8, temp >> 1);
}

/*
停止蜂鸣器（PWM占空比置零）
*/
void stop(void) {
    TIM_SetCompare2(TIM3, 0); //TIM3_CH2->PC7
    TIM_SetCompare3(TIM2, 0); //TIM2_CH3->PB10
    TIM_SetCompare1(TIM4, 0); //TIM4_CH1->PD12
    TIM_SetCompare4(TIM5, 0); //TIM5_CH4->PA3
    TIM_SetCompare1(TIM1, 0); //TIM1_CH1N->PA7(互补PWM输出：TIM1_CH1->PA8)
    TIM_SetCompare1(TIM8, 0); //TIM8_CH1->PC6
}

u16 slot_ms = 15;
u8 play_notes = 1; //当做bool类型用。0：弹钢琴；1：演奏音符
/*
playing notes
play the piano
*/
void play(u16 music[NOTES][8], u16 length) {
    u16 i, j;

    if (play_notes) {
        for (i = 0; i < length; i++) {
            buzzerSound(music[i][0], music[i][3], music[i][4], music[i][5], music[i][6], music[i][7]);
            for (j = music[i][1]; j > 0; j--)
                delay_ms(slot_ms);
            stop();
            for (j = music[i][2]; j > 0; j--)
                delay_ms(slot_ms);
            if (USART_RX_STA & 0x8000)
                break;
        }
        stop();
    } else {
        stop();
        buzzerSound(music[0][0], music[0][1], music[0][2], music[0][3], music[0][4], music[0][5]);
    }
}
