enum_types {
  type_info {
    name: "Enum"
    size: 4
    alignment: 4
    referenced_type: "type-1"
    source_file: "/development/vndk/tools/header-checker/tests/input/enum.h"
    linker_set_key: "Enum"
    self_type: "type-1"
  }
  underlying_type: "type-2"
  enum_fields {
    enum_field_value: 0
    name: "VALUE0"
  }
  enum_fields {
    enum_field_value: 3
    name: "VALUE3"
  }
  access: public_access
  tag_info {
    unique_id: "_ZTS4Enum"
  }
}
enum_types {
  type_info {
    name: "EnumClass"
    size: 4
    alignment: 4
    referenced_type: "type-3"
    source_file: "/development/vndk/tools/header-checker/tests/input/enum.h"
    linker_set_key: "EnumClass"
    self_type: "type-3"
  }
  underlying_type: "type-4"
  enum_fields {
    enum_field_value: 0
    name: "EnumClass::VALUE0"
  }
  enum_fields {
    enum_field_value: 3
    name: "EnumClass::VALUE3"
  }
  access: public_access
  tag_info {
    unique_id: "_ZTS9EnumClass"
  }
}
builtin_types {
  type_info {
    name: "unsigned int"
    size: 4
    alignment: 4
    referenced_type: "type-2"
    source_file: ""
    linker_set_key: "unsigned int"
    self_type: "type-2"
  }
  is_unsigned: true
  is_integral: true
}
builtin_types {
  type_info {
    name: "int"
    size: 4
    alignment: 4
    referenced_type: "type-4"
    source_file: ""
    linker_set_key: "int"
    self_type: "type-4"
  }
  is_unsigned: false
  is_integral: true
}
