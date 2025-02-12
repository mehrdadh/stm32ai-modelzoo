## __Sensing Getting Start Package__

This project provides an STM32 Microcontroler embedded real time environement to execute [X-CUBE-AI](https://www.st.com/en/embedded-software/x-cube-ai.html) generated model targetting sensing application

### __Keywords__

Getting Start, Model Zoo, Sensing, X-CUBE-AI

### __Directory contents__

This repository is structured as follows:

| Directory                                                              | Content                                                   |
|:---------------------------------------------------------------------- |:--------------------------------------------------------- |
| Projects\STM32L4R9ZI-STWIN\Applications\GetStart\STM32CubeIDE         | IDE project files                                          |
| Projects\STM32L4R9ZI-STWIN\Applications\GetStart                       | Getting start application                                 |
| Projects\STM32L4R9ZI-STWIN\Applications\GetStart\X-Cube-AI             | *Place holder* for AI model                             |
| Projects\STM32L4R9ZI-STWIN\Applications\GetStart\DPU                   | Digital processing units                                  |
| Projects\STM32L4R9ZI-STWIN\Applications\GetStart\SensorManager         | Sensor manager                                            |
| Projects\STM32L4R9ZI-STWIN\Applications\GetStart\signal_processing_lib | Signal processing utilities                               |
| Projects\STM32L4R9ZI-STWIN\Applications\GetStart\mx                    | Hardware related application files                        |
| Drivers                                                                | Hardware drivers & base port                              |
| Middlewares\ST\eLooM                                                   | Application Framework                                     |
| Middlewares\ST\STM32_AI_Library                                        | *Place holder* for AI runtime library                   |
| Middlewares\ST\Third_Party\FreeRTOS                                    | Real time operating system                                |

### __Hardware and Software environment__

This example runs on [STEVAL-STWINKT1B](https://www.st.com/en/evaluation-tools/steval-stwinkt1b.html)

#### __STM32CubeIDE tool installation__

The STM32CubeIDE tool is required to manage an STM32 AI C-project. It allows to install in a simple way, the requested tools to compile, build and flash a firmware on a STM32 development board.

Download [STM32CubeIDE](https://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-ides/stm32cubeide.html), extract the package and execute the installer.

#### __X-CUBE-AI tool installation__

[X-CUBE-AI](https://www.st.com/en/embedded-software/x-cube-ai.html) is an STM32Cube Expansion Package, which is part of the STM32Cube.AI ecosystem. It extends STM32CubeMX capabilities with automatic conversion of pretrained artificial intelligence algorithms, including neural network and classical machine learning models. It integrates also a generated optimized library into the user's project.

This software is tested with [X-CUBE-AI](https://www.st.com/en/embedded-software/x-cube-ai.html) `v7.3.0`. It is advised that the user uses the same version to avoid any potential compaitability issues.

The pack can be installed through [STM32CubeMX](https://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-configurators-and-code-generators/stm32cubemx.html) through the  *STM32CubeExpansion* pack mechanism.

#### __Installation of the X-CUBE-AI runtime__

Please copy

```bash
<X_CUBE_AI-directory-path>
  \- Middlewares/ST/AI/Inc/*.h
```

into the middleware project directory `'<getting-start-install-dir>/Middlewares/ST/STM32_AI_Library/Inc'`

and

```bash
<X_CUBE_AI-directory-path>
  \- Middlewares/ST/AI/Lib/NetworkRuntime730_CM4_GCC.a
```

into the middleware project directory `'<getting-start-install-dir>/Middlewares/ST/STM32_AI_Library/Lib'`

#### __Generation and Installation of the X-CUBE-AI model__

This package does not provides the AI model generated by [X-CUBE-AI](https://www.st.com/en/embedded-software/x-cube-ai.html).
The user needs to generate the AI model either using the *GUI* (*G*raphical *U*ser *I*nterface) or the *CLI* (*C*ommand *L*ine *I*nterface).

The Package does not support multiple network, hence please make sure to generate a unique AI network with its default name *network*:

With the *GUI*

![single network configuration](_htmresc/XcubeAisingleNetwork.png)

With the *CLI*, just do *not* use the option ```-n/--name```

After generating the network, please copy the resulting following files:

```bash
<output-directory-path>
  \- App/network*
```

into the project directory `'<getting-start-install-dir>/Projects/STM32L4R9ZI-STWIN/Applications/GetStart/X-CUBE-AI/App'`

### __How to use it?__

The purpose of this package is to stream physical data acquired by sensors into a processing chain including a preprocessing step that typically would preform a first level of feature extraction, the machine learning inference itself, and a post processing step before exposing the results to the user in real time.

The getting start application consists of 3 phases :

1. Configuration
2. Execution
3. Real Time synthesis

#### __Configuration__

The user has the possibility to override the default configuration by altering the user configuration header file, `'<getting-start-install-dir>/Projects/STM32L4R9ZI-STWIN/Applications/GetStart/Inc/ai_model_config.h'`.

In this file, you first can describe the number and the nature of the model output:

```C
#define CTRL_X_CUBE_AI_MODE_NB_OUTPUT          (1U) /* or (2U)*/
#define CTRL_X_CUBE_AI_MODE_OUTPUT_1           (CTRL_AI_CLASS_DISTRIBUTION)
```

This version supports one or two output, and each output can be:

```C
#define CTRL_AI_CLASS_DISTRIBUTION (1U)
#define CTRL_AI_CLASS_IDX          (2U)
```

Then you describe the class indexes and their labels in this way:

```C
#define CTRL_X_CUBE_AI_MODE_CLASS_NUMBER       (4U)
#define CTRL_X_CUBE_AI_MODE_CLASS_LIST         {"Stationary","Walking","Jogging","Biking"}
```

The rest of the model details will be embedded in the `.c` and `.h` files generated by the tool [X-CUBE-AI](https://www.st.com/en/embedded-software/x-cube-ai.html). see section

You can choose to apply a available pre-processing from

```C
#define CTRL_AI_GRAV_ROT_SUPPR     (3U)
#define CTRL_AI_GRAV_ROT           (4U)
#define CTRL_AI_BYPASS             (5U)```
```

by defining:

```C
#define CTRL_X_CUBE_AI_PREPROC                 (CTRL_AI_GRAV_ROT_SUPPR)
```

You will now describe the sensor that will connect to the AI processing chain:

```C
#define CTRL_X_CUBE_AI_SENSOR_TYPE             (COM_TYPE_ACC)
#define CTRL_X_CUBE_AI_SENSOR_ODR              (26.0F)
#define CTRL_X_CUBE_AI_SENSOR_FS               (4.0F)
#define CTRL_X_CUBE_AI_NB_SAMPLES              (20U)
```

Today , only the 3D acceloremeter type is available , but you can vary the *F*ull *S*cale (*FS*) parameter given in G and the *O*utput *D*ata *R*ate (*ODR*).


This parameters needs to be consistent with the model topology that will be executed
 you can define the number of output 
You can find Herebelow a typical configuration for HAR Deep Neural Network:

```C
#define CTRL_X_CUBE_AI_MODE_NB_OUTPUT          (1U)
#define CTRL_X_CUBE_AI_MODE_OUTPUT_1           CTRL_AI_CLASS_DISTRIBUTION
#define CTRL_X_CUBE_AI_MODE_CLASS_NUMBER       (4U)
#define CTRL_X_CUBE_AI_MODE_CLASS_LIST         {"Jogging","Stationary","Stairs","Walking"}
#define CTRL_X_CUBE_AI_SENSOR_TYPE             (COM_TYPE_ACC)
#define CTRL_X_CUBE_AI_SENSOR_ODR              (26.0F)
#define CTRL_X_CUBE_AI_SENSOR_FS               (4.0F)
#define CTRL_X_CUBE_AI_NB_SAMPLES              (0U)  // or number of signals you want to run inference for
#define CTRL_X_CUBE_AI_PREPROC                 (CTRL_AI_GRAV_ROT_SUPPR)
```

During this phase the AI model is loaded, and the hardware is set up.
A sensor among the available ones is configured.

The package includes a project executing a controller task that is configurable 
The Application itself is implemented in :

```bash
<getting-start-install-dir>
  |- Middlewares/ST/eLooM/*
  |- Projects/STM32L4R9ZI-STWIN/Applications/GetStart/Inc/*
  \- Projects/STM32L4R9ZI-STWIN/Applications/GetStart/Src/*
```

The application is configurable through the following header file `'<getting-start-install-dir>/Projects/STM32L4R9ZI-STWIN/Applications/GetStart/Inc/config_user.h'`

throught the file  all needed drivers to support STMWIN1B boards (), but also

```bash
<getting-start-install-dir>
        |- _htmlresc/*
        |- Drivers/*
        |- Middlewares/*
        |- Project/*
        |- README.md
        \- LICENSE

```

To orchester these various steps properly in sequence a real time Operating System is used (FreeRTOS), on top of which an application framework is added composed of the following modules:

- *eLooM*, the application framework
- The *S*ensor *M*anager (*SM*) to configure and get raw data from the sensors on the board.
- A *D*igital *P*rocessing *U*nit (*DPU*) to process data. 

These various components can be found :

```bash
<getting-start-install-dir>
  |- Middlewares/Third_Party/FreeRTOS/*
  |- Middlewares/ST/eLooM/*
  |- Projects/STM32L4R9ZI-STWIN/Applications/GetStart/DPU/*
  \- Projects/STM32L4R9ZI-STWIN/Applications/GetStart/SensorManager/*
```

More details can be found in [ST wiki](https://wiki.st.com/stm32mcu/wiki/AI:FP-AI-MONITOR1_an_introduction_to_the_technology_behind).

#### __STM32CubeIDE tool launch__

You can find the STM32 Cube IDE project here :
`'<getting-start-install-dir>/Projects/STM32L4R9ZI-STWIN/Applications/GetStart/STM32CubeIDE/.project'` It can be launched after [STM32CubeIDE](https://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-ides/stm32cubeide.html) is installed by a double click on the file name in windows file explorer for instance.

### __History__

#### __V0.1 Initial version__

- Includes sensor capture and pre-processing
- Outputs results on STLink VCom with inference time, class and confidence...
- Based on FP-AI-MONITOR technology eLoom and sensor manager but simplified
- Limited to CubeIDE / arm gcc toolchain
- Manageable through CubeIDE (open, modification, debug)
- STWIN1B Support
- Tested with HAR IGN, SVC & GMP models
