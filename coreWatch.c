#include "coreWatch.h"
#include "reloj.h"
#include "systemConfig.h"
//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
// Wait until next_activation (absolute time)
// Necesita de la función "delay" de WiringPi.
void DelayUntil(unsigned int next) {
	unsigned int now = millis();
	if (next > now) {
		delay(next - now);
	}
}

// VARIABLES GLOBALES
TipoCoreWatch g_coreWatch;
static int g_flagsCoreWatch;
static int g_digitoPulsado;

fsm_trans_t fsmTransCoreWatch[] = {
    {START, CompruebaSetupDone, STAND_BY, Start},
	{STAND_BY, CompruebaTimeActualizado, STAND_BY, ShowTime},

	{STAND_BY, CompruebaSetCancelNewTime, SET_TIME, PrepareSetNewTime},
	{SET_TIME, CompruebaSetCancelNewTime, STAND_BY, CancelSetNewTime},
	{STAND_BY, CompruebaReset, STAND_BY, Reset},

	{SET_TIME, CompruebaNewTimeIsReady, STAND_BY, SetNewTime},
	{SET_TIME, CompruebaDigitoPulsado, SET_TIME, ProcesaDigitoTime},

    {-1, NULL, -1, NULL}
};

fsm_trans_t fsmTransDeteccionComandos[] = {
		{WAIT_COMMAND, CompruebaTeclaPulsada, WAIT_COMMAND, ProcesaTeclaPulsada},
		{-1, NULL, -1, NULL}
};

// FUNCIONES DE INICIALIZACION DE VARIABLES
int ConfiguraInicializaSistema(TipoCoreWatch *p_sistema) {
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch = 0;
	piUnlock(SYSTEM_KEY);

	p_sistema->tempTime = 0;
	p_sistema->digitosGuardados = 0;
	p_sistema->digitoPulsado = 0;
	p_sistema->lcdId = -1;

	int aux1 = ConfiguraInicializaReloj(&p_sistema->reloj);

	/*----- Configuración del teclado -----*/
	#if VERSION >= 3
		int arrayFilas[4] = {GPIO_KEYBOARD_ROW_1,GPIO_KEYBOARD_ROW_2,GPIO_KEYBOARD_ROW_3,GPIO_KEYBOARD_ROW_4};
		int arrayColumnas[4] = {GPIO_KEYBOARD_COL_1,GPIO_KEYBOARD_COL_2,GPIO_KEYBOARD_COL_3,GPIO_KEYBOARD_COL_4};

		memcpy(p_sistema->teclado.filas, arrayFilas, sizeof(arrayFilas));
		memcpy(p_sistema->teclado.columnas, arrayColumnas, sizeof(arrayColumnas));

		int a = wiringPiSetupGpio();
		if (a != 0) {
			return -1;
			exit(0);
		}

		ConfiguraInicializaTeclado(&p_sistema->teclado);
	#endif


	/*----- Configuración del LCD -----*/
	#if VERSION >= 4
		p_sistema->lcdId = lcdInit(2, 12, 8, GPIO_LCD_RS, GPIO_LCD_EN,
		GPIO_LCD_D0, GPIO_LCD_D1, GPIO_LCD_D2, GPIO_LCD_D3,
		GPIO_LCD_D4, GPIO_LCD_D5, GPIO_LCD_D6, GPIO_LCD_D7);


		lcdPuts(p_sistema->lcdId, "Bienvenido");

		// sleep(500);

		if(p_sistema->lcdId == -1) {
			return -1;
			exit(0);
		}
	#endif

	if (aux1 == 0) {
		return aux1;
	} else {
		int aux2 = piThreadCreate(ThreadExploraTecladoPC);

		piLock(STD_IO_LCD_BUFFER_KEY);
		if (aux2 == 0) {
			printf("HEBRA CREADA!\r\n");
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch |= FLAG_SETUP_DONE;
			piUnlock(SYSTEM_KEY);
		} else {
			printf("ERROR AL CREAR HEBRA!");
		}
		fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
		return aux2;
	}

	piLock(SYSTEM_KEY);
	g_flagsCoreWatch |= FLAG_SETUP_DONE;
	piUnlock(SYSTEM_KEY);
}

