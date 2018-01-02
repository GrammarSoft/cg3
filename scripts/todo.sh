#!/bin/bash
rm -fv src/*~
rm -f TODO.list
cat TODO > TODO.list
echo "----------" >> TODO.list
grep -i todo src/* | perl -wpne 's/^([^:]+):\s*(.+)$/$2\t: $1/;' | perl -wpne 's@^//\s*@@g;' | LC_ALL=C sort >> TODO.list
