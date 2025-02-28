#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "matriz_led.pio.h"

#define QTD_PIXELS 25      // número de leds na matriz
#define PIN_LED 7          // pino de saída do led
#define LED_VERMELHO 13    // pino de saída do led vermelho
#define LED_AZUL 12        // pino de saída do led azul
#define LED_VERDE 11       // pino de saída do led verde
#define BOTAO_A 5          // Pino do botão A
#define BOTAO_B 6          // Pino do botão B
#define BUZZER_PIN 21      // Configuração do pino do buzzer
#define DEBOUNCE_DELAY 300 // Tempo de debounce em milissegundos

volatile int c = 0; // Variável global para armazenar o número exibido (0 a 9)
volatile uint32_t last_interrupt_time = 0;

PIO pio;
uint sm;
float r = 1.0, g = 1.0, b = 1.0;
bool green_led_on = false;
bool blue_led_on = false;
bool buzzer_on = false;
void gpio_callback(uint gpio, uint32_t events)
{
  static uint32_t last_gpio = 0;

  uint32_t current_time = to_ms_since_boot(get_absolute_time());

  // Verifica se é o mesmo botão pressionado e se está dentro do tempo de debounce
  if (gpio == last_gpio && (current_time - last_interrupt_time < DEBOUNCE_DELAY))
    return;

  last_interrupt_time = current_time;
  last_gpio = gpio;

  if (gpio == BOTAO_A)
  {
    green_led_on = !green_led_on;
    gpio_put(LED_VERDE, green_led_on);
  }
  else if (gpio == BOTAO_B)
  {
    blue_led_on = !blue_led_on;
    gpio_put(LED_AZUL, blue_led_on);
    buzzer_on = !buzzer_on;
    gpio_put(BUZZER_PIN, buzzer_on);
  }
}

void led_vermelho()
{
  gpio_put(LED_VERMELHO, 1);
  sleep_ms(500);
  gpio_put(LED_VERMELHO, 0);
  sleep_ms(500);
}

void setup_gpio()
{
  gpio_init(LED_VERMELHO);
  gpio_init(LED_VERDE);
  gpio_init(LED_AZUL);
  gpio_init(BUZZER_PIN);
  gpio_init(BOTAO_A);
  gpio_init(BOTAO_B);

  gpio_set_dir(LED_VERMELHO, GPIO_OUT);
  gpio_set_dir(LED_VERDE, GPIO_OUT);
  gpio_set_dir(LED_AZUL, GPIO_OUT);
  gpio_set_dir(BUZZER_PIN, GPIO_OUT);
  gpio_set_dir(BOTAO_A, GPIO_IN);
  gpio_set_dir(BOTAO_B, GPIO_IN);

  gpio_pull_up(BOTAO_A);
  gpio_pull_up(BOTAO_B);

  gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, gpio_callback);
  gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, gpio_callback);
}
uint matrix_rgb(float r, float g, float b)
{
  unsigned char R, G, B;
  R = r * 10;
  G = g * 10;
  B = b * 10;
  return (G << 24) | (R << 16) | (B << 8);
}

void desenho_pio(double *desenho)
{
  for (int16_t i = 0; i < QTD_PIXELS; i++)
  {
    uint32_t valor_led = matrix_rgb(desenho[i] * r, desenho[i] * g, desenho[i] * b);
    pio_sm_put_blocking(pio, sm, valor_led);
  }
}

double circulo[9][25] = {
    // Frame 1: Borda externa
    {1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 0.0, 0.0, 0.0, 1.0,
     1.0, 0.0, 0.0, 0.0, 1.0,
     1.0, 0.0, 0.0, 0.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0},

    // Frame 2: Borda interna
    {1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 0.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0},

    // Frame 3: Preenchimento parcial
    {1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0},

    // Frame 4: Centro expandido
    {1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0},

    // Frame 5: Centro quase completo
    {1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0},

    // Frame 6: Centro completo
    {1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0},

    // Frame 7: Redução externa
    {0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 1.0, 1.0, 1.0, 0.0,
     0.0, 1.0, 1.0, 1.0, 0.0,
     0.0, 1.0, 1.0, 1.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0},

    // Frame 8: Redução interna
    {0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 1.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0},

    // Frame 9: Centro final
    {0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0}};

int main()
{
  pio = pio0;
  bool frequenciaClock;

  frequenciaClock = set_sys_clock_khz(128000, false);
  stdio_init_all();
  setup_gpio();

  uint offset = pio_add_program(pio, &pio_matrix_program);
  sm = pio_claim_unused_sm(pio, true);
  pio_matrix_program_init(pio, sm, offset, PIN_LED);

  while (true)
  {
    led_vermelho();
    desenho_pio(circulo[c]);
    if (c < 8)
    {
      c++;
    }
    else
    {
      c = 0;
    }
    sleep_ms(50);
  }
}
