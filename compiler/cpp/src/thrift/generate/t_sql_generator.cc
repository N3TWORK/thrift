/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sstream>
#include <algorithm>
#include "thrift/platform.h"
#include "thrift/version.h"
#include "thrift/generate/t_generator.h"

using std::map;
using std::ofstream;
using std::ostringstream;
using std::string;
using std::stringstream;
using std::vector;

static const string endl = "\n"; // avoid ostream << std::endl flushes
static const string BASE_SQL_OBJ = "_BASE_TABLE_";

/**
 * Sql ORM code generator.
 *
 */
class t_sql_generator : public t_generator {
public:
  t_sql_generator(t_program* program,
                 const std::map<std::string, std::string>& parsed_options,
                 const std::string& option_string)
    : t_generator(program) {
    std::map<std::string, std::string>::const_iterator iter;

    gen_sqlalchemy_ = true;
    coding_ = "";

    for( iter = parsed_options.begin(); iter != parsed_options.end(); ++iter) {
      if( iter->first.compare("python") == 0) {
        gen_sqlalchemy_ = true;
      } else if( iter->first.compare("coding") == 0) {
        coding_ = iter->second;
      } else {
        throw "unknown option sql:" + iter->first;
      }
    }

    copy_options_ = option_string;
    out_dir_base_ = "gen-py";
  }

  virtual std::string indent_str() const {
    return "    ";
  }

  /**
   * Init and close methods
   */

  void init_generator();
  void close_generator();

  /**
   * Program-level generation functions
   */

  void generate_typedef(t_typedef* ttypedef);
  void generate_enum(t_enum* tenum);
  void generate_const(t_const* tconst);
  void generate_struct(t_struct* tstruct);
  void generate_forward_declaration(t_struct* tstruct);
  void generate_xception(t_struct* txception);
  void generate_service(t_service* tservice);

  std::string render_const_value(t_type* type, t_const_value* value);

  /**
   * Struct generation code
   */

  void generate_py_struct(t_struct* tstruct);
  void generate_py_thrift_spec(std::ofstream& out, t_struct* tstruct, bool is_exception);
  void generate_py_struct_definition(std::ofstream& out,
                                     t_struct* tstruct);
  void generate_py_struct_reader(std::ofstream& out, t_struct* tstruct);
  void generate_py_struct_required_validator(std::ofstream& out, t_struct* tstruct);
  void generate_py_sqlalchemy_table(std::ofstream& out, t_struct* tstruct);

  /**
   * Serialization constructs
   */

  void generate_deserialize_field(std::ofstream& out,
                                  t_field* tfield,
                                  std::string prefix = "");

  void generate_deserialize_struct(std::ofstream& out, t_struct* tstruct, std::string prefix = "");

  void generate_deserialize_container(std::ofstream& out, t_type* ttype, std::string prefix = "");

  void generate_deserialize_set_element(std::ofstream& out, t_set* tset, std::string prefix = "");

  void generate_deserialize_map_element(std::ofstream& out, t_map* tmap, std::string prefix = "");

  void generate_deserialize_list_element(std::ofstream& out,
                                         t_list* tlist,
                                         std::string prefix = "");

  void generate_python_docstring(std::ofstream& out, t_struct* tstruct);

  void generate_python_docstring(std::ofstream& out, t_function* tfunction);

  void generate_python_docstring(std::ofstream& out,
                                 t_doc* tdoc,
                                 t_struct* tstruct,
                                 const char* subheader);

  void generate_python_docstring(std::ofstream& out, t_doc* tdoc);

  /**
   * Helper rendering functions
   */

  std::string py_autogen_comment();
  std::string py_imports();
  std::string render_field_default_value(t_field* tfield);
  std::string type_name(t_type* ttype);
  std::string type_to_enum(t_type* ttype);
  std::string type_to_spec_args(t_type* ttype);
  std::string base_type_to_sql_type(t_type* ttype);
  std::string default_table_fields(std::string table_name);
  std::string sql_table_repr();
  std::string register_pivot_table(std::ostream& append_out,
                                   std::string parent_table,
                                   std::string parent_column,
                                   std::string table_name);
  std::string instantiate_pivot_table(std::string var_name, std::string prefix);
  std::string type_to_sqlalchemy_column(std::ostream& append_out,
                                        t_type* ttype,
                                        std::string table_name,
                                        std::string column_name);

  static std::string get_real_py_module(const t_program* program, std::string package_dir="") {
    std::string real_module = program->get_namespace("py");
    if (real_module.empty()) {
      return program->get_name();
    }
    return package_dir + real_module;
  }

  static bool is_immutable(t_type* ttype) {
    return ttype->annotations_.find("python.immutable") != ttype->annotations_.end();
  }

private:

  /**
   * True if we should generate SQLAlchemy classes.
   */
  bool gen_sqlalchemy_;

  std::string copy_options_;

