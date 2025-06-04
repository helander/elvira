#include <stdint.h>
#include "common/types.h"
#include "set.h"

extern Set engine_ports;

extern void pre_run_audio_input(EnginePort *port,  uint64_t frame, float denom,
                                uint64_t n_samples);
extern void post_run_audio_input(EnginePort *port);
extern void pre_run_audio_output(EnginePort *port,  uint64_t frame, float denom,
                                 uint64_t n_samples);
extern void post_run_audio_output(EnginePort *port);
extern void pre_run_control_input(EnginePort *port, uint64_t frame, float denom,
                                  uint64_t n_samples);
extern void post_run_control_input(EnginePort *port);
extern void pre_run_control_output(EnginePort *port, uint64_t frame, float denom,
                                   uint64_t n_samples);
extern void post_run_control_output(EnginePort *port);
