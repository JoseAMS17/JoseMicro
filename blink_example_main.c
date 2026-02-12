
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

/* ===================== DEFINES ===================== */

#define MOTOR_ON        1
#define MOTOR_OFF       0
#define buzzerOFF       0
#define buzzerON        1
#define lampON          1
#define lampOff         0
#define LEDON           1
#define LEDOFF          0
#define Presionado      1
#define SinPresionar    0

#define TIEMPO_11S      11
#define timer           1000   // divisor para reloj

#define ESTADO_INIT         0
#define ESTADO_CERRADO      1
#define ESTADO_ABIERTO      2
#define ESTADO_ABRIENDO     3
#define ESTADO_CERRANDO     4
#define ESTADO_DESCO        5
#define ESTADO_ERROR        6
#define ESTADO_STOP         7

/* ===================== GPIO INPUTS ===================== */

#define PIN_BA     GPIO_NUM_13
#define PIN_BC     GPIO_NUM_12
#define PIN_BP     GPIO_NUM_14
#define PIN_BE     GPIO_NUM_27
#define PIN_BR     GPIO_NUM_26
#define PIN_FCA    GPIO_NUM_33
#define PIN_FCC    GPIO_NUM_32
#define PIN_MA     GPIO_NUM_25   // motor abrir
#define PIN_MC     GPIO_NUM_4    // motor cerrar
#define PIN_BUZZER GPIO_NUM_16
#define PIN_LAMP   GPIO_NUM_17
#define PIN_LEDR   GPIO_NUM_5


/* ===================== PROTOTYPES ===================== */

int func_ESTADO_INT(void);
int func_ESTADO_ABIERTO(void);
int func_ESTADO_CERRADO(void);
int func_ESTADO_ABRIENDO(void);
int func_ESTADO_CERRANDO(void);
int func_ESTADO_DESCO(void);
int func_ESTADO_ERROR(void);
int func_ESTADO_STOP(void);

/* ===================== GLOBAL VARIABLES ===================== */

int ESTADO_ACTUAL    = ESTADO_INIT;
int ESTADO_ANTERIOR  = ESTADO_INIT;
int ESTADO_SIGUIENTE = ESTADO_INIT;

int timer_11s    = 0;
int timer_activo = 0;

/* ===================== IO STRUCT ===================== */

struct IO
{
    /* inputs */
    unsigned int ba  : 1;
    unsigned int bc  : 1;
    unsigned int bp  : 1;
    unsigned int be  : 1;
    unsigned int br  : 1;
    unsigned int fca : 1;
    unsigned int fcc : 1;

    /* outputs */
    unsigned int ma     : 1;
    unsigned int mc     : 1;
    unsigned int buzzer : 1;
    unsigned int lamp   : 1;
    unsigned int ledR   : 1;

} io;

/* ===================== GPIO ===================== */

void gpio_init_inputs(void)
{
    gpio_config_t io_conf = {0};

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    io_conf.pin_bit_mask =
        (1ULL << PIN_BA)  |
        (1ULL << PIN_BC)  |
        (1ULL << PIN_BP)  |
        (1ULL << PIN_BE)  |
        (1ULL << PIN_BR)  |
        (1ULL << PIN_FCA) |
        (1ULL << PIN_FCC);

    gpio_config(&io_conf);
}

void gpio_init_outputs(void)
{
    gpio_config_t io_conf = {0};

    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    io_conf.pin_bit_mask =
        (1ULL << PIN_MA)     |
        (1ULL << PIN_MC)     |
        (1ULL << PIN_BUZZER) |
        (1ULL << PIN_LAMP)   |
        (1ULL << PIN_LEDR);

    gpio_config(&io_conf);
}


void leer_entradas(void)
{
    io.ba  = (gpio_get_level(PIN_BA)  == 0) ? Presionado : SinPresionar;
    io.bc  = (gpio_get_level(PIN_BC)  == 0) ? Presionado : SinPresionar;
    io.bp  = (gpio_get_level(PIN_BP)  == 0) ? Presionado : SinPresionar;
    io.be  = (gpio_get_level(PIN_BE)  == 0) ? Presionado : SinPresionar;
    io.br  = (gpio_get_level(PIN_BR)  == 0) ? Presionado : SinPresionar;
    io.fca = (gpio_get_level(PIN_FCA) == 0) ? Presionado : SinPresionar;
    io.fcc = (gpio_get_level(PIN_FCC) == 0) ? Presionado : SinPresionar;
}

