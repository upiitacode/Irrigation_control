#include "stm32f0xx.h"                  // Device header
#include "retarget_STM32F0.h"
#include "serial_stdio.h"
#include "LCD.h"
#include "clock.h"
void setToMaxSpeed(void);
Serial_t UART2_serial = {UART2_getChar, UART2_sendChar};
Serial_t LCD_serial = {NULL, lcd_sendChar};

#define IN_BUFFER_SIZE 40
char inputBuffer[IN_BUFFER_SIZE];


static int last_encoderA;
static int last_encoderB;
static int encoder_lastPos;
static int encoder_pos;
static int encoder_enable=0;


void encoder_init(void){
	//ECH_A PB6, ECH_B PB7 
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB,ENABLE);
	
	GPIO_InitTypeDef myGPIO;
	GPIO_StructInit(&myGPIO);
	myGPIO.GPIO_Mode=GPIO_Mode_IN;
	myGPIO.GPIO_Pin=GPIO_Pin_6|GPIO_Pin_7;
	myGPIO.GPIO_PuPd=GPIO_PuPd_UP;
	GPIO_Init(GPIOB,&myGPIO);
	
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_SYSCFGEN,ENABLE);//Turn on SYSCFG since it's disabled by default
	EXTI_InitTypeDef myEXTI;
	EXTI_StructInit(&myEXTI);
	myEXTI.EXTI_Line=EXTI_Line6|EXTI_Line7;
	myEXTI.EXTI_LineCmd=ENABLE;
	myEXTI.EXTI_Mode=EXTI_Mode_Interrupt;
	myEXTI.EXTI_Trigger=EXTI_Trigger_Rising_Falling;
	EXTI_Init(&myEXTI);
	
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB,EXTI_PinSource6);//Reamp Line 6 to Port B
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB,EXTI_PinSource7);//Reamp Line 7 to Port B
	
	
	last_encoderA=GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_6);
	last_encoderB=GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7);
	
	EXTI_ClearITPendingBit(EXTI_Line6);//Clear pening interrupts
	EXTI_ClearITPendingBit(EXTI_Line7);//Clear pening interrupts
	
	NVIC_EnableIRQ(EXTI4_15_IRQn);//Enable EXTI0  interrupts
}

int encoder_hasChanged(void){
	if(encoder_lastPos!=encoder_pos){
		encoder_lastPos=encoder_pos;
		return 1;
	}else{
		return 0;
	}
}

void encoder_process(void){
	int encoder_A=GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_6);
	int encoder_B=GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7);
	
	if(encoder_A&&encoder_B&&encoder_enable){
		if((!last_encoderA)&&(encoder_A)) encoder_pos++;
		if((!last_encoderB)&&(encoder_B)) encoder_pos--;
		encoder_enable=0;
	}
	if((!encoder_A)&&(!encoder_B)){
		encoder_enable=1;
	}
	
	last_encoderA=encoder_A;
	last_encoderB=encoder_B;
}

int timer_count;
int last_timer_count;
volatile uint32_t time_val;

int main(void)
{	
	clock_time_t * myClock=(clock_time_t *) &time_val;
	encoder_init();
	UART2_init(9600);
	clock_init();
	lcd_init();
	serial_printf(UART2_serial,"\nSystem ready\n");
	serial_printf(LCD_serial,"\fPOS %d",encoder_pos);
	serial_printf(LCD_serial,"\nTime ");
	
	while(1){
		if(encoder_hasChanged()){
			lcd_goto(4,0);
			serial_printf(LCD_serial,"%d  ",encoder_pos);
		}
		timer_count=clock_getSeconds();
		if(last_timer_count!=timer_count){
			time_val=clock_getTime();
			lcd_goto(5,1);
			serial_printf(LCD_serial,"%2d:%2d:%2d:%2d",myClock->days,myClock->hours,myClock->minutes,myClock->seconds);
			last_timer_count=timer_count;
		}
	}
}

void EXTI4_15_IRQHandler(void){
	if(EXTI_GetITStatus(EXTI_Line6)||EXTI_GetITStatus(EXTI_Line7)){
		if(EXTI_GetITStatus(EXTI_Line6)) EXTI_ClearITPendingBit(EXTI_Line6);
		if(EXTI_GetITStatus(EXTI_Line7)) EXTI_ClearITPendingBit(EXTI_Line7); 
		encoder_process();
	}
}
