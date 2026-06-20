#!/bin/env python

import os

def of_interest(fname):
    for ending in [ ".h", ".c", ".cpp", ".S", ".s" ]:
        if fname.endswith(ending):
            return True
    return False

def recurse(base, store):
    for fname in os.listdir(base):
        if base.startswith("./.pio"):
            continue
        if not base.startswith("./"):
            if fname.startswith(check):
                if fname != good:
                    continue
        path = os.path.join(base, fname)
        if os.path.isdir(path):
            recurse(path, store)
        elif os.path.isfile(path):
            if of_interest(fname):
                if not path in store:
                    store[path] = True

#
#

if __name__ == "__main__":
    import argparse

    p = argparse.ArgumentParser()
    p.add_argument('path', nargs='+')
    p.add_argument('-g', '--good', dest='good')
    p.add_argument('-c', '--check', dest='check', default="esp32")
    args = p.parse_args()

    #print(args)
    check = args.check
    good = args.good

    paths = args.path

    store = {}
    for path in paths:
        recurse(path, store)
    for key in store.keys():
        print(key)

#   F
