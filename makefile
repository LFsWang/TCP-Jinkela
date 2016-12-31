all:ftp_proxy.cpp
	g++ ftp_proxy.cpp -std=c++14 -Wall -Wextra -pedantic -fsanitize=undefined
