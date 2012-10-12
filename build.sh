# Todo: Replace this with a python script
mkdir -p build/Firefighters.app/Contents/MacOS || true

clang++ -Fdeps/sfml-1.6/lib64 -framework SFML -framework sfml-audio -framework sfml-graphics -framework sfml-network -framework sfml-system -framework sfml-window -framework OpenGL -Ideps/sfml-1.6/lib64/SFML.framework/Headers -Isource -o build/Firefighters.app/Contents/MacOS/Firefighters source/*/*.cpp

