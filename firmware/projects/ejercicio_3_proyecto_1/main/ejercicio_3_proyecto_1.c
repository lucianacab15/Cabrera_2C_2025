/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD 500
#define  ON 1
#define OFF 0
#define TOGGLE  2

/*==================[internal data definition]===============================*/
// struct Tleds
// {
//     uint8_t mode;      // ON, OFF, TOGGLE
// 	uint8_t n_led;        //indica el número de led a controlar
// 	uint8_t n_ciclos;   //indica la cantidad de ciclos de encendido/apagado
// 	uint16_t periodo;    //indica el tiempo de cada ciclo
// }my_leds; 

// CON ESTO DEFINO LA ESTRUCTURA
struct Tleds // T de tipo
{
    uint8_t mode;      // ON, OFF, TOGGLE
	uint8_t n_led;        //indica el número de led a controlar
	uint8_t n_ciclos;   //indica la cantidad de ciclos de encendido/apagado
	uint16_t periodo;    //indica el tiempo de cada ciclo
}; 

//CON ESTO GUARDO LA ESTRUCTURA EN MY_LEDS -- OCUPA LUGAR EN LA MEMORIA
struct Tleds my_leds; 


/*==================[internal functions declaration]=========================*/
void function_leds(struct Tleds * ptr);

/*==================[external functions definition]==========================*/

void app_main(void){
	uint8_t teclas;
	LedsInit();
	SwitchesInit();
	
    my_leds.mode =0;
	my_leds.n_led=;
	my_leds.n_ciclos=;
	my_leds.periodo=;



	// function_leds(&my_leds) -- OTRA FORMA DE LO DE ABAJO
	
	struct Tleds * ptrstruct;
	ptrstruct = &my_leds;
	
	
	// ptrstruct -> mode =0;
	function_leds(ptrstruct); // LLAMO A LA FUNCION

}


void function_leds(struct Tleds * ptr)
{
	switch(ptr -> mode){
		case ON:
			switch (ptr -> n_led)
			{
			case LED_1:
				LedOn(LED_1)
				break;

			case LED_2:
				LedOn(LED_2)
				break;

			case LED_3:
				LedOn(LED_3)
				break;
			
			}

		case OFF:
			switch (ptr -> n_led)
			{
			case LED_1:
				LedOff(LED_1)
				break;

			case LED_2:
				LedOff(LED_2)
				break;

			case LED_3:
				LedOff(LED_3)
				break;
			
			}

		case TOGGLE:
			for (n_ciclos, i< n_ciclos, i++){
				switch (ptr -> n_led){
					case LED_1:
						LedToggle(LED_1)
						break;
					
					case LED_2;
						LedToggle(LED_2)
						break;

					case LED_3;
						LedToggle(LED_3)
						break;

					default:
						break;
				}
			}
	}
}

/*==================[end of file]============================================*/