#include "linkLayer.h"

#include "hardware/pio.h"

#include "zephyr/drivers//misc/pio_rpi_pico/pio_rpi_pico.h"
#include <zephyr/drivers/pinctrl.h>

#define TX_RX_DONE_IRQ 0
#define TX_VALUE_IRQ 1

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

PINCTRL_DT_DEFINE(DT_NODELABEL(pio_link));

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

void* g_receiveUserData = NULL;
static ReceiveHandler g_receiveCallback = NULL;

void* g_transmitUserData = NULL;
static TransmitHandler g_transmitCallback = NULL;

void* g_transiveDoneUserdata = NULL;
static TransiveDoneHandler g_transiveDoneCallback = NULL;

static enum LinkMode g_mode = SLAVE;

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

RPI_PICO_PIO_DEFINE_PROGRAM(pio_master_fw, 0, 26,
                            //     .wrap_target
    0xe087, //  0: set    pindirs, 7                 
    0xe007, //  1: set    pins, 7                    
    0x0082, //  2: jmp    y--, 2                     
    0xc001, //  3: irq    nowait 1                   
    0xe03e, //  4: set    x, 30                      
    0x0245, //  5: jmp    x--, 5                 [2] 
    0x80a0, //  6: pull   block                      
    0xa047, //  7: mov    y, osr                     
    0xe02f, //  8: set    x, 15                      
    0x80a0, //  9: pull   block                      
    0xe005, // 10: set    pins, 5                    
    0xef04, // 11: set    pins, 4                [15]
    0x6e01, // 12: out    pins, 1                [14]
    0x004c, // 13: jmp    x--, 12                    
    0xef05, // 14: set    pins, 5                [15]
    0xe001, // 15: set    pins, 1                    
    0xe086, // 16: set    pindirs, 6                 
    0xe03f, // 17: set    x, 31                      
    0x0053, // 18: jmp    x--, 19                    
    0x0035, // 19: jmp    !x, 21                     
    0x00d2, // 20: jmp    pin, 18                    
    0xf62f, // 21: set    x, 15                  [22]
    0x4e01, // 22: in     pins, 1                [14]
    0x0056, // 23: jmp    x--, 22                    
    0x9020, // 24: push   block                  [16]
    0xfe05, // 25: set    pins, 5                [30]
    0xc000, // 26: irq    nowait 0                   
            //     .wrap
);

RPI_PICO_PIO_DEFINE_PROGRAM(pio_slave_fw, 0, 18,
                //     .wrap_target
    0xe001, //  0: set    pins, 1                    
    0xe084, //  1: set    pindirs, 4                 
    0xe02f, //  2: set    x, 15                      
    0x2020, //  3: wait   0 pin, 0                   
    0xd701, //  4: irq    nowait 1               [23]
    0x4e01, //  5: in     pins, 1                [14]
    0x0045, //  6: jmp    x--, 5                     
    0x8020, //  7: push   block                      
    0xe001, //  8: set    pins, 1                    
    0xe085, //  9: set    pindirs, 5                 
    0xbf42, // 10: nop                           [31]
    0xe02f, // 11: set    x, 15                      
    0x80a0, // 12: pull   block                      
    0xf000, // 13: set    pins, 0                [16]
    0x6e01, // 14: out    pins, 1                [14]
    0x004e, // 15: jmp    x--, 14                    
    0xf001, // 16: set    pins, 1                [16]
    0xc000, // 17: irq    nowait 0                   
    0xbf42, // 18: nop                           [31]
            //     .wrap
);

static PIO g_pio = NULL;
static size_t g_sm = 0;

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

uint16_t reverse_bit16(uint16_t x)
{
	x = ((x & 0x5555) << 1) | ((x & 0xAAAA) >> 1);
	x = ((x & 0x3333) << 2) | ((x & 0xCCCC) >> 2);
	x = ((x & 0x0F0F) << 4) | ((x & 0xF0F0) >> 4);
	return (x << 8) | (x >> 8);
}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

