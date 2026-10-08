build mode target:
    cmake --build ./build/{{mode}}/ --target {{target}} 

config mode:
    cmake -S . -B ./build/{{mode}}/ -DCMAKE_BUILD_TYPE={{mode}}

run mode target +params="":
    ./build/{{mode}}/{{target}} {{params}}