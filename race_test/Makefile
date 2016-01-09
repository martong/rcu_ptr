all:
	#$(CXX) -std=c++14 -g race_test.cpp -o race_test
	$(CXX) -std=c++14 -O2 -g -fsanitize=thread -static-libtsan race_test.cpp -o race_test
