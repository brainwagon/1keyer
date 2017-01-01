#include <stdint.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

// 1keyer.c

#include "1keyer.h"

// MIT License
// 
// Copyright (c) 2016, Mark VandeWettering
// 
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject
// to the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#define MAIN_DDR		DDRD
#define MAIN_PORT		PORTD
#define DIT_PIN			2		// digital pin 2
#define DAH_PIN			3		// digital pin 3
#define OUT_PIN			4		// digital pin 4
#define SIDETONE_PIN		5		// digital pin 5

#define DIT_DOWN	((MAIN_PORT&_BV(DIT_PIN)) == 0)
#define DAH_DOWN	((MAIN_PORT&_BV(DAH_PIN)) == 0)

////////////////////////////////////////////////////////////////////////

#define LED_DDR		DDRB
#define LED_PORT	PORTB
#define LED_PIN		5

void
init()
{
    MAIN_DDR  |= _BV(OUT_PIN) | _BV(SIDETONE_PIN) ;
    MAIN_PORT |= _BV(DIT_PIN) | _BV(DAH_PIN) ;
    LED_DDR   |= _BV(LED_PIN) ;
}

////////////////////////////////////////////////////////////////////////

void
key_set(uint8_t f )
{
    if (f) {
	LED_PORT |= _BV(LED_PIN) ;
	TIMSK1 |= _BV(OCIE1A) ;
	MAIN_PORT |= _BV(OUT_PIN) ;
    } else {
	LED_PORT &= ~_BV(LED_PIN) ;
	TIMSK1 &= ~_BV(OCIE1A) ;
	MAIN_PORT &= ~(_BV(OUT_PIN) | _BV(SIDETONE_PIN)) ;
    }
}


////////////////////////////////////////////////////////////////////////

// Generate a tick once a second...

#define CLOCK_PRESCALER		64
#define FREQ			(700)
#define OVERFLOW_COUNT		((F_CPU/CLOCK_PRESCALER/(2*FREQ))-1)

#define SIDETONE_DDR	DDRB
#define SIDETONE_PORT	MAIN_PORT
#define SIDETONE_PIN	5

ISR(TIMER1_COMPA_vect)
{
    SIDETONE_PORT ^= _BV(SIDETONE_PIN) ;
}

void
sidetone_init() 
{
    SIDETONE_DDR |= _BV(SIDETONE_PIN) ;

    TCCR1A = 0 ;
    TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10) ;
    OCR1A = OVERFLOW_COUNT ;
}

////////////////////////////////////////////////////////////////////////

#define WPM	(12)

// With standard spacing, the length of a dit can be computed as 
// 	1200 ms / words per minute
// Hence, for 12 wpm, you wend up with 100ms dits.

const int ditlen = 1200 / WPM ;

#define		MODE_START	(0)
#define		MODE_DIT	(1)
#define		MODE_DAH	(2)
#define 	MODE_ECHAR	(3)
#define		MODE_EWORD	(4)
#define 	MODE_ASCII	(5)

char mode = MODE_START ;
char acc ;
char accbit ;

#define DAH_BIT		(1)
#define DIT_BIT		(2)
#define OUT_BIT		(3)
#define LED_BIT		(5)

inline void
delay_1ms()
{
    __builtin_avr_delay_cycles(F_CPU / 1000UL) ;
}

void
delay_ms(int n)
{
    int i ;
    for (i=0; i<n; i++) 
	delay_1ms() ;
}

////////////////////////////////////////////////////////////////////////

void
do_element(uint8_t e)
{
    key_set(1) ;
    delay_ms(e ? 3 * ditlen : ditlen) ;
    key_set(0) ;
    delay_ms(ditlen) ;
}

////////////////////////////////////////////////////////////////////////

uint8_t
lookup_char(uint8_t acc)
{
    uint8_t i, c, p ;

    for (i=0; i<NSYMS; i++) {
	c = pgm_read_byte(&chartab[i]) ;
	p = pgm_read_byte(& pattab[i]) ;
	if (acc == p) return c ;
    }
    return 0 ;
}

uint8_t
lookup_pat(uint8_t ch)
{
    uint8_t i, c, p ;

    // uppercase only
    if (ch >= 'a' && ch <= 'z')
	ch = 'A' + ch - 'a' ;

    for (i=0; i<NSYMS; i++) {
	c = pgm_read_byte(&chartab[i]) ;
	p = pgm_read_byte(& pattab[i]) ;
	if (ch == c) return p ;
    }
    return 0 ;
}


////////////////////////////////////////////////////////////////////////

