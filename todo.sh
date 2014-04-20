#!/bin/bash
rm -f TODO.list
cat TODO > TODO.list
echo "----------" >> TODO.list
grep -i todo src/* >> TODO.list
