#pragma once
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <map>
#include <set>
#include <sstream>
#include "ASTNodes.h"

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

//---PARSER---
class Parser
{
private:
	vector<Token> tokens;
	size_t pos;
	map<string, CollectionType> variables;
	map<string, pair<vector<CollectionType>, CollectionType>> functions;
	int string_temp_counter;
	CollectionType expr_type;
	set<string> helper_includes;

	struct FormatSpec
	{
		string alignment;
		string width;
		string precision;
		char type;
	};

public:
	Parser(const vector<Token>& t) : tokens(t), pos(0), string_temp_counter(0)
	{
		helper_includes.insert("common.h");		//Always include common.h for standard includes
	}

	vector<unique_ptr<ASTNode>> parse_program()
	{
		vector<unique_ptr<ASTNode>> program;

		string include_code;

		for (const auto& include : helper_includes)
			include_code += "#include \"" + include + "\"\n";

		program.push_back(make_unique<HelperNode>(include_code));

		while (tokens[pos].type != TokenType::EOF_TOKEN)
			program.push_back(parse_statement());

		return program;
	}

	static string vartype_to_c(VarType type)
	{
		switch (type)
		{
		case VarType::INT:
			return "int";
		case VarType::FLOAT:
			return "float";
		case VarType::STRING:
			return "string";
		case VarType::BOOL:
			return "bool";
		case VarType::LIST:
			return "list";
		case VarType::TUPLE:
			return "tuple";
		case VarType::DICT:
			return "dict";
		default:
			return "void";
		}
	}

	const map<string, CollectionType>& get_variables() const
	{
		return variables;		//Exposes variables map
	}

private:
	CollectionType token_to_vartype(TokenType type)
	{
		if (type == TokenType::INT)
			return{ VarType::INT, VarType::NONE, VarType::NONE, VarType::NONE };

		if (type == TokenType::FLOAT)
			return{ VarType::FLOAT, VarType::NONE, VarType::NONE, VarType::NONE };

		if (type == TokenType::STRING)
			return{ VarType::STRING, VarType::NONE, VarType::NONE, VarType::NONE };

		if (type == TokenType::BOOL)
			return{ VarType::BOOL, VarType::NONE, VarType::NONE, VarType::NONE };

		throw runtime_error("Invalid Type at Line " + to_string(tokens[pos].line));
	}

	Token expect(TokenType type)
	{
		if (pos >= tokens.size() || tokens[pos].type != type)
			throw runtime_error("Unexpected Token Type at Line " + to_string(tokens[pos].line));

		return tokens[pos++];
	}

	CollectionType parse_collection_type()
	{
		CollectionType result;

		if (tokens[pos].type == TokenType::LIST)
		{
			expect(TokenType::LIST);
			expect(TokenType::LBRACKET);

			result.base_type = VarType::LIST;
			result.element_type = token_to_vartype(tokens[pos].type).base_type;

			expect(tokens[pos].type);
			expect(TokenType::RBRACKET);

			helper_includes.insert("list_" + vartype_to_c(result.element_type) + ".h");
		}
		else if (tokens[pos].type == TokenType::TUPLE)
		{
			expect(TokenType::TUPLE);
			expect(TokenType::LBRACKET);

			result.base_type = VarType::TUPLE;
			result.element_type = token_to_vartype(tokens[pos].type).base_type;

			expect(tokens[pos].type);
			expect(TokenType::RBRACKET);

			helper_includes.insert("tuple_" + vartype_to_c(result.element_type) + ".h");
		}
		else if (tokens[pos].type == TokenType::DICT)
		{
			expect(TokenType::DICT);
			expect(TokenType::LBRACKET);

			result.base_type = VarType::DICT;
			result.key_type = token_to_vartype(tokens[pos].type).base_type;

			if (result.key_type != VarType::STRING)
				throw runtime_error("Dictionary Keys Must be Strings at Line " + to_string(tokens[pos].line));

			expect(TokenType::STRING);
			expect(TokenType::COMMA);

			result.value_type = token_to_vartype(tokens[pos].type).base_type;

			expect(tokens[pos].type);
			expect(TokenType::RBRACKET);

			helper_includes.insert("dict_string_" + vartype_to_c(result.value_type) + ".h");
		}
		else
		{
			result = token_to_vartype(tokens[pos].type);

			if (result.base_type == VarType::STRING)
				helper_includes.insert("string_utils.h");
		}

		return result;
	}

	unique_ptr<ASTNode> parse_statement()
	{
		if (tokens[pos].type == TokenType::DEF)
			return parse_function();
		else if (tokens[pos].type == TokenType::RETURN)
			return parse_return();
		else if (tokens[pos].type == TokenType::PRINT)
			return parse_print();
		else if (tokens[pos].type == TokenType::IF)
			return parse_if();
		else if (tokens[pos].type == TokenType::FOR)
			return parse_for();
		else if (tokens[pos].type == TokenType::WHILE)
			return parse_while();
		else if (tokens[pos].type == TokenType::MATCH)
			return parse_match();
		else if (tokens[pos].type == TokenType::INT || tokens[pos].type == TokenType::FLOAT ||
			tokens[pos].type == TokenType::STRING || tokens[pos].type == TokenType::BOOL ||
			tokens[pos].type == TokenType::LIST || tokens[pos].type == TokenType::TUPLE ||
			tokens[pos].type == TokenType::DICT)
			return parse_assignment();
		else if (tokens[pos].type == TokenType::IDENTIFIER && tokens[pos + 1].type == TokenType::LPAREN)
			return parse_function_call();
		else if (tokens[pos].type == TokenType::IDENTIFIER && tokens[pos + 1].type == TokenType::DOT)
			return parse_method_call();
		else if (tokens[pos].type == TokenType::IDENTIFIER && tokens[pos + 1].type == TokenType::LBRACKET)
			return parse_index_assignment();
		else
			throw runtime_error("Unexpected Token at Line " + to_string(tokens[pos].line));
	}

