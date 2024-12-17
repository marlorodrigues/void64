#ifndef UTILS_H
#define UTILS_H

#include <libdragon.h>

static float deltaTime = 0.0f;
static float lastTimeMs = 0.0f;

void get_delta_time() {
    // Obtém o tempo atual em milissegundos
    double nowMs = (double)TICKS_TO_MS(TICKS_READ());

    // Calcula o deltaTime antes de atualizar lastTimeMs
    deltaTime = (float)(nowMs - lastTimeMs);

    // Atualiza lastTimeMs com o tempo atual para o próximo cálculo
    lastTimeMs = nowMs;

}

float get_time_s()
{
	return (float)((double)get_ticks_us() / 1000000.0);
}


#endif
