#include "led.h"
#include "delay.h"
#include "sys.h"
#include "timer.h"
#include "usart.h"

/*
stm32 �Ѻ�ջ(stm32 Heap & Stack)
��ջ��������벻����ʾ����Ҫע��
�Ѻ�ջ�д���RAM��������ֶ��ٿ��������󣬵�����������ֵ���ܳ�����Ƭ��Ӳ����ʵ��RAM�ߴ�
stm32�ڴ�SRAM�����ǣ�65535B����
���������ϣ��ֲ������Ƿ���ջ�еģ����ջ����ý�С����ô�������ͺ���
��˵�������main�ڲ�����ʱ������Ϊ�ֲ�������ջ�з����ڴ������
startup_stm32f10x_hd.s���Ը���ջ�Ĵ�С��
*.map�ļ���ʾ�ڴ������Ϣ��
��֮����������Ϊȫ�ֱ���������ջ��
*/

/*
	�����ַ�����ʽ��,;.�������6���ҡ�
	��λ��Hz,10ms,10ms,Hz,Hz,Hz,Hz,Hz��
	��Ƶ��1������ʱ�䣬����ʱ��,Ƶ��2,Ƶ��3,Ƶ��4,Ƶ��5��Ƶ��6����
*/
u16 music[NOTES][8] = {0};

int main(void) {
    u16 i, j, k;
    u16 len;        //���ڽ������ݳ���
    u16 length = 0; //music�������Ч����

    delay_init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
    uart_init(115200);
    LED_Init();
    //TIM_PWM_Init(7999,8); //PWMƵ��=72 000 000/(7999+1)/(8+1)=1000 Hz
    TIM_PWM_Init(65535, PRESCALE - 1);
    printf("initOK");

    while (1) {
        if (USART_RX_STA & 0x8000) {
            len = USART_RX_STA & 0x3fff; //�õ��˴ν��յ������ݳ���
            if (play_notes) {
                printf("\r\nBytes:%d\r\n", len);
            } else {
                for (i = 0; i < len; i++) {
                    // printf("%X,", USART_RX_BUF[i]); //��8bits����תΪ16����
                    USART_SendData(USART1, USART_RX_BUF[i]); //�����ֽ���
                    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) != SET); //�ȴ����ͽ���
                }
            }
            USART_RX_STA = 0;

            /*����*/
            i = 0;
            do {
                for (j = 0; j < 8; j++)
                    music[i][j] = 0;
                i++;
            } while (i < length);
            length = 0;

            /*���봮�ڽ�������*/
            i = 0;
            j = 0;
            for (k = 0; k < len; k++) {
                if (USART_RX_BUF[k] == '.') {
                    length = i + 1;
                    break;
                }
                if (USART_RX_BUF[k] == ';') {
                    i++;
                    j = 0;
                    continue;
                }
                if (USART_RX_BUF[k] == ',') {
                    if (j < 7)
                        j++;
                    continue;
                }
                if (USART_RX_BUF[k] > '/' && USART_RX_BUF[k] < ':') //������0~9
                {
                    music[i][j] = 10 * music[i][j] + USART_RX_BUF[k] - '0';
                }
            }

            /*��������*/
            if (length) {
                LED1 = 0;
                play(music, length);
                LED1 = 1;
            } else //����
            {
                switch (music[0][0]) {
                    case 1: //�����������ٶ�
                        if (music[0][1]) {
                            slot_ms = music[0][1];
                        }
                        break;
                    case 2: //0�������٣�1����������
                        play_notes = (music[0][1]) ? 1 : 0;
                        break;
                    case 9: //��ѯ����Ӧ��
                        if (music[0][1] == 9)
                            printf("{speed:%dpiano:%d}\r\n", slot_ms, play_notes);
                        break;

                    default:
                        break;
                }
            }
        }
        delay_ms(1);
    }
}