	pair<string, VarType> parse_expression()
	{
		string result;
		VarType type = VarType::NONE;

		if (tokens[pos].type == TokenType::NUMBER)
		{
			result = expect(TokenType::NUMBER).value;
			type = VarType::INT;
			expr_type = { VarType::INT, VarType::NONE, VarType::NONE, VarType::NONE };
		}
		else if (tokens[pos].type == TokenType::FLOATING)
		{
			result = expect(TokenType::FLOATING).value;
			type = VarType::FLOAT;
			expr_type = { VarType::FLOAT, VarType::NONE, VarType::NONE, VarType::NONE };
		}
		else if (tokens[pos].type == TokenType::STRING_LITERAL)
		{
			result = "\"" + expect(TokenType::STRING_LITERAL).value + "\"";
			type = VarType::STRING;
			expr_type = { VarType::STRING, VarType::NONE, VarType::NONE, VarType::NONE };
			helper_includes.insert("string_utils.h");
		}
		else if (tokens[pos].type == TokenType::TRUE || tokens[pos].type == TokenType::FALSE)
		{
			result = expect(tokens[pos].type).value;
			type = VarType::BOOL;
			expr_type = { VarType::BOOL, VarType::NONE, VarType::NONE, VarType::NONE };
		}
		else if (tokens[pos].type == TokenType::IDENTIFIER && tokens[pos + 1].type == TokenType::LPAREN)
		{
			string func_name = expect(TokenType::IDENTIFIER).value;
			expect(TokenType::LPAREN);
			vector<string> args;

			if (tokens[pos].type != TokenType::RPAREN)
			{
				auto expr = parse_expression();
				args.push_back(expr.first);

				while (tokens[pos].type == TokenType::COMMA)
				{
					expect(TokenType::COMMA);

					expr = parse_expression();
					args.push_back(expr.first);
				}
			}

			expect(TokenType::RPAREN);

			if (functions.find(func_name) == functions.end())
				throw runtime_error("Undefined function " + func_name + " at line " + to_string(tokens[pos].line));

			result = func_name + "(";

			for (size_t i = 0; i < args.size(); ++i)
			{
				result += args[i];

				if (i < args.size() - 1)
					result += ", ";
			}

			result += ")";
			type = functions[func_name].second.base_type;
			expr_type = functions[func_name].second;
		}
		else if (tokens[pos].type == TokenType::IDENTIFIER && tokens[pos + 1].type == TokenType::LBRACKET)
		{
			string var = expect(TokenType::IDENTIFIER).value;
			expect(TokenType::LBRACKET);
			auto index = parse_expression();
			expect(TokenType::RBRACKET);

			if (variables.find(var) == variables.end())
				throw runtime_error("Undefined Variable " + var + " at Line " + to_string(tokens[pos].line));

			CollectionType var_type = variables[var];

			if (var_type.base_type == VarType::LIST || var_type.base_type == VarType::TUPLE)
			{
				result = var + "->data[" + index.first + "]";
				type = var_type.element_type;
				expr_type = { var_type.element_type, VarType::NONE, VarType::NONE, VarType::NONE };

				if (var_type.base_type == VarType::LIST)
					helper_includes.insert("list_" + vartype_to_c(var_type.element_type) + ".h");
				else
					helper_includes.insert("tuple_" + vartype_to_c(var_type.element_type) + ".h");
			}
			else if (var_type.base_type == VarType::DICT)
			{
				string temp_var = "temp_dict_get_" + to_string(rand());
				result = "dict_get_" + vartype_to_c(var_type.key_type) + vartype_to_c(var_type.value_type) + "(" + var + ", " + index.first + ")";
				type = var_type.value_type;
				expr_type = { var_type.value_type, VarType::NONE, VarType::NONE, VarType::NONE };
				helper_includes.insert("dict_string_" + vartype_to_c(var_type.value_type) + ".h");
			}
			else
				throw runtime_error("Indexing Only Supported for Lists, Tuples, and Dicts at Line " + to_string(tokens[pos].line));
		}
		else if (tokens[pos].type == TokenType::IDENTIFIER && tokens[pos + 1].type == TokenType::DOT)
		{
			string var = expect(TokenType::IDENTIFIER).value;
			expect(TokenType::DOT);
			string method = expect(TokenType::CALL_METHOD).value;
			expect(TokenType::LPAREN);
			vector<string> args;

			if (tokens[pos].type != TokenType::RPAREN)
			{
				auto expr = parse_expression();
				args.push_back(expr.first);

				while (tokens[pos].type == TokenType::COMMA)
				{
					expect(TokenType::COMMA);
					expr = parse_expression();
					args.push_back(expr.first);
				}
			}

			expect(TokenType::RPAREN);

			if (variables.find(var) == variables.end())
				throw runtime_error("Undefined Variable " + var + " at Line " + to_string(tokens[pos].line));

			CollectionType var_type = variables[var];

			if (var_type.base_type != VarType::STRING && var_type.base_type != VarType::LIST)
				throw runtime_error("Method Call Only Supported for Strings and Lists at Line " + to_string(tokens[pos].line));

			CollectionType return_type;

			if (method == "append")
			{
				if (var_type.base_type != VarType::LIST)
					throw runtime_error("Append Method Only Supported for Lists at Line " + to_string(tokens[pos].line));

				helper_includes.insert("list_" + vartype_to_c(var_type.element_type) + ".h");
				return_type = { VarType::NONE, VarType::NONE, VarType::NONE, VarType::NONE };
			}
			else if (method == "upper" || method == "lower" || method == "strip" ||
				method == "replace" || method == "split" || method == "find")
			{
				if (var_type.base_type != VarType::STRING)
					throw runtime_error("String Method Only Supported for Strings at Line " + to_string(tokens[pos].line));

				helper_includes.insert("string_utils.h");

				if (method == "split")
					return_type = { VarType::LIST, VarType::STRING, VarType::NONE, VarType::NONE };
				else if (method == "find")
					return_type = { VarType::INT, VarType::NONE, VarType::NONE, VarType::NONE };
				else
					return_type = { VarType::STRING, VarType::NONE, VarType::NONE, VarType::NONE };
			}
			else
				throw runtime_error("Unsupported Method " + method + " at Line " + to_string(tokens[pos].line));

			string temp_var = "temp_method_" + to_string(rand());

			if (method == "append")
			{
				result = "list_append_" + vartype_to_c(var_type.element_type) + "(" + var + ", " + args[0] + ")";
				type = VarType::NONE;
			}
			else if (method == "upper" || method == "lower" || method == "strip")
			{
				result = "char* " + temp_var + " = str_" + method + "(" + var + ")";
				type = VarType::STRING;
			}
			else if (method == "replace")
			{
				result = "char* " + temp_var + " = str_replace(" + var + ", " + args[0] + ", " + args[1] + ")";
				type = VarType::STRING;
			}
			else if (method == "split")
			{
				result = "ListString* " + temp_var + " = str_split(" + var +
					(args.empty() ? ", NULL" : ", " + args[0]) + ")";
				type = VarType::LIST;
				expr_type = { VarType::LIST, VarType::STRING, VarType::NONE, VarType::NONE };
			}
			else if (method == "find")
			{
				result = "int " + temp_var + " = str_find(" + var + ", " + args[0] + ")";
				type = VarType::INT;
			}
		}
		else if (tokens[pos].type == TokenType::IDENTIFIER)
		{
			string var = expect(TokenType::IDENTIFIER).value;

			if (variables.find(var) == variables.end())
				throw runtime_error("Undefined Variable " + var + " at Line " + to_string(tokens[pos].line));

			result = var;
			type = variables[var].base_type;
			expr_type = variables[var];

			if (type == VarType::LIST)
				helper_includes.insert("list_" + vartype_to_c(expr_type.element_type) + ".h");
			else if (type == VarType::TUPLE)
				helper_includes.insert("tuple_" + vartype_to_c(expr_type.element_type) + ".h");
			else if (type == VarType::DICT)
				helper_includes.insert("dict_string_" + vartype_to_c(expr_type.value_type) + ".h");
			else if (type == VarType::STRING)
				helper_includes.insert("string_utils.h");
		}
		else if (tokens[pos].type == TokenType::FSTRING_START)
		{
			expect(TokenType::FSTRING_START);

			string format;
			vector<string> args;
			vector<VarType> arg_types;

			while (tokens[pos].type != TokenType::FSTRING_END)
			{
				if (tokens[pos].type == TokenType::STRING_LITERAL)
				{
					string str = expect(TokenType::STRING_LITERAL).value;
					format += str;
					helper_includes.insert("string_utils.h");
				}
				else if (tokens[pos].type == TokenType::FSTRING_EXPR_START)
				{
					expect(TokenType::FSTRING_EXPR_START);

					auto expr = parse_expression();
					args.push_back(expr.first);
					arg_types.push_back(expr.second);
					FormatSpec spec;

					if (tokens[pos].type == TokenType::FSTRING_FORMAT_SPEC)
					{
						string format_spec = expect(TokenType::FSTRING_FORMAT_SPEC).value;
						size_t i = 0;

						if (format_spec[i] == '<' || format_spec[i] == '>' || format_spec[i] == '^')
						{
							spec.alignment = format_spec[i];
							i++;
						}

						while (i < format_spec.size() && isdigit(format_spec[i]))
						{
							spec.width += format_spec[i];
							i++;
						}

						if (i < format_spec.size() && format_spec[i] == '.')
						{
							i++;

							while (i < format_spec.size() && isdigit(format_spec[i]))
							{
								spec.precision += format_spec[i];
								i++;
							}
						}

						if (i < format_spec.size())
							spec.type = format_spec[i];

						string format_str;

						if (!spec.alignment.empty())
							format_str += spec.alignment;

						if (!spec.width.empty())
							format_str += spec.width;

						if (!spec.precision.empty())
							format_str += "." + spec.precision;

						format_str += spec.type;
						format += "%" + format_str;
					}
					else
					{
						if (expr.second == VarType::INT)
							format += "%d";
						else if (expr.second == VarType::FLOAT)
							format += "%f";
						else if (expr.second == VarType::STRING)
							format += "%s";
						else if (expr.second == VarType::BOOL)
							format += "%s";
						else if (expr.second == VarType::LIST || expr.second == VarType::TUPLE || expr.second == VarType::DICT)
						{
							format += "%s";

							if (expr.second == VarType::LIST)
								helper_includes.insert("list_" + vartype_to_c(expr_type.element_type) + ".h");
							else if (expr.second == VarType::TUPLE)
								helper_includes.insert("tuple_" + vartype_to_c(expr_type.element_type) + ".h");
							else if (expr.second == VarType::DICT)
								helper_includes.insert("dict_string_" + vartype_to_c(expr_type.value_type) + ".h");
						}
					}

					expect(TokenType::FSTRING_EXPR_END);
				}
			}

			expect(TokenType::FSTRING_END);

			string temp_var = "temp_string_" + to_string(string_temp_counter++);
			result = "char " + temp_var + "[1024];\n";
			result += "snprintf(" + temp_var + ", 1024, \"" + format + "\"";

			for (const auto& arg : args)
			{
				result += ", ";

				if (arg_types[&arg - &args[0]] == VarType::BOOL)
					result += arg + " ? \"true\" : \"false\"";
				else if (arg_types[&arg - &args[0]] == VarType::LIST)
				{
					result += "list_to_string_" + vartype_to_c(expr_type.element_type) + "(" + arg + ")";
					helper_includes.insert("list_" + vartype_to_c(expr_type.element_type) + ".h");
				}
				else if (arg_types[&arg - &args[0]] == VarType::TUPLE)
				{
					result += "tuple_to_string_" + vartype_to_c(expr_type.element_type) + "(" + arg + ")";
					helper_includes.insert("tuple_" + vartype_to_c(expr_type.element_type) + ".h");
				}
				else if (arg_types[&arg - &args[0]] == VarType::DICT)
				{
					result += "dict_to_string_" + vartype_to_c(expr_type.key_type) + vartype_to_c(expr_type.value_type) + "(" + arg + ")";
					helper_includes.insert("dict_string_" + vartype_to_c(expr_type.value_type) + ".h");
				}
				else
					result += arg;
			}

			result += ");\n    " + temp_var;
			type = VarType::STRING;
			expr_type = { VarType::STRING, VarType::NONE, VarType::NONE, VarType::NONE };
			helper_includes.insert("string_utils.h");
		}
		else if (tokens[pos].type == TokenType::LBRACKET)
		{
			expect(TokenType::LBRACKET);
			vector<string> elements;
			CollectionType list_type;

			if (tokens[pos].type != TokenType::RBRACKET)
			{
				auto expr = parse_expression();
				elements.push_back(expr.first);
				list_type.element_type = expr.second;

				while (tokens[pos].type == TokenType::COMMA)
				{
					expect(TokenType::COMMA);
					expr = parse_expression();

					if (expr.second != list_type.element_type)
						throw runtime_error("Inconsistent List Element Types at Line " + to_string(tokens[pos].line));

					elements.push_back(expr.first);
				}
			}

			expect(TokenType::RBRACKET);
			list_type.base_type = VarType::LIST;
			helper_includes.insert("list_" + vartype_to_c(list_type.element_type) + ".h");

			//return make_unique<ListNode>(elements, list_type);

			string temp_var = "temp_list_" + to_string(string_temp_counter++);
			string result = vartype_to_c(list_type.element_type) + "List*" + temp_var + " = create_list_" + vartype_to_c(list_type.element_type) + "();\n";

			for (const auto& elem : elements)
				result += "list_append_" + vartype_to_c(list_type.element_type) + "(" + temp_var + ", " + elem + ");\n";

			result += temp_var;
			type = VarType::LIST;
			expr_type = list_type;

			return{ result, type };
		}
		else if (tokens[pos].type == TokenType::LPAREN)
		{
			expect(TokenType::LPAREN);
			vector<string> elements;
			CollectionType tuple_type;

			if (tokens[pos].type != TokenType::RPAREN)
			{
				auto expr = parse_expression();
				elements.push_back(expr.first);
				tuple_type.element_type = expr.second;

				while (tokens[pos].type == TokenType::COMMA)
				{
					expect(TokenType::COMMA);
					expr = parse_expression();

					if (expr.second != tuple_type.element_type)
						throw runtime_error("Inconsistent tuple element types at line " + to_string(tokens[pos].line));

					elements.push_back(expr.first);
				}
			}

			expect(TokenType::RPAREN);
			tuple_type.base_type = VarType::TUPLE;
			helper_includes.insert("tuple_" + vartype_to_c(tuple_type.element_type) + ".h");

			//return make_unique<TupleNode>(elements, tuple_type);

			string temp_var = "temp_tuple_" + to_string(string_temp_counter++);
			string result = vartype_to_c(tuple_type.element_type) + "Tuple* " + temp_var + " = create_tuple_" + vartype_to_c(tuple_type.element_type) + "(" + to_string(elements.size()) + ");\n";

			for (size_t i = 0; i < elements.size(); ++i)
				result += temp_var + "->data[" + to_string(i) + "] = " + elements[i] + ";\n";

			result += temp_var;
			type = VarType::TUPLE;
			expr_type = tuple_type;

			return{ result, type };
		}
		else if (tokens[pos].type == TokenType::LBRACE)
		{
			expect(TokenType::LBRACE);
			vector<pair<string, string>> entries;
			CollectionType dict_type;

			if (tokens[pos].type != TokenType::RBRACE)
			{
				auto key = parse_expression();

				if (key.second != VarType::STRING)
					throw runtime_error("Dictionary Key Must be a String at Line " + to_string(tokens[pos].line));

				expect(TokenType::COLON);
				auto value = parse_expression();
				entries.emplace_back(key.first, value.first);
				dict_type.key_type = VarType::STRING;
				dict_type.value_type = value.second;

				while (tokens[pos].type == TokenType::COMMA)
				{
					expect(TokenType::COMMA);
					key = parse_expression();

					if (key.second != VarType::STRING)
						throw runtime_error("Dictionary Key Must be a String at Line " + to_string(tokens[pos].line));

					expect(TokenType::COLON);
					value = parse_expression();

					if (value.second != dict_type.value_type)
						throw runtime_error("Inconsistent Dictionary Value Types at Line " + to_string(tokens[pos].line));

					entries.emplace_back(key.first, value.first);
				}
			}

			expect(TokenType::RBRACE);
			dict_type.base_type = VarType::DICT;
			helper_includes.insert("dict_string_" + vartype_to_c(dict_type.value_type) + ".h");

			//return make_unique<DictNode>(entries, dict_type);

			string temp_var = "temp_dict_" + to_string(string_temp_counter++);
			string result = "DictString" + vartype_to_c(dict_type.value_type) + "* " + temp_var + " = create_dict_string_" + vartype_to_c(dict_type.value_type) + "();\n";

			for (const auto& entry : entries)
				result += "dict_set_string_" + vartype_to_c(dict_type.value_type) + "(" + temp_var + ", " + entry.first + ", " + entry.second + ");\n";

			result += temp_var;
			type = VarType::DICT;
			expr_type = dict_type;

			return{ result, type };
		}
		else if (tokens[pos].type == TokenType::LEN)
		{
			expect(TokenType::LEN);
			expect(TokenType::LPAREN);
			auto expr = parse_expression();
			expect(TokenType::RPAREN);

			if (expr.second != VarType::STRING && expr.second != VarType::LIST &&
				expr.second != VarType::TUPLE && expr.second != VarType::DICT)
				throw runtime_error("LEN Function Only Supported for Strings, Lists, Tuples, and Dicts at Line " + to_string(tokens[pos].line));

			if (expr.second == VarType::STRING)
			{
				helper_includes.insert("string_utils.h");
				result = "strlen(" + expr.first + ")";
			}
			else if (expr.second == VarType::LIST)
			{
				helper_includes.insert("list_" + vartype_to_c(expr_type.element_type) + ".h");
				result = expr.first + "->size";
			}
			else if (expr.second == VarType::TUPLE)
			{
				helper_includes.insert("tuple_" + vartype_to_c(expr_type.element_type) + ".h");
				result = expr.first + "->size";
			}
			else if (expr.second == VarType::DICT)
			{
				helper_includes.insert("dict_string_" + vartype_to_c(expr_type.value_type) + ".h");
				result = expr.first + "->size";
			}

			type = VarType::INT;
			expr_type = { VarType::INT, VarType::NONE, VarType::NONE, VarType::NONE };

			return{ result, type };

			//return make_unique<LenNode>(expr.first, expr_type);
		}
		else
			throw runtime_error("Invalid Expression at Line " + to_string(tokens[pos].line));

		while (tokens[pos].type == TokenType::PLUS || tokens[pos].type == TokenType::MINUS ||
			tokens[pos].type == TokenType::MULT || tokens[pos].type == TokenType::DIV ||
			tokens[pos].type == TokenType::EQ || tokens[pos].type == TokenType::NOTEQ ||
			tokens[pos].type == TokenType::LESSER || tokens[pos].type == TokenType::GREATER ||
			tokens[pos].type == TokenType::LESSEREQ || tokens[pos].type == TokenType::GREATEREQ ||
			tokens[pos].type == TokenType::AND || tokens[pos].type == TokenType::OR)
		{
			string op;
			VarType result_type = type;

			if (tokens[pos].type == TokenType::PLUS)
			{
				op = "+";
				expect(TokenType::PLUS);

				if (type == VarType::STRING || type == VarType::LIST)
				{
					result_type = type;

					if (type == VarType::STRING)
						helper_includes.insert("string_utils.h");
					else
						helper_includes.insert("list_" + vartype_to_c(expr_type.element_type) + ".h");
				}
				else if (type == VarType::INT || type == VarType::FLOAT)
					result_type = type;
				else
					throw runtime_error("Invalid Operand Types for '+' at Line " + to_string(tokens[pos].line));
			}
			else if (tokens[pos].type == TokenType::MINUS)
			{
				op = "-";
				expect(TokenType::MINUS);

				if (type != VarType::INT && type != VarType::FLOAT)
					throw std::runtime_error("Invalid Operand Types for '-' at Line " + to_string(tokens[pos].line));

				result_type = type;
			}
			else if (tokens[pos].type == TokenType::MULT)
			{
				op = "*";
				expect(TokenType::MULT);

				if (type != VarType::INT && type != VarType::FLOAT)
					throw runtime_error("Invalid Operand Types for '*' at Line " + to_string(tokens[pos].line));

				result_type = type;
			}
			else if (tokens[pos].type == TokenType::DIV)
			{
				op = "/";
				expect(TokenType::DIV);

				if (type != VarType::INT && type != VarType::FLOAT)
					throw runtime_error("Invalid Operand Types for '/' at Line " + to_string(tokens[pos].line));

				result_type = VarType::FLOAT;
			}
			else if (tokens[pos].type == TokenType::EQ)
			{
				op = "==";
				expect(TokenType::EQ);

				result_type = VarType::BOOL;
			}
			else if (tokens[pos].type == TokenType::NOTEQ)
			{
				op = "!=";
				expect(TokenType::NOTEQ);

				result_type = VarType::BOOL;
			}
			else if (tokens[pos].type == TokenType::GREATER)
			{
				op = ">";
				expect(TokenType::GREATER);

				result_type = VarType::BOOL;
			}
			else if (tokens[pos].type == TokenType::LESSER)
			{
				op = "<";
				expect(TokenType::LESSER);

				result_type = VarType::BOOL;
			}
			else if (tokens[pos].type == TokenType::GREATEREQ)
			{
				op = ">=";
				expect(TokenType::GREATEREQ);

				result_type = VarType::BOOL;
			}
			else if (tokens[pos].type == TokenType::LESSEREQ)
			{
				op = ">=";
				expect(TokenType::LESSEREQ);

				result_type = VarType::BOOL;
			}
			else if (tokens[pos].type == TokenType::AND)
			{
				op = "&&";
				expect(TokenType::AND);

				if (type != VarType::BOOL)
					throw runtime_error("Invalid Operand Types for 'AND' at Line " + to_string(tokens[pos].line));

				result_type = VarType::BOOL;
			}
			else if (tokens[pos].type == TokenType::OR)
			{
				op = "||";
				expect(TokenType::OR);

				if (type != VarType::BOOL)
					throw runtime_error("Invalid Operand Types for 'OR' at Line " + to_string(tokens[pos].line));

				result_type = VarType::BOOL;
			}

			auto right = parse_expression();

			if (type != VarType::BOOL && right.second != VarType::BOOL &&
				type != right.second && !(type == VarType::FLOAT && right.second == VarType::INT))
				throw runtime_error("Type Mismatch in Operation at Line " + to_string(tokens[pos].line));

			/*
			if (result.first.find("snprintf") != string::npos)
			{
				string temp_var = result.substr(result.first.find_last_of(' ') + 1);

				result = temp_var;
			}
			*/

			if (right.first.find("snprintf") != string::npos)
			{
				string temp_var = right.first.substr(right.first.find_last_of(' ') + 1);
				right.first = temp_var;
			}

			result = "(" + result + " " + op + " " + right.first + ")";
			type = result_type;
			expr_type.base_type = result_type;
		}

		return{ result, type };
	}

