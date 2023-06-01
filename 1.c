#include <STC8.h>
#include "delay.h"
#include "intrins.h"
typedef unsigned char BYTE;
typedef unsigned int WORD;
#define FOSC 11059200UL
#define BRT (65536 - FOSC / 9600 / 4)
#define uint unsigned int
#define uchar unsigned char
#define u8 unsigned char
#define u16 unsigned int
#define u32 unsigned long
#define DataPort P2 // 定义数据端口 程序中遇到DataPort 则用P0 替换
uchar code q[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, 0x76, 0x00, 0x40, 0xff};
uchar q2[] = {0x00, 0x00, 0x00, 0x00};
sbit led1 = P0 ^ 0;
sbit led2 = P0 ^ 1;
sbit led3 = P0 ^ 2;
sbit led4 = P0 ^ 3;

sbit enout = P1 ^ 1;
sbit enin = P3 ^ 5;

sbit k1 = P3 ^ 4;

sbit s1 = P3 ^ 2;
sbit s2 = P3 ^ 3;

sbit l1 = P1 ^ 6;
sbit l2 = P1 ^ 7;

sbit in2 = P1 ^ 5;

uchar q1[20];
uchar x, xi, x1, x2, bt1, bt2;
uint miao, miao1;

uchar a_a, wd1, sw3, id = 0, ks = 0;

uint num1, sw1, sw2, sw4;

bit busy;
char wptr;
char rptr;
char buffer[16];
void IapIdle()
{
	IAP_CONTR = 0;	  // 关闭IAP功能
	IAP_CMD = 0;	  // 清除命令寄存器
	IAP_TRIG = 0;	  // 清除触发寄存器
	IAP_ADDRH = 0x80; // 将地址设置到非IAP区域
	IAP_ADDRL = 0;
}

char IapRead(int addr)
{
	char dat;

	IAP_CONTR = 0x80;	   // 使能IAP
	IAP_TPS = 40;		   // 设置等待参数12MHz
	IAP_CMD = 1;		   // 设置IAP读命令
	IAP_ADDRL = addr;	   // 设置IAP低地址
	IAP_ADDRH = addr >> 8; // 设置IAP高地址
	IAP_TRIG = 0x5a;	   // 写触发命令(0x5a)
	IAP_TRIG = 0xa5;	   // 写触发命令(0xa5)
	_nop_();
	dat = IAP_DATA; // 读IAP数据
	IapIdle();		// 关闭IAP功能

	return dat;
}

void IapProgram(int addr, char dat)
{
	IAP_CONTR = 0x80;	   // 使能IAP
	IAP_TPS = 40;		   // 设置等待参数12MHz
	IAP_CMD = 2;		   // 设置IAP写命令
	IAP_ADDRL = addr;	   // 设置IAP低地址
	IAP_ADDRH = addr >> 8; // 设置IAP高地址
	IAP_DATA = dat;		   // 写IAP数据
	IAP_TRIG = 0x5a;	   // 写触发命令(0x5a)
	IAP_TRIG = 0xa5;	   // 写触发命令(0xa5)
	_nop_();
	IapIdle(); // 关闭IAP功能
}

void IapErase(int addr)
{
	IAP_CONTR = 0x80;	   // 使能IAP
	IAP_TPS = 40;		   // 设置等待参数12MHz
	IAP_CMD = 3;		   // 设置IAP擦除命令
	IAP_ADDRL = addr;	   // 设置IAP低地址
	IAP_ADDRH = addr >> 8; // 设置IAP高地址
	IAP_TRIG = 0x5a;	   // 写触发命令(0x5a)
	IAP_TRIG = 0xa5;	   // 写触发命令(0xa5)
	_nop_();			   //
	IapIdle();			   // 关闭IAP功能
}