PI_THREAD(ThreadExploraTecladoPC) {
	int teclaPulsada;
	while(1) {
		delay(10);
		if(kbhit() != 0) {
			piLock(KEYBOARD_KEY);
			teclaPulsada = kbread();
			piUnlock(KEYBOARD_KEY);
			piLock(SYSTEM_KEY);
			piLock(STD_IO_LCD_BUFFER_KEY);
			if (teclaPulsada == TECLA_RESET) {
				g_flagsCoreWatch |= FLAG_RESET;
			} else if (teclaPulsada == TECLA_SET_CANCEL_TIME){
				g_flagsCoreWatch |= FLAG_SET_CANCEL_NEW_TIME;
			} else if (EsNumero(teclaPulsada)) {
				g_coreWatch.digitoPulsado = teclaPulsada - 48;
				g_flagsCoreWatch |= FLAG_DIGITO_PULSADO;
			} else if (teclaPulsada == TECLA_EXIT) {
				//printf("\r\nSaliendo del sistema...\r\n");
				//fflush(stdout);
				//exit(0);
			} else if ((teclaPulsada != '\n') && (teclaPulsada != '\r') && (teclaPulsada != 0xA)) {
				printf("Tecla desconocida\r\n");
				fflush(stdout);
			}
			piUnlock(STD_IO_LCD_BUFFER_KEY);
			piUnlock(SYSTEM_KEY);
		}
	}
}

int EsNumero(char value) {
	if (value >= 0x30 && value <= 0x39){
		return 1;
	}
	return 0;
}

int CompruebaDigitoPulsado(fsm_t* p_this) {
	int aux;
	piLock(SYSTEM_KEY);
	aux = g_flagsCoreWatch & FLAG_DIGITO_PULSADO;
	piUnlock(SYSTEM_KEY);
	return aux;
}

int CompruebaNewTimeIsReady(fsm_t* p_this) {
	int aux;
	piLock(SYSTEM_KEY);
	aux = g_flagsCoreWatch & FLAG_NEW_TIME_IS_READY;
	piUnlock(SYSTEM_KEY);
	return aux;
}

int CompruebaReset(fsm_t* p_this) {
	int aux;
	piLock(SYSTEM_KEY);
	aux = g_flagsCoreWatch & FLAG_RESET;
	piUnlock(SYSTEM_KEY);
	return aux;
}

int CompruebaSetCancelNewTime(fsm_t* p_this) {
	int aux;
	piLock(SYSTEM_KEY);
	aux = g_flagsCoreWatch & FLAG_SET_CANCEL_NEW_TIME;
	piUnlock(SYSTEM_KEY);
	return aux;
}

int CompruebaSetupDone(fsm_t* p_this) {
	int aux;
	piLock(SYSTEM_KEY);
	aux = g_flagsCoreWatch & FLAG_SETUP_DONE;
	piUnlock(SYSTEM_KEY);
	return aux;
}

int CompruebaTeclaPulsada(fsm_t* p_this) {
	TipoTecladoShared aux1 = GetTecladoSharedVar();
	int aux2 = aux1.flags & FLAG_TECLA_PULSADA;
	return aux2;
}

int CompruebaTimeActualizado(fsm_t* p_this) {
	piLock(SYSTEM_KEY);
	int aux = GetRelojSharedVar().flags & FLAG_TIME_ACTUALIZADO;
	piUnlock(SYSTEM_KEY);
	return aux;
}

void Start(fsm_t* p_this) {
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= (~FLAG_SETUP_DONE);
	piUnlock(SYSTEM_KEY);
}

void ShowTime(fsm_t* p_this) {
	TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);
    // fsm_t* fsm_new (int state, fsm_trans_t* tt, void* user_data);

	TipoRelojShared relojShared = GetRelojSharedVar();

	piLock(SYSTEM_KEY);
	relojShared.flags &= (~FLAG_TIME_ACTUALIZADO);
	piUnlock(SYSTEM_KEY);

	SetRelojSharedVar(relojShared);

	#if VERSION == 4
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("Son las: %d:%d:%d del %d/%d/%d\r\n", p_sistema->reloj.hora.hh, p_sistema->reloj.hora.mm,
		p_sistema->reloj.hora.ss, p_sistema->reloj.calendario.dd, p_sistema->reloj.calendario.MM, 
		p_sistema->reloj.calendario.yyyy);
		fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

	#if VERSION >= 4
		int lcdId = p_sistema->lcdId;
		piLock(STD_IO_LCD_BUFFER_KEY);

		lcdClear(lcdId);
		lcdPosition(lcdId, 0, 0);
		lcdPrintf(lcdId, " %d:%d:%d",p_sistema->reloj.hora.hh, p_sistema->reloj.hora.mm,
				p_sistema->reloj.hora.ss);
		lcdPosition(lcdId, 0, 1);
		lcdPrintf(lcdId, " %d/%d/%d", p_sistema->reloj.calendario.dd, p_sistema->reloj.calendario.MM,
				p_sistema->reloj.calendario.yyyy);
		lcdPosition(lcdId, 0, 0);

		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
}

