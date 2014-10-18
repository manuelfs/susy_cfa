#ifndef H_GEN_CFA
#define H_GEN_CFA

#include <set>
#include <utility>
#include <string>

#include "TChain.h"

typedef std::pair<std::string, std::string> Var;

void GetVariables(TChain *chain, std::set<Var>& vars);
std::string AllCaps(std::string str);

#endif
