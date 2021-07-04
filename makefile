all:
	g++ -Wall -c recibe_bytes.cpp -lwiringPi
	g++ -Wall -c envia_bytes.cpp -lwiringPi
	g++ -Wall -c slip.cpp
	g++ -Wall -c fcs.cpp
	g++ -Wall -o rec recibe_bytes.o slip.o fcs.o -lwiringPi
	g++ -Wall -o env envia_bytes.o slip.o fcs.o -lwiringPi