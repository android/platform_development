import flask_testing
import os
import unittest
from flask import Flask, jsonify, render_template, request
from flask_testing import LiveServerTestCase, TestCase
from preprocess import prepare
from server import *
from urllib.request import urlopen

app.config['TESTING'] = True

class TestViews(TestCase):
    def create_app(self):

        return app

    def test_get_file(self):
        test_arg = 'example.c'
        response = self.client.get("/get_file", query_string=dict(path=test_arg))
        ret = response.json['result']
        with open(os.path.join(root_path, test_arg), 'r') as f:
            self.assertEqual(ret, f.read())

    def test_load_file(self):
        test_arg = 'example.c:53'
        response = self.client.get("/load_file", query_string=dict(path=test_arg))
        deps, codes = json.loads(response.json['deps']), json.loads(response.json['codes'])
        with open(data_path, 'r') as f:
            cdata = json.load(f)

        self.assertEqual(deps, cdata[test_arg][0])
        self.assertEqual(codes, cdata[test_arg][1])

    def test_save_all(self):
        test_arg = {
                'path': 'example.c:53',
                'deps': json.dumps(['test_lib.so']),
                'codes': json.dumps(['example.c:#define HAL_LIBRARY_PATH1',
                    'example.c:#define HAL_LIBRARY_PATH2'])
                }
        response = self.client.get("/save_all", query_string=test_arg)

        with open(data_path, 'r') as f:
            cdata = json.load(f)

        self.assertEqual(['test_lib.so'],  cdata[test_arg['path']][0])
        self.assertEqual(['example.c:#define HAL_LIBRARY_PATH1', 'example.c:#define HAL_LIBRARY_PATH2'], cdata[test_arg['path']][1])

class TestPreprocess(unittest.TestCase):
    def test_prepare(self):
        os.remove(data_path)
        prepare(android_root='test', pattern='dlopen', is_regex=False)
        self.assertTrue( os.path.exists(data_path) )

if __name__ == '__main__':
    prepare('test', 'dlopen', False)
    unittest.main()
