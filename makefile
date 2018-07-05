include rulse
#DIRS := $(shell find . -maxdepth 3 -type d | grep src)
DIRS := $(shell find ./ -name makefile | grep -v "\./makefile" | sed 's/makefile.*//')
all:
	cd ./src/read_write_lock/src/ && make
	cd ./src/Server_data/src/ && make
