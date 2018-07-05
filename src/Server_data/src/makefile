include $(Rulse)
%.o:%.cpp
	$(CPP) $(CPPFLAGS) $(INC) -c $^ -o $@ 
all:$(OBJ)
	$(CPP) $(CPPFLAGS) $(INC) -o Test_Svr_Data $(OBJ)
	mv $(OBJ) $(Lib)
	cp Test_Svr_Data $(Bin)
