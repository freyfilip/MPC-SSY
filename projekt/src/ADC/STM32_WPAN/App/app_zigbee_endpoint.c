
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_zigbee_endpoint.c
  * Description        : Zigbee Application to manage endpoints and these clusters.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <assert.h>
#include <stdint.h>

#include "app_common.h"
#include "app_conf.h"
#include "log_module.h"
#include "app_entry.h"
#include "app_zigbee.h"
#include "dbg_trace.h"
#include "ieee802154_enums.h"
#include "mcp_enums.h"

#include "stm32_lpm.h"
#include "stm32_rtos.h"
#include "stm32_timer.h"
#include "stm32_lpm_if.h"

#include "zigbee.h"
#include "zigbee.nwk.h"
#include "zigbee.security.h"

/* Private includes -----------------------------------------------------------*/
#include "zcl/zcl.h"
#include "zcl/general/zcl.temp.meas.h"
#include "zcl/general/zcl.press.meas.h"

/* USER CODE BEGIN PI */
#include "app_bsp.h"

/* Used to simulate a Temperature Sensor */
#include "zigbee_plat.h"

/* USER CODE END PI */

/* Private defines -----------------------------------------------------------*/
#define APP_ZIGBEE_CHANNEL                25u
#define APP_ZIGBEE_CHANNEL_MASK           ( 1u << APP_ZIGBEE_CHANNEL )
#define APP_ZIGBEE_TX_POWER               ((int8_t) 10)    /* TX-Power is at +10 dBm. */

#define APP_ZIGBEE_ENDPOINT               18u
#define APP_ZIGBEE_PROFILE_ID             ZCL_PROFILE_HOME_AUTOMATION
#define APP_ZIGBEE_DEVICE_ID              ZCL_DEVICE_TEMPERATURE_SENSOR
#define APP_ZIGBEE_GROUP_ADDRESS          0x0001u

#define APP_ZIGBEE_CLUSTER_ID             ZCL_CLUSTER_MEAS_TEMPERATURE
#define APP_ZIGBEE_CLUSTER_NAME           "TempMeas Server"

/* MeasTemperature specific defines ----------------------------------------------------*/
#define APP_ZIGBEE_TEMP_MIN               -4000
#define APP_ZIGBEE_TEMP_MAX               12500
#define APP_ZIGBEE_TEMP_TOLERANCE         50

/* SoundSensor specific defines ----------------------------------------------------*/
#define SOUND_SENSOR_MIN_ADC      180
#define SOUND_SENSOR_MAX_ADC      3950
#define SOUND_SENSOR_MIN_DB       35.0f
#define SOUND_SENSOR_MAX_DB      110.0f

/* MoistureSensor specific defines ----------------------------------------------------*/
#define MOISTURE_SENSOR_MIN_ADC      400
#define MOISTURE_SENSOR_MAX_ADC      3800
#define MOISTURE_SENSOR_MIN_PERCENT   0.0f
#define MOISTURE_SENSOR_MAX_PERCENT 100.0f

#define APP_ZIGBEE_TEMPMEAS_UPDATE_PERIOD (uint32_t)( 500u ) /* 500ms */

#define APP_ZIGBEE_APPLICATION_NAME       APP_ZIGBEE_CLUSTER_NAME
#define APP_ZIGBEE_APPLICATION_OS_NAME    "."

// -- Redefine task to better code read --
#define CFG_TASK_ZIGBEE_APP_SENSOR_READ         CFG_TASK_ZIGBEE_APP1
#define TASK_ZIGBEE_APP_SENSOR_READ_PRIORITY    CFG_SEQ_PRIO_1

/* USER CODE BEGIN PD */
#define SOUND_SENSOR_ADC_CHANNEL     ADC_CHANNEL_0
#define MOISTURE_SENSOR_ADC_CHANNEL  ADC_CHANNEL_3

extern ADC_HandleTypeDef hadc4;
#define APP_ZIGBEE_ENDPOINT_MOISTURE          19u
#define APP_ZIGBEE_MOISTURE_MIN               0
#define APP_ZIGBEE_MOISTURE_MAX               10000
#define APP_ZIGBEE_MOISTURE_TOLERANCE         500     /* ~5 % tolerance */

#define APP_ZIGBEE_TEMP_START             4000                /* start 40.00 dB */

/* USER CODE END PD */

