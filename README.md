# lithp
an incomplete, fairly useless lisp dialect

for now its like most other dialects except slow, few features and quoted
expressions are written like `{}` instead of `'()`.


## Getting started
You need editline, on Debian based distros that is:

    `$ sudo apt-get install libedit-dev`


Also for _some_ syntax highlighting in lithp files, I have the following line
in my vim config:

    `autocmd BufNewFile,BufRead *.th  set syntax=clojure`


To run lithp:

    `$ make`
    `$ ./lithp`


## Todo
* wrap c syscalls
* a cool program in lithp
* namespaces
* docstrings for better repl experience
* write stdlib -- networking would be cool
* automatically import some core of stdlib
* arbitrary sized integers
* float data type
* macros
* tail call optimization
* garbage collection
* implement lenv with a mapping/dictionary
* some kind of ranges/list comprehensions ([] still unused)
