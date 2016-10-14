CC := gcc
CFLAGS := -g3 -Wall -lcrypto -pthread $(shell pkg-config --libs --cflags gtk+-2.0)
OBJECTS := callbacks.o calendar_window.o capture_image.o chat_window.o emoji_window.o global_settings.o logger.o login_window.o main.o main_window.o register_window.o resize_image.o settings_window.o common.o handler.o server.o user_dao.o avatar_select_window.o notification.o group_dao.o change_skin.o
TARGET := sealink
STRIPFLAGS := -s

$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(CFLAGS)

strip_all:
	strip $(STRIPFLAGS) $(TARGET)

clean:
	rm *.o
	rm $(TARGET)