// -- Redefine Clusters to better code read --
#define TempMeasServer                    pstZbCluster[0]

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private constants ---------------------------------------------------------*/
/* USER CODE BEGIN PC */

/* USER CODE END PC */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static UTIL_TIMER_Object_t      stTimerUpdateMeasure;
static int16_t                  iTemperatureCurrent;
static int16_t                  iMoistureCurrent;

static struct ZbZclClusterT    *pstMoistureServer = NULL;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN PFP */
static void APP_ZIGBEE_ApplicationTaskInit        ( void );
static void APP_ZIGBEE_TempMeasAttributeUpdate    ( void );
static void APP_ZIGBEE_TimerUpdateCallback        ( void * arg );
static uint32_t APP_ZIGBEE_ReadADC(uint32_t ulChannel);

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/

void APP_ZIGBEE_ApplicationInit(void)
{
  LOG_INFO_APP( "ZIGBEE Application Init" );

  /* Initialization of the Zigbee stack */
  APP_ZIGBEE_Init();

  /* Configure Application Form/Join parameters : Startup, Persistence and Start with/without Form/Join */
  stZigbeeAppInfo.eStartupControl = ZbStartTypeJoin;
  stZigbeeAppInfo.bPersistNotification = false;
  stZigbeeAppInfo.bNwkStartup = true;

  /* USER CODE BEGIN APP_ZIGBEE_ApplicationInit */
  /* Initialization of used Tasks */
  APP_ZIGBEE_ApplicationTaskInit();

  /* USER CODE END APP_ZIGBEE_ApplicationInit */

  /* Initialize Zigbee stack layers */
  APP_ZIGBEE_StackLayersInit();
}

void APP_ZIGBEE_ApplicationStart( void )
{
  /* USER CODE BEGIN APP_ZIGBEE_ApplicationStart */
  /* Update default Temperature */
  iTemperatureCurrent = APP_ZIGBEE_TEMP_START;
  iMoistureCurrent    = 5000;                     /* start 50.00 % */

  /* Display Extended & Short Address */
  LOG_INFO_APP( "Use Short Address : 0x%04X", ZbShortAddress( stZigbeeAppInfo.pstZigbee ) );
  LOG_INFO_APP( "%s ready to work !", APP_ZIGBEE_APPLICATION_NAME );

  /* Start periodic Sensor Measure */
  UTIL_TIMER_Start( &stTimerUpdateMeasure );

  /* USER CODE END APP_ZIGBEE_ApplicationStart */

#if ( CFG_LPM_LEVEL != 0)
  /* Authorize LowPower now */
  UTIL_LPM_SetMaxMode( 1 << CFG_LPM_APP, UTIL_LPM_MAX_MODE );
#endif /* CFG_LPM_LEVEL */
}

void APP_ZIGBEE_ConfigEndpoints(void)
{
  struct ZbApsmeAddEndpointReqT   stRequest;
  struct ZbApsmeAddEndpointConfT  stConfig;

  /* USER CODE BEGIN APP_ZIGBEE_ConfigEndpoints1 */

  /* USER CODE END APP_ZIGBEE_ConfigEndpoints1 */

  memset( &stRequest, 0, sizeof( stRequest ) );
  memset( &stConfig, 0, sizeof( stConfig ) );

  stRequest.profileId = APP_ZIGBEE_PROFILE_ID;
  stRequest.deviceId = APP_ZIGBEE_DEVICE_ID;
  stRequest.endpoint = APP_ZIGBEE_ENDPOINT;
  ZbZclAddEndpoint( stZigbeeAppInfo.pstZigbee, &stRequest, &stConfig );
  assert( stConfig.status == ZB_STATUS_SUCCESS );

  stZigbeeAppInfo.TempMeasServer = ZbZclTempMeasServerAlloc( stZigbeeAppInfo.pstZigbee, APP_ZIGBEE_ENDPOINT, APP_ZIGBEE_TEMP_MIN, APP_ZIGBEE_TEMP_MAX, APP_ZIGBEE_TEMP_TOLERANCE );
  assert( stZigbeeAppInfo.TempMeasServer != NULL );
  if ( ZbZclClusterEndpointRegister( stZigbeeAppInfo.TempMeasServer ) == false )
  {
    LOG_ERROR_APP( "Error during TempMeas Server Endpoint Register." );
  }

  /* USER CODE BEGIN APP_ZIGBEE_ConfigEndpoints2 */
  memset( &stRequest, 0, sizeof( stRequest ) );
  memset( &stConfig, 0, sizeof( stConfig ) );

  stRequest.profileId = APP_ZIGBEE_PROFILE_ID;
  stRequest.deviceId = APP_ZIGBEE_DEVICE_ID;
  stRequest.endpoint = APP_ZIGBEE_ENDPOINT_MOISTURE;
  ZbZclAddEndpoint( stZigbeeAppInfo.pstZigbee, &stRequest, &stConfig );
  assert( stConfig.status == ZB_STATUS_SUCCESS );

  pstMoistureServer = ZbZclPressMeasServerAlloc( stZigbeeAppInfo.pstZigbee,
                                                 APP_ZIGBEE_ENDPOINT_MOISTURE,
                                                 APP_ZIGBEE_MOISTURE_MIN,
                                                 APP_ZIGBEE_MOISTURE_MAX);
  assert( pstMoistureServer != NULL );
  if ( ZbZclClusterEndpointRegister( pstMoistureServer ) == false )
  {
    LOG_ERROR_APP( "Error during Moisture (PressMeas) Server Endpoint Register." );
  }
  /* USER CODE END APP_ZIGBEE_ConfigEndpoints2 */
}

