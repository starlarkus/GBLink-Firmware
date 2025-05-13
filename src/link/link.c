#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/spinlock.h>

#include "stm32f0xx.h"      
#include "stm32f0xx_ll_tim.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_bus.h"

#include "link.h"

#define ARR_VALUE 554
#define GPIO_PIN_LEADER_START 4
#define GPIO_PIN_FOLLOWR_START 5
#define GPIO_PIN_DATA_INPUT_OUTPUT 10

static struct gpio_callback g_link_start_rx_uart_callback;
static struct gpio_callback g_link_start_tx_uart_callback;

void (*g_handler)(void);
static TransiveHandler g_transiveCallback = NULL;

static TransiveDoneHandler g_transiveDoneCallback = NULL;

const struct device* gpioIO = DEVICE_DT_GET(DT_NODELABEL(gpioa));

static uint16_t g_link_received = 0;
static uint16_t g_link_transmit = 0;
static uint8_t g_transmitBitIndex = 0;
static uint8_t g_receiveBitIndex = 0;

bool g_ignoreRxInterrupt = false;

void link_setTransiveCallback(TransiveHandler cb) {
    g_transiveCallback = cb;
}

void link_setTransiveDoneCallback(TransiveDoneHandler cb)
{
    g_transiveDoneCallback = cb;
}

ISR_DIRECT_DECLARE(timer_irq)
{   
    int key = irq_lock();
    LL_TIM_ClearFlag_UPDATE(TIM1);
    g_handler();
    irq_unlock(key);
    return 1;
}

static void link_poll_in_16_uart()
{   
    gpio_pin_toggle(gpioIO, 3);
    WRITE_BIT(g_link_received, g_receiveBitIndex, gpio_pin_get(gpioIO, GPIO_PIN_DATA_INPUT_OUTPUT));
    gpio_pin_toggle(gpioIO, 3);    
    
    g_receiveBitIndex++;
    if (g_receiveBitIndex >= 16)
    {
        g_receiveBitIndex = 0;
        LL_TIM_DisableCounter(TIM1);
        LL_TIM_SetCounter(TIM1, 0);
    }
    
}

static void link_transmitComplete_cb()
{   
    gpio_pin_toggle(gpioIO, 3);
    if (g_transmitBitIndex <= 1)
    {
        //Start Bit
        gpio_pin_set(gpioIO, GPIO_PIN_DATA_INPUT_OUTPUT, 0);
    }
    else if (g_transmitBitIndex == 18)
    {
        //Stop Bit
        gpio_pin_set(gpioIO, GPIO_PIN_DATA_INPUT_OUTPUT, 1);
        LL_TIM_DisableCounter(TIM1);
        LL_TIM_SetCounter(TIM1, 0);
        g_transmitBitIndex = 0;

        gpio_pin_configure(gpioIO, GPIO_PIN_DATA_INPUT_OUTPUT, GPIO_INPUT);
        gpio_pin_interrupt_configure(gpioIO, GPIO_PIN_DATA_INPUT_OUTPUT, GPIO_INT_EDGE_FALLING);
        g_handler = link_poll_in_16_uart;
        
        LL_TIM_SetAutoReload(TIM1, ARR_VALUE);
        g_ignoreRxInterrupt = false;
        if (g_transiveDoneCallback != NULL) g_transiveDoneCallback();
    }
    else
    {
        gpio_pin_set(gpioIO, GPIO_PIN_DATA_INPUT_OUTPUT, (g_link_transmit & 1));
        g_link_transmit >>= 1;
    }
    gpio_pin_toggle(gpioIO, 3);
    g_transmitBitIndex++;
}

static void link_startReceiveUart(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
    
    if (g_ignoreRxInterrupt) return;
    
    g_ignoreRxInterrupt = true;
    g_link_received = 0;
    gpio_pin_interrupt_configure(gpioIO, GPIO_PIN_DATA_INPUT_OUTPUT, GPIO_INT_DISABLE);
    LL_TIM_EnableCounter(TIM1);
    link_poll_in_16_uart();
}

static void link_startTransmitUart(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
    if (g_transiveCallback != NULL)
    {
        g_link_transmit = g_transiveCallback(g_link_received);
    }
    gpio_pin_set(gpioIO, 3, GPIO_ACTIVE_HIGH);
    gpio_pin_configure(gpioIO, GPIO_PIN_DATA_INPUT_OUTPUT, GPIO_OUTPUT);
    g_handler = link_transmitComplete_cb;
    LL_TIM_DisableCounter(TIM1);
    LL_TIM_SetCounter(TIM1, 0);
    LL_TIM_SetAutoReload(TIM1, ARR_VALUE);
    LL_TIM_GenerateEvent_UPDATE(TIM1);
    LL_TIM_EnableCounter(TIM1);
    LL_TIM_ClearFlag_UPDATE(TIM1);
    LL_TIM_EnableCounter(TIM1);
}

static void link_set_up_timer()
{
    //LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);
    //LL_RCC_SetTIMClockSource(LL_RCC_TIM1_CLKSOURCE_PLL);
    LL_TIM_SetCounterMode(TIM1, LL_TIM_COUNTERDIRECTION_UP);
    LL_TIM_SetCounter(TIM1, 0);
    LL_TIM_SetPrescaler(TIM1, 0);
    LL_TIM_EnableUpdateEvent(TIM1);
    LL_TIM_SetUpdateSource(TIM1, LL_TIM_UPDATESOURCE_COUNTER);
    LL_TIM_EnableIT_UPDATE(TIM1);
    // IRQ_DIRECT_CONNECT(TIM4_IRQn, 0, timer_irq, 0);
    // NVIC_EnableIRQ(TIM4_IRQn);
}


static int link_init()
{
    link_set_up_timer();
    gpio_pin_configure(gpioIO, GPIO_PIN_LEADER_START, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_configure(gpioIO, GPIO_PIN_FOLLOWR_START, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_configure(gpioIO, GPIO_PIN_DATA_INPUT_OUTPUT, GPIO_INPUT);
    gpio_pin_configure(gpioIO, 3, GPIO_OUTPUT_HIGH);

    gpio_init_callback(&g_link_start_rx_uart_callback, link_startReceiveUart, (1 << GPIO_PIN_DATA_INPUT_OUTPUT));
    gpio_add_callback(gpioIO, &g_link_start_rx_uart_callback);
    gpio_pin_interrupt_configure(gpioIO, GPIO_PIN_DATA_INPUT_OUTPUT, GPIO_INT_EDGE_FALLING);

    gpio_init_callback(&g_link_start_tx_uart_callback, link_startTransmitUart, (1 << GPIO_PIN_FOLLOWR_START));
    gpio_add_callback(gpioIO, &g_link_start_tx_uart_callback);
    gpio_pin_interrupt_configure(gpioIO, GPIO_PIN_FOLLOWR_START, GPIO_INT_EDGE_FALLING);

    return 0;
}

SYS_INIT(link_init, APPLICATION, 1);