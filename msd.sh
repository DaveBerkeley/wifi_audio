#!/bin/bash

cat $1 | lex.py -D -m "fn='command' or fn='common'" -m "cmd:='cmd=(\S*)'" -m "seq:=' seq=(\d+)'" -m "code:=' code=(\d+)'" -m "cmd!='unknown'"  -j | ./msd.py > /tmp/a.msc
mscgen -i /tmp/a.msc -T png -o ~/tmp/a.png

#   FIN
