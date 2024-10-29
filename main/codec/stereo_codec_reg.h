/*
 * Stereo audio codec controller handler
 *  WM8960 codec implemented
 */

#ifndef __STEREO_CODEC_REG_H__
#define __STEREO_CODEC_REG_H__


#define R0_LeftInputVolume 0
#define R1_RightInputVolume 1

/*
 * R2 (02h) LOUT1 volume
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8     | OUT1VU          | N/A        | Headphone Output PGA Volume Update
 *        |                 |            | Writing a 1 to this bit will cause
 *        |                 |            | left and right headphone output volumes
 *        |                 |            | to be updated (LOUT1VOL and ROUT1VOL)
 * -------+-----------------+------------+--------------------------------------------------
 *  7     | LO1ZC           | 0          | Left Headphone Output Zero Cross Enable
 *        |                 |            | 0 = Change gain immediately
 *        |                 |            | 1 = Change gain on zero cross only
 * -------+-----------------+------------+--------------------------------------------------
 *  6:0   | LOUT1VOL[6:0]   | 0          | LOUT1 Volume
 *        |                 |            | 1111111 = +6dB
 *        |                 |            | ... 1dB steps down to
 *        |                 |            | 0110000 = -73dB
 *        |                 |            | 0101111 to 0000000 = Analogue MUTE
 */
#define R2_LOUT1Volume 2

/*
 * R3 (03h) ROUT1 volume
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8     | OUT1VU          | N/A        | Headphone Output PGA Volume Update
 *        |                 |            | Writing a 1 to this bit will cause
 *        |                 |            | left and right headphone output volumes
 *        |                 |            | to be updated (LOUT1VOL and ROUT1VOL)
 * -------+-----------------+------------+--------------------------------------------------
 *  7     | RO1ZC           | 0          | Right Headphone Output Zero Cross Enable
 *        |                 |            | 0 = Change gain immediately
 *        |                 |            | 1 = Change gain on zero cross only
 * -------+-----------------+------------+--------------------------------------------------
 *  6:0   | ROUT1VOL[6:0]   | 0          | ROUT1 Volume
 *        |                 |            | 1111111 = +6dB
 *        |                 |            | ... 1dB steps down to
 *        |                 |            | 0110000 = -73dB
 *        |                 |            | 0101111 to 0000000 = Analogue MUTE
 */
#define R3_ROUT1Volume 3

/*
 * R4 (04h) Clocking (1)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8:6   | ADCDIV[2:0]     | 000        | ADC Sample rate divider (Also determines
 *        |                 |            | ADCLRC in master mode)
 *        |                 |            | 000 = SYSCLK / (1.0 * 256)
 *        |                 |            | 001 = SYSCLK / (1.5 * 256)
 *        |                 |            | 010 = SYSCLK / (2 * 256)
 *        |                 |            | 011 = SYSCLK / (3 * 256)
 *        |                 |            | 100 = SYSCLK / (4 * 256)
 *        |                 |            | 101 = SYSCLK / (5.5 * 256)
 *        |                 |            | 110 = SYSCLK / (6 * 256)
 *        |                 |            | 111 = Reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  5:3   | DACDIV[2:0]     | 000        | DAC Sample rate divider (Also determines
 *        |                 |            | DACLRC in master mode)
 *        |                 |            | 000 = SYSCLK / (1.0 * 256)
 *        |                 |            | 001 = SYSCLK / (1.5 * 256)
 *        |                 |            | 010 = SYSCLK / (2 * 256)
 *        |                 |            | 011 = SYSCLK / (3 * 256)
 *        |                 |            | 100 = SYSCLK / (4 * 256)
 *        |                 |            | 101 = SYSCLK / (5.5 * 256)
 *        |                 |            | 110 = SYSCLK / (6 * 256)
 *        |                 |            | 111 = Reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  2:1   | SYSCLKDIV[1:0]  | 00         | SYSCLK Pre-divider. Clock source (MCLK or PLL
 *        |                 |            | output) will be divided by this value to
 *        |                 |            | generate SYSCLK.
 *        |                 |            | 00 = Divide SYSCLK by 1
 *        |                 |            | 01 = Reserved
 *        |                 |            | 10 = Divide SYSCLK by 2
 *        |                 |            | 11 = Reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  0     | CLKSEL          | 0          | SYSCLK Selection
 *        |                 |            | 0 = SYSCLK derived from MCLK
 *        |                 |            | 1 = SYSCLK derived from PLL output
 */
