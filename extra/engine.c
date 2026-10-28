/*
 * Mini engine de juegos para el TP2
 *
 * Es un servidor HTTP mínimo (de juguete, no usar para nada serio) que sirve
 * una página con un canvas. La página pide frames al servidor; por cada pedido
 * se invoca la función de frame del alumno y se responde con los comandos de
 * dibujo.
 *
 * No hace falta leer ni entender este archivo para hacer el TP.
 *
 * Es mas, no miren este archivo. O si, no puedo obligarlos.
 *
 * Dejo algunos comentarios para los curiosos
 */

// No, no hagan esto. Está nada mas para poder usar funciones no C99 en este archivo
#define _POSIX_C_SOURCE 200809L

#include "engine.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define ROJO "\033[91m"
#define VERDE "\033[92m"
#define AMARILLO "\033[93m"
#define NORMAL "\033[0m"

#define MAX_CONEXIONES 16
#define TAM_PEDIDO 16384
#define TAM_RESPUESTA_FRAME 8192
#define MAX_TAM_IMAGEN (8 * 1024 * 1024)

//Página principal. Adaptado del original del 2021 pero inline en vez de hosteado.
//Quizás hubiese sido mejor dejarlo como archivo separado en vez de minificado en el .c
static const char *const PAGINA[] = {
	"<!DOCTYPE html><html><head><meta charset='utf-8'><title>TP2</title>",
	"<style>",
	"html,body{margin:0;height:100%;background:#111;color:#999;font:13px monospace}",
	"body{display:flex;flex-direction:column;align-items:center;justify-content:center}",
	"canvas{image-rendering:pixelated;background:#000}",
	"#estado{margin-top:8px;min-height:16px}", "</style></head><body>",
	"<canvas id='c' width='256' height='224'></canvas><div id='estado'>Cargando...</div>",
	"<script>", "'use strict';", "const W=256,H=224,AT=16,AL=14,T=16;",
	"const cv=document.getElementById('c'),g=cv.getContext('2d'),est=document.getElementById('estado');",
	"let tiles,sprites,fuente,sid=0,fps=30,activo=true;",
	"function escalar(){const e=Math.max(1,Math.floor(Math.min(innerWidth/W,(innerHeight-40)/H)));",
	" cv.style.width=W*e+'px';cv.style.height=H*e+'px';}",
	"addEventListener('resize',escalar);escalar();",
	"function cargar(src){return new Promise((ok,mal)=>{const i=new Image();",
	" i.onload=()=>ok(i);i.onerror=()=>mal('no se pudo cargar '+src);i.src=src;});}",
	/* Entrada */
	"const BOTONES={ArrowUp:1,ArrowDown:2,ArrowLeft:4,ArrowRight:8,KeyZ:16,KeyX:32,Enter:64,NumpadEnter:64,Escape:128};",
	"let mantenidos=0,presionados=0;const caracteres=[];",
	"addEventListener('keydown',e=>{",
	" if(e.metaKey||(e.ctrlKey&&!e.altKey))return;",
	" const b=BOTONES[e.code];",
	" if(b){mantenidos|=b;if(!e.repeat)presionados|=b;}",
	" if(e.key==='Backspace')caracteres.push(8);",
	" else if(e.key==='Enter')caracteres.push(10);",
	" else if(e.key.length===1){const c=e.key.charCodeAt(0);if(c>=32&&c<=126)caracteres.push(c);}",
	" if(b||e.key==='Backspace'||e.key===' ')e.preventDefault();", "});",
	"addEventListener('keyup',e=>{const b=BOTONES[e.code];if(b)mantenidos&=~b;});",
	"addEventListener('blur',()=>{mantenidos=0;});",
	/* Dibujo */
	"const tintes=new Map();",
	"function fuenteColor(c){let t=tintes.get(c);if(t)return t;",
	" t=document.createElement('canvas');t.width=fuente.width;t.height=fuente.height;",
	" const x=t.getContext('2d');x.drawImage(fuente,0,0);x.globalCompositeOperation='source-in';",
	" x.fillStyle='#'+c.toString(16).padStart(6,'0');x.fillRect(0,0,t.width,t.height);",
	" tintes.set(c,t);return t;}",
	"function celda(img,n,tam){const cols=Math.max(1,Math.floor(img.width/tam));",
	" return [(n%cols)*tam,Math.floor(n/cols)*tam];}",
	"function dibujar(lineas){",
	" g.imageSmoothingEnabled=false;g.fillStyle='#000';g.fillRect(0,0,W,H);",
	" for(const l of lineas){", "  const p=l.split(' ');",
	"  if(p[0]==='T'){for(let i=0;i<AT*AL;i++){const [sx,sy]=celda(tiles,+p[i+1],T);",
	"    g.drawImage(tiles,sx,sy,T,T,(i%AT)*T,Math.floor(i/AT)*T,T,T);}}",
	"  else if(p[0]==='S'){const id=+p[1],x=+p[2],y=+p[3],f=+p[4];const [sx,sy]=celda(sprites,id,T);",
	"    g.save();g.translate(x+(f&1?T:0),y+(f&2?T:0));g.scale(f&1?-1:1,f&2?-1:1);",
	"    g.drawImage(sprites,sx,sy,T,T,0,0,T,T);g.restore();}",
	"  else if(p[0]==='X'){const x=+p[1],y=+p[2],img=fuenteColor(+p[3]);let i=0;",
	"    for(const ch of p.slice(4).join(' ')){let c=ch.codePointAt(0);if(c>255)c=63;",
	"      const [sx,sy]=celda(img,c,8);g.drawImage(img,sx,sy,8,8,x+i*8,y,8,8);i++;}}",
	" }}", "function terminar(msj){activo=false;est.textContent=msj;",
	" g.fillStyle='rgba(0,0,0,0.6)';g.fillRect(0,0,W,H);}",
	/* Loop principal */
	"let cuadros=0,latencia=0,tMedicion=performance.now();",
	"async function pedirFrame(){",
	" const c=caracteres.length?caracteres.shift():0;",
	" const url='/frame?s='+sid+'&m='+mantenidos+'&p='+presionados+'&c='+c;",
	" presionados=0;const t0=performance.now();",
	" const r=await fetch(url,{cache:'no-store'});const lineas=(await r.text()).split('\\n');",
	" latencia+=performance.now()-t0;cuadros++;",
	" if(lineas[0]==='OTRA'){terminar('El juego se abrió en otra pestaña');return;}",
	" dibujar(lineas);",
	" if(lineas.includes('FIN'))terminar('Juego terminado');}",
	"async function loop(){const paso=1000/fps;let prox=performance.now();",
	" while(activo){",
	"  try{await pedirFrame();}catch(e){terminar('Se perdió la conexión con el juego');break;}",
	"  const ahora=performance.now();", "  if(ahora-tMedicion>=1000){",
	"   if(activo)est.textContent=(cuadros*1000/(ahora-tMedicion)).toFixed(1)+' fps | '+(latencia/cuadros).toFixed(2)+' ms por frame';",
	"   cuadros=0;latencia=0;tMedicion=ahora;}",
	"  prox+=paso;if(prox<ahora-paso)prox=ahora;",
	"  await new Promise(ok=>setTimeout(ok,Math.max(0,prox-ahora)));}}",
	"(async()=>{try{",
	" [tiles,sprites,fuente]=await Promise.all([cargar('/tiles.png'),cargar('/sprites.png'),cargar('/fuente.png')]);",
	" const p=(await (await fetch('/inicio',{cache:'no-store'})).text()).split('\\n');",
	" sid=+p[0];fps=+p[1]||30;document.title=p[2]||'TP2';est.textContent='';",
	" loop();", "}catch(e){est.textContent='Error: '+e;}})();",
	"</script></body></html>", NULL
};

