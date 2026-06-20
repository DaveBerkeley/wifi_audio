
PROJECT=AUDIO_TX

#TARGET=seeed_xiao_esp32c3
#TARGET=c3_supermini
#TARGET=c6_supermini
#TARGET=esp32-s3-devkitc-1
TARGET=esp32-s3-devkitc-2
#TARGET=esp32-dev

DEVICE=esp32s3

MODE = run
#MODE = debug

.PHONY: all flash

NAME=$(shell git config user.name)
COPYRIGHT='" (C) ${NAME} "'
BANNER=src/banner.cpp

all:
	echo "//    ${PROJECT} banner. Auto generated, do not edit" > ${BANNER}
	echo "const char *banner = {" >> ${BANNER}
	echo ${PROJECT} | sed 's/_/ /g' | figlet | sed 's/\\/\\\\/g' | awk '//{printf "    \"%s\\r\\n\"\n", $$0}' >> ${BANNER}
	echo '"\\r\\n"' >> ${BANNER}
	date +"\"built %Y/%m/%d %H:%M:%S git=$$(git rev-parse --short HEAD) \"" >> ${BANNER}
	echo "\"esp-idf=\" IDF_VER" >> ${BANNER}
	echo ${COPYRIGHT} >> ${BANNER}
	date +"\"%Y\r\n\"" >> ${BANNER}
	echo "};" >> ${BANNER}
	PLATFORMIO_BUILD_FLAGS="-DPROJECT=${PROJECT} -D${PROJECT}" pio $(MODE) -e $(TARGET) 

flash:
	PLATFORMIO_BUILD_FLAGS="-DPROJECT=${PROJECT} -D${PROJECT}" pio $(MODE) -e $(TARGET) --target upload --verbose

debug:
	PLATFORMIO_BUILD_FLAGS="-DPROJECT=${PROJECT} -D${PROJECT}" pio debug -e $(TARGET)

FRAMEWORK=~/.platformio/packages/framework-espidf/components
#FRAMEWORK=~/.platformio/packages/framework-espidf@src-33f6675d8844e266f7d075822f499274/components

ctags:
	./ctags_path.py . $(FRAMEWORK) --check esp32 --good $(DEVICE) > /tmp/ctags.txt
	ctags --fields=+n -L /tmp/ctags.txt 

clean:
	rm -rf .pio managed_components
	scons -c 
	find . -name "*~" | xargs rm -f

cleanx:
	find .pio -name "*.o" | grep panglos | xargs rm

cleanlib:
	scons -c third_party/build/libopus.a

dump:
	~/.platformio/packages/toolchain-xtensa-$(DEVICE)/bin/xtensa-$(DEVICE)-elf-objdump .pio/build/$(TARGET)/firmware.elf -d -S
	#~/.platformio/packages/toolchain-riscv32-esp/bin/riscv32-esp-elf-objdump .pio/build/$(TARGET)/firmware.elf -d -S

sine.wav:
	./make_sine.py

tdd: sine.wav
	scons

test: tdd
	valgrind ./tdd --gtest_filter="-RtspServer.Test"

clang: export CC=clang
clang: export CXX=clang++
clang: $(APP)
	scons

#	JTAG debug

openocd:
	#~/.platformio/tools/tool-openocd-esp32/bin/openocd -f board/esp32-builtin.cfg
	~/.platformio/tools/tool-openocd-esp32/bin/openocd -f board/esp32s3-builtin.cfg

gdb:
	~/.platformio/tools/tool-xtensa-esp-elf-gdb/bin/xtensa-esp32-elf-gdb

gdb-local:
	gdb -nh -x .gdbinit.native ./tdd

#	FIN
