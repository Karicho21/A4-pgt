CXXFLAGS=-I ~/rapidjson/include -std=c++11 -pthread
LDFLAGS=-lcurl -pthread
CXX=g++
TARGET=gsc

all: $(TARGET)

$(TARGET): gsc.o
	$(CXX) $< -o $@ $(LDFLAGS)

gsc.o: gsc.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-rm -f $(TARGET) gsc.o

.PHONY: all clean

