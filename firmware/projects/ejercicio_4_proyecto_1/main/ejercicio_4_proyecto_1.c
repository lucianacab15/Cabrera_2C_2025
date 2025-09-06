/*! @mainpage Proyecto LCD con ESP32
 *
 * @section genDesc General Description
 *
 * Este programa permite mostrar un número en un display LCD de 7 segmentos
 * utilizando el conversor BCD-CD4543.  
 * Se implementan funciones para:
 * - Convertir un número decimal a un arreglo BCD.
 * - Configurar los GPIOs según un dígito BCD.
 * - Mostrar el número completo en el LCD manejando los pines de selección.
 *
 * @section hardConn Hardware Connection
 *
 * |    Señal    |   GPIO ESP32 |
 * |:-----------:|:------------:|
 * | BCD b0      |   GPIO_20    |
 * | BCD b1      |   GPIO_21    |
 * | BCD b2      |   GPIO_22    |
 * | BCD b3      |   GPIO_23    |
 * | Dígito 1    |   GPIO_19    |
 * | Dígito 2    |   GPIO_18    |
 * | Dígito 3    |   GPIO_9     |
 *
 * @section changelog Changelog
 *
 * |   Date       | Description                   |
 * |:------------:|:------------------------------|
 * | 05/09/2025   | Creación de la documentación  |
 *
 * @author Luciana
 */

/*==================[inclusions]=============================================*/
/**
 * @file lcd_display.c
 * @brief Funciones para mostrar números en un display LCD usando BCD y GPIOs.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio_mcu.h"
#include "lcditse0803.h"

/*==================[macros and definitions]=================================*/

#define numero_a_mostrar 127   /**< Número que se mostrará en el display */
#define digitos_del_LCD 3      /**< Cantidad de dígitos del display */

/*==================[internal data definition]===============================*/
/**
 * @struct gpioConf_t
 * @brief Estructura para configurar un pin GPIO.
 */

typedef struct
{
    gpio_t pin;   /*!< GPIO pin number */
    io_t dir;     /*!< GPIO direction '0' IN;  '1' OUT */
} gpioConf_t;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Convierte un número decimal en un arreglo BCD.
 *
 * @param[in] data Número en decimal (32 bits).
 * @param[in] digits Cantidad de dígitos a convertir.
 * @param[out] bcd_number Arreglo donde se guardan los dígitos en BCD.
 * @return int8_t Devuelve 0 si la conversión fue exitosa.
 */

int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number);

/**
 * @brief Configura los GPIOs según el valor de un dígito BCD.
 *
 * @param[in] bcd_digit Dígito BCD (0 a 9).
 * @param[in] gpio_array Vector de estructuras con los GPIO mapeados a b0-b3.
 */

void setGpioFromBcd(uint8_t bcd_digit, gpioConf_t *gpio_array);

/**
 * @brief Muestra un número en el LCD usando BCD y multiplexado.
 *
 * @param[in] data Número decimal de 32 bits.
 * @param[in] digits Cantidad de dígitos del LCD.
 * @param[in] gpio_array Vector de GPIOs para datos BCD.
 * @param[in] digit_array Vector de GPIOs para selección de dígitos.
 */

void displayNumber(uint32_t data, uint8_t digits, gpioConf_t *gpio_array, gpioConf_t *digit_array);

/*==================[external functions definition]==========================*/

int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number){
	for (int i= digits - 1; i>=0 ; i--){ 
		bcd_number[i] = data % 10; /**< Obtiene el último dígito */
		data /= 10; /**< Elimina ese dígito */
	}

	return 0;
}

/**
 * @brief Mapeo de pines GPIO a bits BCD.
 */
gpioConf_t gpio_map[4] = {
    {GPIO_20, 1},  /**< b0 */
    {GPIO_21, 1},  /**< b1 */
    {GPIO_22, 1},  /**< b2 */
    {GPIO_23, 1}   /**< b3 */
};

/**
 * @brief Mapeo de pines GPIO a dígitos del display.
 */
gpioConf_t digit_map[3] = {
    {GPIO_19, 1},  /**< Dígito 1 */
    {GPIO_18, 1},  /**< Dígito 2 */
    {GPIO_9,  1}   /**< Dígito 3 */
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

/**
 * @brief Función principal del programa.
 */

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