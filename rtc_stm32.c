/*
 * rtc.c
 *
 *  Created on: 27 Feb 2019
 *      Author: Stefan Jaritz
 */

#include "rtc_stm32.h"

#include <zephyr.h>
#include <device.h>
#include <stm32f1xx_ll_rtc.h>
#include <stm32f1xx_ll_rcc.h>
#include <stm32f1xx_ll_bus.h>
#include <stm32f1xx_ll_pwr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(rtc_stm32, LOG_LEVEL_DBG);

// prescaler values for LSE @ 32768 Hz
#define RTC_PREDIV_ASYNC 0x7F
#define RTC_PREDIV_SYNC 0x00FF

static u8_t stm32_rtc_readValue(unsigned int v, const unsigned int mask, const unsigned int bytePos) {
	v = v & mask;
	v >>= (bytePos * 8);
	v = __LL_RTC_CONVERT_BCD2BIN(v);
	return v;
}

rtc_ts_t rtc_ts_fromRaw(rtc_raw_ts_t * ts) {
	rtc_ts_t r;

	r.d.day = stm32_rtc_readValue(ts->d,0x00FF0000,2);
	r.d.month = stm32_rtc_readValue(ts->d,0x0000FF00,1);;
	r.d.year = stm32_rtc_readValue(ts->d,0x000000FF,0);;

	r.t.hours = stm32_rtc_readValue(ts->t, 0x00FF0000,2);
	r.t.minutes = stm32_rtc_readValue(ts->t,0x0000FF00,1);
	r.t.seconds = stm32_rtc_readValue(ts->t,0x000000FF,0);

	r.ms = (1000 * (RTC_PREDIV_SYNC - ts->ss)) / (RTC_PREDIV_SYNC + 1);
	return r;
}

static void stm32_rtc_setTime(rtc_ts_t ts) {
/*	LL_RTC_DateTypeDef rtc_date = {
		.Year = ts.d.year,
		.Month = ts.d.month,
		.Day = ts.d.day,
		.WeekDay = ts.d.weekday
	};

	LL_RTC_TimeTypeDef rtc_time = {
		.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24,
		.Hours = ts.t.hours,
		.Minutes = ts.t.minutes,
		.Seconds = ts.t.seconds
	};

	if (LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_date) != SUCCESS) {
		LOG_ERR("set date failed");
	}

	if (LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_time) != SUCCESS) {
		LOG_ERR("set time failed");
	}

	LOG_INF("set RTC clock to %02u-%02u-%02u %02u:%02u:%02u:%04u",
		(unsigned int) ts.d.day,
		(unsigned int) ts.d.month,
		(unsigned int) ts.d.year,
		(unsigned int) ts.t.hours,
		(unsigned int) ts.t.minutes,
		(unsigned int) ts.t.seconds,
		(unsigned int) ts.ms
	);*/
}

static int stm32_rtc_initilize(struct device *dev) {
	(void) dev;

	LL_PWR_EnableBkUpAccess();
	LL_RCC_ForceBackupDomainReset();
	LL_RCC_ReleaseBackupDomainReset();

	LL_RCC_LSE_Enable();

	/* Wait untill LSE is ready */
	while (LL_RCC_LSE_IsReady() != 1) {};

	LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
	LL_RCC_EnableRTC();

    LL_PWR_EnableBkUpAccess();
    /* Enable BKP CLK enable for backup registers */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_BKP);
  /* Peripheral clock enable */
    LL_RCC_EnableRTC();

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC and set the Time and Date 
  */
  LL_RTC_InitTypeDef RTC_InitStruct = {0};

  RTC_InitStruct.AsynchPrescaler = 0xFFFFFFFFU;
  LL_RTC_Init(RTC, &RTC_InitStruct);
  LL_RTC_SetAsynchPrescaler(RTC, 0xFFFFFFFFU);
  /** Initialize RTC and set the Time and Date 
  */
  LL_RTC_TimeTypeDef RTC_TimeStruct = {0};
  RTC_TimeStruct.Hours = 20;
  RTC_TimeStruct.Minutes = 11;
  RTC_TimeStruct.Seconds = 22;
  LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BCD, &RTC_TimeStruct);


	if (LL_RTC_DeInit(RTC) != SUCCESS) {
		return -EIO;
	}

	if (LL_RTC_Init(RTC, &RTC_InitStruct) != SUCCESS) {
		return -EIO;
	}

	//  LL_RTC_EnableShadowRegBypass(RTC);

	// stm32_rtc_setTime(startTime);

	LOG_INF("RTC initialised");
	return 0;
}

static void stm32_rtc_getTimestamp(rtc_raw_ts_t * ts) {
	// get sub section
	// ts->ss = (u16_t) LL_RTC_TIME_GetSubSecond(RTC);
	// 0x00HHMMSS in bcd format
	ts->t = LL_RTC_TIME_Get(RTC);
	// 0xWWDDMMYY
	// ts->d = LL_RTC_DATE_Get(RTC);
}

void rtc_setTime (rtc_ts_t st) {
	stm32_rtc_setTime(st);
}

void rtc_getTimestamp (rtc_raw_ts_t * ts) {
	stm32_rtc_getTimestamp(ts);
}


static const rtc_api_t rtc_stm32_API = {
	.getTimestamp = stm32_rtc_getTimestamp,
	.setTime = stm32_rtc_setTime,
};

DEVICE_AND_API_INIT(rtcStm32, RTC_NAME, &stm32_rtc_initilize,
	NULL, NULL,
	POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
	&rtc_stm32_API
	);
