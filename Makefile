GCC=g++
CXXFLAGS=-fPIC -std=c++11 -c -I./src
LDFLAGS=-fPIC -std=c++11 -shared
LDLIBS=-ldl -lssl -lcurl

all: libblt_hook.so

clean:
	rm ./build/*.o ./libblt_hook.so

libblt_hook.so: src/linux_preload.c build/console.o build/init.o build/http.o build/signatures.o build/subhook.o build/threading.o build/logging.o build/files.o build/compression.o
	${GCC} ${LDFLAGS} $^ -o $@ ${LDLIBS}

build/console.o: src/console/console.cpp
	${GCC} ${CXXFLAGS} $^ -o $@

build/http.o: src/http/http.cpp
	${GCC} ${CXXFLAGS} $^ -o $@

build/signatures.o: src/signatures/signatures_linux.cpp
	${GCC} ${CXXFLAGS} $^ -o $@

build/subhook.o: src/subhook/subhook.c
	${GCC} ${CXXFLAGS} $^ -o $@ -fpermissive

build/threading.o: src/threading/threadqueue.cpp
	${GCC} ${CXXFLAGS} $^ -o $@

build/logging.o: src/util/logging.cpp
	${GCC} ${CXXFLAGS} $^ -o $@

build/files.o: src/util/files_linux.cpp
	${GCC} ${CXXFLAGS} $^ -o $@

build/compression.o: src/util/compression.cpp
	${GCC} ${CXXFLAGS} $^ -o $@

build/init.o: src/InitiateState.cpp
	${GCC} ${CXXFLAGS} $^ -o $@
