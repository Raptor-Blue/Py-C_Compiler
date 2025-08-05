#pragma once
#include <vector>
#include <string>
#include <memory>
#include "parser.h"

using namespace std;

//---GLOBALS---
//Token Types
enum class TokenType
{
	DEF, RETURN, PRINT, CALL_METHOD,
	INT, FLOAT, STRING, BOOL, LIST, TUPLE, DICT,
	AND, OR, NOT, TRUE, FALSE,
	IF, ELIF, ELSE, FOR, WHILE, IN, RANGE, MATCH, CASE,
	IDENTIFIER, NUMBER, FLOATING, STRING_LITERAL,
	FSTRING_START, FSTRING_END, FSTRING_EXPR_START, FSTRING_EXPR_END, FSTRING_FORMAT_SPEC, ALIGNMENT,
	COLON, COMMA, SEP, DOT, LEN,
	EQUALS, EQ, NOTEQ, GREATER, LESSER, GREATEREQ, LESSEREQ,
	PLUS, MINUS, MULT, DIV,
	LPAREN, RPAREN, LBRACKET, RBRACKET, LBRACE, RBRACE,
	INDENT, DEDENT,
	NEWLINE, EOF_TOKEN
};

//Token Structure
struct Token
{
	TokenType type;
	string value;
	int line;
};

//Variable Type
enum class VarType
{
	INT, FLOAT, STRING, BOOL, LIST, TUPLE, DICT, NONE
};

//Collection Type
struct CollectionType
{
	VarType base_type;
	VarType element_type;
	VarType key_type;
	VarType value_type;
};

//---ABSTRACT SYNTAX TREE---
//Abstract Syntax Tree
struct ASTNode
{
	virtual string generate_c_code(vector<string>& gc_strings) const = 0;
	virtual ~ASTNode() = default;
};

//Helper Code Node
struct HelperNode : public ASTNode
{
	string code;

	HelperNode(const string& c) : code(c) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		return code;
	}
};

struct AssignNode : public ASTNode
{
	string var;
	string expr;
	CollectionType type;
	bool is_declaration;

	AssignNode(const string& v, const string& e, CollectionType t, bool decl) : var(v), expr(e), type(t), is_declaration(decl) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		string code;
		string c_type;

		if (type.base_type == VarType::INT)
			c_type = "int";
		else if (type.base_type == VarType::FLOAT)
			c_type = "float";
		else if (type.base_type == VarType::STRING)
		{
			c_type = "char*";
			gc_strings.push_back(var);
		}
		else if (type.base_type == VarType::BOOL)
			c_type = "int";							//BOOL as INT (?)
		else if (type.base_type == VarType::LIST)
		{
			c_type = "List" + Parser::vartype_to_c(type.element_type) + "*";
			gc_strings.push_back(var);
		}
		else if (type.base_type == VarType::TUPLE)
		{
			c_type = "Tuple" + Parser::vartype_to_c(type.element_type) + "*";
			gc_strings.push_back(var);
		}
		else if (type.base_type == VarType::DICT)
		{
			c_type = "DictString" + Parser::vartype_to_c(type.value_type) + "*";
			gc_strings.push_back(var);
		}

		if (is_declaration)
		{
			if (type.base_type == VarType::STRING)
			{
				code += "char* " + var + " = (char*)malloc(strlen(" + expr + ") + 1);\n";
				code += "    strcpy(" + var + ", " + expr + ");\n";
			}
			else if (type.base_type == VarType::LIST)
				code += c_type + " " + var + " = " + expr + ";\n";
			else if (type.base_type == VarType::TUPLE)
				code += c_type + " " + var + " = " + expr + ";\n";
			else if (type.base_type == VarType::DICT)
				code += c_type + " " + var + " = " + expr + ";\n";
			else
				code += c_type + " " + var + " = " + expr + ";\n";
		}
		else
		{
			if (type.base_type == VarType::STRING)
			{
				code += "free(" + var + ");\n";
				code += "    " + var + " = (char*)malloc(strlen(" + expr + ") + 1);\n";
				code += "    strcpy(" + var + ", " + expr + ");\n";
			}
			else
				code += var + " = " + expr + ";\n";
		}

		return code;
	}
};

struct FunctionNode : public ASTNode
{
	string name;
	vector<pair<string, CollectionType>> args;
	CollectionType return_type;
	vector<unique_ptr<ASTNode>> body;

