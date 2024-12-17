/*************************************************************/
/* Author  : Zeyad El-Gedawy 								 */
/* Version : V01			 								 */
/* Date	   : 03 October		 								 */
/*************************************************************/

#include "adc.h"
#include "tm4c123gh6pm_registers.h"

void ADC_vInit(void)
{
	// Enable the ADC clock
	
	SYSCTL_RCGCADC_REG |= 0x00000003;
	while(!(SYSCTL_PRADC_REG & 0x03));
	
	// Enable the clock to GPIO_E
	
	SET_BIT(SYSCTL_RCGCGPIO_REG,4);
	
	// Set the GPIO_E AFSEL bits for the ADC 
	
	SET_BIT(GPIO_PORTE_AFSEL_REG,2);
	SET_BIT(GPIO_PORTE_AFSEL_REG,3);
	
	// Configure the analog input by clearing the corresponding DEN bit 
	
	CLR_BIT(GPIO_PORTE_DEN_REG,2);
	CLR_BIT(GPIO_PORTE_DEN_REG,3);
	
	// Disable the analog isolation circuit for ADC input pins 
	
	SET_BIT(GPIO_PORTE_AMSEL_REG,2);
	SET_BIT(GPIO_PORTE_AMSEL_REG,3);
	
	// Ensure that the sample sequencer is disabled by clearing the corresponding ASEN bit
	
	ADC0_ACTSS &= 0x00;

	
	// Configure the trigger event for the sample sequencer in the ADCEMUX register
	
	ADC0_EMUX   &= ~(0x0000F000);
	

	// Configure the sample sequence input source (ADC channel 0 & 1)
	
	ADC0_SSMUX3 = (0x00000000);

	
	// Configuration information for each sample
	
	SET_BIT(ADC0_SSCTL3,1);
	SET_BIT(ADC0_SSCTL3,2);
	
	// If interrupts are to be used, set the corresponding MASK bit in the ADCIM register.
	SET_BIT(ADC0_ISC,3);    // Clear ADC 0 Interrupt flag
	SET_BIT(ADC0_IM,3);
	NVIC_PRI4_REG = (NVIC_PRI4_REG & ADC0_SEQ3_PRIORITY_MASK) | (ADC0_SEQ3_INTERRUPT_PRIORITY<<ADC0_SEQ3_PRIORITY_BITS_POS); // Set ADC 0 Priority
	SET_BIT(NVIC_EN0_REG,17);
	
	// Enable the sample sequencer 
	
	SET_BIT(ADC0_ACTSS,3);
	
	/***************************/
	ADC1_ACTSS &= 0x00;

	ADC1_EMUX   &= ~(0x0000F000);

	ADC1_SSMUX3 =   0x00000001 ;

	SET_BIT(ADC1_SSCTL3,1);
    SET_BIT(ADC1_SSCTL3,2);

    SET_BIT(ADC1_ISC,3);    // Clear ADC 1 Interrupt flag
    SET_BIT(ADC1_IM,3);
    NVIC_PRI12_REG = (NVIC_PRI12_REG & ADC1_SEQ3_PRIORITY_MASK) | (ADC1_SEQ3_INTERRUPT_PRIORITY<<ADC1_SEQ3_PRIORITY_BITS_POS); // Set ADC 1 Priority
    SET_BIT(NVIC_EN1_REG,19);

    SET_BIT(ADC1_ACTSS,3);
}
void ADC0_vStartConversion(void)
{
	SET_BIT(ADC0_PSSI,3);
}
void ADC1_vStartConversion(void)
{
	SET_BIT(ADC1_PSSI,3);
}
void ADC0_vRead(uint32* ADC_u32Value)
{	
	*ADC_u32Value = ADC0_FIFO3;
}
void ADC1_vRead(uint32* ADC_u32Value)
{	
	*ADC_u32Value = ADC1_FIFO3;
}
