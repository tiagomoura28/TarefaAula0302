/*
 * Por: Tiago Moura
 *    Comunicação serial com I2C
 *  
 * Uso da interface I2C para comunicação com o Display OLED e Matriz de Leds
 * 
 *  
 * Este programa escreve uma mensagem no display OLED.
 * 
 * 
*/


#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "hardware/pio.h"
#include "DisplayC.pio.h"

#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"


#include "hardware/clocks.h"
#include "DisplayC.pio.h"

#include "hardware/uart.h"

#include "pico/bootrom.h"


const uint button_0 = 5;

//rotina da interrupção
static void gpio_irq_handler(uint gpio, uint32_t events){
    printf("Interrupção ocorreu no pino %d, no evento %d\n", gpio, events);
    printf("HABILITANDO O MODO GRAVAÇÃO");
	reset_usb_boot(0,0); //habilita o modo de gravação do microcontrolador
}




//dentro da main

    
//configuração necessária para matriz de leds
#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7

// Variável global para armazenar a cor (Entre 0 e 255 para intensidade)
uint8_t led_r = 0; // Intensidade do vermelho
uint8_t led_g = 0; // Intensidade do verde
uint8_t led_b = 20; // Intensidade do azul

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
//parte da uart led
#define led_pin_g 11
#define led_pin_b 12
#define led_pin_r 13
//
static volatile uint a = 1;
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)

// Prototipação da função de interrupção
static void gpio_irq_handler(uint gpio, uint32_t events);

// Representação de números de 0 a 9 para a matriz 5x5
bool numeros[10][NUM_PIXELS] = {
    
  {1, 1, 1, 1, 1,
   1, 0, 0, 0, 1,
   1, 0, 0, 0, 1,
   1, 0, 0, 0, 1, 
   1, 1, 1, 1, 1}, // 0

  {0, 1, 0, 0, 0,
   0, 0, 0, 1, 0,
   0, 1, 0, 0, 0, 
   0, 0, 0, 1, 0,
   0, 1, 0, 0, 0}, // 1

  {1, 1, 1, 1, 1, 
   1, 0, 0, 0, 0,
   1, 1, 1, 1, 1,
   0, 0, 0, 0, 1,
   1, 1, 1, 1, 1}, // 2

  {1, 1, 1, 1, 1, 
   0, 0, 0, 0, 1,
   1, 1, 1, 1, 1,
   0, 0, 0, 0, 1,
   1, 1, 1, 1, 1}, // 3

  {0, 1, 0, 0, 0,
   0, 0, 0, 1, 0,
   0, 1, 1, 1, 1, 
   1, 0, 0, 1, 0,
   0, 1, 0, 0, 1}, // 4

  {1, 1, 1, 1, 1, 
   0, 0, 0, 0, 1,
   1, 1, 1, 1, 1,
   1, 0, 0, 0, 0,
   1, 1, 1, 1, 1}, // 5

  {1, 1, 1, 1, 1, 
   1, 0, 0, 0, 1,
   1, 1, 1, 1, 1,
   1, 0, 0, 0, 0,
   1, 1, 1, 1, 1}, // 6

  {0, 1, 0, 0, 0,
   0, 0, 0, 1, 0,
   0, 1, 0, 0, 0, 
   0, 0, 0, 1, 0,
   0, 1, 1, 1, 1}, // 7

  {1, 1, 1, 1, 1, 
   1, 0, 0, 0, 1,
   1, 1, 1, 1, 1,
   1, 0, 0, 0, 1,
   1, 1, 1, 1, 1}, // 8
   
  {1, 1, 1, 1, 1, 
   0, 0, 0, 0, 1,
   1, 1, 1, 1, 1,
   1, 0, 0, 0, 1,
   1, 1, 1, 1, 1}  // 9
};

// Variável global para armazenar o número atual exibido
//int numero_atual = NUMERO_INICIAL;

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}


void set_number_on_leds(int num)
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        if (numeros[num][i])
        {
            put_pixel(urgb_u32(led_r, led_g, led_b));  // Acende o LED com a cor atual
        }
        else
        {
            put_pixel(0);  // Apaga o LED
        }
    }
}



