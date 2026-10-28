#include "extra/engine.h"
#include <string.h>

// Este es un main de ejemplo de uso de engine.h
// Al igual que el informe, es un template
// Reusen lo que crean necesario, reescriban lo que corresponde

#define BLANCO 0xFFFFFF
#define AMARILLO 0xFFE060
#define GRIS 0xA0A0A0
#define LILA 0xA700A7
#define MAX_ENTRADA 12

enum modo { MODO_MENU, MODO_ENTRADA, MODO_JUEGO };
enum direccion { DIR_ABAJO, DIR_ARRIBA, DIR_DERECHA, DIR_IZQUIERDA };

struct juego {
	enum modo modo;

	struct {
		int x, y;
		enum direccion direccion;
		bool caminando;
		unsigned pasos;
	} jugador;

	struct {
		unsigned seleccionado;
	} menu;
	char entrada[MAX_ENTRADA];
};

unsigned char cada_n_frames(unsigned frame, unsigned cada)
{
	unsigned resto = frame % cada;
	return resto > cada / 2;
}

unsigned char obtener_sprite_jugador(struct juego *j, unsigned frame)
{
	unsigned char offset = cada_n_frames(frame, 30);
	enum direccion actual = j->jugador.direccion;
	if (actual == DIR_ABAJO)
		return 0 + offset;
	if (actual == DIR_ARRIBA)
		return 2 + offset;
	if (actual == DIR_IZQUIERDA)
		return 4 + offset;
	//DIR_DERECHA
	return 6 + offset;
}

int dibujar_juego(const struct entrada *e, pantalla_t *p, void *ctx)
{
	struct juego *j = ctx;
	int factor_avance = 1;

	if (engine_manteniendo(e, BOTON_Z))
		factor_avance = 2;

	if (engine_manteniendo(e, BOTON_ABAJO)) {
		j->jugador.direccion = DIR_ABAJO;
		j->jugador.y += factor_avance;
	}
	if (engine_manteniendo(e, BOTON_ARRIBA)) {
		j->jugador.direccion = DIR_ARRIBA;
		j->jugador.y -= factor_avance;
	}
	if (engine_manteniendo(e, BOTON_DERECHA)) {
		j->jugador.direccion = DIR_DERECHA;
		j->jugador.x += factor_avance;
	}
	if (engine_manteniendo(e, BOTON_IZQUIERDA)) {
		j->jugador.direccion = DIR_IZQUIERDA;
		j->jugador.x -= factor_avance;
	}

	//dibujar un fondo asi nomas
	engine_llenar(p, 1);
	engine_tile(p, 5, 5, 3);
	engine_tile(p, 7, 10, 2);
	engine_tile(p, 12, 7, 2);

	//dibujar algunos pokemon
	engine_sprite(p, 16 + cada_n_frames(e->frame, 15), 100, 50, 0);
	engine_sprite(p, 18 + cada_n_frames(e->frame, 9), 120, 150, 0);
	engine_sprite(p, 20 + cada_n_frames(e->frame, 12), 200, 10, 0);
	engine_sprite(p, 22 + cada_n_frames(e->frame, 13), 50, 100, 0);

	//dibujar el personaje principal
	unsigned char sj = obtener_sprite_jugador(j, e->frame);
	engine_sprite(p, sj, j->jugador.x, j->jugador.y, 0);

	//y el nombre
	engine_texto(p, 20, 200, AMARILLO, j->entrada);

	if (engine_presionado(e, BOTON_ESCAPE))
		j->modo = MODO_MENU;

	return 0;
}

unsigned color_condicional(bool condicion, unsigned color_verdadero,
			   unsigned color_falso)
{
	if (condicion)
		return color_verdadero;
	return color_falso;
}

int dibujar_menu(const struct entrada *e, pantalla_t *p, void *ctx)
{
	struct juego *j = ctx;
	if (engine_presionado(e, BOTON_ABAJO))
		j->menu.seleccionado++;
	else if (engine_presionado(e, BOTON_ARRIBA))
		j->menu.seleccionado--;

	j->menu.seleccionado %= 3;
	unsigned char seleccionado = (unsigned char)j->menu.seleccionado;

	engine_texto(p, 40, 80,
		     color_condicional(seleccionado == 0, LILA, BLANCO),
		     "N - Cambiar nombre");

	engine_texto(p, 40, 90,
		     color_condicional(seleccionado == 1, LILA, BLANCO),
		     "J - Iniciar juego");

	engine_texto(p, 40, 100,
		     color_condicional(seleccionado == 2, LILA, BLANCO),
		     "S - Salir");

	engine_sprite(p, 32 + seleccionado, 16, 76 + seleccionado * 10,
		      REFLEJAR_NO);

	bool enter = engine_presionado(e, BOTON_ENTER);
	if (e->caracter == 'n' || e->caracter == 'N' ||
	    (enter && seleccionado == 0))
		j->modo = MODO_ENTRADA;

	if (engine_presionado(e, BOTON_ESCAPE) || e->caracter == 'j' ||
	    e->caracter == 'J' || (enter && seleccionado == 1))
		j->modo = MODO_JUEGO;

	if (e->caracter == 's' || e->caracter == 'S' ||
	    (enter && seleccionado == 2))
		return 1;

	return 0;
}

int dibujar_entrada(const struct entrada *e, pantalla_t *p, void *ctx)
{
	struct juego *j = ctx;
	engine_texto(p, 10, 10, BLANCO, "Ingresa tu nombre:");

	//Ojo con hacer esto si no saben que el string está realmente limitado por tamaño
	unsigned char caracteres = (unsigned char)strlen(j->entrada);
	engine_texto(p, 25, 30, AMARILLO, j->entrada);
	engine_sprite(p, 32, 25 + caracteres * 8, 30 - 4, REFLEJAR_NO);

	if (engine_presionado(e, BOTON_ENTER))
		j->modo = MODO_MENU;

	if (e->caracter == 0)
		return 0;

	if (e->caracter == CARACTER_BORRAR) {
		if (caracteres > 0)
			j->entrada[caracteres - 1] = 0;
		return 0;
	}

	if (caracteres < MAX_ENTRADA - 1)
		j->entrada[caracteres] = (char)e->caracter;

	return 0;
}

int procesar_frame(const struct entrada *e, pantalla_t *p, void *ctx)
{
	struct juego *j = ctx;

	if (j->modo == MODO_JUEGO)
		return dibujar_juego(e, p, ctx);
	if (j->modo == MODO_MENU)
		return dibujar_menu(e, p, ctx);
	else
		return dibujar_entrada(e, p, ctx);
}

int main(int argc, char *argv[])
{
	struct juego juego;
	memset(&juego, 0, sizeof(juego));
	juego.modo = MODO_MENU;

	struct engine_config config = { 0 };
	config.titulo = "Demo TP2";

	if (argc > 1)
		config.puerto = argv[1];

	if (engine_correr(&config, procesar_frame, &juego) != 0)
		return 1;

	return 0;
}
