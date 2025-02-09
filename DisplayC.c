/*
 * Por: Tiago Moura adaptação do código https://github.com/wiltonlacerda/EmbarcaTechU4C6/tree/main/I2C_DisplayEmC
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

//prototipo rotina da interrupção

//configuração necessária para matriz de leds
#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define BUTTON_A_PIN 5  // Botão A no GPIO 5
#define BUTTON_0_PIN 22
#define BUTTON_B_PIN 6  // Botão B no GPIO 6

int numero=10;//variavel global usada na matriz de LED

// Variável global para armazenar a cor (Entre 0 e 255 para intensidade)
uint8_t led_r = 0; // Intensidade do vermelho
uint8_t led_g = 0; // Intensidade do verde
uint8_t led_b = 20; // Intensidade do azul
//vARIAVÉIS NECESSÁRIAS PARA i2c
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
//PINOS  DOS LEDS RGB
#define led_pin_g 11
#define led_pin_b 12
#define led_pin_r 13
//necessário para o debounce dentro da interrupção
static volatile uint a = 1;
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)


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



static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

//função para colocar numero na LED 5x5
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

bool cor = false;//necessário para colocar cor preta na tela do display
ssd1306_t ssd; // Inicializa a estrutura do display
static void gpio_irq_rhandler(uint gpio, uint32_t events);


int main()
{
  
  //parte do codigo para bootsel com botao
//inicializar o botão de interrupção - pino 22
gpio_init(BUTTON_0_PIN);
gpio_set_dir(BUTTON_0_PIN, GPIO_IN);
gpio_pull_up(BUTTON_0_PIN);



gpio_init(BUTTON_A_PIN);  // Inicializa o pino do botão A
gpio_set_dir(BUTTON_A_PIN, GPIO_IN);  // Define o botão A como entrada
gpio_pull_up(BUTTON_A_PIN);  // Habilita o resistor de pull-up no botão A

    gpio_init(BUTTON_B_PIN);  // Inicializa o pino do botão B
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);  // Define o botão B como entrada
    gpio_pull_up(BUTTON_B_PIN);  // Habilita o resistor de pull-up no botão B

    //interrupção da joystick habilitada
    gpio_set_irq_enabled_with_callback(BUTTON_0_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_rhandler);
    //interrupção da botoes habilitada
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_rhandler);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_rhandler);


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

    
  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA); // Pull up the data line
  gpio_pull_up(I2C_SCL); // Pull up the clock line
  
 
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_send_data(&ssd); // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  
  //Coonfigurção matriz de LEDs ws2812
  PIO pio = pio0;
  int sm = 0;
  uint offset = pio_add_program(pio, &ws2812_program);
  ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

  while (true)
  {
    
    ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo no display
//referente a comunicação serial com o display
if (stdio_usb_connected())
{ // Certifica-se de que o USB está conectado
    char c;
    if (scanf("%c", &c) == 1)
    { // Lê caractere da entrada padrão
        printf("Recebido: '%c'\n", c);
        if ((c >= 'A' && c <= 'Z')||(c >= 'a' && c <= 'z')){

          ssd1306_fill(&ssd, cor); // Limpa o display
          ssd1306_draw_char(&ssd, c, 8, 10);
          
        }else if (c >= '0' && c <= '9'){
          numero = c - '0';
          ssd1306_fill(&ssd, cor); // Limpa o display
          ssd1306_draw_char(&ssd, c, 8, 10);
          set_number_on_leds(numero);
          }else{
          printf("Comando inválido: '%c'\n", c);
          ssd1306_draw_string(&ssd, "Comando invalido", 8, 10); // Desenha uma string
        }
    }
}
//sleep_ms(40);
 
    ssd1306_send_data(&ssd); // Atualiza o display

    sleep_ms(1000);
  }
}

// Função de interrupção com debouncing
static void gpio_irq_rhandler(uint gpio, uint32_t events)
{
    // Obtém o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time());
   // printf("A = %d\n", a);
    // Verifica se passou tempo suficiente desde o último evento
    if (current_time - last_time > 200000) // 200 ms de debouncing
    {
        last_time = current_time; // Atualiza o tempo do último evento
       if (!gpio_get(BUTTON_A_PIN)){  // Botão A pressionado (ativo em nível baixo){

      bool estado_atual = gpio_get(led_pin_g); // Obtém o estado atual
      gpio_put(led_pin_g, !estado_atual);   
            
            ssd1306_fill(&ssd, cor); // Limpa o display
            if((estado_atual)==true){
            ssd1306_draw_string(&ssd, "VERDE OFF", 20, 30);
            ssd1306_send_data(&ssd); // Atualiza o display
                printf("Desligando O LED VERDE\n");}
            else {       
               ssd1306_draw_string(&ssd, "VERDE ON", 20, 30);
              ssd1306_send_data(&ssd); // Atualiza o display
              printf("Ligando O LED VERDE\n");}
            
        }
        else if (!gpio_get(BUTTON_B_PIN)){            

        // Verificar o estado do botão B para decrementar o número
      
        bool estado_atual = gpio_get(led_pin_b); // Obtém o estado atual
        gpio_put(led_pin_b, !estado_atual);      // Alterna o estado
        ssd1306_fill(&ssd, cor); // Limpa o display
        if((estado_atual)==true){
        ssd1306_draw_string(&ssd, "AZUL OFF", 20, 30);
        ssd1306_send_data(&ssd); // Atualiza o display
            printf("Desligando O LED AZUL\n");}
        else {       
           ssd1306_draw_string(&ssd, "AZUL ON", 20, 30);
          ssd1306_send_data(&ssd); // Atualiza o display
          printf("Ligando O LED AZUL\n");}


        }else{
          ssd1306_fill(&ssd, cor); // Limpa o display
          printf("HABILITANDO O MODO GRAVAÇÃO\n");
          printf("Interrupção ocorreu no pino %d, no evento %d\n", gpio, events);
          ssd1306_draw_string(&ssd, "BOOTSEL", 15, 48); // Desenha uma string
          ssd1306_send_data(&ssd); // Atualiza o display
          reset_usb_boot(0,0);
        }
            
        }
    }
  
    
   
