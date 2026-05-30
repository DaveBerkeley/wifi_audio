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
        if thread.startswith("client_"):
            clients.append(thread)

print("msc {")
print("\t", ", ".join(clients), ", server;")

t0 = None
tprev = None

for msg in messages:
    fn = msg['fn']
    cmd = msg.get('cmd')
    seq = msg.get('seq')
    code = msg.get('code')
    t = msg['t']
    client = msg.get('thread')
    state = msg.get('state')
    if t0 == None:
        t0 = t
    t -= t0

    if tprev and ((t - tprev) > 1000):
        print('\t---;')

    if fn == 'command':
        print(f'\t{client}=>server [ label="t={t} cmd={cmd}", ID={seq} ];')
    if fn == 'common':
        print(f'\tserver=>{client} [ label="t={t} code={code} reply={cmd}", ID={seq} ];')
    if fn == 'set_state':
        if client == 'main': client = 'server'
        print(f'\t{client} box {client} [ label="set_state({state})" ];')

    tprev = t

print("}")

#   FIN