#define R4_Clocking_1 4

/*
 * R5 (05h) ADC and DAC Control (1)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  7     | DACDIV2         | 0          | DAC 6dB Attenuate
 *        |                 |            | 0 = Disabled (0dB)
 *        |                 |            | 1 = -6dB Enabled
 * -------+-----------------+------------+--------------------------------------------------
 *  6:5   | ADCPOL[1:0]     | 00         | ADC polarity control:
 *        |                 |            | 00 = Polarity not inverted
 *        |                 |            | 01 = ADC L inverted
 *        |                 |            | 10 = ADC R inverted
 *        |                 |            | 11 = ADC L and R inverted
 * -------+-----------------+------------+--------------------------------------------------
 *  4 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  3     | DACMU           | 1          | DAC Digital Soft Mute
 *        |                 |            | 1 = Mute
 *        |                 |            | 0 = No mute (signal active)
 * -------+-----------------+------------+--------------------------------------------------
 *  2:1   | DEEMPH[1:0]     | 00         | De-emphasis Control
 *        |                 |            | 00 = No de-emphasis
 *        |                 |            | 01 = 32kHz sample rate
 *        |                 |            | 10 = 44.1kHz sample rate
 *        |                 |            | 11 = 48kHz sample rate
 * -------+-----------------+------------+--------------------------------------------------
 *  0     | ADCHPD          | 0          | ADC High Pass Filter Disable
 *        |                 |            | 0 = Enable high pass filter on
 *        |                 |            |     left and right channels
 *        |                 |            | 1 = Disable high pass filter on
 *        |                 |            |     left and right channels
 */
#define R5_ADCAndDACControl_1 5

/*
 * R6 (06h) ADC and DAC Control (2)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8:7 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  6:5   | DACPOL[1:0]     | 00         | DAC polarity control:
 *        |                 |            | 00 = Polarity not inverted
 *        |                 |            | 01 = DAC L inverted
 *        |                 |            | 10 = DAC R inverted
 *        |                 |            | 11 = DAC L and R inverted
 * -------+-----------------+------------+--------------------------------------------------
 *  4 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  3     | DACSMM          | 0          | DAC Soft Mute Mode
 *        |                 |            | 0 = Disabling soft-mute (DACMU=0) will cause
 *        |                 |            | the volume to change immediately to the
 *        |                 |            | LDACVOL / RDACVOL settings
 *        |                 |            | 1 = Disabling soft-mute (DACMU=0) will cause
 *        |                 |            | the volume to ramp up gradually to the
 *        |                 |            | LDACVOL / RDACVOL settings
 * -------+-----------------+------------+--------------------------------------------------
 *  2     | DACMR           | 0          | DAC Soft Mute Ramp Rate
 *        |                 |            | 0 = Fast ramp (24kHz at fs=48k, providing
 *        |                 |            | maximum delay of 10.7ms)
 *        |                 |            | 1 = Slow ramp (1.5kHz at fs=48k, providing
 *        |                 |            | maximum delay of 171ms)
 * -------+-----------------+------------+--------------------------------------------------
 *  1     | DACSLOPE        | 0          | Selects DAC filter characteristics
 *        |                 |            | 0 = Normal mode
 *        |                 |            | 1 = Sloping stopband
 * -------+-----------------+------------+--------------------------------------------------
 *  0 - reserved
 */
#define R6_ADCAndDACControl_2 6

