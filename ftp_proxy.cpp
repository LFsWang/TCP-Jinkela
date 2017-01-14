#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <climits>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <arpa/inet.h>


#define MAXSIZE 2048
#define FTP_PORT 8740
#define FTP_PASV_CODE 227
#define FTP_ADDR "140.114.71.159"

int proxy_IP[4];

int connect_FTP(int ser_port, int clifd);
int proxy_func(int ser_port, int clifd, int rate);
int create_server(int port);

double rspeed = 0;
int stop = 0;
double limit = 30.0;

double uspeed = 0;
int ustop = 0;
double ulimit = 30.0;





int main (int argc, char **argv) {
	int ctrlfd, connfd, port, rate = 0;
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_in cliaddr;
	if (argc < 3) {
		printf("[v] Usage: ./executableFile <ProxyIP> <ProxyPort> <Drate> <Urate>\n");
		return -1;
	}

	sscanf(argv[1], " %d.%d.%d.%d", &proxy_IP[0], &proxy_IP[1], &proxy_IP[2], &proxy_IP[3]);
	port = atoi(argv[2]);
	if( argc >= 4 ){
		limit = std::atoi(argv[3]);
		printf("[v] Set download rate as %f kbs\n",limit);
	}else limit = 1024*1000;

	if( argc >= 5 ){
		ulimit = std::atoi(argv[4]);
		printf("[v] Set upload rate as %f kbs\n",ulimit);
	}else ulimit = 1024*1000;

	ctrlfd = create_server(port);
	clilen = sizeof(struct sockaddr_in);
	for (;;) {
		connfd = accept(ctrlfd, (struct sockaddr *)&cliaddr, &clilen);
		if (connfd < 0) {
			printf("[x] Accept failed\n");
			return 0;
		}

		printf("[v] Client: %s:%d connect!\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
		if ((childpid = fork()) == 0) {
			close(ctrlfd);
			proxy_func(FTP_PORT, connfd, rate);
			printf("[v] Client: %s:%d terminated!\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
			exit(0);
		}

		close(connfd);
	}
	return 0;
}

int connect_FTP(int ser_port, int clifd) {
	int sockfd;
	char addr[] = FTP_ADDR;
	int byte_num;
	char buffer[MAXSIZE];
	struct sockaddr_in servaddr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("[x] Create socket error");
		return -1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(ser_port);

	if (inet_pton(AF_INET, addr, &servaddr.sin_addr) <= 0) {
		printf("[v] Inet_pton error for %s", addr);
		return -1;
	}

	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		printf("[x] Connect error");
		return -1;
	}

	printf("[v] Connect to FTP server\n");
	if (ser_port == FTP_PORT) {
		if ((byte_num = read(sockfd, buffer, MAXSIZE)) <= 0) {
			printf("[x] Connection establish failed.\n");
		}

		if (write(clifd, buffer, byte_num) < 0) {
			printf("[x] Write to client failed.\n");
			return -1;
		}
	}

	return sockfd;
}

inline double fix_speed(double limit,double speed){
	if( limit >= 1024 )
		speed = (85*speed+93440)/192;
	else if(limit >= 768)
		speed = (201.5*speed+31488)/256;
	else if(limit >= 512)
		speed = (112.2*speed+6553.6)/128;
	else if(limit >= 256)
		speed = (125.05*speed+288)/128;
	return speed;
}
inline void cal_speed(double sub,double pspeed,double limit,int &speed_k){
    if(sub>1.0){
        if( pspeed > limit ){
            if(speed_k>0)++speed_k;
            else speed_k=1;
        }else if( pspeed < limit ){
            if(speed_k<0)--speed_k;
            else speed_k=-1;
        }
    }else{
        if( pspeed > limit ){
            speed_k=1;
        }else if( uspeed < limit ){
            speed_k=-1;
        }
    }
}

int speed_k,uspeed_k;
const double eps=1e-1;
const int LOWERSIZE=512;
int proxy_func(int ser_port, int clifd, int rate) {
	char buffer[MAXSIZE];
	int serfd = -1, datafd = -1, connfd;
	int data_port;
	int byte_num;
	int status, pasv[7];
	int childpid;
	socklen_t clilen;
	struct sockaddr_in cliaddr;

	// select vars
	int maxfdp1;
	int nready = 0;
	fd_set rset, allset;

	// connect to FTP server
	if ((serfd = connect_FTP(ser_port, clifd)) < 0) {
		printf("[x] Connect to FTP server failed.\n");
		return -1;
	}

	// datafd = serfd;

	// initialize select vars
	FD_ZERO(&allset);
	FD_SET(clifd, &allset);
	FD_SET(serfd, &allset);
	speed_k=1;
	uspeed_k=1;
	// selecting
	auto tmd = std::chrono::high_resolution_clock::now();
	std::chrono::nanoseconds ed=tmd-tmd,ued=tmd-tmd;
	int r_buf_size=MAXSIZE,u_buf_size=MAXSIZE;
	for (;;) {
		auto s = std::chrono::high_resolution_clock::now();
		// reset select vars
		rset = allset;
		maxfdp1 = std::max(clifd, serfd) + 1;

		// select descriptor
		nready = select(maxfdp1, &rset, NULL, NULL, NULL);
		if (nready > 0) {
			// check FTP client socket fd
			if (FD_ISSET(clifd, &rset)) {
				memset(buffer, 0, u_buf_size);
				if ((byte_num = read(clifd, buffer, u_buf_size)) <= 0) {
					printf("[!] Client terminated the connection.\n");
					break;
				}

				if (write(serfd, buffer, byte_num) < 0) {
					printf("[x] Write to server failed.\n");
					break;
				}
				usleep(ustop);
				auto e = std::chrono::high_resolution_clock::now();
				std::chrono::nanoseconds ns = e-s+ued;
				double speed = (double)byte_num/(ns.count())*(1E9/1024);
				speed=fix_speed(ulimit,speed);
				uspeed = uspeed==0 ? speed : (uspeed * 0.6 + speed * 0.4);
				double sub=std::abs(uspeed-ulimit);
				if(sub>eps){
                    cal_speed(sub,uspeed,ulimit,uspeed_k);
                    int next_ustop=ustop+100*uspeed_k;
					ustop = std::max(0,next_ustop);
					if(next_ustop<0&&u_buf_size==LOWERSIZE){
						u_buf_size=MAXSIZE;
						printf("[!] Change u_buf_size %d\n",u_buf_size);
					}
					if(ustop>20000&&u_buf_size==MAXSIZE){
						u_buf_size=LOWERSIZE;
						ustop=0;
						printf("[!] Change u_buf_size %d\n",u_buf_size);
					}
				}
				//printf("[!] %3d bytes use %9lld ns, aka %5.4f kb/s %6d\n",byte_num,(long long)ns.count(),uspeed,ustop);
				ued=std::chrono::high_resolution_clock::now()-e;
			}

			// check FTP server socket fd
			if (FD_ISSET(serfd, &rset)) {
				memset(buffer, 0, r_buf_size);
				//auto s = std::chrono::high_resolution_clock::now();
				if ((byte_num = read(serfd, buffer, r_buf_size)) <= 0) {
					printf("[!] Server terminated the connection.\n");
					break;
				}
				//auto re = std::chrono::high_resolution_clock::now();
				if(ser_port == FTP_PORT)
					buffer[byte_num] = '\0';

				status = atoi(buffer);

				if (status == FTP_PASV_CODE && ser_port == FTP_PORT) {
					puts(buffer);

					sscanf(buffer, "%d Entering Passive Mode (%d,%d,%d,%d,%d,%d)",&pasv[0],&pasv[1],&pasv[2],&pasv[3],&pasv[4],&pasv[5],&pasv[6]);
					memset(buffer, 0, byte_num);
					sprintf(buffer, "%d Entering Passive Mode (%d,%d,%d,%d,%d,%d)\n", status, proxy_IP[0], proxy_IP[1], proxy_IP[2], proxy_IP[3], pasv[5], pasv[6]);

					speed_k=1;
					if ((childpid = fork()) == 0) {
						data_port = pasv[5] * 256 + pasv[6];
						datafd = create_server(data_port);
						printf("[-] Waiting for data connection!\n");
						clilen = sizeof(struct sockaddr_in);
						connfd = accept(datafd, (struct sockaddr *)&cliaddr, &clilen);
						if (connfd < 0) {
							printf("[x] Accept failed\n");
							return 0;
						}

						printf("[v] Data connection from: %s:%d connect.\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
						proxy_func(data_port, connfd, rate);
						printf("[!] End of data connection!\n");
						exit(0);
					}
				}

				if (write(clifd, buffer, byte_num) < 0) {
					printf("[x] Write to client failed.\n");
					break;
				}
				usleep(stop);
				auto e = std::chrono::high_resolution_clock::now();
				std::chrono::nanoseconds ns = e-s+ed;
				double speed = (double)byte_num/(ns.count())*(1E9/1024);
				speed=fix_speed(limit,speed);

				rspeed = rspeed==0 ? speed : (rspeed * 0.6 + speed * 0.4);
                double sub=std::abs(rspeed-limit);
				if(sub>eps){
                    cal_speed(sub,rspeed,limit,speed_k);
                    int next_stop=stop+100*speed_k;
					stop = std::max(0,next_stop);
					if(next_stop<0&&r_buf_size==LOWERSIZE){
						r_buf_size=MAXSIZE;
						printf("[!] Change r_buf_size %d\n",r_buf_size);
					}
					if(stop>20000&&r_buf_size==MAXSIZE){
						r_buf_size=LOWERSIZE;
						stop=0;
						printf("[!] Change r_buf_size %d\n",r_buf_size);
					}
				}
				//printf("[!] %3d bytes use %9lld ns, aka %5.4f kb/s %6d\n",byte_num,(long long)ns.count(),rspeed,stop);
				ed=std::chrono::high_resolution_clock::now()-e;
			}
		} else {
			printf("[x] Select() returns -1. ERROR!\n");
			return -1;
		}
	}
	return 0;
}

int create_server(int port) {
	int listenfd;
	struct sockaddr_in servaddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);
	int bsz = 8;
	if(setsockopt(listenfd,SOL_SOCKET,SO_RCVBUF,&bsz,sizeof(bsz)) < 0)
	{
		perror("SO_RCVBUF failed. Error");
		return -1;
	}
	printf("[!] SO_RCVBUF %d\n",bsz);
	if (bind(listenfd, (struct sockaddr *)&servaddr , sizeof(servaddr)) < 0) {
		//print the error message
		perror("bind failed. Error");
		return -1;
	}

	listen(listenfd, 3);
	return listenfd;
}