  /**
   * specify generated file encoding
   * eg. # -*- coding: utf-8 -*-
   */
  string coding_;

  /**
   * File streams
   */

  std::ofstream f_types_;
  std::ofstream f_consts_;
  std::ofstream f_service_;

  std::string package_dir_;
  std::string module_;
};

/**
 * Prepares for file generation by opening up the necessary file output
 * streams.
 *
 * @param tprogram The program to generate
 */
void t_sql_generator::init_generator() {
  // Make output directory
  string module = get_real_py_module(program_);
  package_dir_ = get_out_dir();
  module_ = module;
  while (true) {
    // TODO: Do better error checking here.
    MKDIR(package_dir_.c_str());
    std::ofstream init_py((package_dir_ + "/__init__.py").c_str(), std::ios_base::app);
    init_py.close();
    if (module.empty()) {
      break;
    }
    string::size_type pos = module.find('.');
    if (pos == string::npos) {
      package_dir_ += "/";
      package_dir_ += module;
      module.clear();
    } else {
      package_dir_ += "/";
      package_dir_ += module.substr(0, pos);
      module.erase(0, pos + 1);
    }
  }

  // Make output file
  string f_types_name = package_dir_ + "/" + "ttypes.py";
  f_types_.open(f_types_name.c_str());

  string f_init_name = package_dir_ + "/__init__.py";
  ofstream f_init;
  f_init.open(f_init_name.c_str());
  f_init << "__all__ = ['ttypes'";
  vector<t_service*> services = program_->get_services();
  vector<t_service*>::iterator sv_iter;
  for (sv_iter = services.begin(); sv_iter != services.end(); ++sv_iter) {
    f_init << ", '" << (*sv_iter)->get_name() << "'";
  }
  f_init << "]" << endl;
  f_init.close();

  // Print header
  f_types_ << py_autogen_comment() << endl
           << py_imports() << endl;
  f_types_ << BASE_SQL_OBJ << " = declarative_base()" << endl;

}

/**
 * Autogen'd comment
 */
string t_sql_generator::py_autogen_comment() {
  string coding;
  if (!coding_.empty()) {
      coding = "# -*- coding: " + coding_ + " -*-\n";
  }
  return coding + std::string("#\n") + "# Autogenerated by Thrift Compiler (" + THRIFT_VERSION + ")\n"
         + "#\n" + "# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING\n" + "#\n"
         + "# Author: David the Intern\n" + "#\n"
         + "#  options string: " + copy_options_ + "\n" + "#\n";
}

/**
 * Prints standard thrift imports
 */
string t_sql_generator::py_imports() {
  ostringstream ss;

  // Sqlalchemy imports
  ss << "import sqlalchemy as db"
      << endl
      << "import sqlalchemy.orm as orm"
      << endl
      << "from sqlalchemy.ext.declarative import declarative_base"
      << endl
      << "from sqlalchemy.orm.collections import attribute_mapped_collection"
      << endl << endl;

  // Thrift imports
  ss << "from thrift.Thrift import TType, TMessageType, TFrozenDict, TException, "
        "TApplicationException"
     << endl
     << "from thrift.protocol.TProtocol import TProtocolException"
     << endl
     << "from thrift.TRecursive import fix_spec"
     << endl;

  // For utf-8 strings
  ss << endl << "import sys" << endl;
  
  return ss.str();
}

/**
 * Closes the type files
 */
void t_sql_generator::close_generator() {
  // Close types file
  f_types_.close();
}

/**
 * Generates a typedef. This is not done in Python, types are all implicit.
 *
 * @param ttypedef The type definition
 */
void t_sql_generator::generate_typedef(t_typedef* ttypedef) {
  (void)ttypedef;
}

/**
 * Generates code for an enumerated type. Done using a class to scope
 * the values.
 *
 * @param tenum The enumeration
 */
void t_sql_generator::generate_enum(t_enum* tenum) {
  f_types_ << endl << endl << "class " << tenum->get_name() << "(object):" << endl;
  indent_up();
  generate_python_docstring(f_types_, tenum);

  indent(f_types_) << "_VALUES_TO_NAMES = {" << endl;

  vector<t_enum_value*> constants = tenum->get_constants();
  vector<t_enum_value*>::iterator c_iter;
  indent_up();
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    int value = (*c_iter)->get_value();
    indent(f_types_) << value << ": \""
                     << escape_string((*c_iter)->get_name()) << "\"," << endl;
  }
  indent_down();
  indent(f_types_) << "}" << endl << endl;

  indent(f_types_) << "def __iter__(self):" << endl
                   << indent() << indent() << "for v in self._VALUES_TO_NAMES.values():" << endl
                   << indent() << indent() << indent() << "yield v" << endl << endl;

  indent(f_types_) << "@classmethod" << endl << indent() << "def name(cls, value):" << endl
                   << indent() << indent() << "return cls._VALUES_TO_NAMES[value]";
  indent_down();

  f_types_ << endl;
}