/*
 * R7 (07h) Audio Interface
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8     | ALRSWAP         | 0          | Left/Right ADC Channel Swap
 *        |                 |            | 0 = Output left and right data as normal
 *        |                 |            | 1 = Swap left and right ADC data
 *        |                 |            |     in audio interface
 * -------+-----------------+------------+--------------------------------------------------
 *  7     | BCLKINV         | 0          | BCLK invert bit (for master and slave modes)
 *        |                 |            | 0 = BCLK not inverted
 *        |                 |            | 1 = BCLK inverted
 * -------+-----------------+------------+--------------------------------------------------
 *  6     | MS              | 0          | Master / Slave Mode Control
 *        |                 |            | 0 = Enable slave mode
 *        |                 |            | 1 = Enable master mode
 * -------+-----------------+------------+--------------------------------------------------
 *  5     | DLRSWAP         | 0          | Left/Right DAC Channel Swap
 *        |                 |            | 0 = Output left and right data as normal
 *        |                 |            | 1 = Swap left and right DAC data
 *        |                 |            |     in audio interface
 * -------+-----------------+------------+--------------------------------------------------
 *  4     | LRP             | 0          | Right, left and I2S modes – LRCLK polarity
 *        |                 |            | 0 = normal LRCLK polarity
 *        |                 |            | 1 = invert LRCLK polarity
 *        |                 |            |--------------------------------------------------
 *        |                 |            | DSP Mode – mode A/B select
 *        |                 |            | 0 = MSB is available on 2nd BCLK rising edge
 *        |                 |            |     after LRC rising edge (mode A)
 *        |                 |            | 1 = MSB is available on 1st BCLK rising edge
 *        |                 |            |     after LRC rising edge (mode B)
 * -------+-----------------+------------+--------------------------------------------------
 *  3:2   | WL[1:0]         | 10         | Audio Data Word Length
 *        |                 |            | 00 = 16 bits
 *        |                 |            | 01 = 20 bits
 *        |                 |            | 10 = 24 bits
 *        |                 |            | 11 = 32 bits (Right Justified mode does
 *        |                 |            |      not support 32-bit data)
 * -------+-----------------+------------+--------------------------------------------------
 *  1:0   | FORMAT[1:0]     | 10         | 00 = Right justified
 *        |                 |            | 01 = Left justified
 *        |                 |            | 10 = I2S Format
 *        |                 |            | 11 = DSP Mode
 */
#define R7_AudioInterface_1 7

#define R8_Clocking_2 8
#define R9_AudioInterface_2 9

/*
 * R10 (0Ah) Left DAC Volume
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8     | DACVU           | N/A        | DAC Volume Update
 *        |                 |            | Writing a 1 to this bit will cause
 *        |                 |            | left and right DAC volumes to be updated
 *        |                 |            | (LDACVOL and RDACVOL)
 * -------+-----------------+------------+--------------------------------------------------
 *  7:0   | LDACVOL[7:0]    | 1111 1111  | Left DAC Digital Volume Control
 *        |                 |            | 0000 0000 = Digital Mute
 *        |                 |            | 0000 0001 = -127dB
 *        |                 |            | 0000 0010 = -126.5dB
 *        |                 |            | ... 0.5dB steps up to
 *        |                 |            | 1111 1111 = 0dB
 */
#define R10_LeftDACVolume 10

/*
 * R11 (0Bh) Left DAC Volume
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8     | DACVU           | N/A        | DAC Volume Update
 *        |                 |            | Writing a 1 to this bit will cause
 *        |                 |            | left and right DAC volumes to be updated
 *        |                 |            | (LDACVOL and RDACVOL)
 * -------+-----------------+------------+--------------------------------------------------
 *  7:0   | RDACVOL[7:0]    | 1111 1111  | Right DAC Digital Volume Control
 *        |                 |            | 0000 0000 = Digital Mute
 *        |                 |            | 0000 0001 = -127dB
 *        |                 |            | 0000 0010 = -126.5dB
 *        |                 |            | ... 0.5dB steps up to
 *        |                 |            | 1111 1111 = 0dB
 */
#define R11_RightDACVolume 11

#define R12_Reserved 12
#define R13_Reserved 13
#define R14_Reserved 14

/*
 * R15 (0Fh) Reset
 * Writing to this register resets all registers to their default state
 */
