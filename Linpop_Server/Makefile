CC := gcc
OBJECTS := server_main.o server.o common.o protocol.o dbconn.o login.o
TARGET := server
CFLAGS := -Wall -pthread -g3 -lcrypto $(shell mysql_config --cflags --libs)
STRIP := strip
STRIPFLAGS := -s

$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(CFLAGS)

strip_all:
	$(STRIP) $(STRIPFLAGS) $(TARGET)

clean:
	rm *.o
	rm $(TARGET)