	unique_ptr<ASTNode> parse_assignment()
	{
		CollectionType type = parse_collection_type();
		string var = expect(TokenType::IDENTIFIER).value;
		expect(TokenType::EQUALS);
		auto expr = parse_expression();

		if (type.base_type == VarType::INT && expr.second != VarType::INT)
			throw runtime_error("Type Mismatch in Assignment at Line " + to_string(tokens[pos].line));

		if (type.base_type == VarType::FLOAT && expr.second != VarType::FLOAT && expr.second != VarType::INT)
			throw runtime_error("Type Mismatch in Assignment at Line " + to_string(tokens[pos].line));

		if (type.base_type == VarType::STRING && expr.second != VarType::STRING)
			throw runtime_error("Type Mismatch in Assignment at Line " + to_string(tokens[pos].line));

		if (type.base_type == VarType::BOOL && expr.second != VarType::BOOL)
			throw runtime_error("Type Mismatch in Assignment at Line " + to_string(tokens[pos].line));

		if (type.base_type == VarType::LIST && (expr.second != VarType::LIST || type.element_type != expr_type.element_type))
			throw runtime_error("Type Mismatch in List Assignment at Line " + to_string(tokens[pos].line));

		if (type.base_type == VarType::TUPLE && (expr.second != VarType::TUPLE || type.element_type != expr_type.element_type))
			throw runtime_error("Type Mismatch in Tuple Assignment at Line " + to_string(tokens[pos].line));

		if (type.base_type == VarType::DICT && (expr.second != VarType::DICT ||
			type.key_type != expr_type.key_type || type.value_type != expr_type.value_type))
			throw runtime_error("Type Mismatch in Dict Assignment at Line " + to_string(tokens[pos].line));

		bool is_declaration = variables.find(var) == variables.end();
		variables[var] = type;

		expect(TokenType::NEWLINE);

		return make_unique<AssignNode>(var, expr.first, type, is_declaration);
	}

