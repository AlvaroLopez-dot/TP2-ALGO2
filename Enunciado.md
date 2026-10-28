<div align="right">
<img width="32px" src="img/algo2.svg">
</div>

# TP2: Aplicando los conocimientos adquiridos

Para este TP se piden 3 cosas:

- **Cambiar la implementación del TP1**: Se pide que busquen en el TP1 cosas que se puedan mejorar y reimplementar utilizando los conocimientos adquiridos durante la materia. Esto incluye implementacion de funciones y estructuras. En lo posible deberían mantener las mismas pruebas que hicieron para el TP1. Se pueden agregar mas. En caso de necesitar cambiar alguna prueba del TP1 explique por qué. Tener en cuenta si utilizaron un vector para almacenar los pokemon, es obligatorio cambiar esta estructura por otro tipo de entidad. Trabajos que entreguen vector como entidad de almacenamiento del TP1 seran automaticamente desaprobados.
- **Implementar un TDA Menú**: Este TDA debe permitirnos interactuar con nuestro programa. Debe ser lo suficientemente genérico para poder ser utilizado tanto por la consola como por la interfaz gráfica.
- **Implementar la lógica del juego pedido**

## Lógica del juego

El juego pedido consiste en cargar una lista de pokemon de un archivo utilizando las primitivas del TP1 para luego utilizarlos en una prueba de observación y memoria. Las reglas son las siguientes:

- Al comenzar la partida el jugador selecciona una dificultad (a criterio del alumno)
- La partida consiste en mostrar por pantalla una serie de pokemones viajando ya sea de izquierda a derecha o derecha a izquierda (no necesariamente todos en la misma dirección)
- Al finalizar esta etapa, se le presenta un cuestionario al jugador: 3 preguntas con 3 posibles respuestas cada una.
- El jugador responde las preguntas y recibe un puntaje en base a la cantidad de aciertos.

### Los pokemon

Los pokemon viajan por la pantalla en diferentes direcciones y velocidades. La trayectoria de cada pokemon no tiene por qué ser lineal (se recomienda que no lo sea). Cada pokemon debe tener una forma de ser identificado del resto (diferente forma). Adicionalmente cada pokemon debe poder ser distinguido por rareza (por color).

**Nota**: La velocidad a la cual se mueve un pokemon no tiene que ser exactamente la que indica el pokedex, pudiendo variar para hacer el juego mas interesante.

### El cuestionario

Durante la etapa del cuestionario se seleccionan preguntas de un pool de preguntas predeterminadas a establecer por el alumno. Algunos ejemplos de preguntas posibles:

- **¿Cuántos pokemon totales viste pasar?**
- **¿Cuántos pokemon legendarios viste?**
- **¿Cuál de estos pokemon fue el mas rápido?**
- **¿Cuántos pokemon viste pasar de derecha a izquierda?**
- **¿Cuántos tipos diferentes de pokemon viste?**

Cada una de las preguntas debe presentar 3 posibles respuestas donde una sola debe ser correcta.

### Fin del juego

Al finalizar el cuestionario se muestra la cantidad de aciertos y las estadísticas de la corrida (las datos con las respuestas correctas a todas las preguntas posibles, se hayan utilizado o no).


## Menú principal

La aplicación requiere de 1 o 2 parámetros. El primer parámetro es el nombre del archivo **.csv** a utilizar para el juego. El segundo parámetro es opcional y especifica el puerto a utilizar para la interfaz gráfica. Si se omite el segundo parámetro, el juego comienza en modo consola, si se especifica en modo gráfico.

Ya sea en modo consola o gráfico, el juego debe mostrar un menú con las siguientes opciones:

- **Jugar (J)**: Inicia el juego. En modo consola muestra una leyenda indicando que sólo se puede ejecutar en modo gráfico.
- **Jugar con semilla (S)**: Pide al usuario ingresar una semilla para el generador de números aleatorios y luego inicia el juego.
- **Mostrar pokemon (M)**: Pide al usuario ingresar un nombre de pokemon y lo busca. Si lo encuentra, muestra los datos por pantalla.
- **Mostrar informacion (I)**: Muestra un resumen de los datos de la pokedex (cantidad total de pokemones, cantidad de diferentes tipos de rareza, pokemon mas y menos veloz/pesado)
- **Salir (Q)**: Sale del juego

---


## Puntos teóricos

 - Explicar los cambios realizados al TP1. Justifique cada cambio aplicado.
 - Explique cómo funciona el menú y cómo abstrae el problema de la independencia de la interfaz gráfica.
 - En general, explique qué estructuras de las implementadas fueron utilizadas para implementar la lógica del TP2 y por qué.
