#!/usr/bin/env python

import argparse
import wave
import os

# CLI arg junk.

parser = argparse.ArgumentParser(description="Test")
parser.add_argument("filein",  help="the wav file to be read from")
parser.add_argument("fileout", help="the name of the C header and source to be written to (fileout.h and fileout.c)")
parser.add_argument("--max", metavar="N", dest="max_samples", type=int, default=1024, help="the maximum number of samples to process")
args = parser.parse_args()

# Open the wav file and read max_samples samples.

wav = wave.open(args.filein, "r");
samples = bytearray(wav.readframes(args.max_samples))
wav.close()

# Write the header file.

n = len(samples)/4;

f = open(args.fileout + ".h", "w")
f.write("#ifndef " + args.fileout.upper() + "_H\n")
f.write("#define " + args.fileout.upper() + "_H\n")
f.write("\n")
f.write("extern float *" + args.fileout + ";\n")
f.write("const int " + args.fileout + "_len = " + str(n) + ";\n")
f.write("\n")
f.write("#endif\n")
f.close()

# Write the source file.

f = open(args.fileout + ".c", "w")
f.write("#include \"" + args.fileout + ".h\"\n")
f.write("\n")
f.write("const float *" + args.fileout + " {\n")
for i in range(0, n):
    f.write("    0x%02x, 0x%02x, 0x%02x, 0x%02x,\n" % (samples[i*4+0], samples[i*4+1], samples[i*4+2], samples[i*4+3]))
f.seek(-2, os.SEEK_CUR)
f.write("\n};\n")
f.close()
