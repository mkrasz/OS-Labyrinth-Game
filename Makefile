all: labyrinth
labyrinth: inout.o main.o game.o generating.o
	gcc -Wall -std=gnu99 -o labyrinth inout.o main.o game.o generating.o -lpthread -lm
inout.o: src/inout.c lib/inout.h lib/structures.h lib/generating.h lib/game.h
	gcc -Wall -std=gnu99 -c src/inout.c -lpthread -lm
main.o: src/main.c lib/inout.h lib/structures.h lib/generating.h lib/game.h
	gcc -Wall -std=gnu99 -c src/main.c -lpthread -lm
game.o: src/game.c lib/inout.h lib/structures.h lib/generating.h lib/game.h
	gcc -Wall -std=gnu99 -c src/game.c -lpthread -lm
generating.o: src/generating.c lib/inout.h lib/structures.h lib/generating.h lib/game.h
	gcc -Wall -std=gnu99 -c src/generating.c -lpthread -lm
clean:
	rm *.o labyrinth
.PHONY: clean all