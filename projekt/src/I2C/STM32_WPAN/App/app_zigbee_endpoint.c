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

/* USER CODE BEGIN PI */
#include "app_bsp.h"
#include "main.h"

/* USER CODE END PI */

/* Private defines -----------------------------------------------------------*/
#define APP_ZIGBEE_CHANNEL                25u
#define APP_ZIGBEE_CHANNEL_MASK           ( 1u << APP_ZIGBEE_CHANNEL )
#define APP_ZIGBEE_TX_POWER               ((int8_t) 10)    /* TX-Power is at +10 dBm. */

#define APP_ZIGBEE_ENDPOINT               17u
#define APP_ZIGBEE_PROFILE_ID             ZCL_PROFILE_HOME_AUTOMATION
#define APP_ZIGBEE_DEVICE_ID              ZCL_DEVICE_TEMPERATURE_SENSOR
#define APP_ZIGBEE_GROUP_ADDRESS          0x0001u

#define APP_ZIGBEE_CLUSTER_ID             ZCL_CLUSTER_MEAS_TEMPERATURE
#define APP_ZIGBEE_CLUSTER_NAME           "TempMeas Server"

/* MeasTemperature specific defines ----------------------------------------------------*/
#define APP_ZIGBEE_TEMP_MIN               -4000
#define APP_ZIGBEE_TEMP_MAX               12500
#define APP_ZIGBEE_TEMP_TOLERANCE         50
/* USER CODE BEGIN MeasTemperature defines */
/* USER CODE END MeasTemperature defines */

/* USER CODE BEGIN PD */
#define APP_ZIGBEE_STARTUP_FAIL_DELAY     500u

#define APP_ZIGBEE_TEMP_START             2500
#define APP_ZIGBEE_TEMPMEAS_UPDATE_PERIOD (uint32_t)( 500u

#define APP_ZIGBEE_APPLICATION_NAME       APP_ZIGBEE_CLUSTER_NAME
#define APP_ZIGBEE_APPLICATION_OS_NAME    "."

/* -- Redefine task to better code read -- */
#define CFG_TASK_ZIGBEE_APP_SENSOR_READ         CFG_TASK_ZIGBEE_APP1
#define TASK_ZIGBEE_APP_SENSOR_READ_PRIORITY    CFG_SEQ_PRIO_1

#define APP_ZIGBEE_BMP180_I2C             (&hi2c1)
#define BMP180_I2C_ADDR                   (0x77u << 1)
#define BMP180_REG_CALIB_START            0xAAu
#define BMP180_REG_CONTROL                0xF4u
#define BMP180_REG_DATA_MSB               0xF6u
#define BMP180_REG_CHIP_ID                0xD0u
#define BMP180_CHIP_ID_VALUE              0x55u
#define BMP180_CMD_TEMP                   0x2Eu
#define BMP180_TEMP_CONV_DELAY_MS         5u
#define BMP180_I2C_TIMEOUT_MS             100u

/* USER CODE END PD */

/* -- Redefine Clusters to better code read -- */
#define TempMeasServer                    pstZbCluster[0]

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef struct
{
  int16_t   AC1;
  int16_t   AC2;
  int16_t   AC3;
  uint16_t  AC4;
  uint16_t  AC5;
  uint16_t  AC6;
  int16_t   B1;
  int16_t   B2;
  int16_t   MB;
  int16_t   MC;
  int16_t   MD;
} BMP180_CalibT;

/* USER CODE END PTD */

/* Private constants ---------------------------------------------------------*/
/* USER CODE BEGIN PC */

/* USER CODE END PC */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
extern I2C_HandleTypeDef hi2c1;
static UTIL_TIMER_Object_t  stTimerUpdateMeasure;
static int16_t              iTemperatureCurrent;
static BMP180_CalibT        stBMP180Calib;
static bool                 bBMP180Ready = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN PFP */
static void APP_ZIGBEE_ApplicationTaskInit        ( void );
static void APP_ZIGBEE_TempMeasAttributeUpdate    ( void );
static void APP_ZIGBEE_TimerUpdateCallback        ( void * arg );

