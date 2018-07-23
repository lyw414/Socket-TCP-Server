include $(Rulse)
%.o:%.cpp
	$(CPP) $(CPPFLAGS) $(INC) $(DEFINE) -c $^ -o $@ 
all:$(OBJ)
	$(CPP) $(CPPFLAGS) $(INC) $(DEFINE) -o Read_Write_File_test $(OBJ)
	mv $(OBJ) $(Lib)
	cp Read_Write_File_test $(Bin)
