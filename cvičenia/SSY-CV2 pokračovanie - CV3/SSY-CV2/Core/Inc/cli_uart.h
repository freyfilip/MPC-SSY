#ifndef __CLI_UART_H
#define __CLI_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Inicializace CLI serveru.
 *        Spustí příjem přes UART3 (IT režim).
 */
void CLI_UART_Init(void);

/**
 * @brief Hlavní zpracování CLI.
 *        Volat pravidelně v hlavní smyčce while(1).
 */
void CLI_UART_Process(void);

/**
 * @brief Vypíše text do terminálu.
 */
void CLI_Print(const char *s);

/**
 * @brief printf styl výpis do terminálu.
 */
void CLI_Printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_UART_H */
