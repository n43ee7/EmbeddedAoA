# Embedded Device AoA/TDoA approximation using Real-time DSP

## MPU Hardware Details 
- Target Hardware: Texas Instruments Tiva-C (TM123GH6PM) MPU Developer kit 
- Target MPU Description: ARM M4F Microprocessor 
- Target Hardware Peripherals: 
  - ADC0 - Sequence Sampler 
  - Analog Comparator (CoNeg -ve) 
  - GPIO PORT C - F 
  - NVIC Interrupts (ADC, Comparator) 
  - Gated Clocks - PLL @ 40 MHz 

## ASIC Hardware Details
### Hardware BoM
| Part                                      | Quantity |
|-------------------------------------------|----------|
| TM4C123G evaluation board (ARM M4F)      | 1        |
| LM2902 (quad op amp)                      | 1        |
| 2.2kΩ, 5% resistor (mic bias)            | 3 or 4   |
| 1kΩ, 5% resistor (inverting input)       | 3 or 4   |
| 10kΩ, 5% resistor (shunt input)          | 3 or 4   |
| 100kΩ, 5% resistor (amplifiers)          | 3 or 4   |
| 0.1μF capacitor (supply bypassing)        | 2 or more|
| 1μF capacitor (microphone AC coupler)    | 3 or 4   |
| 10μF capacitor (supply)                   | 1        |
| 14pin 300mil socket (op amp)             | 1        |
| 2x10 double-row header, unshrouded        | 2        |
| CMC-9745-44P microphone                   | 3 or 4   |
| Microphone holder (short)                 | 3        |
| Microphone holder (tall)                  | 1 optional|
| #4 x ¼” sheet metal screw                 | 3 or 4   |
| 80x120cm FR4 PC board                     | 1        |
### PCB Modeling
NULL

## Software
### Programming Details 
- IDE Prototyped on: Texas Instruments Code Composer Studio V15 
- Programming Target Language: C/99 
- Programming Dependencies: NONE (ALL CUSTOM DEPENDENCIES in source/include)
- Software Implementations/Algorithms:
    - Direct ADC trigger response sampling (source/D-ADC)
    - uDMA Ping-Pong Time sampling (source/uDMA) 
### User Interface Software commands
To interact with this device, a virtual COM port is established using a 115200 baud, 8N1 protocol with no hardware handshaking. Below are the supported commands:
To interact with this device, a virtual COM port is established using a 115200 baud, 8N1 protocol with no hardware handshaking. Below are the supported commands:

- `reset`: Initiates a hardware reset when received.
  
- `average`: Displays the average value (in DAC units and SPL) of each microphone.

- `level`: Updates the reference level of the comparator to determine the detection threshold.

- `backoff B`: Sets the backoff between the first and subsequent microphone signal threshold levels. If "B" is missing, returns the current setting.

- `holdoff H`: Sets the minimum time before the next event can be detected. If "H" is missing, returns the current setting.

- `hysteresis Y`: Determines the decrease in the average required after an "event" before the next event can be detected. If "Y" is missing, returns the current setting.

- `aoa`: Returns the most current angle of arrival (theta and optionally theta and phi).

- `aoa always|stop`: Controls the display of the AoA information of each "event" as it is detected.

- `tdoa ON|OFF`: Enables or disables the display of TDoA data for qualified events (when all sensors detect the signal within the possible time window) when AoA data is shown.

- `fail ON|OFF`: Enables or disables the display of partial data from the sensors when there is no qualified event (when TDoA is greater than possible or is incomplete).

- `help` : Displays the Terminal system help menu 
  
## Licensing and Usage 
The product was designed under MIT licensing. The owner of this repository claims no responsibility for using this software package. The contributor approved no commercial usage. The repository was designed solely for academic purposes, and the contributor claims no responsibility for usage against academic integrity policies if applicable.
