UNAME_S := $(shell uname -s)
.PHONY: race_test
.PHONY: unit_test
ifeq ($(UNAME_S),Darwin)
race_test:
	$(CXX) -std=c++14 -g race_test/race_test.cpp -o race_test/race_test
endif
ifeq ($(UNAME_S),Linux)
race_test:
	$(CXX) -std=c++14 -O2 -g -fsanitize=thread -static-libtsan race_test/race_test.cpp -o race_test/race_test
endif

unit_test:
	$(CXX) -std=c++14 -g unit_test/unit_test.cpp -o unit_test/unit_test

all: unit_test race_test
