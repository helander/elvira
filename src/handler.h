/*
 * ============================================================================
 *  File:       handler.h
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      Function prototypes for all event loop handlers.
 *
 * ============================================================================
 */
#include <pipewire/pipewire.h>

/* ========================================================================== */
/*                               Public Macros                                */
/* ========================================================================== */

/* ========================================================================== */
/*                              Public Typedefs                               */
/* ========================================================================== */

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
extern void on_destroy(void *data);
extern void on_state_changed(void *data, enum pw_filter_state old, enum pw_filter_state state,
                             const char *error);
extern void on_io_changed(void *data, void *port_data, uint32_t id, void *area, uint32_t size);
extern void on_param_changed(void *data, void *port_data, uint32_t id, const struct spa_pod *param);
extern void on_add_buffer(void *data, void *port_data, struct pw_buffer *buffer);
extern void on_remove_buffer(void *data, void *port_data, struct pw_buffer *buffer);
extern void on_process(void *data, struct spa_io_position *position);
extern void on_drained(void *data);

extern void on_registry_global(void *object, uint32_t id, uint32_t permissions, const char *type, uint32_t version,  const struct spa_dict *props);
extern int on_metadata_property(void *object,                                                                                                                                                      
                                 uint32_t subject,                                                                                                                                                 
                                 const char *key,                                                                                                                                                  
                                 const char *type,                                                                                                                                                 
                                 const char *value);

extern int on_host_preset(struct spa_loop *loop, bool async, uint32_t seq, const void *data,
                          size_t size, void *user_data);
extern int on_host_save(struct spa_loop *loop, bool async, uint32_t seq, const void *data,
                        size_t size, void *user_data);
extern int on_host_worker(struct spa_loop *loop, bool async, uint32_t seq, const void *data,
                          size_t size, void *user_data);
extern int on_host_control_in(struct spa_loop *loop, bool async, uint32_t seq, const void *data,
                          size_t size, void *user_data);
extern int on_ui_start(struct spa_loop *loop, bool async, uint32_t seq, const void *data,
                       size_t size, void *user_data);
extern int on_port_event_atom(struct spa_loop *loop, bool async, uint32_t seq, const void *data,
                              size_t size, void *user_data);
extern int on_port_event_aseq(struct spa_loop *loop, bool async, uint32_t seq, const void *data,
                              size_t size, void *user_data);
extern int on_input_midi_event(struct spa_loop *loop, bool async, uint32_t seq, const void *data,
                              size_t size, void *user_data);
extern int on_output_midi_event(struct spa_loop *loop, bool async, uint32_t seq, const void *data,
                              size_t size, void *user_data);