#define R15_Reset 15

#define R16_3DControl 16
#define R17_ALC_1 17
#define R18_ALC_2 18
#define R19_ALC_3 19
#define R20_NoiseGate 20
#define R21_LeftADCVolume 21
#define R22_RightADCVolume 22

/*
 * R23 (17h) Additional Control (1)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8     | TSDEN           | 1          | Thermal Shutdown Enable
 *        |                 |            | 0 = Thermal shutdown disabled
 *        |                 |            | 1 = Thermal shutdown enabled
 *        |                 |            | (TSENSEN must be enabled for
 *        |                 |            | this function to work)
 * -------+-----------------+------------+--------------------------------------------------
 *  7:6   | VSEL[1:0]       | 11         | Analogue Bias Optimisation
 *        |                 |            | 00 = Reserved
 *        |                 |            | 01 = Increased bias current optimized
 *        |                 |            |      for AVDD=2.7V
 *        |                 |            | 1X = Lowest bias current, optimized
 *        |                 |            |      for AVDD=3.3V
 * -------+-----------------+------------+--------------------------------------------------
 *  5 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  4     | DMONOMIX        | 0          | DAC Mono Mix
 *        |                 |            | 0 = Stereo
 *        |                 |            | 1 = Mono (Mono MIX output on enabled DACs)
 * -------+-----------------+------------+--------------------------------------------------
 *  3:2   | DATSEL[1:0]     | 00         | ADC Data Output Select
 *        |                 |            | 00: left data = left ADC;
 *        |                 |            |     right data = right ADC
 *        |                 |            | 01: left data = left ADC;
 *        |                 |            |     right data = left ADC
 *        |                 |            | 10: left data = right ADC;
 *        |                 |            |     right data = right ADC
 *        |                 |            | 11: left data = right ADC;
 *        |                 |            |     right data = left ADC
 * -------+-----------------+------------+--------------------------------------------------
 *  1     | TOCLKSEL        | 0          | Slow Clock Select (Used for volume update
 *        |                 |            | timeouts and for jack detect debounce)
 *        |                 |            | 0 = SYSCLK / 221 (Slower Response)
 *        |                 |            | 1 = SYSCLK / 219 (Faster Response)
 * -------+-----------------+------------+--------------------------------------------------
 *  0     | TOEN            | 0          | Enables Slow Clock for Volume Update Timeout
 *        |                 |            | and Jack Detect Debounce
 *        |                 |            | 0 = Slow clock disabled
 *        |                 |            | 1 = Slow clock enabled
 */
#define R23_AdditionalControl_1 23

#define R24_AdditionalControl_2 24

/*
 * R25 (19h) Power management (1)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8:7   | VMIDSEL[1:0]    | 00         | Vmid Divider Enable and Select
 *        |                 |            | 00 = Vmid disabled (for OFF mode)
 *        |                 |            | 01 = 2 x 50kΩ divider enabled
 *        |                 |            |      (for playback/record)
 *        |                 |            | 10 = 2 x 250kΩ divider enabled
 *        |                 |            |      (for low-power standby)
 *        |                 |            | 11 = 2 x 5kΩ divider enabled (for fast start-up)
 * -------+-----------------+------------+--------------------------------------------------
 *  6     | VREF            | 0          | VREF (necessary for all other functions)
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 * -------+-----------------+------------+--------------------------------------------------
 *  5     | AINL            | 0          | Analogue in PGA Left
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 * -------+-----------------+------------+--------------------------------------------------
 *  4     | AINR            | 0          | Analogue in PGA Right
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 * -------+-----------------+------------+--------------------------------------------------
 *  3     | ADCL            | 0          | ADC Left
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 * -------+-----------------+------------+--------------------------------------------------
 *  2     | ADCR            | 0          | ADC Right
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 * -------+-----------------+------------+--------------------------------------------------
 *  1     | MICB            | 0          | MICBIAS
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 * -------+-----------------+------------+--------------------------------------------------
 *  0     | DIGENB          | 0          | Master Clock Disable
 *        |                 |            | 0 = Master clock enabled
 *        |                 |            | 1 = Master clock disabled
 */
