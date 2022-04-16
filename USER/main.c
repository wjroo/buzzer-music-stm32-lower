#include "led.h"
#include "delay.h"
#include "sys.h"
#include "timer.h"
#include "usart.h"

/*
stm32 堆和栈(stm32 Heap & Stack)
堆栈溢出，编译不会提示，需要注意
堆和栈有存在RAM里，他两各分多少看函数需求，但是他两的总值不能超过单片机硬件的实际RAM尺寸
stm32内存SRAM好像是：65535B？。
经查验资料，局部变量是放在栈中的，如果栈定义得较小，那么变量数就很少
因此当数组在main内部定义时，是作为局部变量从栈中分配内存给它。
startup_stm32f10x_hd.s可以更改栈的大小。
*.map文件显示内存分配信息。
总之：大数组作为全局变量，防爆栈。
*/

/*
	串口字符串形式“,;.”，最多6和弦。
	单位：Hz,10ms,10ms,Hz,Hz,Hz,Hz,Hz。
	（频率1，有声时间，无声时间,频率2,频率3,频率4,频率5，频率6）。
*/
u16 music[NOTES][8] = {0};

int main(void) {
    u16 i, j, k;
    u16 len;        //串口接收数据长度
    u16 length = 0; //music数组的有效长度

    delay_init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
    uart_init(115200);
    LED_Init();
    //TIM_PWM_Init(7999,8); //PWM频率=72 000 000/(7999+1)/(8+1)=1000 Hz
    TIM_PWM_Init(65535, PRESCALE - 1);
    printf("initOK");

    while (1) {
        if (USART_RX_STA & 0x8000) {
            len = USART_RX_STA & 0x3fff; //得到此次接收到的数据长度
            if (play_notes) {
                printf("\r\nBytes:%d\r\n", len);
            } else {
                for (i = 0; i < len; i++) {
                    // printf("%X,", USART_RX_BUF[i]); //把8bits数据转为16进制
                    USART_SendData(USART1, USART_RX_BUF[i]); //发送字节流
                    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) != SET); //等待发送结束
                }
            }
            USART_RX_STA = 0;

            /*清零*/
            i = 0;
            do {
                for (j = 0; j < 8; j++)
                    music[i][j] = 0;
                i++;
            } while (i < length);
            length = 0;

            /*翻译串口接收内容*/
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
                if (USART_RX_BUF[k] > '/' && USART_RX_BUF[k] < ':') //是数字0~9
                {
                    music[i][j] = 10 * music[i][j] + USART_RX_BUF[k] - '0';
                }
            }

            /*播放音乐*/
            if (length) {
                LED1 = 0;
                play(music, length);
                LED1 = 1;
            } else //设置
            {
                switch (music[0][0]) {
                    case 1: //播放音符的速度
                        if (music[0][1]) {
                            slot_ms = music[0][1];
                        }
                        break;
                    case 2: //0：弹钢琴；1：演奏音符
                        play_notes = (music[0][1]) ? 1 : 0;
                        break;
                    case 9: //查询设置应答
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
