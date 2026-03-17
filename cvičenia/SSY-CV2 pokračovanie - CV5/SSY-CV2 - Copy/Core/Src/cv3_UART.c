/*
 * cv3_UART.c
 *
 *  Created on: 3. 3. 2026
 *      Author: Student
 */

#include "main.h"
#include "cv3_UART.h"
#include "usart.h"

char hello[20] = "Hello from STM32!\r\n";
uint8_t ch ;
static  uint8_t rx_byte ;

void cv3_init(){
	HAL_UART_Transmit(&huart3 , hello , strlen(hello) , 100);
}

void cv3_polling(){

	HAL_UART_Receive(&huart3 , &ch , 1 , HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart3 , &ch , 1 , HAL_MAX_DELAY) ;
}

void UART_StartRxIT(void){
	  HAL_UART_Receive_IT(&huart3 , ( uint8_t* )&rx_byte , 1);
 }


void transmit_receive(){
	HAL_UART_Transmit_IT(&huart3 , ( uint8_t* )&rx_byte , 1 );
	HAL_UART_Receive_IT(&huart3 , ( uint8_t* )&rx_byte , 1 );
}

void UART_PrintNumber(uint16_t number){
    char buffer[20];

    sprintf(buffer, "%u\r\n", number);
    HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 100);
}
