#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/spinlock.h>

#include  <zephyr/drivers/dma.h>

#include "stm32f0xx.h"
#include "stm32f0xx_ll_tim.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_dma.h"

#include "link.h"

#define ARR_VALUE 416
#define GPIO_PIN_DEBUG 1
#define GPIO_PIN_LEADER_START 4
#define GPIO_PIN_FOLLOWR_START 8
#define GPIO_PIN_DATA_INPUT_OUTPUT 11
#define BSSR_SET (1 << GPIO_PIN_DATA_INPUT_OUTPUT)
#define BSSR_RESET (1 << (GPIO_PIN_DATA_INPUT_OUTPUT + 16))
#define DMA_CHANNEL_RX 3
#define DMA_CHANNEL_TX 5

static struct gpio_callback g_link_start_rx_uart_callback;
static struct gpio_callback g_link_start_tx_uart_callback;

void (*g_handler)(void);
static TransiveHandler g_transiveCallback = NULL;

static TransiveDoneHandler g_transiveDoneCallback = NULL;

const struct device* gpioIO = DEVICE_DT_GET(DT_NODELABEL(gpioa));
const struct device* dma = DEVICE_DT_GET(DT_NODELABEL(dma1));

static uint32_t tx_data[18] = {};
static uint32_t rx_data[17] = {};

static uint16_t g_link_received = 0;
static uint16_t g_link_transmit = 0;
static const uint32_t mask = 1 << GPIO_PIN_DATA_INPUT_OUTPUT;

static uint16_t dummy_transmit(uint16_t)
{
    return 0x00;
}

static void dma_complete(const struct device *dev, void *user_data, uint32_t channel,
    int status)
{
    if (g_handler != NULL) g_handler();
}

static inline void link_transmitToDma()
{
    tx_data[1] = (g_link_transmit & (1 << 0))  ? BSSR_SET : BSSR_RESET;
    tx_data[2] = (g_link_transmit & (1 << 1))  ? BSSR_SET : BSSR_RESET;
    tx_data[3] = (g_link_transmit & (1 << 2))  ? BSSR_SET : BSSR_RESET;
    tx_data[4] = (g_link_transmit & (1 << 3))  ? BSSR_SET : BSSR_RESET;
    tx_data[5] = (g_link_transmit & (1 << 4))  ? BSSR_SET : BSSR_RESET;
    tx_data[6] = (g_link_transmit & (1 << 5))  ? BSSR_SET : BSSR_RESET;
    tx_data[7] = (g_link_transmit & (1 << 6))  ? BSSR_SET : BSSR_RESET;
    tx_data[8] = (g_link_transmit & (1 << 7))  ? BSSR_SET : BSSR_RESET;
    tx_data[9] = (g_link_transmit & (1 << 8))  ? BSSR_SET : BSSR_RESET;
    tx_data[10] = (g_link_transmit & (1 << 9))  ? BSSR_SET : BSSR_RESET;
    tx_data[11] = (g_link_transmit & (1 << 10)) ? BSSR_SET : BSSR_RESET;
    tx_data[12] = (g_link_transmit & (1 << 11)) ? BSSR_SET : BSSR_RESET;
    tx_data[13] = (g_link_transmit & (1 << 12)) ? BSSR_SET : BSSR_RESET;
    tx_data[14] = (g_link_transmit & (1 << 13)) ? BSSR_SET : BSSR_RESET;
    tx_data[15] = (g_link_transmit & (1 << 14)) ? BSSR_SET : BSSR_RESET;
    tx_data[16] = (g_link_transmit & (1 << 15)) ? BSSR_SET : BSSR_RESET;
}

