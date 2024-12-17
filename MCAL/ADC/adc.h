/*************************************************************/
/* Author  : Zeyad El-Gedawy 								 */
/* Version : V01			 								 */
/* Date	   : 03 October		 								 */
/*************************************************************/

#ifndef				ADC_H
#define				ADC_H

#include "std_types.h"


#define ADC0_SEQ3_PRIORITY_MASK             0xFFFF1FFF
#define ADC0_SEQ3_PRIORITY_BITS_POS         13
#define ADC0_SEQ3_INTERRUPT_PRIORITY        5

#define ADC1_SEQ3_PRIORITY_MASK             0x1FFFFFFF
#define ADC1_SEQ3_PRIORITY_BITS_POS         29
#define ADC1_SEQ3_INTERRUPT_PRIORITY        5

void ADC_vInit(void);

void ADC0_vStartConversion(void);
void ADC1_vStartConversion(void);

void ADC0_vRead(uint32* ADC_u32Value);
void ADC1_vRead(uint32* ADC_u32Value);

#endif
