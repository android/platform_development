#ifndef ABI_DIFF_HELPERS
#define ABI_DIFF_HELPERS

#include <ir_representation.h>

#include <deque>

// Classes which act as middle-men between clang AST parsing routines and
// message format specific dumpers.
namespace abi_util {

using MergeStatus = TextFormatToIRReader::MergeStatus;

enum DiffStatus {
  // Previous stages of CompareAndTypeDiff should not dump the diff.
  no_diff = 0,
  // Previous stages of CompareAndTypeDiff should dump the diff if required.
  direct_diff = 1,
};

std::string Unwind(const std::deque<std::string> *type_queue);

class AbiDiffHelper {
 public:
  AbiDiffHelper(
      const AbiElementMap<const abi_util::TypeIR *> &old_types,
      const AbiElementMap<const abi_util::TypeIR *> &new_types,
      std::set<std::string> *type_cache,
      abi_util::IRDiffDumper *ir_diff_dumper = nullptr,
      AbiElementMap<MergeStatus> *local_to_global_type_id_map = nullptr)
      : old_types_(old_types), new_types_(new_types),
        type_cache_(type_cache), ir_diff_dumper_(ir_diff_dumper),
        local_to_global_type_id_map_(local_to_global_type_id_map) { }

  struct FastDiffDecision {
    bool was_decision_taken_ = false;
    DiffStatus diff_decision_;
    FastDiffDecision(bool was_decision_taken,
                     DiffStatus diff_decision = DiffStatus::no_diff)
        : was_decision_taken_(was_decision_taken),
          diff_decision_(diff_decision) { }
  };

  FastDiffDecision GetFastDiffDecision(const std::string &old_type_id,
                                       const std::string &new_type_id);

  DiffStatus CompareAndDumpTypeDiff(
      const std::string &old_type_str, const std::string &new_type_str,
      std::deque<std::string> *type_queue = nullptr,
      abi_util::IRDiffDumper::DiffKind diff_kind = DiffMessageIR::Unreferenced);

  DiffStatus CompareAndDumpTypeDiff(
      const abi_util::TypeIR *old_type, const abi_util::TypeIR *new_type,
      abi_util::LinkableMessageKind kind,
      std::deque<std::string> *type_queue = nullptr,
      abi_util::IRDiffDumper::DiffKind diff_kind = DiffMessageIR::Unreferenced);


  DiffStatus CompareRecordTypes(const abi_util::RecordTypeIR *old_type,
                                const abi_util::RecordTypeIR *new_type,
                                std::deque<std::string> *type_queue,
                                abi_util::IRDiffDumper::DiffKind diff_kind);

  DiffStatus CompareQualifiedTypes(const abi_util::QualifiedTypeIR *old_type,
                                   const abi_util::QualifiedTypeIR *new_type,
                                   std::deque<std::string> *type_queue,
                                   abi_util::IRDiffDumper::DiffKind diff_kind);

  DiffStatus ComparePointerTypes(const abi_util::PointerTypeIR *old_type,
                                 const abi_util::PointerTypeIR *new_type,
                                 std::deque<std::string> *type_queue,
                                 abi_util::IRDiffDumper::DiffKind diff_kind);

  DiffStatus CompareLvalueReferenceTypes(
      const abi_util::LvalueReferenceTypeIR *old_type,
      const abi_util::LvalueReferenceTypeIR *new_type,
      std::deque<std::string> *type_queue,
      abi_util::IRDiffDumper::DiffKind diff_kind);

  DiffStatus CompareRvalueReferenceTypes(
      const abi_util::RvalueReferenceTypeIR *old_type,
      const abi_util::RvalueReferenceTypeIR *new_type,
      std::deque<std::string> *type_queue,
      abi_util::IRDiffDumper::DiffKind diff_kind);


  DiffStatus CompareBuiltinTypes(const abi_util::BuiltinTypeIR *old_type,
                                 const abi_util::BuiltinTypeIR *new_type);
  static void CompareEnumFields(
    const std::vector<abi_util::EnumFieldIR> &old_fields,
    const std::vector<abi_util::EnumFieldIR> &new_fields,
    abi_util::EnumTypeDiffIR *enum_type_diff_ir);

  DiffStatus CompareEnumTypes(const abi_util::EnumTypeIR *old_type,
                              const abi_util::EnumTypeIR *new_type,
                              std::deque<std::string> *type_queue,
                              abi_util::IRDiffDumper::DiffKind diff_kind);

  std::unique_ptr<abi_util::RecordFieldDiffIR> CompareCommonRecordFields(
    const abi_util::RecordFieldIR *old_field,
    const abi_util::RecordFieldIR *new_field,
    std::deque<std::string> *type_queue,
    abi_util::IRDiffDumper::DiffKind diff_kind);

  std::pair<std::vector<abi_util::RecordFieldDiffIR>,
      std::vector<const abi_util::RecordFieldIR *>>
  CompareRecordFields(
      const std::vector<abi_util::RecordFieldIR> &old_fields,
      const std::vector<abi_util::RecordFieldIR> &new_fields,
      std::deque<std::string> *type_queue,
      abi_util::IRDiffDumper::DiffKind diff_kind);

  DiffStatus CompareFunctionParameters(
      const std::vector<abi_util::ParamIR> &old_parameters,
      const std::vector<abi_util::ParamIR> &new_parameters,
      std::deque<std::string> *type_queue,
      abi_util::IRDiffDumper::DiffKind diff_kind);

  bool CompareBaseSpecifiers(
      const std::vector<abi_util::CXXBaseSpecifierIR> &old_base_specifiers,
      const std::vector<abi_util::CXXBaseSpecifierIR> &new_base_specifiers,
      std::deque<std::string> *type_queue,
      abi_util::IRDiffDumper::DiffKind diff_kind);

  bool CompareVTables(const abi_util::RecordTypeIR *old_record,
                      const abi_util::RecordTypeIR *new_record);

  bool CompareVTableComponents(
      const abi_util::VTableComponentIR &old_component,
      const abi_util::VTableComponentIR &new_component);

  void CompareTemplateInfo(
      const std::vector<abi_util::TemplateElementIR> &old_template_elements,
      const std::vector<abi_util::TemplateElementIR> &new_template_elements,
      std::deque<std::string> *type_queue,
      abi_util::IRDiffDumper::DiffKind diff_kind);


  bool CompareSizeAndAlignment(const abi_util::TypeIR *old_ti,
                               const abi_util::TypeIR *new_ti);

  template <typename DiffType, typename DiffElement>
  bool AddToDiff(DiffType *mutable_diff, const DiffElement *oldp,
                 const DiffElement *newp,
                 std::deque<std::string> *type_queue = nullptr);
 protected:
  const AbiElementMap<const abi_util::TypeIR *> &old_types_;
  const AbiElementMap<const abi_util::TypeIR *> &new_types_;
  std::set<std::string> *type_cache_ = nullptr;
  abi_util::IRDiffDumper *ir_diff_dumper_ = nullptr;
  AbiElementMap<MergeStatus> *local_to_global_type_id_map_ = nullptr;
};

} // namespace abi_util
#endif
