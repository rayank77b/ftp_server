CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -pthread 
LDFLAGS = -lssl -lcrypto
TARGET = ftpserver
SRC = main.cpp FtpServer.cpp Logger.cpp ErrorHandler.cpp CommandParser.cpp UserAuth.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
