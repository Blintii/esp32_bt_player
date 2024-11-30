
import sys
import os
import math
import datetime


# this script should be called from /main/CMakeLists.txt in the start of build process


def calc_window():
    tmp_N = DSP_FFT_IN_N - 1
    i = 0

    while i < DSP_FFT_IN_N:
        if i:
            if i%8: file.write(", ")
            else: file.write(",\n    ")

        file.write(float_fmt.format(0.35875 - 0.48829 * math.cos((2 * math.pi * i) / tmp_N) + 0.14128 * math.cos((4 * math.pi * i) / tmp_N) - 0.01168 * math.cos((6 * math.pi * i) / tmp_N)))
        i += 1

def calc_twiddle():
    tmp_N = DSP_FFT_IN_N
    i = 0

    while i < DSP_FFT_RES_N:
        if i:
            if i%4: file.write(", {")
            else: file.write(",\n    {")

        file.write(float_fmt.format(math.cos(i * 2 * math.pi/tmp_N)))
        file.write(", " + float_fmt.format(math.sin(i * 2 * math.pi/tmp_N)) + "}")
        i += 1

def calc_revbits():
    i = 0

    while i < DSP_FFT_RES_N:
        rev_bit = 0
        n = 0

        while n < DSP_FFT_EXP:
            if i & (1 << n):
                rev_bit |= 1 << (DSP_FFT_EXP - 1 - n)
            n += 1

        if i:
            if i%22: file.write(", ")
            else: file.write(",\n    ")

        file.write(str(rev_bit))
        i += 1


if(3 < len(sys.argv) and sys.argv[1] == "--fft_exp"):
    now = datetime.datetime.now()
    DSP_FFT_EXP = int(sys.argv[2])
    DSP_FFT_IN_N = pow(2, DSP_FFT_EXP)
    DSP_FFT_RES_N = DSP_FFT_IN_N / 2
    float_fmt = "{:.10f}"

    with open(sys.argv[3], "w+", encoding="utf-8") as file:
        file.write("/*\n"
                    " * GENERATED FILE\n"
                    f" * from {os.path.basename(__file__)}\n"
                    f" * at {now.strftime('%Y.%m.%d. %H:%M:%S')}\n"
                    " * for DSP FFT (Fast Fourier Transform)\n"
                    " */\n"
                    "\n"
                    "#include \"stdint.h\"\n"
                    "\n"
                    "#include \"dsp.h\"\n"
                    "\n"
                    "\n"
                    "/* Blackman–Harris window implemented\n"
                    " * from https://en.wikipedia.org/wiki/Window_function#Blackman–Harris_window */\n"
                    "const float window_lut[DSP_FFT_IN_N] = {\n"
                    "    ")

        calc_window()
        file.write("\n};\n"
                    "const dsp_comp twiddle_lut[DSP_FFT_RES_N] = {\n"
                    "    {")

        calc_twiddle()
        file.write("\n};\n"
                    "const uint16_t rev_bits_lut[DSP_FFT_IN_N] = {\n"
                    "    ")

        calc_revbits()
        file.write("\n};\n")
    exit(0)
else:
    print("wrong args :(")
    exit(1)