	unique_ptr<ASTNode> parse_function()
	{
		expect(TokenType::DEF);

		string name = expect(TokenType::IDENTIFIER).value;

		expect(TokenType::LPAREN);

		vector<pair<string, CollectionType>> args;
		vector<CollectionType> arg_types;

		if (tokens[pos].type != TokenType::RPAREN)
		{
			CollectionType type = parse_collection_type();
			string arg_name = expect(TokenType::IDENTIFIER).value;

			args.emplace_back(arg_name, type);
			arg_types.push_back(type);
			variables[arg_name] = type;

			while (tokens[pos].type == TokenType::COMMA)
			{
				expect(TokenType::COMMA);

				type = parse_collection_type();
				arg_name = expect(TokenType::IDENTIFIER).value;

				args.emplace_back(arg_name, type);
				arg_types.push_back(type);
				variables[arg_name] = type;
			}
		}

		expect(TokenType::RPAREN);

		CollectionType return_type = { VarType::NONE, VarType::NONE, VarType::NONE, VarType::NONE };

		if (tokens[pos].type == TokenType::COLON && tokens[pos + 1].type != TokenType::NEWLINE)
		{
			expect(TokenType::COLON);

			return_type = parse_collection_type();
		}

		expect(TokenType::COLON);
		expect(TokenType::NEWLINE);
		expect(TokenType::INDENT);

		functions[name] = { arg_types, return_type };

		auto func = make_unique<FunctionNode>(name, args, return_type);

		while (tokens[pos].type != TokenType::DEDENT && tokens[pos].type != TokenType::EOF_TOKEN)
			func->body.push_back(parse_statement());

		expect(TokenType::DEDENT);

		return func;
	}

