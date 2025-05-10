#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>

#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <fcntl.h>

struct command
{
  std::string file_executable{};
  std::vector<std::string> arguments{};
};

struct process {
  command command{};
  std::string file_stream_input{};
  std::string file_stream_output{};
};

void change_directory();
void daemonize();
void close_files();
void create_log();

int main(const int amounts_arguments, const char* const arguments[]) {
  if(amounts_arguments <= 1) {
    std::cerr << "configuration isn't defined" << '\n';

    return 1;
  }

  std::ifstream input{arguments[1]};

  if(!input) {
    std::cerr << "fail to get access for read to " << arguments[1] << '\n';

    return 1;
  }

  for(std::string line{}; std::getline(input, line);) {
    std::stringstream input{line};
    process process{};
    std::vector<std::string> read(1);

    input >> process.command.file_executable;

    while(input >> read.back()) {
      read.emplace_back();
    }

    read.pop_back();
    process.file_stream_input = read.at(read.size() - 2);
    process.file_stream_output = read.at(read.size() - 1);
    read.pop_back();
    read.pop_back();
    read.shrink_to_fit();
    process.command.arguments = std::move(read);
    std::cerr << "executable file: " << process.command.file_executable << '\n';

    for (const auto& argument : process.command.arguments) {
      std::cerr << "argument: " << argument << '\n';
    }

    std::cerr << "input stream file: " << process.file_stream_input
      << '\n' << "output stream file: " << process.file_stream_output << '\n';
  }

  std::cerr << "daemonize\n";
  daemonize();
  std::cerr << "close files\n";
  close_files();
  std::cerr << "change directory\n";
  change_directory();
  std::cerr << "create log\n";
  create_log();

  return 0;
}

void change_directory() {
  const int status = chdir("/");

  if(status != 0) {
    throw std::runtime_error{"fail to change current working directory"};
  }
}

void daemonize() {
  const pid_t child{fork()};

  if(child == (pid_t)-1) {
    throw std::runtime_error{"fail to create child process"};
  }

  else if(child != 0) {
    exit(0);
  }

  const pid_t session{setsid()};

  if(session == (pid_t)-1) {
    throw std::runtime_error{"fail to create session"};
  }
}

void close_files() {
  struct rlimit maximum_file_descriptor_number{};
  const int status{getrlimit(RLIMIT_NOFILE, &maximum_file_descriptor_number)};

  if(status != 0) {
    throw std::runtime_error{"fail to get a maximum file descriptor number"};
  }

  for(int descriptor{3}; descriptor < maximum_file_descriptor_number.rlim_max; ++descriptor ) {
    (const void)close(descriptor);
  }
}

void create_log() {
  const int descriptor_log = open("/tmp/huinit.log", O_CREAT | O_TRUNC | O_WRONLY, 0600);

  if(descriptor_log < 0) {
    throw std::runtime_error{"fail to create log"};
  }

  (const void)write(descriptor_log, "start\n", 6);
  (const void)close(descriptor_log);
}