	FunctionNode(const string& n, const vector<pair<string, CollectionType>>& a, CollectionType rt) : name(n), args(a), return_type(rt) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		string code;

		//Function Return Type
		string return_c_type = return_type.base_type == VarType::NONE ? "void" :
			return_type.base_type == VarType::INT ? "int" :
			return_type.base_type == VarType::FLOAT ? "float" :
			return_type.base_type == VarType::STRING ? "char*" :
			return_type.base_type == VarType::BOOL ? "int" :
			return_type.base_type == VarType::LIST ? "List" + Parser::vartype_to_c(return_type.element_type) + "*" :
			return_type.base_type == VarType::TUPLE ? "Tuple" + Parser::vartype_to_c(return_type.element_type) + "*" :
			"DictString" + Parser::vartype_to_c(return_type.value_type) + "*";

		code += return_c_type + " " + name + "(";

		//Function Argument Type(s)
		for (size_t i = 0; i < args.size(); ++i)
		{
			const auto& arg = args[i];
			string arg_type = arg.second.base_type == VarType::INT ? "int" :
				arg.second.base_type == VarType::FLOAT ? "float" :
				arg.second.base_type == VarType::STRING ? "char*" :
				arg.second.base_type == VarType::BOOL ? "int" :
				arg.second.base_type == VarType::LIST ? "List" + Parser::vartype_to_c(arg.second.element_type) + "*" :
				arg.second.base_type == VarType::TUPLE ? "Tuple" + Parser::vartype_to_c(arg.second.element_type) + "*" :
				"DictString" + Parser::vartype_to_c(arg.second.value_type) + "*";

			code += arg_type + " " + arg.first;

			if (arg.second.base_type == VarType::STRING || arg.second.base_type == VarType::LIST ||
				arg.second.base_type == VarType::TUPLE || arg.second.base_type == VarType::DICT)
				gc_strings.push_back(arg.first);

			if (i < args.size() - 1)
				code += ", ";
		}

		code += ")\n{\n";

		//Function Body
		for (const auto& node : body)
			code += "    " + node->generate_c_code(gc_strings) + "\n";

		//Cleanup
		for (const auto& var : gc_strings)
		{
			if (return_type.base_type == VarType::STRING && var == "return_value")
				continue;

			if (variables.find(var) != variables.end())
			{
				CollectionType var_type = variables.at(var);

				if (var_type.base_type == VarType::STRING)
					code += "    free_string(" + var + ");\n";
				else if (var_type.base_type == VarType::LIST)
					code += "    free_list_" + Parser::vartype_to_c(var_type.element_type) + "(" + var + ");\n";
				else if (var_type.base_type == VarType::TUPLE)
					code += "    free_tuple_" + Parser::vartype_to_c(var_type.element_type) + "(" + var + ");\n";
				else if (var_type.base_type == VarType::DICT)
					code += "    free_dict_string_" + Parser::vartype_to_c(var_type.value_type) + "(" + var + ");\n";
			}
		}

		gc_strings.clear();
		code += "\n}\n";

		return code;
	}

private:
	map<string, CollectionType> variables;		//Local variable tracking for cleanup
};

struct CallNode : public ASTNode
{
	string func_name;
	vector<string> args;
	CollectionType return_type;

	CallNode(const string& fn, const vector<string>& a, CollectionType rt) : func_name(fn), args(a), return_type(rt) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		string code;

		if (return_type.base_type == VarType::STRING || return_type.base_type == VarType::LIST ||
			return_type.base_type == VarType::TUPLE || return_type.base_type == VarType::DICT)
		{
			string temp_var = "temp_call_" + to_string(rand());
			string c_type = return_type.base_type == VarType::STRING ? "char*" :
				return_type.base_type == VarType::LIST ? "List" + Parser::vartype_to_c(return_type.element_type) + "*" :
				return_type.base_type == VarType::TUPLE ? "Tuple" + Parser::vartype_to_c(return_type.element_type) + "*" :
				"DictString" + Parser::vartype_to_c(return_type.value_type) + "*";

			code += c_type + " " + temp_var + " = " + func_name + "(";
			gc_strings.push_back(temp_var);
		}
		else
			code += func_name + "(";

		for (size_t i = 0; i < args.size(); ++i)
		{
			code += args[i];

			if (i < args.size() - 1)
				code += ", ";
		}

