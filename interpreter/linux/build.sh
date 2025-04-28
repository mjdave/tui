#!/bin/bash
cmake -H. -Bbuild
sleep 2
if cmake --build build -- -j4; then
mkdir -p tui
cp -f bin/tui tui/tui
ln -sf ../../../examples tui/examples
else
exit 1;
fi
