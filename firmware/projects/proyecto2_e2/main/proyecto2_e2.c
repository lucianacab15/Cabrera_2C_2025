/**
 * @file main.c
 * @brief Aplicación con ESP32 que mide distancia mediante el sensor ultrasónico HC-SR04
 *        y muestra los resultados en un display LCD ITSE0803, además de indicar el rango
 *        mediante LEDs. Controla la medición con temporizador e interrupciones de teclas.
 *
 * @details
 * El sistema utiliza un **timer MCU** para generar interrupciones periódicas que notifican
 * una tarea encargada de medir y mostrar la distancia. Las teclas de usuario permiten:
 * - Pausar/reanudar la medición.
 * - Activar o desactivar el modo de retención (hold).
 *
 * - **SW1:** Habilita o deshabilita la medición.
 * - **SW2:** Activa/desactiva el modo HOLD (mantener el valor mostrado).
 *
 * Se emplean:
 * - **Tareas FreeRTOS:** para procesar las mediciones y actualizar la interfaz.
 * - **Interrupciones por GPIO:** para manejo de teclas.
 * - **Timer MCU:** para temporizar la adquisición de datos.
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
#include "timer_mcu.h"

/*==================[macros and definitions]=================================*/

/**
 * @brief Período entre mediciones de distancia (en milisegundos).
 */
#define CONFIG_MEASURE_PERIOD     300

/**
 * @brief Umbrales de distancia en centímetros para el encendido de LEDs.
 */
#define MIN_DIST   10   /**< Distancia mínima: <10 cm, LEDs apagados. */
#define MED_DIST   20   /**< Distancia media: entre 10 y 20 cm. */
#define MAX_DIST   30   /**< Distancia máxima: entre 20 y 30 cm. */

/*==================[internal data definition]===============================*/

/** @brief Handle de la tarea de medición y visualización. */
TaskHandle_t Medir_MostrarPantalla_task_handle = NULL;

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

/**
 * @brief Función de callback del temporizador.
 *
 * @param[in] pvParameter Parámetro del callback (no utilizado).
 *
 * @details
 * Esta función se ejecuta cada vez que el temporizador MCU vence su período,
 * notificando a la tarea `Medir_MostrarPantalla_Task` para que realice una nueva medición.
 */
void Medir_MostrarPantalla(void *pvParameter)
{
    vTaskNotifyGiveFromISR(Medir_MostrarPantalla_task_handle, pdFALSE);
}

/*==================[tasks definition]=======================================*/
/**
 * @brief Tarea encargada de medir la distancia y actualizar la pantalla LCD y los LEDs.
 *
 * @param[in] pvParameter Parámetro de tarea (no utilizado).
 *
 * @details
 * La tarea queda bloqueada esperando una notificación del timer.
 * Al recibirla, mide la distancia con el sensor HC-SR04 y actualiza la pantalla
 * y los LEDs según las banderas `medir` y `hold`.
 */
static void Medir_MostrarPantalla_Task(void *pvParameter)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint16_t distancia = 0;
        if (medir == true) {
            distancia = HcSr04ReadDistanceInCentimeters();
        }

        if (hold == false && medir == true) {
            encenderLedSegunDistancia(distancia);
            LcdItsE0803Write(distancia);
        }
        // Si hold == true, mantiene la última medición mostrada
    }
}

/**
 * @brief Callback de la tecla 1.
 *
 * @details
 * Alterna la bandera `medir`.  
 * Si se desactiva la medición:
 * - Apaga todos los LEDs.
 * - Limpia el display LCD.
 */
void tecla_1()
{
    medir = !medir;
    if (medir == false) {
        LedsOffAll();
        LcdItsE0803Off();
    }
}

/**
 * @brief Callback de la tecla 2.
 *
 * @details
 * Alterna la bandera `hold` (modo de retención del valor mostrado).
 */
void tecla_2()
{
    hold = !hold;
}

/*==================[external functions definition]==========================*/
/**
 * @brief Función principal de la aplicación.
 *
 * @details
 * Inicializa todos los periféricos (LEDs, LCD, sensor ultrasónico, teclas y timer),
 * configura las interrupciones de teclas y crea la tarea principal de medición.
 * Luego inicia el temporizador que dispara periódicamente la medición.
 */
void app_main(void)
{
    LedsInit();
    LcdItsE0803Init();
    HcSr04Init(GPIO_3, GPIO_2);
    SwitchesInit();

    timer_config_t timer_medir = {
        .timer = TIMER_A,
        .period = CONFIG_MEASURE_PERIOD * 1000, /**< Conversión de ms a µs. */
        .func_p = Medir_MostrarPantalla,
        .param_p = NULL
    };
    TimerInit(&timer_medir);

    SwitchActivInt(SWITCH_1, tecla_1, NULL);
    SwitchActivInt(SWITCH_2, tecla_2, NULL);

    xTaskCreate(&Medir_MostrarPantalla_Task, "MEDIR_Y_MOSTRAR", 2048, NULL, 5, &Medir_MostrarPantalla_task_handle); 

    TimerStart(timer_medir.timer);
}

/*==================[end of file]============================================*/
