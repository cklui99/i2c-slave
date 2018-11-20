/**
 * \file
 *
 * \brief ADC Basic driver implementation.
 *
 (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms,you may use this software and
    any derivatives exclusively with Microchip products.It is your responsibility
    to comply with third party license terms applicable to your use of third party
    software (including open source software) that may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 */

/**
 * \defgroup doc_driver_adc_basic ADC Basic Driver
 * \ingroup doc_driver_adc
 *
 * \section doc_driver_adc_rev Revision History
 * - v0.0.0.1 Initial Commit
 *
 *@{
 */
#include <adc_basic.h>
#include <board.h>

/** Function pointer to callback function called by IRQ.
    NULL=default value: No callback function is to be used.
*/
adc_irq_cb_t ADC_0_cb = NULL; /**
                               * \brief Initialize ADC interface
                               * If module is configured to disabled state, the clock to the ADC is disabled
                               * if this is supported by the device's clock system.
                               *
                               * \return Initialization status.
                               * \retval 0 the ADC init was successful
                               * \retval 1 the ADC init was not successful
                               */
int8_t ADC_0_init()
{

	// ADC0.CALIB = ADC_DUTYCYC_DUTY50_gc; /* 50% Duty cycle */

	// ADC0.CTRLB = ADC_SAMPNUM_ACC1_gc; /* 1 ADC sample */

	ADC0.CTRLC = ADC_PRESC_DIV16_gc     /* CLK_PER divided by 16 */
	             | ADC_REFSEL_INTREF_gc /* Internal reference */
	             | 0 << ADC_SAMPCAP_bp; /* Sample Capacitance Selection: disabled */

	// ADC0.CTRLD = 0 << ADC_ASDV_bp /* Automatic Sampling Delay Variation: disabled */
	//		 | 0x0 << ADC_SAMPDLY_gp /* Sampling Delay Selection: 0x0 */
	//		 | ADC_INITDLY_DLY0_gc; /* Delay 0 CLK_ADC cycles */

	// ADC0.CTRLE = ADC_WINCM_NONE_gc; /* No Window Comparison */

	ADC0.DBGCTRL = 1 << ADC_DBGRUN_bp; /* Debug run: enabled */

	// ADC0.EVCTRL = 0 << ADC_STARTEI_bp; /* Start Event Input Enable: disabled */

	ADC0.INTCTRL = 1 << ADC_RESRDY_bp  /* Result Ready Interrupt Enable: enabled */
	               | 0 << ADC_WCMP_bp; /* Window Comparator Interrupt Enable: disabled */

	// ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc; /* ADC input pin 0 */

	// ADC0.SAMPCTRL = 0x0 << ADC_SAMPLEN_gp; /* Sample length: 0x0 */

	// ADC0.WINHT = 0x0; /* Window Comparator High Threshold: 0x0 */

	// ADC0.WINLT = 0x0; /* Window Comparator Low Threshold: 0x0 */

	ADC0.CTRLA = 1 << ADC_ENABLE_bp     /* ADC Enable: enabled */
	             | 0 << ADC_FREERUN_bp  /* ADC Freerun mode: disabled */
	             | ADC_RESSEL_10BIT_gc  /* 10-bit mode */
	             | 1 << ADC_RUNSTBY_bp; /* Run standby mode: enabled */

	return 0;
}

/**
 * \brief Enable ADC_0
 * 1. If supported by the clock system, enables the clock to the ADC
 * 2. Enables the ADC module by setting the enable-bit in the ADC control register
 *
 * \return Nothing
 */
void ADC_0_enable()
{
	ADC0.CTRLA |= ADC_ENABLE_bm;
}
/**
 * \brief Disable ADC_0
 * 1. Disables the ADC module by clearing the enable-bit in the ADC control register
 * 2. If supported by the clock system, disables the clock to the ADC
 *
 * \return Nothing
 */
void ADC_0_disable()
{
	ADC0.CTRLA &= ~ADC_ENABLE_bm;
}

/**
 * \brief Start a conversion on ADC_0
 *
 * \param[in] channel The ADC channel to start conversion on
 *
 * \return Nothing
 */
void ADC_0_start_conversion(adc_0_channel_t channel)
{
	ADC0.MUXPOS  = channel;
	ADC0.COMMAND = ADC_STCONV_bm;
}

/**
 * \brief Check if the ADC conversion is done
 *
 * \return The status of ADC converison done check
 * \retval true The ADC conversion is done
 * \retval false The ADC converison is not done
 */
bool ADC_0_is_conversion_done()
{
	/* debug adc lockup */
	count++; //debug

	/* debug adc lockup */
	return (ADC0.INTFLAGS & ADC_RESRDY_bm);
}

/**
 * \brief Read a conversion result from ADC_0
 *
 * \return Conversion result read from the ADC_0 ADC module
 */
adc_result_t ADC_0_get_conversion_result(void)
{
	return (ADC0.RES);
}

/**
 * \brief Start a conversion, wait until ready, and return the conversion result
 *
 * \return Conversion result read from the ADC_0 ADC module
 */

/* 
*  Revised by CK Lui 
*  Date: Aug 14, 2018
*  Program is hung by the while loop waiting for the ADC result.
*  Add an counter to break the while loop if ADC result is delayed too long.
*
*/

adc_result_t ADC_0_get_conversion(adc_0_channel_t channel)
{
	adc_result_t res;
	//uint8_t count = 0;

	ADC_0_start_conversion(channel);
	//while (!ADC_0_is_conversion_done())
	//;

	count = 0;	//debug
	adc_err = 0; //debug
	while (!ADC_0_is_conversion_done() || count == 100)
		;

	/* avoid adc hangs the program */
	//while (!ADC_0_is_conversion_done()){
		//count++;
		//if (count == 0xFFFF) break;
		/*
		{
			ADC0.INTFLAGS &= ADC_RESRDY_bm;	// reset ADC Result Ready Interrupt Flag
			if (channel == 7) adc_err = 7;
			else if (channel == 10) adc_err = 10;
			break;
		}*/
	//}
	/*if (count == 0xFFFF){
				ADC0.INTFLAGS &= ADC_RESRDY_bm;	// reset ADC Result Ready Interrupt Flag
			if (channel == 7) adc_err = 7;
			else if (channel == 10) adc_err = 10;
	}*/

	//adc_err = count; //debug
	/* avoid adc hangs the program */
	if (count == 100) adc_err = channel;
	if (!adc_err) {
	res = ADC_0_get_conversion_result();
	ADC0.INTFLAGS |= ADC_RESRDY_bm;
	count = 0;	// reset count	
	}	

	return res;
}

/**
 * \brief Return the number of bits in the ADC conversion result
 *
 * \return The number of bits in the ADC conversion result
 */
uint8_t ADC_0_get_resolution()
{
	return (ADC0.CTRLA & ADC_RESSEL_bm) ? 8 : 10;
}

/**
 * \brief Register a callback function to be called at the end of the ADC ISR.
 *
 * \param[in] f Pointer to function to be called
 *
 * \return Nothing.
 */
void ADC_0_register_callback(adc_irq_cb_t f)
{
	ADC_0_cb = f;
}

ISR(ADC0_RESRDY_vect)
{
	// Clear the interrupt flag
	ADC0.INTFLAGS |= ADC_RESRDY_bm;

	if (ADC_0_cb != NULL) {
		ADC_0_cb();
	}
}
