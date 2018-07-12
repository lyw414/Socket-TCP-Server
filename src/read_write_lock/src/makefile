include $(Rulse)
%.o:%.cpp
	$(CPP) $(CPPFLAGS) $(INC) $(DEFINE) -c $^ -o $@ 
all:$(OBJ)
	$(CPP) $(CPPFLAGS) $(INC) $(DEFINE) -o Test $(OBJ)
	mv $(OBJ) $(Lib)
	cp Test $(Bin)
