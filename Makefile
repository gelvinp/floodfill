floodfill: floodfill.o
	g++ -std=c++17 -g -Wall floodfill.o -o floodfill

floodfill.o: floodfill.cpp
	g++ -std=c++17 -g -Wall -c floodfill.cpp -o floodfill.o

clean:
	rm *.o floodfill
