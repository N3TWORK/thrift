
#include "thrift/parse/t_enum.h"
#include "thrift/parse/t_program.h"

void t_enum::do_value_drops(t_program *p) {
  int keep = 0;
  for(int i = 0; i < constants_.size(); i++) {
    if(!p->should_drop(constants_[i])) {
      constants_[keep++] = constants_[i];
    }
  }
  constants_.resize(keep);
}
