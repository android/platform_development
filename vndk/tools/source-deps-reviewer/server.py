import argparse
from flask import Flask, jsonify, render_template, request
import json
import os
from preprocess import prepare, add_pattern
import subprocess
from utils import *

app = Flask(__name__)

root_path = 'test'
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
        print(fl, code)
        '''
        if not same(os.path.join(root_path, fl), code):
            return False
        '''
        if not same( fl, code):
            return False
    return True

@app.route('/get_started')
def _get_started():
    lst, done = [], []
    for key,item in sorted(data.items()):
        lst.append(key)
        if not (len(item[0])==0 and len(item[1])==0 ):
            done.append( check(item[1]) )
        else:
            done.append(False)

    return jsonify(lst=json.dumps(lst), done=json.dumps(done),
            pattern=args.pattern, is_regex=args.is_regex)

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
    #path = os.path.join(root_path, request.args.get('path'))
    path = request.args.get('path')
    if not os.path.exists(path):
        print('No such file')
        return jsonify('')
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
    with open(data_path, 'w') as f:
        json.dump(data, f, sort_keys=True, indent=4)

    print('after update')
    print(data)
    return jsonify(result='done')

# This function add pattern to grep
@app.route('/add_pattern')
def _add_pattern():
    patt = request.args.get('pattern')
    is_regex = request.args.get('is_regex')
    print(patt, is_regex)
    add_pattern(args.android_root, patt, is_regex)
    global data
    with open(data_path, 'r') as f:
        data = json.load(f)
    print(data)
    return jsonify(result='done')

@app.route('/')
def render():
    return render_template('index.html')

@app.before_first_request
def _run_on_start():
    global data
    with open(data_path, 'r') as f:
        data = json.load(f)

if __name__=='__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--android-root', default='test')
    parser.add_argument('--pattern', default='dlopen')
    parser.add_argument('-is_regex', action='store_true')
    parser.add_argument('-load', action='store_true')

    global args
    args = parser.parse_args()
    print(args.is_regex)
    print(args.load)

    if not args.load:
        prepare(args.android_root, args.pattern, args.is_regex)
    else:
        assert os.path.exists(args.load)

    root_path = os.path.expanduser(args.android_root)
    assert os.path.exists(data_path)

    app.run(debug=True)
