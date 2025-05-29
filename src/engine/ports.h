#include "types.h"

extern void ports_init(Engine* engine);
extern void ports_write_port(void* const controller, const uint32_t port_index,
                             const uint32_t buffer_size, const uint32_t protocol,
                             const void* const buffer);

extern void pre_run_audio_input(EnginePort *port, Engine *engine, uint64_t frame, float denom, uint64_t n_samples);
extern void post_run_audio_input(EnginePort *port, Engine *engine);
extern void pre_run_audio_output(EnginePort *port, Engine *engine, uint64_t frame, float denom, uint64_t n_samples); 
extern void post_run_audio_output(EnginePort *port, Engine *engine);
extern void pre_run_control_input(EnginePort *port, Engine *engine, uint64_t frame, float denom, uint64_t n_samples);
extern void post_run_control_input(EnginePort *port, Engine *engine);
extern void pre_run_control_output(EnginePort *port, Engine *engine, uint64_t frame, float denom, uint64_t n_samples);
extern void post_run_control_output(EnginePort *port, Engine *engine);
