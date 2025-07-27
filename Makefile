CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -pthread
TARGET = ftpserver
SRC = main.cpp FtpServer.cpp Logger.cpp ErrorHandler.cpp CommandParser.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
