P=gtfs
OBJECTS=$(P).o shapes.o
CFLAGS = `pkg-config --cflags glib-2.0` -I$(HOME)/opt/include -g -Wall -std=gnu11 -O3
LDLIBS = `pkg-config --libs glib-2.0` -L$(HOME)/opt/lib -lcsv -lgdal

$(P): $(OBJECTS)

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

run: $(P)
	rm -rf out
	mkdir out
	./$(P) data/shapes.txt

clean:
	rm -rf $(OBJECTS) $(P) out

out:
	mkdir out

valgrind: $(P)
	G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind --leak-check=full ./$(P) data/shapes.txt
