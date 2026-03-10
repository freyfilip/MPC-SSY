/*
 * cv3_UART.h
 *
 *  Created on: 3. 3. 2026
 *      Author: Student
 */

#ifndef INC_CV3_UART_H_
#define INC_CV3_UART_H_

void cv3_init();

void cv3_polling();

void UART_StartRxIT(void);

void transmit_receive();

#endif /* INC_CV3_UART_H_ */