#define RX_BUFFER_SIZE	(128)
#define RX_BUFFER_MASK	(RX_BUFFER_SIZE-1)

volatile uint8_t buffer_head = 0 ;
volatile uint8_t buffer_tail = 0 ;
volatile uint8_t buffer_count = 0 ;
volatile uint8_t buffer[RX_BUFFER_SIZE] ;

ISR(USART_RX_vect)
{
    uint8_t c = UDR0 ;

    if (c >= 'a' && c <= 'z')
	c = 'A' + c - 'a' ;

    // queue up the characters we know about
    buffer[buffer_tail++] = c ;
    buffer_tail &= RX_BUFFER_MASK ;
    buffer_count ++ ;
    if (buffer_count == RX_BUFFER_SIZE)
	UCSR0B &= ~_BV(RXCIE0) ;
}

#if 0
ISR(USART_UDRE_vect)
{
    UDR0 = buffer[buffer_head++] ;
    buffer_head &= RX_BUFFER_MASK ;
    buffer_count -- ;
    if (buffer_count == 0)
	UCSR0B &= ~_BV(UDRIE0) ;
}
#endif

////////////////////////////////////////////////////////////////////////

#define BAUD	9600

#include <util/setbaud.h>

void
uart_init()
{
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;

#if USE_2X
    UCSR0A |= _BV(U2X0);
#else
    UCSR0A &= ~(_BV(U2X0));
#endif
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0) ;
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00) ;
}

uint8_t 
uart_getchar()
{
    uint8_t c ;
    cli() ;
    c = buffer[buffer_head++] ;
    buffer_head &= RX_BUFFER_MASK ;
    buffer_count -- ;
    sei() ;
    return c ;
}

void
uart_putchar(char c) 
{
    loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
    UDR0 = c;
}
////////////////////////////////////////////////////////////////////////

void
sendpattern(uint8_t pattern)
{
    while (pattern != 1) {
	do_element(pattern & 1) ;
	pattern >>= 1 ;
    }

    delay_ms(2*ditlen) ;
}

void
send(uint8_t c)
{
    uint8_t p ;
    p = lookup_pat(c) ;
    if (p) {
	sendpattern(p) ;
	uart_putchar(c) ;
    } else {
	delay_ms(7*ditlen) ;
	uart_putchar(c) ;
    }
}


////////////////////////////////////////////////////////////////////////

void
version()
{
    uart_putchar('1') ;
    uart_putchar('K') ;
    uart_putchar('\n') ;
}

////////////////////////////////////////////////////////////////////////

int 
main(void)
{
    uint8_t c ;

    init() ;
    uart_init() ;
    sidetone_init() ;

    sei() ;
    version() ;

    for (;;) {

    	switch (mode) {

	case MODE_START:				// wait for the 
							// start of a new
	    acc = 0 ;					// character
	    accbit = 1 ;	

	    if (buffer_count > 0)
		mode = MODE_ASCII ;
	    else if (DIT_DOWN) 
		mode = MODE_DIT ;
	    else if (DAH_DOWN)
		mode = MODE_DAH ;
	    break ;

	case MODE_DIT:					// send a dit

	    accbit <<= 1 ;

	    do_element(0) ;
	    if (accbit == 0x80) {
		mode = MODE_ECHAR ;
	    } else if (DAH_DOWN) {
		mode = MODE_DAH ;
	    } else if (!DIT_DOWN) {
		mode = MODE_ECHAR ;
	    }
	    break ;

	case MODE_DAH:					// send a dah

	    acc |= accbit ;
	    accbit <<= 1 ;

	    do_element(1) ;

	    if (accbit == 7) {
		mode = MODE_ECHAR ;
	    } else if (DIT_DOWN) {
		mode = MODE_DIT ;
	    } else if (!DAH_DOWN) {
		mode = MODE_ECHAR ;
	    }
	    break ;

	case MODE_ECHAR:				// end of character

	    delay_ms(2*ditlen) ;
	    uart_putchar(lookup_char(acc | accbit)) ;

	    acc = 0 ; accbit = 1 ;

	    if (DIT_DOWN)
		mode = MODE_DIT ;
	    else if (DAH_DOWN)
		mode = MODE_DAH ;
	    else
		mode = MODE_EWORD ;

	    break ;

	case MODE_EWORD:				// end of word

	    delay_ms(4*ditlen) ;
	    break ;
	
	case MODE_ASCII:

	    c = uart_getchar() ;
	    send(c) ;

	    if (buffer_count == 0)
		mode = MODE_START ;

	    break ;
	}
    }
}