// Manejo de entradas
bool engine_presionado(const struct entrada *entrada, unsigned boton)
{
	return entrada && (entrada->presionados & boton) != 0;
}

bool engine_manteniendo(const struct entrada *entrada, unsigned boton)
{
	return entrada && (entrada->mantenidos & boton) != 0;
}

// Funciones y definiciones de "dibujo"
struct sprite {
	unsigned char id;
	int x, y, reflejo;
};

struct texto {
	int x, y;
	unsigned color;
	char texto[MAX_LARGO_TEXTO + 1];
};

// Esta es la información que se envía al navegador cada frame
struct pantalla {
	unsigned char tiles[PANTALLA_ALTO][PANTALLA_ANCHO];
	struct sprite sprites[MAX_SPRITES];
	size_t cantidad_sprites;
	struct texto textos[MAX_TEXTOS];
	size_t cantidad_textos;
};

static void pantalla_vaciar(pantalla_t *p)
{
	memset(p->tiles, 0, sizeof(p->tiles));
	p->cantidad_sprites = 0;
	p->cantidad_textos = 0;
}

bool engine_tile(pantalla_t *p, int x, int y, unsigned char tile)
{
	if (!p || x < 0 || y < 0 || x >= PANTALLA_ANCHO || y >= PANTALLA_ALTO)
		return false;
	p->tiles[y][x] = tile;
	return true;
}

