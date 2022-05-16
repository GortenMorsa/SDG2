#ifndef COREWATCH_H_
#define COREWATCH_H_

// INCLUDES
// Propios:
#include "systemConfig.h"     // Sistema: includes, entrenadora (GPIOs, MUTEXes y entorno), setup de perifericos y otros otros.
#include "reloj.h"
#include "teclado_TL04.h"

// DEFINES Y ENUMS
enum FSM_ESTADOS_SISTEMA {
    START,
    STAND_BY,
    SET_TIME
};

enum FSM_DETECCION_COMANDOS {
	WAIT_COMMAND
};

// FLAGS FSM DEL SISTEMA CORE WATCH
#define FLAG_SETUP_DONE 0x01
#define FLAG_SET_CANCEL_NEW_TIME 0x02
#define FLAG_NEW_TIME_IS_READY 0x04
#define FLAG_DIGITO_PULSADO 0x08
#define FLAG_RESET 0x10

#define FLAG_TECLA_PULSADA 0x2

#define TECLA_RESET 0x46
#define TECLA_EXIT 0x42
#define TECLA_SET_CANCEL_TIME 0x45


// DECLARACIÓN ESTRUCTURAS
typedef struct {
    TipoReloj reloj ;
    TipoTeclado teclado ;
    int lcdId ;
    int tempTime ;
    int digitosGuardados ;
    int digitoPulsado ;
} TipoCoreWatch ;

// DECLARACIÓN VARIABLES

// DEFINICIÓN VARIABLES

#define ESPERA_MENSAJE_MS 500

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION DE LAS VARIABLES
//------------------------------------------------------

//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
void DelayUntil(unsigned int next);
int ConfiguraInicializaSistema(TipoCoreWatch *p_sistema);
int EsNumero(char value);

//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
int CompruebaDigitoPulsado(fsm_t* p_this);
int CompruebaNewTimeIsReady(fsm_t* p_this);
int CompruebaReset(fsm_t* p_this);
int CompruebaSetCancelNewTime(fsm_t* p_this);
int CompruebaSetupDone(fsm_t* p_this);
int CompruebaTeclaPulsada(fsm_t* p_this);
int CompruebaTimeActualizado(fsm_t* p_this);


//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
void CancelSetNewTime(fsm_t* p_this);
void PrepareSetNewTime(fsm_t* p_this);
void ProcesaDigitoTime(fsm_t* p_this);
void ProcesaTeclaPulsada(fsm_t* p_this);
void Reset(fsm_t* p_this);
void SetNewTime(fsm_t* p_this);
void ShowTime(fsm_t* p_this);
void Start(fsm_t* p_this);

//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------

//------------------------------------------------------
// FUNCIONES LIGADAS A THREADS ADICIONALES
//------------------------------------------------------
PI_THREAD(ThreadExploraTecladoPC);
#endif /* EAGENDA_H */
