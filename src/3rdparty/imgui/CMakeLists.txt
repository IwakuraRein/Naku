file(GLOB headers *.hpp *.h)
file(GLOB sources *.cpp *.c)

list(SORT headers)
list(SORT sources)

source_group(Headers FILES ${headers})
source_group(Sources FILES ${sources})

add_library(imgui ${sources} ${headers})