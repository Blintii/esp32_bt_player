/*
 * ACH (Audio Codec Handler)
 */

#ifndef __ACH_REG_H__
#define __ACH_REG_H__


typedef union {
/* WM8960 has 9bit width registers */
    uint16_t raw: 9;
    uint16_t R0_LeftInputVolume: 9;
    uint16_t R1_RightInputVolume: 9;
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
    struct {
        uint16_t LOUT1VOL: 7;
        uint16_t LO1ZC: 1;
        uint16_t OUT1VU: 1;
    } R2_LOUT1Volume;
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
    struct {
        uint16_t ROUT1VOL: 7;
        uint16_t RO1ZC: 1;
        uint16_t OUT1VU: 1;
    } R3_ROUT1Volume;
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
    struct {
        uint16_t CLKSEL: 1;
        uint16_t SYSCLKDIV: 2;
        uint16_t DACDIV: 3;
        uint16_t ADCDIV: 3;
    } R4_Clocking_1;
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
    struct {
        uint16_t ADCHPD: 1;
        uint16_t DEEMPH: 2;
        uint16_t DACMU: 1;
        uint16_t reserved4: 1;
        uint16_t ADCPOL: 2;
        uint16_t DACDIV2: 1;
        uint16_t reserved8: 1;
    } R5_ADCAndDACControl_1;
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
    struct {
        uint16_t reserved0: 1;
        uint16_t DACSLOPE: 1;
        uint16_t DACMR: 1;
        uint16_t DACSMM: 1;
        uint16_t reserved4: 1;
        uint16_t DACPOL: 2;
        uint16_t reserved7: 2;
    } R6_ADCAndDACControl_2;
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
    struct {
        uint16_t FORMAT: 2;
        uint16_t WL: 2;
        uint16_t LRP: 1;
        uint16_t DLRSWAP: 1;
        uint16_t MS: 1;
        uint16_t BCLKINV: 1;
        uint16_t ALRSWAP: 1;
    } R7_AudioInterface_1;
    uint16_t R8_Clocking_2: 9;
    uint16_t R9_AudioInterface_2: 9;
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
    struct {
        uint16_t LDACVOL: 8;
        uint16_t DACVU: 1;
    } R10_LeftDACVolume;
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
    struct {
        uint16_t RDACVOL: 8;
        uint16_t DACVU: 1;
    } R11_RightDACVolume;
    uint16_t R12_Reserved: 9;
    uint16_t R13_Reserved: 9;
    uint16_t R14_Reserved: 9;
/*
 * R15 (0Fh) Reset
 * Writing to this register resets all registers to their default state
 */
    uint16_t R15_Reset: 9;
    uint16_t R16_3DControl: 9;
    uint16_t R17_ALC_1: 9;
    uint16_t R18_ALC_2: 9;
    uint16_t R19_ALC_3: 9;
    uint16_t R20_NoiseGate: 9;
    uint16_t R21_LeftADCVolume: 9;
    uint16_t R22_RightADCVolume: 9;
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
 *        |                 |            | 0 = SYSCLK / 2^21 (Slower Response)
 *        |                 |            | 1 = SYSCLK / 2^19 (Faster Response)
 * -------+-----------------+------------+--------------------------------------------------
 *  0     | TOEN            | 0          | Enables Slow Clock for Volume Update Timeout
 *        |                 |            | and Jack Detect Debounce
 *        |                 |            | 0 = Slow clock disabled
 *        |                 |            | 1 = Slow clock enabled
 */
    struct {
        uint16_t TOEN: 1;
        uint16_t TOCLKSEL: 1;
        uint16_t DATSEL: 2;
        uint16_t DMONOMIX: 1;
        uint16_t reserved5: 1;
        uint16_t VSEL: 2;
        uint16_t TSDEN: 1;
    } R23_AdditionalControl_1;
    uint16_t R24_AdditionalControl_2: 9;
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
    struct {
        uint16_t DIGENB: 1;
        uint16_t MICB: 1;
        uint16_t ADCR: 1;
        uint16_t ADCL: 1;
        uint16_t AINR: 1;
        uint16_t AINL: 1;
        uint16_t VREF: 1;
        uint16_t VMIDSEL: 2;
    } R25_PowerManagment_1;
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
    struct {
        uint16_t PLLEN: 1;
        uint16_t OUT3: 1;
        uint16_t reserved2: 1;
        uint16_t SPKR: 1;
        uint16_t SPKL: 1;
        uint16_t ROUT1: 1;
        uint16_t LOUT1: 1;
        uint16_t DACR: 1;
        uint16_t DACL: 1;
    } R26_PowerManagment_2;
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
    struct {
        uint16_t ADC_ALC_SR: 3;
        uint16_t OUT3CAP: 1;
        uint16_t reserved4: 2;
        uint16_t VROI: 1;
        uint16_t reserved7: 2;
    } R27_AdditionalControl_3;
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
    struct {
        uint16_t HPSTBY: 1;
        uint16_t reserved1: 1;
        uint16_t SOFT_ST: 1;
        uint16_t BUFIOEN: 1;
        uint16_t BUFDCOPEN: 1;
        uint16_t reserved5: 2;
        uint16_t POBCTRL: 1;
        uint16_t reserved8: 1;
    } R28_AntiPop_1;
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
    struct {
        uint16_t reserved0: 4;
        uint16_t DRES: 2;
        uint16_t DISOP: 1;
        uint16_t reserved7: 2;
    } R29_AntiPop_2;
    uint16_t R30_Reserved: 9;
    uint16_t R31_Reserved: 9;
    uint16_t R32_ADCLSignalPath: 9;
    uint16_t R33_ADCRSignalPath: 9;
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
    struct {
        uint16_t reserved0: 4;
        uint16_t LI2LOVOL: 3;
        uint16_t LI2LO: 1;
        uint16_t LD2LO: 1;
    } R34_LeftOutMix;
    uint16_t R35_Reserved: 9;
    uint16_t R36_Reserved: 9;
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
    struct {
        uint16_t reserved0: 4;
        uint16_t RI2ROVOL: 3;
        uint16_t RI2RO: 1;
        uint16_t RD2RO: 1;
    } R37_RightOutMix;
    uint16_t R38_MonoOutMix_1: 9;
    uint16_t R39_MonoOutMix_2: 9;
    uint16_t R40_LeftSpeakerVolume: 9;
    uint16_t R41_RightSpeakerVolume: 9;
    uint16_t R42_OUT3Volume: 9;
    uint16_t R43_LeftInputBoostMixer: 9;
    uint16_t R44_RightInputBoostMixer: 9;
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
    struct {
        uint16_t reserved0: 4;
        uint16_t LB2LOVOL: 3;
        uint16_t LB2LO: 1;
        uint16_t reserved8: 1;
    } R45_LeftBypass;
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
    struct {
        uint16_t reserved0: 4;
        uint16_t RB2ROVOL: 3;
        uint16_t RB2RO: 1;
        uint16_t reserved8: 1;
    } R46_RightBypass;
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
    struct {
        uint16_t reserved0: 2;
        uint16_t ROMIX: 1;
        uint16_t LOMIX: 1;
        uint16_t RMIC: 1;
        uint16_t LMIC: 1;
        uint16_t reserved6: 3;
    } R47_PowerManagement_3;
    uint16_t R48_AdditionalControl_4: 9;
    uint16_t R49_ClassDControl_1: 9;
    uint16_t R50_Reserved: 9;
    uint16_t R51_ClassDControl_2: 9;
/*
 * R52 (34h) PLL (1)
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
    struct {
        uint16_t PLLN: 4;
        uint16_t PLLPRESCALE: 1;
        uint16_t SDM: 1;
        uint16_t OPCLKDIV: 3;
    } R52_PLL_1;
/*
 * R53 (35h) PLL (2)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  7:0   | PLLK[23:16]     | 0011 0001  | Fractional (K) part of PLL1 input/output
 *        |                 | 31h        | frequency ratio (treat as one 24-digit binary
 *        |                 |            | number)
 */
    struct {
        uint16_t PLLK: 8;
        uint16_t reserved8: 1;
    } R53_PLL_2;
/*
 * R54 (36h) PLL (3)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  7:0   | PLLK[15:8]      | 0010 0110  | Fractional (K) part of PLL1 input/output
 *        |                 | 26h        | frequency ratio (treat as one 24-digit binary
 *        |                 |            | number)
 */
    struct {
        uint16_t PLLK: 8;
        uint16_t reserved8: 1;
    } R54_PLL_3;
/*
 * R55 (37h) PLL (4)
 *
 *  bits  | label           | default    | description
 * -------+-----------------+------------+--------------------------------------------------
 *  8 - reserved
 * -------+-----------------+------------+--------------------------------------------------
 *  7:0   | PLLK[7:0]       | 1110 1001  | Fractional (K) part of PLL1 input/output
 *        |                 | E9h        | frequency ratio (treat as one 24-digit binary
 *        |                 |            | number)
 */
    struct {
        uint16_t PLLK: 8;
        uint16_t reserved8: 1;
    } R55_PLL_4;
} ach_reg_WM8960;


#endif /* __ACH_REG_H__ */