void engine_llenar(pantalla_t *p, unsigned char tile)
{
	if (p)
		memset(p->tiles, tile, sizeof(p->tiles));
}

bool engine_sprite(pantalla_t *p, unsigned char sprite, int x, int y,
		   int reflejo)
{
	if (!p || p->cantidad_sprites >= MAX_SPRITES)
		return false;
	struct sprite *s = &p->sprites[p->cantidad_sprites++];
	s->id = sprite;
	s->x = x;
	s->y = y;
	s->reflejo = reflejo & (REFLEJAR_X | REFLEJAR_Y);
	return true;
}

bool engine_texto(pantalla_t *p, int x, int y, unsigned color,
		  const char *texto)
{
	if (!p || !texto || p->cantidad_textos >= MAX_TEXTOS)
		return false;
	struct texto *t = &p->textos[p->cantidad_textos++];
	t->x = x;
	t->y = y;
	t->color = color & 0xFFFFFFu;

	size_t largo = strlen(texto);
	if (largo > MAX_LARGO_TEXTO) {
		// Los caracteres UTF8 pueden ser multi byte, vamos sacando
		// caracteres teniendo cuidado de no dejar basura al final
		// (cortar caracter al medio)
		largo = MAX_LARGO_TEXTO;
		while (largo > 0 &&
		       ((unsigned char)texto[largo] & 0xC0) == 0x80)
			largo--;
	}
	for (size_t i = 0; i < largo; i++) {
		unsigned char c = (unsigned char)texto[i];
		// Supongo que podría dejar los caracteres fuera del rango "imprimible"
		// para usar como símbolos, pero de última el cuatrimestre que viene
		t->texto[i] = (c < 32 || c == 127) ? ' ' : (char)c;
	}
	t->texto[largo] = 0;
	return true;
}

// De acá para abajo es manejo de la lógica interna / conexiones
struct archivo {
	char *datos;
	size_t largo;
};

struct conexion {
	int fd;
	size_t largo;
	char pedido[TAM_PEDIDO + 1];
};

struct engine {
	struct engine_config config;
	engine_frame_t frame;
	void *ctx;

	char *pagina;
	size_t largo_pagina;
	struct archivo tiles, sprites, fuente;

	unsigned sesion;
	unsigned numero_frame;
	bool terminado;

	pantalla_t pantalla;
	char respuesta[TAM_RESPUESTA_FRAME];

	struct conexion conexiones[MAX_CONEXIONES];
};

