#ifndef RELOJ_H_
#define RELOJ_H_

#include "systemConfig.h"
#include "util.h"

enum FSM_ESTADOS_RELOJ {
    WAIT_TIC
};

#define FLAG_ACTUALIZA_RELOJ 0x01
#define FLAG_TIME_ACTUALIZADO 0x02

#define DEFAULT_DAY 28
#define DEFAULT_MONTH 2
#define DEFAULT_YEAR 2020

#define DEFAULT_HOUR 0
#define DEFAULT_MIN 0
#define DEFAULT_SEC 0

#define DEFAULT_TIME_FORMAT 24
#define PRECISION_RELOJ_MS 1000

#define TIME_FORMAT_12_H 12
#define TIME_FORMAT_24_H 24

#define MAX_MIN 59
#define MAX_HOUR 23

typedef struct {
    int dd;
    int MM;
    int yyyy ;
} TipoCalendario ;

typedef struct {
    int hh;
    int mm;
    int ss;
    int formato ;
} TipoHora ;

typedef struct {
    int timestamp ;
    TipoHora hora ;
    TipoCalendario calendario ;
    tmr_t * tmrTic ;
} TipoReloj ;

typedef struct {
    int flags ;
} TipoRelojShared ;

extern fsm_trans_t g_fsmTransReloj[];

#define MAXMONTH 12
extern int DIAS_MESES[2][MAXMONTH];

int ConfiguraInicializaReloj (TipoReloj *p_reloj);
void ResetReloj(TipoReloj *p_reloj);

void ActualizaFecha(TipoCalendario *p_fecha);
void ActualizaHora(TipoHora *p_hora);
int CalculaDiasMes(int month, int year);
int EsBisiesto(int year);
TipoRelojShared GetRelojSharedVar();
int SetFecha(int nuevaFecha, TipoCalendario *p_fecha);
int SetFormato(int nuevoFOrmato, TipoHora *p_hora);
int SetHora(int nuevaHora, TipoHora *p_hora);
void SetRelojSharedVar(TipoRelojShared value);

int CompruebaTic(fsm_t *p_this);

void ActualizaReloj(fsm_t *p_this);

void tmr_actualiza_reloj_isr(union sigval value);

#define static TipoRelojShared g_relojSharedVars;
#endif
