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
#include "gpio_mcu.h"
#include "lcditse0803.h"

/*==================[macros and definitions]=================================*/
#define numero_a_mostrar 127
#define digitos_del_LCD 3

/*==================[internal data definition]===============================*/
typedef struct
{
    gpio_t pin;   /*!< GPIO pin number */
    io_t dir;     /*!< GPIO direction '0' IN;  '1' OUT */
} gpioConf_t;

/*==================[internal functions declaration]=========================*/
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number);

void setGpioFromBcd(uint8_t bcd_digit, gpioConf_t *gpio_array);

void displayNumber(uint32_t data, uint8_t digits, gpioConf_t *gpio_array, gpioConf_t *digit_array);

/*==================[external functions definition]==========================*/

int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number){
	for (int i= digits - 1; i>=0 ; i--){ // digits - 1 inicializa con el ultimo digito del arreglo; se recorre el arerglo de atrás para adelante hasta llegar a o; se le resta 1 en cada vuelta
		bcd_number[i] = data % 10; //tomo el ultimo digito (en la division quedo con el resto)
		data /= 10; //elimino ese digito (al data ser de tipo uint32_t es un entero)
	}

	return 0;
}

// Pines de datos (BCD -> GPIOs del 7 segmentos)
gpioConf_t gpio_map[4] = {
    {GPIO_20, 1},  // b0
    {GPIO_21, 1},  // b1
    {GPIO_22, 1},  // b2
    {GPIO_23, 1}   // b3
};

// Pines de selección de dígito
gpioConf_t digit_map[3] = {
    {GPIO_19, 1},  // Dígito 1
    {GPIO_18, 1},  // Dígito 2
    {GPIO_9,  1}   // Dígito 3
};



void setGpioFromBcd(uint8_t bcd_digit, gpioConf_t *gpio_array){
	for (int i=0 ; i < 4; i++){
		GPIOInit(gpio_array[i].pin, gpio_array[i].dir);
	}

	for (int i = 0; i < 4; i++){
		uint8_t bit_val = (bcd_digit >> i) & 0001 ; //pongo el bit de interes en la ultima posición y con & borro todo los demás 
		if (bit_val  == 1)
		{
			GPIOOn(gpio_array[i].pin);
			printf("GPIO ON %d\n", gpio_array[i].pin);
		}
		else
		{
			GPIOOff(gpio_array[i].pin);
			printf("GPIO OFF %d\n", gpio_array[i].pin);
		}
	}
}

void displayNumber(uint32_t data, uint8_t digits, gpioConf_t *gpio_array, gpioConf_t *digit_array){
	uint8_t bcd_digits[digits];

	convertToBcdArray(data, digits, bcd_digits); //uso la funcion para convertir el arreglo en BCD

	// Primero limpio todos los selectores de dígito
	for (int j = 0; j < digits; j++){
		GPIOInit(digit_array[j].pin, digit_array[j].dir);
		GPIOOff(digit_array[j].pin);
		printf("GPIO OFF %d\n", digit_array[j].pin);
	}
	 // 2. Recorro cada dígito
    for (int i = 0; i < digits; i++)
    {

        // Cargo el valor BCD en las líneas de datos
        setGpioFromBcd(bcd_digits[i], gpio_array);

        // Activo el dígito correspondiente
        GPIOOn(digit_array[i].pin);
        GPIOOff(digit_array[i].pin);
		printf("GPIO ON %d\n", digit_array[i].pin);
	}
}

void app_main(void)
{
    // Inicializo los GPIOs
    for(int i=0; i<4; i++) {
        GPIOInit(gpio_map[i].pin, gpio_map[i].dir);
    }
    for(int i=0; i<3; i++) {
        GPIOInit(digit_map[i].pin, digit_map[i].dir);
    }

    uint32_t numero = numero_a_mostrar;  
    uint8_t digitos = digitos_del_LCD;      

    // Mostrar el número
    displayNumber(numero, digitos, gpio_map, digit_map);

}
/*==================[end of file]============================================*/