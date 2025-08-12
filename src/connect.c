/*
 * ============================================================================
 *  File:       connect.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      External interface for parameter control.
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/socket.h>
#include <errno.h>

#include <lilv/lilv.h>
#include <lv2/patch/patch.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/urid/urid.h>


#include "constants.h"
#include "runtime.h"
#include "ports.h"

#define TCP_SERVER_IP   "127.0.0.1"
#define TCP_SERVER_PORT 5555
#define BUFFER_SIZE 1024

/* ========================================================================== */
/*                               Local State                                  */
/* ========================================================================== */

static    int sockfd;
static    pthread_t recv_thread;
static    char instance_id[64];

/* ========================================================================== */
/*                              Local Functions                               */
/* ========================================================================== */

static int get_atom_port() {
   LilvNode* atom_AtomPort = lilv_new_uri(constants.world, LV2_ATOM__AtomPort);

   const LilvPlugin *plugin = host->lilvPlugin;

   uint32_t num_ports = lilv_plugin_get_num_ports(plugin);
   for (uint32_t i = 0; i < num_ports; ++i) {
      const LilvPort* port = lilv_plugin_get_port_by_index(plugin, i);
      if (lilv_port_is_a(plugin, port, atom_AtomPort)) {
         lilv_node_free(atom_AtomPort);
         return i;
      }
   }

   lilv_node_free(atom_AtomPort);
   return -1;
}

static int get_patch_port() {
   LilvNode* atom_AtomPort = lilv_new_uri(constants.world, LV2_ATOM__AtomPort);
   LilvNode* atom_supports = lilv_new_uri(constants.world, LV2_ATOM__supports);
   LilvNode* patch_Message = lilv_new_uri(constants.world, LV2_PATCH__Message);

   const LilvPlugin *plugin = host->lilvPlugin;

   uint32_t num_ports = lilv_plugin_get_num_ports(plugin);
   for (uint32_t i = 0; i < num_ports; ++i) {
      const LilvPort* port = lilv_plugin_get_port_by_index(plugin, i);
      if (lilv_port_is_a(plugin, port, atom_AtomPort)) {
         const LilvNodes* values = lilv_port_get_value(plugin, port, atom_supports);
         LILV_FOREACH(nodes, j, values) {
            const LilvNode* val = lilv_nodes_get(values, j);
            if (lilv_node_equals(val, patch_Message)) {
               lilv_node_free(atom_AtomPort);
               lilv_node_free(atom_supports);
               lilv_node_free(patch_Message);
               return i;
            }
         }
      }
   }

   lilv_node_free(atom_AtomPort);
   lilv_node_free(atom_supports);
   lilv_node_free(patch_Message);
   return -1;
}

static void set_control_input_port(const char *key, const char *value) {
   printf("\nset_control_input_port %s   %s",key,value);fflush(stdout);
   float float_value = atof(value);
   int port_index = atoi(key);
   ports_write(NULL, port_index, sizeof(float), 0U, &float_value);
}

static void set_midicc_parameter(const char *key, const char *value) {
   LilvNode* param_uri = lilv_new_uri(constants.world, key);
   LilvNode* rdf_type = lilv_new_uri(constants.world, "http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
   LilvNode* midi_parameter = lilv_new_uri(constants.world, "http://helander.network/lv2/elvira#MidiParameter");

   if (!lilv_world_ask(constants.world, param_uri, rdf_type, midi_parameter)) return;

   int port_index = get_atom_port();
   if (port_index == -1) {
      //pw_log_error("Found no port that supports atoms and midi messages");
      return;
   }
   //pw_log_debug("Midi parameter using port %d", port_index);
   int cc,val;
   sscanf(key, "%d",&cc);
   sscanf(value, "%d",&val);
   //pw_log_debug("Midi parameter using port %d cc %d  val %d", port_index,cc,val);

   LV2_Atom_Forge forge;
   uint8_t buffer[1000];
   LV2_Atom_Forge_Frame frame;

   int channel = 0;
   uint8_t msg[3] = {
      (uint8_t)(0xB0 | (channel & 0x0F)),
      cc,
      val
   };

   LV2_URID midi_event = constants_map(constants, LV2_MIDI__MidiEvent);
   lv2_atom_forge_init(&forge, &constants.map);

   lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));
   lv2_atom_forge_atom(&forge, 3, midi_event);
   lv2_atom_forge_raw(&forge, msg, 3);

   lv2_atom_forge_pop(&forge, &frame);

   ports_write(NULL, port_index, ((LV2_Atom*)buffer)->size + sizeof(LV2_Atom), constants.atom_eventTransfer, buffer);
}

static void set_patch_parameter(const char *key, const char *value) {
   LilvNode* param_uri = lilv_new_uri(constants.world, key);
   LilvNode* rdf_type = lilv_new_uri(constants.world, "http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
   LilvNode* lv2_parameter = lilv_new_uri(constants.world, "http://lv2plug.in/ns/lv2core#Parameter");

   if (!lilv_world_ask(constants.world, param_uri, rdf_type, lv2_parameter)) return;

   int port_index = get_patch_port();
   if (port_index == -1) {
      //pw_log_error("Found no port that supports atoms and patch messages");
      return;
   } 
   //pw_log_debug("Patch parameter using port %d", port_index);
   LV2_Atom_Forge forge;
   uint8_t buffer[1000];
   LV2_Atom_Forge_Frame frame;

   LV2_URID parameter_key = constants_map(constants, key);
   lv2_atom_forge_init(&forge, &constants.map);

   lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));
   lv2_atom_forge_object(&forge, &frame, 0, constants.patch_Set);
   lv2_atom_forge_key(&forge, constants.patch_property);
   lv2_atom_forge_urid(&forge, parameter_key);
   lv2_atom_forge_key(&forge, constants.patch_value);
   lv2_atom_forge_string(&forge, value, strlen(value));
   lv2_atom_forge_pop(&forge, &frame);

   ports_write(NULL, port_index, ((LV2_Atom*)buffer)->size + sizeof(LV2_Atom), constants.atom_eventTransfer, buffer);
}

