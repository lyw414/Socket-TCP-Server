Rulse=`pwd`/rulse
RootPath=`pwd`
export Rulse RootPath
rm ./include/*
find ./ -name *.h | xargs -I {} cp {} ./include
find ./ -name *.msg | xargs -I {} cp {} ./include
make
