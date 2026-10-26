#ifdef _WIN32
/* Disable const qualifier warnings on Windows for this file */
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
#endif

#include <string.h>
#include "lexer.h"
#include "buffer.h"
#include "mystr.h"

int isalpha(char c) { return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z'; }

int isnumber(char c) { return '0' <= c && c <= '9'; }

static const char *c_keywords[] = {
    /* Control flow */
    "if",
    "else",
    "switch",
    "case",
    "default",
    "while",
    "do",
    "for",

    /* Jump statements */
    "return",
    "break",
    "continue",
    "goto",

    /* Operators / special keywords */
    "sizeof",
    "_Alignof",
    "_Generic",

    /* C11 / C23 */
    "static_assert",
    "alignas",
    "alignof",
    "noreturn",
    "thread_local",

    "", // sentinel
};

static const char *c_types[] = {
    /* Fundamental types */
    "void",
    "char",
    "short",
    "int",
    "long",
    "float",
    "double",
    "signed",
    "unsigned",
    "_Bool",
    "_Complex",
    "_Imaginary",

    /* Common typedefs */
    "size_t",
    "ptrdiff_t",
    "intptr_t",
    "uintptr_t",
    "int8_t",
    "int16_t",
    "int32_t",
    "int64_t",
    "uint8_t",
    "uint16_t",
    "uint32_t",
    "uint64_t",
    "FILE",
    "time_t",
    "clock_t",
    "wchar_t",
    "bool",

    /* Type modifiers / qualifiers */
    "const",
    "volatile",
    "restrict",
    "_Atomic",

    /* Storage class */
    "auto",
    "register",
    "static",
    "extern",
    "typedef",
    "_Thread_local",

    /* Function specifiers */
    "inline",
    "_Noreturn",

    /* User-defined types */
    "struct",
    "union",
    "enum",

    "", // sentinel
};

static const char *cpp_keywords[] = {
    /* Control flow */
    "if",
    "else",
    "switch",
    "case",
    "default",
    "while",
    "do",
    "for",

    /* Jump statements */
    "return",
    "break",
    "continue",
    "goto",

    /* Exception handling */
    "try",
    "catch",
    "throw",

    /* OOP keywords */
    "class",
    "namespace",
    "using",
    "public",
    "private",
    "protected",
    "friend",
    "virtual",
    "override",
    "final",

    /* Memory management */
    "new",
    "delete",
    "this",

    /* Template keywords */
    "template",
    "typename",

    /* Type keywords */
    "decltype",
    "auto",

    /* Function specifiers */
    "explicit",
    "inline",
    "constexpr",
    "consteval",
    "constinit",
    "noexcept",

    /* Other keywords */
    "operator",
    "sizeof",
    "typeid",
    "static_cast",
    "dynamic_cast",
    "const_cast",
    "reinterpret_cast",
    "nullptr",
    "alignof",
    "alignas",
    "static_assert",
    "concept",
    "requires",
    "co_await",
    "co_return",
    "co_yield",

    "", // sentinel
};

static const char *cpp_types[] = {
    /* Fundamental types */
    "void",
    "bool",
    "char",
    "wchar_t",
    "char8_t",
    "char16_t",
    "char32_t",
    "short",
    "int",
    "long",
    "float",
    "double",
    "signed",
    "unsigned",

    /* Type modifiers / qualifiers */
    "const",
    "volatile",
    "mutable",
    "constexpr",
    "consteval",
    "constinit",

    /* Storage class */
    "auto",
    "register",
    "static",
    "extern",
    "thread_local",

    /* User-defined types */
    "struct",
    "union",
    "enum",
    "class",
    "typedef",

    /* Common typedefs */
    "size_t",
    "ptrdiff_t",
    "nullptr_t",
    "intptr_t",
    "uintptr_t",
    "int8_t",
    "int16_t",
    "int32_t",
    "int64_t",
    "uint8_t",
    "uint16_t",
    "uint32_t",
    "uint64_t",

    "", // sentinel
};

static const char *java_keywords[] = {
    /* Control flow */
    "if",
    "else",
    "switch",
    "case",
    "default",
    "while",
    "do",
    "for",

    /* Jump statements */
    "return",
    "break",
    "continue",

    /* Exception handling */
    "try",
    "catch",
    "finally",
    "throw",
    "throws",

    /* OOP keywords */
    "class",
    "interface",
    "enum",
    "package",
    "import",
    "extends",
    "implements",
    "public",
    "private",
    "protected",
    "abstract",
    "final",
    "static",
    "synchronized",
    "native",
    "strictfp",
    "transient",
    "volatile",

    /* Object-related */
    "new",
    "this",
    "super",
    "instanceof",

    /* Boolean literals */
    "true",
    "false",
    "null",

    /* Module system (Java 9+) */
    "module",
    "requires",
    "exports",
    "opens",
    "provides",
    "uses",
    "with",

    /* Other keywords */
    "assert",
    "var",

    "", // sentinel
};

static const char *java_types[] = {
    /* Primitive types */
    "void",
    "boolean",
    "byte",
    "char",
    "short",
    "int",
    "long",
    "float",
    "double",

    /* Common wrapper classes */
    "Boolean",
    "Byte",
    "Character",
    "Short",
    "Integer",
    "Long",
    "Float",
    "Double",
    "String",
    "Object",

    /* Common collection interfaces */
    "List",
    "Set",
    "Map",
    "Collection",
    "Iterable",
    "Iterator",
    "ArrayList",
    "HashMap",
    "HashSet",
    "LinkedList",

    /* Common utility classes */
    "System",
    "Math",
    "Thread",
    "Runnable",
    "Exception",
    "RuntimeException",
    "Error",
    "Class",

    "", // sentinel
};

static const char *js_keywords[] = {
    /* Control flow */
    "if",
    "else",
    "switch",
    "case",
    "default",
    "while",
    "do",
    "for",

    /* Jump statements */
    "return",
    "break",
    "continue",

    /* Function keywords */
    "function",
    "async",
    "await",
    "yield",

    /* Variable declarations */
    "var",
    "let",
    "const",

    /* OOP keywords */
    "class",
    "extends",
    "super",
    "this",
    "new",
    "static",

    /* Exception handling */
    "try",
    "catch",
    "finally",
    "throw",

    /* Operators */
    "typeof",
    "instanceof",
    "delete",
    "in",
    "of",
    "void",

    /* Module system */
    "import",
    "export",
    "from",
    "as",
    "default",

    /* Other keywords */
    "with",
    "debugger",

    /* Reserved for future use */
    "enum",
    "implements",
    "interface",
    "package",
    "private",
    "protected",
    "public",

    "", // sentinel
};

static const char *js_types[] = {
    /* Primitive wrapper objects */
    "Object",
    "Array",
    "String",
    "Number",
    "Boolean",
    "Symbol",
    "BigInt",

    /* Special values */
    "undefined",
    "null",
    "true",
    "false",
    "NaN",
    "Infinity",

    /* Built-in objects */
    "Function",
    "Date",
    "RegExp",
    "Error",
    "TypeError",
    "ReferenceError",
    "SyntaxError",
    "RangeError",

    /* Collections */
    "Map",
    "Set",
    "WeakMap",
    "WeakSet",
    "Promise",

    /* Typed arrays */
    "ArrayBuffer",
    "Int8Array",
    "Uint8Array",
    "Int16Array",
    "Uint16Array",
    "Int32Array",
    "Uint32Array",
    "Float32Array",
    "Float64Array",

    /* Global objects */
    "Math",
    "JSON",
    "console",
    "window",
    "document",
    "global",
    "process",

    /* Reflect and Proxy */
    "Reflect",
    "Proxy",

    "", // sentinel
};

static const char *ts_keywords[] = {
    /* Control flow */
    "if",
    "else",
    "switch",
    "case",
    "default",
    "while",
    "do",
    "for",

    /* Jump statements */
    "return",
    "break",
    "continue",

    /* Function keywords */
    "function",
    "async",
    "await",
    "yield",

    /* Variable declarations */
    "var",
    "let",
    "const",

    /* OOP keywords */
    "class",
    "extends",
    "super",
    "this",
    "new",
    "static",

    /* Exception handling */
    "try",
    "catch",
    "finally",
    "throw",

    /* Operators */
    "typeof",
    "instanceof",
    "delete",
    "in",
    "of",
    "void",

    /* Module system */
    "import",
    "export",
    "from",
    "as",
    "default",

    /* TypeScript-specific keywords */
    "interface",
    "type",
    "enum",
    "namespace",
    "module",
    "declare",
    "abstract",
    "implements",
    "public",
    "private",
    "protected",
    "readonly",
    "get",
    "set",
    "constructor",

    /* Type operators */
    "keyof",
    "infer",
    "is",
    "asserts",
    "satisfies",

    /* Other keywords */
    "with",
    "debugger",
    "package",

    "", // sentinel
};

static const char *ts_types[] = {
    /* TypeScript primitive types */
    "any",
    "unknown",
    "never",
    "void",
    "string",
    "number",
    "boolean",
    "bigint",
    "symbol",
    "object",
    "undefined",
    "null",

    /* Special values */
    "true",
    "false",
    "NaN",
    "Infinity",

    /* Built-in utility types */
    "Partial",
    "Required",
    "Readonly",
    "Record",
    "Pick",
    "Omit",
    "Exclude",
    "Extract",
    "NonNullable",
    "ReturnType",
    "InstanceType",
    "Parameters",
    "ConstructorParameters",
    "Awaited",

    /* Primitive wrapper objects */
    "Object",
    "Array",
    "String",
    "Number",
    "Boolean",
    "Symbol",
    "BigInt",

    /* Built-in objects */
    "Function",
    "Date",
    "RegExp",
    "Error",
    "TypeError",
    "ReferenceError",
    "SyntaxError",
    "RangeError",

    /* Collections */
    "Map",
    "Set",
    "WeakMap",
    "WeakSet",
    "Promise",

    /* Typed arrays */
    "ArrayBuffer",
    "Int8Array",
    "Uint8Array",
    "Int16Array",
    "Uint16Array",
    "Int32Array",
    "Uint32Array",
    "Float32Array",
    "Float64Array",

    /* Global objects */
    "Math",
    "JSON",
    "console",
    "window",
    "document",
    "global",
    "process",

    /* Reflect and Proxy */
    "Reflect",
    "Proxy",

    "", // sentinel
};

static const char *python_keywords[] = {
    /* Control flow */
    "if",
    "elif",
    "else",
    "while",
    "for",
    "break",
    "continue",
    "pass",

    /* Function keywords */
    "def",
    "return",
    "yield",
    "lambda",

    /* Class keywords */
    "class",

    /* Exception handling */
    "try",
    "except",
    "finally",
    "raise",
    "assert",

    /* Context managers */
    "with",
    "as",

    /* Import statements */
    "import",
    "from",

    /* Boolean and None */
    "True",
    "False",
    "None",

    /* Logical operators */
    "and",
    "or",
    "not",
    "is",
    "in",

    /* Variable scope */
    "global",
    "nonlocal",
    "del",

    /* Async */
    "async",
    "await",

    /* Other */
    "match",
    "case",

    "", // sentinel
};

static const char *python_types[] = {
    /* Built-in types */
    "int",
    "float",
    "complex",
    "str",
    "bool",
    "list",
    "tuple",
    "dict",
    "set",
    "frozenset",
    "bytes",
    "bytearray",
    "memoryview",
    "range",

    /* Special objects */
    "object",
    "type",
    "super",
    "property",
    "staticmethod",
    "classmethod",

    /* Common built-in functions that look like types */
    "len",
    "enumerate",
    "zip",
    "map",
    "filter",
    "iter",
    "next",
    "reversed",
    "sorted",
    "sum",
    "min",
    "max",
    "abs",
    "round",
    "pow",
    "divmod",
    "all",
    "any",
    "callable",
    "isinstance",
    "issubclass",
    "hasattr",
    "getattr",
    "setattr",
    "delattr",
    "dir",
    "vars",
    "id",
    "hash",
    "hex",
    "oct",
    "bin",
    "chr",
    "ord",
    "ascii",
    "repr",
    "format",
    "open",
    "input",
    "print",

    /* Common exceptions */
    "Exception",
    "BaseException",
    "ValueError",
    "TypeError",
    "KeyError",
    "IndexError",
    "AttributeError",
    "NameError",
    "RuntimeError",
    "NotImplementedError",
    "StopIteration",
    "ImportError",
    "IOError",
    "FileNotFoundError",
    "ZeroDivisionError",
    "KeyboardInterrupt",
    "SystemExit",

    /* Special names */
    "self",
    "cls",
    "__init__",
    "__name__",
    "__main__",
    "__file__",
    "__doc__",
    "__dict__",
    "__class__",
    "__str__",
    "__repr__",

    "", // sentinel
};

void lexer_init(Lexer *l, const char *source, int lang) {
    l->source = source;
    l->pos = 0;
    l->pos_screen = 0;
    l->state = LEX_BEGIN;
    l->finished = 0;
    l->lang = lang;
    if (lang == LANG_C){
	  l->keywords = c_keywords;
	  l->types = c_types;
    } else if (lang == LANG_CPP){
	  l->keywords = cpp_keywords;
	  l->types = cpp_types;
    } else if (lang == LANG_JAVA){
	  l->keywords = java_keywords;
	  l->types = java_types;
    } else if (lang == LANG_JS){
	  l->keywords = js_keywords;
	  l->types = js_types;
    } else if (lang == LANG_TS){
	  l->keywords = ts_keywords;
	  l->types = ts_types;
    } else if (lang == LANG_PY){
	  l->keywords = python_keywords;
	  l->types = python_types;
    }
	MyStr_init(&l->mystr, source);
}

int in_list(const char *c, int len, const char *kw[]) {
    for (int i = 0; kw[i][0] != '\0'; i++) {
        if ((int)strlen(kw[i]) == len && memcmp(c, kw[i], len) == 0) {
            return 1;
        }
    }

    return 0;
}

int lexer_next(Lexer *l, Token *out_token) {
    if (l->finished) {
        return 0;
    }

    // If starting in a multi-line state, initialize the token
    if (l->state == LEX_SLASH_STAR || l->state == LEX_SLASH_STAR_STAR) {
        out_token->type = TOK_COMMENT;
        out_token->start = 0;
    }

    int emit = 0;
    while (1) {
	  //char c = l->source[l->pos];
	  char c;
		if (l->reprocess == 1){
		  l->reprocess = 0;
		} else {
		  if (MyStr_next_cluster(&l->mystr)) {
			if (l->mystr.width_codepoints > 1) continue;
			if (l->mystr.first_codepoint > 256) continue;
		  }
		  else{
			l->mystr.first_codepoint = 0;
		  }
		}
		c = l->mystr.first_codepoint;

        switch (l->state) {
        case LEX_LITERAL_STRING_ESCAPE:
            l->state = LEX_LITERAL_STRING;
            break;
        case LEX_LITERAL_STRING:
            if (c == '\\') {
                l->state = LEX_LITERAL_STRING_ESCAPE;
            }
            if (c == '"') {
                out_token->end = l->pos;
                //out_token->end_screen = l->pos_screen;
				out_token->end_screen = l->mystr.pos_column;
                out_token->color = 125; // red
                emit = 1;
                l->state = LEX_NORMAL;
            }
            break;
        case LEX_BEGIN_HASH:
            if (c == '\0') {
                out_token->end = l->pos;
                //out_token->end_screen = l->pos_screen;
				out_token->end_screen = l->mystr.pos_column;
                out_token->color = 94; // brown
                emit = 1;
                l->finished = 1;
                l->state = LEX_BEGIN;
            }
            break;
        case LEX_BEGIN:
            /* fall through */ // ← Add this comment
        case LEX_NORMAL:
            if (c == '#') {
                if (l->lang == LANG_PY) {
                    // Python comments
                    l->state = LEX_SLASH_SLASH;
                    out_token->type = TOK_COMMENT;
                    out_token->start = l->pos;
                    out_token->start_screen = l->mystr.pos_column;
                } else {
                    // C/C++ preprocessor directives
                    l->state = LEX_BEGIN_HASH;
                    out_token->type = TOK_PREPROC;
                    out_token->start = l->pos;
                    out_token->start_screen = l->mystr.pos_column;
                }
            }
            if (c == '/') {
                l->state = LEX_SLASH;
            }
            if (c == '"') {
                if (l->lang == LANG_PY) {
                    l->state = LEX_PY_QUOTE_DOUBLE_1;
                    out_token->start = l->pos;
                    out_token->start_screen = l->mystr.pos_column;
                } else {
                    l->state = LEX_LITERAL_STRING;
                    out_token->type = TOK_LITERAL_STRING;
                    out_token->start = l->pos;
                    out_token->start_screen = l->mystr.pos_column;
                }
            }
            if (c == '\'') {
                if (l->lang == LANG_PY) {
                    l->state = LEX_PY_QUOTE_SINGLE_1;
                    out_token->start = l->pos;
                    out_token->start_screen = l->mystr.pos_column;
                }
            }
            if (c == '{' || c == '}' || c == '(' || c == ')' || c == '[' || c == ']') {
                out_token->type = TOK_BRACKET;
                out_token->start = l->pos;
                out_token->end = l->pos + 1;
                //out_token->start_screen = l->pos_screen;
                out_token->start_screen = l->mystr.pos_column;
                //out_token->end_screen = l->pos_screen + 1;
                out_token->end_screen = l->mystr.pos_column + 1;
                out_token->color = 27; // blue
                emit = 1;
            }
            if (c == '=' || c == '-' || c == '>' || c == ';') {
                out_token->type = TOK_BRACKET;
                out_token->start = l->pos;
                out_token->end = l->pos + 1;
                //out_token->start_screen = l->pos_screen;
                out_token->start_screen = l->mystr.pos_column;
                //out_token->end_screen = l->pos_screen + 1;
                out_token->end_screen = l->mystr.pos_column + 1;
                out_token->color = 240;
                emit = 1;
            }
            if (isalpha(c) || c == '_') {
                out_token->start = l->pos;
                //out_token->start_screen = l->pos_screen;
                out_token->start_screen = l->mystr.pos_column;
                l->state = LEX_IDENTIFIER;
            }
            if (isnumber(c)) {
                out_token->type = TOK_LITERAL_NUMBER;
                out_token->start = l->pos;
                //out_token->start_screen = l->pos_screen;
                out_token->start_screen = l->mystr.pos_column;
                l->state = LEX_LITERAL_NUMBER;
            }
            break;
        case LEX_LITERAL_NUMBER:
            if (isnumber(c)) {
            } else {
                out_token->type = TOK_LITERAL_NUMBER;
                out_token->end = l->pos;
                //out_token->end_screen = l->pos_screen;
				out_token->end_screen = l->mystr.pos_column;
                out_token->color = 28; // green
                emit = 1;
                l->state = LEX_NORMAL;
				l->reprocess = 1;
                l->pos--;
                l->pos_screen--; // reprocess character that ended the literal
            }
            break;
        case LEX_IDENTIFIER:
            if (isalpha(c) || isnumber(c) || c == '_') {
            } else if (c == '(') {
                out_token->type = TOK_FUNCTION;
                out_token->end = l->pos;
                //out_token->end_screen = l->pos_screen;
                out_token->end_screen = l->mystr.pos_column;
                // out_token->color = 20; // blue
                // out_token->color = 5; // purple
                out_token->color = 94; // brown
                emit = 1;
                l->state = LEX_NORMAL;
				l->reprocess = 1;
                l->pos--;        // reprocess c in normal mode
                l->pos_screen--; // reprocess c in normal mode
            } else {
                out_token->type = TOK_IDENTIFIER;
                out_token->end = l->pos;
                //out_token->end_screen = l->pos_screen;
                out_token->end_screen = l->mystr.pos_column;
                out_token->color = 16; // black
                char *str = l->source + out_token->start;
                int width = out_token->end - out_token->start;
                // if (is_keyword(l->source + out_token->start, out_token->end - out_token->start))
                if (in_list(str, width, l->keywords))
                    out_token->color = 90; // pink
                // else if (is_type(l->source + out_token->start, out_token->end -
                // out_token->start))
                if (in_list(str, width, l->types))
                    out_token->color = 27; // blue
                emit = 1;
                l->state = LEX_NORMAL;
				l->reprocess = 1;
                l->pos_screen--; // reprocess c in normal mode
                l->pos--;        // reprocess c in normal mode
            }
            break;
        case LEX_SLASH:
            if (c == '*') {
                l->state = LEX_SLASH_STAR;
                out_token->start = l->pos - 1;
                out_token->start_screen = l->pos_screen - 1;
            } else if (c == '/') {
                l->state = LEX_SLASH_SLASH;
                out_token->start = l->pos - 1;
                out_token->start_screen = l->pos_screen - 1;
            } else {
                l->state = LEX_NORMAL;
                l->pos_screen--; // reprocess
                l->pos--;        // reprocess
            }
            break;
        case LEX_SLASH_SLASH:
            if (c == '\0') {
                out_token->type = TOK_COMMENT;
                out_token->end = l->pos;
                //out_token->end_screen = l->pos_screen;
				out_token->end_screen = l->mystr.pos_column;
                out_token->color = 28; // green
                emit = 1;
                l->finished = 1;
                l->state = LEX_NORMAL;
            }
            break;
        case LEX_SLASH_STAR:
            if (c == '\0') {
                out_token->type = TOK_COMMENT;
                out_token->end = l->pos;
                //out_token->end_screen = l->pos_screen;
				out_token->end_screen = l->mystr.pos_column;
                out_token->color = 28; // green
                emit = 1;
                l->finished = 1;
            }
            if (c == '*') {
                l->state = LEX_SLASH_STAR_STAR;
            }
            break;
        case LEX_SLASH_STAR_STAR:
            if (c == '/') {
                out_token->type = TOK_COMMENT;
                out_token->end = l->pos + 1;
                //out_token->end_screen = l->pos_screen + 1;
                out_token->end_screen = l->mystr.pos_column + 1;
                out_token->color = 28; // green

                emit = 1;
                l->state = LEX_NORMAL;
            } else if (c == '*') {
                l->state = LEX_SLASH_STAR_STAR;
            } else {
                l->state = LEX_SLASH_STAR;
            }
            break;

        // Python string states
        case LEX_PY_QUOTE_DOUBLE_1:
            if (c == '"') {
                l->state = LEX_PY_QUOTE_DOUBLE_2;
            } else {
                // Single " string
                l->state = LEX_LITERAL_STRING;
                out_token->type = TOK_LITERAL_STRING;
                l->reprocess = 1;
                l->pos--;
                l->pos_screen--;
            }
            break;

        case LEX_PY_QUOTE_DOUBLE_2:
            if (c == '"') {
                // Triple quote!
                l->state = LEX_PYTHON_TRIPLE_DOUBLE;
                out_token->type = TOK_LITERAL_STRING;
            } else {
                // Empty string "", emit it
                out_token->type = TOK_LITERAL_STRING;
                out_token->end = l->pos;
                out_token->end_screen = l->mystr.pos_column;
                out_token->color = 125;
                emit = 1;
                l->state = LEX_NORMAL;
                l->reprocess = 1;
                l->pos--;
                l->pos_screen--;
            }
            break;

        case LEX_PY_QUOTE_SINGLE_1:
            if (c == '\'') {
                l->state = LEX_PY_QUOTE_SINGLE_2;
            } else {
                // Single ' string
                l->state = LEX_LITERAL_STRING_SINGLE;
                out_token->type = TOK_LITERAL_STRING;
                l->reprocess = 1;
                l->pos--;
                l->pos_screen--;
            }
            break;

        case LEX_PY_QUOTE_SINGLE_2:
            if (c == '\'') {
                // Triple quote!
                l->state = LEX_PYTHON_TRIPLE_SINGLE;
                out_token->type = TOK_LITERAL_STRING;
            } else {
                // Empty string '', emit it
                out_token->type = TOK_LITERAL_STRING;
                out_token->end = l->pos;
                out_token->end_screen = l->mystr.pos_column;
                out_token->color = 125;
                emit = 1;
                l->state = LEX_NORMAL;
                l->reprocess = 1;
                l->pos--;
                l->pos_screen--;
            }
            break;

        case LEX_LITERAL_STRING_SINGLE:
            if (c == '\\') {
                l->state = LEX_LITERAL_STRING_SINGLE_ESCAPE;
            }
            if (c == '\'') {
                out_token->end = l->pos;
                out_token->end_screen = l->mystr.pos_column;
                out_token->color = 125; // red
                emit = 1;
                l->state = LEX_NORMAL;
            }
            break;

        case LEX_LITERAL_STRING_SINGLE_ESCAPE:
            l->state = LEX_LITERAL_STRING_SINGLE;
            break;

        case LEX_PYTHON_TRIPLE_DOUBLE:
            if (c == '"') {
                l->state = LEX_PYTHON_TRIPLE_DOUBLE_1;
            }
            if (c == '\0') {
                out_token->type = TOK_LITERAL_STRING;
                out_token->end = l->pos;
                out_token->end_screen = l->mystr.pos_column;
                out_token->color = 125; // red
                emit = 1;
                l->finished = 1;
            }
            break;

        case LEX_PYTHON_TRIPLE_DOUBLE_1:
            if (c == '"') {
                l->state = LEX_PYTHON_TRIPLE_DOUBLE_2;
            } else {
                // False alarm, back to triple quote
                l->state = LEX_PYTHON_TRIPLE_DOUBLE;
            }
            break;

        case LEX_PYTHON_TRIPLE_DOUBLE_2:
            if (c == '"') {
                // Closing triple quote!
                out_token->end = l->pos + 1;
                out_token->end_screen = l->mystr.pos_column + 1;
                out_token->color = 125;
                emit = 1;
                l->state = LEX_NORMAL;
            } else {
                // False alarm, back to triple quote
                l->state = LEX_PYTHON_TRIPLE_DOUBLE;
            }
            break;

        case LEX_PYTHON_TRIPLE_SINGLE:
            if (c == '\'') {
                l->state = LEX_PYTHON_TRIPLE_SINGLE_1;
            }
            if (c == '\0') {
                out_token->type = TOK_LITERAL_STRING;
                out_token->end = l->pos;
                out_token->end_screen = l->mystr.pos_column;
                out_token->color = 125; // red
                emit = 1;
                l->finished = 1;
            }
            break;

        case LEX_PYTHON_TRIPLE_SINGLE_1:
            if (c == '\'') {
                l->state = LEX_PYTHON_TRIPLE_SINGLE_2;
            } else {
                // False alarm, back to triple quote
                l->state = LEX_PYTHON_TRIPLE_SINGLE;
            }
            break;

        case LEX_PYTHON_TRIPLE_SINGLE_2:
            if (c == '\'') {
                // Closing triple quote!
                out_token->end = l->pos + 1;
                out_token->end_screen = l->mystr.pos_column + 1;
                out_token->color = 125;
                emit = 1;
                l->state = LEX_NORMAL;
            } else {
                // False alarm, back to triple quote
                l->state = LEX_PYTHON_TRIPLE_SINGLE;
            }
            break;
        }
        l->pos++;
        // l->pos_screen+=cp_width(c);
        if (c == '\t')
            l->pos_screen += 4;
        else
            l->pos_screen++;
        if (emit) {
            return 1;
        }
        if (c == '\0')
            return 0;
    }
    return 0;
}
