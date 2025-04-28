#!/bin/bash
cmake -H. -Bbuild
sleep 2
#cmake --build build -- -j4 VERBOSE=1
if cmake --build build -- -j4; then
mkdir -p sapiens_server/GameResources/
cp -f bin/sapiens_server sapiens_server/linuxServer
cp -f bin/libSPCommon.so sapiens_server/
cp -f bin/libSPVanilla.so ../GameResources/mods/
cp -f ../Tools/steam_appid.txt sapiens_server/ 
cp -f ../ThirdParty/steamworks_sdk_159/sdk/redistributable_bin/linux64/libsteam_api.so sapiens_server/
#cp -f ThirdPartyLinux/steamclient.so sapiens_server/
rsync -a --delete ../GameResources/scripts sapiens_server/GameResources/
rsync -a --delete ../GameResources/models sapiens_server/GameResources/
rsync -a --delete ../GameResources/mods sapiens_server/GameResources/
rsync -a --delete ../GameResources/localizations sapiens_server/GameResources/
else
exit 1;
fi
