#include "reloj.h"
#include "ent2004cfConfig.h"

#define min(X,Y) ((X) < (Y) ? (X) : (Y))
#define max(X,Y) ((X) > (Y) ? (X) : (Y))

static TipoRelojShared g_relojSharedVars;

fsm_trans_t g_fsmTransReloj[] = {
    {WAIT_TIC, CompruebaTic, WAIT_TIC, ActualizaReloj},
    {-1, NULL, -1, NULL}
};

void ResetReloj(TipoReloj *p_reloj) {
    TipoCalendario calendario = {DEFAULT_DAY, DEFAULT_MONTH, DEFAULT_YEAR};
    p_reloj->calendario = calendario;
    TipoHora hora = {DEFAULT_HOUR, DEFAULT_MIN, DEFAULT_SEC, DEFAULT_TIME_FORMAT};
    p_reloj->hora = hora;
    p_reloj->timestamp = 0;

    piLock(RELOJ_KEY);
    g_relojSharedVars.flags = 0;
    piUnlock(RELOJ_KEY);
};

int ConfiguraInicializaReloj(TipoReloj *p_reloj) {
    ResetReloj(p_reloj);
    tmr_t* tmrTmp = tmr_new(tmr_actualiza_reloj_isr);
    p_reloj->tmrTic = tmrTmp;
    tmr_startms_periodic(p_reloj->tmrTic, PRECISION_RELOJ_MS);
    return 1;
}

int CompruebaTic(fsm_t *p_this){
    piLock(RELOJ_KEY);
    int result = g_relojSharedVars.flags & FLAG_ACTUALIZA_RELOJ;
    piUnlock(RELOJ_KEY);

    // prueba
    // encender: flags |= FLAG_X;
    // apagar: flags &= (~FLAG)

    return result;
};

void ActualizaReloj(fsm_t *p_this){
    TipoReloj *p_miReloj = (TipoReloj*) (p_this->user_data);
    // fsm_t* fsm_new (int state, fsm_trans_t* tt, void* user_data);
    p_miReloj->timestamp += 1;
    ActualizaHora(&(p_miReloj->hora));
    
    if (p_miReloj->hora.hh == 0 && p_miReloj->hora.mm == 0 && p_miReloj->hora.ss == 0) {
        ActualizaFecha(&(p_miReloj->calendario));
    }

    piLock(RELOJ_KEY);
    g_relojSharedVars.flags &= (~FLAG_ACTUALIZA_RELOJ);
    g_relojSharedVars.flags |= FLAG_TIME_ACTUALIZADO;
    piUnlock(RELOJ_KEY);
};

void ActualizaHora(TipoHora *p_hora) {
    p_hora->ss += 1;
    p_hora->ss %= 60;
    if (p_hora->ss == 0) {
        p_hora->mm += 1;
        p_hora->mm %= 60;
        if (p_hora->mm == 0) {
            p_hora->hh += 1;
            if (p_hora->formato == TIME_FORMAT_12_H) {
                p_hora->hh %= (p_hora->formato + 1);
            } else {
                p_hora->hh %= p_hora->formato;
            }
        }
    }
}

void ActualizaFecha(TipoCalendario *p_fecha) {
    int diasMes = CalculaDiasMes(p_fecha->MM, p_fecha->yyyy);
    p_fecha->dd += 1;
    p_fecha->dd %= (diasMes + 1);
    int temp = p_fecha->dd;
    p_fecha->dd = MAX(temp, 1);
    if (p_fecha->dd == 1) {
        p_fecha->MM += 1;
        p_fecha->MM %= (MAXMONTH + 1);
        p_fecha->MM = MAX(p_fecha->MM, 1);
        if (p_fecha->MM == 1) {
            p_fecha->yyyy += 1;
        }
    }
}

int CalculaDiasMes(int month, int year) {
    int arr[12] = { 31, 28 + EsBisiesto(year), 31, 30, 31, 30,
                    31, 31, 30, 31, 30, 31};
    return arr[month - 1];
}

int EsBisiesto(int year) {
    if( ( (year%4 == 0) && (year%100 !=0) ) || (year%400 == 0)) {  
        return 1;  
    } else {  
        return 0;
    } 
}

int SetHora(int horaInt, TipoHora *p_hora) {
    if (horaInt < 0) {
        return 1;
    } else {
        int horaExtraida =  min(horaInt/100, MAX_HOUR);
        int minutosExtraidos = min(horaInt%100, MAX_MIN);

        if (p_hora->formato == TIME_FORMAT_12_H) {
            horaExtraida = abs(horaExtraida - TIME_FORMAT_12_H);
        }

        p_hora->hh = horaExtraida;
        p_hora->mm = minutosExtraidos;
        p_hora->ss = 0;

        return 0;
    }
}

TipoRelojShared GetRelojSharedVar() {
    piLock(RELOJ_KEY);
    TipoRelojShared copia = g_relojSharedVars;
    piUnlock(RELOJ_KEY);
    return copia;
}

void SetRelojSharedVar(TipoRelojShared value){
    piLock(RELOJ_KEY);
    g_relojSharedVars = value;
    piUnlock(RELOJ_KEY);
};

void tmr_actualiza_reloj_isr(union sigval value) {
    piLock(RELOJ_KEY);
    g_relojSharedVars.flags |= FLAG_ACTUALIZA_RELOJ;
    piUnlock(RELOJ_KEY);
}