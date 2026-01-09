#!/bin/bash
cmake -H. -Bbuild
sleep 2
if cmake --build build -- -j4; then
mkdir -p tui
cp -f bin/tui tui/tui
cp -rf ../../examples tui/
cp -rf ../../tests tui/
printf "\nThe tui interpreter binary is now installed in ./tui/\nRun an example with:\n\ncd tui\n./tui examples/example.tui\n\n"
else
exit 1;
fi
