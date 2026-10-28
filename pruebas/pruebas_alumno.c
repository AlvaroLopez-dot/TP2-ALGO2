#include "pa2m.h"
#include <stdio.h>

/**
 * DISCLAIMER:
 * - Esto es un ejemplo, este código no debe ser parte del archivo definitivo al 
 *   momento de la entrega final.
 * - Las funciones que se prueban (ej. sumar, es_par) se deben implementar
 *   en los archivos de código correspondientes (por ejemplo, lista.c), no 
 *   dentro de los archivos de pruebas.
 * - En este ejemplo se incluyen funciones mínimas implementadas (sumar(), las 
 *   demás son funciones necesarias para pruebas) solo para que el ejemplo 
 *   compile y se entienda la estructura de las pruebas.
 * - Para ver un ejemplo más completo, con las pruebas modularizadas en varios
 *   archivos (un TDA por archivo), ver el repositorio de ejemplo de pruebas:
 *   https://github.com/A2M-202601/EJEMPLO_PRUEBAS
 */

int sumar(int a, int b)
{
	return a + b;
}

/**
 * Pruebas de suma
 */

void prueba_sumar_positivos()
{
	// Inicialización
	int primer_sumando = 1, segundo_sumando = 2;
	int resultado = sumar(primer_sumando, segundo_sumando);

	// Afirmación
	pa2m_afirmar(resultado == 3, "La suma de %i y %i es %i", primer_sumando,
		     segundo_sumando, resultado);

	// Destrucción (si es necesario)
}

void prueba_sumar_con_numeros_negativos()
{
	// Inicialización
	int primer_sumando = -5, segundo_sumando = -3,
	    primer_resultado_esperado = -8;
	int tercer_sumando = -1, cuarto_sumando = 7,
	    segundo_resultado_esperado = 6;

	int primer_resultado = sumar(primer_sumando, segundo_sumando);
	int segundo_resultado = sumar(tercer_sumando, cuarto_sumando);

	// Afirmación
	pa2m_afirmar(
		primer_resultado == primer_resultado_esperado,
		"La suma de dos números negativos es correcta (obtenido = %d; esperado = %d)",
		primer_resultado, primer_resultado_esperado);

	pa2m_afirmar(
		segundo_resultado == segundo_resultado_esperado,
		"La suma de un número negativo y un número positivo es correcta (%d = %d)",
		segundo_resultado, segundo_resultado_esperado);

	// Destrucción (si es necesario)
}

void pruebas_unitarias_sumar()
{
	prueba_sumar_positivos();

	prueba_sumar_con_numeros_negativos();
}

/**
 * Main: Conjunto de grupos de pruebas
 */

int main()
{
	pa2m_nuevo_grupo("============== PRUEBAS DEL TP X ===============");

	pa2m_nuevo_grupo("Pruebas de Suma del TDA X");
	pruebas_unitarias_sumar();

	return pa2m_mostrar_reporte();
}
