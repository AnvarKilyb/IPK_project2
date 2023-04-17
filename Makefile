CC = g++
CFLAGS = -g -Wall
TARGET = server

all: $(TARGET)

$(TARGET) : $(TARGET).cpp
			$(CC) $(CFLAGS) -o ipkcpd $(TARGET).cpp

clean:
	rm ipkcpd
