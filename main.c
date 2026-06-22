/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for Hello World Example using HAL APIs.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2022-2024, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#include "cy_gpio.h"
#include "cy_pdl.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "cycfg_pins.h"
#include "FreeRTOS.h"
#include "task.h"


/*******************************************************************************
* Macros
*******************************************************************************/
#define USER_LED_PORT                     (GPIO_PRT1)
#define USER_LED_PIN                      (5U)

#define USER_BTN_PORT                     (GPIO_PRT0)
#define USER_BTN_PIN                      (4U)

/* Interrupt priority for the GPIO interrupt */
#define GPIO_INTERRUPT_PRIORITY           (5U)

/* Button-handling task configuration */
#define BUTTON_TASK_STACK_SIZE            (configMINIMAL_STACK_SIZE + 256U)
#define BUTTON_TASK_PRIORITY              (configMAX_PRIORITIES - 1)


/*******************************************************************************
* Global / File-scope Variables
*******************************************************************************/
/* Set by the GPIO ISR each time the user button is pressed */
static volatile bool button_flag = false;


/*******************************************************************************
* GPIO Interrupt Configuration
*******************************************************************************/
static const cy_stc_sysint_t intrCfg =
{
    CYBSP_USER_BTN_IRQ,
    GPIO_INTERRUPT_PRIORITY
};


/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void gpio_interrupt_handler(void);
static void btn_init(void);
static void button_task(void *args);


/*******************************************************************************
* Function Name: gpio_interrupt_handler
********************************************************************************
* Summary:
*  GPIO interrupt handler for the user button. Keeps the ISR minimal: it just
*  flags the press and clears the interrupt. All work is done in button_task.
*******************************************************************************/
static void gpio_interrupt_handler(void)
{
    if (0UL != Cy_GPIO_GetInterruptStatusMasked(USER_BTN_PORT, USER_BTN_PIN))
    {
        button_flag = true;
        Cy_GPIO_ClearInterrupt(USER_BTN_PORT, USER_BTN_PIN);
    }
}


/*******************************************************************************
* Function Name: btn_init
********************************************************************************
* Summary:
*  Initializes the user button pin and its falling-edge interrupt.
*******************************************************************************/
static void btn_init(void)
{
    /* Initialize the user button pin (input, pull-up) */
    Cy_GPIO_Pin_Init(CYBSP_USER_BTN_PORT, CYBSP_USER_BTN_PIN, &CYBSP_USER_BTN_config);

    /* A falling edge (press) must be configured or the IRQ never asserts */
    Cy_GPIO_SetInterruptEdge(CYBSP_USER_BTN_PORT, CYBSP_USER_BTN_PIN, CY_GPIO_INTR_FALLING);

    /* Clear any stale latched edge before enabling the NVIC line */
    Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN_PORT, CYBSP_USER_BTN_PIN);
    NVIC_ClearPendingIRQ(intrCfg.intrSrc);

    (void)Cy_SysInt_Init(&intrCfg, &gpio_interrupt_handler);
    NVIC_EnableIRQ(intrCfg.intrSrc);

    Cy_GPIO_SetInterruptMask(CYBSP_USER_BTN_PORT, CYBSP_USER_BTN_PIN, 1UL);
}


/*******************************************************************************
* Function Name: button_task
********************************************************************************
* Summary:
*  FreeRTOS task. On each button press it toggles the user LED.
*******************************************************************************/
static void button_task(void *args)
{
    (void)args;

    for (;;)
    {
        if (button_flag)
        {
            button_flag = false;

            /* Toggle the user LED */
            Cy_GPIO_Inv(USER_LED_PORT, USER_LED_PIN);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*  Initializes the board, button and LED, then starts the FreeRTOS scheduler.
*  A button task handles presses by toggling the LED.
*******************************************************************************/
int main(void)
{
    cy_rslt_t  result;
    BaseType_t xReturned;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init_fc(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
            CYBSP_DEBUG_UART_CTS, CYBSP_DEBUG_UART_RTS, CY_RETARGET_IO_BAUDRATE);
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Initialize the user LED (off by default) */
    Cy_GPIO_Pin_Init(USER_LED_PORT, USER_LED_PIN, &CYBSP_USER_LED_config);
    Cy_GPIO_Write(USER_LED_PORT, USER_LED_PIN, CYBSP_LED_STATE_OFF);

    /* Initialize the user button + interrupt */
    btn_init();

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("****************** "
           "PDL + FreeRTOS: Button / LED "
           "****************** \r\n\n");
    printf("Press the user button to toggle the LED\r\n\r\n");

    /* Create the button-handling task */
    xReturned = xTaskCreate(button_task,
                            "BTN_TASK",
                            BUTTON_TASK_STACK_SIZE,
                            NULL,
                            BUTTON_TASK_PRIORITY,
                            NULL);
    CY_ASSERT(pdPASS == xReturned);
    (void)xReturned;

    /* Start the FreeRTOS scheduler (does not return) */
    vTaskStartScheduler();

    /* Should never reach here */
    for (;;) {}
}

/* [] END OF FILE */

