/**
 * @file main.c
 * @brief Aplicación con ESP32 que mide distancia mediante el sensor ultrasónico HC-SR04,
 *        muestra los resultados en un display LCD ITSE0803, los indica mediante LEDs y los
 *        transmite por UART a una PC. Permite cambiar unidades, frecuencia y modos de medición.
 *
 * @details
 * Este programa utiliza el sensor HC-SR04 para medir distancias y las presenta tanto en una 
 * interfaz local (LCD + LEDs) como por puerto serie hacia la PC. El usuario puede interactuar
 * mediante teclas físicas y comandos UART.
 *
 * **Controles disponibles:**
 * - **SW1 / tecla 'O' (ON/OFF):** Activa o desactiva la medición.
 * - **SW2 / tecla 'H' (HOLD):** Congela la lectura actual.
 * - **Tecla 'I':** Cambia las unidades entre centímetros (cm) y pulgadas (in).
 * - **Tecla 'F':** Aumenta la frecuencia de medición (disminuye el período del timer).
 * - **Tecla 'S':** Disminuye la frecuencia de medición (aumenta el período del timer).
 *
 * **Periféricos utilizados:**
 * - **Sensor ultrasónico:** HC-SR04 (distancia en cm o pulgadas).
 * - **Display LCD:** ITSE0803.
 * - **LEDs:** Indicadores de rango (<10, 10–20, 20–30, >30 cm).
 * - **UART PC:** Comunicación serie con la computadora.
 * - **Timer MCU:** Genera interrupciones periódicas para disparar la medición.
 *
 * **Arquitectura del sistema:**
 * - Una tarea FreeRTOS (`Medir_MostrarPantalla_Task`) mide, muestra y transmite las lecturas.
 * - Interrupciones de teclas (SW1, SW2) cambian estados de medición y retención.
 * - Entrada UART permite comandos remotos equivalentes.
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
#include "uart_mcu.h"

/*==================[macros and definitions]=================================*/

/**
 * @brief Período inicial entre mediciones (en microsegundos).
 */
#define CONFIG_MEASURE_PERIOD 1000 * 1000    

/**
 * @brief Período entre lecturas de teclas (en milisegundos).
 */
#define CONFIG_READING_PERIOD 20             

/**
 * @brief Valores límite para la variación del tiempo de lectura (en microsegundos).
 */
#define TIEMPO_DE_LECTURA_MINIMO 100 * 1000  /**< 100 ms mínimo. */
#define TIEMPO_DE_LECTURA_MAXIMO 2000 * 1000 /**< 2000 ms máximo. */

/**
 * @brief Paso de ajuste del tiempo de lectura (en milisegundos).
 */
#define TIEMPO_DE_LECTURA_STEP 100           

/**
 * @brief Umbrales de distancia en centímetros para encendido de LEDs.
 */
#define MIN_DIST 10 /**< <10 cm, todos los LEDs apagados. */
#define MED_DIST 20 /**< 10–20 cm, LED1 encendido. */
#define MAX_DIST 30 /**< 20–30 cm, LED1 y LED2 encendidos. */

/*==================[internal data declaration]==============================*/

/**
 * @brief Prototipo del callback de medición usado por el timer MCU.
 */
void Medir_MostrarPantalla(void *pvParameter);

/**
 * @brief Prototipo del callback de lectura UART.
 */
void leer_teclado(void *param);

/*==================[internal data definition]===============================*/

/** @brief Handle de la tarea principal de medición. */
TaskHandle_t Medir_MostrarPantalla_task_handle = NULL;

/** @brief Bandera de activación de la medición. */
bool medir = true;

/** @brief Bandera de retención (mantener última medición). */
bool hold = false;

/** @brief Bandera de selección de unidades (false = cm, true = pulgadas). */
bool pulgadas = false;

/**
 * @brief Configuración del timer MCU utilizado para disparar las mediciones.
 */
timer_config_t timer_medir = {
    .timer = TIMER_A,
    .period = CONFIG_MEASURE_PERIOD,
    .func_p = Medir_MostrarPantalla,
    .param_p = NULL,
};

/**
 * @brief Configuración del puerto UART conectado a la PC.
 */
serial_config_t my_uart = {
    .port = UART_PC,        /**< Puerto UART principal. */
    .baud_rate = 19200,     /**< Velocidad de transmisión. */
    .func_p = leer_teclado, /**< Función de lectura por software (polling). */
    .param_p = NULL,
};

/*==================[internal functions declaration]=========================*/
/**
 * @brief Enciende los LEDs según el rango de distancia medido.
 *
 * @param[in] distance Distancia medida en centímetros.
 */