// volatile porque puede cambiar durante el manejo de la señal
static volatile sig_atomic_t interrumpido = 0;

static void manejar_ctrl_c(int senial)
{
	(void)senial;
	interrumpido = 1;
}

// Lectura de archivos
//
// Lee todo el archivo y lo mete en un buffer, validación mínima (header)
static bool cargar_png(const char *ruta, struct archivo *a)
{
	static const unsigned char FIRMA_PNG[8] = { 0x89, 'P',	'N',  'G',
						    '\r', '\n', 0x1a, '\n' };
	FILE *f = fopen(ruta, "rb");
	if (!f) {
		fprintf(stderr, ROJO "No se pudo abrir '%s': %s" NORMAL "\n",
			ruta, strerror(errno));
		return false;
	}

	long tam = -1;
	if (fseek(f, 0, SEEK_END) == 0)
		tam = ftell(f);
	if (tam < 0 || tam > MAX_TAM_IMAGEN || fseek(f, 0, SEEK_SET) != 0) {
		fprintf(stderr, ROJO "No se pudo leer '%s'" NORMAL "\n", ruta);
		fclose(f);
		return false;
	}

	a->datos = malloc((size_t)tam + 1);
	if (!a->datos) {
		fclose(f);
		return false;
	}
	a->largo = fread(a->datos, 1, (size_t)tam, f);
	fclose(f);

	if (a->largo < sizeof(FIRMA_PNG) ||
	    memcmp(a->datos, FIRMA_PNG, sizeof(FIRMA_PNG)) != 0) {
		fprintf(stderr, ROJO "'%s' no es un archivo PNG" NORMAL "\n",
			ruta);
		return false;
	}
	return true;
}

// lo mismo, mete toda la pagina en un buffer
static bool armar_pagina(struct engine *e)
{
	size_t largo = 0;
	for (size_t i = 0; PAGINA[i]; i++)
		largo += strlen(PAGINA[i]) + 1;

	e->pagina = malloc(largo + 1);
	if (!e->pagina)
		return false;

	size_t pos = 0;
	for (size_t i = 0; PAGINA[i]; i++) {
		size_t l = strlen(PAGINA[i]);
		memcpy(e->pagina + pos, PAGINA[i], l);
		pos += l;
		e->pagina[pos++] = '\n';
	}
	e->pagina[pos] = 0;
	e->largo_pagina = pos;
	return true;
}

// Manda un buffer por un socket
static bool enviar_todo(int fd, const char *datos, size_t largo)
{
	while (largo > 0) {
		ssize_t n = send(fd, datos, largo, 0);
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			return false;
		datos += n;
		largo -= (size_t)n;
	}
	return true;
}

// No tenemos nada especial, usamos siempre el mismo encabezado. El tipo es para
// agregar el mime type para los distintos tipos de archivo
static bool responder(int fd, const char *estado, const char *tipo,
		      const char *cuerpo, size_t largo)
{
	char encabezado[256];
	int n = snprintf(encabezado, sizeof(encabezado),
			 "HTTP/1.1 %s\r\n"
			 "Content-Type: %s\r\n"
			 "Content-Length: %zu\r\n"
			 "Cache-Control: no-store\r\n"
			 "Connection: keep-alive\r\n"
			 "\r\n",
			 estado, tipo, largo);
	if (n < 0 || (size_t)n >= sizeof(encabezado))
		return false;
	return enviar_todo(fd, encabezado, (size_t)n) &&
	       enviar_todo(fd, cuerpo, largo);
}

static bool responder_texto(int fd, const char *texto)
{
	return responder(fd, "200 OK", "text/plain; charset=utf-8", texto,
			 strlen(texto));
}

