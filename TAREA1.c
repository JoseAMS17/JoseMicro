#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define boton GPIO_NUM_0
#define led   GPIO_NUM_2

void holamundo(void)
{
   
    gpio_set_direction(boton, GPIO_MODE_INPUT);
    gpio_pullup_en(boton);


    gpio_set_direction(led, GPIO_MODE_OUTPUT);

    while (1)
    {
        if (gpio_get_level(boton) == 0) {
            printf("Hello world!\n");
            gpio_set_level(led, 1);
        } else {
            gpio_set_level(led, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); 

    }
}
