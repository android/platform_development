"""Abstraction layer for different ABIs."""

import re
import symbol

PC_REGISTERS = {
    'arm': 'pc',
    'arm64': 'pc',
    'mips': 'epc',
    'x86': 'eip',
    'x86_64': 'rip'
}


def UnpackLittleEndian(word):
  """Split a hexadecimal string in little endian order."""
  return [word[x:x+2] for x in range(len(word) - 2, -2, -2)]


def UnpackBigEndian(word):
  """Split a hexadecimal string in big endian order."""
  return [word[x:x+2] for x in range(0, len(word), 2)]


COMPILE = 'gcc'
DISASSEMBLE = 'objdump'
LINK = 'ld'
UNPACK = 'unpack'

OPTIONS = {
    'x86': {
        COMPILE: ['-m32'],
        LINK: ['-melf_i386']
    }
}


PROGRAM_COUNTER_RE = {
    'arm': re.compile(
        '    ip [0-9a-f]{8}  sp [0-9a-f]{8}  lr [0-9a-f]{8}  pc ([0-9a-f]{8})'
        '  cpsr [0-9a-f]{8}'),
    'arm64': re.compile('    sp   [0-9a-f]{16}  pc   ([0-9a-f]{16})'),
    'mips': re.compile(
        ' hi [0-9a-f]{8}  lo [0-9a-f]{8} bva [0-9a-f]{8} epc ([0-9a-f]{8})'),
    'x86': re.compile(
        '    eip ([0-9a-f]{8})  ebp [0-9a-f]{8}  esp [0-9a-f]{8}'
        '  flags [0-9a-f]{8}'),
    'x86_64': re.compile(
        '    rip ([0-9a-f]{16})  rbp [0-9a-f]{16}  rsp [0-9a-f]{16}'
        '  eflags [0-9a-f]{16}')
}


class Architecture(object):
  """Creates an architecture abstraction for a given ABI.

  Args:
    name: The abi name, as represented in a tombstone.
  """

  def __init__(self, name):
    symbol.ARCH = name
    self.toolchain = symbol.FindToolchain()
    self.pc_register = PC_REGISTERS[name]
    self.pc_re = PROGRAM_COUNTER_RE[name]
    self.options = OPTIONS.get(name, {})

  def Compile(self, args):
    """Generates a compile command, appending the given args."""
    return [symbol.ToolPath(COMPILE)] + self.options.get(COMPILE, []) + args

  def Link(self, args):
    """Generates a link command, appending the given args."""
    return [symbol.ToolPath(LINK)] + self.options.get(LINK, []) + args

  def Disassemble(self, args):
    """Generates a disassemble command, appending the given args."""
    return ([symbol.ToolPath(DISASSEMBLE)] + self.options.get(DISASSEMBLE, []) +
            args)

  def FindProgramCounter(self, line):
    """Finds the program counter in a register dump of a tombstone.

    Args:
      line: a string from the tombstone

    Returns:
      None if no program counter was found or an integer representation of the
      program counter's value.
    """
    match = self.pc_re.match(line)
    if match:
      return int(match.group(1), 16)
    return None

  def WordToBytes(self, word):
    """Unpacks a hexadecimal string in the architecture's byte order.

    Args:
      word: A string representing a hexadecimal value.

    Returns:
      An array of hexadecimal byte values.
    """
    return self.options.get(UNPACK, UnpackLittleEndian)(word)
