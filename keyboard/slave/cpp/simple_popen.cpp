#include <string>
#include <iostream>
#include <stdexcept>

std::string pythonWrapper(const char* cmd) {
  // https://stackoverflow.com/questions/478898/how-to-execute-a-command-and
  // -get-output-of-command-within-c-using-posix

  char buffer[128];
  std::string result = "";
  FILE* pipe = popen(cmd, "r");
  if (!pipe) throw std::runtime_error("popen() failed!");
  try {
    while (!feof(pipe)) {
      if (fgets(buffer, 128, pipe) != NULL)
        result += buffer;
    }
  } catch (...) {
    pclose(pipe);
    throw;
  }
  pclose(pipe);
  return result;
}

int main() {
	std::string result = pythonWrapper("python ../encryption/AES.py");
	//std::cout << "check" << std::endl;
	std::cout << result << std::endl;
	return 0;
}
