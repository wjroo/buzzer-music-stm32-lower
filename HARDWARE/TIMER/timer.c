#include "timer.h"
#include "delay.h"
#include "usart.h"

/*

���ڡ�STM32��һ����ʱ�������·��ͬƵ�ʼ�ռ�ձȵ�PWM������Ƚ�ģʽ����

����ʹ��STM32���PWMʱ��ʹ�ö�ʱ����PWM���ģʽ���������ɣ���������������PWM���о��޵ģ�
��ֻ��������·Ƶ����ͬ��PWM�������趨��TIMx_PSC��Ԥ��Ƶ�Ĵ�������TIMx_ARR���Զ���װ�ؼĴ�������
��ʱPWM��Ƶ�ʾͱ��������ˣ�Ϊϵͳ��ʱ��/TIMx_PSC+1/TIMx_ARR+1�������ͨ���ı����ͨ����CCR�Ĵ������ı�ռ�ձȡ�
����������������ɶ�·��ͬƵ�ʵ�PWM�Ļ���ʹ���������ֻ��ʹ�ö����ʱ���ˣ�
�������ڶ�ʱ����Դ���ٵİ��������ǲ���ȡ�ġ�32��ʱ����һ������Ƚϵ�ģʽ���������ɶ�·��ͬƵ�ʼ�ռ�ձȵ�PWM��

ע�⣡����
ʹ������Ƚϵķ���������ʹ��1����ʱ�����������Ч��������·��ͬƵ�ʼ�ռ�ձȵ�PWM��
���Ա�PWM���ģʽ��ȱ��϶������������жϵĴ���������ɵ�PWMƵ�ʽϸ�ʱ����Ƶ���Ľ���Ƚ��жϣ�
����ܻ����Ƭ�������ϴ�ĸ���������������ϵ�Ƶ�ʵ�PWMʱ�����ַ������Ǻܺ��õġ�
��������������������������������
��Ȩ����������ΪCSDN������������Ĵ�߸�����ԭ�����£���ѭ CC 4.0 BY-SA ��ȨЭ�飬ת���븽��ԭ�ĳ������Ӽ���������
ԭ�����ӣ�https://blog.csdn.net/a568713197/article/details/89070265

*/

