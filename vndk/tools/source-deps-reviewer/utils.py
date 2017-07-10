import re
import os
import hashlib
import subprocess
import bisect

FILE_EXT_BLACK_LIST = {
    b'.1',
    b'.ac',
    b'.cmake',
    b'.html',
    b'.info',
    b'.la',
    b'.m4',
    b'.map',
    b'.md',
    b'.py',
    b'.rst',
    b'.sh',
    b'.sym',
    b'.txt',
    b'.xml',
}

FILE_NAME_BLACK_LIST = {
    b'CHANGES.0',
    b'ChangeLog',
    b'config.h.in',
    b'configure',
    b'configure.in',
    b'configure.linaro',
    b'libtool',
}

PATH_PATTERN_BLACK_LIST = (
    b'autom4te.cache',
    b'dejagnu',
    b'llvm/Config/Config',
)
def is_empty_file(file_path):
    return os.stat(file_path).st_size == 0

def sanitize_code_unmatched(code):
    # Remove unmatched quotes until EOL.
    code = re.sub(b'"[^"]*$', b'', code)
    # Remove unmatched C comments.
    code = re.sub(b'/\\*.*$', b'', code)
    return code

def sanitize_code_matched(code):
    # Remove matching quotes.
    code = re.sub(b'"(?:[^"\\\\]|(?:\\\\.))*"', b'', code)
    # Remove C++ comments.
    code = re.sub(b'//[^\\r\\n]*$', b'', code)
    # Remove matched C comments.
    code = re.sub(b'/\\*(?:[^*]|(?:\\*+[^*/]))*\\*+/', b'', code)
    return code

def sanitize_code(code):
    return sanitize_code_unmatched(sanitize_code_matched(code))

'''
BUF_SIZE = 65536  # lets read stuff in 64kb chunks!
def hash_file(file_path):
    sha1 = hashlib.sha1()
    with open(file_path, 'rb') as f:
        while True:
            data = f.read(BUF_SIZE)
            if not data:
                break
            sha1.update(data)
    return sha1.hexdigest()

def find_ending_brace(string_from_first_brace):
    starts = [m.start() for m in re.finditer('{', string_from_first_brace, re.MULTILINE)][1:]
    ends = [m.start() for m in re.finditer('}', string_from_first_brace, re.MULTILINE)]

    i = 0
    j = 0
    current_scope_depth = 1

    while(current_scope_depth > 0):
        if(ends[j] < starts[i]):
            current_scope_depth -= 1
            j += 1
        elif(ends[j] > starts[i]):
            current_scope_depth += 1
            i += 1
            if(i == len(starts)): # in case we reached the end (fewer { than })
                j += 1
            break

    return ends[j-1]

def get_start_byte(code, line_no):
    #return fl.split('\n')[line_no-1]
    ret = sum([len(x) for x in code.split('\n')[:line_no-1]]) + line_no - 1
    return ret

def hash_obj(obj_path):
    func_name, file_ext = os.path.splitext( os.path.basename(obj_path) )
    if file_ext != '.func': return hash_file(obj_path)

    file_path = os.path.dirname(obj_path)
    tag = subprocess.check_output(['ctags', '-x', '--c-kinds=fp', file_path]).decode('utf-8')
    for line in tag.split('\n'):
        tmp = line.split()
        if len(tmp) > 3:
            if tmp[0] == func_name:
                line_no = int(tmp[2])
                break

    with open(file_path, 'r') as f:
        code = f.read()
    start_byte = get_start_byte(code, line_no)
    end_byte = find_ending_brace(code[start_byte:]) + start_byte
    data = code[start_byte:end_byte+1].encode('utf-8')
    sh1 = hashlib.sha1()
    sh1.update(data)
    return sh1.hexdigest()

#given line_no of a file, determine what function is it in
def in_function(file_path, line_no):
    res = []
    tag = subprocess.check_output(['ctags', '-x', '--c-kinds=fp', file_path]).decode('utf-8')
    for line in tag.split('\n'):
        tmp = line.split()
        if len(tmp) > 3 and tmp[1] == 'function' :
            res.append( (int(tmp[2]), tmp[0]) )
    res.sort()
    if len(res) == 0:
        return None
    i = bisect.bisect_right(res, (line_no, 0))
    return os.path.join(file_path, res[i-1][1]+'.func')

# determine whether the given code segment has been changed
def code_segment(file_path, code):
    pass

if __name__=='__main__':
    print(hash_obj('test/tmp.c/code.func'))
    print(in_function('test/tmp.c', 14))

'''
