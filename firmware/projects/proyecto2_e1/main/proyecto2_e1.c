/**
 * @file main.c
 * @brief Aplicación con ESP32 que mide distancia mediante el sensor ultrasónico HC-SR04 
 *        y muestra los resultados en un display LCD ITSE0803, además de indicar el rango 
 *        mediante LEDs. Controla las funciones de medición y retención (hold) mediante teclas.
 *
 * @details
 * El sistema realiza mediciones periódicas de distancia, actualizando el valor en pantalla y
 * encendiendo LEDs según el rango medido. Dos teclas permiten pausar/reanudar la medición y 
 * activar o desactivar el modo de retención del valor en pantalla.
 *
 * - **SW1:** Habilita o deshabilita la medición.
 * - **SW2:** Activa/desactiva el modo HOLD (mantiene el último valor mostrado).
 *
 * Se ejecutan dos tareas FreeRTOS:
 * - `Medir_MostrarPantalla_Task`: mide distancia y actualiza pantalla/LEDs.
 * - `Teclas_Task`: gestiona el estado de las teclas y banderas de control.
 *
 *  @section hardConn Hardware Connection
 *
 * |    Peripheral  | 	ESP32-C6	|
 * |:--------------:|:--------------|
 * | 	ECHO	 	| 	GPIO_3		|
 * | 	TRIGGER	 	| 	GPIO_2		|
 * | 	Vcc		 	| 	+5V			|
 * | 	GND		 	| 	GND			|
 * | 	LED_1	 	| 	GPIO_11		|
 * | 	LED_2	 	| 	GPIO_10		|
 * | 	LED_3	 	| 	GPIO_5		|
 * 
 * @author  Luciana Cabrera
 * @date    Octubre 2025
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "hc_sr04.h"
#include "lcditse0803.h"
#include "switch.h"

/*==================[macros and definitions]=================================*/

/**
 * @brief Período entre mediciones de distancia (en milisegundos).
 */
#define CONFIG_MEASURE_PERIOD     500

/**
 * @brief Período entre lecturas del estado de las teclas (en milisegundos).
 */
#define CONFIG_READING_PERIOD      20

/**
 * @brief Umbrales de distancia en centímetros para el encendido de LEDs.
 */
#define MIN_DIST   10   /**< Distancia mínima: <10 cm, LEDs apagados. */
#define MED_DIST   20   /**< Distancia media: entre 10 y 20 cm. */
#define MAX_DIST   30   /**< Distancia máxima: entre 20 y 30 cm. */

/*==================[internal data definition]===============================*/

/** @brief Handle de la tarea de medición y visualización. */
TaskHandle_t Medir_MostrarPantalla_task_handle = NULL;

/** @brief Handle de la tarea de lectura de teclas. */
TaskHandle_t teclas_task_handle = NULL;

/** @brief Bandera que indica si se está midiendo distancia. */
bool medir = true;

/** @brief Bandera que indica si está activo el modo HOLD (mantener valor). */
bool hold = false;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Enciende los LEDs según el rango de distancia medido.
 *
 * @param[in] distance Distancia medida en centímetros.
 *
 * @details
 * - <10 cm: Todos los LEDs apagados.  
 * - 10–20 cm: Enciende LED_1.  
 * - 20–30 cm: Enciende LED_1 y LED_2.  
 * - >30 cm: Enciende LED_1, LED_2 y LED_3.
 */
void encenderLedSegunDistancia(uint32_t distance)
{
    if (distance < MIN_DIST ) { 
        LedsOffAll();
    } 
    else if (distance >= MIN_DIST && distance < MED_DIST ) { 
        LedOn(LED_1);
        LedOff(LED_2);
        LedOff(LED_3);
    } 
    else if (distance >= MED_DIST && distance < MAX_DIST) { 
        LedOn(LED_1);
        LedOn(LED_2);
        LedOff(LED_3);
    } 
    else if (distance >= MAX_DIST) { 
        LedOn(LED_1);
        LedOn(LED_2);
        LedOn(LED_3);
    }
}

/*==================[tasks definition]=======================================*/
/**
 * @brief Tarea encargada de medir la distancia y actualizar la pantalla LCD y los LEDs.
 *
 * @param[in] pvParameter Parámetro de tarea (no utilizado).
 *
 * @details
 * - Si `medir == true`, se realiza una medición con el sensor HC-SR04.  
 * - Si `hold == false`, la distancia se muestra en el LCD y se actualizan los LEDs.  
 * - Si `hold == true`, se mantiene la última medición mostrada.  
 * - El ciclo se repite cada `CONFIG_MEASURE_PERIOD` milisegundos.
 */
static void Medir_MostrarPantalla_Task(void *pvParameter){
    while (true)
    {
        uint16_t distancia = 0;
        if (medir == true) {
            distancia = HcSr04ReadDistanceInCentimeters();
        }

        if (hold == false && medir == true){
            encenderLedSegunDistancia(distancia);
            LcdItsE0803Write(distancia);
        }

        vTaskDelay(CONFIG_MEASURE_PERIOD / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Tarea encargada de leer las teclas y actualizar las banderas de control.
 *
 * @param[in] pvParameter Parámetro de tarea (no utilizado).
 *
 * @details
 * - **SW1:** Activa/desactiva la medición (`medir`).  
 * - **SW2:** Activa/desactiva el modo de retención (`hold`).  
 * - Si se desactiva la medición, se apagan LEDs y display.
 */
static void Teclas_Task(void *pvParameter)
{
    while (1)
    {
        uint8_t switchOn = SwitchesRead();

        if (switchOn == SWITCH_1) { 
            medir = !medir;
            if (medir == false){
                LedsOffAll();
                LcdItsE0803Off();
            }
        }
        else if (switchOn == SWITCH_2) {
            hold = !hold;
        }

        vTaskDelay(CONFIG_READING_PERIOD / portTICK_PERIOD_MS);
    }
}

/*==================[external functions definition]==========================*/
/**
 * @brief Función principal de la aplicación.
 *
 * @details
 * Inicializa periféricos (LEDs, LCD, sensor y teclas) y crea las tareas FreeRTOS:
 * - `Medir_MostrarPantalla_Task`
 * - `Teclas_Task`
 */
void app_main(void)
{
    LedsInit();
    LcdItsE0803Init();
    HcSr04Init(GPIO_3, GPIO_2);
    SwitchesInit();

    xTaskCreate(&Medir_MostrarPantalla_Task, "MEDIR_Y_MOSTRAR", 2048, NULL, 5, &Medir_MostrarPantalla_task_handle); 
    xTaskCreate(&Teclas_Task, "TECLAS", 2048, NULL, 5, &teclas_task_handle);
}

/*==================[end of file]============================================*/
