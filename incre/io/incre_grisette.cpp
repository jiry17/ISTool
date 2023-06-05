#include "istool/incre/io/incre_grisette.h"
#include "istool/incre/language/incre.h"
#include "glog/logging.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>

using namespace incre;
/*
void incre::getVarFromTerm(const std::shared_ptr<TermData> &term,
  TypeContext* ctx, std::vector<std::vector<std::string> > &result) {
    if (term->getType() == TermType::LET) {
      auto* tm_let = dynamic_cast<TmLet*>(term.get());
    }
  }

void incre::getVarFromBinding(const std::shared_ptr<BindingData> &binding, 
  TypeContext* ctx, std::vector<std::vector<std::string> > &result) {
    if (binding->getType() == BindingType::TERM) {
      auto* term_binding = dynamic_cast<TermBinding*>(binding.get());
      getVarFromTerm(term_binding->term, ctx, result);
    }
  }

void incre::getVarFromCommand(const std::shared_ptr<CommandData> &command, 
  TypeContext* ctx,  const std::function<Ty(const Term&, TypeContext*)>& type_checker,
  std::vector<std::vector<std::string> > &result) {
    if (command->getType() == CommandType::BIND) {
      auto* command_bind = dynamic_cast<CommandBind*>(command.get());
      getVarFromBinding(command_bind->binding, ctx, result);
    }
  }

void incre::getVarFromProgram(const std::shared_ptr<ProgramData> &prog, 
  std::vector<std::vector<std::string> > &result) {
    auto* ctx = new TypeContext();
    ExternalTypeMap ext({{TermType::LABEL, ExternalTypeRule{label}},
      {TermType::ALIGN, ExternalTypeRule{align}}});
    auto type_checker = [ext](const Term& term, TypeContext* ctx) {
        return incre::getType(term, ctx, ext);
    }
    
    for (auto& command: prog->commands) {
      getVarFromCommand(command, ctx, type_checker);
      ctx->clear();
    }
    delete ctx;
  }
  */
//term中let和abs类型的话，term中包含的name就是需要的那个name
// 遇到align就把ctx中的内容放到result中
// command只需要考虑BIND，binding只需要考虑TermBinding，因为需要的东西在Term里面才有