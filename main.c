#define F_CPU 16000000UL
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>

#define SPIPORT PORTB
#define SPIPIN PINB
#define SPIDDR DDRB

#define SS PB2
#define MOSI PB3
#define MISO PB4
#define SCK PB5

#define Decode 0x9
#define Intensity 0xA
#define Scan 0xB
#define Shutdown 0xC
#define Test 0xF

#define RIGHT 'd'
#define LEFT 'a'
#define UP 'w'
#define DOWN 's'
#define NONE 0x00

#define USART_BAUDRATE 9600
#define BAUD_PRESCALER (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#define SPEED_DEALY 200
#define COUNTER (65535 - ((F_CPU / 1024) / (1000 / SPEED_DEALY)))

struct dot_t {
  uint8_t x;
  uint8_t y;
  struct dot_t *next;
};

typedef struct dot_t dot_t;

typedef enum { true = 1, false = 0 } bool;

inline void SPI_init();
inline void SPI_write(const uint8_t);
inline uint8_t SPI_read();
inline void max7219(const uint8_t, const uint8_t);
inline void display();
inline void move();
inline void generate_food();
inline void USART_init();
inline void ADC_init();
inline uint8_t ADC_read();
inline dot_t *make_dot();
inline void game_over();
inline void timer_init();

dot_t snake;
dot_t food;
dot_t *ptr = &snake;

volatile uint8_t dx = 1;
volatile uint8_t dy = 0;
volatile uint8_t direction = DOWN;
volatile uint8_t key;
volatile bool disable_key = false;

ISR(USART_RX_vect) {
  key = UDR0;
  disable_key = false;
  switch (key) {
  case RIGHT:
    if (direction == LEFT)
      disable_key = true;
    break;
  case LEFT:
    if (direction == RIGHT)
      disable_key = true;
    break;
  case UP:
    if (direction == DOWN)
      disable_key = true;
    break;
  case DOWN:
    if (direction == UP)
      disable_key = true;
    break;
  default:
    break;
  }
  if (!disable_key)
    direction = key;
}

ISR(TIMER1_OVF_vect) {
  move();
  display();
  wdt_reset();
  TCNT1 = COUNTER;
}

int main() {
  wdt_enable(WDTO_2S);

  SPI_init();
  USART_init();

  max7219(Shutdown, 0);
  max7219(Test, 0);
  max7219(Decode, 0);
  max7219(Intensity, 0xF);
  max7219(Scan, 7);
  max7219(Shutdown, 1);

  ADC_init();
  srandom(ADC_read());

  dot_t *self = ptr;
  snake.x = 1, snake.y = 1;
  for (uint8_t i = 0; i < 3; i++) {
    dot_t *node = make_dot();
    self->next = node;
    node->x = 1, node->y = i + 2, node->next = 0;
    self = node;
  }

  generate_food();

  timer_init();

  sei();

  for (;;) {
  }

  return 0;
}

void SPI_init() {
  SPIDDR |= _BV(MOSI) | _BV(SCK) | _BV(SS);
  SPIDDR &= ~_BV(MISO);
  SPIPORT |= _BV(SS);

  SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
  SPSR &= ~(1 << SPI2X);
}

void SPI_write(const uint8_t data) {
  SPDR = data;
  while (!(SPSR & (1 << SPIF)))
    ;
}

uint8_t SPI_read() {
  SPDR = 0xFF;
  while (!(SPSR & (1 << SPIF)))
    ;
  return SPDR;
}

void max7219(const uint8_t r, const uint8_t d) {
  SPIPORT &= ~_BV(SS);
  SPI_write(r);
  SPI_write(d);
  SPIPORT |= _BV(SS);
}

void display() {
  uint8_t LED = 0x00;
  for (uint8_t i = 0; i < 8; i++) {
    LED ^= LED;

    if (food.x == i) {
      LED |= _BV(food.y);
    }

    dot_t *self = ptr;
    while (self != 0) {
      if (self->x == i) {
        LED |= _BV(self->y);
      }
      self = self->next;
    }
    max7219(i + 1, LED);
  }
}

void generate_food() {
  dot_t *self = ptr;
GEN:
  food.x = random() % 8;
  food.y = random() % 8;

  while (self != 0) {
    if (food.x == self->x && food.y == self->y) {
      goto GEN;
    }
    self = self->next;
  }
}

void move() {
  switch (direction) {
  case RIGHT:
    dx = 0;
    dy = 7;
    break;
  case LEFT:
    dx = 0;
    dy = 1;
    break;
  case UP:
    dx = 7;
    dy = 0;
    break;
  case DOWN:
    dx = 1;
    dy = 0;
    break;
  default:
    break;
  }

  dot_t *self = ptr;

  uint8_t next_x, next_y, last_x, last_y, head_x, head_y;
  next_x = self->x + dx;
  next_y = self->y + dy;

  if (next_x >= 8)
    next_x -= 8;

  if (next_y >= 8)
    next_y -= 8;

  head_x = next_x, head_y = next_y;

  if (next_x == food.x && next_y == food.y) {
    dot_t *node = make_dot();
    node->x = food.x, node->y = food.y, node->next = self;
    ptr = node;
    generate_food();
    return;
  }

  while (self != 0) {
    last_x = self->x;
    last_y = self->y;
    self->x = next_x;
    self->y = next_y;
    next_x = last_x;
    next_y = last_y;
    if (head_x == next_x && head_y == next_y) {
      display();
      game_over();
    }
    self = self->next;
  }
}

void USART_init() {
  UBRR0H = (BAUD_PRESCALER >> 8);
  UBRR0L = BAUD_PRESCALER;
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
  UCSR0C = (0 << UMSEL00) | (0 << USBS0) | (3 << UCSZ00);
}

void ADC_init() {
  ADMUX = (1 << REFS0);
  ADCSRA = (1 << ADEN) | (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);
}

uint8_t ADC_read() {
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC))
    ;
  return ADCL;
}

void game_over() {
  max7219(Shutdown, 0);
  for (;;) {
  }
}

dot_t *make_dot() { return malloc(sizeof(dot_t)); }

void timer_init() {
  TCNT1 = COUNTER;
  TCCR1A = 0x00;
  TCCR1B = (1 << CS10) | (1 << CS12);
  TIMSK1 = (1 << TOIE1);
}