static bool responder_404(int fd)
{
	return responder(fd, "404 Not Found", "text/plain", "404\n", 4);
}
// Busca 'clave' en un query string 'a=1&b=2' y devuelve su valor numérico
static unsigned long parametro(const char *query, const char *clave)
{
	size_t largo_clave = strlen(clave);
	const char *p = query;
	while (p && *p) {
		if (strncmp(p, clave, largo_clave) == 0 &&
		    p[largo_clave] == '=')
			return strtoul(p + largo_clave + 1, NULL, 10);
		p = strchr(p, '&');
		if (p)
			p++;
	}
	return 0;
}

// Esto se usa para formatear los datos a enviar en el cuerpo de la respuesta
static void agregar(char *buffer, size_t *pos, const char *formato, ...)
{
	if (*pos >= TAM_RESPUESTA_FRAME)
		return;
	va_list args;
	va_start(args, formato);
	int n = vsnprintf(buffer + *pos, TAM_RESPUESTA_FRAME - *pos, formato,
			  args);
	va_end(args);
	if (n > 0)
		*pos += (size_t)n;
	if (*pos >= TAM_RESPUESTA_FRAME)
		*pos = TAM_RESPUESTA_FRAME - 1;
}

static size_t serializar_pantalla(struct engine *e)
{
	pantalla_t *p = &e->pantalla;
	char *r = e->respuesta;
	size_t pos = 0;

	agregar(r, &pos, "T");
	for (int y = 0; y < PANTALLA_ALTO; y++)
		for (int x = 0; x < PANTALLA_ANCHO; x++)
			agregar(r, &pos, " %u", (unsigned)p->tiles[y][x]);
	agregar(r, &pos, "\n");

	for (size_t i = 0; i < p->cantidad_sprites; i++) {
		struct sprite *s = &p->sprites[i];
		agregar(r, &pos, "S %u %d %d %d\n", (unsigned)s->id, s->x, s->y,
			s->reflejo);
	}

	for (size_t i = 0; i < p->cantidad_textos; i++) {
		struct texto *t = &p->textos[i];
		agregar(r, &pos, "X %d %d %u %s\n", t->x, t->y, t->color,
			t->texto);
	}

	if (e->terminado)
		agregar(r, &pos, "FIN\n");

	return pos;
}

// Esto procesa la info que nos manda el navegador para cada frame
static bool atender_frame(struct engine *e, int fd, const char *query)
{
	if (e->terminado || parametro(query, "s") != e->sesion)
		return responder_texto(fd, "OTRA\n");

	struct entrada entrada;
	entrada.frame = e->numero_frame++;
	entrada.mantenidos = (unsigned)parametro(query, "m") & 0xFFu;
	entrada.presionados = (unsigned)parametro(query, "p") & 0xFFu;
	unsigned long c = parametro(query, "c");
	entrada.caracter = (c == CARACTER_BORRAR || c == CARACTER_ENTER ||
			    (c >= 32 && c <= 126)) ?
				   (int)c :
				   0;

	pantalla_vaciar(&e->pantalla);
	if (e->frame(&entrada, &e->pantalla, e->ctx) != 0)
		e->terminado = true;

	size_t largo = serializar_pantalla(e);
	return responder(fd, "200 OK", "text/plain; charset=utf-8",
			 e->respuesta, largo);
}

// Y esta es la lógica principal para manejo del request HTTP
// Solo aceptamos GET y devolvemos uno de los recursos pedidos (pagina, pngs, frame, etc)
static bool atender_pedido(struct engine *e, int fd, char *pedido)
{
	char *metodo = strtok(pedido, " ");
	char *uri = strtok(NULL, " \r\n");
	if (!metodo || !uri || strcmp(metodo, "GET") != 0)
		return responder_404(fd);

	char *query = strchr(uri, '?');
	if (query)
		*query++ = 0;

	if (strcmp(uri, "/frame") == 0)
		return atender_frame(e, fd, query);

	if (strcmp(uri, "/") == 0)
		return responder(fd, "200 OK", "text/html; charset=utf-8",
				 e->pagina, e->largo_pagina);

	if (strcmp(uri, "/inicio") == 0) {
		char texto[512];
		e->sesion++;
		snprintf(texto, sizeof(texto), "%u\n%u\n%s\n", e->sesion,
			 e->config.fps, e->config.titulo);
		printf(VERDE "Navegador conectado" NORMAL "\n");
		return responder_texto(fd, texto);
	}

	struct archivo *a = NULL;
	if (strcmp(uri, "/tiles.png") == 0)
		a = &e->tiles;
	else if (strcmp(uri, "/sprites.png") == 0)
		a = &e->sprites;
	else if (strcmp(uri, "/fuente.png") == 0)
		a = &e->fuente;
	if (a)
		return responder(fd, "200 OK", "image/png", a->datos, a->largo);

	return responder_404(fd);
}