/**
 * Generate a constant value
 */
void t_sql_generator::generate_const(t_const* tconst) {
  return;
}

/**
 * Prints the value of a constant with the given type. Note that type checking
 * is NOT performed in this function as it is always run beforehand using the
 * validate_types method in main.cc
 */
string t_sql_generator::render_const_value(t_type* type, t_const_value* value) {
  type = get_true_type(type);
  std::ostringstream out;

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_STRING:
      out << '"' << get_escaped_string(value) << '"';
      break;
    case t_base_type::TYPE_BOOL:
      out << (value->get_integer() > 0 ? "True" : "False");
      break;
    case t_base_type::TYPE_I8:
    case t_base_type::TYPE_I16:
    case t_base_type::TYPE_I32:
    case t_base_type::TYPE_I64:
      out << value->get_integer();
      break;
    case t_base_type::TYPE_DOUBLE:
      if (value->get_type() == t_const_value::CV_INTEGER) {
        out << value->get_integer();
      } else {
        out << value->get_double();
      }
      break;
    default:
      throw "compiler error: no const of base type " + t_base_type::t_base_name(tbase);
    }
  } else if (type->is_enum()) {
    out << value->get_integer();
  } else if (type->is_struct() || type->is_xception()) {
    out << type_name(type) << "(**{" << endl;
    indent_up();
    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;
    const map<t_const_value*, t_const_value*, t_const_value::value_compare>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      t_type* field_type = NULL;
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if ((*f_iter)->get_name() == v_iter->first->get_string()) {
          field_type = (*f_iter)->get_type();
        }
      }
      if (field_type == NULL) {
        throw "type error: " + type->get_name() + " has no field " + v_iter->first->get_string();
      }
      indent(out) << render_const_value(g_type_string, v_iter->first) << ": "
          << render_const_value(field_type, v_iter->second) << "," << endl;
    }
    indent_down();
    indent(out) << "})";
  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();
    if (is_immutable(type)) {
      out << "TFrozenDict(";
    }
    out << "{" << endl;
    indent_up();
    const map<t_const_value*, t_const_value*, t_const_value::value_compare>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      indent(out) << render_const_value(ktype, v_iter->first) << ": "
          << render_const_value(vtype, v_iter->second) << "," << endl;
    }
    indent_down();
    indent(out) << "}";
    if (is_immutable(type)) {
      out << ")";
    }
  } else if (type->is_list() || type->is_set()) {
    t_type* etype;
    if (type->is_list()) {
      etype = ((t_list*)type)->get_elem_type();
    } else {
      etype = ((t_set*)type)->get_elem_type();
    }
    if (type->is_set()) {
      if (is_immutable(type)) {
        out << "frozen";
      }
      out << "set(";
    }
    if (is_immutable(type) || type->is_set()) {
      out << "(" << endl;
    } else {
      out << "[" << endl;
    }
    indent_up();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      indent(out) << render_const_value(etype, *v_iter) << "," << endl;
    }
    indent_down();
    if (is_immutable(type) || type->is_set()) {
      indent(out) << ")";
    } else {
      indent(out) << "]";
    }
    if (type->is_set()) {
      out << ")";
    }
  } else {
    throw "CANNOT GENERATE CONSTANT FOR TYPE: " + type->get_name();
  }

  return out.str();
}

/**
 * Generates the "forward declarations" for python structs.
 * These are actually full class definitions so that calls to generate_struct
 * can add the thrift_spec field.  This is needed so that all thrift_spec
 * definitions are grouped at the end of the file to enable co-recursive structs.
 */
void t_sql_generator::generate_forward_declaration(t_struct* tstruct) {
    generate_py_struct(tstruct);
}

/**
 * Generates a python struct
 */
void t_sql_generator::generate_struct(t_struct* tstruct) {
  return;
}

/**
 * Generates a struct definition for a thrift exception. Basically the same
 * as a struct but extends the Exception class.
 *
 * @param txception The struct definition
 */
void t_sql_generator::generate_xception(t_struct* txception) {
  return;
}

/**
 * Generates a python struct
 */
void t_sql_generator::generate_py_struct(t_struct* tstruct) {
  generate_py_struct_definition(f_types_, tstruct);
}


/**
 * Generate the thrift_spec for a struct
 * For example,
 *   all_structs.append(Recursive)
 *   Recursive.thrift_spec = (
 *       None,  # 0
 *       (1, TType.LIST, 'Children', (TType.STRUCT, (Recursive, None), False), None, ),  # 1
 *   )
 */