	unique_ptr<ASTNode> parse_function_call()
	{
		string func_name = expect(TokenType::IDENTIFIER).value;

		expect(TokenType::LPAREN);

		vector<string> args;

		if (tokens[pos].type != TokenType::RPAREN)
		{
			auto expr = parse_expression();
			args.push_back(expr.first);

			while (tokens[pos].type == TokenType::COMMA)
			{
				expect(TokenType::COMMA);

				expr = parse_expression();
				args.push_back(expr.first);
			}
		}

		expect(TokenType::RPAREN);
		expect(TokenType::NEWLINE);

		if (functions.find(func_name) == functions.end())
			throw runtime_error("Undefined Function " + func_name + " at Line " + to_string(tokens[pos].line));

		return make_unique<CallNode>(func_name, args, functions[func_name].second);
	}

	unique_ptr<ASTNode> parse_method_call()
	{
		string var = expect(TokenType::IDENTIFIER).value;
		expect(TokenType::DOT);
		string method = expect(TokenType::CALL_METHOD).value;
		expect(TokenType::LPAREN);
		vector<string> args;

		if (tokens[pos].type != TokenType::RPAREN)
		{
			auto expr = parse_expression();
			args.push_back(expr.first);

			while (tokens[pos].type == TokenType::COMMA)
			{
				expect(TokenType::COMMA);
				expr = parse_expression();
				args.push_back(expr.first);
			}
		}

		expect(TokenType::RPAREN);
		expect(TokenType::NEWLINE);

		if (variables.find(var) == variables.end())
			throw runtime_error("Undefined Variable " + var + " at Line " + to_string(tokens[pos].line));

		CollectionType var_type = variables[var];

		if (var_type.base_type != VarType::STRING && var_type.base_type != VarType::LIST)
			throw runtime_error("Method Call Only Supported for Strings and Lists at Line " + to_string(tokens[pos].line));

		CollectionType return_type;

		if (method == "append")
		{
			if (var_type.base_type != VarType::LIST)
				throw runtime_error("'Append' Method Only Supported for Lists at Line " + to_string(tokens[pos].line));

			helper_includes.insert("list_" + vartype_to_c(var_type.element_type) + ".h");
			return_type = { VarType::NONE, VarType::NONE, VarType::NONE, VarType::NONE };
		}
		else if (method == "upper" || method == "lower" || method == "strip" || method == "replace" ||
			method == "split" || method == "find")
		{
			if (var_type.base_type != VarType::STRING)
				throw runtime_error("'String' Methods Only Supported for Strings at Line " + to_string(tokens[pos].line));

			helper_includes.insert("string_utils.h");

			if (method == "split")
				return_type = { VarType::LIST, VarType::STRING, VarType::NONE, VarType::NONE };
			else if (method == "find")
				return_type = { VarType::INT, VarType::NONE, VarType::NONE, VarType::NONE };
			else
				return_type = { VarType::STRING, VarType::NONE, VarType::NONE, VarType::NONE };
		}
		else
			throw runtime_error("Unsupported Method " + method + " at Line " + to_string(tokens[pos].line));

		return make_unique<MethodCallNode>(var, method, args, return_type);
	}