uint16_t value = 0x00;
static void pioIsr_done(const void* arg)
{
    (void)arg;
    uint16_t rxData = pio_sm_get(g_pio, g_sm);
    rxData = reverse_bit16(rxData);
    if (g_receiveCallback) g_receiveCallback(rxData, g_receiveUserData);
    if (g_transiveDoneCallback) g_transiveDoneCallback(g_transiveDoneUserdata);
    pio_interrupt_clear(g_pio, TX_RX_DONE_IRQ);
}

static void pioIsr_tx(const void* arg)
{
    (void)arg;
    struct NextTransmit txValue = 
    {
        0xDEAD, 
        50000
    };

    if (g_transmitCallback) txValue = g_transmitCallback(g_transmitUserData);
    if (g_mode == MASTER) pio_sm_put(g_pio, g_sm, txValue.timingUs);
    pio_sm_put(g_pio, g_sm, txValue.value);
    pio_interrupt_clear(g_pio, TX_VALUE_IRQ);
    return;
}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//
// Interface
//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

void link_setTransmitCallback(TransmitHandler cb, void* userData) 
{
    g_transmitUserData = userData;
    g_transmitCallback = cb;
}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

void link_setReceiveCallback(ReceiveHandler cb, void* userData) 
{
    g_receiveUserData = userData;
    g_receiveCallback = cb;
}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

void link_setTransiveDoneCallback(TransiveDoneHandler cb, void* user_data)
{
    g_transiveDoneUserdata = user_data;
    g_transiveDoneCallback = cb;
}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

enum LinkMode link_getMode()
{
    return g_mode;
}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

void link_startTransive() {}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

static void link_configureMaster()
{
    g_mode = MASTER;
    pio_sm_set_enabled(g_pio, g_sm, false);
    pio_clear_instruction_memory(g_pio);

	uint32_t offset = pio_add_program(g_pio, RPI_PICO_PIO_GET_PROGRAM(pio_master_fw));
	pio_sm_config sm_config = pio_get_default_sm_config();

    #pragma push_macro("pio0")
    #undef pio0
    uint32_t SD_pin = DT_RPI_PICO_PIO_PIN_BY_NAME(DT_CHILD(DT_NODELABEL(pio0), piolink), default, 0, link_pins, 0);
    uint32_t SC_pin = DT_RPI_PICO_PIO_PIN_BY_NAME(DT_CHILD(DT_NODELABEL(pio0), piolink), default, 0, link_pins, 1);
    uint32_t SO_pin = DT_RPI_PICO_PIO_PIN_BY_NAME(DT_CHILD(DT_NODELABEL(pio0), piolink), default, 0, link_pins, 2);
    uint32_t SI_pin = DT_RPI_PICO_PIO_PIN_BY_NAME(DT_CHILD(DT_NODELABEL(pio0), piolink), default, 0, link_pins, 3);
    #pragma pop_macro("pio0")
    
    sm_config_set_out_pins(&sm_config, SD_pin, 1);
    sm_config_set_set_pins(&sm_config, SD_pin, 4);
    sm_config_set_in_pins(&sm_config, SD_pin);
    sm_config_set_jmp_pin(&sm_config, SD_pin);

    sm_config_set_out_shift(&sm_config, true, false, 0);
    sm_config_set_in_shift(&sm_config, false, false, 0);

    pio_gpio_init(g_pio, SD_pin);
    pio_gpio_init(g_pio, SC_pin);
    pio_gpio_init(g_pio, SO_pin);
    pio_gpio_init(g_pio, SI_pin);

	sm_config_set_clkdiv(&sm_config, 67.816f); // ~540 ns per inst, 16 inst equal baud 115200

	sm_config_set_wrap(&sm_config,
			   offset + RPI_PICO_PIO_GET_WRAP_TARGET(pio_master_fw),
			   offset + RPI_PICO_PIO_GET_WRAP(pio_master_fw));
    
	pio_sm_init(g_pio, g_sm, -1, &sm_config);
	pio_sm_set_enabled(g_pio, g_sm, true);
}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

