/******************************************************************************
* File Name : cy_profiler.h
*
* Description :
* Public interface for the DWT cycle-count based MIPS profiler (profiler.c).
*******************************************************************************/
#ifndef _CY_PROFILER_H_
#define _CY_PROFILER_H_

#include <inttypes.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
* Low level DWT cycle counter helpers
*******************************************************************************/

/* Initialize the DWT cycle counter. Must be called once before profiling. */
void DWTCyCNTInit(void);

/* Reset the DWT cycle counter to 0. */
uint32_t Cy_Reset_Cycles(void);

/* Read the current DWT cycle counter value. */
uint32_t Cy_Get_Cycles(void);

/*******************************************************************************
* Profiler public API
*******************************************************************************/

/* Initialize the profiler (enables the DWT cycle counter). */
void cy_profiler_init(void);

/* Start a measurement (resets the cycle counter). */
void cy_profiler_start(void);

/* Stop a measurement (latches the elapsed cycle count). */
void cy_profiler_stop(void);

/* Get the number of cycles captured between start and stop. */
uint32_t cy_profiler_get_cycles(void);

/* Deinitialize the profiler (disables the DWT cycle counter). */
void cy_profiler_deinit(void);

#if defined(__cplusplus)
}
#endif

#endif /* _CY_PROFILER_H_ */

/* [] END OF FILE */
