SRC=${wildcard *.cpp}
OBJ=${patsubst %.cpp,%.o,$(SRC)}
%.o:%.cpp
	g++ -m32 -std=c++11 -D LINUX_PROJ -Wall -g -lpthread -c $^ -o $@ -I ../include
all:$(OBJ)	
	g++ -m32 -std=c++11 -D LINUX_PROJ -Wall -g -lpthread -o Test $(OBJ) -I ../include
