/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2018-2023 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "Subprocess.h"
#include <cerrno>
#if defined(__PLUMED_HAS_SUBPROCESS) || defined(__PLUMED_HAS_DLOPEN)
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
#endif

namespace PLMD {

int Subprocess::run(const std::string& executable,
                    const std::vector<std::string>& arguments,
                    const std::map<std::string,std::string>& overrides) {
#if defined(__PLUMED_HAS_SUBPROCESS) || defined(__PLUMED_HAS_DLOPEN)
  std::vector<char*> argv {const_cast<char*>(executable.c_str())};
  for(const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }
  argv.push_back(nullptr);

  std::vector<std::string> environment;
  for(char** entry=environ; entry && *entry; ++entry) {
    const std::string setting(*entry);
    if(overrides.count(setting.substr(0,setting.find('=')))==0) {
      environment.push_back(setting);
    }
  }
  for(const auto& entry : overrides) {
    if(entry.first.empty() || entry.first.find('=')!=std::string::npos) {
      errno=EINVAL;
      return -1;
    }
    environment.push_back(entry.first+"="+entry.second);
  }
  std::vector<char*> envp;
  for(auto& entry : environment) {
    envp.push_back(entry.data());
  }
  envp.push_back(nullptr);

  pid_t child;
  const int error=posix_spawn(&child,executable.c_str(),nullptr,nullptr,argv.data(),envp.data());
  if(error!=0) {
    errno=error;
    return -1;
  }
  int status=0;
  pid_t result;
  do {
    result=waitpid(child,&status,0);
  } while(result<0 && errno==EINTR);
  return result<0 ? -1 : status;
#else
  errno=ENOSYS;
  return -1;
#endif
}

}
