#include "teclado_TL04.h"
#include "reloj.h"
#include "ent2004cfConfig.h"



const char tecladoTL04[NUM_FILAS_TECLADO][NUM_COLUMNAS_TECLADO] = {
		{'1', '2', '3', 'C'},
		{'4', '5', '6', 'D'},
		{'7', '8', '9', 'E'},
		{'A', '0', 'B', 'F'}
};

// Maquina de estados: lista de transiciones
// {EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
fsm_trans_t g_fsmTransExcitacionColumnas[] = {
		{ TECLADO_ESPERA_COLUMNA, CompruebaTimeoutColumna, TECLADO_ESPERA_COLUMNA, TecladoExcitaColumna },
		{-1, NULL, -1, NULL },
};

static TipoTecladoShared g_tecladoSharedVars;
static void (*array_row_p[4]) () = {teclado_fila_1_isr, teclado_fila_2_isr, teclado_fila_3_isr, teclado_fila_4_isr};

//------------------------------------------------------
// FUCNIONES DE INICIALIZACION DE LAS VARIABLES ESPECIFICAS
//------------------------------------------------------
void ConfiguraInicializaTeclado(TipoTeclado *p_teclado) {
// A completar por el alumno...

	// Inicializacion de elementos de la variable global de tipo TipoTecladoShared:
	// 1. Valores iniciales de todos los "debounceTime"
	// 2. Valores iniciales de todos "columnaActual", "teclaDetectada" y "flags"

	TipoTecladoShared aux = {
			.flags = 0,
			.debounceTime = {0, 0, 0, 0},
			.columnaActual = 0,
			.teclaDetectada = ' '
	};

	SetTecladoSharedVar(aux);

	// Inicializacion de elementos de la estructura TipoTeclado:

	// Inicializacion del HW:
	// 1. Configura GPIOs de las columnas:
	// 	  (i) Configura los pines y (ii) da valores a la salida

	int i = 0;
	for (i=0; i<4; i++) {
		pinMode(p_teclado->columnas[i], OUTPUT);
	}

	digitalWrite(GPIO_KEYBOARD_COL_1, HIGH);
	digitalWrite(GPIO_KEYBOARD_COL_2, LOW);
	digitalWrite(GPIO_KEYBOARD_COL_3, LOW);
	digitalWrite(GPIO_KEYBOARD_COL_4, LOW);

	// 2. Configura GPIOs de las filas:
	// 	  (i) Configura los pines y (ii) asigna ISRs (y su polaridad)

	i = 0;
	for (i=0; i<4; i++) {
		pinMode(p_teclado->filas[i], INPUT);
		pullUpDnControl(p_teclado->filas[i], PUD_DOWN); // PUT_DOWN ES LO DE APRETAR EL BOTON, NO AL LEVANTARLO
		wiringPiISR(p_teclado->filas[i], INT_EDGE_RISING, array_row_p[i]);
	}

	// Inicializacion del temporizador:
	// 3. Crear y asignar temporizador de excitacion de columnas
	// 4. Lanzar temporizador
	tmr_t* tmrTmp = tmr_new(timer_duracion_columna_isr);
	p_teclado->tmr_duracion_columna = tmrTmp;
	tmr_startms((tmr_t*) (p_teclado->tmr_duracion_columna), TIMEOUT_COLUMNA_TECLADO_MS);
}

//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
/* Getter y setters de variables globales */
TipoTecladoShared GetTecladoSharedVar() {
	// A completar por el alumno
	piLock(KEYBOARD_KEY);
	TipoTecladoShared copia = g_tecladoSharedVars;
	piUnlock(KEYBOARD_KEY);
	return copia;
}
void SetTecladoSharedVar(TipoTecladoShared value) {
	// A completar por el alumno
	piLock(KEYBOARD_KEY);
    g_tecladoSharedVars = value;
    piUnlock(KEYBOARD_KEY);
}

