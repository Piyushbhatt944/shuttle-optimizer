CXX      = g++
CXXFLAGS = -Iinclude -Wall -std=c++14
TARGET   = shuttle_optimizer
SRCS     = src/main.cpp src/data.cpp src/scheduler.cpp src/optimizer.cpp src/evaluator.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	del /f /q $(TARGET).exe 2>nul || rm -f $(TARGET)

.PHONY: all clean