void Reset(fsm_t* p_this) {
	TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);
	ResetReloj(&p_sistema->reloj);

	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= (~FLAG_RESET);
	piUnlock(SYSTEM_KEY);

	#if VERSION <= 4
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("\r\n[RESET] Hora reiniciada\r\n");
		fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

	#if VERSION >= 4
		piLock(STD_IO_LCD_BUFFER_KEY);

		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId, 0, 1);
		lcdPrintf(p_sistema->lcdId, "RESET");
		delay(ESPERA_MENSAJE_MS);
		lcdClear(p_sistema->lcdId);

		piUnlock(STD_IO_LCD_BUFFER_KEY);

	#endif
}

void PrepareSetNewTime(fsm_t* p_this) {
	TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);
	int formato = p_sistema->reloj.hora.formato;
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= (~FLAG_DIGITO_PULSADO);
	g_flagsCoreWatch &= (~FLAG_SET_CANCEL_NEW_TIME);
	piUnlock(SYSTEM_KEY);

	#if VERSION <= 4
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("\r\n[SET_TIME] Introduzca la nueva hora en formato 0-%d\r\n", formato);
		fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

	#if VERSION >= 4
		piLock(STD_IO_LCD_BUFFER_KEY);

		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId, 0, 1);
		lcdPrintf(p_sistema->lcdId, "FORMAT: 0-%d", formato);

		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
}

void CancelSetNewTime(fsm_t* p_this) {
	TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);
	p_sistema->tempTime = 0;
	p_sistema->digitosGuardados = 0;
	
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= (~FLAG_SET_CANCEL_NEW_TIME);
	piUnlock(SYSTEM_KEY);

	#if VERSION <= 4
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("\r\n[SET_TIME] Operacion cancelada\r\n");
		fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

	#if VERSION >= 4
		piLock(STD_IO_LCD_BUFFER_KEY);

		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId, 0, 1);
		lcdPrintf(p_sistema->lcdId, "CANCELADO");
		delay(ESPERA_MENSAJE_MS);
		lcdClear(p_sistema->lcdId);

		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
}

void ProcesaDigitoTime(fsm_t* p_this) {
	TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);

	int l_tempTime = p_sistema->tempTime;
	int l_digitosGuardados = p_sistema->digitosGuardados;

	int ultimoDigito = p_sistema->digitoPulsado;

	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= (~FLAG_DIGITO_PULSADO);
	piUnlock(SYSTEM_KEY);

	if (l_digitosGuardados == 0) {
		if (p_sistema->reloj.hora.formato == 12) {
			ultimoDigito = MIN(1, ultimoDigito);
		} else {
			ultimoDigito = MIN(2, ultimoDigito);
		}
		l_tempTime = l_tempTime*10 + ultimoDigito;
		l_digitosGuardados++;

	} else if (l_digitosGuardados == 1) {
		if (p_sistema->reloj.hora.formato == 12) {
			if (l_tempTime == 0) {
				ultimoDigito = MAX(1, ultimoDigito);
			} else {
				ultimoDigito = MIN(2, ultimoDigito);
			}
		} else {
			if (l_tempTime == 2) {
				ultimoDigito = MIN(3, ultimoDigito);
			}
		}
		l_tempTime = l_tempTime*10 + ultimoDigito;
		l_digitosGuardados++;
	} else {
		if (l_digitosGuardados == 2) {
			l_tempTime = l_tempTime*10 + MIN(5, ultimoDigito);
			l_digitosGuardados++;
		} else {
			l_tempTime = l_tempTime*10 + ultimoDigito;
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch &= (~FLAG_DIGITO_PULSADO);
			g_flagsCoreWatch |= FLAG_NEW_TIME_IS_READY;
			piUnlock(SYSTEM_KEY);
		}
	}

	if (l_digitosGuardados < 3) {
		if (l_tempTime > 2359) {
			l_tempTime %= 10000;
			l_tempTime = 100*MIN((int) (l_tempTime/100), 23) + MIN(
				l_tempTime%100, 59);
		}
	}

	#if VERSION <= 4
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("\r\n[SET_TIME] Nueva hora temporal %d\r\n", l_tempTime);
		fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

	#if VERSION >= 4
		piLock(STD_IO_LCD_BUFFER_KEY);

		lcdPosition(p_sistema->lcdId, 0, 0);
		lcdPrintf(p_sistema->lcdId, "            ");
		lcdPosition(p_sistema->lcdId, 0, 0);
		lcdPrintf(p_sistema->lcdId, "SET: %d", l_tempTime);

		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

	p_sistema->tempTime = l_tempTime;
	p_sistema->digitosGuardados = l_digitosGuardados;
}

