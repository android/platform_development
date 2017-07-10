import argparse
import bisect
import hashlib
import json
import os
import subprocess
import sys
from builtins import input
from data_utils import *
from flask import Flask, jsonify, render_template, request
from preprocess import add_pattern, prepare

app = Flask(__name__)

pattern_path = 'patterns'
data_path = 'data.json'
data = {}

# whether the code segment is exactly in file
def same(fl, code):
    with open(fl, 'r') as f:
        fc = f.read()
        return code in fc

# check if the file needes to be reiewed again
def check(codes):
    for item in codes:
        fl = item.split(':')[0]
        code = item[len(fl)+1:]
        if not same( fl, code):
            return False
    return True

@app.route('/get_started')
def _get_started():
    lst, done= [], []
    for key,item in sorted(data.items()):
        lst.append(key)
        if not (len(item[0])==0 and len(item[1])==0 ):
            done.append( check(item[1]) )
        else:
            done.append(False)

    pattern_lst = load_pattern()

    return jsonify(lst=json.dumps(lst),
                   done=json.dumps(done),
                   pattern_lst=json.dumps(pattern_lst))

@app.route('/load_file')
def _load_file():
    path = request.args.get('path')

    if path not in data.keys():
        print('No such entry', path)
        return jsonify(result='')
    deps, codes = data[path]

    return jsonify(deps=json.dumps(deps),codes=json.dumps(codes))

@app.route('/get_file')
def _get_file():
    path = request.args.get('path')
    if not os.path.exists(path):
        return jsonify(result='No such file')
    with open(path, 'r') as f:
        code = f.read()

    return jsonify(result=code)

@app.route('/save_all')
def _save_all():
    path = request.args.get('path')
    deps = json.loads(request.args.get('deps'))
    codes = json.loads(request.args.get('codes'))

    data[path] = (deps, codes)
    # save update to file
    save_data(data)
    return jsonify(result='done')

# This function add pattern to grep
@app.route('/add_pattern')
def _add_pattern():
    patt = request.args.get('pattern')
    is_regex = request.args.get('is_regex')
    add_pattern(args.android_root, patt, is_regex)
    global data
    data = load_data()
    save_pattern(patt)
    return jsonify(result='done')

@app.route('/')
def render():
    return render_template('index.html')

@app.before_first_request
def _run_on_start():
    global data
    data = load_data()

# reference:
# https://stackoverflow.com/questions/3041986/apt-command-line-interface-like-yes-no-input
def query_yes_no(question, default="yes"):
    """Ask a yes/no question via input() and return their answer.
    "question" is a string that is presented to the user.
    "default" is the presumed answer if the user just hits <Enter>.
    It must be "yes" (the default), "no" or None (meaning
    an answer is required of the user).
    The "answer" return value is True for "yes" or False for "no".
    """
    valid = {"yes": True, "y": True, "ye": True,
             "no": False, "n": False}
    if default is None:
        prompt = " [y/n] "
    elif default == "yes":
        prompt = " [Y/n] "
    elif default == "no":
        prompt = " [y/N] "
    else:
        raise ValueError("invalid default answer: '%s'" % default)
    while True:
        sys.stdout.write(question + prompt)
        choice = input().lower()
        if default is not None and choice == '':
            return valid[default]
        elif choice in valid:
            return valid[choice]
        else:
            sys.stdout.write("Please respond with 'yes' or 'no' "
                             "(or 'y' or 'n').\n")

if __name__=='__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--android-root', default='test')
    parser.add_argument('--pattern', default='dlopen')
    parser.add_argument('-is_regex', action='store_true')

    global args
    args = parser.parse_args()

    if os.path.exists(data_path):
        confirm = query_yes_no('Overwrite previous results')
        if confirm:
            prepare(args.android_root, args.pattern, args.is_regex)
    else:
        prepare(args.android_root, args.pattern, args.is_regex)

    assert_data_exist()
    app.run()