bool APP_ZIGBEE_ConfigGroupAddr( void )
{
  struct ZbApsmeAddGroupReqT  stRequest;
  struct ZbApsmeAddGroupConfT stConfig;

  memset( &stRequest, 0, sizeof( stRequest ) );

  stRequest.endpt = APP_ZIGBEE_ENDPOINT;
  stRequest.groupAddr = APP_ZIGBEE_GROUP_ADDRESS;
  ZbApsmeAddGroupReq( stZigbeeAppInfo.pstZigbee, &stRequest, &stConfig );

  return true;
}

void APP_ZIGBEE_GetStartupConfig( struct ZbStartupT * pstConfig )
{
  ZbStartupConfigGetProDefaults( pstConfig );

  memcpy( pstConfig->security.preconfiguredLinkKey, sec_key_ha, ZB_SEC_KEYSIZE );

  pstConfig->startupControl = stZigbeeAppInfo.eStartupControl;
  pstConfig->channelList.count = 1;
  pstConfig->channelList.list[0].page = 0;
  pstConfig->channelList.list[0].channelMask = APP_ZIGBEE_CHANNEL_MASK;

  if ( APP_ZIGBEE_SetTxPower( APP_ZIGBEE_TX_POWER ) == false )
  {
    LOG_ERROR_APP( "Switching to %d dB failed.", APP_ZIGBEE_TX_POWER );
    return;
  }
}

void APP_ZIGBEE_SetNewDevice( uint16_t iShortAddress, uint64_t dlExtendedAddress, uint8_t cCapability )
{
  LOG_INFO_APP( "New Device (%d) on Network : with Extended ( " LOG_DISPLAY64() " ) and Short ( 0x%04X ) Address.", cCapability, LOG_NUMBER64( dlExtendedAddress ), iShortAddress );
}

void APP_ZIGBEE_PrintApplicationInfo(void)
{
  LOG_INFO_APP( "**********************************************************" );
  LOG_INFO_APP( "Network config : CENTRALIZED ROUTER" );

  LOG_INFO_APP( "Application Flashed : Zigbee %s %s", APP_ZIGBEE_APPLICATION_NAME, APP_ZIGBEE_APPLICATION_OS_NAME );

  LOG_INFO_APP( "Channel used: %d.", APP_ZIGBEE_CHANNEL );

  APP_ZIGBEE_PrintGenericInfo();

  LOG_INFO_APP( "Clusters allocated are:" );
  LOG_INFO_APP( "  %s (SOUND) on Endpoint %d.", APP_ZIGBEE_CLUSTER_NAME, APP_ZIGBEE_ENDPOINT );
  LOG_INFO_APP( "  PressMeas (MOISTURE) on Endpoint %d.", APP_ZIGBEE_ENDPOINT_MOISTURE );

  LOG_INFO_APP( "**********************************************************" );
}

/* USER CODE BEGIN FD_LOCAL_FUNCTIONS */

