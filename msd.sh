#!/bin/bash

export M=" \
    (fn='set_state' and state:='state:=(\S*)') \
    or ((fn='~Client' or fn='del_client') and client:='client=(\S*)')\
    or (fn='send_error' and code:='code=(\d+)' and seq:=' seq=(\d+)' and err:=' msg=(.*)')\
    or ( \
        (fn='command' or fn='common') \
        and cmd:='cmd=(\S*)' and seq:=' seq=(\d+)' and code:=' code=(\d+)' and cmd!='unknown' \
    )"
cat $1 | lex.py -M panglos -m "${M}" -j > /tmp/b.txt
cat /tmp/b.txt | ./msd.py > /tmp/a.msc
mscgen -i /tmp/a.msc -T png -o ~/tmp/a.png

#   FIN