//
void write_eeprom()
{
	IapErase(0x0400); // 扇区擦除
	IapProgram(0x0400 + 1, id);
	IapProgram(0x0400 + 2, num1 / 256);
	IapProgram(0x0400 + 3, num1 % 256);

	IapProgram(0x0400 + 4, sw1 / 256);
	IapProgram(0x0400 + 5, sw1 % 256);

	IapProgram(0x0400 + 6, sw2 / 256);
	IapProgram(0x0400 + 7, sw2 % 256);

	IapProgram(0x0400 + 8, sw3);

	IapProgram(0x0400 + 9, wd1);

	IapProgram(0x0400 + 10, sw4 / 256);
	IapProgram(0x0400 + 11, sw4 % 256);

	IapProgram(0x0400 + 13, ks);

	IapProgram(0x0400 + 60, a_a);
}
/******************把数据从单片机内部eeprom中读出来*****************/
void read_eeprom()
{
	uchar t, t1;
	id = IapRead(0x0400 + 1);

	t = IapRead(0x0400 + 2);
	t1 = IapRead(0x0400 + 3);
	num1 = t * 256 + t1;
	t = IapRead(0x0400 + 4);
	t1 = IapRead(0x0400 + 5);
	sw1 = t * 256 + t1;

	t = IapRead(0x0400 + 6);
	t1 = IapRead(0x0400 + 7);
	sw2 = t * 256 + t1;

	sw3 = IapRead(0x0400 + 8);
	wd1 = IapRead(0x0400 + 9);

	t = IapRead(0x0400 + 10);
	t1 = IapRead(0x0400 + 11);
	sw4 = t * 256 + t1;

	ks = IapRead(0x0400 + 13);

	a_a = IapRead(0x0400 + 60);
}
/**************开机自检eeprom初始化*****************/
void init_eeprom()
{
	read_eeprom();
	if (a_a != 6) // 新的单片机初始单片机内问eeprom
	{
		id = 1;
		a_a = 6;
		num1 = 1000;
		sw1 = 10000; // 1a
		sw2 = 300;	 // 5v
		sw3 = 10;
		wd1 = 40;
		sw4 = 000;
		write_eeprom(); // 保存数据
	}
}
void UartInit() // 11.0592 9600
{
	SCON = 0x50;  // 8位数据,可变波特率
	AUXR |= 0x40; // 定时器时钟1T模式
	AUXR &= 0xFE; // 串口1选择定时器1为波特率发生器
	TMOD &= 0x0F; // 设置定时器模式
	TL1 = 0xE0;	  // 设置定时初始值
	TH1 = 0xFE;	  // 设置定时初始值
	ET1 = 0;	  // 禁止定时器%d中断
	TR1 = 1;	  // 定时器1开始计时
	ES = 1;
	EA = 1;
	P_SW1 = 0x00; // RXD/P3.0, TXD/P3.1
	//  P_SW1 = 0x40;                               //RXD_2/P3.6, TXD_2/P3.7
	//  P_SW1 = 0x80;                               //RXD_3/P1.6, TXD_3/P1.7
	//	PS=1;
}

void UartSendByte(u8 dat)
{
	SBUF = dat;
	while (TI == 0)
		;
	TI = 0;
}

void UartSendStr(u8 *str)
{
	while (*str)
	{
		UartSendByte(*str);
		str++;
	}
}
void Uart2Init()
{
	S2CON = 0x10;
	T2L = BRT;
	T2H = BRT >> 8;
	AUXR |= 0x14;
	IE2 = 0x01;
	EA = 1;
}

void Uart2Send(char dat)
{
	while (busy)
		;
	busy = 1;
	S2BUF = dat;
}

void Uart2SendStr(char *p)
{
	while (*p)
	{
		Uart2Send(*p++);
	}
}

