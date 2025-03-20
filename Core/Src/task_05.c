/*
 * task_05.c
 *
 *  Created on: Mar 21, 2025
 *      Author: Admin
 */
#include "task_05.h"

void StartTask05(void *argument)
{
	const char *mess = "Task: 05\r\n";
	for (;;)
	{
		printf(mess); // 81 words from stack
		printf("Worked: %.3f sec\r\n", HAL_GetTick() / 1000.0f);
		HAL_IWDG_Refresh(&hiwdg);
		osDelay(10000 / portTICK_RATE_MS);
	}
}