static inline void link_dmaToReceived()
{
    if (rx_data[1] & mask) g_link_received |= (1 << 0);
    if (rx_data[2] & mask) g_link_received |= (1 << 1);
    if (rx_data[3] & mask) g_link_received |= (1 << 2);
    if (rx_data[4] & mask) g_link_received |= (1 << 3);

    if (rx_data[5] & mask) g_link_received |= (1 << 4);
    if (rx_data[6] & mask) g_link_received |= (1 << 5);
    if (rx_data[7] & mask) g_link_received |= (1 << 6);
    if (rx_data[8] & mask) g_link_received |= (1 << 7);

    if (rx_data[9] & mask) g_link_received |= (1 << 8);
    if (rx_data[10] & mask) g_link_received |= (1 << 9);
    if (rx_data[11] & mask) g_link_received |= (1 << 10);
    if (rx_data[12] & mask) g_link_received |= (1 << 11);

    if (rx_data[13] & mask) g_link_received |= (1 << 12);
    if (rx_data[14] & mask) g_link_received |= (1 << 13);
    if (rx_data[15] & mask) g_link_received |= (1 << 14);
    if (rx_data[16] & mask) g_link_received |= (1 << 15);
}

static void dma_complete_dummy(const struct device *dev, void *user_data, uint32_t channel,
    int status)
{
    int lock = irq_lock();

    //gpio_pin_toggle(gpioIO, GPIO_PIN_DEBUG);
    (*(volatile uint32_t *)(0x48000000 | 0x18)) = (1 << (GPIO_PIN_DEBUG + 16));

    link_dmaToReceived();
    g_link_transmit = g_transiveCallback(g_link_received);
    link_transmitToDma();

    //gpio_pin_configure(gpioIO, GPIO_PIN_DATA_INPUT_OUTPUT, GPIO_OUTPUT_HIGH);
    (*(volatile uint32_t *)0x48000000) &= ~(3U << (GPIO_PIN_DATA_INPUT_OUTPUT * 2));
    (*(volatile uint32_t *)0x48000000) |= (1U << (GPIO_PIN_DATA_INPUT_OUTPUT * 2));
    (*(volatile uint32_t *)(0x48000000 | 0x18)) = BSSR_SET;

    dma_start(dma, DMA_CHANNEL_TX);
    LL_TIM_EnableCounter(TIM15);

    //gpio_pin_toggle(gpioIO, GPIO_PIN_DEBUG);
    (*(volatile uint32_t *)(0x48000000 | 0x18)) = (1 << GPIO_PIN_DEBUG);

    irq_unlock(lock);
}

static struct dma_block_config tx_config = {
    .dest_address = (uint32_t)(0x48000000 | 0x18), // GPIOA->BSRR
    .source_address = (uint32_t)tx_data,
    .block_size = sizeof(tx_data),
    .source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
    .dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE
};

static struct dma_config dma_cfg_tx = {
    .channel_direction = MEMORY_TO_PERIPHERAL,
    .complete_callback_en = 0,
    .error_callback_dis = 1,
    .source_data_size = sizeof(uint32_t),
    .dest_data_size = sizeof(uint32_t),
    .source_burst_length = 1,
    .dest_burst_length = 1,
    .block_count = 1,
    .channel_priority = 0x03,
    .dma_callback = &dma_complete,
    .head_block = &tx_config
};

static struct dma_block_config rx_config = {
    .dest_address = (uint32_t)rx_data,
    .source_address = (uint32_t) (0x48000000 | 0x10), // GPIOA->IDR
    .block_size = sizeof(rx_data),
    .source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
    .dest_addr_adj = DMA_ADDR_ADJ_INCREMENT
};

static struct dma_config dma_cfg_rx = {
    .channel_direction = PERIPHERAL_TO_MEMORY,
    .complete_callback_en = 0,
    .error_callback_dis = 1,
    .source_data_size = sizeof(uint32_t),
    .dest_data_size = sizeof(uint32_t),
    .source_burst_length = 1,
    .dest_burst_length = 1,
    .block_count = 1,
    .channel_priority = 0x00,
    .dma_callback = dma_complete_dummy,
    .head_block = &rx_config
};

void link_setTransiveCallback(TransiveHandler cb, void* user_data) {
    g_transiveCallback = cb;
}

void link_setTransiveDoneCallback(TransiveDoneHandler cb, void* user_data)
{
    g_transiveDoneCallback = cb;
}

