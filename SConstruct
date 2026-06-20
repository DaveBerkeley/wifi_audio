
files = [
    'src/rtsp.cpp',
    'src/rtsp_server.cpp',
    'src/rtp.cpp',
    'src/utils.cpp',
    'src/audio_codec_pcm.cpp',
    'src/audio_codec_opus.cpp',
    'src/server.cpp',
    'src/wav_file.cpp',

    'test/main.cpp',
    'test/rtsp.cpp',
    'test/rtsp_server.cpp',
    'test/rtp.cpp',
    'test/audio_codec_opus.cpp',

    'lib/cli/src/cli.cpp',
    'lib/cli/src/list.cpp',

    # https://github.com/eyalroz/printf
    'lib/printf/src/printf/printf.c',
]

panglos = [
    'src/thread.cpp',
    'src/time.cpp',
    'src/io.cpp',
    'src/object.cpp',
    'src/list.cpp',
    'src/json.cpp',
    'src/device.cpp',
    'src/event_queue.cpp',
    'src/verbose.cpp',
    'src/socket.cpp',
    'src/batch.cpp',
    'src/network.cpp',
    'src/fmt.cpp',
    'src/date.cpp',
    'src/watchdog.cpp',

    'src/drivers/spi.cpp',
    'src/drivers/7-segment.cpp',

    'src/app/event.cpp',
    'src/app/devices.cpp',

    'src/linux/thread.cpp',
    'src/linux/mutex.cpp',
    'src/linux/queue.cpp',
    'src/linux/semaphore.cpp',
    'src/linux/time.cpp',
    'src/linux/storage.cpp',

    #'unit-tests/thread.cpp',

]

for path in panglos:
    files.append('../panglos/' + path)

#
#

ccflags = [
    '-Wswitch-default',
    #'-Wswitch-enum',
    '-Wconversion',
    #'-Wsign-conversion',
    '-Wunused-parameter',

    '-Wno-missing-field-initializers',
    '-Wformat=2', # strict level
    '-Werror=format',
    '-Wno-format-zero-length',
    '-Wno-format-nonliteral',

    '-g',
    '-DGTEST=1',
    '-DARCH_LINUX=1',
    '-DAUDIO_TX=1',
    '-Ithird_party/opus/include',
]

static_analyser = [
    # gcc static analyser options
    '-Wanalyzer-allocation-size',
    '-Wanalyzer-deref-before-check',
    '-Wanalyzer-double-fclose',
    '-Wanalyzer-double-free',
    '-Wanalyzer-exposure-through-output-file',
    '-Wanalyzer-exposure-through-uninit-copy',
    '-Wanalyzer-fd-access-mode-mismatch',
    '-Wanalyzer-fd-double-close',
    '-Wanalyzer-fd-leak',
    '-Wanalyzer-fd-phase-mismatch',
    '-Wanalyzer-fd-type-mismatch',
    '-Wanalyzer-fd-use-after-close',
    '-Wanalyzer-fd-use-without-check',
    '-Wanalyzer-file-leak',
    '-Wanalyzer-free-of-non-heap',
    '-Wanalyzer-imprecise-fp-arithmetic',
    '-Wanalyzer-infinite-recursion',
    '-Wanalyzer-jump-through-null',
    '-Wanalyzer-malloc-leak',
    '-Wanalyzer-mismatching-deallocation',
    '-Wanalyzer-null-argument',
    '-Wanalyzer-null-dereference',
    '-Wanalyzer-out-of-bounds',
    '-Wanalyzer-possible-null-argument',
    '-Wanalyzer-possible-null-dereference',
    '-Wanalyzer-putenv-of-auto-var',
    '-Wanalyzer-shift-count-negative',
    '-Wanalyzer-shift-count-overflow',
    '-Wanalyzer-stale-setjmp-buffer',
    '-Wanalyzer-tainted-allocation-size',
    '-Wanalyzer-tainted-array-index',
    '-Wanalyzer-tainted-assertion',
    '-Wanalyzer-tainted-divisor',
    '-Wanalyzer-tainted-offset',
    '-Wanalyzer-tainted-size',
    '-Wanalyzer-unsafe-call-within-signal-handler',
    '-Wanalyzer-use-after-free',
    '-Wanalyzer-use-of-pointer-in-stale-stack-frame',
    '-Wanalyzer-use-of-uninitialized-value',
    '-Wanalyzer-va-arg-type-mismatch',
    '-Wanalyzer-va-list-exhausted',
    '-Wanalyzer-va-list-leak',
    '-Wanalyzer-va-list-use-after-va-end',
    '-Wanalyzer-write-to-const',
    '-Wanalyzer-write-to-string-literal',

#    '-Wanalyzer-div-by-zero',
#    '-Wanalyzer-infinite-loop',
#    '-Wanalyzer-mkostemp-redundant-flags',
#    '-Wanalyzer-mktemp-missing-placeholder',
#    '-Wanalyzer-mktemp-of-string-literal',
#    '-Wanalyzer-overlapping-buffers',
#    '-Wanalyzer-throw-of-unexpected-type',
#    '-Wanalyzer-undefined-behavior-ptrdiff',
#    '-Wanalyzer-undefined-behavior-strtok',
]

cpppath = [
    'src',
    'lib',
    'lib/printf/src',
    'lib/panglos/src',
    'lib/cli/src',
]

cflags = [
    '-Wall',
    '-Wextra',
    '-Werror',
    '-Wformat=2', # strict level
    '-Werror=format',
]

cxxflags = [
    '-std=c++14',
    '-Wall',
    '-Wextra',
    '-Werror',
]

lflags = [
    '-lgtest_main',
    '-lgtest',
    '-lpthread',
    '-g',
]

libpath = [
    'third_party/build',
]

libs = [
    'libopus',
]

import os
cc = os.environ.get('CC', 'gcc')
cxx = os.environ.get('CXX', 'g++')

if cc == 'clang':
    sane = [
        '-fsanitize=thread',
        '-fsanitize=alignment',
        '-fno-sanitize-recover=all',
        '-Wno-implicit-int-float-conversion',
        '-Wswitch-enum',
    ]
    for x in sane:
        cflags  += [ x ]
        ccflags += [ x ]
        lflags  += [ x ]
else:
    pass
    #ccflags += static_analyser

tool_prefix = ''
cross_cflags = []

SConscript('third_party/SConscript', exports="tool_prefix cross_cflags")

env = Environment(CFLAGS=cflags, CCFLAGS=ccflags, CXXFLAGS=cxxflags, LINKFLAGS=lflags, CPPPATH=cpppath, CC=cc, CXX=cxx)
tdd = env.Program(target='tdd', source=files, LIBS=libs, LIBPATH=libpath)
Default(tdd)

