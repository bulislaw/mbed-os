# Platform BSP using CMSIS-Pack in Mbed OS

# Table of contents

1. [Mbed OS design document](#mbed-os-design-document)
1. [Table of contents](#table-of-contents)
1. [Introduction](#introduction)
    1. [Overview and background](#overview-and-background)
    1. [Requirements and assumptions](#requirements-and-assumptions)
1. [System architecture and high-level design](#system-architecture-and-high-level-design)
    1. [System architecture and component interaction](#system-architecture-and-component-interaction)
1. [Detailed design](#detailed-design)
1. [Usage scenarios and examples](#usage-scenarios-and-examples)
1. [Tools and configuration changes](#tools-and-configuration-changes)
1. [Contributors](#contributors)

### Revision history

0.1 - Initial thoughts based on various prior conversation and reflections - Bartek Szatkowski <bartek.szatkowski@arm.com> - 9.01.2020

# Introduction

### Overview and background

Board support package (BSP) can be understood as many different things depending on context or purpose, in this document I'll focus exclusively on Mbed OS.

BSP provides all the software components and configuration necessary to boot the hardware, run the OS and user application. This can include:

* SOC configuration (clocks, pin out, etc)
* Memory map (linker file and configuration)
* Low level hardware setup
* Mbed OS specific configuration
* Peripheral drivers (and Mbed HAL implementation)
* Libraries
* Other drivers (eg. comms stack)
* Docs/examples/misc

Currently in Mbed OS the concept of BSP is synonymous with a development board support. Although abstraction for SOC family and SOC exist, they are primarily used to enable standard development boards. Modules and custom hardware support is possible, but it's an afterthought and it's not straightforward.

For all levels of hardware support the integration process, requirement and interfaces are not very well defined. Although Mbed OS provides a hardware abstraction layer (HAL) its scope is limited to peripheral drivers. Partners and users wishing to port their hardware to Mbed OS have to mostly rely on examples of already integrated platforms and extensively use help from Mbed teams.

As it stands hardware definition and support in Mbed OS consist of:

* Porting guide

  It provides general information and guidance through the porting process. It's focusing on HAL porting and leaves a lot of lower level details to the user.

* Target definitions and configuration

  Information are split between multiple places without clear rules and the correct location is dictated by what works, rather than definition. They also are often out of sync. Some of the places where bits of information can be found:

  * Online portal - various platform details, supported Mbed versions, HW detect codes
  * `targets/targets.json` file - master record for configuration from the OS point of view. All the families of SOCs, SOCs and development boards are defined there. It also carries a good portion of OS and HW configuration including detect codes and supported Mbed versions.
  * Source files and headers - Mbed OS doesn't clearly specify how targets are supposed to integrate low level initialisation and configuration. Also it's not clearly defined what kind of information should be present in `targets.json` and what should be defined in header files.
  * Additional configuration in tools. For example detect codes are repeated in `mbed-ls`.
  * CMSIS-Packs - Mbed OS has a cache of target definitions extracted from CMSIS-Packs, it's mostly used for memory layout and configuration.
  * Linker script - each target has its own linker script (for each toolchain). There's a template in the porting guide, but it's not very clear how and where all the values and addresses should be defined.

* HAL APIs

  Hardware abstraction layer for peripheral drivers. We started updates to the specification, APIs and tests, which is not yet finished.

* Porting tests (including FPGA test shield)

Mbed OS doesn't treat custom hardware support in any special way, except providing an option to overlay external, out of tree, board support. There's no clean and generic way of bundling and configuring additional drivers that may be needed (eg. For comms stacks). It's unclear how user should progress from defining SOC configuration using silicon vendor tools (like STM CubeMX) to board port that can be integrated with Mbed OS.

During module porting work our partners and internal teams highlighted that hardware (including custom designs) support needs more focus. This was also highlighted as top issue during the recent community survey.

### Requirements and assumptions

Hardware support is critical part of any OS. Well defined and generic but simple integration layer is a key to enabling rich varieties of existing hardware and setting users for success.

Looking at the hardware available on the market and using past customer and partner interactions we can distinguish multiple levels of hardware targets:

* Processor architecture - Current scope limited to Arm (v6, v7 and v8).
* Architecture profile or family - Mbed OS is focused on Cortex-M processors, but there's limited support for Cortex-A present.
* Core - Specific implementation of architecture and profile, eg. Cortex-M4.
* SoC family - Collection of related system-on-chip designs from a vendor. It can be build on one or more MCU cores, which don't need to be of the same type or even architecture.
* SoC - A physical chip, contains one or more MCU cores, buses, various memories, timers, caches and other peripherals.
* External peripheral - They are not part of the hierarchy outlined above, but they are important building blocks of the final design. It can be any piece of hardware external to the SoC, usually requiring a driver or a configuration, eg. external memories, clock sources, modems.
* Board - From the OS point of view it's a usable physical hardware, it can be either development board or a final product. A board is a collection of one or more SoCs and zero or more external peripherals.

# System architecture and high-level design



# Detailed design



# Tools and configuration changes



# Contributors

Graham Hammond, Sophie Williams, Bartek Szatkowski, Arek Załuski