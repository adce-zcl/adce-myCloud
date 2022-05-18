TARGET=cloud-server
OBJS=db_pool.o cloud.o cloud-server.o
CXX=g++
CFLAGS+= -Wall -g
CXXFLAGS += -I/usr/include/mysql -I/usr/include/mysql++
LDFLAGS += -L/usr/lib64/mysql
LIBS +=-pthread -lstdc++ -lmysqlpp -lmysqlclient

$(TARGET):$(OBJS)
	$(CXX) $^ -o $@ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LIBS)

clean:
	$(RM) $(TARGET) $(OBJS)