void t_sql_generator::generate_py_thrift_spec(ofstream& out,
                                             t_struct* tstruct,
                                             bool /*is_exception*/) {
  const vector<t_field*>& sorted_members = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator m_iter;

  // Add struct definition to list so thrift_spec can be fixed for recursive structures.
  indent(out) << "all_structs.append(" << tstruct->get_name() << ")" << endl;

  if (sorted_members.empty() || (sorted_members[0]->get_key() >= 0)) {
    indent(out) << tstruct->get_name() << ".thrift_spec = (" << endl;
    indent_up();

    int sorted_keys_pos = 0;
    for (m_iter = sorted_members.begin(); m_iter != sorted_members.end(); ++m_iter) {

      for (; sorted_keys_pos != (*m_iter)->get_key(); sorted_keys_pos++) {
        indent(out) << "None,  # " << sorted_keys_pos << endl;
      }

      indent(out) << "(" << (*m_iter)->get_key() << ", " << type_to_enum((*m_iter)->get_type())
                  << ", "
                  << "'" << (*m_iter)->get_name() << "'"
                  << ", " << type_to_spec_args((*m_iter)->get_type()) << ", "
                  << render_field_default_value(*m_iter) << ", "
                  << "),"
                  << "  # " << sorted_keys_pos << endl;

      sorted_keys_pos++;
    }

    indent_down();
    indent(out) << ")" << endl;
  } else {
    indent(out) << tstruct->get_name() << ".thrift_spec = ()" << endl;
  }
}

/**
 * Generates a struct definition for a thrift data type.
 *
 * @param tstruct The struct definition
 */
void t_sql_generator::generate_py_struct_definition(ofstream& out,
                                                   t_struct* tstruct) {
  out << endl << endl << "class " << tstruct->get_name();
  out << "(" << BASE_SQL_OBJ << ")";
  out << ":" << endl;
  indent_up();
  generate_python_docstring(out, tstruct);

  out << endl;

  generate_py_sqlalchemy_table(out, tstruct);
  indent_down();
  return;
}

/**
 * Generates the read method for a struct
 */
void t_sql_generator::generate_py_struct_reader(ofstream& out, t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;


  indent(out) << "def read(self, iprot):" << endl;
  indent_up();

  const char* id = "self";

  indent(out) << "if iprot._fast_decode is not None "
                 "and isinstance(iprot.trans, TTransport.CReadableTransport) "
                 "and "
              << id << ".thrift_spec is not None:" << endl;
  indent_up();

  indent(out) << "iprot._fast_decode(self, iprot, [self.__class__, self.thrift_spec])" << endl;
  indent(out) << "return" << endl;

  indent_down();

  indent(out) << "iprot.readStructBegin()" << endl;

  // Loop over reading in fields
  indent(out) << "while True:" << endl;
  indent_up();

  // Read beginning field marker
  indent(out) << "(fname, ftype, fid) = iprot.readFieldBegin()" << endl;

  // Check for field STOP marker and break
  indent(out) << "if ftype == TType.STOP:" << endl;
  indent_up();
  indent(out) << "break" << endl;
  indent_down();

  // Switch statement on the field we are reading
  bool first = true;

  // Generate deserialization code for known cases
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
      out << indent() << "if ";
    } else {
      out << indent() << "elif ";
    }
    out << "fid == " << (*f_iter)->get_key() << ":" << endl;
    indent_up();
    indent(out) << "if ftype == " << type_to_enum((*f_iter)->get_type()) << ":" << endl;
    indent_up();
    generate_deserialize_field(out, *f_iter, "self.");
    indent_down();
    out << indent() << "else:" << endl << indent() << indent_str() << "iprot.skip(ftype)" << endl;
    indent_down();
  }

  // In the default case we skip the field
  out << indent() << "else:" << endl << indent() << indent_str() << "iprot.skip(ftype)" << endl;

  // Read field end marker
  indent(out) << "iprot.readFieldEnd()" << endl;

  indent_down();

  indent(out) << "iprot.readStructEnd()" << endl;

  indent_down();
  out << endl;
}

void t_sql_generator::generate_py_struct_required_validator(ofstream& out, t_struct* tstruct) {
  indent(out) << "def validate(self):" << endl;
  indent_up();

  const vector<t_field*>& fields = tstruct->get_members();

  if (fields.size() > 0) {
    vector<t_field*>::const_iterator f_iter;

    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
      t_field* field = (*f_iter);
      if (field->get_req() == t_field::T_REQUIRED) {
        indent(out) << "if self." << field->get_name() << " is None:" << endl;
        indent(out) << indent_str() << "raise TProtocolException(message='Required field "
                    << field->get_name() << " is unset!')" << endl;
      }
    }
  }

  indent(out) << "return" << endl;
  indent_down();
}

/**
 * Generates a thrift service.
 *
 * @param tservice The service definition
 */
void t_sql_generator::generate_service(t_service* tservice) {
  return;
}

/**
 * Generates a Base SQLAlchemy class for a thrift struct (represents a DB table)
 *
 * @param tstruct The struct to serialize
 */ 
