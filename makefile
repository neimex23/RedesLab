todo: mensajeria
mensajeria: mensajeria.o
	g++ -Wall -o mensajeria mensajeria.o
mensajeria.o: mensajeria.cpp
	g++ -Wall -c mensajeria.cpp
	
limpiar: clean
clean:
	rm -f *.o
	rm -f mensajeria
	rm -f md5.txt