void escribir_salidas(void)
{
    gpio_set_level(PIN_MA,     io.ma);
    gpio_set_level(PIN_MC,     io.mc);
    gpio_set_level(PIN_BUZZER, io.buzzer);
    gpio_set_level(PIN_LAMP,   io.lamp);
    gpio_set_level(PIN_LEDR,   io.ledR);
}

/* ===================== TIMER ===================== */

void timer_task(void *pv)
{
    while (1)
    {
        if (timer_activo && timer_11s > 0)
        {
            timer_11s--;
        }

        vTaskDelay(pdMS_TO_TICKS(timer));
    }
}

void iniciar_timer_11s(void)
{
    timer_11s = TIEMPO_11S;
    timer_activo = 1;
}

void detener_timer(void)
{
    timer_activo = 0;
}

/* ===================== FSM STATES ===================== */

int func_ESTADO_INT(void)
{
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL   = ESTADO_SIGUIENTE;

    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.buzzer = buzzerOFF;
    io.lamp = lampOff;
    io.ledR = LEDOFF;

    for (;;)
    {
        if (io.fca == Presionado && io.fcc == SinPresionar)
        {
            printf("ESTADO ABIERTO\n");
            return ESTADO_ABIERTO;
        }

        if (io.fca == SinPresionar && io.fcc == Presionado)
        {
            printf("ESTADO CERRADO\n");
            return ESTADO_CERRADO;
        }
        else if (io.fca == SinPresionar && io.fcc == SinPresionar)
        {
            printf("ESTADO DESCONOCIDO\n");
            return ESTADO_DESCO;
        }
        else
        {
            printf("ESTADO ERROR\n");
            return ESTADO_ERROR;
        }
    }
}

int func_ESTADO_ABIERTO(void)
{
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL   = ESTADO_SIGUIENTE;

    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.buzzer = buzzerON;
    io.lamp = lampON;
    io.ledR = LEDOFF;

    for (;;)
    {
        if (timer_activo == 0)
        {
            iniciar_timer_11s();
        }

        if (io.br == Presionado)
        {
            printf("ESTADO DE REINICIO\n");
            printf("ESTADO CERRANDO\n");
            return ESTADO_CERRANDO;
        }

        if (timer_activo && timer_11s == 0)
        {
            detener_timer();
            printf("ESTADO CERRANDO\n");
            return ESTADO_CERRANDO;
        }

        if (io.fca == Presionado && io.fcc == SinPresionar && io.bc == Presionado)
        {
            detener_timer();
            printf("ESTADO CERRANDO\n");
            return ESTADO_CERRANDO;
        }
        else if (io.fca == Presionado && io.fcc == SinPresionar && io.ba == Presionado)
        {
            detener_timer();
            printf("ESTADO CERRANDO\n");
            return ESTADO_CERRANDO;
        }
        else if (io.fca == Presionado && io.fcc == Presionado)
        {
            printf("ESTADO ERROR\n");
            return ESTADO_ERROR;
        }
    }
}

int func_ESTADO_CERRADO(void)
{
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL   = ESTADO_SIGUIENTE;

    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.buzzer = buzzerOFF;
    io.lamp = lampOff;
    io.ledR = LEDOFF;

    for (;;)
    {
        if (io.fcc == Presionado && io.fca == SinPresionar && io.ma == Presionado)
        {
            printf("ESTADO ABRIENDO\n");
            return ESTADO_ABRIENDO;
        }

        if (io.br == Presionado)
        {
            printf("ESTADO DE REINICIO\n");
            printf("ESTADO CERRANDO\n");
            return ESTADO_CERRANDO;
        }
        else if (io.fca == Presionado && io.ma == Presionado)
        {
            printf("ESTADO ERROR\n");
            return ESTADO_ERROR;
        }
        else
        {
            printf("ESTADO CERRADO\n");
            return ESTADO_CERRADO;
        }
    }
}

int func_ESTADO_ABRIENDO(void)
{
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL   = ESTADO_SIGUIENTE;

    io.ma = MOTOR_ON;
    io.mc = MOTOR_OFF;
    io.buzzer = buzzerOFF;
    io.lamp = lampOff;
    io.ledR = LEDOFF;

    for (;;)
    {
        if (io.fca == Presionado && io.fcc == SinPresionar)
        {
            printf("ESTADO ABIERTO\n");
            return ESTADO_ABIERTO;
        }

        if (io.br == Presionado)
        {
            printf("ESTADO DE REINICIO\n");
            printf("ESTADO CERRANDO\n");
            return ESTADO_CERRANDO;
        }
        else if (io.fca == SinPresionar &&
                 io.fcc == SinPresionar &&
                 (io.ba || io.bc || io.be || io.bp))
        {
            printf("ESTADO STOP\n");
            return ESTADO_STOP;
        }
        else if (io.fca == Presionado && io.fcc == Presionado)
        {
            printf("ESTADO ERROR\n");
            return ESTADO_ERROR;
        }
        else if (io.fca == SinPresionar && io.fcc == Presionado)
        {
            printf("ESTADO ERROR\n");
            return ESTADO_ERROR;
        }
        else
        {
            printf("ESTADO ABRIENDO\n");
            return ESTADO_ABRIENDO;
        }
    }
}