// Y ahora sí la lógica de sockets
static void cerrar_conexion(struct conexion *c)
{
	if (c->fd >= 0)
		close(c->fd);
	c->fd = -1;
	c->largo = 0;
}

static void aceptar_conexion(struct engine *e, int servidor)
{
	int fd = accept(servidor, NULL, NULL);
	if (fd < 0)
		return;
	// deshabilitamos la dedmora
	int si = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &si, sizeof(si));
	for (size_t i = 0; i < MAX_CONEXIONES; i++) {
		if (e->conexiones[i].fd < 0) {
			e->conexiones[i].fd = fd;
			e->conexiones[i].largo = 0;
			return;
		}
	}
	close(fd);
}

static void leer_conexion(struct engine *e, struct conexion *c)
{
	ssize_t n = recv(c->fd, c->pedido + c->largo, TAM_PEDIDO - c->largo, 0);
	if (n <= 0) {
		if (n < 0 && errno == EINTR)
			return;
		cerrar_conexion(c);
		return;
	}
	c->largo += (size_t)n;
	c->pedido[c->largo] = 0;

	/* Puede haber más de un pedido en el buffer */
	char *fin;
	while (c->fd >= 0 && (fin = strstr(c->pedido, "\r\n\r\n"))) {
		size_t consumido = (size_t)(fin - c->pedido) + 4;
		*fin = 0;
		if (!atender_pedido(e, c->fd, c->pedido)) {
			cerrar_conexion(c);
			return;
		}
		memmove(c->pedido, c->pedido + consumido, c->largo - consumido);
		c->largo -= consumido;
		c->pedido[c->largo] = 0;
	}

	if (c->largo >= TAM_PEDIDO)
		cerrar_conexion(c); /* Pedido demasiado grande */
}

static int crear_servidor(const char *puerto)
{
	char *fin;
	long numero = strtol(puerto, &fin, 10);
	if (*fin || numero <= 0 || numero > 65535) {
		fprintf(stderr, ROJO "Puerto inválido: '%s'" NORMAL "\n",
			puerto);
		return -1;
	}

	int servidor = socket(AF_INET, SOCK_STREAM, 0);
	if (servidor < 0) {
		perror(ROJO "Error al crear el servidor" NORMAL);
		return -1;
	}

	int si = 1;
	setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &si, sizeof(si));

	struct sockaddr_in direccion;
	memset(&direccion, 0, sizeof(direccion));
	direccion.sin_family = AF_INET;
	direccion.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	direccion.sin_port = htons((uint16_t)numero);

	if (bind(servidor, (struct sockaddr *)&direccion, sizeof(direccion)) !=
		    0 ||
	    listen(servidor, MAX_CONEXIONES) != 0) {
		fprintf(stderr,
			ROJO "No se pudo iniciar el servidor en el puerto %s: "
			     "%s" NORMAL "\n",
			puerto, strerror(errno));
		close(servidor);
		return -1;
	}
	return servidor;
}

