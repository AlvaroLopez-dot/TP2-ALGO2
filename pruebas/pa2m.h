#ifndef __PA2M_H_
#define __PA2M_H_

/**
 * Registra y reporta una prueba: evalúa 'afirmacion' e imprime su resultado.
 *
 * 'descripcion' es una cadena de formato al estilo printf, seguida de los
 * argumentos correspondientes al mismo. 
 *
 * Pre: 'descripcion' es una cadena válida (no NULL, terminada en '\0') y los
 *      argumentos variádicos se corresponden con sus especificadores de formato.
 * Post: cuenta una prueba corrida. Si 'afirmacion' es falsa (0), además la
 *       cuenta como fallida. Imprime una línea con ✓ (éxito) o ✗ (fallo)
 *       seguida de la descripción.
 */
void pa2m_afirmar(int afirmacion, const char *descripcion, ...);

/**
 * Imprime un encabezado para agrupar visualmente las pruebas que siguen.
 *
 * Pre: 'descripcion' es una cadena válida (no NULL, terminada en '\0').
 * Post: imprime el título 'descripcion' subrayado. No modifica los contadores
 *       de pruebas.
 */
void pa2m_nuevo_grupo(const char *descripcion);

/**
 * Imprime el reporte final con la cantidad de pruebas corridas y fallidas.
 *
 * Pensada para llamarse una vez, al terminar todas las pruebas. Su valor de
 * retorno sirve como código de salida del programa (0 indica éxito).
 *
 * Post: imprime el resumen y devuelve la cantidad de pruebas fallidas (0 si
 *       todas pasaron).
 */
int pa2m_mostrar_reporte();

#endif // __PA2M_H_
