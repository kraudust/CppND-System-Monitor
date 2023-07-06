#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include "format.h"

std::string Format::ElapsedTime(long seconds)
{
  std::chrono::seconds chrono_secs{seconds};
  std::chrono::hours chrono_hrs = std::chrono::duration_cast<std::chrono::hours>(chrono_secs);
  chrono_secs -= std::chrono::duration_cast<std::chrono::seconds>(chrono_hrs);
  std::chrono::minutes chrono_mins = std::chrono::duration_cast<std::chrono::minutes>(chrono_secs);
  chrono_secs -= std::chrono::duration_cast<std::chrono::seconds>(chrono_mins);

  std::stringstream ss{};
  ss << std::setw(3) << std::setfill('0') << chrono_hrs.count()   // HHH
     << std::setw(1) << ":"                                       // :
     << std::setw(2) << std::setfill('0') << chrono_mins.count()  // MM
     << std::setw(1) << ":"                                       // :
     << std::setw(2) << std::setfill('0') << chrono_secs.count(); // SS
  return ss.str();
}
