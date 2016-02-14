#define F_CPU 8000000

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "serial.h"

#define NUM_BUTTON_COMBINATIONS	7

#define BUTTON_1		0b00000001
#define BUTTON_2		0b00000010
#define BUTTON_3		0b00000100
#define BUTTON_1_2		0b00000011
#define BUTTON_1_3		0b00000101
#define BUTTON_2_3		0b00000110
#define BUTTON_1_2_3	0b00000111

#define LED_1		PB1
#define LED_2		PB3
#define LED_3		PB4
#define LED_MASK 	0b11100101

#define INPUT	PB2

#define MAIN_LOOP_DELAY		100	// ms

volatile uint8_t buttons_pressed = 0;
static char *adc_result_string;

struct lut {
	uint8_t adc_value;
	uint8_t buttons_pressed;
};

static struct lut lookup_table[NUM_BUTTON_COMBINATIONS] = {
	{145, BUTTON_1_2_3},
	{155, BUTTON_1_2},
	{170, BUTTON_1_3},
	{185, BUTTON_1},
	{200, BUTTON_2_3},
	{220, BUTTON_2},
	{240, BUTTON_3},
};

void setup_IO(void) 
{

	DDRB = 0b11111101;
	PORTB = 0b11111111; // Sink current for LEDs, so high for off

}

void setup_ADC(void)
{

	// Datasheet section 17.13 p.134-136
	// Reference = VCC
	ADMUX &= ~(1 << REFS1 | 1 << REFS0);

	// Left adjust result
    ADMUX |= (1 << ADLAR);

	// Single ended input on PB2
	ADMUX &= ~(1 << MUX3 | 1 << MUX2 | 1 << MUX1);
    ADMUX |= (1 << MUX0);

	// /128 prescaler
	ADCSRA |= (1 << ADPS2 | 1 << ADPS1 | 1 << ADPS0);	

	// Free running mode
	ADCSRA |= (1 << ADATE);
	ADCSRB &= ~(1 << ADTS2 | 1 << ADTS1 | 1 << ADTS0);

	// Interrupt enable
    ADCSRA |= (1 << ADIE);

	// Chocks away
	ADCSRA |= (1 << ADEN);
	ADCSRA |= (1 << ADSC);

}

static void debug_write_value(uint8_t value) 
{

	sprintf(adc_result_string, "%d\n", value);
	serial_send_data(adc_result_string,4);

}

ISR(ADC_vect)
{

	uint8_t adc_value = ADCH;
	uint8_t i;
	
	debug_write_value(adc_value);

	buttons_pressed = 0;

	for (i = 0; i < NUM_BUTTON_COMBINATIONS; i++) {

		if (adc_value < lookup_table[i].adc_value) {
			buttons_pressed = lookup_table[i].buttons_pressed;
			break;
		}
	}

}

int main(void)
{

	adc_result_string = malloc(10);

	setup_IO();
	serial_initialise();
	setup_ADC();

	// Main loop
	while (1) {

		// Light LEDS for which buttons_pressed bit is high
		PORTB = (PORTB & LED_MASK) | (0xFF ^ buttons_pressed);
		_delay_ms(MAIN_LOOP_DELAY);

	}

	// Never reached
	return 0;

}
