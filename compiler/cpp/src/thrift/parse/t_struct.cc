#include "thrift/main.h"
#include "thrift/parse/t_doc.h"
#include "thrift/parse/t_scope.h"
#include "thrift/parse/t_base_type.h"
#include "thrift/parse/t_typedef.h"
#include "thrift/parse/t_enum.h"
#include "thrift/parse/t_const.h"
#include "thrift/parse/t_struct.h"
#include "thrift/parse/t_program.h"

bool t_struct::append(t_field* elem) {
  if(program_->should_drop(elem)) return true;
  typedef members_type::iterator iter_type;
  std::pair<iter_type, iter_type> bounds = std::equal_range(members_in_id_order_.begin(),
															members_in_id_order_.end(),
															elem,
															t_field::key_compare());
  if (bounds.first != bounds.second) {
	return false;
  }
  // returns false when there is a conflict of field names
  if (get_field_by_name(elem->get_name()) != NULL) {
	return false;
  }
  members_.push_back(elem);
  members_in_id_order_.insert(bounds.second, elem);
  validate_union_member(elem);
  elem->parent_struct_ = this;
  return true;
}
