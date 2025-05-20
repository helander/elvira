
#include "program.h"
#include "node_data.h"
#include "node.h"

struct node_data n1,n2;

void main() {
	program_init();
        node_init(&n1);
        node_set_plugin(&n1,"http://sfztools.github.io/sfizz");
        node_set_name(&n1,"my fizz");
        node_set_samplerate(&n1,48000);
        node_set_latency(&n1,256);
        node_set_ui_show(&n1,0);
        //n1.host.start_ui = 0;
        node_start(&n1);
        node_init(&n2);
        node_set_plugin(&n2,"http://gareus.org/oss/lv2/b_synth");
        node_set_name(&n2,"my b3");
        node_set_samplerate(&n2,48000);
        node_set_latency(&n2,512);
        node_set_ui_show(&n2,0);
        //n2.host.start_ui = 0;
        node_start(&n2);
        program_gtk_main();
}
