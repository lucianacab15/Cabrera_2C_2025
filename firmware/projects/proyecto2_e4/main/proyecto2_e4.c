/**
 * @file main.c
 * @brief Ejemplo de lectura y escritura analógica periódica con FreeRTOS y periféricos MCU.
 *
 * Este programa utiliza el ADC (conversor analógico-digital) y el DAC (conversor digital-analógico)
 * de una MCU para realizar las siguientes tareas:
 * 
 *  - Leer una señal analógica desde el canal ADC_CH1.
 *  - Enviar el valor leído al puerto serie (UART) hacia la PC.
 *  - Generar una señal de salida (DAC) a partir de datos almacenados en un arreglo (por ejemplo, un ECG).
 * 
 * Las mediciones y escrituras se ejecutan mediante temporizadores (Timers) que notifican a tareas de FreeRTOS.
 * Cada tarea realiza su función al recibir la notificación desde la rutina de interrupción (ISR).
 * 
 * @note Este ejemplo demuestra cómo usar timers para generar tareas periódicas sin bloquear el sistema operativo.
 * 
 * @authors
 *   - Estudiante de Bioingeniería (Luciana Cabrera)
 * 
 * @date 2025
 */

/*==================[inclusiones]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"

/*==================[definiciones]============================================*/

/**
 * @brief Canal analógico a leer mediante el ADC.
 */
#define ADC_CHANNEL CH1

/**
 * @brief Puerto UART utilizado para comunicación con la PC.
 */
#define UART_PORT UART_PC

/**
 * @brief Velocidad de comunicación UART en baudios.
 */
#define UART_BAUDRATE 115200

/**
 * @brief Frecuencia de muestreo en Hz (lecturas por segundo).
 */
#define SAMPLE_FREQUENCY 1000

/**
 * @brief Cantidad de muestras en la señal de prueba (onda senoidal o ECG).
 */
#define SIGNAL_SIZE 231


/*==================[variables globales]======================================*/

/**
 * @brief Manejadores de tareas de FreeRTOS.
 */
TaskHandle_t analogReadAndSendTask_Handle = NULL; /**< Tarea que lee el ADC y envía por UART. */
TaskHandle_t analogWriteTask_handle = NULL;       /**< Tarea que escribe en el DAC. */

/**
 * @brief Variables globales de trabajo.
 */
uint32_t adc_value = 0;    /**< Valor leido del ADC. */
uint16_t signal_value = 0; /**< Valor a escribir en el DAC. */
uint16_t sample_index = 0; /**< Índice actual de la señal. */

/*==================[señales de prueba almacenadas en arreglos]================*/

/**
 * @brief Señal de prueba simple (incremental).
 */
const char test_signal[SIGNAL_SIZE] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
    101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
    121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
    141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
    161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
    181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
    201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220,
    221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231,
};


/**
 * @brief Señal de electrocardiograma (ECG) de ejemplo, con 231 muestras.
 */
const char ecg[SIGNAL_SIZE] = {

    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76,
};    

/**
 * @brief Otra señal de ECG más detallada (256 muestras).
 */
const char otro_ecg[256] = {
    17,17,17,17,17,17,17,17,17,17,17,18,18,18,17,17,17,17,17,17,17,18,18,18,18,18,18,18,17,17,
    16,16,16,16,17,17,18,18,18,17,17,17,17,18,18,19,21,22,24,25,26,27,28,29,31,32,33,34,34,35,
    37,38,37,34,29,24,19,15,14,15,16,17,17,17,16,15,14,13,13,13,13,13,13,13,12,12,10,6,2,3,
    15,43,88,145,199,237,252,242,211,167,117,70,35,16,14,22,32,38,37,32,27,24,24,26,27,28,28,27,28,28,
    30,31,31,31,32,33,34,36,38,39,40,41,42,43,45,47,49,51,53,55,57,60,62,65,68,71,75,79,83,87,
    92,97,101,106,111,116,121,125,129,133,136,138,139,140,140,139,137,133,129,123,117,109,101,
    92,84,77,70,64,58,52,47,42,39,36,34,31,30,28,27,26,25,25,25,25,25,25,25,25,24,24,24,24,25,
    25,25,25,25,25,25,24,24,24,24,24,24,24,24,23,23,22,22,21,21,21,20,20,20,20,20,19,19,18,18,
    18,19,19,19,19,18,17,17,18,18,18,18,18,18,18,18,17,17,17,17,17,17,17
};    


/*==================[funciones internas - prototipos]==========================*/
static void analogReadAndSend(void *param);
static void analogReadAndSendTask(void *param);
static void analogWrite(void *param);
static void analogWriteTask(void *param);
char sampleSignal(void);


/*==================[funciones internas]=======================================*/

/**
 * @brief Rutina de interrupción (ISR) del Timer A.
 *
 * Esta función se ejecuta periódicamente según el periodo configurado en el
 * `timer_medir`. Su única tarea es notificar a la tarea de FreeRTOS
 * asociada (`analogReadAndSendTask_Handle`) para que realice la lectura del ADC.
 *
 * @param param Parámetro no utilizado (NULL).
 */
