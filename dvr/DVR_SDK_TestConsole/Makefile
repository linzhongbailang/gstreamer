TOOLSPREFIX = arm-none-linux-gnueabi-

TARGETDIR = ./bin
TARGET    = $(TARGETDIR)/test
SOURCES   = $(wildcard ./src/*.cpp)
OBJS      = $(patsubst %.cpp,%.o,$(SOURCES))

all: $(TARGET)

$(TARGET):$(OBJS)
	$(TOOLSPREFIX)g++ -o $@ $^ -L./lib -lavplayer -lCiAVNSDK -lcinav -larpresent -lasound -lpthread -licudata -licuuc -licui18n -lm -ldl

%.o:%.cpp
	$(TOOLSPREFIX)g++ -I./inc/ -I./src/ -c -o $@ $^

clean:
	rm $(TARGETDIR)/test
	rm -rf $(OBJS)