void encenderLedSegunDistancia(uint32_t distance)
{
    if (distance < MIN_DIST) { 
        LedsOffAll();
    }
    else if (distance >= MIN_DIST && distance < MED_DIST) { 
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
 * @brief Callback del temporizador: notifica a la tarea de medición.
 *
 * @param[in] pvParameter Parámetro de callback (no utilizado).
 */
void Medir_MostrarPantalla(void *pvParameter)
{
    vTaskNotifyGiveFromISR(Medir_MostrarPantalla_task_handle, pdFALSE);
}

/**
 * @brief Callback asociado a la tecla física SW1.
 *
 * @details Activa o desactiva la medición. Si se desactiva, apaga LEDs y display.
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
 * @brief Callback asociado a la tecla física SW2.
 *
 * @details Alterna el modo HOLD (retención del valor en pantalla).
 */
void tecla_2()
{
    hold = !hold;
}

/**
 * @brief Lee comandos recibidos por UART desde la PC y ejecuta acciones.
 *
 * @param[in] param Parámetro opcional (no utilizado).
 *
 * @details
 * Comandos válidos:
 * - `'O'`: alterna la bandera de medición (`tecla_1()`).
 * - `'H'`: alterna la bandera de retención (`tecla_2()`).
 * - `'I'`: cambia unidades cm/pulgadas.
 * - `'F'`: aumenta frecuencia de medición (reduce período del timer).
 * - `'S'`: disminuye frecuencia de medición (aumenta período del timer).
 */
void leer_teclado(void *param)
{
    uint8_t letra;
    UartReadByte(UART_PC, &letra);
    UartSendByte(UART_PC, (char *)&letra); // eco

    switch (letra)
    {
    case 'O': tecla_1(); break;
    case 'H': tecla_2(); break;
    case 'I': pulgadas = !pulgadas; break;
    case 'F':
        if (timer_medir.period > TIEMPO_DE_LECTURA_MINIMO) {
            timer_medir.period -= TIEMPO_DE_LECTURA_STEP;
            TimerUpdatePeriod(timer_medir.timer, timer_medir.period);
        }
        break;
    case 'S':
        if (timer_medir.period < TIEMPO_DE_LECTURA_MAXIMO) {
            timer_medir.period += TIEMPO_DE_LECTURA_STEP;
            TimerUpdatePeriod(timer_medir.timer, timer_medir.period);
        }
        break;
    default: break;
    }
}

/**
 * @brief Envía la distancia medida a través del puerto serie hacia la PC.
 *
 * @param[in] distancia Valor numérico de la distancia medida.
 *
 * @details
 * El formato del mensaje enviado es:
 * ```
 * Distancia: <valor> cm\r\n
 * ```
 * o
 * ```
 * Distancia: <valor> in\r\n
 * ```
 * según la unidad seleccionada.
 */
void mandar_distancia(uint16_t distancia)
{
    UartSendString(UART_PC, "Distancia: ");
    const char* numero = UartItoa(distancia, 10);
    UartSendString(UART_PC, numero);
    if (pulgadas == true)
        UartSendString(UART_PC, " in\r\n");
    else
        UartSendString(UART_PC, " cm\r\n");
}

/*==================[tasks definition]=======================================*/
/**
 * @brief Tarea principal que mide, muestra y transmite la distancia.
 *
 * @param[in] pvParameter Parámetro de tarea (no utilizado).
 *
 * @details
 * Espera notificaciones del temporizador para realizar mediciones.
 * Dependiendo de las banderas activas:
 * - Mide en cm o en pulgadas.
 * - Envía el valor por UART.
 * - Actualiza LEDs y display LCD (si `hold == false`).
 */
static void Medir_MostrarPantalla_Task(void *pvParameter)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint16_t distancia = 0;

        if (medir == true && pulgadas == false) {
            distancia = HcSr04ReadDistanceInCentimeters();
            mandar_distancia(distancia);
        }
        else if (medir == true && pulgadas == true) {
            distancia = HcSr04ReadDistanceInInches();
            mandar_distancia(distancia);
        }

        if (hold == false && medir == true) {
            encenderLedSegunDistancia(distancia);
            LcdItsE0803Write(distancia);
        }
    }
}

/*==================[external functions definition]==========================*/
/**
 * @brief Función principal del programa.
 *
 * @details
 * Inicializa todos los periféricos (LEDs, LCD, sensor, teclas, UART y timer),
 * configura las interrupciones y crea la tarea principal.  
 * Luego inicia el temporizador para comenzar la adquisición periódica.
 */
void app_main(void)
{
    LedsInit();
    LcdItsE0803Init();
    HcSr04Init(GPIO_3, GPIO_2);
    SwitchesInit();
    TimerInit(&timer_medir);
    UartInit(&my_uart);

    SwitchActivInt(SWITCH_1, tecla_1, NULL);
    SwitchActivInt(SWITCH_2, tecla_2, NULL);

    xTaskCreate(&Medir_MostrarPantalla_Task, "MEDIR_Y_MOSTRAR", 2048, NULL, 5, &Medir_MostrarPantalla_task_handle);
    TimerStart(timer_medir.timer);
}

/*==================[end of file]============================================*/
