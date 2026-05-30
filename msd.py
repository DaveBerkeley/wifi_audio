#!/bin/env python

import json
import sys

clients = []
messages = []
for line in sys.stdin:
    line = line.strip()
    j = json.loads(line)
    messages.append(j)
    thread = j['thread']
    if not thread in clients:
        clients.append(thread)

print("msc {")
print("\t", ", ".join(clients), ", server;")

t0 = None
tprev = None

for msg in messages:
    fn = msg['fn']
    cmd = msg['cmd']
    seq = msg['seq']
    t = msg['t']
    client = msg['thread']
    if t0 == None:
        t0 = t
    t -= t0

    if tprev and ((t - tprev) > 1000):
        print('---;')

    if fn == 'command':
        print(f'\t{client}=>server [ label="t={t} cmd={cmd}", ID={seq} ];')
    if fn == 'common':
        print(f'\tserver=>{client} [ label="t={t} reply={cmd}", ID={seq} ];')

    tprev = t

print("}")

#   FIN
