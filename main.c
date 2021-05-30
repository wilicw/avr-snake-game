#define F_CPU 16000000UL
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

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

struct dot_t {
  uint8_t x;
  uint8_t y;
  struct dot_t *pre;
};

typedef struct dot_t dot_t;

void SPI_init();
void SPI_write(const uint8_t);
uint8_t SPI_read();
void max7219(const uint8_t, const uint8_t);
void display();
void move();
void generate_food();
void USART_init();
void game_over();

dot_t head, body, tail;
dot_t food;
dot_t *ptr = &tail;

uint8_t dx = 1;
uint8_t dy = 0;

uint8_t direction = DOWN;

uint8_t key;

ISR(USART_RX_vect) {
  key = UDR0;
  switch (key) {
  case RIGHT:
    if (direction != LEFT) {
      direction = key;
    }
    break;
  case LEFT:
    if (direction != RIGHT) {
      direction = key;
    }
    break;
  case UP:
    if (direction != DOWN) {
      direction = key;
    }
    break;
  case DOWN:
    if (direction != UP) {
      direction = key;
    }
    break;

  default:
    break;
  }
}

int main() {
  SPI_init();
  USART_init();
  _delay_ms(1000);
  max7219(Shutdown, 0);
  max7219(Test, 0);
  max7219(Decode, 0);
  max7219(Intensity, 0x3);
  max7219(Scan, 7);
  max7219(Shutdown, 1);

  for (uint8_t i = 0; i < 35; i++) {
    generate_food();
  }

  tail.x = 1, tail.y = 2, tail.pre = &body;
  body.x = 1, body.y = 1, body.pre = &head;
  head.x = 1, head.y = 0, head.pre = 0;

  max7219(Test, 1);
  _delay_ms(1000);
  max7219(Test, 0);
  _delay_ms(1000);

  sei();

  for (;;) {
    move();
    display();
    _delay_ms(350);
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
      self = self->pre;
    }
    max7219(i + 1, LED);
  }
}

void generate_food() {
  dot_t *self = ptr;
GEN:
  food.x = rand() % 8;
  food.y = rand() % 8;

  while (self != 0) {
    if (food.x == self->x && food.y == self->y) {
      goto GEN;
    }
    self = self->pre;
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

  direction = NONE;

  dot_t *self = ptr;
  while (self != 0) {
    if (self->pre == 0) {
      uint8_t next_x, next_y;
      next_x = self->x + dx;
      next_y = self->y + dy;

      if (next_x >= 8) {
        next_x -= 8;
      }
      if (next_y >= 8) {
        next_y -= 8;
      }

      if (next_x == food.x && next_y == food.y) {
        dot_t *node = malloc(sizeof(dot_t));
        node->x = food.x, node->y = food.y, node->pre = 0;
        self->pre = node;
        generate_food();
      }

      self->x = next_x;
      self->y = next_y;

    } else {
      self->x = self->pre->x;
      self->y = self->pre->y;
    }
    self = self->pre;
  }
}

void USART_init() {
  UBRR0H = (BAUD_PRESCALER >> 8);
  UBRR0L = BAUD_PRESCALER;
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
  UCSR0C = (0 << UMSEL00) | (0 << USBS0) | (3 << UCSZ00);
}

void game_over() {
  for (;;) {
  }
}