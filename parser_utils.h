
#ifndef __PARSER_UTILS_H_
#define __PARSER_UTILS_H_

#include "bool.h"
#include "stream.h"
#include "ast.h"
#include "operators.h"

typedef bool (*NodeParser)(Stream*,Node*);
typedef bool (*OperatorParser)(Stream*,Node*,Operator*);

bool readIdentifier(Stream* s, String* out);
bool readNumber(Stream* s, String* out);

bool readWhitespace(Stream* s);

bool readString(Stream* s, const char* pattern);

void expect(Stream* s, const char* pattern);

bool parseTerminated(Stream* s, Node* parent, bool (*parser)(Stream* s, Node* out), const char* terminator);
bool parseSeparated(Stream* s, Node* parent, bool (*parser)(Stream* s, Node* out), const char* separator);
bool childParse(Stream* s, Node* parent, bool (*parser)(Stream* s, Node* out));

bool parseOperatorExpresion(Stream* s, Node* out, bool (*leaf_parser)(Stream* s, Node* out), bool (*operator_parser)(Stream*,Operator*,bool));


#endif

