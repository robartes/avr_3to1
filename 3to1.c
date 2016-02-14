#define F_CPU 8000000

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "serial.h"

#define NUM_BUTTON_COMBINATIONS	7

#define LED_1		PB1
#define LED_2		PB3
#define LED_3		PB4
#define LED_MASK 	0b11100101

// Button to LED definitions, assuming LEDs on pins above
// This are the values the PORTB is OR'ed with later
// All non LED pins 0
// Lit LEDs 0
// Unlit LEDs 1
#define BUTTON_1		0b00011000
#define BUTTON_2		0b00010010
#define BUTTON_3		0b00001010
#define BUTTON_1_2		0b00010000
#define BUTTON_1_3		0b00001000
#define BUTTON_2_3		0b00000010
#define BUTTON_1_2_3	0b00000000
#define NO_BUTTONS		0b00011010


#define INPUT	PB2

#define MAIN_LOOP_DELAY		100	// ms

volatile uint8_t buttons_pressed = 0;
volatile uint8_t adc_output_counter = 0;
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

	DDRB = 0b00011010;
	PORTB = 0b11111011; // Sink current for LEDs, so high for off
						// No pullup for PB2

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
	
	if (adc_output_counter++ == 0)	
		debug_write_value(adc_value);

	buttons_pressed = NO_BUTTONS;

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
	serial_send_data("Canary",6);
	setup_ADC();


	// Main loop
	while (1) {

		// Light LEDS for which buttons_pressed bit is high. There's some
		// bit twiddling involved
		PORTB = (PORTB & LED_MASK) | buttons_pressed;
		_delay_ms(MAIN_LOOP_DELAY);

	}

	// Never reached
	return 0;

}
