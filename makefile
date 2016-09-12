CC = gcc
CFLAGS=-I. `pkg-config --cflags --libs pocketsphinx sphinxbase` -lportaudio -g
main: main.c stt.c record.c 
	$(CC) $(CFLAGS) -o main main.c stt.c record.c 