	unique_ptr<ASTNode> parse_return()
	{
		expect(TokenType::RETURN);
		auto expr = parse_expression();
		expect(TokenType::NEWLINE);

		return make_unique<ReturnNode>(expr.first, expr_type);
	}

	unique_ptr<ASTNode> parse_print()
	{
		expect(TokenType::PRINT);
		expect(TokenType::LPAREN);

		vector<pair<string, VarType>> values;
		string separator = " ";

		if (tokens[pos].type != TokenType::RPAREN)
		{
			auto expr = parse_expression();

			if ((expr.second == VarType::STRING || expr.second == VarType::LIST ||
				expr.second == VarType::TUPLE || expr.second == VarType::DICT) &&
				expr.first.find("snprintf") != string::npos)
			{
				string temp_var = expr.first.substr(expr.first.find_last_of(' ') + 1);
				values.emplace_back(temp_var, expr.second);
			}
			else
				values.emplace_back(expr.first, expr.second);

			// Register includes for collection types in print
			if (expr.second == VarType::LIST)
				helper_includes.insert("list_" + vartype_to_c(expr_type.element_type) + ".h");
			else if (expr.second == VarType::TUPLE)
				helper_includes.insert("tuple_" + vartype_to_c(expr_type.element_type) + ".h");
			else if (expr.second == VarType::DICT)
				helper_includes.insert("dict_string_" + vartype_to_c(expr_type.value_type) + ".h");
			else if (expr.second == VarType::STRING)
				helper_includes.insert("string_utils.h");

			while (tokens[pos].type == TokenType::COMMA)
			{
				expect(TokenType::COMMA);

				if (tokens[pos].type == TokenType::SEP)
				{
					expect(TokenType::SEP);
					expect(TokenType::EQUALS);

					if (tokens[pos].type != TokenType::STRING_LITERAL)
						throw runtime_error("Separator Must be a String at Line " + to_string(tokens[pos].line));

					separator = expect(TokenType::STRING_LITERAL).value;
					helper_includes.insert("string_utils.h");

					break;
				}

				expr = parse_expression();

				if ((expr.second == VarType::STRING || expr.second == VarType::LIST ||
					expr.second == VarType::TUPLE || expr.second == VarType::DICT) &&
					expr.first.find("snprintf") != string::npos)
				{
					string temp_var = expr.first.substr(expr.first.find_last_of(' ') + 1);
					values.emplace_back(temp_var, expr.second);
				}
				else
					values.emplace_back(expr.first, expr.second);

				if (expr.second == VarType::LIST)
					helper_includes.insert("list_" + vartype_to_c(expr_type.element_type) + ".h");
				else if (expr.second == VarType::TUPLE)
					helper_includes.insert("tuple_" + vartype_to_c(expr_type.element_type) + ".h");
				else if (expr.second == VarType::DICT)
					helper_includes.insert("dict_string_" + vartype_to_c(expr_type.value_type) + ".h");
				else if (expr.second == VarType::STRING)
					helper_includes.insert("string_utils.h");
			}
		}

		expect(TokenType::RPAREN);
		expect(TokenType::NEWLINE);

		return make_unique<PrintNode>(values, separator);
	}

