#ifndef SOUND_GENERATOR_H_
#define SOUND_GENERATOR_H_

#ifdef __cplusplus
extern "C" {
#endif



void start_single_signale(unsigned delay);
void start_signale_series(unsigned delay, unsigned count);
void sound_off();
void short_signale();
void long_signale();
void sig_disable();



#ifdef __cplusplus
}
#endif









#endif