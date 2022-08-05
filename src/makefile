BIN=ChatServer
LIBPATH=-L /usr/lib64/mysql -lmysqlclient -ljsoncpp -lpthread

$(BIN):ChatServer.cpp
	g++ $^ -o $@ $(LIBPATH)

.PHONY:clean
clean:
	rm $(BIN)
