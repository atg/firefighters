# Todo: Replace this with a python script
mkdir -p build/Firefighters.app/Contents/MacOS || true

# Build .protos
deps/protobuf-2.4.1/src/protoc -Isource/net --cpp_out=source/net source/net/wire.proto
mv source/net/wire.pb.cc source/net/wire.pb.cpp

clang++ -std=c++0x -Ideps/protobuf-2.4.1/src -Ldeps/protobuf-2.4.1/src/.libs/ -lprotobuf -Fdeps/sfml-1.6/lib64 -framework SFML -framework sfml-audio -framework sfml-graphics -framework sfml-network -framework sfml-system -framework sfml-window -framework OpenGL -Ideps/sfml-1.6/lib64/SFML.framework/Headers -Isource -o build/Firefighters.app/Contents/MacOS/Firefighters source/*/*.cpp

