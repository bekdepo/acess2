SCRIPT_NAME=elf
OUTPUT_FORMAT=elf64-x86_64
TEXT_START_ADDR=0x00400000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
COMMONPAGESIZE="CONSTANT (COMMONPAGESIZE)"
TEMPLATE_NAME=elf64

ARCH=x86_64
MACHINE=
NOP=0x90909090
GENERATE_SHLIB_SCRIPT=yes
GENERATE_PIE_SCRIPT=yes

NO_SMALL_DATA=yes
SEPARATE_GOTPLT=12

ELF_INTERPRETER_NAME=\"/Acess/Libs/ld-acess.so\"
