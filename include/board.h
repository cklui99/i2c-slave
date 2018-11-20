/****************************************************************************
//  board.h
//  I2C communication on custom power board as a slave
//
//  Created by CK Lui on April 4, 2018.
//  Copyright © 2018 CK Lui. All rights reserved.
//
//  Revision: 0.1	Date: May 10, 2018
//	Added +5V specifications MIN_5V and MAX_5V.
//	Added v5AdcRegH & v5AdcRegL for 5V ADC result.
//	Deleted "BoardStatusReg.ac12vFault" flag.
//	Added "BoardStatusReg.dc5vStatus" flag.
//	Added prototype function "check_5V_status".
//	Deleted "adc_result" variable.
//	Added "check_debug_PB3(void)" feature.
//
//  Revision: 0.2	Date: Aug 14, 2018
//	Changed MAX_5VIN value to match the hardware setting.
//
//  Revision: 0.3	Date: Aug 29, 2018
//	Changed MAX_5VIN value to match the hardware setting.
//
****************************************************************************/

#ifndef BOARD_H
#define BOARD_H

/****************************************************************************
  Global definitions
****************************************************************************/
  
union BoardStatusReg_t                       // Status byte holding flags.
{
    unsigned char all;
    struct
    {
        unsigned char board:1;			//Board config, default Beaglephone
		unsigned char ac12vOn:1;		//AC12V on flag; 0: off 1: on
		//unsigned char ac12vFault:1;		//AC12V fault status flag; 0: no 1: yes
		unsigned char ac12vStatus:1;	//AC12V status flag; 0: bad 1: good
		unsigned char dc5vStatus:1;		//+5V status flag; 0: bad 1: good			
        unsigned char chargerOn:1;		//Charger status flag; 0: off 1: on
        unsigned char chargingDone:1;	//Charging done status flag; 0: no 1: yes
		unsigned char ntcFault:1;		//NTC fault status flag; 0: no 1: yes
		unsigned char batteryFault:1;   //Bad battery fault status flag; 0: no 1: yes 
    };
};

extern union BoardStatusReg_t BoardStatusReg;
extern uint8_t vinAdcRegH;
extern uint8_t vinAdcRegL;
extern uint16_t vinAdc;
extern uint8_t v5AdcRegH;
extern uint8_t v5AdcRegL;
extern uint16_t v5Adc;
extern uint8_t shdnReg;
extern uint8_t chargerReg;
//extern uint8_t adc_rdy;

/****************************************************************************
  Function definitions
****************************************************************************/
void check_board_config(void);	//Check board config
//void check_debug_PB3(void); //Check PB3 for debugging mode
void check_AC12V_status(adc_result_t AC12V_ADC); // Check AC 12V input status
void check_5V_status(adc_result_t DC5V_ADC); // Check DC +5V output status
void check_charger_status(void); // Check charger status
//volatile uint16_t adc_result;	// 10-bit ADC result
void ADC_start_conversion(void); // Start ADC function
void ADC_get_result(void); // Get ADC result

/****************************************************************************
  Bit and byte definitions
****************************************************************************/
//#define MIN_12V	0x29D	//9.60VIN or 3.27VSENSE
#define MIN_12V	0x2A9	//9.60VIN or 2.89VSENSE
//#define MAX_12V	0x3EE	//14.40VIN or 4.91VSENSE
#define MAX_12V	0X3FF	//14.40VIN or 4.34VSENSE
//#define MIN_5V	0x390	//4.9V or 4.45VSENSE
//#define MAX_5V	0x3B5	//5.1V or 4.63VSENSE
//#define MIN_5VIN	0x21B	//5.08V or 2.29VSENSE
#define MIN_5VIN	0x15E	//5.98V or 1.48VSENSE
//#define MAX_5VIN	0x3D5	//9.24V or 4.16VSENSE
#define MAX_5VIN	0x226	//9.40V or 2.33VSENSE
#define MAX_GND	0x14	//0.3VIN or 0.1VSENSE

#define ENABLE_SUPPLY	0x0		// Enable DC-DC supply
#define DISABLE_SUPPLY	0xFF	// Disable DC-DC supply
#define RESET_SUPPLY	0xAA	// Reset DC-DC supply

#define TRUE          1
#define FALSE         0

/****************************************************************************
  TWI State codes
****************************************************************************/
// General charger status codes                      
#define CHARGER_RESTART	0xFF
#define CHARGER_DISABLE	0x55

#endif
