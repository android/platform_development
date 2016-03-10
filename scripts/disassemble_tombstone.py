#!/usr/bin/python

"""Disassemble the code stored in a tombstone.

The classes in this module use an interface, ProcessLine, so that they can be
chained together to do arbitrary procerssing. The current classes support
disassembling the bytes embedded in tombstones and printing output to stdout.
"""


import re
import subprocess
import sys
import tempfile
import architecture


class Printer(object):

  def __init__(self):
    pass

  def ProcessLine(self, line):
    print line,


class Disassembler(object):
  """Finds the code block in a tombstone and disassembles it.

  Args:
    sink: A processor that will receive the new lines
    offset: Number of bytes to skip in the code block. Can be useful to force
      instruction alignment.
  """

  def __init__(self, sink, offset=0):
    self.abi_line = re.compile("(ABI: \'(.*)\')")
    self.program_counter_line = None
    self.sink = sink
    self.start_address_offset = offset
    self.start_address = None
    self.current_address = None
    self.program_counter = None
    self.current_processor = self.SearchForArch
    self.scratch_file = None
    self.toolchain = None

  def SearchForArch(self, line):
    """Search for the line that will give the architecture."""
    self.sink.ProcessLine(line)
    abi_header = self.abi_line.search(line)
    if abi_header:
      self.architecture = architecture.Architecture(abi_header.group(2))
      self.current_processor = self.SearchForProgramCounter

  def SearchForProgramCounter(self, line):
    """Find the program counter in the register dump."""
    self.sink.ProcessLine(line)
    self.program_counter = self.architecture.FindProgramCounter(line)
    if self.program_counter:
      self.current_processor = self.SearchForCodeBlock

  def SearchForCodeBlock(self, line):
    """Emit lines until we reach the code block."""
    self.sink.ProcessLine(line)
    if line.startswith('code around '):
      self.StartCodeBlock()

  def StartCodeBlock(self):
    """Begin processing o f the code block."""
    self.scratch_file = tempfile.NamedTemporaryFile(suffix='.s')
    self.current_processor = self.ProcessCodeBlock

  def ProcessCodeBlock(self, line):
    """Interpret the bytes in the code block."""
    words = line.split()
    if len(line) != 67:
      self.EndCodeBlock()
      return
    words = line.split()
    if self.start_address is None:
      self.current_address = int(words[0], 16)
      self.start_address = self.current_address + self.start_address_offset
    else:
      assert self.current_address == int(words[0], 16)
    for word in words[1:5]:
      # Handle byte swapping
      for byte in self.architecture.WordToBytes(word):
        if self.current_address == self.program_counter:
          self.scratch_file.write('program_counter_was_here:\n')
        if self.current_address >= self.start_address:
          self.scratch_file.write('  .byte 0x%s\n' % byte)
        self.current_address += 1

  def EndCodeBlock(self):
    """Disassembles the bytes, feeding the result to the next processor."""
    self.scratch_file.flush()
    object_file = tempfile.NamedTemporaryFile(suffix='.o')
    subprocess.check_call(self.architecture.Compile([
        '-c', '-o', object_file.name, self.scratch_file.name]))
    self.scratch_file.close()
    linked_file = tempfile.NamedTemporaryFile(suffix='.o')
    cmd = self.architecture.Link([
        '-Ttext', '0x%x' % self.start_address, '-o', linked_file.name,
        object_file.name])
    subprocess.check_call(cmd)
    object_file.close()
    disassembler = subprocess.Popen(self.architecture.Disassemble([
        '-S', linked_file.name]), stdout=subprocess.PIPE)
    emit = False
    start_pattern = '%x ' % self.start_address
    arrow_pattern = '%x:\t' % self.program_counter
    for line in disassembler.stdout:
      emit = emit or line.startswith(start_pattern)
      if emit and len(line) > 1 and line.find('program_counter_was_here') == -1:
        if line.startswith(arrow_pattern):
          self.sink.ProcessLine('--->' + line)
        else:
          self.sink.ProcessLine('    ' + line)
    linked_file.close()
    self.scratch_file = None
    self.current_processor = self.PassThrough
    self.sink.ProcessLine('\n')
    return

  def PassThrough(self, line):
    """Pass all of the lines after the code block through without changes."""
    self.sink.ProcessLine(line)

  def ProcessLine(self, line):
    self.current_processor(line)


def main(argv):
  for fn in argv[1:]:
    f = open(fn, 'r')
    disassembler = Disassembler(sink=Printer())
    for line in f:
      disassembler.ProcessLine(line)


if __name__ == '__main__':
  main(sys.argv)
