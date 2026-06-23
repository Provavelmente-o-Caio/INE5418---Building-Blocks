CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -g

TARGET := distribuida_snapshot

SRC := \
	main.cpp \
	app/Agencia.cpp \
	app/Conta.cpp \
	app/BankApplication.cpp \
	net/Message.cpp \
	net/NetworkNode.cpp

OBJ := $(SRC:.cpp=.o)

.PHONY: all clean run1 run2 run3 debug

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run1: $(TARGET)
	./$(TARGET) 1
run2: $(TARGET)
	./$(TARGET) 2

run3: $(TARGET)
	./$(TARGET) 3

debug: CXXFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
debug: clean $(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)