int func_ESTADO_CERRANDO(void)
{
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL   = ESTADO_SIGUIENTE;

    io.ma = MOTOR_OFF;
    io.mc = MOTOR_ON;
    io.buzzer = buzzerON;
    io.lamp = lampOff;
    io.ledR = LEDOFF;

    for (;;)
    {
        if (io.fca == SinPresionar && io.fcc == Presionado)
        {
            printf("ESTADO CERRADO\n");
            return ESTADO_CERRADO;
        }
        else if ((io.fca == SinPresionar && io.fcc == SinPresionar) ||
                 (io.ba || io.bc || io.be || io.bp))
        {
            printf("ESTADO STOP\n");
            return ESTADO_STOP;
        }
        else if (io.fca == SinPresionar && io.fcc == Presionado)
        {
            printf("ESTADO ERROR\n");
            return ESTADO_ERROR;
        }
    }
}

int func_ESTADO_ERROR(void)
{
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.buzzer = buzzerOFF;
    io.lamp = lampOff;
    io.ledR = LEDON;

    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL   = ESTADO_SIGUIENTE;

    for (;;)
    {
        if (io.br == Presionado)
        {
            printf("ESTADO DE REINICIO\n");
            printf("ESTADO CERRANDO\n");
            return ESTADO_CERRANDO;
        }
    }
}

int func_ESTADO_DESCO(void)
{
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL   = ESTADO_SIGUIENTE;

    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.buzzer = buzzerOFF;
    io.lamp = lampOff;
    io.ledR = LEDON;

    for (;;)
    {
        printf("ESTADO CERRANDO\n");
        return ESTADO_CERRANDO;
    }
}

int func_ESTADO_STOP(void)
{
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL   = ESTADO_SIGUIENTE;

    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.buzzer = buzzerOFF;
    io.lamp = lampOff;
    io.ledR = LEDOFF;

    for (;;)
    {
        if (io.ba == Presionado &&
            io.bc == SinPresionar &&
            io.fca == SinPresionar &&
            io.fcc == SinPresionar)
        {
            printf("ESTADO ABRIENDO\n");
            return ESTADO_ABRIENDO;
        }

        if (io.br == Presionado)
        {
            printf("ESTADO DE REINICIO\n");
            printf("ESTADO CERRANDO\n");
            return ESTADO_CERRANDO;
        }
        else if (io.ba == SinPresionar &&
                 io.bc == Presionado &&
                 io.fca == SinPresionar &&
                 io.fcc == SinPresionar)
        {
            printf("ESTADO CERRANDO\n");
            return ESTADO_CERRANDO;
        }
    }
}

/* ===================== MAIN ===================== */

void app_main(void)
{
    gpio_init_inputs();
    gpio_init_outputs();
    xTaskCreate(timer_task, "timer_1s", 2048, NULL, 5, NULL);

    while (true)
    {
        leer_entradas();

        if (ESTADO_ACTUAL == ESTADO_INIT)
            ESTADO_SIGUIENTE = func_ESTADO_INT();
        else if (ESTADO_ACTUAL == ESTADO_CERRADO)
            ESTADO_SIGUIENTE = func_ESTADO_CERRADO();
        else if (ESTADO_ACTUAL == ESTADO_ABIERTO)
            ESTADO_SIGUIENTE = func_ESTADO_ABIERTO();
        else if (ESTADO_ACTUAL == ESTADO_ABRIENDO)
            ESTADO_SIGUIENTE = func_ESTADO_ABRIENDO();
        else if (ESTADO_ACTUAL == ESTADO_CERRANDO)
            ESTADO_SIGUIENTE = func_ESTADO_CERRANDO();
        else if (ESTADO_ACTUAL == ESTADO_DESCO)
            ESTADO_SIGUIENTE = func_ESTADO_DESCO();
        else if (ESTADO_ACTUAL == ESTADO_STOP)
            ESTADO_SIGUIENTE = func_ESTADO_STOP();
        else
            ESTADO_SIGUIENTE = func_ESTADO_ERROR();

        ESTADO_ANTERIOR = ESTADO_ACTUAL;
        ESTADO_ACTUAL   = ESTADO_SIGUIENTE;

        escribir_salidas();   

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
