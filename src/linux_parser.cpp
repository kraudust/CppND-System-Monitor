#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

#include "linux_parser.h"

std::string LinuxParser::OperatingSystem()
{
  std::string line;
  std::string key;
  std::string value;
  std::ifstream stream(kOSPath);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          stream.close();
          return value;
        }
      }
    }
    stream.close();
  }
  return value;
}

std::string LinuxParser::Kernel()
{
  std::string os, kernel, version;
  std::string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
    stream.close();
  }
  return kernel;
}

// BONUS: Update this to use std::filesystem
std::vector<int> LinuxParser::Pids()
{
  std::vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      std::string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

float LinuxParser::MemoryUtilization()
{
  const std::string memTotal = "MemTotal:";
  const std::string memAvailable = "MemAvailable:";
  float mem_total = findValueByKey<float>(memTotal, kMeminfoFilename);
  float mem_available = findValueByKey<float>(memAvailable, kMeminfoFilename);
  return (mem_total - mem_available) / mem_total;
}

long LinuxParser::UpTime()
{
  return getValueOfFile<long>(kUptimeFilename);
}

long LinuxParser::Jiffies()
{
  return ActiveJiffies() + IdleJiffies();
}

long LinuxParser::ActiveJiffies(int pid)
{
  std::string line;
  std::string line_element;
  long utime;
  long stime;
  long cutime;
  long cstime;
  std::ifstream stream(kProcDirectory + '/' + std::to_string(pid) + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    // We don't care about the first 12 elements, so stream those away
    for (int i = 0; i < 13; ++i) {
      linestream >> line_element;
    }
    linestream >> utime; // 13th element (0 indexed)
    linestream >> stime; // 14th element (0 indexed)
    linestream >> cutime; // 15th element (0 indexed)
    linestream >> cstime; // 16th element (0 indexed)
    stream.close();
  }
  return utime + stime + cutime + cstime;
}

long LinuxParser::ActiveJiffies()
{
  std::vector<long> cpu_util_vec = CpuUtilization();
  return (
    cpu_util_vec[CPUStates::kUser_] + cpu_util_vec[CPUStates::kNice_] +
    cpu_util_vec[CPUStates::kSystem_] + cpu_util_vec[CPUStates::kIRQ_] +
    cpu_util_vec[CPUStates::kSoftIRQ_] + cpu_util_vec[CPUStates::kSteal_]);
}

long LinuxParser::IdleJiffies()
{
  std::vector<long> cpu_util_vec = CpuUtilization();
  return cpu_util_vec[CPUStates::kIdle_] + cpu_util_vec[CPUStates::kIOwait_];
}

std::vector<long> LinuxParser::CpuUtilization()
{
  std::vector<long> cpu_utilization;
  long cpu_element;
  std::string key_name;
  std::string line;
  std::ifstream stream(kProcDirectory + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> key_name;
    while (linestream >> cpu_element) {
      cpu_utilization.push_back(cpu_element);
    }
    stream.close();
  }
  return cpu_utilization;
}

int LinuxParser::TotalProcesses()
{
  const std::string key{"processes"};
  return findValueByKey<int>(key, kStatFilename);
}

int LinuxParser::RunningProcesses()
{
  const std::string key{"procs_running"};
  return findValueByKey<int>(key, kStatFilename);
}

std::string LinuxParser::Command(int pid)
{
  const std::string filename{'/' + std::to_string(pid) + kCmdlineFilename};
  return getValueOfFile<std::string>(filename);
}

std::string LinuxParser::Ram(int pid)
{
  int ram = 0;
  const std::string key{"VmRSS:"}; // VmSize is sum of all virtual memory, we want physical ram (VmRSS)
  const std::string filename{'/' + std::to_string(pid) + kStatusFilename};
  ram = findValueByKey<int>(key, filename);
  return std::to_string(ram / 1000);
}

std::string LinuxParser::Uid(int pid)
{
  const std::string key{"Uid:"};
  const std::string filename{'/' + std::to_string(pid) + kStatusFilename};
  return findValueByKey<std::string>(key, filename);
}

std::string LinuxParser::User(int pid)
{
  std::string p_uid = Uid(pid);
  std::string user;
  std::string x;
  std::string user_uid;
  std::string line;
  std::ifstream stream(kPasswordPath);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      linestream >> user >> x >> user_uid;
      if (user_uid == p_uid) {
        break;
        stream.close();
      }
    }
    stream.close();
  }
  return user;
}

long LinuxParser::UpTime(int pid)
{
  std::string line;
  std::string line_element;
  long start_time;
  std::ifstream stream(kProcDirectory + '/' + std::to_string(pid) + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    // We don't care about the first 20 elements, so stream those away
    for (int i = 0; i < 21; ++i) {
      linestream >> line_element;
    }
    // The 21st element (0 indexed) is the process starttime
    linestream >> start_time;
    stream.close();
  }

  // Get system uptime
  long sys_uptime = UpTime();
  return sys_uptime - (start_time / sysconf(_SC_CLK_TCK));
}
