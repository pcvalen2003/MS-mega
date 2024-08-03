#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL
#define BAUDRATE 31250
#define BAUD_PRESCALLER (((F_CPU / (BAUDRATE * 16UL))) - 1)

typedef struct midi_msg {
		uint8_t byte0;
		uint8_t byte1;
		uint8_t byte2;
		void (*rutina_recepcion)();
	} Midi_msg;

uint8_t byte_numero = 0;

Midi_msg* destino;

void MIDI_init(Midi_msg* midi_destino, void (*rutina_recepcion_)()){
	UBRR0H = (uint8_t)(BAUD_PRESCALLER >> 8); //preescalador del baudrate parte alta
	UBRR0L = (uint8_t)(BAUD_PRESCALLER); //preescalador del baudrate parte baja
	UCSR0B = (1 << RXEN0)|(1 << RXCIE0); //se habilita recepción x interrupción
	UCSR0C = ((1 << UCSZ00)|(1 << UCSZ01)); //configura 8 bits de datos
	
	destino = midi_destino;
	
	midi_destino->byte0 = 0x00;
	midi_destino->byte1 = 0x00;
	midi_destino->byte2 = 0x00;
	midi_destino->rutina_recepcion = rutina_recepcion_;
	
	sei(); // Habilito interrupciones globales
}


ISR(USART_RX_vect){
	uint8_t mensaje = UDR0;
	if((mensaje & 0x80) == 0x80) byte_numero = 0;
	
	switch(byte_numero){
		case 0:
		destino->byte0 = mensaje;
			
		byte_numero = 1;
		return;
		
		case 1:
		destino->byte1 = mensaje;
		byte_numero = 2;
		return;
		
		case 2:
		destino->byte2 = mensaje;
		byte_numero = 1;
		destino->rutina_recepcion();
		return;
		
		default:
		return;
	}
	
}