static void link_transiveDone()
{   
    g_link_received = 0;
    
    LL_TIM_DisableCounter(TIM15);
    LL_TIM_SetCounter(TIM15, 0);

    LL_TIM_DisableCounter(TIM3);
    LL_TIM_SetCounter(TIM3, 0);

    if (g_transiveDoneCallback != NULL) g_transiveDoneCallback();

    // Prepare state for next recive
    gpio_pin_configure(gpioIO, GPIO_PIN_DATA_INPUT_OUTPUT, GPIO_INPUT | GPIO_PULL_UP);
    dma_config(dma, DMA_CHANNEL_TX, &dma_cfg_tx);
    dma_config(dma, DMA_CHANNEL_RX, &dma_cfg_rx);
    dma_start(dma, DMA_CHANNEL_RX);
}

static void link_startReceiveUart(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
    LL_TIM_EnableCounter(TIM3);
}

static void link_set_up_timer()
{
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM15);
    LL_TIM_SetClockSource(TIM15, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetCounterMode(TIM15, LL_TIM_COUNTERDIRECTION_UP);
    LL_TIM_SetUpdateSource(TIM15, LL_TIM_UPDATESOURCE_COUNTER);
    LL_TIM_SetAutoReload(TIM15, ARR_VALUE);
    LL_TIM_SetCounter(TIM15, 0);
    LL_TIM_SetPrescaler(TIM15, 0);
    LL_TIM_EnableDMAReq_UPDATE(TIM15);
    LL_TIM_EnableIT_UPDATE(TIM15);
    LL_TIM_GenerateEvent_UPDATE(TIM15);
    LL_TIM_EnableAllOutputs(TIM15);
    LL_TIM_SetTriggerOutput(TIM15, LL_TIM_TRGO_UPDATE);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);
    LL_TIM_SetClockSource(TIM3, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetCounterMode(TIM3, LL_TIM_COUNTERDIRECTION_UP);
    LL_TIM_SetUpdateSource(TIM3, LL_TIM_UPDATESOURCE_COUNTER);
    LL_TIM_SetAutoReload(TIM3, ARR_VALUE);
    LL_TIM_SetCounter(TIM3, 0);
    LL_TIM_SetPrescaler(TIM3, 0);
    LL_TIM_EnableDMAReq_UPDATE(TIM3);
    LL_TIM_EnableIT_UPDATE(TIM3);
    LL_TIM_GenerateEvent_UPDATE(TIM3);
    LL_TIM_EnableAllOutputs(TIM3);
    LL_TIM_SetTriggerOutput(TIM3, LL_TIM_TRGO_UPDATE);
}

static int link_init()
{
    tx_data[0] = BSSR_RESET;
    tx_data[17] = BSSR_SET;

    g_handler = link_transiveDone;
    g_transiveCallback = &dummy_transmit;

    gpio_pin_configure(gpioIO, GPIO_PIN_LEADER_START, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_configure(gpioIO, GPIO_PIN_FOLLOWR_START, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_configure(gpioIO, GPIO_PIN_DATA_INPUT_OUTPUT, GPIO_INPUT | GPIO_PULL_UP);

    gpio_pin_configure(gpioIO, GPIO_PIN_DEBUG, GPIO_OUTPUT_HIGH);
    link_set_up_timer();

    gpio_init_callback(&g_link_start_rx_uart_callback, link_startReceiveUart, (1 << GPIO_PIN_LEADER_START));
    gpio_add_callback(gpioIO, &g_link_start_rx_uart_callback);
    gpio_pin_interrupt_configure(gpioIO, GPIO_PIN_LEADER_START, GPIO_INT_EDGE_FALLING);

    dma_config(dma, DMA_CHANNEL_TX, &dma_cfg_tx);
    dma_config(dma, DMA_CHANNEL_RX, &dma_cfg_rx);
    dma_start(dma, DMA_CHANNEL_RX);
    //dma_start(dma, DMA_CHANNEL_TX);
    return 0;
}

SYS_INIT(link_init, APPLICATION, 1);