// Ahora si, creamos el server web con el puerto pedido
static void servir(struct engine *e, int servidor)
{
	struct pollfd fds[MAX_CONEXIONES + 1];
	struct conexion *de_fd[MAX_CONEXIONES + 1];

	while (!e->terminado && !interrumpido) {
		nfds_t n = 0;
		fds[n].fd = servidor;
		fds[n].events = POLLIN;
		de_fd[n++] = NULL;
		for (size_t i = 0; i < MAX_CONEXIONES; i++) {
			if (e->conexiones[i].fd >= 0) {
				fds[n].fd = e->conexiones[i].fd;
				fds[n].events = POLLIN;
				de_fd[n++] = &e->conexiones[i];
			}
		}

		int listos = poll(fds, n, 500);
		if (listos < 0) {
			if (errno == EINTR)
				continue;
			perror(ROJO "Error en poll" NORMAL);
			return;
		}

		for (nfds_t i = 1; i < n && !e->terminado; i++)
			if (fds[i].revents & (POLLIN | POLLHUP | POLLERR))
				leer_conexion(e, de_fd[i]);

		if (fds[0].revents & POLLIN)
			aceptar_conexion(e, servidor);
	}
}

static const char *o_defecto(const char *valor, const char *defecto)
{
	return valor ? valor : defecto;
}

// Y por último combinamos todo.
// Esto arranca el server y empieza a invocar la lógica del usuario
int engine_correr(const struct engine_config *config, engine_frame_t frame,
		  void *ctx)
{
	if (!frame)
		return -1;

	struct engine *e = calloc(1, sizeof(struct engine));
	if (!e)
		return -1;

	struct engine_config vacia = { 0 };
	if (!config)
		config = &vacia;
	e->config.titulo = o_defecto(config->titulo, "TP2");
	e->config.puerto = o_defecto(config->puerto, "8080");
	e->config.tiles = o_defecto(config->tiles, "assets/tiles.png");
	e->config.sprites = o_defecto(config->sprites, "assets/sprites.png");
	e->config.fuente = o_defecto(config->fuente, "assets/fuente.png");
	e->config.fps = config->fps ? config->fps : 30;
	e->frame = frame;
	e->ctx = ctx;
	for (size_t i = 0; i < MAX_CONEXIONES; i++)
		e->conexiones[i].fd = -1;

	int resultado = -1;
	int servidor = -1;

	if (!armar_pagina(e) || !cargar_png(e->config.tiles, &e->tiles) ||
	    !cargar_png(e->config.sprites, &e->sprites) ||
	    !cargar_png(e->config.fuente, &e->fuente))
		goto liberar;

	servidor = crear_servidor(e->config.puerto);
	if (servidor < 0)
		goto liberar;

	// Ctrl-C termina el loop; SIGPIPE se ignora (el navegador puede
	// cerrar una conexión en cualquier momento) mas allá del keep-alive
	struct sigaction nueva, vieja_int, vieja_pipe;
	memset(&nueva, 0, sizeof(nueva));
	sigemptyset(&nueva.sa_mask);
	nueva.sa_handler = manejar_ctrl_c;
	sigaction(SIGINT, &nueva, &vieja_int);
	nueva.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &nueva, &vieja_pipe);
	interrumpido = 0;

	printf("Juego iniciado. Abrir " VERDE "http://localhost:%s" NORMAL
	       " en el navegador (Ctrl-C para salir)\n",
	       e->config.puerto);
	fflush(stdout);

	servir(e, servidor);

	if (interrumpido)
		printf("\n" AMARILLO "Juego interrumpido" NORMAL "\n");

	sigaction(SIGINT, &vieja_int, NULL);
	sigaction(SIGPIPE, &vieja_pipe, NULL);
	resultado = 0;

liberar:
	for (size_t i = 0; i < MAX_CONEXIONES; i++)
		cerrar_conexion(&e->conexiones[i]);
	if (servidor >= 0)
		close(servidor);
	free(e->pagina);
	free(e->tiles.datos);
	free(e->sprites.datos);
	free(e->fuente.datos);
	free(e);
	return resultado;
}
