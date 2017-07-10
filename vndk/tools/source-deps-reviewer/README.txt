requirements:
  Google codesearch is used, install cindex, csearch by
  $ go get github.com/google/codesearch/cmd/cindex
  $ go get github.com/google/codesearch/cmd/csearch

Usage:
  Add flag -is_regex if the pattern given is a regex
  python3 server.py --android-root DIRECTORY_ROOT_PATH --pattern dlopen 
  open browser at localhost:5000

  If this is not the first time running this tool on the directory, add -load to load previous results
  python3 server.py --android-root DIRECTORY_ROOT_PATH --pattern  dlopen  -load 

Esc:
  Prism, a syntax highlighter is used
  all codes of Prism can be found at https://github.com/PrismJS/prism

To run functional_tests.py 
  $ pip install -r requirements.txt
  $ python function_tests.py

