#include "client.h"


using namespace http;


Client::Client(int socket_ID, sockaddr_in TCP_struct)
{
	// this->info = std::make_unique<Client_Data>();
	this->socket_ID = socket_ID;
	this->TCP_struct = TCP_struct;

	IP4_address =
		std::string(inet_ntoa(this->TCP_struct.sin_addr));

	this->session_start_time =
		std::string("*session_start_time 12.34*"); // TO-DO

	this->read_buffer.resize(BUFFER_SIZE);
	this->request_count = 0;
	this->time_alive = 0.0;
	this->client_alive = true;
}




/**********************************************************************************/
void Client::main_loop()
{
	log_connection();
	while (client_alive)
	{
		if (new_message()) {
			process_request();
		}
	}
}
/**********************************************************************************/


bool Client::new_message()
{
	int msg_len = 0;
	read_buffer.resize(BUFFER_SIZE);

	try {
		if (msg_len = read(
			socket_ID, read_buffer.data(), BUFFER_SIZE - 1) <= 0) 
			throw std::runtime_error("\nclient quit!");
	}
	catch (const std::runtime_error& e) {
		client_alive = false;
		LOG(e.what());
		return false;
	}

	session_requests.emplace_back(Request(read_buffer.data()));

	return true;
}


void Client::LOG(const char* out)
{
	#if TERMINAL_OUT
	std::lock_guard<std::mutex> gaurd(client_lock);
	std::cout << out << '\n';
	#endif
}


void Client::log_connection()
{
	#if TERMINAL_OUT
	std::lock_guard<std::mutex> gaurd(client_lock);
	std::cout << "\nnew client: " << IP4_address << '\n';
	#endif
}


void Client::process_request()
{
	Request& curr_req = session_requests.back();

	if (curr_req.get_method() == "GET" && 
	    curr_req.get_path() == "/" && 
		curr_req.clean()) { 
			homepage_response(); 
	} 
	else if (curr_req.get_method() == "GET" && 
	         curr_req.get_path()[0] == '/' &&
			 curr_req.get_path().size() > 1 && 
			 curr_req.clean()) { 
			 	unique_response();
	}
	else {
		error_response(BAD_REQUEST);
	}
}


void Client::error_response(int http_code)
{
	LOG("error");
	unique_response();
	session_responses.emplace_back(Response(http_code));
}


bool Client::read_from_file(int file_fd, char* content_buffer, int* file_size) 
{
	int _read = 1;

	while (_read > 0) { 
		if (_read = read(file_fd, (void*)content_buffer, 10) == -1) {
			printf("HERE\n");
			perror("read");
			exit(1);
			return false;
		}
		else { *file_size += _read; }
	}

	return true;
}


void Client::homepage_response()
{
	LOG("nice index");
	std::string home = std::string(ROOT) + std::string(HOMEPAGE);
	const char *home_c = home.c_str();
	int fd = open(home_c, O_RDONLY, 0);
	char buffer[10];
	int bytes_read = 1;

	while (bytes_read > 0)
	{
		bytes_read = read(fd, buffer, 10);
		write(this->socket_ID, buffer, bytes_read);
	}

	session_responses.emplace_back(Response(OK, "index.html"));
}


void Client::unique_response()
{
	LOG("unique");

	int file_fd = search();
	if (file_fd > -1) {

		int bytes_read = 1;
		char buffer[10];

		while (bytes_read > 0)
		{
			bytes_read = read(file_fd, buffer, 10);
			write(this->socket_ID, buffer, bytes_read);
		}

		session_responses.emplace_back(Response(OK));
		/* else { error_response(INTERNAL_SERVER_ERROR); }
		   need to come back for sys call error handling
		*/
	}
	else { error_response(NOT_FOUND); }
	close(file_fd);
}


bool Client::is_alive()
{
	return client_alive;
}


int Client::get_count()
{
	return request_count;
}


void Client::deny()
{
	write(socket_ID, BANNED_IP_MSG, strlen(BANNED_IP_MSG));
	client_alive = false;
	close(this->socket_ID);

	// need general purpose logger based on #define AUTOLOG
	// LOG("banned IP denied: " << IP4_address << "\n\n";
}


int Client::search()
{
	DIR* root_ptr = opendir(ROOT);
	struct dirent* entry = nullptr;

	auto path = session_requests.back().get_path();
	path.erase(0, 1);
	int target_fd = 0;

	while ((entry = readdir(root_ptr)) != NULL && target_fd == 0) 
	{
		if (entry->d_name == path) {
			if (entry->d_type == DT_REG) 
			{
				std::string full_path = ROOT + '/' + path;
				const char* c_path = full_path.c_str();
				target_fd = open(c_path, O_RDONLY, 0644);
			}
			else if (entry->d_type == DT_DIR) {
				target_fd = -2;  // flag for now
			}
		}
	}
	
	// std::cout << target_fd << '\n';
	return target_fd;
}


const std::string& Client::get_IP() const
{
	return IP4_address;
}


void Client::client_end_log()
{
	std::lock_guard<std::mutex> gaurd(client_lock);
	std::cout << "client log:\n"
		      << "[ " << IP4_address << " ]"
		      << "joined at: " << session_start_time << "\n\n";

	std::for_each(session_requests.begin(), 
			      session_requests.end(),
				 [&](Request request) { std::cout << request; }
	);
}


void Client::log_data__()
{
	std::cout << "session start: " << session_start_time << "\n";
	int len = this->session_requests.size();

	for (int i = 0; i < len; ++i) {
		std::cout << session_requests[i];
		std::cout << session_responses[i];
	}
}