void t_sql_generator::generate_py_sqlalchemy_table(ofstream& out, t_struct* tstruct) {
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;
  ostringstream post_table_generation;
 
  out << default_table_fields(tstruct->get_name());

  // Create a column for each struct field
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    out << type_to_sqlalchemy_column(post_table_generation, (*m_iter)->get_type(), tstruct->get_name(), (*m_iter)->get_name());
  }
  out << endl;

  generate_py_struct_reader(out, tstruct);

  out << sql_table_repr();
  out << post_table_generation.str();
}

/**
 * Deserializes a field of any type.
 */
void t_sql_generator::generate_deserialize_field(ofstream& out,
                                                t_field* tfield,
                                                string prefix) {
  t_type* type = get_true_type(tfield->get_type());

  if (type->is_void()) {
    throw "CANNOT GENERATE DESERIALIZE CODE FOR void TYPE: " + prefix + tfield->get_name();
  }

  string name = prefix + tfield->get_name();

  if (type->is_struct() || type->is_xception()) {
    generate_deserialize_struct(out, (t_struct*)type, name);
  } else if (type->is_container()) {
    generate_deserialize_container(out, type, name);
  } else if (type->is_base_type()) {
    indent(out) << name << " = iprot.";

    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "compiler error: cannot serialize void field in a struct: " + name;
    case t_base_type::TYPE_STRING:
      if (type->is_binary()) {
        out << "readBinary()";
      } else {
        out << "readString().decode('utf-8') if sys.version_info[0] == 2 else iprot.readString()";
      }
      break;
    case t_base_type::TYPE_BOOL:
      out << "readBool()";
      break;
    case t_base_type::TYPE_I8:
      out << "readByte()";
      break;
    case t_base_type::TYPE_I16:
      out << "readI16()";
      break;
    case t_base_type::TYPE_I32:
      out << "readI32()";
      break;
    case t_base_type::TYPE_I64:
      out << "readI64()";
      break;
    case t_base_type::TYPE_DOUBLE:
      out << "readDouble()";
      break;
    default:
      throw "compiler error: no Python name for base type " + t_base_type::t_base_name(tbase);
    }
    out << endl;
  } else if (type->is_enum()) {
    indent(out) << name << " = " << type->get_name() + ".name(iprot.readI32())" << endl;
  } else {
    printf("DO NOT KNOW HOW TO DESERIALIZE FIELD '%s' TYPE '%s'\n",
           tfield->get_name().c_str(),
           type->get_name().c_str());
  }
}

/**
 * Generates an unserializer for a struct, calling read()
 */
void t_sql_generator::generate_deserialize_struct(ofstream& out, t_struct* tstruct, string prefix) {
  out << indent() << prefix << " = " << type_name(tstruct) << "()" << endl
      << indent() << prefix << ".read(iprot)" << endl;
}

/**
 * Serialize a container by writing out the header followed by
 * data and then a footer.
 */
void t_sql_generator::generate_deserialize_container(ofstream& out, t_type* ttype, string prefix) {
  string size = tmp("_size");
  string ktype = tmp("_ktype");
  string vtype = tmp("_vtype");
  string etype = tmp("_etype");

  t_field fsize(g_type_i32, size);
  t_field fktype(g_type_i8, ktype);
  t_field fvtype(g_type_i8, vtype);
  t_field fetype(g_type_i8, etype);

  // Declare variables, read header
  if (ttype->is_map()) {
    out << indent() << prefix << " = {}" << endl << indent() << "(" << ktype << ", " << vtype
        << ", " << size << ") = iprot.readMapBegin()" << endl;
  } else if (ttype->is_set()) {
    out << indent() << prefix << " = set()" << endl << indent() << "(" << etype << ", " << size
        << ") = iprot.readSetBegin()" << endl;
  } else if (ttype->is_list()) {
    out << indent() << prefix << " = []" << endl << indent() << "(" << etype << ", " << size
        << ") = iprot.readListBegin()" << endl;
  }

  // For loop iterates over elements
  string i = tmp("_i");
  indent(out) <<
    "for " << i << " in range(" << size << "):" << endl;

  indent_up();

  if (ttype->is_map()) {
    generate_deserialize_map_element(out, (t_map*)ttype, prefix);
  } else if (ttype->is_set()) {
    generate_deserialize_set_element(out, (t_set*)ttype, prefix);
  } else if (ttype->is_list()) {
    generate_deserialize_list_element(out, (t_list*)ttype, prefix);
  }

  indent_down();

  // Read container end
  if (ttype->is_map()) {
    indent(out) << "iprot.readMapEnd()" << endl;
  } else if (ttype->is_set()) {
    indent(out) << "iprot.readSetEnd()" << endl;
  } else if (ttype->is_list()) {
    indent(out) << "iprot.readListEnd()" << endl;
  }
}

/**
 * Generates code to deserialize a map
 */