	unique_ptr<ASTNode> parse_if()
	{
		expect(TokenType::IF);

		auto condition = parse_expression();

		expect(TokenType::COLON);
		expect(TokenType::NEWLINE);
		expect(TokenType::INDENT);

		auto if_node = make_unique<IfNode>(condition.first);

		while (tokens[pos].type != TokenType::DEDENT && tokens[pos].type != TokenType::ELIF &&
			tokens[pos].type != TokenType::ELSE && tokens[pos].type != TokenType::EOF_TOKEN)
			if_node->body.push_back(parse_statement());

		expect(TokenType::DEDENT);

		while (tokens[pos].type == TokenType::ELIF)
		{
			expect(TokenType::ELIF);

			auto elif_condition = parse_expression();

			expect(TokenType::COLON);
			expect(TokenType::NEWLINE);
			expect(TokenType::INDENT);

			vector<unique_ptr<ASTNode>> elif_body;

			while (tokens[pos].type != TokenType::DEDENT && tokens[pos].type != TokenType::ELIF &&
				tokens[pos].type != TokenType::ELSE && tokens[pos].type != TokenType::EOF_TOKEN)
				elif_body.push_back(parse_statement());

			expect(TokenType::DEDENT);

			if_node->elif_clauses.emplace_back(elif_condition.first, move(elif_body));
		}

		if (tokens[pos].type == TokenType::ELSE)
		{
			expect(TokenType::ELSE);
			expect(TokenType::COLON);
			expect(TokenType::NEWLINE);
			expect(TokenType::INDENT);

			while (tokens[pos].type != TokenType::DEDENT && tokens[pos].type != TokenType::EOF_TOKEN)
				if_node->else_body.push_back(parse_statement());

			expect(TokenType::DEDENT);
		}

		return if_node;
	}

