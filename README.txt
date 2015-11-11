dova - assembler/disassembler created by Harrison Miller

////////// COMPILING DOVA //////////
g++ dova.cpp -o dova
or other modern c++ compiler equivalent

////////// RUNNING DOVA //////////
usage: ./dova <inputfile> <outputfile> <options:-xbpd>

to use the assembler:
.dova tests/jump.asm a.out

to output instruction addresses and hexadecimal/binary instructions:
./dova tests/allinstructions.asm a.out -xbp

to use the disassembler:
./dova a.out b.asm -d

the disassembler takes binary only

////////// MISC //////////
the fulltest shell script
assembles a file, disassembles it, then reassembles the output
checking if both assembley machine code outputs are the same
using diff.