void t_sql_generator::generate_deserialize_map_element(ofstream& out, t_map* tmap, string prefix) {
  string elem = tmp("_elem");
  string key = elem + ".Key";
  string val = elem + ".Value";
  t_field fkey(tmap->get_key_type(), key);
  t_field fval(tmap->get_val_type(), val);

  // Instantiate pivot table
  out << instantiate_pivot_table(elem, prefix);

  generate_deserialize_field(out, &fkey);
  generate_deserialize_field(out, &fval);

  indent(out) << prefix << "[" << key << "] = " << elem << endl;
}

/**
 * Write a set element
 */
void t_sql_generator::generate_deserialize_set_element(ofstream& out, t_set* tset, string prefix) {
  string elem = tmp("_elem");
  t_field felem(tset->get_elem_type(), elem + ".Item");

  // Instantiate pivot table
  out << instantiate_pivot_table(elem, prefix);

  generate_deserialize_field(out, &felem);

  indent(out) << prefix << ".add(" << elem << ")" << endl;
}

/**
 * Write a list element
 */
void t_sql_generator::generate_deserialize_list_element(ofstream& out,
                                                       t_list* tlist,
                                                       string prefix) {
  string elem = tmp("_elem");
  t_field felem(tlist->get_elem_type(), elem + ".Item");

  // Instantiate pivot table
  out << instantiate_pivot_table(elem, prefix);

  generate_deserialize_field(out, &felem);

  indent(out) << prefix << ".append(" << elem << ")" << endl;
}

/**
 * Generates the docstring for a given struct.
 */
void t_sql_generator::generate_python_docstring(ofstream& out, t_struct* tstruct) {
  generate_python_docstring(out, tstruct, tstruct, "Attributes");
}

/**
 * Generates the docstring for a given function.
 */
void t_sql_generator::generate_python_docstring(ofstream& out, t_function* tfunction) {
  generate_python_docstring(out, tfunction, tfunction->get_arglist(), "Parameters");
}

/**
 * Generates the docstring for a struct or function.
 */
void t_sql_generator::generate_python_docstring(ofstream& out,
                                               t_doc* tdoc,
                                               t_struct* tstruct,
                                               const char* subheader) {
  bool has_doc = false;
  stringstream ss;
  if (tdoc->has_doc()) {
    has_doc = true;
    ss << tdoc->get_doc();
  }

  const vector<t_field*>& fields = tstruct->get_members();
  if (fields.size() > 0) {
    if (has_doc) {
      ss << endl;
    }
    has_doc = true;
    ss << subheader << ":\n";
    vector<t_field*>::const_iterator p_iter;
    for (p_iter = fields.begin(); p_iter != fields.end(); ++p_iter) {
      t_field* p = *p_iter;
      ss << " - " << p->get_name();
      if (p->has_doc()) {
        ss << ": " << p->get_doc();
      } else {
        ss << endl;
      }
    }
  }

  if (has_doc) {
    generate_docstring_comment(out, "\"\"\"\n", "", ss.str(), "\"\"\"\n");
  }
}

/**
 * Generates the docstring for a generic object.
 */
void t_sql_generator::generate_python_docstring(ofstream& out, t_doc* tdoc) {
  if (tdoc->has_doc()) {
    generate_docstring_comment(out, "\"\"\"\n", "", tdoc->get_doc(), "\"\"\"\n");
  }
}

/**
 * Renders a field default value, returns None otherwise.
 *
 * @param tfield The field
 */
string t_sql_generator::render_field_default_value(t_field* tfield) {
  t_type* type = get_true_type(tfield->get_type());
  if (tfield->get_value() != NULL) {
    return render_const_value(type, tfield->get_value());
  } else {
    return "None";
  }
}

string t_sql_generator::type_name(t_type* ttype) {
  while (ttype->is_typedef()) {
    ttype = ((t_typedef*)ttype)->get_type();
  }

  t_program* program = ttype->get_program();
  if (ttype->is_service()) {
    return get_real_py_module(program) + "." + ttype->get_name();
  }
  if (program != NULL && program != program_) {
    return get_real_py_module(program) + ".ttypes." + ttype->get_name();
  }
  return ttype->get_name();
}

/**
 * Converts the parse type to a Python tyoe
 */
string t_sql_generator::type_to_enum(t_type* type) {
  type = get_true_type(type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "NO T_VOID CONSTRUCT";
    case t_base_type::TYPE_STRING:
      return "TType.STRING";
    case t_base_type::TYPE_BOOL:
      return "TType.BOOL";
    case t_base_type::TYPE_I8:
      return "TType.BYTE";
    case t_base_type::TYPE_I16:
      return "TType.I16";
    case t_base_type::TYPE_I32:
      return "TType.I32";
    case t_base_type::TYPE_I64:
      return "TType.I64";
    case t_base_type::TYPE_DOUBLE:
      return "TType.DOUBLE";
    }
  } else if (type->is_enum()) {
    return "TType.I32";
  } else if (type->is_struct() || type->is_xception()) {
    return "TType.STRUCT";
  } else if (type->is_map()) {
    return "TType.MAP";
  } else if (type->is_set()) {
    return "TType.SET";
  } else if (type->is_list()) {
    return "TType.LIST";
  }

  throw "INVALID TYPE IN type_to_enum: " + type->get_name();
}

