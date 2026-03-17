/*
 * cli_uart.c
 * Simple UART CLI server for NUCLEO-F439ZI
 */

#include "cli_uart.h"
#include "main.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>


/* -------------------------------------------------------------------------- */
/* Configuration                                                              */
/* -------------------------------------------------------------------------- */

#define CLI_RX_BUF_SIZE     256
#define CLI_LINE_BUF_SIZE   128

/* -------------------------------------------------------------------------- */
/* Variables                                                                  */
/* -------------------------------------------------------------------------- */

static volatile uint8_t  rxBuf[CLI_RX_BUF_SIZE];
static volatile uint16_t rxHead = 0;
static volatile uint16_t rxTail = 0;

static char lineBuf[CLI_LINE_BUF_SIZE];
static uint16_t lineLen = 0;

static uint8_t echo = 1;
static uint8_t rxByte;

/* -------------------------------------------------------------------------- */
/* Low level send                                                             */
/* -------------------------------------------------------------------------- */

static void CLI_Send(const uint8_t *data, uint16_t len)
{
    if (!data || !len) return;
    HAL_UART_Transmit(&huart3, (uint8_t*)data, len, HAL_MAX_DELAY);
}

void CLI_Print(const char *s)
{
    CLI_Send((uint8_t*)s, strlen(s));
}

void CLI_Printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    CLI_Print(buf);
}

/* -------------------------------------------------------------------------- */
/* Ring buffer                                                                */
/* -------------------------------------------------------------------------- */

static void RX_Put(uint8_t b)
{
    uint16_t next = (rxHead + 1) % CLI_RX_BUF_SIZE;
    if (next != rxTail)
    {
        rxBuf[rxHead] = b;
        rxHead = next;
    }
}

static int RX_Get(void)
{
    if (rxHead == rxTail) return -1;
    uint8_t b = rxBuf[rxTail];
    rxTail = (rxTail + 1) % CLI_RX_BUF_SIZE;
    return b;
}

/* -------------------------------------------------------------------------- */
/* Init                                                                       */
/* -------------------------------------------------------------------------- */

void CLI_UART_Init(void)
{
    rxHead = rxTail = 0;
    lineLen = 0;
    echo = 1;

    CLI_Print("\r\nSimple CLI ready\r\nType HELP\r\n> ");

    HAL_UART_Receive_IT(&huart3, &rxByte, 1);
}

/* -------------------------------------------------------------------------- */
/* HAL Callback                                                               */
/* -------------------------------------------------------------------------- */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        RX_Put(rxByte);
        HAL_UART_Receive_IT(&huart3, &rxByte, 1);
    }
}

/* -------------------------------------------------------------------------- */
/* HELP                                                                       */
/* -------------------------------------------------------------------------- */

static void CLI_ShowHelp(void)
{
    CLI_Print(
        "\r\nAvailable commands:\r\n"
        "  HELP        - show this help\r\n"
        "  RLED ON     - turn RED LED ON\r\n"
        "  RLED OFF    - turn RED LED OFF\r\n"
        "  ECHO        - toggle echo\r\n"
        "  STATE       - show button state\r\n"
    );
}

/* -------------------------------------------------------------------------- */
/* Command parser                                                             */
/* -------------------------------------------------------------------------- */

static void CLI_HandleLine(char *line)
{
    while (*line == ' ') line++;
    if (*line == 0) return;

    char *cmd = strtok(line, " ");
    if (!cmd) return;

    /* HELP */
    if (strcasecmp(cmd, "HELP") == 0)
    {
        CLI_ShowHelp();
        return;
    }

    /* RLED */
    if (strcasecmp(cmd, "RLED") == 0)
    {
        char *arg = strtok(NULL, " ");

        if (!arg)
        {
            CLI_Print("Usage: RLED ON/OFF\r\n");
            return;
        }

        if (strcasecmp(arg, "ON") == 0)
        {
        	GPIOB->BSRR = (1 << 14);
            CLI_Print("Red LED ON\r\n");
            return;
        }

        if (strcasecmp(arg, "OFF") == 0)
        {
        	GPIOB->BSRR = (1 << (14 + 16));
            CLI_Print("Red LED OFF\r\n");
            return;
        }

        CLI_Print("Usage: RLED ON/OFF\r\n");
        return;
    }

    /* ECHO */
    if (strcasecmp(cmd, "ECHO") == 0)
    {
        echo = !echo;
        CLI_Print(echo ? "Echo ON\r\n" : "Echo OFF\r\n");
        return;
    }

    /* STATE */
    if (strcasecmp(cmd, "STATE") == 0)
    {
    	unsigned int pressed=BTN_read();

        if (pressed)
            CLI_Print("Button: PRESSED\r\n");
        else
            CLI_Print("Button: RELEASED\r\n");
        return;
    }

    if (strcasecmp(cmd, "ADC") == 0)
    {
    	StartMeasurement_IT();
		CLI_Print("ADC measurement started...\r\n");
		return;
    }

    CLI_Print("Unknown command. Type HELP.\r\n");
}

/* -------------------------------------------------------------------------- */
/* Main processing (call in while(1))                                         */
/* -------------------------------------------------------------------------- */

void CLI_UART_Process(void)
{
    int ch;

    while ((ch = RX_Get()) >= 0)
    {
        char c = (char)ch;

        if (c == '\r' || c == '\n')
        {
            CLI_Print("\r\n");

            if (lineLen > 0)
            {
                lineBuf[lineLen] = 0;
                CLI_HandleLine(lineBuf);
                lineLen = 0;
            }

            CLI_Print("> ");
            continue;
        }

        if (c == '\b' || c == 0x7F)
        {
            if (lineLen > 0)
            {
                lineLen--;
                if (echo) CLI_Print("\b \b");
            }
            continue;
        }

        if (lineLen < CLI_LINE_BUF_SIZE - 1)
        {
            lineBuf[lineLen++] = c;
            if (echo) CLI_Send((uint8_t*)&c, 1);
        }
    }
}
