#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <forward_list>
#include <unordered_map>

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
void write_to_log(const std::string&);
void run(const process&, int position);

std::vector<pid_t> children{};
std::string file_configuration{};
std::vector<process> processes{};

void start_huinit() {
  write_to_log("start huinit\n");
  processes = {};

  {
    std::ifstream input{file_configuration};

    if(!input) {
      write_to_log("fail to get access for read to " + file_configuration + '\n');

      return;
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

  children = std::vector(processes.size(), (const pid_t)-1);

  for(int position_process{}; position_process < processes.size(); ++position_process) {
    write_to_log("launch process at position " + std::to_string(position_process) + "\n");
    run(processes.at(position_process), position_process);
  }

  while (!processes.empty()) {
    const pid_t changed_state{waitpid(-1, NULL, 0)};

    if(changed_state < 0) {
      // ...
    }

    for(int position{}; position < children.size(); ++position) {
      if(children.at(position) == changed_state) {
        children.at(position) = (const pid_t)-1;
        write_to_log("relaunch process at position " + std::to_string(position) + '\n');
        run(processes.at(position), position);
      }
    }
  }
}

void handle_sighup(int number) {
  write_to_log("sighup, restart\n");

  for (const pid_t child : children) {
    if(child == (pid_t)-1) {
      continue;
    }

    const int status{kill(child, SIGTERM)};

    if (status < 0) {
      // ...
    }

    else {
      write_to_log("kill " + std::to_string(child) + '\n');
    }
  }

  start_huinit();
}

int main(const int amounts_arguments, const char* const arguments[]) {
  if(amounts_arguments <= 1) {
    std::cerr << "configuration isn't defined" << '\n';
    return 1;
  }

  if (signal(SIGHUP, handle_sighup) == SIG_ERR) {
    return 1;
  }

  daemonize();
  close_files();
  change_directory();

  file_configuration = arguments[1];
  start_huinit();
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

  for(int descriptor{0}; descriptor < maximum_file_descriptor_number.rlim_max; ++descriptor ) {
    (const void)close(descriptor);
  }
}

void write_to_log(const std::string& information) {
  const int descriptor_log = open("/tmp/huinit.log", O_CREAT | O_WRONLY | O_APPEND, 0600);

  if(descriptor_log < 0) {
    throw std::runtime_error{"fail to create log"};
  }

  (const void)write(descriptor_log, information.c_str(), information.size());

  { const int status{fsync(descriptor_log)};

    if(status < 0) {
      // ...
    }
  }

  (const void)close(descriptor_log);
}

void run(const process& process, int position) {
  if(children.at(position) != (const pid_t)-1) {
    return;
  }

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

  { pid_t identifier{fork()};

    if(identifier == 0) {
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
      children.at(position) = identifier;

      // for(int position{1}; position < amount_arguments - 1; ++position) {
      //   delete arguments[position];
      // }

      delete[] arguments;
    }

    else {
      throw std::runtime_error{"fail to create child process"};
    }
  }
}
