include $(Rulse)
%.o:%.cpp
	$(CPP) $(CPPFLAGS) $(INC) $(DEFINE) -c $^ -o $@ 
all:$(OBJ)
	$(CPP) $(CPPFLAGS) $(INC) $(DEFINE) -o Test_Svr_Data $(OBJ)
	mv $(OBJ) $(Lib)
	cp Test_Svr_Data $(Bin)
