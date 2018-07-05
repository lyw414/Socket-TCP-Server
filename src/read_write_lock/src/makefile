include $(Rulse)
%.o:%.cpp
	$(CPP) $(CPPFLAGS) $(INC) -c $^ -o $@ 
all:$(OBJ)
	$(CPP) $(CPPFLAGS) $(INC) -o Test $(OBJ)
	mv $(OBJ) $(Lib)
	cp Test $(Bin)
