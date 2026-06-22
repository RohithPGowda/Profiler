/*******************************************************************************
* File Name        : main.c
*
* Description      : Main routine for CM55 CPU.
*                    Button-driven start/stop of PDM→AFE→I2S streaming loopback.
*
* Related Document : See README.md
*
********************************************************************************
* Copyright 2023-2025, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cy_gpio.h"
/*******************************************************************************
* Macros
*******************************************************************************/
#define GPIO_INTERRUPT_PRIORITY   (5U)
#define STREAMING_TASK_STACK_SIZE (1024U)
#define STREAMING_TASK_PRIORITY   (configMAX_PRIORITIES - 1)

/*******************************************************************************
* Global / File-scope Variables
*******************************************************************************/
static volatile bool button_flag  = false;
static volatile bool is_streaming = false;

/*******************************************************************************
* GPIO Interrupt Configuration
*******************************************************************************/
static const cy_stc_sysint_t intrCfg =
{
    CYBSP_USER_BTN_IRQ,
    GPIO_INTERRUPT_PRIORITY
};

/*******************************************************************************
* Function Name: gpio_interrupt_handler
*******************************************************************************/
static void gpio_interrupt_handler(void)
{
    if (Cy_GPIO_GetInterruptStatus(CYBSP_USER_BTN_PORT, CYBSP_USER_BTN_PIN))
    {
        button_flag = true;
        Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN_PORT, CYBSP_USER_BTN_PIN);
    }
}

/*******************************************************************************
* Function Name: btn_init
*******************************************************************************/
static void btn_init(void)
{
    Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN_PORT, CYBSP_USER_BTN_PIN);
    NVIC_ClearPendingIRQ(intrCfg.intrSrc);
    Cy_SysInt_Init(&intrCfg, &gpio_interrupt_handler);
    NVIC_EnableIRQ(intrCfg.intrSrc);
}

/*******************************************************************************
* Function Name: streaming_task
*******************************************************************************/
static void streaming_task(void *args)
{
    (void)args;

    for (;;)
    {
        if (button_flag)
        {
            button_flag = false;

            if (!is_streaming)
            {
                is_streaming = true;

               
               // app_i2s_activate();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*******************************************************************************
* Function Name: main
*******************************************************************************/
int main(void)
{
    cy_rslt_t  result;
    BaseType_t xReturned;

  
    result = cybsp_init();
    if (CY_RSLT_SUCCESS != result)
    {
        __disable_irq();
        CY_ASSERT(0);
        while (true) {}
    }


    __enable_irq();

    /* ── LED off by default ───────────────────────────────────────────── */
 Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN, 0);

    /* ── Peripheral init ──────────────────────────────────────────────── */
    btn_init();
  
    xReturned = xTaskCreate(streaming_task,
                            "STREAM_TASK",
                            STREAMING_TASK_STACK_SIZE,
                            NULL,
                            STREAMING_TASK_PRIORITY,
                            NULL);
    CY_ASSERT(xReturned == pdPASS);
    (void)xReturned;

    vTaskStartScheduler();

    for (;;) {}
}

/* [] END OF FILE */