static void APP_ZIGBEE_ApplicationTaskInit( void )
{
  UTIL_SEQ_RegTask( 1U << CFG_TASK_ZIGBEE_APP_SENSOR_READ, UTIL_SEQ_RFU, APP_ZIGBEE_TempMeasAttributeUpdate );

  UTIL_TIMER_Create( &stTimerUpdateMeasure, APP_ZIGBEE_TEMPMEAS_UPDATE_PERIOD, UTIL_TIMER_PERIODIC, APP_ZIGBEE_TimerUpdateCallback, NULL );
}

void APP_BSP_Button1Action(void)
{
  UTIL_TIMER_Start( &stTimerUpdateMeasure );
}

void APP_BSP_Button2Action(void)
{
  UTIL_TIMER_Stop( &stTimerUpdateMeasure );
}

static void APP_ZIGBEE_TimerUpdateCallback( void * arg )
{
  UTIL_SEQ_SetTask( 1u << CFG_TASK_ZIGBEE_APP_SENSOR_READ, TASK_ZIGBEE_APP_SENSOR_READ_PRIORITY );
}

static uint32_t APP_ZIGBEE_ReadADC(uint32_t ulChannel)
{
  uint32_t adc_raw_value = 0;

  HAL_ADC_Start(&hadc4);
  if (HAL_ADC_PollForConversion(&hadc4, 10) == HAL_OK)
  {
    adc_raw_value = HAL_ADC_GetValue(&hadc4);
  }
  HAL_ADC_Stop(&hadc4);

  return adc_raw_value;
}

static void APP_ZIGBEE_TempMeasAttributeUpdate( void )
{
  enum ZclStatusCodeT eStatus;

  uint32_t adc_sound = APP_ZIGBEE_ReadADC(SOUND_SENSOR_ADC_CHANNEL);

  float normalized_sound = 0.0f;
  if (adc_sound > SOUND_SENSOR_MIN_ADC)
  {
    normalized_sound = (float)(adc_sound - SOUND_SENSOR_MIN_ADC) /
                       (float)(SOUND_SENSOR_MAX_ADC - SOUND_SENSOR_MIN_ADC);
    if (normalized_sound > 1.0f) normalized_sound = 1.0f;
  }
  float sound_dB = SOUND_SENSOR_MIN_DB + normalized_sound * (SOUND_SENSOR_MAX_DB - SOUND_SENSOR_MIN_DB);
  iTemperatureCurrent = (int16_t)(sound_dB * 100.0f);

  LOG_INFO_APP( "[SOUND MEAS] Raw ADC: %lu → %.1f dB (sent: %d)", adc_sound, sound_dB, iTemperatureCurrent );

  eStatus = ZbZclAttrIntegerWrite(stZigbeeAppInfo.TempMeasServer,
                                  ZCL_TEMP_MEAS_ATTR_MEAS_VAL,
                                  iTemperatureCurrent);
  if (eStatus != ZCL_STATUS_SUCCESS)
    LOG_ERROR_APP( "[SOUND MEAS] Attribute Write error (0x%02X)", eStatus );

  uint32_t adc_moisture = APP_ZIGBEE_ReadADC(MOISTURE_SENSOR_ADC_CHANNEL);

  float normalized_moist = 0.0f;
  if (adc_moisture > MOISTURE_SENSOR_MIN_ADC)
  {
    normalized_moist = (float)(adc_moisture - MOISTURE_SENSOR_MIN_ADC) /
                       (float)(MOISTURE_SENSOR_MAX_ADC - MOISTURE_SENSOR_MIN_ADC);
    if (normalized_moist > 1.0f) normalized_moist = 1.0f;
  }
  float moisture_percent = MOISTURE_SENSOR_MIN_PERCENT + normalized_moist * (MOISTURE_SENSOR_MAX_PERCENT - MOISTURE_SENSOR_MIN_PERCENT);
  iMoistureCurrent = (int16_t)(moisture_percent * 100.0f);

  LOG_INFO_APP( "[MOISTURE MEAS] Raw ADC: %lu → %.1f %% (sent: %d)", adc_moisture, moisture_percent, iMoistureCurrent );

  eStatus = ZbZclAttrIntegerWrite(pstMoistureServer,
                                  ZCL_PRESS_MEAS_ATTR_MEAS_VAL,
								  iMoistureCurrent);
  if (eStatus != ZCL_STATUS_SUCCESS)
    LOG_ERROR_APP( "[MOISTURE MEAS] Attribute Write error (0x%02X)", eStatus );

  APP_LED_TOGGLE(LED_WORK);
}

/* USER CODE END FD_LOCAL_FUNCTIONS */
