
/* parser.h */

#ifndef __PARSER_H_
#define __PARSER_H_

#include <stdbool.h>

#include "stream.h"
#include "ptree.h"

bool parseModule(Stream* s, Node* out);

#endif