void ActualizaExcitacionTecladoGPIO(int columna) {
	// ATENCION: Evitar que este mas de una columna activa a la vez.

	// A completar por el alumno
	// ...
	switch(columna){
		case 0:
			digitalWrite(GPIO_KEYBOARD_COL_1, HIGH);
			digitalWrite(GPIO_KEYBOARD_COL_2, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_3, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_4, LOW);
			break;
		case 1:
			digitalWrite(GPIO_KEYBOARD_COL_1, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_2, HIGH);
			digitalWrite(GPIO_KEYBOARD_COL_3, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_4, LOW);
			break;
		case 2:
			digitalWrite(GPIO_KEYBOARD_COL_1, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_2, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_3, HIGH);
			digitalWrite(GPIO_KEYBOARD_COL_4, LOW);
			break;
		case 3:
			digitalWrite(GPIO_KEYBOARD_COL_1, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_2, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_3, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_4, HIGH);
			break;
		default:
			break;
	}
}

//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
int CompruebaTimeoutColumna(fsm_t* p_this) {
	int result = 0;
	// A completar por el alumno...
	piLock(KEYBOARD_KEY);
	result = g_tecladoSharedVars.flags & FLAG_TIMEOUT_COLUMNA_TECLADO;
	piUnlock(KEYBOARD_KEY);
	return result;
}


//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LAS MAQUINAS DE ESTADOS
//------------------------------------------------------
void TecladoExcitaColumna(fsm_t* p_this) {
	TipoTeclado *p_teclado = (TipoTeclado*)(p_this->user_data);

	// 1. Actualizo que columna SE VA a excitar
	// 2. Ha pasado el timer y es hora de excitar la siguiente columna:
	//    (i) Llamada a ActualizaExcitacionTecladoGPIO con columna A ACTIVAR como argumento
	// 3. Actualizar la variable flags
	// 4. Manejar el temporizador para que vuelva a avisarnos

	// A completar por el alumno
	// ...

	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.columnaActual += 1;
	g_tecladoSharedVars.columnaActual %= 4;
	ActualizaExcitacionTecladoGPIO(g_tecladoSharedVars.columnaActual);
	g_tecladoSharedVars.flags &= ~FLAG_TIMEOUT_COLUMNA_TECLADO;
	piUnlock(KEYBOARD_KEY);

	tmr_startms((tmr_t*) (p_teclado->tmr_duracion_columna), TIMEOUT_COLUMNA_TECLADO_MS);
}

//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------
static void boton_isr (int b) {
	int now = millis();
	if (now < g_tecladoSharedVars.debounceTime[b]) {
		return;
	}
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.teclaDetectada = tecladoTL04[b][g_tecladoSharedVars.columnaActual];
	g_tecladoSharedVars.flags |= FLAG_TECLA_PULSADA;
	g_tecladoSharedVars.debounceTime[b] = now + DEBOUNCE_TIME_MS;
	piUnlock(KEYBOARD_KEY);
}

void teclado_fila_1_isr(void) {
	// 1. Comprobar si ha pasado el tiempo de guarda de anti-rebotes
	// 2. Atender a la interrupcion:
	// 	  (i) Guardar la tecla detectada en g_tecladoSharedVars
	//    (ii) Activar flag para avisar de que hay una tecla pulsada
	// 3. Actualizar el tiempo de guarda del anti-rebotes
	boton_isr(0);
}

void teclado_fila_2_isr(void) {
	// A completar por el alumno
	boton_isr(1);
}

void teclado_fila_3_isr(void) {
	// A completar por el alumno
	boton_isr(2);
}

void teclado_fila_4_isr (void) {
	// A completar por el alumno
	boton_isr(3);
}

void timer_duracion_columna_isr(union sigval value) {
	// Simplemente avisa que ha pasado el tiempo para excitar la siguiente columna
	// A completar por el alumno
	// ...
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags |= FLAG_TIMEOUT_COLUMNA_TECLADO;
	piUnlock(KEYBOARD_KEY);
}