		code += ");\n";

		return code;
	}
};

struct MethodCallNode : public ASTNode
{
	string var;
	string method;
	vector<string> args;
	CollectionType return_type;

	MethodCallNode(const string& v, const string& m, const vector<string>& a, CollectionType rt) : var(v), method(m), args(a), return_type(rt) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		string code;
		string temp_var = "temp_method_" + to_string(rand());

		if (method == "append")
			code += "list_append_" + Parser::vartype_to_c(return_type.element_type) + "(" + var + ", " + args[0] + ");\n";
		else if (method == "upper" || method == "lower" || method == "strip")
		{
			code += "char* " + temp_var + " = str_" + method + "(" + var + ");\n";
			gc_strings.push_back(temp_var);
		}
		else if (method == "replace")
		{
			code += "char* " + temp_var + " = str_replace(" + var + ", " + args[0] + ", " + args[1] + ");\n";
			gc_strings.push_back(temp_var);
		}
		else if (method == "split")
		{
			code += "ListString* " + temp_var + " = str_split(" + var + ", " + (args.empty() ? "NULL" : args[0]) + ");\n";
			gc_strings.push_back(temp_var);
		}
		else if (method == "find")
			code += "int " + temp_var + " = str_find(" + var + ", " + args[0] + ");\n";

		return code;
	}
};

struct ReturnNode : public ASTNode
{
	string expr;
	CollectionType type;

	ReturnNode(const string& e, CollectionType t) : expr(e), type(t) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		if (type.base_type == VarType::STRING || type.base_type == VarType::LIST ||
			type.base_type == VarType::TUPLE || type.base_type == VarType::DICT)
			gc_strings.push_back("return_value");

		return "return_value = " + expr + ";\n    return return_value;\n";
	}
};

struct PrintNode : public ASTNode
{
	vector<pair<string, VarType>> values;
	string separator;

	PrintNode(const vector<pair<string, VarType>>& vals, const string& sep) : values(vals), separator(sep) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		string code;
		string format;
		string args;

		for (size_t i = 0; i < values.size(); ++i)
		{
			const auto& val = values[i];

			if (val.second == VarType::INT)
				format += "%d";
			else if (val.second == VarType::FLOAT)
				format += "%f";
			else if (val.second == VarType::STRING)
				format += "%s";
			else if (val.second == VarType::BOOL)
			{
				format += "%s";
				args += ", " + val.first + " ? \"true\" : \"false\"";
			}
			else if (val.second == VarType::LIST)
			{
				format += "%s";
				args += ", list_to_string_" + Parser::vartype_to_c(val.second) + "(" + val.first + ")";
			}
			else if (val.second == VarType::TUPLE)
			{
				format += "%s";
				args += ", tuple_to_string_" + Parser::vartype_to_c(val.second) + "(" + val.first + ")";
			}
			else if (val.second == VarType::DICT)
			{
				format += "%s";
				args += ", dict_to_string_string_" + Parser::vartype_to_c(val.second) + "(" + val.first + ")";
			}

			if (i < values.size() - 1)
				format += separator;

			if (val.second != VarType::BOOL && val.second != VarType::LIST &&
				val.second != VarType::TUPLE && val.second != VarType::DICT)
				args += ", " + val.first;
		}

		format += "\\n";
		code += "printf(\"" + format + "\"" + args + ");\n";

		return code;
	}
};

struct IfNode : public ASTNode
{
	string condition;
	vector<unique_ptr<ASTNode>> body;
	vector<pair<string, vector<unique_ptr<ASTNode>>>> elif_clauses;
	vector<unique_ptr<ASTNode>> else_body;

	IfNode(const string& cond) : condition(cond) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		string code = "if (" + condition + ")\n{\n";

		for (const auto& node : body)
			code += "    " + node->generate_c_code(gc_strings) + "\n";

		code += "\n}";

		for (const auto& elif : elif_clauses)
		{
			code += " else if (" + elif.first + ")\n{\n";

			for (const auto& node : elif.second)
				code += "    " + node->generate_c_code(gc_strings) + "\n";

			code += "\n}";
		}

		if (!else_body.empty())
		{
			code += " else\n{\n";

			for (const auto& node : else_body)
				code += "    " + node->generate_c_code(gc_strings) + "\n";

			code += "\n}";
		}

