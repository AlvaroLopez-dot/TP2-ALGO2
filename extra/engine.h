#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <stdbool.h>

#define PANTALLA_ANCHO 16
#define PANTALLA_ALTO 14
#define TILE_PX 16
#define SPRITE_PX 16
#define FUENTE_PX 8
#define PANTALLA_ANCHO_PX (PANTALLA_ANCHO * TILE_PX) // 256px
#define PANTALLA_ALTO_PX (PANTALLA_ALTO * TILE_PX) // 224px

#define MAX_SPRITES 16
#define MAX_TEXTOS 16
#define MAX_LARGO_TEXTO 64

#define REFLEJAR_NO 0
#define REFLEJAR_X 1
#define REFLEJAR_Y 2

#define BOTON_ARRIBA (1u << 0)
#define BOTON_ABAJO (1u << 1)
#define BOTON_IZQUIERDA (1u << 2)
#define BOTON_DERECHA (1u << 3)
#define BOTON_Z (1u << 4)
#define BOTON_X (1u << 5)
#define BOTON_ENTER (1u << 6)
#define BOTON_ESCAPE (1u << 7)

#define CARACTER_BORRAR '\b' /* Backspace */
#define CARACTER_ENTER '\n' /* Enter */

struct entrada {
	// numero de frame actual
	unsigned frame;
	// botones que se mantienen apretados
	unsigned mantenidos;
	// botones que acaban de ser apretados el frame actual
	unsigned presionados;
	/*
	 * Caracter ingresado desde el frame anterior (0 si no hubo ninguno).
	 * Peude ser texto o los caracteres especiales CARACTER_BORRAR o
	 * CARACTER_ENTER. Si se ingresan varios entre frames, llegan de a uno en
	 * los frames siguientes.
	 */
	int caracter;
};

typedef struct pantalla pantalla_t;

// Configuración del engine, es todo opcional
struct engine_config {
	// Titulo de la página
	const char *titulo;
	// 8080 por defecto
	const char *puerto;
	// Los siguientes 3 usan por defecto los archivos en assets/, se pueden personalizar
	const char *tiles;
	const char *sprites;
	const char *fuente;
	// Por defecto 30fps
	unsigned fps;
};

/*
 * Función que el engine invoca en cada frame. Actualiza el estado del juego y
 * redibuja. A implementar por el usuario.
 *
 * Debe devolver 0 para seguir o cualquier otro valor para terminar el juego.
 */
typedef int (*engine_frame_t)(const struct entrada *entrada,
			      pantalla_t *pantalla, void *ctx);

/*
 * Devuelve true si 'boton' (por ejemplo BOTON_ENTER) fue presionado en este
 * frame.
 *
 * Se pueden combinar botones (ej. BOTON_ENTER | BOTON_Z) y devuelve true si
 * alguno de esos botones fueron presionados.
 */
bool engine_presionado(const struct entrada *entrada, unsigned boton);

/*
 * Igual que presionado, pero se mantiene a lo largo de los frames
 */
bool engine_manteniendo(const struct entrada *entrada, unsigned boton);

/*
 * Pone un tile en la pantalla. X e Y se especifican en tiles.
 */
bool engine_tile(pantalla_t *pantalla, int x, int y, unsigned char tile);

/*
 * Borra toda la pantalla con un tile
 */
void engine_llenar(pantalla_t *pantalla, unsigned char tile);

/*
 * Dibuja un sprite en la pantalla. X e Y se especifican en pixeles. reflejo es
 * REFLEJAR_NO, REFLEJAR_X, REFLEJAR_Y o REFLEJAR_X | REFLEJAR_Y.
 *
 * Devuelve false si ya se dibujaron MAX_SPRITES sprites en este frame.
 */
bool engine_sprite(pantalla_t *pantalla, unsigned char sprite, int x, int y,
		   int reflejo);

/*
 * Escribe 'texto' en (x, y) en pixeles.
 * Color tiene formato 0xRRGGBB.
 *
 * Cada caracter ocupa FUENTE_PX x FUENTE_PX pixels.
 */
bool engine_texto(pantalla_t *pantalla, int x, int y, unsigned color,
		  const char *texto);
/*
 * Carga las imágenes, inicia el servidor y atiende al navegador hasta que la
 * función de frame devuelva un valor distinto de 0 o se presione Ctrl-C.
 *
 * 'config' puede ser NULL para usar todos los valores por defecto. Los campos
 * NULL (o 0) también toman su valor por defecto.
 *
 * Devuelve 0 si terminó correctamente o -1 si no pudo cargar las imágenes o
 * iniciar el servidor.
 */
int engine_correr(const struct engine_config *config, engine_frame_t frame,
		  void *ctx);

#endif // __ENGINE_H__
