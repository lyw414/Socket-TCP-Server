include $(Rulse)
%.o:%.cpp
	$(CPP) $(CPPFLAGS) $(INC) $(DEFINE) -c $^ -o $@ 
all:$(OBJ)
	$(CPP) $(CPPFLAGS) $(INC) $(DEFINE) -o Socket_Test $(OBJ)
	mv $(OBJ) $(Lib)
	cp Socket_Test $(Bin)