#define R25_PowerManagment_1 25

/*
 * R26 (1Ah) Power management (2)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8     | DACL            | 0          | DAC Left
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 * -------+-----------------+------------+--------------------------------------------------
 *  7     | DACR            | 0          | DAC Right
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 * -------+-----------------+------------+--------------------------------------------------
 *  6     | LOUT1           | 0          | LOUT1 Output Buffer
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 * -------+-----------------+------------+--------------------------------------------------
 *  5     | ROUT1           | 0          | ROUT1 Output Buffer
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 * -------+-----------------+------------+--------------------------------------------------
 *  4     | SPKL            | 0          | SPK_LP/SPK_LN Output Buffers
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 * -------+-----------------+------------+--------------------------------------------------
 *  3     | SPKR            | 0          | SPK_RP/SPK_RN Output Buffers
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 * -------+-----------------+------------+--------------------------------------------------
 *  2 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  1     | OUT3            | 0          | OUT3 Output Buffer
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 * -------+-----------------+------------+--------------------------------------------------
 *  0     | PLLEN           | 0          | PLL Enable
 *        |                 |            | 0 = Power down
 *        |                 |            | 1 = Power up
 */
#define R26_PowerManagment_2 26

/*
 * R27 (1Bh) Additional Control (3)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8:7 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  6     | VROI            | 0          | VREF to Analogue Output Resistance
 *        |                 |            | (Disabled Outputs)
 *        |                 |            | 0 = 500Ω VMID to output
 *        |                 |            | 1 = 20kΩ VMID to output
 * -------+-----------------+------------+--------------------------------------------------
 *  5:4 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  3     | OUT3CAP         | 0          | Capless Mode Headphone Switch Enable
 *        |                 |            | 0 = OUT3 unaffected by jack detect events
 *        |                 |            | 1 = OUT3 enabled and disabled together with HP_L
 *        |                 |            |     and HP_R in response to jack detect events
 * -------+-----------------+------------+--------------------------------------------------
 *  2:0   | ADC_ALC_SR      | 000        | ALC Sample Rate
 *        |                 |            | 000 = 44.1k / 48k
 *        |                 |            | 001 = 32k
 *        |                 |            | 010 = 22.05k / 24k
 *        |                 |            | 011 = 16k
 *        |                 |            | 100 = 11.25k / 12k
 *        |                 |            | 101 = 8k
 *        |                 |            | 110 and 111 = Reserved
 */
#define R27_AdditionalControl_3 27

/*
 * R28 (1Ch) Anti-Pop (1)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  7     | POBCTRL         | 0          | Selects the bias current source for output
 *        |                 |            | amplifiers and VMID buffer
 *        |                 |            | 0 = VMID / R bias
 *        |                 |            | 1 = VGS / R bias
 * -------+-----------------+------------+--------------------------------------------------
 *  6:5 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  4     | BUFDCOPEN       | 0          | Enables the VGS / R current generator
 *        |                 |            | 0 = Disabled
 *        |                 |            | 1 = Enabled
 * -------+-----------------+------------+--------------------------------------------------
 *  3     | BUFIOEN         | 0          | Enables the VGS / R current generator and the
 *        |                 |            | analogue input and output bias
 *        |                 |            | 0 = Disabled
 *        |                 |            | 1 = Enabled
 * -------+-----------------+------------+--------------------------------------------------
 *  2     | SOFT_ST         | 0          | Enables VMID soft start
 *        |                 |            | 0 = Disabled
 *        |                 |            | 1 = Enabled
 * -------+-----------------+------------+--------------------------------------------------
 *  1 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  0     | HPSTBY          | 0          | Headphone Amplifier Standby
 *        |                 |            | 0 = Standby mode disabled (Normal operation)
 *        |                 |            | 1 = Standby mode enabled
 */
#define R28_AntiPop_1 28