static void analogReadAndSend(void *param){
    vTaskNotifyGiveFromISR(analogReadAndSendTask_Handle, pdFALSE);
}

/**
 * @brief Tarea que lee el ADC y envía el valor por UART.
 *
 * Cada vez que recibe una notificación desde el Timer A, esta tarea:
 * 1. Lee una muestra analógica desde el canal ADC.
 * 2. Convierte el valor leído a texto (usando `UartItoa()`).
 * 3. Envía el valor por UART a la PC.
 *
 * Ejemplo del formato de salida:
 * ```
 * >analog_voltage:1023
 * ```
 *
 * @param param Parámetro no utilizado (NULL).
 */
static void analogReadAndSendTask(void *param){
    while(1){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        AnalogInputReadSingle(ADC_CHANNEL, &adc_value);
        UartSendString(UART_PORT, ">analog_voltage:");
        UartSendString(UART_PORT, (char *)UartItoa(adc_value, 10));
        UartSendString(UART_PORT, "\r\n");
    }
}

/**
 * @brief Devuelve una muestra de la señal de prueba y actualiza el índice.
 *
 * Esta función actúa como un **generador de señal discreta**:
 * - Devuelve el valor actual del arreglo `test_signal`.
 * - Incrementa el índice `sample_index`.
 * - Cuando llega al final, vuelve a 0, repitiendo la señal.
 *
 * @return Valor de la señal actual convertido a `uint16_t`.
 *
 * @note Cambiando `test_signal` por `ecg` o `otro_ecg`, se puede reproducir una señal diferente.
 */
char sampleSignal(void){
    signal_value = test_signal[sample_index];
    if (sample_index < (sizeof(test_signal)/sizeof(test_signal[0]) - 1)){
        sample_index++;
    } else {
        sample_index = 0;
    }
    return ((uint16_t)signal_value);
}

/**
 * @brief Rutina de interrupción (ISR) del Timer B.
 *
 * Se ejecuta periódicamente para indicar a la tarea de escritura analógica
 * (`analogWriteTask_handle`) que debe actualizar la salida DAC con la siguiente muestra.
 *
 * @param param Parámetro no utilizado (NULL).
 */
static void analogWrite(void *param){
    vTaskNotifyGiveFromISR(analogWriteTask_handle, pdFALSE);
}

/**
 * @brief Tarea encargada de escribir valores en el DAC.
 *
 * Cada vez que recibe una notificación desde el Timer B:
 * 1. Llama a `sampleSignal()` para obtener la siguiente muestra del arreglo.
 * 2. Escribe esa muestra en la salida analógica mediante `AnalogOutputWrite()`.
 *
 * De este modo, se reconstruye una señal analógica periódica a partir de los datos discretos.
 *
 * @param param Parámetro no utilizado (NULL).
 */
static void analogWriteTask(void *param){
    while(1){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        AnalogOutputWrite(sampleSignal());
    }
}


/*==================[función principal]========================================*/

/**
 * @brief Función principal del programa.
 *
 * Inicializa todos los periféricos y crea las tareas y timers necesarios.
 * 
 * - Configura el ADC en modo lectura simple.
 * - Inicializa el DAC.
 * - Configura la UART para enviar datos a la PC.
 * - Crea dos timers (A y B) con igual frecuencia de muestreo:
 *   - Timer A → Notifica la tarea de lectura (ADC + UART).
 *   - Timer B → Notifica la tarea de escritura (DAC).
 * - Crea las tareas correspondientes y arranca ambos timers.
 */
void app_main(void){
    analog_input_config_t adc_config = {
        .input = ADC_CHANNEL,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL,
        .sample_frec = 0
    };
    AnalogInputInit(&adc_config);
    AnalogOutputInit();

    serial_config_t uart_config = {
        .port = UART_PORT,
        .baud_rate = UART_BAUDRATE,
        .func_p = NULL,
        .param_p = NULL
    };
    UartInit(&uart_config);

    timer_config_t analogReadAndSendTimer_config = {
        .timer = TIMER_A,
        .period = 1000000 / SAMPLE_FREQUENCY, /**< Periodo en microsegundos. */
        .func_p = analogReadAndSend,
        .param_p = NULL
    };

    timer_config_t analogWriteTimer_config = {
        .timer = TIMER_B,
        .period = 1000000 / SAMPLE_FREQUENCY, /**< Periodo en microsegundos. */
        .func_p = analogWrite,
        .param_p = NULL
    };

    /* Creación de tareas FreeRTOS */
    xTaskCreate(analogReadAndSendTask, "Analog Read and Send Task", 2048, NULL, 5, &analogReadAndSendTask_Handle);
    xTaskCreate(analogWriteTask, "Analog Write Task", 2048, NULL, 5, &analogWriteTask_handle);

    /* Inicialización y arranque de timers */
    TimerInit(&analogReadAndSendTimer_config);
    TimerStart(TIMER_A);
    TimerInit(&analogWriteTimer_config);
    TimerStart(TIMER_B);
}

/*==================[fin del archivo]=========================================*/
