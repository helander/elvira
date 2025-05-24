# LWira
Pronounced “elvira”, is an lv2 host for pipewire

Origin of project name: Lv2pipeWire

elvira is so much easier to write, so in the following we will use elvira;)

This is very much work in progress. A lot of basic functionality is in place. Code is ugly and will be improved. Documentation is missing, but it will be there eventually.

Pipewire is a great fit for implementing an lv2 host.

An elvira based system contains few or many lv2 plugin instances. The instances are managed (created/deleted) by a Controller. Each plugin instance is represented by an Engine. In addition to the lv2 plugin instance, the Engine also contains a pipewire node. This node represents the required mechanisms and resources for integration into a pipwire infrastructure. The Controller contains a similar pipewire node, however currently it differs from the Engines' pipewire node by not exposing any pipewire ports. Pipewire ports makes it possible to connect an Engines inputs and outputs to/from other pipewire nodes. 

pipewire contains a CLI tool (pw-cli) which could be used to send commands to arbitrary pipewire nodes. Currently, this is used for a couple of different tasks in elvira:
* Request the controller to add a set of new Engines
* Request the controller to remove a set of existing engines
* Request an Engine to save its current state in a Preset
* Request an Engine to apply a previously saved Preset
* Assign a value to an lv2 input control port

The first two of these operations are targeted towards the Controller, and this is the reason for the Controller being a pipewire node. The remaining operations are targeted towards a specifc Engine and addressed to the Engine's pipewire node. (Some of these commands do not exist in the currently available codebase, but they will be in the near future).

<img src="./elvira.svg">