static void link_configureSlave()
{
    g_mode = SLAVE;
    pio_sm_set_enabled(g_pio, g_sm, false);
    pio_clear_instruction_memory(g_pio);

	uint32_t offset = pio_add_program(g_pio, RPI_PICO_PIO_GET_PROGRAM(pio_slave_fw));
	pio_sm_config sm_config = pio_get_default_sm_config();

    #pragma push_macro("pio0")
    #undef pio0
    uint32_t SD_pin = DT_RPI_PICO_PIO_PIN_BY_NAME(DT_CHILD(DT_NODELABEL(pio0), piolink), default, 0, link_pins, 0);
    uint32_t SC_pin = DT_RPI_PICO_PIO_PIN_BY_NAME(DT_CHILD(DT_NODELABEL(pio0), piolink), default, 0, link_pins, 1);
    uint32_t SO_pin = DT_RPI_PICO_PIO_PIN_BY_NAME(DT_CHILD(DT_NODELABEL(pio0), piolink), default, 0, link_pins, 2);
    uint32_t SI_pin = DT_RPI_PICO_PIO_PIN_BY_NAME(DT_CHILD(DT_NODELABEL(pio0), piolink), default, 0, link_pins, 3);
    #pragma pop_macro("pio0")

    sm_config_set_out_pins(&sm_config, SD_pin, 1);
    sm_config_set_set_pins(&sm_config, SD_pin, 4);
    sm_config_set_in_pins(&sm_config, SD_pin);

    sm_config_set_out_shift(&sm_config, true, false, 0);
    sm_config_set_in_shift(&sm_config, false, false, 0);

    pio_gpio_init(g_pio, SD_pin);
    pio_gpio_init(g_pio, SC_pin);
    pio_gpio_init(g_pio, SO_pin);
    pio_gpio_init(g_pio, SI_pin);

	sm_config_set_clkdiv(&sm_config, 67.816f); // ~540 ns per inst, 16 inst equal baud 115200

	sm_config_set_wrap(&sm_config,
			   offset + RPI_PICO_PIO_GET_WRAP_TARGET(pio_slave_fw),
			   offset + RPI_PICO_PIO_GET_WRAP(pio_slave_fw));
    
	pio_sm_init(g_pio, g_sm, -1, &sm_config);
	pio_sm_set_enabled(g_pio, g_sm, true);
}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

void link_changeMode(enum LinkMode mode)
{
    switch (mode)
    {
        case SLAVE:
            link_configureSlave();
            break;
        case MASTER:
            link_configureMaster();
            break;
    }   
}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

static int link_init()
{
    const struct device* dev = DEVICE_DT_GET(DT_PROP(DT_NODELABEL(pio_link), pio));

    const struct pinctrl_dev_config* config = PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(pio_link));
    g_pio = pio_rpi_pico_get_pio(dev);

    pio_rpi_pico_allocate_sm(dev, &g_sm);

    IRQ_CONNECT(PIO0_IRQ_0 , 0, pioIsr_done, NULL, 0);
    IRQ_CONNECT(PIO0_IRQ_1 , 0, pioIsr_tx, NULL, 0);

    irq_enable(PIO0_IRQ_0);
    irq_enable(PIO0_IRQ_1);

    pio_set_irq0_source_enabled(g_pio, pis_interrupt0, true);
    pio_set_irq1_source_enabled(g_pio, pis_interrupt1, true);

    int ret = pinctrl_apply_state(config, PINCTRL_STATE_DEFAULT);

    link_configureSlave();

    return ret;
}

SYS_INIT(link_init, APPLICATION, 1);
