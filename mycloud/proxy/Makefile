TARGET=proxy-server
OBJS=proxy-server.o proxy.o
CXX=g++
CFLAGS+= -Wall -g
# CXXFLAGS += -I/usr/include/mysql -I/usr/include/mysql++
# LDFLAGS += -L/usr/lib64/mysql
LIBS += -lstdc++

$(TARGET):$(OBJS)
	$(CXX) $^ -o $@ $(CFLAGS) $(LIBS)

clean:
	$(RM) $(TARGET) $(OBJS)