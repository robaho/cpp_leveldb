CXX = clang++
# CXXFLAGS = -std=c++20 -Wall -fsanitize=address -fno-omit-frame-pointer -pedantic-errors -g -I include
# CXXFLAGS = -std=c++20 -Wall -pedantic-errors -g -I include
CXXFLAGS = -std=c++20 -O3 -Wall -pedantic-errors -g -I include

TEST_SRCS = ${wildcard *_test.cpp}
TEST_OBJS = $(addprefix bin/, $(TEST_SRCS:.cpp=.o))
TEST_MAINS = $(addprefix bin/, $(TEST_SRCS:.cpp=))

HEADERS = ${wildcard include/*.h}

SRCS =	database.cpp databaseops.cpp disksegment.cpp \
		diskio.cpp snapshot.cpp logfile.cpp memorysegment.cpp \
		merger.cpp

OBJS = $(addprefix bin/, $(SRCS:.cpp=.o))

MAIN = bin/skiplist_test
MAIN_OBJ = ${basename ${MAIN}}.o

LIB = bin/cpp_leveldb.a

.PRECIOUS: bin/%.o

all: ${MAIN} $(TEST_MAINS) bin/cpp_leveldb.a
	@echo compile finished

test: ${TEST_MAINS}

run_tests: ${TEST_MAINS}
	for main in $^ ; do \
		$$main; \
	done

${LIB}: ${OBJS}
	ar r ${LIB} ${OBJS}

${MAIN}: ${MAIN_OBJ} ${LIB}
	${CXX} ${CXXFLAGS} ${MAIN_OBJ} ${LIB} -o ${MAIN}

bin/%_test: bin/%_test.o ${LIB}
	${CXX} ${CXXFLAGS} $@.o ${LIB} -o $@ 

bin/%.o: %.cpp ${HEADERS}
	@ mkdir -p bin
	${CXX} ${CXXFLAGS} -c $(notdir $(basename $@).cpp) -o $@

clean:
	rm -rf bin *~.


