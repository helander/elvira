<img src="./docs/img/elvira.svg">

# Project
*elvira* is an lv2 host for pipewire. Unlike other lv2 hosts that is implemented using jack APIs and uses the pipewire jack "emulation", the elvira application only uses native pipewire APIs.

A lot of basic functionality is in place.
In case you are missing something vital, do not hesitate to let me know.

Pipewire is a great fit for implementing an lv2 host.
A lot of functionality required for the elvira program is provided by the pipewire client API. In addition the pipewire infrastructure provides
features that can help implement various lv2 plugin related management scenarios. 

As an example, I have created a web based management interface for elvira, that automatically discovers all elvira program instances and let you perform some basic tasks towards these instances.
Note, that no time has been spent on making this page look good. The web interface application is called *elwira* (w=web).

By nature elvira is a single plugin host, very much like *jalv*. With the web control application (elwira), you get a multi plugin host based on the single plugin host (elvira).

# Building
There are very few dependencies so building on most linux systems should be pretty straight forward.
The *tools/build* script along with the CMakeLists.txt should both be rather easy to understand, but in case you have problems I can probably help.
The build script will install elvira as /usr/bin/elvira.

In the elwira folder you find a web server program that contains a simple web page to control your elvira program instances. The web server program, called elwira, is a self contained
single executable file. To build the elwira executable, you need go development tools installed and then do:
```console
foo@bar:~$ cd elwira
foo@bar:~$ go build

```

# Running
Running is simple since elvira is just a single executable file. As long as the elvira binary is in your PATH, just run elvira at your shell prompt:
```console
foo@bar:~$ elvira myname http://example.net/lv2/plugin/myplugin --showui

```
To run the elwira web server do
```console
foo@bar:~$ ./elwira/elwira

```
The elwira program prints the url of the elvira control page, so just open your browser on this url. 
The elwira program interacts with the pipewire tools on your system. The following CLI programs needs to be available:
* pw-cli
* pw-dump
* pw-metadata

In order to use elvira, pipewire must be installed on your system and then you will most likely have the required pw-* programs.

# Logs
The elvira lv2 host provides the logging feature to its plugin. Any logging performed by the plugin is sent to the pipewire log for the elvira program. By default, the log of a pipewire client
is printed on the client's stdout device.

If you start an instance in a shell using the elvira program, its stdout device is the shell terminal (if no redirection is performed).

In case you create an instance using the elwira web interface, the stdout is redirected to a file. This file is immediately marked for deletion, and will be deleted once the elvira
process terminates. The reason is to ensure not to pollute the filesystem with old "logs". Until it is deleted, its content could be acceesed using

```console
foo@bar:~$ cat /proc/<pid>/fd/1

```
where <pid> should be replaced with the process id (pid) of the elvira process.

All log entries coming from the plugin instance are tagged with "lv2.plugin", in order to easily be able to extract that specific type of information from the pipewire log.

# Setting control input port values
Each plugin may have a number of control input ports (these are not exposed as any kind of pipewire port), determined by the plugin. You have the ability
to assign values to these ports using the following command:

```console
foo@bar:~$ pw-metadata -- <node id> control.in.<port index> <float value>

```

The "--" allows you to have negative float values.

Example command:

```console
foo@bar:~$ pw-metadata -- 84 control.in.4 -6.4

```

This will assign the value -6.4 to the control input port with index 4 on the elvira instance with pipewire node id 84.

# Connections ?
For managing connections (links) between elvira instances and the pipewire infrastructre I use *qpwgraph*, an excellent tool.

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
Luckily, for many cases it is enough to manage this via a shell command, to assign values to control ports.

The *pipewire Control ports* are similar in nature to the *lv2 Atom ports*. This is especially true for carrying of *Midi* messages.

# Principle of operation
```mermaid
sequenceDiagram
    actor rt_thread as RT thread<br/>(pw managed)
    participant ports as Ports<br/>(elvira module)
    participant handlers as Handlers<br/>(elvira module)
    participant plugin as plugin<br/>(lv2 instance)
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
```
This diagram shows the core operation of the elvira program. Assisted by the pipewire infrastructure a realtime thread cyclically calls elvira's on_process handler. The handler
copies data from pipewire input ports to the plugin's input ports. Then the plugin's run method is called. When run is done, the handler copies data from the plugin's output ports to pipewire
output ports. This process is then repeated over and over again at regular time intervals.

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