	unique_ptr<ASTNode> parse_for()
	{
		expect(TokenType::FOR);
		string var = expect(TokenType::IDENTIFIER).value;
		expect(TokenType::IN);
		expect(TokenType::RANGE);
		expect(TokenType::LPAREN);

		auto start = parse_expression();

		expect(TokenType::COMMA);

		auto end = parse_expression();

		expect(TokenType::RPAREN);
		expect(TokenType::COLON);
		expect(TokenType::NEWLINE);
		expect(TokenType::INDENT);

		auto for_node = make_unique<ForNode>(var, start.first, end.first);
		variables[var] = { VarType::INT, VarType::NONE, VarType::NONE, VarType::NONE };

		while (tokens[pos].type != TokenType::DEDENT && tokens[pos].type != TokenType::EOF_TOKEN)
			for_node->body.push_back(parse_statement());

		expect(TokenType::DEDENT);

		return for_node;
	}

	unique_ptr<ASTNode> parse_while()
	{
		expect(TokenType::WHILE);

		auto condition = parse_expression();

		expect(TokenType::COLON);
		expect(TokenType::NEWLINE);
		expect(TokenType::INDENT);

		auto while_node = make_unique<WhileNode>(condition.first);

		while (tokens[pos].type != TokenType::DEDENT && tokens[pos].type != TokenType::EOF_TOKEN)
			while_node->body.push_back(parse_statement());

		expect(TokenType::DEDENT);

		return while_node;
	}

	unique_ptr<ASTNode> parse_match()
	{
		expect(TokenType::MATCH);
		auto expr = parse_expression();

		if (expr.second != VarType::INT && expr.second != VarType::BOOL)
			throw runtime_error("Match expression must be int or bool at line " + to_string(tokens[pos].line));

		expect(TokenType::COLON);
		expect(TokenType::NEWLINE);
		expect(TokenType::INDENT);

		auto match_node = make_unique<MatchNode>(expr.first, expr.second);

		while (tokens[pos].type == TokenType::CASE)
		{
			expect(TokenType::CASE);
			string pattern = "_";

			if (tokens[pos].type == TokenType::NUMBER)
				pattern = expect(TokenType::NUMBER).value;
			else if (tokens[pos].type == TokenType::TRUE || tokens[pos].type == TokenType::FALSE)
				pattern = expect(tokens[pos].type).value;
			else if (tokens[pos].type == TokenType::IDENTIFIER && tokens[pos].value == "_")
				expect(TokenType::IDENTIFIER);

			expect(TokenType::COLON);
			expect(TokenType::NEWLINE);
			expect(TokenType::INDENT);

			vector<unique_ptr<ASTNode>> case_body;

			while (tokens[pos].type != TokenType::DEDENT && tokens[pos].type != TokenType::EOF_TOKEN)
				case_body.push_back(parse_statement());

			expect(TokenType::DEDENT);

			if (pattern == "_")
				match_node->default_case = move(case_body);
			else
				match_node->cases.emplace_back(pattern, move(case_body));
		}

		expect(TokenType::DEDENT);

		return match_node;
	}

	unique_ptr<ASTNode> parse_index_assignment()
	{
		string var = expect(TokenType::IDENTIFIER).value;
		expect(TokenType::LBRACKET);

		auto index = parse_expression();

		expect(TokenType::RBRACKET);
		expect(TokenType::EQUALS);

		auto value = parse_expression();

		expect(TokenType::NEWLINE);

		if (variables.find(var) == variables.end())
			throw runtime_error("Undefined Variable " + var + " at Line " + to_string(tokens[pos].line));

		CollectionType var_type = variables[var];

		if (var_type.base_type != VarType::LIST && var_type.base_type != VarType::DICT)
			throw runtime_error("Indexing Only Supported for Lists and Dicts at Line " + to_string(tokens[pos].line));

		if (var_type.base_type == VarType::LIST && index.second != VarType::INT)
			throw runtime_error("List Index Must be an Integer at Line " + to_string(tokens[pos].line));

		if (var_type.base_type == VarType::DICT && index.second != VarType::STRING)
			throw runtime_error("Dict Index Must be a String at Line " + to_string(tokens[pos].line));

		if (var_type.base_type == VarType::LIST && var_type.element_type != value.second)
			throw runtime_error("Type Mismatch in List Assignment at Line " + to_string(tokens[pos].line));

		if (var_type.base_type == VarType::DICT && var_type.value_type != value.second)
			throw runtime_error("Type Mismatch in Dict Assignment at Line " + to_string(tokens[pos].line));

		// Register includes for index assignment
		if (var_type.base_type == VarType::LIST)
			helper_includes.insert("list_" + vartype_to_c(var_type.element_type) + ".h");
		else
			helper_includes.insert("dict_string_" + vartype_to_c(var_type.value_type) + ".h");

		string code;

		if (var_type.base_type == VarType::LIST)
			code = var + "->data[" + index.first + "] = " + value.first + ";\n";
		else
			code = "dict_set_" + vartype_to_c(var_type.key_type) + vartype_to_c(var_type.value_type) + "(" + var + ", " + index.first + ", " + value.first + ");\n";

		return make_unique<HelperNode>(code);
	}
};