/*
 * R29 (1Dh) Anti-Pop (2)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8:7 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  6     | DISOP           | 0          | Discharges the DC-blocking headphone capacitors
 *        |                 |            | on HP_L and HP_R
 *        |                 |            | 0 = Disabled
 *        |                 |            | 1 = Enabled
 * -------+-----------------+------------+--------------------------------------------------
 *  5:4   | DRES[1:0]       | 00         | DRES determines the value of the resistors used
 *        |                 |            | to discharge the DC-blocking headphone
 *        |                 |            | capacitors when DISOP=1
 *        |                 |            | 00 = 400Ω
 *        |                 |            | 01 = 200Ω
 *        |                 |            | 10 = 600Ω
 *        |                 |            | 11 = 150Ω
 * -------+-----------------+------------+--------------------------------------------------
 *  3:0 - reserved
 */
#define R29_AntiPop_2 29

#define R30_Reserved 30
#define R31_Reserved 31
#define R32_ADCLSignalPath 32
#define R33_ADCRSignalPath 33

/*
 * R34 (22h) Left Out Mix
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8     | LD2LO           | 0          | Left DAC to Left Output Mixer
 *        |                 |            | 0 = Disable (Mute)
 *        |                 |            | 1 = Enable Path
 * -------+-----------------+------------+--------------------------------------------------
 *  7     | LI2LO           | 0          | LINPUT3 to Left Output Mixer
 *        |                 |            | 0 = Disable (Mute)
 *        |                 |            | 1 = Enable Path
 * -------+-----------------+------------+--------------------------------------------------
 *  6:4   | LI2LOVOL[2:0]   | 101        | LINPUT3 to Left Output Mixer Volume
 *        |                 |            | 000 = 0dB
 *        |                 |            | ... 3dB steps up to
 *        |                 |            | 111 = -21dB
 * -------+-----------------+------------+--------------------------------------------------
 *  3:0 - reserved
 */
#define R34_LeftOutMix 34

#define R35_Reserved 35
#define R36_Reserved 36

/*
 * R37 (25h) Right Out Mix
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8     | RD2RO           | 0          | Right DAC to Right Output Mixer
 *        |                 |            | 0 = Disable (Mute)
 *        |                 |            | 1 = Enable Path
 * -------+-----------------+------------+--------------------------------------------------
 *  7     | RI2RO           | 0          | RINPUT3 to Right Output Mixer
 *        |                 |            | 0 = Disable (Mute)
 *        |                 |            | 1 = Enable Path
 * -------+-----------------+------------+--------------------------------------------------
 *  6:4   | RI2ROVOL[2:0]   | 101        | RINPUT3 to Right Output Mixer Volume
 *        |                 |            | 000 = 0dB
 *        |                 |            | ... 3dB steps up to
 *        |                 |            | 111 = -21dB
 * -------+-----------------+------------+--------------------------------------------------
 *  3:0 - reserved
 */
#define R37_RightOutMix 37

#define R38_MonoOutMix_1 38
#define R39_MonoOutMix_2 39
#define R40_LeftSpeakerVolume 40
#define R41_RightSpeakerVolume 41
#define R42_OUT3Volume 42
#define R43_LeftInputBoostMixer 43
#define R44_RightInputBoostMixer 44

/*
 * R45 (2Dh) Left Bypass
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  7     | LB2LO           | 0          | Left Input Boost Mixer to Left Output Mixer
 *        |                 |            | 0 = Disable (Mute)
 *        |                 |            | 1 = Enable Path
 * -------+-----------------+------------+--------------------------------------------------
 *  6:4   | LB2LOVOL[2:0]   | 101        | Left Input Boost Mixer to Left Output Mixer
 *        |                 |            | Volume
 *        |                 |            | 000 = 0dB
 *        |                 |            | ... 3dB steps up to
 *        |                 |            | 111 = -21dB
 * -------+-----------------+------------+--------------------------------------------------
 *  3:0 - reserved
 */
#define R45_LeftBypass 45

