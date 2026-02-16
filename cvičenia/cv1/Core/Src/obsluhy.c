/*
 * obsluhy.c
 *
 *  Created on: 10. 2. 2026
 *      Author: Student
 */

#include "main.h"
#include "obsluhy.h"
#include <stdio.h>
#include <stdlib.h>

extern UART_HandleTypeDef huart3;

PUTCHAR_PROTOTYPE
{
	HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF) ;
	return ch;
}
