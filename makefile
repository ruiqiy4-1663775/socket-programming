#hello: hello.o
#	g++ -o hello hello.o
all: client serverM serverC serverEE serverCS

client: client.cpp
	g++ -o client client.cpp

serverM: serverM.cpp
	g++ -o serverM serverM.cpp

serverC: serverC.cpp
	g++ -o serverC serverC.cpp

serverEE: serverEE.cpp
	g++ -o serverEE serverEE.cpp

serverCS: serverCS.cpp
	g++ -o serverCS serverCS.cpp

clean:
	-rm *.o client serverM serverC serverEE serverCS