static bool BMP180_Init                           ( void );
static bool BMP180_ReadRawTemperature             ( int32_t * pRawTemp );
static bool BMP180_GetTemperature                 ( int16_t * pTempHundredths );

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/

/**
 * @brief  Zigbee application initialization
 * @param  None
 * @retval None
 */
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

/**
 * @brief  Zigbee application start
 * @param  None
 * @retval None
 */
void APP_ZIGBEE_ApplicationStart( void )
{
  /* USER CODE BEGIN APP_ZIGBEE_ApplicationStart */
  /* Fallback hodnota kym nepride prvy odcitok zo senzora */
  iTemperatureCurrent = APP_ZIGBEE_TEMP_START;

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

/**
 * @brief  Configure Zigbee application endpoints
 * @param  None
 * @retval None
 */
void APP_ZIGBEE_ConfigEndpoints(void)
{
  struct ZbApsmeAddEndpointReqT   stRequest;
  struct ZbApsmeAddEndpointConfT  stConfig;
  /* USER CODE BEGIN APP_ZIGBEE_ConfigEndpoints1 */

  /* USER CODE END APP_ZIGBEE_ConfigEndpoints1 */

  /* Add EndPoint */
  memset( &stRequest, 0, sizeof( stRequest ) );
  memset( &stConfig, 0, sizeof( stConfig ) );

  stRequest.profileId = APP_ZIGBEE_PROFILE_ID;
  stRequest.deviceId = APP_ZIGBEE_DEVICE_ID;
  stRequest.endpoint = APP_ZIGBEE_ENDPOINT;
  ZbZclAddEndpoint( stZigbeeAppInfo.pstZigbee, &stRequest, &stConfig );
  assert( stConfig.status == ZB_STATUS_SUCCESS );

  /* Add TempMeas Server Cluster */
  stZigbeeAppInfo.TempMeasServer = ZbZclTempMeasServerAlloc( stZigbeeAppInfo.pstZigbee, APP_ZIGBEE_ENDPOINT, APP_ZIGBEE_TEMP_MIN, APP_ZIGBEE_TEMP_MAX, APP_ZIGBEE_TEMP_TOLERANCE );
  assert( stZigbeeAppInfo.TempMeasServer != NULL );
  if ( ZbZclClusterEndpointRegister( stZigbeeAppInfo.TempMeasServer ) == false )
  {
    LOG_ERROR_APP( "Error during TempMeas Server Endpoint Register." );
  }

  /* USER CODE BEGIN APP_ZIGBEE_ConfigEndpoints2 */

  /* USER CODE END APP_ZIGBEE_ConfigEndpoints2 */
}

/**
 * @brief  Set Group Addressing mode (if used)
 * @param  None
 * @retval 'true' if Group Address used else 'false'.
 */
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

/**
 * @brief  Return the Startup Configuration
 * @param  pstConfig  Configuration structure to fill
 * @retval None
 */
void APP_ZIGBEE_GetStartupConfig( struct ZbStartupT * pstConfig )
{
  /* Attempt to join a zigbee network */
  ZbStartupConfigGetProDefaults( pstConfig );

  /* Using the default HA preconfigured Link Key */
  memcpy( pstConfig->security.preconfiguredLinkKey, sec_key_ha, ZB_SEC_KEYSIZE );

  /* Setting up additional startup configuration parameters */
  pstConfig->startupControl = stZigbeeAppInfo.eStartupControl;
  pstConfig->channelList.count = 1;
  pstConfig->channelList.list[0].page = 0;
  pstConfig->channelList.list[0].channelMask = APP_ZIGBEE_CHANNEL_MASK;

  /* Set the TX-Power */
  if ( APP_ZIGBEE_SetTxPower( APP_ZIGBEE_TX_POWER ) == false )
  {
    LOG_ERROR_APP( "Switching to %d dB failed.", APP_ZIGBEE_TX_POWER );
    return;
  }

  /* USER CODE BEGIN APP_ZIGBEE_GetStartupConfig */

  /* USER CODE END APP_ZIGBEE_GetStartupConfig */
}

/**
 * @brief  Manage a New Device on Network (called only if Coord or Router).
 * @param  iShortAddress      Short Address of new Device
 * @param  dlExtendedAddress  Extended Address of new Device
 * @param  cCapability        Capability of new Device
 * @retval Group Address
 */
void APP_ZIGBEE_SetNewDevice( uint16_t iShortAddress, uint64_t dlExtendedAddress, uint8_t cCapability )
{
  LOG_INFO_APP( "New Device (%d) on Network : with Extended ( " LOG_DISPLAY64() " ) and Short ( 0x%04X ) Address.", cCapability, LOG_NUMBER64( dlExtendedAddress ), iShortAddress );

  /* USER CODE BEGIN APP_ZIGBEE_SetNewDevice */

  /* USER CODE END APP_ZIGBEE_SetNewDevice */
}

/**
 * @brief  Print application information to the console
 * @param  None
 * @retval None
 */
void APP_ZIGBEE_PrintApplicationInfo(void)
{
  LOG_INFO_APP( "**********************************************************" );
  LOG_INFO_APP( "Network config : CENTRALIZED ROUTER" );

  /* USER CODE BEGIN APP_ZIGBEE_PrintApplicationInfo1 */
  LOG_INFO_APP( "Application Flashed : Zigbee %s %s", APP_ZIGBEE_APPLICATION_NAME, APP_ZIGBEE_APPLICATION_OS_NAME );

  /* USER CODE END APP_ZIGBEE_PrintApplicationInfo1 */
  LOG_INFO_APP( "Channel used: %d.", APP_ZIGBEE_CHANNEL );

  APP_ZIGBEE_PrintGenericInfo();

  LOG_INFO_APP( "Clusters allocated are:" );
  LOG_INFO_APP( "  %s on Endpoint %d.", APP_ZIGBEE_CLUSTER_NAME, APP_ZIGBEE_ENDPOINT );

  /* USER CODE BEGIN APP_ZIGBEE_PrintApplicationInfo2 */

  /* USER CODE END APP_ZIGBEE_PrintApplicationInfo2 */

  LOG_INFO_APP( "**********************************************************" );
}

/* USER CODE BEGIN FD_LOCAL_FUNCTIONS */

static bool BMP180_Init( void )
{
  uint8_t  ucChipId = 0u;
  uint8_t  aucCalib[22];
  HAL_StatusTypeDef eStatus;

  /* Overenie Chip ID - musi byt 0x55 */
  eStatus = HAL_I2C_Mem_Read( APP_ZIGBEE_BMP180_I2C,
                               BMP180_I2C_ADDR,
                               BMP180_REG_CHIP_ID,
                               I2C_MEMADD_SIZE_8BIT,
                               &ucChipId, 1u,
                               BMP180_I2C_TIMEOUT_MS );
  if ( ( eStatus != HAL_OK ) || ( ucChipId != BMP180_CHIP_ID_VALUE ) )
  {
    LOG_ERROR_APP( "[BMP180] Sensor not found or wrong chip ID (0x%02X). Check wiring & I2C address.", ucChipId );
    return false;
  }

  eStatus = HAL_I2C_Mem_Read( APP_ZIGBEE_BMP180_I2C,
                               BMP180_I2C_ADDR,
                               BMP180_REG_CALIB_START,
                               I2C_MEMADD_SIZE_8BIT,
                               aucCalib, sizeof(aucCalib),
                               BMP180_I2C_TIMEOUT_MS );
  if ( eStatus != HAL_OK )
  {
    LOG_ERROR_APP( "[BMP180] Failed to read calibration data." );
    return false;
  }

  /* Rozlozenie kalibracnych bajtov (Big-Endian, MSB first) */
  stBMP180Calib.AC1 = (int16_t) ( ((uint16_t)aucCalib[0]  << 8u) | aucCalib[1] );
  stBMP180Calib.AC2 = (int16_t) ( ((uint16_t)aucCalib[2]  << 8u) | aucCalib[3] );
  stBMP180Calib.AC3 = (int16_t) ( ((uint16_t)aucCalib[4]  << 8u) | aucCalib[5] );
  stBMP180Calib.AC4 = (uint16_t)( ((uint16_t)aucCalib[6]  << 8u) | aucCalib[7] );
  stBMP180Calib.AC5 = (uint16_t)( ((uint16_t)aucCalib[8]  << 8u) | aucCalib[9] );
  stBMP180Calib.AC6 = (uint16_t)( ((uint16_t)aucCalib[10] << 8u) | aucCalib[11] );
  stBMP180Calib.B1  = (int16_t) ( ((uint16_t)aucCalib[12] << 8u) | aucCalib[13] );
  stBMP180Calib.B2  = (int16_t) ( ((uint16_t)aucCalib[14] << 8u) | aucCalib[15] );
  stBMP180Calib.MB  = (int16_t) ( ((uint16_t)aucCalib[16] << 8u) | aucCalib[17] );
  stBMP180Calib.MC  = (int16_t) ( ((uint16_t)aucCalib[18] << 8u) | aucCalib[19] );
  stBMP180Calib.MD  = (int16_t) ( ((uint16_t)aucCalib[20] << 8u) | aucCalib[21] );

  LOG_INFO_APP( "[BMP180] Sensor initialized OK. AC1=%d AC5=%u AC6=%u MC=%d MD=%d",
                stBMP180Calib.AC1, stBMP180Calib.AC5, stBMP180Calib.AC6,
                stBMP180Calib.MC, stBMP180Calib.MD );
  return true;
}

static bool BMP180_ReadRawTemperature( int32_t * pRawTemp )
{
  uint8_t           ucCmd    = BMP180_CMD_TEMP;
  uint8_t           aucData[2];
  HAL_StatusTypeDef eStatus;

  eStatus = HAL_I2C_Mem_Write( APP_ZIGBEE_BMP180_I2C,
                                BMP180_I2C_ADDR,
                                BMP180_REG_CONTROL,
                                I2C_MEMADD_SIZE_8BIT,
                                &ucCmd, 1u,
                                BMP180_I2C_TIMEOUT_MS );
  if ( eStatus != HAL_OK )
  {
    LOG_ERROR_APP( "[BMP180] Failed to write CONTROL register." );
    return false;
  }

  HAL_Delay( BMP180_TEMP_CONV_DELAY_MS );

  eStatus = HAL_I2C_Mem_Read( APP_ZIGBEE_BMP180_I2C,
                               BMP180_I2C_ADDR,
                               BMP180_REG_DATA_MSB,
                               I2C_MEMADD_SIZE_8BIT,
                               aucData, 2u,
                               BMP180_I2C_TIMEOUT_MS );
  if ( eStatus != HAL_OK )
  {
    LOG_ERROR_APP( "[BMP180] Failed to read temperature data." );
    return false;
  }

  *pRawTemp = (int32_t)( ((uint16_t)aucData[0] << 8u) | aucData[1] );
  return true;
}

static bool BMP180_GetTemperature( int16_t * pTempHundredths )
{
  int32_t   lUT;
  int32_t   lX1, lX2, lB5;
  int32_t   lTrueTemp;   /* jednotky 0.1 degC */

  if ( !BMP180_ReadRawTemperature( &lUT ) )
  {
    return false;
  }

  lX1 = ( ( lUT - (int32_t)stBMP180Calib.AC6 ) * (int32_t)stBMP180Calib.AC5 ) >> 15;
  lX2 = ( (int32_t)stBMP180Calib.MC << 11 ) / ( lX1 + (int32_t)stBMP180Calib.MD );
  lB5 = lX1 + lX2;
  lTrueTemp = ( lB5 + 8 ) >> 4;

  *pTempHundredths = (int16_t)( lTrueTemp * 10 );

  return true;
}

/* ==========================================================================
 * Zigbee Application Tasks
 * ========================================================================== */

/**
 * @brief  Zigbee application Task initialization
 * @param  None
 * @retval None
 */
static void APP_ZIGBEE_ApplicationTaskInit( void )
{
  bBMP180Ready = BMP180_Init();
  if ( !bBMP180Ready )
  {
    LOG_ERROR_APP( "[BMP180] Init FAILED - temperature readings will not be available!" );
  }

  UTIL_SEQ_RegTask( 1U << CFG_TASK_ZIGBEE_APP_SENSOR_READ, UTIL_SEQ_RFU, APP_ZIGBEE_TempMeasAttributeUpdate );

  UTIL_TIMER_Create( &stTimerUpdateMeasure, APP_ZIGBEE_TEMPMEAS_UPDATE_PERIOD, UTIL_TIMER_PERIODIC, APP_ZIGBEE_TimerUpdateCallback, NULL );
}

/**
 * @brief  Management of the SW1 button. Start periodic measure of Temperature.
 * @param  None
 * @retval None
 */
void APP_BSP_Button1Action(void)
{
  UTIL_TIMER_Start( &stTimerUpdateMeasure );
}

/**
 * @brief  Management of the SW2 button. Stop periodic measure of Temperature.
 * @param  None
 * @retval None
 */
void APP_BSP_Button2Action(void)
{
  UTIL_TIMER_Stop( &stTimerUpdateMeasure );
}

/**
 * @brief  Management of the UpdateTimer Callback to launch a Temperature Read Task
 * @param  arg    Argument
 * @retval None
 */
static void APP_ZIGBEE_TimerUpdateCallback( void * arg )
{
  UTIL_SEQ_SetTask( 1u << CFG_TASK_ZIGBEE_APP_SENSOR_READ, TASK_ZIGBEE_APP_SENSOR_READ_PRIORITY );
}

static void APP_ZIGBEE_TempMeasAttributeUpdate( void )
{
  int16_t             iTempNew;
  enum ZclStatusCodeT eStatus;
  char                szText[10];
  int16_t             iTempBP;
  uint8_t             cTempAP;

  if ( bBMP180Ready )
  {
    if ( BMP180_GetTemperature( &iTempNew ) )
    {
      iTemperatureCurrent = iTempNew;
    }
    else
    {
      LOG_ERROR_APP( "[BMP180] Read failed, keeping last value (%d).", iTemperatureCurrent );
    }
  }
  else
  {
    LOG_WARNING_APP( "[TEMP MEAS] BMP180 not ready, skipping read." );
    return;
  }

  if ( iTemperatureCurrent > APP_ZIGBEE_TEMP_MAX )
  {
    iTemperatureCurrent = APP_ZIGBEE_TEMP_MAX;
  }
  else if ( iTemperatureCurrent < APP_ZIGBEE_TEMP_MIN )
  {
    iTemperatureCurrent = APP_ZIGBEE_TEMP_MIN;
  }

  iTempBP = (int16_t)( iTemperatureCurrent / 100 );
  cTempAP = (uint8_t)( iTemperatureCurrent % 100u );
  snprintf( szText, sizeof(szText), "%d.%02d", iTempBP, cTempAP );
  LOG_INFO_APP( "[TEMP MEAS] BMP180 Temperature : %s C", szText );
  APP_LED_TOGGLE(LED_WORK);

  eStatus = ZbZclAttrIntegerWrite( stZigbeeAppInfo.TempMeasServer, ZCL_TEMP_MEAS_ATTR_MEAS_VAL, iTemperatureCurrent );
  if ( eStatus != ZCL_STATUS_SUCCESS )
  {
    LOG_ERROR_APP( "[TEMP MEAS] Attribute Write error (0x%02X)", eStatus );
  }
}

/* USER CODE END FD_LOCAL_FUNCTIONS */
