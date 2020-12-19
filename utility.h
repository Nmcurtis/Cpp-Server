// Standard library includes:
#include <mutex>
#include <iostream>
#include <exception>
#include <string>


// Program level globals:
extern std::mutex cout_mutex;

/* Utility objects: */

// A simple class for server error exceptions that inherits from std::exception
class Server_Error : public std::exception {
public:
	Server_Error(const std::string& in_msg = "") :
		msg(in_msg)
	{}

	const char* what() const noexcept override {
    return msg.c_str();
  }

private:
	const std::string msg;
};


// Templated print function, thread safe:
template <typename T>
void thread_safe_print(const T& to_print, bool trailing_endline = true) {
  std::lock_guard<std::mutex> cout_lock(cout_mutex);

  std::cout << to_print;
  if (trailing_endline) {
    std::cout << std::endl;
  }
}