void SetNewTime(fsm_t* p_this) {
	TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= (~FLAG_NEW_TIME_IS_READY);
	piUnlock(SYSTEM_KEY);
	SetHora(p_sistema->tempTime, &(p_sistema->reloj.hora));
	p_sistema->tempTime = 0;
	p_sistema->digitosGuardados = 0;
}

void ProcesaTeclaPulsada(fsm_t* p_this) {
	TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);

	TipoTecladoShared aux = GetTecladoSharedVar();
	aux.flags &= ~FLAG_TECLA_PULSADA;
	SetTecladoSharedVar(aux);

	char teclaPulsada = aux.teclaDetectada;

	piLock(SYSTEM_KEY);
	if (teclaPulsada == TECLA_RESET) {
		g_flagsCoreWatch |= FLAG_RESET;
	} else if (teclaPulsada == TECLA_SET_CANCEL_TIME){
		g_flagsCoreWatch |= FLAG_SET_CANCEL_NEW_TIME;
	} else if (EsNumero(teclaPulsada)) {
		p_sistema->digitoPulsado = teclaPulsada - 48;
		g_flagsCoreWatch |= FLAG_DIGITO_PULSADO;
	} else if (teclaPulsada == TECLA_EXIT) {
		#if VERSION <= 4
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("\r\nSaliendo del sistema...\r\n");
			fflush(stdout);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		#endif

		#if VERSION >= 4
			piLock(STD_IO_LCD_BUFFER_KEY);

			lcdClear(p_sistema->lcdId);
			lcdPosition(p_sistema->lcdId, 0, 0);
			lcdPrintf(p_sistema->lcdId, " Hasta luego");
			lcdPosition(p_sistema->lcdId, 0, 1);
			lcdPrintf(p_sistema->lcdId, " I <3 SDG2");
			delay(ESPERA_MENSAJE_MS);
			lcdClear(p_sistema->lcdId);


			piUnlock(STD_IO_LCD_BUFFER_KEY);
		#endif

		delay(ESPERA_MENSAJE_MS);
		exit(0);
	} else if ((teclaPulsada != '\n') && (teclaPulsada != '\r') && (teclaPulsada != 0xA)) {
		piLock(STD_IO_LCD_BUFFER_KEY);

		printf("Tecla desconocida\r\n");
		fflush(stdout);

		piUnlock(STD_IO_LCD_BUFFER_KEY);
		#if VERSION >= 4

				piLock(STD_IO_LCD_BUFFER_KEY);

				lcdPosition(p_sistema->lcdId, 0, 0);
				lcdPrintf(p_sistema->lcdId, "            ");
				lcdPosition(p_sistema->lcdId, 0, 0);
				lcdPrintf(p_sistema->lcdId, "Desconocido");
				delay(ESPERA_MENSAJE_MS);
				lcdPosition(p_sistema->lcdId, 0, 0);
				lcdPrintf(p_sistema->lcdId, "            ");

				piUnlock(STD_IO_LCD_BUFFER_KEY);

		#endif
	}
	piUnlock(SYSTEM_KEY);
}

//------------------------------------------------------
// MAIN
//------------------------------------------------------
int main() {
	unsigned int next;

 	int a1 = ConfiguraInicializaSistema(&g_coreWatch);
	if (a1 != 0) {
		printf("Error!\r\n");
		exit(0);
	}

	fsm_t* fsmCoreWatch = fsm_new(START, fsmTransCoreWatch, &g_coreWatch);
	fsm_t* fsmReloj = fsm_new(WAIT_TIC, g_fsmTransReloj, &(g_coreWatch.reloj));
	fsm_t* deteccionComandosFSM = fsm_new(WAIT_COMMAND,fsmTransDeteccionComandos,&(g_coreWatch));
	fsm_t* tecladoFSM = fsm_new(TECLADO_ESPERA_COLUMNA, g_fsmTransExcitacionColumnas, &(g_coreWatch.teclado));

	next = millis();
	while (1) {
		fsm_fire(fsmCoreWatch);
		fsm_fire(fsmReloj);
		fsm_fire(deteccionComandosFSM);
		fsm_fire(tecladoFSM);
		next += CLK_MS;
		DelayUntil(next);
	}

	tmr_destroy(g_coreWatch.reloj.tmrTic);
	tmr_destroy(g_coreWatch.teclado.tmr_duracion_columna);

	fsm_destroy(fsmCoreWatch);
	fsm_destroy(fsmReloj);
	fsm_destroy(deteccionComandosFSM);
	fsm_destroy(tecladoFSM);
}