void Timer0Init(void) // 2毫秒@11.0592MHz
{
	AUXR |= 0x80; // 定时器时钟1T模式
	TMOD &= 0xF0; // 设置定时器模式
	TL0 = 0x9a;	  // 设置定时初值
	TH0 = 0xa9;	  // 设置定时初值
	TF0 = 0;	  // 清除TF0标志
	TR0 = 1;	  // 定时器0开始计时
	TR0 = 1;	  // 定时器0开始计时
	ET0 = 1;	  // 使能定时器0中断
	PT0 = 1;
	EA = 1;
	//    IT0 = 1;                                    //??INT0?????
	//    EX0 = 1;                                    //??INT0??
	//    EA = 1;
	//
	//	IT1 = 1;                                    //??INT1?????
	//    EX1 = 1;                                    //??INT1??
	//    EA = 1;
}
uchar i;
bit btx1, btx2, btx4;
uint ad1, ad2, ad4;
float ad3;
void main()
{
	// A电流高位 A 01
	// B电流小数点   B.001
	// C电源版本 C 00
	// D电压限制 D12.1
	// E温度设定E 20
	// F0~1电流设置 F.001
	// h   地址设定 h 00
	P0M0 = 0x00;
	P0M1 = 0x00;
	P1M0 = 0x0f; //??P1.0?ADC?
	P1M1 = 0x00;
	P2M0 = 0xff;
	P2M1 = 0x00;
	P3M0 = 0x00;
	P3M1 = 0x00;
	P4M0 = 0x00;
	P4M1 = 0x00;
	P5M0 = 0x00;
	P5M1 = 0x00;
	enout = 0;
	enin = 0;
	init_eeprom();
	UartInit();
	Uart2Init();
	Timer0Init();

	//   if(sw1==0){while(sw2==1);while(sw2==0);if(num7<9)num7=num7+1; write_eeprom();}
	//         if(sw2==0){while(sw1==1);while(sw1==0);if(num7>1)num7=num7-1; write_eeprom();}

	DelayMs(50);
	if (in2 == 1)
	{
		btx2 = 1;
		ks = 0;
	}

	while (1)
	{ // pwm=1;
		//	enout=1;
		//	Uart2SendStr("asdasdasd");
		if (s1 == 0)
		{
			miao1 = 0;
			btx4 = 1;
			while (s2 == 1 && miao1 < 2)
				;
			miao1 = 0;
			while (s2 == 0 && miao1 < 2)
				;
			if (num1 < 1000)
				num1 = num1 + 1;
			ks = 1;
			btx2 = 1;
			DelayMs(1);
		}
		if (s2 == 0)
		{
			miao1 = 0;
			btx4 = 1;
			while (s1 == 1 && miao1 < 2)
				;
			miao1 = 0;
			while (s1 == 0 && miao1 < 2)
				;
			if (num1 > 0)
				num1 = num1 - 1;
			ks = 1;
			btx2 = 1;
			DelayMs(1);
		}

		if (ks == 1)
		{
			if (num1 >= 1000)
			{
				q2[0] = q[num1 / 1000];
			}
			else
				q2[0] = 0x00;
			if (num1 >= 100)
			{
				q2[1] = q[num1 % 1000 / 100];
			}
			else
				q2[1] = 0x00;
			if (num1 >= 10)
			{
				q2[2] = q[num1 % 100 / 10];
			}
			else
				q2[2] = 0x00;
			q2[3] = q[num1 % 10];
		}

		else
		{
			q2[0] = 0x00;
			q2[1] = q[0];
			q2[2] = q[15];
			q2[3] = q[15];
		}
		// UartSendByte('E');
		if (btx2 == 1)
		{
			enout = 1;
			ad3 = 4.095 / sw3;
			(uint) ad1 = (float)sw1 * ad3;
			(uint) ad4 = (float)sw4 * ad3;
			ad3 = (float)(ad1 - ad4) / 1000;
			if (num1 == 0)
			{
				ad1 = 0;
			}
			else
			{
				(uint) ad1 = (float)num1 * ad3 + ad4 + 255;
			};
			ad2 = sw2 * 6.825;
			if (ad1 > 4095)
				ad1 = 4095;
			if (ad2 > 4095)
				ad2 = 4095;
			btx2 = 0;
			UartSendByte('A');
			UartSendByte('B');
			UartSendByte(ad2 / 1000 + 0x30);
			UartSendByte(ad2 % 1000 / 100 + 0x30);
			UartSendByte(ad2 % 100 / 10 + 0x30);
			UartSendByte(ad2 % 10 + 0x30);
			UartSendByte('E');
			UartSendByte(ad1 / 1000 + 0x30);
			UartSendByte(ad1 % 1000 / 100 + 0x30);
			UartSendByte(ad1 % 100 / 10 + 0x30);
			UartSendByte(ad1 % 10 + 0x30);
			UartSendByte('C');
			UartSendByte((4095 - ad1) / 1000 + 0x30);
			UartSendByte((4095 - ad1) % 1000 / 100 + 0x30);
			UartSendByte((4095 - ad1) % 100 / 10 + 0x30);
			UartSendByte((4095 - ad1) % 10 + 0x30);
			UartSendByte('V');
			UartSendByte((4095 - ad2) / 1000 + 0x30);
			UartSendByte((4095 - ad2) % 1000 / 100 + 0x30);
			UartSendByte((4095 - ad2) % 100 / 10 + 0x30);
			UartSendByte((4095 - ad2) % 10 + 0x30);
			UartSendByte('B');
			if (ks == 1)
			{
				UartSendByte('1');
			}
			else
				UartSendByte('0');
			if (ks == 1)
			{
				UartSendByte('0');
			}
			else
				UartSendByte('1');

			UartSendByte('#');
			DelayMs(10);
			UartSendByte('A');
			UartSendByte('B');
			UartSendByte(ad2 / 1000 + 0x30);
			UartSendByte(ad2 % 1000 / 100 + 0x30);
			UartSendByte(ad2 % 100 / 10 + 0x30);
			UartSendByte(ad2 % 10 + 0x30);
			UartSendByte('E');
			UartSendByte(ad1 / 1000 + 0x30);
			UartSendByte(ad1 % 1000 / 100 + 0x30);
			UartSendByte(ad1 % 100 / 10 + 0x30);
			UartSendByte(ad1 % 10 + 0x30);
			UartSendByte('C');
			UartSendByte((4095 - ad1) / 1000 + 0x30);
			UartSendByte((4095 - ad1) % 1000 / 100 + 0x30);
			UartSendByte((4095 - ad1) % 100 / 10 + 0x30);
			UartSendByte((4095 - ad1) % 10 + 0x30);
			UartSendByte('V');
			UartSendByte((4095 - ad2) / 1000 + 0x30);
			UartSendByte((4095 - ad2) % 1000 / 100 + 0x30);
			UartSendByte((4095 - ad2) % 100 / 10 + 0x30);
			UartSendByte((4095 - ad2) % 10 + 0x30);
			UartSendByte('B');
			if (ks == 1)
			{
				UartSendByte('1');
			}
			else
				UartSendByte('0');
			if (ks == 1)
			{
				UartSendByte('0');
			}
			else
				UartSendByte('1');

			UartSendByte('#');

			Uart2Send('A');
			Uart2Send(num1 / 1000 + 0x30);
			Uart2Send(num1 % 1000 / 100 + 0x30);
			Uart2Send(num1 % 100 / 10 + 0x30);
			Uart2Send(num1 % 10 + 0x30);
			Uart2Send('#');
			DelayMs(1);
			enout = 0;
		}
		if (in2 == 0)
		{
			if (k1 == 0)
			{
				l1 = 0;
				btx4 = 1;
				EX0 = EX1 = 0;
				miao1 = 0;
				while (k1 == 0)
				{
					l1 = miao1 % 2;
					if (miao1 >= 5)
					{
						i = 0;
						q2[0] = q[10];
						q2[1] = 0x00;
						q2[2] = q[sw1 / 10000];
						q2[3] = q[sw1 % 10000 / 1000];
						x2 = 254;
						l1 = 0;
						while (k1 == 0)
						{
							if (btx1 == 0)
								P2M0 = 0xff;
							else
								P2M0 = 0x00;
						}
						DelayMs(10);
						miao1 = 0;
						while (1)
						{
							if (s1 == 0)
							{
								miao1 = 0;
								while (s2 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s2 == 0 && miao1 < 2)
									;
								if (i < 6)
									i = i + 1;
								DelayMs(1);
							}
							if (s2 == 0)
							{
								miao1 = 0;
								while (s1 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s1 == 0 && miao1 < 2)
									;
								if (i > 0)
									i = i - 1;
								DelayMs(1);
							}
							switch (i)
							{
							case 0:
								q2[0] = q[10];
								q2[1] = 0x00;
								q2[2] = q[sw1 / 10000];
								q2[3] = q[sw1 % 10000 / 1000];
								break;
							case 1:
								q2[0] = q[11] | 0x80;
								q2[1] = q[sw1 % 1000 / 100];
								q2[2] = q[sw1 % 100 / 10];
								q2[3] = q[sw1 % 10];
								break;
							case 2:
								q2[0] = q[12];
								q2[1] = 0x00;
								q2[2] = q[sw3 / 10];
								q2[3] = q[sw3 % 10];
								break;
							case 3:
								q2[0] = q[13];
								q2[1] = q[sw2 / 100];
								q2[2] = q[sw2 % 100 / 10] | 0x80;
								q2[3] = q[sw2 % 10];
								break;
							case 4:
								q2[0] = q[14];
								q2[1] = 0x00;
								q2[2] = q[wd1 / 10000];
								q2[3] = q[wd1 % 10000 / 1000];
								break;
							case 5:
								q2[0] = q[15] | 0x80;
								q2[1] = q[sw4 / 100];
								q2[2] = q[sw4 % 100 / 10];
								q2[3] = q[sw4 % 10];
								break;
							case 6:
								q2[0] = q[16];
								q2[1] = q[id / 100];
								q2[2] = q[id % 100 / 10];
								q2[3] = q[id % 10];
								break;
							}
							if (k1 == 0)
							{
								DelayUs2x(100);
								if (k1 == 0)
								{
									x2 = 60;
									while (k1 == 0)
									{
										if (btx1 == 0)
											P2M0 = 0xff;
										else
											P2M0 = 0x00;
									}
									DelayMs(10);
									miao1 = 0;
									while (1)
									{
										switch (i)
										{
										case 0:
											q2[0] = q[10];
											q2[1] = 0x00;
											q2[2] = q[sw1 / 10000];
											q2[3] = q[sw1 % 10000 / 1000];
											if (s1 == 0)
											{
												miao1 = 0;
												while (s2 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s2 == 0 && miao1 < 2)
													;
												if (sw1 < (sw3 * 1000))
													sw1 = sw1 + 1000;
												DelayMs(1);
											}
											if (s2 == 0)
											{
												miao1 = 0;
												while (s1 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s1 == 0 && miao1 < 2)
													;
												if (sw1 > 1000)
													sw1 = sw1 - 1000;
												DelayMs(1);
											}
											break;

										case 1:
											q2[0] = q[11] | 0x80;
											q2[1] = q[sw1 % 1000 / 100];
											q2[2] = q[sw1 % 100 / 10];
											q2[3] = q[sw1 % 10];
											if (s1 == 0)
											{
												miao1 = 0;
												while (s2 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s2 == 0 && miao1 < 2)
													;
												if (sw1 < (sw3 * 1000))
													sw1 = sw1 + 2;
												DelayMs(1);
											}
											if (s2 == 0)
											{
												miao1 = 0;
												while (s1 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s1 == 0 && miao1 < 2)
													;
												if (sw1 >= 2)
													sw1 = sw1 - 2;
												DelayMs(1);
											}
											break;

										case 2:
											q2[0] = q[12];
											q2[1] = 0x00;
											q2[2] = q[sw3 / 10];
											q2[3] = q[sw3 % 10];
											if (s1 == 0)
											{
												miao1 = 0;
												while (s2 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s2 == 0 && miao1 < 2)
													;
												if (sw3 < 25)
													sw3 = sw3 + 1;
												DelayMs(1);
											}
											if (s2 == 0)
											{
												miao1 = 0;
												while (s1 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s1 == 0 && miao1 < 2)
													;
												if (sw3 > 2)
													sw3 = sw3 - 1;
												DelayMs(1);
											}
											break;

										case 3:
											q2[0] = q[13];
											q2[1] = q[sw2 / 100];
											q2[2] = q[sw2 % 100 / 10] | 0x80;
											q2[3] = q[sw2 % 10];
											if (s1 == 0)
											{
												miao1 = 0;
												while (s2 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s2 == 0 && miao1 < 2)
													;
												if (sw2 < 600)
													sw2 = sw2 + 1;
												DelayMs(1);
											}
											if (s2 == 0)
											{
												miao1 = 0;
												while (s1 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s1 == 0 && miao1 < 2)
													;
												if (sw2 > 10)
													sw2 = sw2 - 1;
												DelayMs(1);
											}
											break;

										case 4:
											q2[0] = q[14];
											q2[1] = 0x00;
											q2[2] = q[wd1 / 10000];
											q2[3] = q[wd1 % 10000 / 1000];
											if (s1 == 0)
											{
												miao1 = 0;
												while (s2 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s2 == 0 && miao1 < 2)
													;
												if (wd1 < 90)
													wd1 = wd1 + 1;
												DelayMs(1);
											}
											if (s2 == 0)
											{
												miao1 = 0;
												while (s1 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s1 == 0 && miao1 < 2)
													;
												if (wd1 > 1)
													wd1 = wd1 - 1;
												DelayMs(1);
											}
											break;

										case 5:
											q2[0] = q[15] | 0x80;
											q2[1] = q[sw4 / 100];
											q2[2] = q[sw4 % 100 / 10];
											q2[3] = q[sw4 % 10];
											if (s1 == 0)
											{
												miao1 = 0;
												while (s2 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s2 == 0 && miao1 < 2)
													;
												if (sw4 < 999)
													sw4 = sw4 + 1;
												DelayMs(1);
											}
											if (s2 == 0)
											{
												miao1 = 0;
												while (s1 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s1 == 0 && miao1 < 2)
													;
												if (sw4 > 0)
													sw4 = sw4 - 1;
												DelayMs(1);
											}
											break;

										case 6:
											q2[0] = q[16];
											q2[1] = q[id / 100];
											q2[2] = q[id % 100 / 10];
											q2[3] = q[id % 10];
											if (s1 == 0)
											{
												miao1 = 0;
												while (s2 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s2 == 0 && miao1 < 2)
													;
												if (id < 250)
													id = id + 1;
												DelayMs(1);
											}
											if (s2 == 0)
											{
												miao1 = 0;
												while (s1 == 1 && miao1 < 2)
													;
												miao1 = 0;
												while (s1 == 0 && miao1 < 2)
													;
												if (id > 0)
													id = id - 1;
												DelayMs(1);
											}
											break;
										}
										if (k1 == 0)
										{
											DelayUs2x(100);
											if (k1 == 0)
											{
												x2 = 254;
												while (k1 == 0)
												{
													if (btx1 == 0)
														P2M0 = 0xff;
													else
														P2M0 = 0x00;
												}
												DelayMs(10);
												miao1 = 0;
												break;
											}
										}
										if (btx1 == 0)
											P2M0 = 0xff;
										else
											P2M0 = 0x00;
									}
								}
							}
							if (miao1 > 5)
							{
								btx2 = 1;
								break;
							}
							if (btx1 == 0)
								P2M0 = 0xff;
							else
								P2M0 = 0x00;
						}
					}
				}
				if (miao1 < 2)
				{
					while (1)
					{
						l1 = 0;
						switch (i)
						{
						case 0:
							x2 = 100;
							if (btx1 == 0)
							{
								q2[0] = q[num1 / 1000];
								q2[1] = q[num1 % 1000 / 100];
								q2[2] = q[num1 % 100 / 10];
								q2[3] = q[num1 % 10];
							}
							else
								q2[3] = 0x00;
							if (s1 == 0)
							{
								miao1 = 0;
								while (s2 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s2 == 0 && miao1 < 2)
									;
								if (num1 < 1000)
									num1 = num1 + 1;
								btx2 = 1;
								DelayMs(1);
							}
							if (s2 == 0)
							{
								miao1 = 0;
								while (s1 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s1 == 0 && miao1 < 2)
									;
								if (num1 > 0)
									num1 = num1 - 1;
								btx2 = 1;
								DelayMs(1);
							}
							break;
						case 1:
							x2 = 100;
							if (btx1 == 0)
							{
								q2[0] = q[num1 / 1000];
								q2[1] = q[num1 % 1000 / 100];
								q2[2] = q[num1 % 100 / 10];
								q2[3] = q[num1 % 10];
							}
							else
								q2[2] = 0x00;
							if (s1 == 0)
							{
								miao1 = 0;
								while (s2 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s2 == 0 && miao1 < 2)
									;
								if (num1 < 990)
									num1 = num1 + 10;
								btx2 = 1;
								DelayMs(1);
							}
							if (s2 == 0)
							{
								miao1 = 0;
								while (s1 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s1 == 0 && miao1 < 2)
									;
								if (num1 > 10)
									num1 = num1 - 10;
								btx2 = 1;
								DelayMs(1);
							}
							break;
						case 2:
							x2 = 100;
							if (btx1 == 0)
							{
								q2[0] = q[num1 / 1000];
								q2[1] = q[num1 % 1000 / 100];
								q2[2] = q[num1 % 100 / 10];
								q2[3] = q[num1 % 10];
							}
							else
								q2[1] = 0x00;
							if (s1 == 0)
							{
								miao1 = 0;
								while (s2 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s2 == 0 && miao1 < 2)
									;
								if (num1 < 900)
									num1 = num1 + 100;
								btx2 = 1;
								DelayMs(1);
							}
							if (s2 == 0)
							{
								miao1 = 0;
								while (s1 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s1 == 0 && miao1 < 2)
									;
								if (num1 > 100)
									num1 = num1 - 100;
								btx2 = 1;
								DelayMs(1);
							}
							break;
						case 3:
							x2 = 100;
							if (btx1 == 0)
							{
								q2[0] = q[num1 / 1000];
								q2[1] = q[num1 % 1000 / 100];
								q2[2] = q[num1 % 100 / 10];
								q2[3] = q[num1 % 10];
							}
							else
								q2[0] = 0x00;
							if (s1 == 0)
							{
								miao1 = 0;
								while (s2 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s2 == 0 && miao1 < 2)
									;
								if (num1 < 1)
									num1 = num1 + 1000;
								btx2 = 1;
								DelayMs(1);
							}
							if (s2 == 0)
							{
								miao1 = 0;
								while (s1 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s1 == 0 && miao1 < 2)
									;
								if (num1 > 999)
									num1 = num1 - 1000;
								btx2 = 1;
								DelayMs(1);
							}
							break;
						}

						if (k1 == 0)
						{
							DelayMs(1);
							miao1 = 0;
							i++;
							if (i >= 4)
								i = 0;
							while (k1 == 0)
								;
							DelayMs(1);
						}
						if (miao1 > 6)
						{
							miao1 = 0;
							break;
						}
					}
				}

				write_eeprom();
				btx2 = 1;
				P2M0 = 0xff;
				l1 = 1;
			}
		}
		////////////////////////////////	////////////////	////////////////	////////////////	////////////////	////////////////	////////////////	////////////////	////////////////	////////////////	////////////////	////////////////
		else
		{

			if (k1 == 0 && ks == 1)
			{
				l1 = 0;
				btx4 = 1;
				EX0 = EX1 = 0;
				miao1 = 0;
				while (k1 == 0)
				{
					l1 = miao1 % 2;
					if (miao1 >= 5)
					{
						i = 0;
						x2 = 254;
						if (ks == 1)
							ks = 0;
						else
							ks = 1;
						UartSendByte('A');
						UartSendByte('B');
						UartSendByte(ad2 / 1000 + 0x30);
						UartSendByte(ad2 % 1000 / 100 + 0x30);
						UartSendByte(ad2 % 100 / 10 + 0x30);
						UartSendByte(ad2 % 10 + 0x30);
						UartSendByte('E');
						UartSendByte(ad1 / 1000 + 0x30);
						UartSendByte(ad1 % 1000 / 100 + 0x30);
						UartSendByte(ad1 % 100 / 10 + 0x30);
						UartSendByte(ad1 % 10 + 0x30);
						UartSendByte('C');
						UartSendByte((4095 - ad1) / 1000 + 0x30);
						UartSendByte((4095 - ad1) % 1000 / 100 + 0x30);
						UartSendByte((4095 - ad1) % 100 / 10 + 0x30);
						UartSendByte((4095 - ad1) % 10 + 0x30);
						UartSendByte('V');
						UartSendByte((4095 - ad2) / 1000 + 0x30);
						UartSendByte((4095 - ad2) % 1000 / 100 + 0x30);
						UartSendByte((4095 - ad2) % 100 / 10 + 0x30);
						UartSendByte((4095 - ad2) % 10 + 0x30);
						UartSendByte('B');
						if (ks == 1)
						{
							UartSendByte('1');
						}
						else
							UartSendByte('0');
						if (ks == 1)
						{
							UartSendByte('0');
						}
						else
							UartSendByte('1');

						UartSendByte('#');

						DelayMs(10);
						UartSendByte('A');
						UartSendByte('B');
						UartSendByte(ad2 / 1000 + 0x30);
						UartSendByte(ad2 % 1000 / 100 + 0x30);
						UartSendByte(ad2 % 100 / 10 + 0x30);
						UartSendByte(ad2 % 10 + 0x30);
						UartSendByte('E');
						UartSendByte(ad1 / 1000 + 0x30);
						UartSendByte(ad1 % 1000 / 100 + 0x30);
						UartSendByte(ad1 % 100 / 10 + 0x30);
						UartSendByte(ad1 % 10 + 0x30);
						UartSendByte('C');
						UartSendByte((4095 - ad1) / 1000 + 0x30);
						UartSendByte((4095 - ad1) % 1000 / 100 + 0x30);
						UartSendByte((4095 - ad1) % 100 / 10 + 0x30);
						UartSendByte((4095 - ad1) % 10 + 0x30);
						UartSendByte('V');
						UartSendByte((4095 - ad2) / 1000 + 0x30);
						UartSendByte((4095 - ad2) % 1000 / 100 + 0x30);
						UartSendByte((4095 - ad2) % 100 / 10 + 0x30);
						UartSendByte((4095 - ad2) % 10 + 0x30);
						UartSendByte('B');
						if (ks == 1)
						{
							UartSendByte('1');
						}
						else
							UartSendByte('0');
						if (ks == 1)
						{
							UartSendByte('0');
						}
						else
							UartSendByte('1');

						UartSendByte('#');

						if (ks == 1)
						{
							if (num1 >= 1000)
							{
								q2[0] = q[num1 / 1000];
							}
							else
								q2[0] = 0x00;
							if (num1 >= 100)
							{
								q2[1] = q[num1 % 1000 / 100];
							}
							else
								q2[1] = 0x00;
							if (num1 >= 10)
							{
								q2[2] = q[num1 % 100 / 10];
							}
							else
								q2[2] = 0x00;
							q2[3] = q[num1 % 10];
						}

						else
						{
							q2[0] = 0x00;
							q2[1] = q[0];
							q2[2] = q[15];
							q2[3] = q[15];
						}
						while (k1 == 0)
							;
						DelayMs(10);
					}
				}

				if (miao1 < 2)
				{
					while (1)
					{
						l1 = 0;
						switch (i)
						{
						case 0:
							x2 = 100;
							if (btx1 == 0)
							{
								q2[0] = q[num1 / 1000];
								q2[1] = q[num1 % 1000 / 100];
								q2[2] = q[num1 % 100 / 10];
								q2[3] = q[num1 % 10];
							}
							else
								q2[3] = 0x00;
							if (s1 == 0)
							{
								miao1 = 0;
								while (s2 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s2 == 0 && miao1 < 2)
									;
								if (num1 < 1000)
									num1 = num1 + 1;
								btx2 = 1;
								DelayMs(1);
							}
							if (s2 == 0)
							{
								miao1 = 0;
								while (s1 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s1 == 0 && miao1 < 2)
									;
								if (num1 > 0)
									num1 = num1 - 1;
								btx2 = 1;
								DelayMs(1);
							}
							break;
						case 1:
							x2 = 100;
							if (btx1 == 0)
							{
								q2[0] = q[num1 / 1000];
								q2[1] = q[num1 % 1000 / 100];
								q2[2] = q[num1 % 100 / 10];
								q2[3] = q[num1 % 10];
							}
							else
								q2[2] = 0x00;
							if (s1 == 0)
							{
								miao1 = 0;
								while (s2 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s2 == 0 && miao1 < 2)
									;
								if (num1 < 990)
									num1 = num1 + 10;
								btx2 = 1;
								DelayMs(1);
							}
							if (s2 == 0)
							{
								miao1 = 0;
								while (s1 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s1 == 0 && miao1 < 2)
									;
								if (num1 > 10)
									num1 = num1 - 10;
								btx2 = 1;
								DelayMs(1);
							}
							break;
						case 2:
							x2 = 100;
							if (btx1 == 0)
							{
								q2[0] = q[num1 / 1000];
								q2[1] = q[num1 % 1000 / 100];
								q2[2] = q[num1 % 100 / 10];
								q2[3] = q[num1 % 10];
							}
							else
								q2[1] = 0x00;
							if (s1 == 0)
							{
								miao1 = 0;
								while (s2 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s2 == 0 && miao1 < 2)
									;
								if (num1 < 900)
									num1 = num1 + 100;
								btx2 = 1;
								DelayMs(1);
							}
							if (s2 == 0)
							{
								miao1 = 0;
								while (s1 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s1 == 0 && miao1 < 2)
									;
								if (num1 > 100)
									num1 = num1 - 100;
								btx2 = 1;
								DelayMs(1);
							}
							break;
						case 3:
							x2 = 100;
							if (btx1 == 0)
							{
								q2[0] = q[num1 / 1000];
								q2[1] = q[num1 % 1000 / 100];
								q2[2] = q[num1 % 100 / 10];
								q2[3] = q[num1 % 10];
							}
							else
								q2[0] = 0x00;
							if (s1 == 0)
							{
								miao1 = 0;
								while (s2 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s2 == 0 && miao1 < 2)
									;
								if (num1 < 1)
									num1 = num1 + 1000;
								btx2 = 1;
								DelayMs(1);
							}
							if (s2 == 0)
							{
								miao1 = 0;
								while (s1 == 1 && miao1 < 2)
									;
								miao1 = 0;
								while (s1 == 0 && miao1 < 2)
									;
								if (num1 > 999)
									num1 = num1 - 1000;
								btx2 = 1;
								DelayMs(1);
							}
							break;
						}

						if (k1 == 0)
						{
							DelayMs(1);
							miao1 = 0;
							i++;
							if (i >= 4)
								i = 0;
							while (k1 == 0)
								;
							DelayMs(1);
						}
						if (miao1 > 6)
						{
							miao1 = 0;
							break;
						}
					}
				}
				write_eeprom();
				btx2 = 1;
				P2M0 = 0xff;
				l1 = 1;
			}

			else if (k1 == 0)
			{
				l1 = 0;
				btx4 = 1;
				EX0 = EX1 = 0;
				miao1 = 0;
				ks = 1;
				btx2 = 1;
				while (k1 == 0)
					;
			}
		}

		if (miao1 > 3 && btx4 == 1)
		{
			btx4 = 0;
			write_eeprom();
		}
	}
}
void INT0_Isr() interrupt 0
{
	EX0 = 0;
	EX1 = 0;
	btx2 = 1;
	if (s1 == 0)
	{
		miao1 = 0;
		while (s2 == 1 && miao1 < 2)
			;
		miao1 = 0;
		while (s2 == 0 && miao1 < 2)
			;
		if (num1 < 1000)
			num1 = num1 + 1;
	} //????
	EX1 = 1;
	EX0 = 1;
}
void INT1_Isr() interrupt 2
{
	EX0 = 0;
	EX1 = 0;
	btx2 = 1;
	if (s2 == 0)
	{
		miao1 = 0;
		while (s1 == 1 && miao1 < 2)
			;
		miao1 = 0;
		while (s1 == 0 && miao1 < 2)
			;
		if (num1 > 0)
			num1 = num1 - 1;
	}
	EX0 = 1;
	EX1 = 1;
}
/* Timer0 interrupt routine */
void tm0_isr() interrupt 1
{
	x++;
	x1++;
	if (x1 > x2)
	{
		x1 = 0;
		btx1 = ~btx1;
	}
	miao++;
	if (miao >= 500)
	{
		miao = 0;
		miao1++;
	}
	led1 = led2 = led3 = led4 = 1;
	DataPort = 0x00;
	if (x == 1)
	{
		led1 = 0;
		led2 = 1;
		led3 = 1;
		led4 = 1;
		DataPort = q2[0];
	}
	if (x == 2)
	{
		led1 = 1;
		led2 = 0;
		led3 = 1;
		led4 = 1;
		DataPort = q2[1];
	}
	if (x == 3)
	{
		led1 = 1;
		led2 = 1;
		led3 = 0;
		led4 = 1;
		DataPort = q2[2];
	}
	if (x == 4)
	{
		x = 0;
		led1 = 1;
		led2 = 1;
		led3 = 1;
		led4 = 0;
		DataPort = q2[3];
	}
}
uchar xi = 0;
void UartIsr() interrupt 4
{

	if (RI)
	{
		RI = 0;
		//			if(SBUF=='A'){xi=0;}
		//      q1[xi]=SBUF;
		//			xi++;
		//
		//			if(xi>=12){if(q1[0]=='A'&&q1[1]=='B'&&q1[11]=='#'){bt1=1;sw1=(q1[2]-0x30)*1000+(q1[3]-0x30)*100+(q1[4]-0x30)*10+(q1[5]-0x30)*1;sw2=(q1[7]-0x30)*1000+(q1[8]-0x30)*100+(q1[9]-0x30)*10+(q1[10]-0x30)*1;}
		//                 if(q1[0]=='A'&&q1[1]=='C'&&q1[11]=='#'){bt1=1;sw3=(q1[2]-0x30)*1000+(q1[3]-0x30)*100+(q1[4]-0x30)*10+(q1[5]-0x30)*1;bt2=q1[10]-0x30;}
		//                }
	}
}
// A1000#
void Uart2Isr() interrupt 8
{
	if (S2CON & 0x02)
	{
		S2CON &= ~0x02;
		busy = 0;
	}
	if (S2CON & 0x01)
	{
		S2CON &= ~0x01;
		if (SBUF == 'A')
		{
			xi = 0;
		}
		q1[xi] = SBUF;
		xi++;

		if (xi >= 6)
		{
			if (q1[0] == 'A' && q1[5] == '#')
			{
				bt1 = 1;
				btx2 = 1;
				num1 = (q1[1] - 0x30) * 1000 + (q1[2] - 0x30) * 100 + (q1[3] - 0x30) * 10 + (q1[4] - 0x30) * 1;
				if (num1 > 1000)
				{
					num1 = 1000;
				}
			}
		}
	}
}