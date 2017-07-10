from flask import Flask, jsonify, render_template, request
from utils import *
from preprocess import prepare
import json
import os
app = Flask(__name__)

import argparse

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
        if not same(os.path.join(root_path, fl), code):
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

    print(data)
    return jsonify(lst=json.dumps(lst), done=json.dumps(done))

@app.route('/load_file')
def _load_file():
    path = request.args.get('path')

    if path not in data.keys():
        print('No such entry', path)
        return jsonify(result='')
    deps, codes = data[path]

    print(data)
    return jsonify(deps=json.dumps(deps),codes=json.dumps(codes))

@app.route('/get_file')
def _get_file():
    path = os.path.join(root_path, request.args.get('path'))
    if not os.path.exists(path):
        print('No such file')
        return jsonify('')
    with open(path, 'r') as f:
        code = f.read()
    print(data)
    return jsonify(result=code)

@app.route('/save_all')
def _save_all():
    path = request.args.get('path')
    deps = json.loads(request.args.get('deps'))
    codes = json.loads(request.args.get('codes'))
    #data = json.load(data_path)
    data[path] = (deps, codes)
    # save update to file
    with open(data_path, 'w') as f:
        json.dump(data, f)

    print('after update')
    print(data)
    return jsonify(result='fuck')

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
    parser.add_argument('--is-regex', default=False)
    args = parser.parse_args()
    print(args.is_regex)
    prepare(args.android_root, args.pattern, args.is_regex)
    root_path = os.path.expanduser(args.android_root)
    assert os.path.exists(data_path)

    app.run(debug=True)
