# Todo: Replace this with a python script
mkdir -p build/Firefighters.app/Contents/MacOS || true

# Freetype
cp deps/libfreetype.6.dylib build/Firefighters.app/Contents/Frameworks/libfreetype.6.dylib
install_name_tool -change /usr/X11/lib/libfreetype.6.dylib @executable_path/../Frameworks/libfreetype.6.dylib build/Firefighters.app/Contents/Frameworks/sfml-graphics.framework/Versions/A/sfml-graphics

# Build .protos
deps/protobuf-2.4.1/src/protoc -Isource/net --cpp_out=source/net source/net/wire.proto
mv source/net/wire.pb.cc source/net/wire.pb.cpp

if [[ -e deps/protobuf-2.4.1/src/.libs/libprotobuf.dylib ]]
then
    mv deps/protobuf-2.4.1/src/.libs/libprotobuf.dylib deps/protobuf-2.4.1/src/.libs/_libprotobuf.dylib
fi

clang++ -std=c++0x -Ideps -Ideps/protobuf-2.4.1/src -Ldeps/protobuf-2.4.1/src/.libs/ -lprotobuf -Fdeps/sfml-1.6/lib64 -framework SFML -framework sfml-audio -framework sfml-graphics -framework sfml-network -framework sfml-system -framework sfml-window -framework OpenGL -Ideps/sfml-1.6/lib64/SFML.framework/Headers -Isource -o build/Firefighters.app/Contents/MacOS/Firefighters source/*/*.cpp

