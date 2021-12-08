#! /bin/bash

mkdir TraceyGL -p
rm TraceyGL/* -rf

cp -r ./src \
	./scenes \
	./Textures \
	./external \
	./config.txt \
	./CMakeLists.txt \
	./CMakeSettings.json \
	--target-directory TraceyGL/

rm TraceyGL.zip
zip TraceyGL.zip TraceyGL/ -r
rm -rf TraceyGL/