int main()
{//parte do codigo para bootsel com botao
//inicializar o botão de interrupção - GPIO5
gpio_init(button_0);
gpio_set_dir(button_0, GPIO_IN);
gpio_pull_up(button_0);

//interrupção da gpio habilitada
gpio_set_irq_enabled_with_callback(button_0, GPIO_IRQ_EDGE_FALL, 1, & gpio_irq_handler);

//parte da uart led
stdio_init_all(); // Inicializa comunicação USB CDC para monitor serial

    // Configura os pinos dos LEDs como saída
    gpio_init(led_pin_r);
    gpio_set_dir(led_pin_r, GPIO_OUT);
    gpio_put(led_pin_r, 0); // Inicialmente desligado

    gpio_init(led_pin_g);
    gpio_set_dir(led_pin_g, GPIO_OUT);
    gpio_put(led_pin_g, 0); // Inicialmente desligado

    gpio_init(led_pin_b);
    gpio_set_dir(led_pin_b, GPIO_OUT);
    gpio_put(led_pin_b, 0); // Inicialmente desligado

    printf("RP2040 inicializado. Envie 'r', 'g' ou 'b' para alternar os LEDs.\n");
  //
  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA); // Pull up the data line
  gpio_pull_up(I2C_SCL); // Pull up the clock line
  ssd1306_t ssd; // Inicializa a estrutura do display
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_send_data(&ssd); // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  bool cor = true;
  //Parte ws2812
  PIO pio = pio0;
  int sm = 0;
  uint offset = pio_add_program(pio, &ws2812_program);
  ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

  while (true)
  {
    cor = !cor;
    // Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
//parte da uart led
if (stdio_usb_connected())
{ // Certifica-se de que o USB está conectado
    char c;
    if (scanf("%c", &c) == 1)
    { // Lê caractere da entrada padrão
        printf("Recebido: '%c'\n", c);
        if ((c >= 'A' && c <= 'Z')||(c >= 'a' && c <= 'z')){

           
        
        //switch (c)
        //{
            //Caso o caractere recebido seja 'r' será lido o estado do led vermelho 
            // o o seu valor será invertido. Logo, se o led estiver aceso ele será apagado
            // e se estiver apagado ele será aceso.
                           // case 'r':
                            ssd1306_draw_char(&ssd, c, 8, 10);
                         //   ssd1306_draw_string(&ssd, "r", 8, 10); // Desenha uma string
           // gpio_put(led_pin_r, !gpio_get(led_pin_r));
           // printf("LED vermelho alternado!\n");
           // break;
       // case 'g':
       // ssd1306_draw_char(&ssd, c, 8, 10);
        //ssd1306_draw_string(&ssd, "g", 8, 10); // Desenha uma string
      
        //alternar led verde
         //   gpio_put(led_pin_g, !gpio_get(led_pin_g));
         //   printf("LED verde alternado!\n");
          //  break;
       // case 'b':
       //   gpio_put(led_pin_b, !gpio_get(led_pin_b));
          //  printf("LED azul alternado!\n");
          //  break;

       // ssd1306_draw_char(&ssd, c, 8, 10);
       // ssd1306_draw_string(&ssd, "b", 8, 10); // Desenha uma string
         
      //  default:
            //printf("Comando inválido: '%c'\n", c);
        }else if (c >= '0' && c <= '9'){
          int num = c - '0';
          ssd1306_draw_char(&ssd, c, 8, 10);
          set_number_on_leds(num);



        }else{
          printf("Comando inválido: '%c'\n", c);
          ssd1306_draw_string(&ssd, "Comando invalido", 8, 10); // Desenha uma string
        }
    }
}
//sleep_ms(40);
  



//


    //ssd1306_draw_string(&ssd, "CEPEDI   Tic37", 8, 10); // Desenha uma string
   // ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 30); // Desenha uma string
   // ssd1306_draw_string(&ssd, "Tiago Moura", 15, 48); // Desenha uma string      
    ssd1306_send_data(&ssd); // Atualiza o display

    sleep_ms(1000);
  }
}