/** See the comment inside generate_py_struct_definition for what this is. */
string t_sql_generator::type_to_spec_args(t_type* ttype) {
  while (ttype->is_typedef()) {
    ttype = ((t_typedef*)ttype)->get_type();
  }

  if (ttype->is_binary()) {
    return  "'BINARY'";
  } else if (ttype->is_base_type()
             && reinterpret_cast<t_base_type*>(ttype)->is_string()) {
    return "'UTF8'";
  } else if (ttype->is_base_type() || ttype->is_enum()) {
    return  "None";
  } else if (ttype->is_struct() || ttype->is_xception()) {
    return "[" + type_name(ttype) + ", None]";
  } else if (ttype->is_map()) {
    return "(" + type_to_enum(((t_map*)ttype)->get_key_type()) + ", "
           + type_to_spec_args(((t_map*)ttype)->get_key_type()) + ", "
           + type_to_enum(((t_map*)ttype)->get_val_type()) + ", "
           + type_to_spec_args(((t_map*)ttype)->get_val_type()) + ", "
           + (is_immutable(ttype) ? "True" : "False") + ")";

  } else if (ttype->is_set()) {
    return "(" + type_to_enum(((t_set*)ttype)->get_elem_type()) + ", "
           + type_to_spec_args(((t_set*)ttype)->get_elem_type()) + ", "
           + (is_immutable(ttype) ? "True" : "False") + ")";

  } else if (ttype->is_list()) {
    return "(" + type_to_enum(((t_list*)ttype)->get_elem_type()) + ", "
           + type_to_spec_args(((t_list*)ttype)->get_elem_type()) + ", "
           + (is_immutable(ttype) ? "True" : "False") + ")";
  }

  throw "INVALID TYPE IN type_to_spec_args: " + ttype->get_name();
}

string t_sql_generator::base_type_to_sql_type(t_type* type) {
  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "NO T_VOID CONSTRUCT";
    case t_base_type::TYPE_STRING:
      return "db.String";
    case t_base_type::TYPE_BOOL:
      return "db.Boolean";
    case t_base_type::TYPE_I8:
      return "db.CHAR";
    case t_base_type::TYPE_I16:
      return "db.SmallInteger";
    case t_base_type::TYPE_I32:
      return "db.Integer";
    case t_base_type::TYPE_I64:
      return "db.BigInteger";
    case t_base_type::TYPE_DOUBLE:
      return "db.Float";
    }
  }
  else if (type->is_enum()) {
    return "db.Enum(*" + type->get_name() + "())";
  }

  throw "INVALID TYPE IN base_type_to_sql_type: " + type->get_name();
}

/**
 * Return the fields utilized for every SQLAlchemy base class
 * 
 * @param table_name The name of the SQL table 
 */
string t_sql_generator::default_table_fields(string table_name) {
  std::ostringstream result;
  indent(result) << "__tablename__ = '" + table_name << "'" << endl;
  indent(result) << "__pivottables__ = {}" << endl;
  indent(result) << "_id = db.Column(db.Integer, primary_key=True)" << endl;
  return result.str();
}

/**
 * Add pivot table to be tracked by parent table class. Generate column for foreign key linking class to pivot table.
 * 
 * @param append_out Stream that will be appended after the parent table class. Pivot tables must be defined outside the parent
 *    table definition.
 * @param parent_table Name of table being linked to the pivot table.
 * @param parent_column Name of column referencing pivot table.
 * @param table_name Name of the pivot table.
 */
string t_sql_generator::register_pivot_table(std::ostream& append_out, string parent_table, string parent_column, string table_name) {
  std::ostringstream result;
  indent(result) << "__pivottables__['" << parent_column << "'] = '" << table_name << "'" << endl;

  append_out << endl << "class " + table_name + "(" << BASE_SQL_OBJ << "):" << endl;
  append_out << default_table_fields(table_name);
  indent(append_out) << parent_table + "_fk" << " = db.Column(db.Integer, db.ForeignKey('" << parent_table + "._id'))" << endl;

  return result.str();
}

/**
 * Render the python content to instantiate a pivot table, specifically during Thrift deserialization.
 * 
 * @param var_name Variable name for the pivot table object
 * @param prefix String prefix to attach to all fields
 */
string t_sql_generator::instantiate_pivot_table(string var_name, string prefix) {
    std::ostringstream result;
    // prefix should be objInstance.fieldName -> split on '.'
    size_t pos = prefix.find('.');
    string object_name = pos != string::npos ? prefix.substr(0, pos) : prefix;
    string field_name = pos != string::npos ? prefix.substr(pos + 1, prefix.length() - pos) : prefix;
    indent(result) << var_name << " = globals()[" << object_name << ".__pivottables__['" << field_name << "']]()" << endl;
    return result.str();
}