static void set_any_parameter(const char *type, const char *key, const char *value) {
   printf("\nset_any_parameter %s   %s   %s",type, key,value);fflush(stdout);
    if (!strcmp(type,"control-input-port")) {
       set_control_input_port(key, value);
    } else if (!strcmp(type,"midicc-parameter")) {
       set_midicc_parameter(key, value);
    } else if (!strcmp(type,"patch-parameter")) {
       set_patch_parameter(key, value);
    }
}

typedef struct {
  char type[100];
  char key[100];
  char value[100];
} AnyParameter;

static int on_set_any_parameter(struct spa_loop *loop, bool async, uint32_t seq, const void *data, size_t size, void *user_data) {
   AnyParameter *par = (AnyParameter *) data;
   printf("\non_set_any_parameter [%s]   [%s]   [%s]| %d  %p  %p  %p",par->type, par->key,par->value,size,par->type,par->key,par->value);fflush(stdout);
   set_any_parameter(par->type,par->key,par->value);
}

static void run_set_any_parameter(char *type, char *key, char *value) {
   AnyParameter data;
   printf("\nrun_set_any_parameter %s   %s   %s | %d  %p    %p    %p",type, key,value,sizeof(data),type,key,value);fflush(stdout);
   strcpy(data.type,type);
   strcpy(data.key,key);
   strcpy(data.value,value);
   pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop), on_set_any_parameter, 0, &data, sizeof(data), false, NULL);
}

static int read_n(int fd, void *buf, size_t n) {
    size_t total = 0;
    char *p = buf;
    while (total < n) {
        ssize_t r = recv(fd, p + total, n - total, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            perror("recv");
            return -1;
        }
        if (r == 0) { // EOF
            return -1;
        }
        total += (size_t)r;
    }
    return 0;
}

static int write_n(int fd, const void *buf, size_t n) {
    size_t total = 0;
    const char *p = buf;
    while (total < n) {
        ssize_t w = send(fd, p + total, n - total, 0);
        if (w < 0) {
            if (errno == EINTR) continue;
            perror("send");
            return -1;
        }
        total += (size_t)w;
    }
    return 0;
}

static ssize_t recv_message(int fd, uint8_t **outbuf, uint32_t max_len) {
    uint32_t netlen;
    if (read_n(fd, &netlen, sizeof(netlen)) != 0) return -1;
    uint32_t len = ntohl(netlen);

    if (len == 0) {
        *outbuf = malloc(1);
        if (!*outbuf) return -1;
        (*outbuf)[0] = '\0';
        return 0;
    }

    if (len > max_len) {
        fprintf(stderr, "message too large: %u > %u\n", len, max_len);
        return -1;
    }

    uint8_t *buf = malloc(len + 1); // +1 if you want NUL for text convenience
    if (!buf) return -1;
    if (read_n(fd, buf, len) != 0) {
        free(buf);
        return -1;
    }
    buf[len] = '\0'; // safe NUL for treating as C-string if appropriate

    *outbuf = buf;
    return (ssize_t)len;
}

static int send_message(int fd, const void *payload, uint32_t len) {
    uint32_t netlen = htonl(len);
    if (write_n(fd, &netlen, sizeof(netlen)) != 0) return -1;
    if (len > 0) {
        if (write_n(fd, payload, len) != 0) return -1;
    }
    return 0;
}

static void handle_server_message(const char* msg) {
    char *xmsg = strdup(msg);
    char *type, *key, *value;
    type = strtok(xmsg,"|");
    key = strtok(NULL,"|");
    value = strtok(NULL,"|");
    fprintf(stdout, "\nUI: received raw: [%s]   size : %d", msg,strlen(msg));
    fprintf(stdout, "\nUI: received type: [%s] key: [%s]   value: [%s]", type, key, value);
    fflush(stdout);

    run_set_any_parameter(type, key, value);

    free(xmsg);
}

static void* recv_loop(void* arg) {

    while (1) {
        uint8_t *buf;
        int len = recv_message(sockfd, &buf, 1000);
        if (len < 0) { 
            perror("Receive message ");
        } else if (len > 0){
            handle_server_message((char *)buf);
        }
        free(buf);
    }
    fprintf(stdout, "UI: receive loop ended\n");
    return NULL;
}

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */

void connect_init() {
    snprintf(instance_id, sizeof(instance_id), "%p", (void*)instance_id);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(TCP_SERVER_PORT);
    inet_pton(AF_INET, TCP_SERVER_IP, &servaddr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        return;
    }

    // Start receiving thread
    pthread_create(&recv_thread, NULL, recv_loop, NULL);

    char buf[128];
    snprintf(buf, sizeof(buf), "ID:%s\n", instance_id);
    send_message(sockfd, buf, strlen(buf));


}

void connect_cleanup() {
    close(sockfd);
    pthread_cancel(recv_thread);
    pthread_join(recv_thread, NULL);
}

