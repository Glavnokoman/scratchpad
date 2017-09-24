#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <map>
#include <string>
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
	enum MODE {CLIENT_SPEAK, CLIENT_LISTEN, SERVER_SPEAK, SERVER_LISTEN};
	const size_t FRAME_SIZE = 1024;

	/// Handle command line parameters
	struct Params{
		Params(int argc, char* argv[]): _pname(argv[0]){
			if(argc != 3){
				usage();
				exit(1);
			}

			auto str_modes = std::map<std::string, MODE>{
				  {"CLIENT_SPEAK", CLIENT_SPEAK},   {"CLIENT_LISTEN", CLIENT_LISTEN}
				, {"SERVER_LISTEN", SERVER_LISTEN}, {"SERVER_SPEAK", SERVER_SPEAK}};

			auto mode_it = str_modes.find(argv[1]);
			if(mode_it != end(str_modes)){
				_mode = mode_it->second;
			} else {
				usage();
				exit(1);
			}

			_filepath = argv[2];
		}

		auto usage() const-> void {
			std::cout << "create client and server exchanging random data via socket" << "\n";
			std::cout << "usage: " << _pname
			          << " {CLIENT_SPEAK|SERVER_LISTEN|CLIENT_LISTEN|SERVER_SPEAK} "
			             "path/to/socket" << "\n";
		}

		auto mode() const { return _mode; }
		auto filepath() const { return _filepath; }

		const char* _pname;    ///< program name
		const char* _filepath; ///< file path used as an interface to socket
		MODE _mode;            ///< execution mode
	}; // struct Params

	/// throw std::runtime_error with the given message and last errno description
	__attribute__ ((noreturn))
	auto throwup(const std::string& mess){
		throw std::runtime_error(mess + " " +  std::strerror(errno));
	}

	auto throwup(bool c, const std::string& mess){
		if(c){ throwup(mess); }
	}

	/// Creates unix socket and binds it to a given path.
	/// If path is nullptr, binds to some temporary file.
	/// @return socket descriptor
	auto bind_local_udp(const char* path)-> int {
		auto s = socket(AF_LOCAL, SOCK_DGRAM, 0);
		throwup(s < 0, "failed create socket");

		const char* p = (path ? path : tmpnam(nullptr));

		auto local = sockaddr_un{};
		local.sun_family = AF_LOCAL;
		strncpy(local.sun_path, p, sizeof(local.sun_path));
		unlink(local.sun_path);
		if(bind(s, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0){
			throwup("failed connecting socket to tmp address");
		}

		return s;
	}

	/// Busy produce 1kB messages. epolls on the given port and sends the last produced message
	/// once that becomes writable.
	__attribute__ ((noreturn))
	auto speak(int sck, sockaddr* remote, socklen_t size_remote){
		auto epollfd = epoll_create1(0);
		throwup(epollfd == -1, "epoll_create1");

		auto ev = epoll_event{};
		ev.events = EPOLLOUT;
		ev.data.fd = sck;
		if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sck, &ev) == -1){
			throwup("epoll_ctl: listen_sock");
		}

		auto i = uint8_t(0);
		for(;;){
			auto buf = std::vector<uint8_t>(FRAME_SIZE, i++);

			auto epoll_events = std::array<epoll_event, 10>{};
			auto nfds = epoll_wait(epollfd, epoll_events.data(), 10, 0);
			throwup(nfds == -1, "epoll_wait");
			if(nfds != 0){
				sendto(sck, buf.data(), buf.size()*sizeof(buf[0]), 0, remote, size_remote);
			}
		}
	}

	/// Listen to a given socket. Gets the message, prints first letter of it, waits for 1 second.
	/// Models consumer, doing heavy computation on a message.
	__attribute__ ((noreturn))
	auto listen(int sck){
		for(;;){
			auto buf = std::vector<uint8_t>(FRAME_SIZE);
			if(recv(sck, buf.data(), buf.size()*sizeof(buf[0]), 0) == -1){
				throwup("receive error");
			}
			std::cout << buf[0] << std::endl;
			std::this_thread::sleep_for(1s);
		}
	}

	/// run the client in transmission mode
	__attribute__ ((noreturn))
	auto client_speak(const char* path){
		auto s = bind_local_udp(nullptr);

		auto remote = sockaddr_un{};
		remote.sun_family = AF_LOCAL;
		strncpy(remote.sun_path, path, sizeof(remote.sun_path));

		speak(s, reinterpret_cast<sockaddr*>(&remote), sizeof(remote));
	}

	/// run the client in receiver mode
	__attribute__ ((noreturn))
	auto client_listen(const char* path){
		auto s = bind_local_udp(nullptr);

		auto remote = sockaddr_un{};
		remote.sun_family = AF_LOCAL;
		strncpy(remote.sun_path, path, sizeof(remote.sun_path));

		sendto(s, "hi", 3, 0, reinterpret_cast<sockaddr*>(&remote), sizeof(remote));
		std::cout << "sent 'hi' to: " << remote.sun_path << std::endl; // debug

		listen(s);
	}

	/// run the server in receiver mode.
	__attribute__ ((noreturn))
	auto server_listen(const char* path){
		auto s = bind_local_udp(path);

		listen(s);
	}

	/// run the server in transmission mode
	__attribute__ ((noreturn))
	auto server_speak(const char* path){
		auto s = bind_local_udp(path);

		auto remote = sockaddr_un{};
		auto remote_len = socklen_t(sizeof(remote));
		auto buf = std::array<char, 3>{};
		if(recvfrom(s, buf.data(), 3, 0, reinterpret_cast<sockaddr*>(&remote), &remote_len) == -1){
			throwup("recvfrom()");
		}
		std::cout << "received '" << std::string(begin(buf), end(buf))
		          << "' from " << remote.sun_path << std::endl; // debug

		speak(s, reinterpret_cast<sockaddr*>(&remote), remote_len);
	}
} // namespace


auto main(int argc, char* argv[]) -> int {
	ios_base::sync_with_stdio(false);

	const auto p = Params(argc, argv);
	switch(p.mode()){
		case MODE::CLIENT_SPEAK:
			client_speak(p.filepath());
		case MODE::SERVER_LISTEN:
			server_listen(p.filepath());
		case MODE::CLIENT_LISTEN:
			client_listen(p.filepath());
		case MODE::SERVER_SPEAK:
			server_speak(p.filepath());
	}

	return 0;
}
