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

### ToDo (temp)

* Finish introduction and architecture of proposed solution

* Seek review
* Work with teams on detailed design
* Think where's the line between overview/requirements and a solution. Move things accordingly. 

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

Hardware support is critical part of any OS. Well defined and generic but simple integration layer is a key to enabling rich varieties of existing hardware and setting users for success. We'll look at couple of different aspects of hardware integration in following chapters.

#### Hardware hierarchy

Looking at the hardware available on the market and using past customer and partner interactions we can distinguish multiple levels of hardware targets integration:

* Processor architecture - Current scope limited to Arm (v6, v7 and v8). Supported mainly through CMSIS-Core and toolchains integration.

* Architecture profile or family - Mbed OS is focused on Cortex-M processors, but there's limited support for Cortex-A present. Supported mainly through CMSIS-Core and toolchains integration.

* Core - Specific implementation of architecture and profile, eg. Cortex-M4. Supported mainly through CMSIS-Core and toolchains integration.

* SoC family - Collection of related system-on-chip designs from a vendor. It can be build on one or more MCU cores, which don't need to be of the same type or even architecture. Will typically include multiple peripheral hardware blocks. Support added by the vendor.

* SoC - A physical chip, contains one or more MCU cores, buses, various memories, timers, caches and other peripherals. Will typically include multiple peripheral hardware blocks. Support added by the vendor.

* Peripheral - They are not part of the hierarchy outlined above, but they are important building blocks. A peripheral would be any piece of additional hardware providing some functionality and requiring software and/or configuration to function. We can divide the peripherals in two groups:

  *  internal - built into the core, SoC family or SoC. Common examples include MPU, timers, GPIO, SPI, I2C.
  *  external - on a separate die included into a board design. Common examples include external flash, connectivity modules, GPS.

  In principle there's no differences between the two groups and most of the peripherals could be implemented in both ways. The division is still useful to illustrate where the hardware block fits into the design and what implication on the overall system it may have. External peripherals are often more complicated and would require using one of the internal peripheral to access them. Drivers can come from  Mbed OS, silicon vendor or third party contributors.

* Board - From the OS point of view it's a usable physical hardware, it can be either development board or a final product. A board is a collection of one or more SoCs and zero or more external peripherals. Development boards would be supported by their vendor, custom designs would usually live outside of the OS tree.

The hierarchy described above can be approximated using UML class diagram:

 ![HW Support hierarchy diagram](res/hardware_support_class.png)

Worth adding that the OS support for hardware should be quite flexible and the above diagram is a generic illustration rather than a strict requirement. Different silicon vendors may choose to define their designs as family of SoCs, SoCs or mix of both. They may define deeper network of inheritance or fold it down into simple hierarchy with extended configuration. There is no one correct way, the optimal choice is hardware dependent and will be influenced by the hardware, tools and processes guiding design for a specific vendor.

Below is a simplified example of one of the existing targets:

![Simplified example of existing HW hardware integration](res/hardware_support_class_example.png)

#### Standard vs custom hardware

By standard hardware I mean a generic purpose SoC or a board that is available on the market for 3rd parties to purchase. It can be used either for development purposes or to base derivative designs on it. Usually it would be quite flexible and meant for wide number of deployment. In turn a custom hardware is designed with a specific use case in mind and it may not be applicable outside it. Often it's meant to be embedded in a final product. BSP for it may not be publicly available.

From the technical point of view there's not difference between a BSP for a standard and custom hardware. They should follow the same integration scheme and adhere to the same requirements. The main difference is with the targeted audience. Standard hardware is targeting general audience (community and customers) and this should be reflected by the quality of documentation and available APIs and other resources. Custom hardware on the other hand usually targets smaller set of people, often internal to a specific product team, available resources may focus on their specific needs.

While standard hardware will usually live in or be closely associated with the OS, custom hardware won't be usually considered fro upstreaming.

#### Hardware porting and abstraction layer

Good hardware integration is a foundation for a solid operating system. Due to plethora of available hardware and a high degree of differentiation between designs from different vendors problem of hardware integration and abstraction is a difficult one.

In Arm ecosystem parts of the abstraction definitions and porting process are already done for us. The Arm architecture, CMSIS-Core, toolchain and standard C library support as well as CMSIS-RTX provide a solid, well defined interface that has to be leveraged by the OS.

We can distinguish two broad categories of integration:

* Bootstrap abstraction layer (BAL) - all the code and configuration needed for the platform to boot and start executing the OS and application code
* Hardware abstraction layer - common interface for accessing various peripherals

Currently the first layer (BAL) doesn't really exist and it's up to the target to boot the os any means necessary. We need to change that, by developing clear interfaces and mechanism that can be tapped by the hardware. One of the most important actions here is to identify common functionality and code and make it part of the OS.

For both BAL and HAL we need to clearly acknowledge that we won't be support all the hardware features by providing one generic and simple API. We should also acknowledge that it is important to allow the hardware vendors to differentiate and users to use their hardware efficiently. The only way of achieving this without nightmare of infinite fragmentation is to provide a base APIs that abstract out the common functionality in a way that small differences can be mapped onto it and at the same time provide a mechanism for vendors to implement extensions that expose their specific hardware features without destroying the look and feel of the OS.

#### Porting process

#### Distribution mechanisms

#### Officially and community supported hardware

# System architecture and high-level design



# Detailed design



# Tools and configuration changes



# Contributors

Graham Hammond, Sophie Williams, Bartek Szatkowski, Arek Załuski