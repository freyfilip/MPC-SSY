/*
 * cv2.c
 *
 *  Created on: 24. 2. 2026
 *      Author: Student
 */


#include "main.h"
#include "cv2.h"

void LED_init(){
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

	GPIOB->MODER &= ~(3U << (14 * 2));
	GPIOB->MODER |= (1U << (14 * 2));

	GPIOB->MODER &= ~(3U << (0 * 2));
	GPIOB->MODER |= (1U << (0 * 2));

	GPIOB->MODER &= ~(3U << (7 * 2));
	GPIOB->MODER |= (1U << (7 * 2));
}

void LED_ON(){
	GPIOB->BSRR = (1 << 14);

	GPIOB->BSRR = (1 << 0);

	GPIOB->BSRR = (1 << 7);
}

void LED_OFF(){
	GPIOB->BSRR = (1 << (14 + 16));

	GPIOB->BSRR = (1 << (0 + 16));

	GPIOB->BSRR = (1 << (7 + 16));
}

void BTN_init(){

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

	GPIOC->MODER &= ~(3U << (13 * 2));
}

unsigned int BTN_read(){

	unsigned int state1=GPIOC->IDR & (1 << 13);
	HAL_Delay(20);
	unsigned int state2=GPIOC->IDR & (1 << 13);

	if((state1) == (state2)){
		return state2;
	}
	return 0;
}

void BTN_function(){
	unsigned int pressed=BTN_read();

	if(pressed){
		LED_ON();
		HAL_Delay(3000);
		LED_OFF();
		HAL_Delay(3000);
	}
}

