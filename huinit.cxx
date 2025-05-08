#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

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

int main(const int amounts_arguments, const char* const arguments[]) {
  if(amounts_arguments <= 1) {
    return 1;
  }

  std::ifstream input{arguments[1]};

  if(!input) {
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
    std::cerr << "executable file:" << process.command.file_executable << '\n';

    for (const auto& argument : process.command.arguments) {
      std::cerr << "argument: " << argument << '\n';
    }

    std::cerr << "input stream file: " << process.file_stream_input
      << '\n' << "output stream file: " << process.file_stream_output << '\n';
  }

  return 0;
}
