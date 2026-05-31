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
print("\t", 'hscale = "2";')
print("\t", ", ".join(clients), ", server, time;")

t0 = None
tprev = None

for msg in messages:
    fn = msg['fn']
    cmd = msg.get('cmd')
    seq = msg.get('seq')
    code = msg.get('code')
    t = msg['t']
    client = msg.get('thread')
    if t0 == None:
        t0 = t
    t -= t0
    if client in [ 'rtsp', 'main' ]:
        client = 'server'

    if tprev and ((t - tprev) > 1000):
        print('\t---;')

    print(f'\ttime note time [ label="{t}" ],')

    args = ''
    if fn == 'set_state':
        args = msg.get('state', '')
    elif fn in [ '~Client', 'del_client' ]:
        args = msg.get('client', '')
    elif fn == 'send_error':
        args = msg['err']

    if fn == 'command':
        print(f'\t{client}=>server [ label="{cmd}, seq={seq}" ];')
    elif fn == 'common':
        print(f'\tserver>>{client} [ label="{cmd} code={code} seq={seq}" ];')
    elif fn == 'send_error':
        print(f'\tserver>>{client} [ label="{args} seq={seq}" ];')
    else:
        print(f'\t{client} box {client} [ label="{fn}({args})" ];')

    tprev = t

print("}")

#   FIN
