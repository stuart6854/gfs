#!/bin/bash

chmod +x GenerateProj_GMake2.sh
chmod +x vendor/premake/premake5
vendor/premake/premake5 --cc=clang --file=build.lua gmake2