/*
 * ms-MEGA_v1.0.c
 *
 * Autor: Valentín Pérez Cerutti
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "midi.h"

Midi_msg mensajeMIDI;


// variables internas
uint8_t nota_destino = 69;		// nota destino
uint16_t nota_gliss = 69 << 8;	// nota con glissando
uint8_t ADC_nuevo = 0;			// flag del ADC
uint8_t ADC_conteo = 0;			// periodos del ADC
uint16_t ADC_promedio = 0;		// cálculo del promedio del ADC
uint8_t ADC_variable = 0;		// multiplexer del ADC


// variables configurables
uint8_t glissando = 0;			// ADC0
uint8_t detune = 127;			// ADC1
uint8_t pw1 = 127;				// ADC2
uint8_t scale0 = 3, scale1 = 3; // ADC3 y ADC4
// scale = 0 (-3), 1 (-2), 2 (-1), 3 (+0), 4 (+1) ó 5 (+2)
// sync en PIND 2
uint8_t bend = 64;
uint8_t velocidad = 0;

// variables de debugging 
// trigger_osc en PIND 3

// señales de salida
uint8_t gate = 0;				// PORTB 5


//					B    C    C#   D    D#   E    F    F#   G    G#   A    A#   b
uint8_t freqs[] = {252, 238, 224, 212, 200, 189, 178, 168, 158, 149, 141, 133, 126};


// Rutina que se ejecutará cuando se reciba un mensaje MIDI completo
void NuevoMensajeMIDI(){
	switch((mensajeMIDI.byte0 & 0xF0)){
		case 0x90: // Note On
		nota_destino = mensajeMIDI.byte1;
		velocidad = mensajeMIDI.byte2;
		return;
		
		case 0x80: // Note Off
		if(mensajeMIDI.byte1 != nota_destino) return;
		nota_destino = 0;
		return;
		
		//case 0xB0: // Mod Wheel
		//glissando = mensajeMIDI.byte2;
		//return;

		case 0xE0: // Pitch bend
		bend = mensajeMIDI.byte2;
		return;
	}
}


	// Interrupciones

// Interrupción del ADC
ISR(ADC_vect){
	ADC_nuevo = 0xFF;
}

// Interrupción para modo Sync
ISR(TIMER0_COMPA_vect){
	TCNT1 = 0; // Timer1 reset
}




int main(void){
	
		// Entradas y salidas
	DDRB = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5);
	DDRD = (1 << 6);
	
	PORTD = (1 << 2) | (1 << 3); // Input pull-up
	
		// Potenciómetros
	// Referencia interna y ajuste a izquierda
	ADMUX = (1 << REFS0) | (1 << ADLAR);
	// Habilitar ADC, iniciar conversión y prescaler /127
	ADCSRA = (1 << ADEN) | (1 << ADSC) | (7 << ADPS0) | (1 << ADIE);
	// Deshabilita buffer digital en ADC0-ADC4
	DIDR0 = (63 << 0);
		
		
		// OSC-0
	// Modo CTC y toggle OC0A on compare match
	TCCR0A = (1 << WGM01) | (1 << COM0A0);
	// Clock externo en pin T0, falling edge
	TCCR0B = (7 << CS00);
		
		// Selector de octava pasa OSC-0
	// Modo CTC y toggle OCR2A on compare match
	TCCR2A = (2 << WGM20) | (1 << COM2A0);
	// Clock interno sin prescaler para octavas superiores
	TCCR2B = (1 << CS20);
		
		// OSC-1
	// Modo fast PWM con ICR1 y salida OCR1B inverting
	TCCR1A = (1 << WGM10) | (3 << COM1B0);
	// Prescaler /8
	TCCR1B = (2 << WGM12) | (2 << CS10);
	
		
		
	// Inicialización de la comunicación MIDI
	MIDI_init(&mensajeMIDI, NuevoMensajeMIDI);
		
	
	while(1){		
	
			// Sección Sync
		if(PIND & (1 << 2))
			TIMSK0 = 0;
		else
			TIMSK0 = (1 << OCIE0A);
		// interrupt por overflow en el timer0
	
	
	
			// Sección ADC
					
		if(ADC_nuevo != 0x00){ // conversión completa
			
			if(ADC_conteo < 64)
				ADC_promedio += ADCH; // promedio 64 muestras
			
			
			if(ADC_conteo == 112){
				
				// variables locales al cálculo de frecuencias
				uint8_t freq0 = 0xff, oct0 = 0xff;
				uint16_t freq1 = 0xffff, pwm1 = 0xffff;
				uint16_t nota_bend;
				
				// ajusto ADC_promedio a 8 bits
				ADC_promedio = ADC_promedio >> 6;
				// y lo asigno a la variable que corresponda
				switch(ADC_variable){
					case 0:
						glissando = ADC_promedio;
						ADC_variable = 1;
						break;
					case 1:
						detune = ADC_promedio;
						ADC_variable = 2;
						break;
					case 2:
						pw1 = ADC_promedio;
						ADC_variable = 3;
						break;
					case 3:
						if(ADC_promedio < 42)		scale0 = 0;
						else if(ADC_promedio < 84)  scale0 = 1;
						else if(ADC_promedio < 127) scale0 = 2;
						else if(ADC_promedio < 170) scale0 = 3;
						else if(ADC_promedio < 212) scale0 = 4;
						else						scale0 = 5;
						ADC_variable = 4;
						break;
					case 4:
						if(ADC_promedio < 42)		scale1 = 0;
						else if(ADC_promedio < 84)  scale1 = 1;
						else if(ADC_promedio < 127) scale1 = 2;
						else if(ADC_promedio < 170) scale1 = 3;
						else if(ADC_promedio < 212) scale1 = 4;
						else						scale1 = 5;
						ADC_variable = 0;
						break;
				}
				
				
				
					// Sección GATE
				
				if(nota_destino != 0){ // Si hay alguna nota apretada
					
					// chequeo el gate
					if(gate == 0){ // trigger ON
						gate = 0xff;
						PORTB |= (1 << 5);
						
						// Prender osciladores
						TCCR0B = (7 << CS00);
						TCCR1B = (2 << WGM12) | (2 << CS10);
						
						// Setear nota de inicio
						nota_gliss = nota_destino << 8;
					}
						
				} else { // cuando nota_destino == 0 (no nota apretada)
					
					// chequeo si hay que apagar el gate
					if(gate == 0xff){ // trigger OFF
						gate = 0;
						PORTB &= ~(1 << 5);
						
						// Apagar osciladores si el usuario lo requiere (PIND3 a tierra)
						if((PIND & (1 << 3)) == 0) {
							TCCR0B = 0x00;
							TCCR1B = 0x00;
						}
					}
				}

					
					// Sección FRECUENCIA y MODIFICADORES
					
				// Cálculo del glissando
				if(nota_gliss < (nota_destino << 8))
					nota_gliss += ((uint32_t)((nota_destino << 8) - nota_gliss)*((glissando>>5)+1)) >> 4;
				else
					nota_gliss -= ((uint32_t)(nota_gliss - (nota_destino << 8))*((glissando>>5)+1)) >> 4;
				
				// Modificación por Bend Wheel  (7 bits)
				nota_bend = nota_gliss - (1 << 9) + (bend << 3);
				
				// Límites superior e inferior de notas
				if(nota_bend > (106 << 8)) nota_bend = (106 << 8);
				else if(nota_bend < (11 << 8)) nota_bend = (11 << 8);
								
				// Cálculo de frecuencias
				if(nota_bend < (23<<8)){
					// frecuencia base
					freq1 = freqs[(nota_bend >> 8) - 11];
					// frecuencia demodulada por glissando para osc1
					freq1 = (freq1 << 7) + (((freqs[(nota_bend>>8) - 10] - (uint8_t)freq1) * ((uint8_t)nota_bend)) >> 1);
					// frecuencia ya demodulada por glissando para osc0
					freq0 = freq1 >> 7;
					// selector de octava para OSC-0
					oct0 = 255;
				} else if(nota_bend < (35<<8)){
					freq1 = freqs[(nota_bend >> 8) - 23];
					freq1 = (freq1 << 6) + (((freqs[(nota_bend>>8) - 22] - (uint8_t)freq1) * ((uint8_t)nota_bend)) >> 2);
					freq0 = freq1 >> 6;
					oct0 = 127;
				} else if(nota_bend < (47<<8)){
					freq1 = freqs[(nota_bend >> 8) - 35];
					freq1 = (freq1 << 5) + (((freqs[(nota_bend>>8) - 34] - (uint8_t)freq1) * ((uint8_t)nota_bend)) >> 3);
					freq0 = freq1 >> 5;
					oct0 = 63;
				} else if(nota_bend < (59<<8)){
					freq1 = freqs[(nota_bend >> 8) - 47];
					freq1 = (freq1 << 4) + (((freqs[(nota_bend>>8) - 46] - (uint8_t)freq1) * ((uint8_t)nota_bend)) >> 4);
					freq0 = freq1 >> 4;
					oct0 = 31;
				} else if (nota_bend < (71<<8)){
					freq1 = freqs[(nota_bend >> 8) - 59];
					freq1 = (freq1 << 3) + (((freqs[(nota_bend>>8) - 58] - (uint8_t)freq1) * ((uint8_t)nota_bend)) >> 5);
					freq0 = freq1 >> 3;
					oct0 = 15;
				} else if (nota_bend < (83<<8)){
					freq1 = freqs[(nota_bend >> 8) - 71];
					freq1 = (freq1 << 2) + (((freqs[(nota_bend>>8) - 70] - (uint8_t)freq1) * ((uint8_t)nota_bend)) >> 6);
					freq0 = freq1 >> 2;
					oct0 = 7;
				} else if (nota_bend < (95<<8)){
					freq1 = freqs[(nota_bend >> 8) - 83];
					freq1 = (freq1 << 1) + (((freqs[(nota_bend>>8) - 82] - (uint8_t)freq1) * ((uint8_t)nota_bend)) >> 7);
					freq0 = freq1 >> 1;
					oct0 = 3;
				} else {
					freq1 = freqs[(nota_bend >> 8) - 95];
					freq1 = freq1 + (((freqs[(nota_bend>>8) - 94] - (uint8_t)freq1) * ((uint8_t)nota_bend)) >> 8);
					freq0 = freq1;
					oct0 = 1;
				}
					
	
				// Modulación del OSC-1 por Detune
				if(detune > 128)
					freq1 -= ((detune - 128) * (uint32_t)freq1) >> 10;
				else
					freq1 += ((128 - detune) * (uint32_t)freq1) >> 10;
			
			
				// Cambio de octava para el OSC-0
				if(scale0 >= 3){
					TCCR2B = (1 << CS20); // no prescaler
					oct0 = oct0 >> (scale0 - 3);
				} else {
					TCCR2B = (2 << CS20); // prescaler /8
					oct0 = oct0 >> scale0;
				}
				
					
				// Cambio de octava para el OSC-1
				if(scale1 >= 3){
					TCCR1B = (2 << WGM12) | (2 << CS10); // prescaler /8
					freq1 = freq1 >> (scale1 - 3);
				} else {
					TCCR1B = (2 << WGM12) | (3 << CS10); // prescaler /64
					freq1 = freq1 >> scale1;
				}
				
				
				// Ancho de pulso para el OSC-1
				pwm1 = ((uint32_t)freq1 * pw1) >> 8;
				
						
				// TIMERS
				OCR0A = freq0;
				OCR1A = freq1 >> 1;
				OCR1B = pwm1 >> 1;
				OCR2A = oct0;


				
					// variables para el ADC
				
				ADC_conteo = 0x00;
				ADC_promedio = 0;
				// y elijo el canal que sigue para muestrear
				ADMUX = (1 << REFS0) | (1 << ADLAR) | (ADC_variable << MUX0);
			}
		
			ADC_nuevo = 0x00; // limpio el flag
			ADCSRA |= (1 << ADSC); // incio nueva conversión
			ADC_conteo++;
		
		}
	
	}
}
