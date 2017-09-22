#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <thread>
#include <vector>

// posix stuff
#include <fcntl.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <signal.h>
#include <unistd.h>

using namespace std;

namespace{
	enum MODE {CLIENT, SERVER};
	const size_t FRAME_SIZE = 1024;
	
	__attribute__ ((noreturn))
	auto throwup(const std::string& mess){
		throw std::runtime_error(mess + " " +  std::strerror(errno));
	}
	
	auto throwup(bool c, const std::string& mess){
		if(c){ throwup(mess); }
	}
	
	/// Handle command line parameters
	struct Params{
		Params(int argc, char* argv[]): _pname(argv[0]){
			if(argc != 3){
				usage();
				exit(1);
			}
			
			if(strcmp(argv[1], "CLIENT") == 0){
				_mode = MODE::CLIENT;
			} else if(strcmp(argv[1], "SERVER") == 0){
				_mode = MODE::SERVER;
			} else {
				usage();
				exit(1);
			}
			
			_filepath = argv[2];
		}
		
		auto usage() const-> void {
			std::cout << "create client and server exchanging random data via socket" << "\n";
			std::cout << "usage: " << _pname << " {CLIENT|SERVER} path/to/socket" << "\n";
		}
		
		auto mode() const { return _mode; }
		auto filepath() const { return _filepath; }
		
		const char* _pname;    ///< program name
		const char* _filepath; ///< file path used as an interface to socket
		MODE _mode;            ///< execution mode
	}; // struct Params
	
	/// run the client
	__attribute__ ((noreturn))
	auto client_speak(const char* path){
		auto s = socket(AF_LOCAL, SOCK_DGRAM, 0);
		throwup(s < 0, "failed create socket");
		
		auto local = sockaddr_un{};
		local.sun_family = AF_LOCAL;
		strncpy(local.sun_path, tmpnam(nullptr), sizeof(local.sun_path));
		if(bind(s, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0){
			throwup("failed connecting socket to tmp address");
		}
		
		auto remote = sockaddr_un{};
		remote.sun_family = AF_LOCAL;
		strncpy(remote.sun_path, path, sizeof(remote.sun_path));

		auto epollfd = epoll_create1(0);
		throwup(epollfd == -1, "epoll_create1");
		
		auto ev = epoll_event{};
		ev.events = EPOLLOUT;
		ev.data.fd = s;
		if(epoll_ctl(epollfd, EPOLL_CTL_ADD, s, &ev) == -1){
			throwup("epoll_ctl: listen_sock");
		}
		
		auto i = uint8_t(0);
		for(;;){
			auto buf = std::vector<uint8_t>(FRAME_SIZE, i++);
			
			auto epoll_events = std::array<epoll_event, 10>{};
			auto nfds = epoll_wait(epollfd, epoll_events.data(), 10, 0);
			throwup(nfds == -1, "epoll_wait");
			if(nfds != 0){
				sendto(s, buf.data(), buf.size()*sizeof(buf[0]), 0, reinterpret_cast<sockaddr*>(&remote), sizeof(remote));
			}
		}
	}
	
	/// run the server. set up the socket, bind to path, receive...
	__attribute__ ((noreturn))
	auto server_listen(const char* path){
		auto s = socket(AF_LOCAL, SOCK_DGRAM, 0);
		if(s < 0){
			throwup("failed to open socket");
		}
		
		auto local = sockaddr_un{};
		local.sun_family = AF_LOCAL;
		strncpy(local.sun_path, path, sizeof(local.sun_path));
		unlink(local.sun_path);
		
		if(bind(s, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0){
			throwup("failed to bind server socket");
		}
		
		for(;;){
			auto buf = std::vector<uint8_t>(FRAME_SIZE); // dgs if i++ wraps around
			if(recv(s, buf.data(), buf.size()*sizeof(buf[0]), 0) < 0) {
				throwup("receive error: ");
			}
			std::cout << buf[0] << std::endl;
			std::this_thread::sleep_for(1s);
		}
	}
} // namespace


auto main(int argc, char* argv[]) -> int {
	ios_base::sync_with_stdio(false);

	const auto p = Params(argc, argv);
	switch(p.mode()){
		case MODE::CLIENT:
			client_speak(p.filepath());
			break;
		case MODE::SERVER:
			server_listen(p.filepath());
			break;
	}

	return 0;
}
