#ifndef HELP_H
#define HELP_H

#include <string>

#include "common.h"

std::string usage();

void overview(int status);
void help(master_t master, controller_t controller, int status);

#endif // HELP_H