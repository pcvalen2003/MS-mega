# MS-mega
Sintetizador monofónico basado en el banco de osciladores del Korg MS-20 implementado por hardware en el ATmega 328p.

Este trabajo fue realizado para la cátedra de Arquitectura de Computadoras (Facultad de Ingeniería UNLP).

# Configuración mínima para testeo
El sintetizador por defecto incializa con la nota MIDI 69 (La 4, f = 440Hz), por lo que sin realizar toda la conección MIDI ni de los potenciómetros se puede realizar un primer testeo.
Para este mismo es necesario conectar:
  - PB 3 -> PD 4
  - {PC 0 - PC 4} -> 3,3 V (sino GND)

Y las salidas de los osciladores se encuentran en
  - PD 6 : OSC 0
  - PB 2 : OSC 1

NOTAS:
El OSC 1 va a presentar un detune moderado por estar conectado a 3,3 V.
Si se conecta PC 2 a GND el ancho de pulso del OSC 1 será casi nulo.

# Añadir conectividad MIDI:
Agregando a esta configuración la señal MIDI por el pin PD 0 la frecuencia de los osciladores va a seguir la nota enviada.
En PB 5 se encuentra la señal de GATE para una estapa posterior de envolvente, pero si no se desea hacer uso de etapa posterior conectando PD 3 a GND se dispone de un GATE on-off incorporado.

# Potenciómetros
Finalmente, los pines PC 0 - PC 4 controlan:
  - PC 0 : Glissando (portamento)
  - PC 1 : Detune
  - PC 2 : PW del OSC 1
  - PC 3 : OCT del OSC 0
  - PC 4 : OCT del OSC 1
Ambos controles de octava tienen un rango de selección de -3 hasta +2 octavas.



# Contacto
pc.valen03@gmail.com