		code += "\n";

		return code;
	}
};

struct ForNode : public ASTNode
{
	string var;
	string start;
	string end;
	vector<unique_ptr<ASTNode>> body;

	ForNode(const string& v, const string& s, const string& e) : var(v), start(s), end(e) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		string code = "for (int " + var + " = " + start + "; " + var + " < " + end + "; " + var + "++)\n{\n";

		for (const auto& node : body)
			code += "    " + node->generate_c_code(gc_strings) + "\n";

		code += "\n}\n";

		return code;
	}
};

struct WhileNode : public ASTNode
{
	string condition;
	vector<unique_ptr<ASTNode>> body;

	WhileNode(const string& cond) : condition(cond) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		string code = "while (" + condition + ")\n{\n";

		for (const auto& node : body)
			code += "    " + node->generate_c_code(gc_strings) + "\n";

		code += "\n}\n";

		return code;
	}
};

struct MatchNode : public ASTNode
{
	string expr;
	VarType expr_type;
	vector<pair<string, vector<unique_ptr<ASTNode>>>> cases;
	vector<unique_ptr<ASTNode>> default_case;

	MatchNode(const string& e, VarType t) : expr(e), expr_type(t) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		string code = "switch (" + expr + ")\n{\n";

		for (const auto& c : cases)
		{
			code += "    case " + c.first + ":\n";

			for (const auto& node : c.second)
				code += "        " + node->generate_c_code(gc_strings) + "\n";

			code += "        break;\n";
		}

		if (!default_case.empty())
		{
			code += "    default:\n";

			for (const auto& node : default_case)
				code += "        " + node->generate_c_code(gc_strings) + "\n";

			code += "        break;\n";
		}

		code += "\n}\n";

		return code;
	}
};

struct ListNode : public ASTNode
{
	vector<string> elements;
	CollectionType type;

	ListNode(const vector<string>& elems, CollectionType t) : elements(elems), type(t) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		string temp_var = "temp_list_" + to_string(rand());
		string code = "List" + Parser::vartype_to_c(type.element_type) + "* " + temp_var +
			" = create_list_" + Parser::vartype_to_c(type.element_type) + "(" +
			to_string(elements.size()) + ");\n";

		for (size_t i = 0; i < elements.size(); ++i)
			code += "    " + temp_var + "->data[" + to_string(i) + "] = " + elements[i] + ";\n";

		gc_strings.push_back(temp_var);

		return code + "    " + temp_var;
	}
};

struct TupleNode : public ASTNode
{
	vector<string> elements;
	CollectionType type;

	TupleNode(const vector<string>& elems, CollectionType t) : elements(elems), type(t) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		string temp_var = "temp_tuple_" + to_string(rand());
		string code = "Tuple" + Parser::vartype_to_c(type.element_type) + "* " + temp_var +
			" = create_tuple_" + Parser::vartype_to_c(type.element_type) + "(" +
			to_string(elements.size()) + ");\n";

		for (size_t i = 0; i < elements.size(); ++i)
			code += "    " + temp_var + "->data[" + to_string(i) + "] = " + elements[i] + ";\n";

		gc_strings.push_back(temp_var);

		return code + "    " + temp_var;
	}
};

struct DictNode : public ASTNode
{
	vector<pair<string, string>> entries;
	CollectionType type;

	DictNode(const std::vector<std::pair<std::string, std::string>>& e, CollectionType t) : entries(e), type(t) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		string temp_var = "temp_dict_" + to_string(rand());
		string code = "DictString" + Parser::vartype_to_c(type.value_type) + "* " + temp_var +
			" = create_dict_string_" + Parser::vartype_to_c(type.value_type) + "();\n";

		for (const auto& entry : entries)
			code += "    dict_set_string_" + Parser::vartype_to_c(type.value_type) + "(" + temp_var + ", " + entry.first + ", " + entry.second + ");\n";

		gc_strings.push_back(temp_var);

		return code + "    " + temp_var;
	}
};

struct LenNode : public ASTNode
{
	string expr;
	CollectionType expr_type;

	LenNode(const string& e, CollectionType t) : expr(e), expr_type(t) {}

	string generate_c_code(vector<string>& gc_strings) const override
	{
		if (expr_type.base_type == VarType::STRING)
			return "strlen(" + expr + ")";
		else
			return expr + "->size";
	}
};