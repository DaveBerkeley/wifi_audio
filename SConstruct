
files = [

    'test/main.cpp',

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

    'src/app/event.cpp',
    'src/app/devices.cpp',

    'src/linux/thread.cpp',
    'src/linux/mutex.cpp',
    'src/linux/queue.cpp',
    'src/linux/semaphore.cpp',
    'src/linux/time.cpp',

    #'unit-tests/stubs.cpp',
    'unit-tests/thread.cpp',

]

for path in panglos:
    files.append('../panglos/' + path)

ccflags = [
    '-Wswitch-default',
    #'-Wswitch-enum',
    '-Wconversion',
    #'-Wsign-conversion',
    '-Wunused-parameter',

    '-Wno-missing-field-initializers',
    '-Wno-format-zero-length',
    '-Wformat-security',

    '-g',
    '-DGTEST=1',
    '-DARCH_LINUX=1',
    '-DAUDIO_TX=1',
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

libpath = [ ]

libs = []

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

env = Environment(CFLAGS=cflags, CCFLAGS=ccflags, CXXFLAGS=cxxflags, LINKFLAGS=lflags, CPPPATH=cpppath, CC=cc, CXX=cxx)
tdd = env.Program(target='tdd', source=files, LIBS=libs, LIBPATH=libpath)

