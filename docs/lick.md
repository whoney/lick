Lick syntax
===========

Types
-----

* Integers: `12345`, `0xDEAD`
* Strings: `"hello\n"`
* Arrays: `[1, 2, 3]`
* Dicts: `{ "key": "value", "otherkey": "othervalue" }`, also `{ .key: "value", .otherkey: "othervalue" }`

  Note for dicts: keys are always evaluated and stored as strings. Both dict["z"] and dict.z forms of subscription are supported.
  
### Rules for + and += operators

* If both arguments are arrays, concatenate them
* If one argument is an array, append or prepend it to the array, like: `2 + [3, 4] == [2, 3, 4]`
* If one of the argument is a string, concatenate, like: `2 + "az" == "2az"`
* When nothing of the above is true, convert both sides to integers and perform a numeric addition.

### Comments

Both `/* .... */` and `// ....` forms are supported.

Predefined variables
--------------------

* `sys.platform` - either "windows" or "unix"
* `sys.hostname` - computer name on Windows, hostname on *nix
* `sys.username` - user name
* `sys.bits` - 32 or 64
* `sys.env` - dictionary with environment variables, excluding PATH. it is writeable. 
* `sys.path` - PATH environment variable, parsed into an array. writeable too.

Control statements
------------------

* `if (x) then-statement [ else else-statement ]`
* `for (init-expr; while-expr; next-expr) loop-statement`
* `for (var-name in array-expr) loop-statement`
* `while (while-expr) loop-statement`
* `break;`
* `continue;`
* `return return-expr;`

Includes and uses
-----------------

* `include expr;`

  Includes specified file; multiple inclusions are automatically prevented. Included file is parsed and
  executed in the same execution context; that means, included file can affect variables in the original scope.
  
* `using expr;`

  Includes specified file, also setting project path to the location of included file. Targets contained in the
  included file are ignored; also, cd() to the location of the included file is performed before execution of this
  statement.
  
  This is intended to be used in sub-projects (sub-project is said to be "using" the main project, in order to inherit
  project-wide settings and libraries, and also share the hash store).
  
* `lick (file-name [, target [, arguments]])`

  Invoke specified target in another file, creating new execution context. If target is omitted, invokes default() or all().


Dependency tracking
-------------------

    depends (expression) { statements }
    
Expression is treated as a list of file names. Example:

    depends (files (".").match("*.cpp")) {
		run ("make");
    }
    

Functions and targets
---------------------

    function name (arg1, arg2) { statements }
or
  
    target name (arg1, arg2) { statements }

Target is simply a function which can be invoked from command line.

Dot syntax for function invocation
----------------------------------

Functions can be invoked as:

    function_name (arg1, arg2)

or

    arg1.function_name (arg2)

Both forms are identical, second form makes it easier to write things like:

    files ("some-dir").match ("*.cpp")

which is the same as:

    match (files ("some-dir"), "*.cpp")

Built-in functions
------------------

* `strlen (x)`
* `length(x)` - returns number of elements in array or dict; same as strlen() for strings
* `replace (where, what, replacement) `- replace every occurence of "what" in "where" with "replacement".
  if where is an array, replacement will be performed for every entry, like:
  
      replace (["ab", "ac"], "a", "z") == ["zb", "zc"]

* `print (...)` - print arguments as strings
* `println (...)` - print arguments as strings and add a newline
* `files (path)` - return array with every file in directory specified by "path", recursively
* `match (name-array, filter1, filter2, ... filterN)` - match "name-array" against shell patterns specified by filter1..filterN,
  returning array with matching entries, like:
  
      match (["a.cpp", "b.cpp", "a.h"], "*.cpp") == ["a.cpp", "b.cpp"]

* `exclude (name-array, filter1, filter2, ... filterN)` - same as match, but return non-matching entries
* `readfile (file-name)` - read entire contents of a file and return as string
* `writefile (file-name, contents)` - write contents into file
* `run (....)` - execute a command. Accepts any mix of strings and arrays as parameters.

If parameter is a string, it is split into separate arguments on spaces, like:
  
    run ("make all"); // will invoke "make" with argument "all"

If parameter is an array, its arguments will be passed as is, like:

    run ("gcc", ["my program.c", "my program.h"]) // will invoke "gcc" with two arguments: "my program.c" and "my program.h"

Note that `run()` invokes `CreateProcess()` on Windows and `exec()` on Unix, meaning that it cannot be used to perform shell commands.
This is good, because shell commands are not portable anyway. Use lick functions instead.

* `capture (....)` - same as run(), but capture standard output and return it
* `exists (file-name)` - returns true if file exists
* `abspath (file-name)` - returns absolute path of file. if file-name is an array, returns an array of absolute names
* `relpath (file-name [, relative_to])` - returns relative path of file. if file-name is an array, returns an array of relative names; if relative_to is omitted, cwd() is assumed
* `dirname (file-name)` - returns directory component of file name (or "." if empty)
* `cd (dir-name)` - changes current working directory. Changes are local to code block!
* `mkdir (dir-name)` - create directory (and intermediate directories if needed)
* `cwd ()` - returns current working directory
* `contains (array, entry)` - returns true if array contains entry
* `implode (array [, separator])` - convert array to string, using separator. Space is the default
* `explode (string [, separators])` - convert string to array, using separators (space and \n\r\t are defaults)
* `fail (reason)` - fail the build
* `contains (array, entry)` - returns true if array contains entry
* `substr (string, start [, length])` - return substring
* `chr (int)` - convert integer value to single-character string; chr(0) is ok!
* `ord (string)` - convert first character of a string to integer value
* `char_at (string, pos)` - return character at specified position (like substr with length == 1)
* `hex (int [, length])` - convert integer to hex representation. if length is specified, pad with zeros 
* `sep ([string])` - if string is specified, replace path separators with system path separators. without arguments, returns system path separator
* `copy (..., to)` - copy files (args can be any combination of arrays and strings. directories will be copied recursively)
* `delete (...)` - delete files (args can be any combination of arrays and strings)

Path separators
---------------

File system functions like `files()` will always return filenames separated with "/", and will accept both "/" and "\\" as separator.


