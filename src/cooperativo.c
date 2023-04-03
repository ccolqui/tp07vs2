/* Carmela Colqui <carmela.colqui@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/** @file expropiativo.c
 **
 ** @brief Ejemplo de un cambio de contexto expropiativo
 **
 ** Ejemplo de la implementación básica de la ejecución de dos tareas
 ** con un planitifador tipo round robin utilizando un cambio de contexto
 ** expropiativo basado en el temporizador del sistema para asignar las cuotas
 ** de tiempo de cada proceso.
 **
 ** @defgroup ejemplos Proyectos de ejemplo
 ** @brief Proyectos de ejemplo de la Especialización en Sistemas Embebidos
 ** @{
 */

/* === Inclusiones de cabeceras ============================================ */

#include <stdint.h>
#include <string.h>

#include "bsp.h"

/* === Definicion y Macros ================================================= */

/** Cantidad de bytes para la pila de cada tarea */
#define STACK_SIZE 256

/** Cantidad de tareas */
#define TASK_COUNT 3

/** Valor de la cuenta para la función de espera */
#define COUNT_DELAY 3000000

/* === Declaraciones de tipos de datos internos ============================ */

typedef uint8_t stack_t[STACK_SIZE];

typedef struct context_s
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t ip;
    uint32_t lr;
} *context_t;

/* === Declaraciones de funciones internas ================================= */

/** @brief Función para generar demoras
 **
 ** Función basica que genera una demora para permitir el parpadeo de los leds
 */
void Delay(void);

/** @brief Funcion que implementa el cambio de contexto
 **
 ** Cada vez que se llama a la función la misma almacena el contexto de la
 ** tarea que la llama, selecciona a la otra tarea como activa y recupera
 ** el contexto de la misma.
 */
void CambioContexto(void);

/** @brief Funcion para configurar el contexto inicial de una tarea
 **
 ** Esta función asigna la pila de una tarea y prepara el contexto
 ** inicial de la misma para que al atender una interrupción se pueda
 ** realizar el cambio de tareas cambiando el puntero de la pila
 */
void CrearTarea(int id, void *entry_point);

/** @brief Función que indica un error en el cambio de contexto
 **
 ** @remark Esta funcion no debería ejecutarse nunca, solo se accede a la
 **         misma si las funciones que implementan las tareas terminan
 */
void Error(void);

/** @brief Función que implementa la primera tarea del sistema */
void TareaA(void);

/** @brief Función que implementa la segunda tarea del sistema */
void TareaB(void);

/** @brief Función que implementa la tercer tarea del sistema EITI - TP N° 7 */
void TareaC(void);

/* === Definiciones de variables internas ================================== */

/** Espacio para la pila de las tareas */
static stack_t stack[TASK_COUNT];

/** Punteros a contexto de cada tarea y del sistema operativo */
static uint32_t context[TASK_COUNT + 1];

/** Puntero para acceder a los recursos de la placa */
board_t board;

/* === Definiciones de variables externas ================================== */

/* === Definiciones de funciones internas ================================== */

void Delay(void)
{
    uint32_t i;

    for (i = COUNT_DELAY; i != 0; i--)
    {
        CambioContexto();
    }
}

__attribute__((naked(), optimize("O0"))) void CambioContexto(void)
{
    static int divisor = 0;
    static int activa = TASK_COUNT;

    __asm__("push {r0-r12, lr}");
    __asm__("str r13, %0"
            : "=m"(context[activa]));
    __asm__("ldr r13, %0"
            :
            : "m"(context[TASK_COUNT]));

    activa = (activa + 1) % TASK_COUNT;
    divisor = (divisor + 1) % 100000;
    if (divisor == 0)
        DigitalOutputToggle(board->led_verde);

    __asm__("str r13, %0"
            : "=m"(context[TASK_COUNT]));
    __asm__("ldr r13, %0"
            :
            : "m"(context[activa]));
    __asm__("pop {r0-r12, lr}");
    __asm__("bx lr");
}

void CrearTarea(int id, void *entry_point)
{
    void *stack_pointer = stack[id] + STACK_SIZE;
    struct context_s *context_pointer = stack_pointer - sizeof(struct context_s);

    memset(context_pointer, 0, sizeof(struct context_s));
    context_pointer->r7 = (uint32_t)(stack_pointer);
    context_pointer->lr = (uint32_t)entry_point;
    context[id] = (uint32_t)(context_pointer);
}

void Error(void)
{
    DigitalOutputActivate(board->led_rojo);
    while (1)
    {
    }
}

void TareaA(void)
{
    while (1)
    {
        if (DigitalInputGetState(board->boton_prueba))
        {
            DigitalOutputActivate(board->led_azul);
        }
        else
        {
            DigitalOutputDeactivate(board->led_azul);
        }
        CambioContexto();
    }
}

void TareaB(void)
{
    while (1)
    {
        DigitalOutputToggle(board->led_amarillo);
        Delay();
    }
}

void TareaC(void)
{
    while (1)
    {
        if (DigitalInputHasChanged(board->boton_cambiar))
        {
            DigitalOutputActivate(board->led_verde);
        }
        else
        {
            DigitalOutputDeactivate(board->led_verde);
        }
        CambioContexto(); // cede volunatariamente el procesador
    }
}
/* === Definiciones de funciones externas ================================== */
int main(void)
{
    /* Configuración de los dispositivos de entrada/salida */
    board = BoardCreate();

    /* Creación de las tareas del sistema */
    CrearTarea(0, TareaA);
    CrearTarea(1, TareaB);
    CrearTarea(2, TareaC);

    /* Arranque del sistema cooperativo */
    CambioContexto();

    return 0;
}

/* === Ciere de documentacion ============================================== */

/** @} Final de la definición del modulo para doxygen */