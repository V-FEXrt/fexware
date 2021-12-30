#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "layer.h"
#include "operation.h"
#include "tokenizer.h"

namespace fex
{

  typedef std::unordered_map< // Row x Key map
      int,
        std::unordered_map< // Operation map
          Operation,
          std::vector< // Tokens representing the action
            Token>>>
      Binding;
  typedef std::vector<Binding> BindingList;
  typedef std::vector<Token> TopLevel;

	std::string parse_source(const std::string &source , Layer* layer);

}

#endif