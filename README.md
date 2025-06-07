<img src="./docs/img/elvira.svg">

# Project
*elvira* is an lv2 host for pipewire. Unlike other lv2 hosts that is implemented using jack APIs and uses the pipewire jack "emulation", the elvira application only uses native pipewire APIs.

A lot of basic functionality is in place.
In case you are missing something vital, do not hesitate to let me know.

Pipewire is a great fit for implementing an lv2 host.
A lot of functionality required for the elvira program is provided by the pipewire client API. In addition the pipewire infrastructure provides
features that can help implement various lv2 plugin related management scenarios. 
As an example, I have created a web based management interface for elvira, that automatically discovers all elvira program instances and let you perform some basic tasks towards these instances.
This web application will soon be made available via this page.

# Building
There are very few dependencies so building on most linux systems should be pretty straight forward.
The *tools/build* script along with the CMakeLists.txt should both be rather easy to understand, but in case you have problems I can probably help.
The build script will install elvira as /usr/bin/elvira.

# Running
Running is simple since elvira is just a single executable file. As long as the elvira binary is in your PATH, just run elvira at your shell prompt:
```console
foo@bar:~$ elvira myname http://example.net/lv2/plugin/myplugin --showui

```

In case you are on a system with systemd, you can use the script at systemd/run_elvira_service.sh (same command line parameters as the elvira program). This will start the elvira program as a
systemd service and disconnected from your shell i/o. 
```console
foo@bar:~$ ./systemd/run_elvira_service.sh myname http://example.net/lv2/plugin/myplugin --showui

```

You can see the stdout/stderr of the service using:
```console
foo@bar:~$ systemctl --user status elvira.myname

```

In case you need to see more output than provided with above command, please see the journalctl documentation.

# LV2_PATH
The LV2_PATH environment variable control where in the filesystem various lv2 related libraries looks for resources (e.g. plugins, presets). 
For elvira to work it is important that this variable is properly set. In case it is unset, elvira will set it to a value corresponding
to the following setting:
```console
foo@bar:~$ export LV2_PATH=$HOME/.lv2:/usr/lib/lv2

```

The elvira function to save a preset of its current state, will save the preset information at $HOME/.lv2 . 
In order for the saved preset to be used, it is vital that this location is part of LV2_PATH.

# Basic terminology
An elvira program contains one single lv2 plugin instance. The elvira program calls the plugin instance and its associated resources an *lv2 host*. In addition to the *lv2 host*, the program also contains a *pipewire node*.
This node represents the required mechanisms and resources for integration into a *pipwire* infrastructure.
The *lv2 host* represents all required mechanisms and resources for creation of an lv2 plugin instance.

An *lv2 host* has ports for connecting between the internals of the plugin and the outside world. Likewise a *pipewire node* has ports to connect to its outer world. 

Pipewire ports makes it possible to connect an Engines inputs and outputs to/from other pipewire nodes. 

*elvira* performs mapping between the *lv2 host* ports and the *pipewire* node ports.

# Port mapping
One of the key aspects of hosting an lv2 host function in a pipewire environment,
is the mapping between the lv2 ports of the plugin instances and the ports of the pipewire nodes.
On some type of ports, namely *Audio ports*, this mapping is very easy to achieve,
mostly due to similar semantics and implementation strategies. 

For other ports, like the *lv2 Control ports*, there is really no correspondence to any pipewire port type.
Luckily, for many cases it is enough to manage this via the command mention above, to assign values to control ports.

The *pipewire Control ports* are similar in nature to the *lv2 Atom ports*. This is especially true for carrying of *Midi* messages.

# Software architecture
The following diagram exposes a number of different aspects of the elvira software architecture. More detailed information is available in the code itself. A lot of the functionality of the elvira implementation comes from the used components, such as pipewire, lv2, lilv, suil, etc. The functionality of these componenents is documented elsewhere.

```mermaid
sequenceDiagram
    actor main as main<br/>(program)
    actor rt_thread as RT thread<br/>(pw managed)
    participant host as Host<br/>(elvira module)
    participant node as Node<br/>(elvira module)
    participant ports as Ports<br/>(elvira module)
    participant runtime as Runtime<br/>(elvira module)
    participant handlers as Handlers<br/>(elvira module)
    participant gtk as Gtk API<br/>(library)
    participant pw as Pipewire API<br/>(library)
    participant lilv as lilv API<br/>(library)
    participant suil as suil API<br/>(library)

    main-->+gtk: gtk_init
    gtk-->-main: 
    main-->+pw: pw_init
    pw-->-main: 

    main-->+runtime: runtime_init
    create actor primary as primary loop<br/>(pw event loop)
    runtime->primary: create
    create actor worker as worker loop<br/>(pw event loop)
    runtime->worker: create
    runtime-->-main: 
 
    main->primary: start
    main->worker: start

    main-->+host: host_setup
    host-->+lilv: load plugin
    lilv-->-host: 
    host-->+lilv: instantiate plugin
    create participant plugin as plugin<br/>(lv2 instance)
    lilv->plugin: create


    lilv-->-host: 
    host-->+lilv: discover plugin ports
    lilv-->-host: 
    host-->-main: 

    main-->+node: node_setup
    node-->+pw: create pw node
    pw-->-node: 
    node-->+pw: create pw ports
    pw-->-node: 
    node-->-main: 

    main-->+ports: ports_setup
    ports-->+ports: create port mappings
    ports-->-ports: 
    ports-->-main: 


    main->>+lilv: lilv_instance_activate
    lilv->>-main: 

    opt Preset
      main->primary: invoke(on_host_preset)
      primary-->+handlers: on_host_preset
      handlers-->+lilv: apply preset
      lilv-->-handlers: 
      handlers-->-primary: 
    end

    opt UI
      main->primary: invoke(on_ui_start)
      primary-->+handlers: on_ui_start
      handlers-->+suil: create UI
      suil-->-handlers: 
      handlers-->+gtk: show UI
      gtk-->-handlers: 
      handlers-->-primary: 
    end

    par
      main-->+gtk: gtk_main
    and
       loop Pipewire periodic execution
          rt_thread->>+handlers: on_process
          handlers-->>+ports: copy inputs to plugin
          ports->>-handlers:  
          handlers-->>+plugin: run
          plugin->>-handlers: 
          handlers-->>+ports: copy outputs from plugin
          ports->>-handlers: 
          handlers->>-rt_thread: 
       end
    end
```