/**
 * Helper function to render the __repr__() function for SQLAlchemy Base classes
 */
string t_sql_generator::sql_table_repr() {
  std::ostringstream out;
  indent(out) << "def __repr__(self):" << endl;
  indent_up();
  out << indent() << "L = ['%s=%r' % (col.name, getattr(self, col.name))" << endl
      << indent() << "     for col in self.__table__.c]" << endl
      << indent() << "return '%s(%s)' % (self.__class__.__name__, ', '.join(L))" << endl
      << endl;
  indent_down();
  return out.str();
}

/**
 * Generates a column for a SQLAlchemy Base class given a Thrift type
 * 
 * @param append_put Stream to be appended after the struct class/table is generated.
 *    Appends related pivot table class onto the output stream after the struct class.
 * @param ttype Thrift type to parse into a column
 * @param table_name Name of the table
 * @param column_name Name of the column
*/
string t_sql_generator::type_to_sqlalchemy_column(std::ostream& append_out, t_type* ttype, string table_name, string column_name) {
    ostringstream out;
    ostringstream rec_append;
    ttype = ttype->get_true_type();

    string pivot_table_name = table_name + "_" + column_name;
    if(pivot_table_name.find("__") != 0)
      pivot_table_name = "__" + pivot_table_name; 

    // Struct field = Many To One relationship (foreign key to another thrift struct table)
    if (ttype->is_struct()) {
      ttype = get_true_type(ttype);
      // Foreign key
      string foreign_key_name = "_" + column_name + "_fk";
      indent(out) << foreign_key_name << " = db.Column(db.Integer, db.ForeignKey('" << ttype->get_name() << "._id'))" << endl;
      // Link tables
      indent(out) << column_name << " = orm.relationship('" << ttype->get_name()
                  << "', foreign_keys='" << table_name +'.' + foreign_key_name << "')" << endl;
    }
    // List -> One To Many relationship (foreign key to pivot table)
    else if (ttype->is_list()) {
      // Create association table
      indent(out) << column_name << " = orm.relationship('" << pivot_table_name << "', backref='" << table_name << "')" << endl;
      out << register_pivot_table(append_out, table_name, column_name, pivot_table_name);

      // Recurse on the element type of the list
      t_type* value_type = ((t_list*)ttype)->get_elem_type();
      append_out << type_to_sqlalchemy_column(rec_append, value_type, pivot_table_name, "Item") << endl;
      indent(append_out) << "def __repr__(self):" << endl
        << indent() << indent() << "return '%s' % self.Item" << endl;
    }
    // Set -> One To Many relationship (foreign key to pivot table)    
    else if (ttype->is_set()) {
      // Create association table
      indent(out) << column_name << " = orm.relationship("
                  << "'" << pivot_table_name << "'"
                  << ", backref='" << table_name << "'"
                  << ", collection_class=set)" << endl;
      out << register_pivot_table(append_out, table_name, column_name, pivot_table_name);

      // Recurse on the element type of the list
      t_type* value_type = ((t_set*)ttype)->get_elem_type();
      append_out << type_to_sqlalchemy_column(rec_append, value_type, pivot_table_name, "Item") << endl;
      indent(append_out) << "def __repr__(self):" << endl
        << indent() << indent() << "return '%s' % self.Item" << endl;
    }
    // Map -> One To Many relationship (foreign key to pivot table)
    else if (ttype->is_map()) {
      // Create association table
      indent(out) << column_name << " = orm.relationship("
                 << "'" << pivot_table_name << "'"
                 << ", backref='" << table_name << "'"
                 << ", collection_class=attribute_mapped_collection('Key'))" << endl;
      out << register_pivot_table(append_out, table_name, column_name, pivot_table_name);

      // Recurse on the key and value types of the map
      append_out << type_to_sqlalchemy_column(rec_append, ((t_map*)ttype)->get_key_type(), pivot_table_name, "Key");
      append_out << type_to_sqlalchemy_column(rec_append, ((t_map*)ttype)->get_val_type(), pivot_table_name, "Value") << endl;
      indent(append_out) << "def __repr__(self):" << endl
        << indent() << indent() << "return '%s' % self.Value" << endl;
    }
    // Base Type
    else {
      indent(out) << column_name << " = db.Column(" << base_type_to_sql_type(ttype) << ")" << endl;
    }

    append_out << rec_append.str();
    return out.str();
}

THRIFT_REGISTER_GENERATOR(
    sql,
    "SQL",
    "    python:                 Generate Python ORM code using SQLAlchemy (Defaults to true).\n"
    "    python.coding=CODING:   Add python file encoding declare in generated file.\n"    )