/*
 * R46 (2Eh) Right Bypass
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  7     | RB2RO           | 0          | Right Input Boost Mixer to Right Output Mixer
 *        |                 |            | 0 = Disable (Mute)
 *        |                 |            | 1 = Enable Path
 * -------+-----------------+------------+--------------------------------------------------
 *  6:4   | RB2ROVOL[2:0]   | 101        | Right Input Boost Mixer to Right Output Mixer
 *        |                 |            | Volume
 *        |                 |            | 000 = 0dB
 *        |                 |            | ... 3dB steps up to
 *        |                 |            | 111 = -21dB
 * -------+-----------------+------------+--------------------------------------------------
 *  3:0 - reserved
 */
#define R46_RightBypass 46

/*
 * R47 (2Fh) Power management (3)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8:6 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  5     | LMIC            | 0          | Left Channel Input PGA Enable
 *        |                 |            | 0 = PGA disabled
 *        |                 |            | 1 = PGA enabled (if AINL = 1)
 * -------+-----------------+------------+--------------------------------------------------
 *  4     | RMIC            | 0          | Right Channel Input PGA Enable
 *        |                 |            | 0 = PGA disabled
 *        |                 |            | 1 = PGA enabled (if AINR = 1)
 * -------+-----------------+------------+--------------------------------------------------
 *  3     | LOMIX           | 0          | Left Output Mixer Enable Control
 *        |                 |            | 0 = Disabled
 *        |                 |            | 1 = Enabled
 * -------+-----------------+------------+--------------------------------------------------
 *  2     | ROMIX           | 0          | Right Output Mixer Enable Control
 *        |                 |            | 0 = Disabled
 *        |                 |            | 1 = Enabled
 * -------+-----------------+------------+--------------------------------------------------
 *  1:0 - reserved
 */
#define R47_PowerManagement_3 47

#define R48_AdditionalControl_4 48
#define R49_ClassDControl_1 49
#define R50_Reserved 50
#define R51_ClassDControl_2 51

/* R52 (34h) PLL (1)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8:6   | OPCLKDIV[2:0]   | 000        | SYSCLK Output to GPIO Clock Division ratio
 *        |                 |            | 000 = SYSCLK
 *        |                 |            | 001 = SYSCLK / 2
 *        |                 |            | 010 = SYSCLK / 3
 *        |                 |            | 011 = SYSCLK / 4
 *        |                 |            | 100 = SYSCLK / 5.5
 *        |                 |            | 101 = SYSCLK / 6
 * -------+-----------------+------------+--------------------------------------------------
 *  5     | SDM             | 0          | 0 = Integer mode
 *        |                 |            | 1 = Fractional mode
 * -------+-----------------+------------+--------------------------------------------------
 *  4     | PLLPRESCALE     | 0          | 0 = Divide MCLK by 1 before input to PLL
 *        |                 |            | 1 = Divide MCLK by 2 before input to PLL
 * -------+-----------------+------------+--------------------------------------------------
 *  3:0   | PLLN[3:0]       | 1000 (8)   | Integer (N) part of PLL input/output
 *        |                 |            | frequency ratio.
 *        |                 |            | Use values greater than 5 and less than 13
 */
#define R52_PLL_1 52

/*
 * R53 (35h) PLL (2)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  7:0   | PLLK[23:16]     | 0011 0001  | Fractional (K) part of PLL1 input/output
 *        |                 | 31h        | frequency ratio (treat as one 24-digit binary
 *        |                 |            | number)
 */
#define R53_PLL_2 53

/*
 * R54 (36h) PLL (3)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  7:0   | PLLK[15:8]      | 0010 0110  | Fractional (K) part of PLL1 input/output
 *        |                 | 26h        | frequency ratio (treat as one 24-digit binary
 *        |                 |            | number)
 */
#define R54_PLL_3 54

/*
 * R55 (37h) PLL (4)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  7:0   | PLLK[7:0]       | 1110 1001  | Fractional (K) part of PLL1 input/output
 *        |                 | E9h        | frequency ratio (treat as one 24-digit binary
 *        |                 |            | number)
 */
#define R55_PLL_4 55


#endif /* __STEREO_CODEC_REG_H__ */
