#! /bin/sh
# a simd module also exists in the macos sdk at /usr/include/simd
# by renaming the directory, we can get around header confusion.
# Hopefully a better solution can be found at some point.

# If sed gives you 
# sed: RE error: illegal byte sequence
# uncomment the following lines to set locale to C
#
# export LC_CTYPE=C 
# export LANG=C

glm_dir="./thirdParty/glm"

# Check if directory is empty
if [ -z "$(ls -A "$glm_dir")" ]; then
    echo "Error: Please run git submodule update --init to set up glm before running this script."
    exit 1
fi

if [ $? -ne 0 ]; then
    echo "Error: glm_dir does not exist: $glm_dir"
    exit 1
fi

mv $glm_dir/glm/simd $glm_dir/glm/tui_simd
find $glm_dir -type f -exec sed -i '' 's/\/simd\//\/tui_simd\//g' {} +

cat > $glm_dir/glm/module.modulemap << EOL
module tuiGLM {
    export *
    requires cplusplus
}
EOL

touch $glm_dir/glm/tui_simd/tuiSimd.cpp

if [ ! -f "./Package.swift" ]; then
    echo "Error: Package.swift does not exist in the current directory."
    exit 1
fi

sed -i '' 's/\/simd/\/tui_simd/g' Package.swift
sed -i '' 's/exclude\: \["simd\/"]/exclude\: \["tui_simd\/"]/g' Package.swift