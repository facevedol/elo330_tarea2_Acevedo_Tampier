csa Version 1.0 10/10/2014
Por: Niklas Tampier - Felipe Acevedo

DESCRIPCIÓN
-----------
csa es un programa que tiene como input una señal de audio, la que es amplificada por una ganancia, para posteriormente recuperar la señal original recuperando las partes saturadas con una interpolación de orden 4.
Se muestran graficos de los 40 [ms] desde el offset de la señal original, la saturada y la recuperada.
Además se tiene la opción de reproducir las tres señales. 

NOTAS DE USO GENERAL
--------------------

- csa necesita que octave y aplay ( disponible con ALSA) estén instalados en el sistema.

- Para compilación ejecutar "make"

SINTAXIS
--------
./csa <archivo_de_audio> <ganancia> <offset> [p]

archivo_de_audio: Audio monocanal en formato PCM, little endian, con signo, 16 bits y frecuencia de muestreo de 8[KHz].
ganancia: número real mayor que 1.
offset: tiempo en [ms].
p: opcional si se desea reproducir los sonidos generados.