/*
arr���Զ���װֵ
psc��ʱ��Ԥ��Ƶ��
*/
void TIM_PWM_Init(u16 arr, u16 psc) {
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /*ʹ��ʱ��*/

    //ʹ�ܶ�ʱ��ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4 | RCC_APB1Periph_TIM5, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 | RCC_APB2Periph_TIM8, ENABLE);
    //ʹ��GPIO�����AFIO���ù���ģ��ʱ��
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE); //TIM3_CH2->PC7��TIM8_CH1->PC6
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE); //TIM2_CH3->PB10
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE); //TIM4_CH1->PD12
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE); //TIM5_CH4->PA3��TIM1_CH1N->PA7(����PWM�����TIM1_CH1->PA8)

    /*��������*/
    /*
        ��Ӣ�� IO���ŷ����.xlsx
        Elite STM32F1_V1.4_SCH������ԭ��ͼ.pdf
        STM32���Ĳο��ֲ�_V10.pdf
            P 118 8.3.7 ��ʱ�����ù�����ӳ��
            P 218 13.3.10 PWM���߼���ʱ��1��8��
            P 269 14.3.9 PWM ģʽ��ͨ�ö�ʱ��2��3��4��5��

            P 184 12.3.3 DAC ���ݸ�ʽ
            P 149 10.3.7 DMA����ӳ��
    */

    //��ʱ����ӳ��
    GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, ENABLE);    //Timer3ȫ����ӳ��  TIM3_CH2->PC7
    GPIO_PinRemapConfig(GPIO_FullRemap_TIM2, ENABLE);    //Timer2ȫ����ӳ��  TIM2_CH3->PB10
    GPIO_PinRemapConfig(GPIO_Remap_TIM4, ENABLE);         //Timer4��ӳ��  TIM4_CH1->PD12
    //Timer5������ӳ��  TIM5_CH4->PA3
    GPIO_PinRemapConfig(GPIO_PartialRemap_TIM1, ENABLE); //Timer1������ӳ�䣬TIM1_CH1N->PA7(����PWM�����TIM1_CH1->PA8)
    //Timer8����ӳ��  TIM8_CH1->PC6

    //��������Ϊ�����������
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //�����������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure); //TIM3_CH2->PC7

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOB, &GPIO_InitStructure); //TIM2_CH3->PB10

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_Init(GPIOD, &GPIO_InitStructure); //TIM4_CH1->PD12

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_Init(GPIOA, &GPIO_InitStructure); //TIM5_CH4->PA3

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8;
    GPIO_Init(GPIOA, &GPIO_InitStructure); //TIM1_CH1N->PA7(����PWM�����TIM1_CH1->PA8)

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_Init(GPIOC, &GPIO_InitStructure); //TIM8_CH1->PC6

    /*��ʱ����ʱ����*/

    //Ԥ��Ƶ���ɽ���������ʱ��Ƶ�ʰ�1-65536֮�������ֵ��Ƶ����Ƶ���ṩ������������Ϊ��������ʱ��
    TIM_TimeBaseStructure.TIM_Period = arr;                        //�Զ���װֵ
    TIM_TimeBaseStructure.TIM_Prescaler = psc;                    //Ԥ��Ƶֵ
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;                //����ʱ�ӷָ�
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM���ϼ���ģʽ

    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
    TIM_TimeBaseInit(TIM8, &TIM_TimeBaseStructure);

    /*��ʱ��PWMͨ������*/

    //��ʼ��PWMģʽ
    //110��PWMģʽ1�� �����ϼ���ʱ��һ��TIMx_CNT<TIMx_CCR1ʱͨ��1Ϊ��Ч��ƽ������Ϊ��Ч��ƽ��
    //�����¼���ʱ��һ��TIMx_CNT>TIMx_CCR1ʱͨ��1Ϊ��Ч��ƽ(OC1REF=0)������Ϊ��Ч��ƽ(OC1REF=1)��
    //111��PWMģʽ2�� �����ϼ���ʱ��һ��TIMx_CNT<TIMx_CCR1ʱͨ��1Ϊ��Ч��ƽ������Ϊ��Ч��ƽ��
    //�����¼���ʱ��һ��TIMx_CNT>TIMx_CCR1ʱͨ��1Ϊ��Ч��ƽ������Ϊ��Ч��ƽ��
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //ѡ��ʱ��ģʽ:TIM�����ȵ���ģʽ2

    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //�Ƚ����ʹ��
    TIM_OCInitStructure.TIM_Pulse = 0;                              //����ռ�ձȣ�
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;      //������ԣ���Ч��ƽ��

    TIM_OC2Init(TIM3, &TIM_OCInitStructure); //TIM3_CH2->PC7
    TIM_OC3Init(TIM2, &TIM_OCInitStructure); //TIM2_CH3->PB10
    TIM_OC1Init(TIM4, &TIM_OCInitStructure); //TIM4_CH1->PD12
    TIM_OC4Init(TIM5, &TIM_OCInitStructure); //TIM5_CH4->PA3
    TIM_OC1Init(TIM1, &TIM_OCInitStructure); //TIM1_CH1N->PA7(����PWM�����TIM1_CH1->PA8)
    TIM_OC1Init(TIM8, &TIM_OCInitStructure); //TIM8_CH1->PC6

    //ʹ��TIMx��CCR2�ϵ�Ԥװ�ؼĴ���
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

    TIM_CCxNCmd(TIM1, TIM_Channel_1, TIM_CCxN_Enable); //TIM1_CH1N��Ĭ���޻������
    TIM_CtrlPWMOutputs(TIM1, ENABLE);                   //����Ǹ߼���ʱ�����У����PWM�����
    TIM_CtrlPWMOutputs(TIM8, ENABLE);

    /*���ж�ʱ��*/

    TIM_Cmd(TIM3, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
    TIM_Cmd(TIM5, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
    TIM_Cmd(TIM8, ENABLE);
}

/*
fre��Ƶ��27~4186Hz
*/
void buzzerSound(u16 fre1, u16 fre2, u16 fre3, u16 fre4, u16 fre5, u16 fre6) {
    u16 temp;

    //TIM3_CH2
    if (fre1 < 27 || (fre1 > 20000)) {
        TIM_SetCompare2(TIM3, 0); //ֹͣ������
        return;                      //Ƶ�ʾ������ȼ�
    }
    temp = (72000000 / PRESCALE / fre1) - 1;
    TIM3->ARR = temp;                  //�Զ���װ��ֵ
    TIM_SetCompare2(TIM3, temp >> 1); //PWMռ�ձ�50%

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
ֹͣ��������PWMռ�ձ����㣩
*/
void stop(void) {
    TIM_SetCompare2(TIM3, 0); //TIM3_CH2->PC7
    TIM_SetCompare3(TIM2, 0); //TIM2_CH3->PB10
    TIM_SetCompare1(TIM4, 0); //TIM4_CH1->PD12
    TIM_SetCompare4(TIM5, 0); //TIM5_CH4->PA3
    TIM_SetCompare1(TIM1, 0); //TIM1_CH1N->PA7(����PWM�����TIM1_CH1->PA8)
    TIM_SetCompare1(TIM8, 0); //TIM8_CH1->PC6
}

u16 slot_ms = 15;
u8 play_notes = 1; //����bool�����á�0�������٣�1����������
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
