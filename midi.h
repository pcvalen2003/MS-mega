#include "midi.c"

#define F_CPU 16000000UL
#define BAUDRATE 31250
#define BAUD_PRESCALLER (((F_CPU / (BAUDRATE * 16UL))) - 1)


void MIDI_init(Midi_msg* midi_destino, void (*rutina_recepcion_)());