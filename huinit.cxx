#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <forward_list>

#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <sys/wait.h>

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
void run(const process&);

std::vector<pid_t> children{};
pid_t identifier{-1};

int main(const int amounts_arguments, const char* const arguments[]) {
  if(amounts_arguments <= 1) {
    std::cerr << "configuration isn't defined" << '\n';

    return 1;
  }

  std::vector<process> processes{};

  {
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

      processes.push_back(process);
    }
  }

  std::cerr << "daemonize\n";
  daemonize();
  std::cerr << "close files\n";
  close_files();
  std::cerr << "change directory\n";
  change_directory();
  std::cerr << "create log\n";
  create_log();
  children = std::vector(processes.size(), -1);

  for(int position_process{}; position_process < processes.size(); ++position_process) {
    run(processes.at(position_process));
  }

  while(true) {
    identifier = waitpid(-1, NULL, 0);

    for(int position{}; position < children.size(); ++position) {
      if(children.at(position) == identifier) {
        // children[position] = (const pid_t)-1;
        // run(processes.at(position));
      }
    }
  }

  return 0;
}

void change_directory() {
  const int status = chdir("/");

  if(status != 0) {
    throw std::runtime_error{"fail to change current working directory"};
  }
}

void daemonize() {
  { const pid_t child{fork()};

    if(child == (const pid_t)-1) {
      throw std::runtime_error{"fail to create child process"};
    }

    else if(child != 0) {
      exit(0);
    }
  }

  { const pid_t session{setsid()};

    if(session == (const pid_t)-1) {
      throw std::runtime_error{"fail to create session"};
    }
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

void run(const process& process) {
  std::string copy_file_executable{process.command.file_executable};
  char* const file_executable{copy_file_executable.data()};
  char** arguments{nullptr};
  const size_t amount_arguments{process.command.arguments.size() + 2};

  try {
    arguments = new char*[amount_arguments]{};
  }

  catch(const std::exception& exception) {
    std::cerr << "exc\n";
  }

  arguments[0] = file_executable;

  { int position_argument{1};

    for(const std::string& argument : process.command.arguments) {
      arguments[position_argument] = (char*)calloc(argument.size() + 1, sizeof(char));
      std::strcpy(arguments[position_argument], argument.c_str());
      ++position_argument;
    }
  }

  arguments[amount_arguments - 1] = nullptr;

  { identifier = fork();

    if(identifier == 0) {
      identifier = getpid();

      { int descriptor_input{-1};

        { const char* const file_stream_input{process.file_stream_input.c_str()};

          descriptor_input = open(file_stream_input, O_RDONLY | O_CREAT, 0644);

          if(descriptor_input < 0) {
            throw std::runtime_error{"fail to open"};
          }
        }

        {
          const int status{dup2(descriptor_input, STDIN_FILENO)};

          if(status < 0) {
            (const void)close(descriptor_input);
            throw std::runtime_error{"fail to duplicate"};
          }
        }

        (const void)close(descriptor_input);
      }

      { int descriptor_output{-1};

        { const char* const file_stream_output{process.file_stream_output.c_str()};

          descriptor_output = open(file_stream_output, O_WRONLY | O_CREAT | O_TRUNC, 0644);

          if(descriptor_output < 0) {
            throw std::runtime_error{"fail to open"};
          }
        }

        {
          const int status{dup2(descriptor_output, STDOUT_FILENO)};

          if(status < 0) {
            (const void)close(descriptor_output);
            throw std::runtime_error{"fail to duplicate"};
          }
        }

        (const void)close(descriptor_output);
      }

      { const int status{execv(file_executable, arguments)};

        if (status == -1) {
          throw std::runtime_error{"fail to run executable"};
        }
      }
    }

    else if(identifier > 0) {
      children.push_back(identifier);
      delete[] arguments;
    }

    else {
      throw std::runtime_error{"fail to create child process"};
    }
  }
}
