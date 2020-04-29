#include "llvm/DebugInfo/DWARF/DWARFContext.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/Binary.h"
#include "llvm/ProfileData/Coverage/CoverageMapping.h"
#include "llvm/ProfileData/Coverage/CoverageMappingReader.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_os_ostream.h"

#include <iostream>
#include <set>
#include <cstring>

using namespace llvm;
using namespace llvm::object;
using namespace llvm::dwarf;
using namespace llvm::coverage;
using namespace std;

uint64_t getAddress(DWARFDie &die) {
  auto addresses = die.getAddressRanges();
  if (addresses->size() > 0) {
    return (*addresses)[0].LowPC;
  }
  return 0;
}

void addInlined(ObjectFile *Obj, std::set<StringRef> &Retained) {
  std::unique_ptr<DWARFContext> DICtx = DWARFContext::create(*Obj);

  auto GetSubroutineName = [](DWARFDie &Sym) {
    return Sym.getSubroutineName(DINameKind::LinkageName);
  };

  auto IsRetained = [&](DWARFDie &Sym) {
    return Retained.count(GetSubroutineName(Sym)) > 0;
  };

  // Walk through an inline chain and mark any symbol transitively inlined into
  // a retained symbol as retained.
  auto AnalyzeChain = [&](SmallVectorImpl<DWARFDie> &Chain) {
    int Idx;
    for (Idx = 0; Idx < Chain.size(); Idx ++) {
      if (IsRetained(Chain[Idx]))
        break;
    }
    if (Idx < Chain.size()) {
      for (int i = 0; i <= Idx; i ++) {
        Retained.insert(GetSubroutineName(Chain[i]));
      }
      return true;
    } else
      return false;
  };

  for (auto &CU: DICtx->compile_units()) {
    for (auto &DIE: CU->dies()) {
      if (DIE.getTag() != DW_TAG_inlined_subroutine)
        continue;

      DWARFDie Subroutine(CU.get(), &DIE);
      if (IsRetained(Subroutine))
        continue;

      SmallVector<DWARFDie, 4> Chain;
      auto Addresses = Subroutine.getAddressRanges();
      if (Addresses->size() == 0) {
        continue;
      }
      for (auto Range: *Addresses) {
        Chain.clear();
        CU->getInlinedChainForAddress(Range.LowPC, Chain);
        if (AnalyzeChain(Chain))
          Retained.insert(GetSubroutineName(Subroutine));
        break;
      }
    }
  }
}

static StringRef StripFileName(StringRef symbol) {
  std::pair<StringRef, StringRef> parts = symbol.split(':');
  if (parts.second.empty())
    return symbol;
  else
    return parts.second;
}

static std::set<StringRef> GetRetainedSymbols(ELFObjectFileBase *ELFBinary) {
  std::set<StringRef> Retained;

  // Start with all symbols in the symtab.
  for (auto Sym : ELFBinary->symbols()) {
    if (auto Name = Sym.getName())
      Retained.insert(Name.get());
  }

  addInlined(ELFBinary, Retained);
  return Retained;
}

void PrintLinkerDiscardedSymbols(MemoryBuffer &Buffer,
                                 std::set<StringRef> &Retained) {
  SmallVector<std::unique_ptr<MemoryBuffer>, 4> objectBuffers;
  auto mapping_readers = BinaryCoverageReader::create(Buffer,
                                                      StringRef(),
                                                      objectBuffers);

  std::set<StringRef> Discarded;
  if (!mapping_readers.takeError()) {
    for (auto& reader : mapping_readers.get()) {
      for (auto mapping_record : *reader) {
        if (!mapping_record.takeError()) {
          StringRef MappingFnName = mapping_record->FunctionName;
          StringRef BaseName = StripFileName(MappingFnName);
          if (!Retained.count(BaseName))
            Discarded.insert(MappingFnName);
        }
      }
    }
  }

  for (auto S: Discarded) {
    cout << S.str() << "\n";
  }
}

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " <binary-to-analyze>\n";
    return -1;
  }

  ErrorOr<std::unique_ptr<MemoryBuffer>> BuffOrErr =
    MemoryBuffer::getFileOrSTDIN(argv[1]);
  if (auto EC = BuffOrErr.getError()) {
    cerr << "Error (getFileOrSTDIN): " << EC.message() << "\n";
    return -1;
  }
  std::unique_ptr<MemoryBuffer> Buffer = std::move(BuffOrErr.get());

  Expected<std::unique_ptr<Binary>> BinOrErr = object::createBinary(*Buffer);
  if (auto EC = errorToErrorCode(BinOrErr.takeError())) {
    cerr << "Error (CreateBinary): " << EC.message() << "\n";
    return -1;
  }

  if (auto *Obj = dyn_cast<ELFObjectFileBase>(BinOrErr->get())) {
    auto Retained = GetRetainedSymbols(Obj);
    PrintLinkerDiscardedSymbols(*Buffer, Retained);
  } else {
    cerr << "Error: dyn_cast<ObjectFile>\n";
    return -1;
  }
  return 0;
}
