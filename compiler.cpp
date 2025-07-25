#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <set>
#include <memory>
#include <map>

using namespace std;

//Token Types
enum class TokenType
{
	DEF, RETURN, PRINT,
	INT, FLOAT, STRING,
	AND, OR,
	IF, FOR, WHILE,
	IN, RANGE,
	IDENTIFIER, NUMBER, FLOATING, STRING_LITERAL,
	COLON, COMMA,
	EQUALS, EQ, GREATER, LESSER, GREATEREQ, LESSEREQ,
	PLUS, MINUS, MULT, DIV,
	LPAREN, RPAREN,
	INDENT, DEDENT,
	NEWLINE, EOF_TOKEN
};

//Variable Type
enum class VarType
{
	INT, FLOAT, STRING, NONE
};

//Token Structure
struct Token
{
	TokenType type;
	string value;
	int line;
};

//Lexer Class
class Lexer
{
private:
	string source;
	size_t pos;
	int line;
	int indent_level;
	vector<int> indent_stack;

public:
	Lexer(const string& src) : source(src), pos(0), line(1), indent_level(0)
	{
		indent_stack.push_back(0);
	}

	vector<Token> tokenize()
	{
		vector<Token> tokens;

		while (pos < source.size())
		{
			char current = source[pos];

			if (current == '\n')
			{
				tokens.push_back({ TokenType::NEWLINE, "", line });
				line++;
				pos++;
				handle_indent(tokens);
			}
			else if (isspace(current) && current != '\n')
			{
				pos++;
				continue;
			}
			else if (isalpha(current))
			{
				tokens.push_back(read_identifier_or_keyword());
			}
			else if (isdigit(current) || current == '.')
			{
				tokens.push_back(read_number_or_float());
			}
			else if (current == '"')
			{
				tokens.push_back(read_string());
			}
			else if (current == ':')
			{
				tokens.push_back({ TokenType::COLON, ":", line });
				pos++;
			}
			else if (current == '=')
			{
				if (pos + 1 < source.size() && source[pos + 1] == '=')
				{
					tokens.push_back({ TokenType::EQ, "==", line });
					pos += 2;
				}
				else
				{
					tokens.push_back({ TokenType::EQUALS, "=", line });
					pos++;
				}
			}
			else if (current == '>')
			{
				tokens.push_back({ TokenType::GREATER, ">", line });
				pos++;
			}
			else if (current == '<')
			{
				tokens.push_back({ TokenType::LESSER, "<", line });
				pos++;
			}
			else if (current == '+')
			{
				tokens.push_back({ TokenType::PLUS, "+", line });
				pos++;
			}
			else if (current == '-')
			{
				tokens.push_back({ TokenType::MINUS, "-", line });
				pos++;
			}
			else if (current == '*')
			{
				tokens.push_back({ TokenType::MULT, "*", line });
				pos++;
			}
			else if (current == '/')
			{
				tokens.push_back({ TokenType::DIV, "/", line });
				pos++;
			}
			else if (current == ',')
			{
				tokens.push_back({ TokenType::COMMA, ",", line });
				pos++;
			}
			else if (current == '(')
			{
				tokens.push_back({ TokenType::LPAREN, "(", line });
				pos++;
			}
			else if (current == ')')
			{
				tokens.push_back({ TokenType::RPAREN, ")", line });
				pos++;
			}
			else
			{
				throw runtime_error("Invalid Character at Line " + to_string(line));
			}
		}

		while (indent_stack.back() > 0)
		{
			indent_stack.pop_back();
			tokens.push_back({ TokenType::DEDENT, "", line });
		}

		tokens.push_back({ TokenType::EOF_TOKEN, "", line });

		return tokens;
	}

private:
	Token read_identifier_or_keyword()
	{
		string value;

		while (pos < source.size() && (isalnum(source[pos]) || source[pos] == '_'))
			value += source[pos++];

		if (value == "def")
			return{ TokenType::DEF, value, line };

		if (value == "int")
			return{ TokenType::INT, value, line };

		if (value == "float")
			return{ TokenType::FLOAT, value, line };

		if (value == "string")
			return{ TokenType::STRING, value, line };

		if (value == "if")
			return{ TokenType::IF, value, line };

		if (value == "for")
			return{ TokenType::FOR, value, line };

		if (value == "while")
			return{ TokenType::WHILE, value, line };

		if (value == "in")
			return{ TokenType::IN, value, line };

		if (value == "range")
			return{ TokenType::RANGE, value, line };

		if (value == "print")
			return{ TokenType::PRINT, value, line };

		if (value == "return")
			return{ TokenType::RETURN, value, line };

		if (value == "and")
			return{ TokenType::AND, value, line };

		if (value == "or")
			return{ TokenType::OR, value, line };

		return{ TokenType::IDENTIFIER, value, line };
	}

	Token read_number_or_float()
	{
		string value;
		bool has_decimal;

		while (pos < source.size() && (isdigit(source[pos]) || source[pos] == '.'))
		{
			if (source[pos] == '.')
			{
				if (has_decimal)
					throw runtime_error("Invalid Number at Line " + to_string(line));

				has_decimal = true;
			}

			value += source[pos++];
		}

		return has_decimal ? Token{ TokenType::FLOATING, value, line } : Token{ TokenType::NUMBER, value, line };
	}

	Token read_string()
	{
		string value;
		pos++; //Skip opening quote

		while (pos < source.size() && source[pos] != '"')
			value += source[pos++];

		pos++; //Skip closing quote

		return{ TokenType::STRING_LITERAL, value, line };
	}

	void handle_indent(vector<Token>& tokens)
	{
		int spaces = 0;

		while (pos < source.size() && (source[pos] == ' ' || source[pos] == '\t'))
		{
			spaces += (source[pos] == '\t' ? 4 : 1);
			pos++;
		}

		if (pos < source.size() && source[pos] != '\n')
		{
			if (spaces > indent_stack.back())
			{
				indent_stack.push_back(spaces);
				tokens.push_back({ TokenType::INDENT, "", line });
			}
			else if (spaces < indent_stack.back())
			{
				while (spaces < indent_stack.back())
				{
					indent_stack.pop_back();
					tokens.push_back({ TokenType::DEDENT, "", line });
				}

				if (spaces != indent_stack.back())
					throw runtime_error("Inconsistent Indentation at Line " + to_string(line));
			}
		}
	}
};

//Abstract Syntax Tree
struct ASTNode
{
	virtual string generate_c_code() const = 0;
	virtual VarType get_type() const
	{
		return VarType::NONE;
	}
	virtual ~ASTNode() = default;
};

//AST Node Types
struct PrintNode : ASTNode
{
	string value;
	VarType type;

	PrintNode(const string& val, VarType t) : value(val), type(t) {}

	string generate_c_code() const override
	{
		if (type == VarType::STRING)
			return "printf(\"%s\\n\", " + value + ");\n";
		else if (type == VarType::INT)
			return "printf(\"%d\\n\", " + value + ");\n";
		else if (type == VarType::FLOAT)
			return "printf(\"%f\\n\", " + value + ");\n";

		return "";
	}
};

struct AssignNode : ASTNode
{
	string var;
	string value;
	VarType type;
	bool is_declaration;

	AssignNode(const string& v, const string& val, VarType t, bool decl) : var(v), value(val), type(t), is_declaration(decl) {}

	string generate_c_code() const override
	{
		if (type == VarType::STRING)
		{
			string temp_var = "temp_" + var + "_" + to_string(rand()); //Unique temp variable
			string code;

			if (is_declaration)
			{
				code = "char* " + var + " = (char*)malloc(strlen(" + value + ") + 1);\n";
				code += "    strcpy(" + var + ", " + value + ");\n";
			}
			else
			{
				code = "free(" + var + ");\n";
				code += "    " + var + " = (char*)malloc(strlen(" + value + ") + 1);\n";
				code += "    strcpy(" + var + ", " + value + ");\n";
			}

			return code;
		}
		else
		{
			string type_str = type == VarType::INT ? "int" : "double";

			if (is_declaration)
				return type_str + " " + var + " = " + value + ";\n";

			return var + " = " + value + ";\n";
		}
	}

	VarType get_type() const override
	{
		return type;
	}
};

struct CallNode : ASTNode
{
	string func_name;
	vector<string> args;
	VarType return_type;

	CallNode(const string& name, const vector<string>& a, VarType rt) : func_name(name), args(a), return_type(rt) {}

	string generate_c_code() const override
	{
		string code = func_name + "(";

		for (size_t i = 0; i < args.size(); ++i)
		{
			code += args[i];

			if (i < args.size() - 1)
				code += ", ";
		}

		code += ")";

		return code;
	}

	VarType get_type() const override
	{
		return return_type;
	}
};

struct ReturnNode : ASTNode
{
	string value;
	VarType type;

	ReturnNode(const string& val, VarType t) : value(val), type(t) {}

	string generate_c_code() const override
	{
		if (type == VarType::STRING)
			return "return strdup(" + value + ");\n";
		
		return "return " + value + ";\n";
	}

	VarType get_type() const override
	{
		return type;
	}
};

struct IfNode : ASTNode
{
	string condition;
	vector<unique_ptr<ASTNode>> body;

	IfNode(const string& cond) : condition(cond) {}

	string generate_c_code() const override
	{
		string code = "if (" + condition + ")\n{\n";

		for (const auto& stmt : body)
			code += "    " + stmt->generate_c_code();

		code += "\n}\n";

		return code;
	}
};

struct ForNode : ASTNode
{
	string var;
	string start;
	string end;
	vector<unique_ptr<ASTNode>> body;

	ForNode(const string& v, const string& s, const string& e) : var(v), start(s), end(e) {}

	string generate_c_code() const override
	{
		string code = "for (int " + var + " = " + start + "; " + var + " < " + end + "; " + var + "++)\n{\n";

		for (const auto& stmt : body)
			code += "    " + stmt->generate_c_code();

		code += "\n}\n";

		return code;
	}
};

struct WhileNode : ASTNode
{
	string condition;
	vector<unique_ptr<ASTNode>> body;

	WhileNode(const string& cond) : condition(cond) {}

	string generate_c_code() const override
	{
		string code = "while (" + condition + ")\n{\n";

		for (const auto& stmt : body)
			code += "    " + stmt->generate_c_code();

		code += "\n}\n";

		return code;
	}
};

struct FunctionNode : ASTNode
{
	string name;
	vector<pair<string, VarType>> args;
	VarType return_type;
	vector<unique_ptr<ASTNode>> body;

	FunctionNode(const string& n, const vector<pair<string, VarType>>& a, VarType rt) : name(n), args(a), return_type(rt) {}

	string generate_c_code() const override
	{
		string type_str = return_type == VarType::INT ? "int" : return_type == VarType::FLOAT ? "double" : "char*";
		string code = type_str + " " + name + "(";

		for (size_t i = 0; i < args.size(); ++i)
		{
			string arg_type = args[i].second == VarType::INT ? "int" : args[i].second == VarType::FLOAT ? "double" : "const char*";
			code += arg_type + " " + args[i].first;

			if (i < args.size() - 1)
				code += ", ";
		}

		code += ")\n{\n";

		for (const auto& stmt : body)
			code += "    " + stmt->generate_c_code();

		if (return_type == VarType::STRING)
			code += "    return NULL;\n"; //Default return for string functions

		code += "\n}\n";

		return code;
	}

	VarType get_type() const override
	{
		return return_type;
	}
};

//Parser Class
class Parser
{
private:
	vector<Token> tokens;
	size_t pos;
	map<string, VarType> variables;							//Track variable types
	map<string, pair<vector<VarType>, VarType>> functions;	//Track function signatures
	int string_temp_counter;								//For unique temporary variables in string concatenation
	//set<string> variables; //Track declared variables
	//set<string> functions; //Track defined functions

public:
	Parser(const vector<Token>& t) : tokens(t), pos(0), string_temp_counter(0) {}

	vector<unique_ptr<ASTNode>> parse_program()
	{
		vector<unique_ptr<ASTNode>> program;

		while (tokens[pos].type != TokenType::EOF_TOKEN)
			program.push_back(parse_statement());

		return program;
	}

private:
	VarType token_to_vartype(TokenType type)
	{
		if (type == TokenType::INT)
			return VarType::INT;

		if (type == TokenType::FLOAT)
			return VarType::FLOAT;

		if (type == TokenType::STRING)
			return VarType::STRING;

		throw runtime_error("Invalid Type at Line " + to_string(tokens[pos].line));
	}

	unique_ptr<ASTNode> parse_statement()
	{
		if (tokens[pos].type == TokenType::DEF)
			return parse_function();
		else if (tokens[pos].type == TokenType::PRINT)
			return parse_print();
		else if (tokens[pos].type == TokenType::IF)
			return parse_if();
		else if (tokens[pos].type == TokenType::FOR)
			return parse_for();
		else if (tokens[pos].type == TokenType::WHILE)
			return parse_while();
		else if (tokens[pos].type == TokenType::RETURN)
			return parse_return();
		else if (tokens[pos].type == TokenType::INT || tokens[pos].type == TokenType::FLOAT || tokens[pos].type == TokenType::STRING)
			return parse_assignment();
		else if (tokens[pos].type == TokenType::IDENTIFIER && tokens[pos + 1].type == TokenType::LPAREN)
			return parse_function_call();

		throw runtime_error("Unexpected Token at Line " + to_string(tokens[pos].line));
	}

	unique_ptr<ASTNode> parse_function()
	{
		expect(TokenType::DEF);

		string name = expect(TokenType::IDENTIFIER).value;

		expect(TokenType::LPAREN);

		vector<pair<string, VarType>> args;
		vector<VarType> arg_types;

		if (tokens[pos].type == TokenType::INT || tokens[pos].type == TokenType::FLOAT || tokens[pos].type == TokenType::STRING)
		{
			VarType type = token_to_vartype(tokens[pos].type);
			
			expect(tokens[pos].type);

			string arg_name = expect(TokenType::IDENTIFIER).value;

			args.emplace_back(arg_name, type);
			arg_types.push_back(type);
			variables[arg_name] = type;

			while (tokens[pos].type == TokenType::COMMA)
			{
				expect(TokenType::COMMA);

				type = token_to_vartype(tokens[pos].type);

				expect(tokens[pos].type);

				arg_name = expect(TokenType::IDENTIFIER).value;
				args.emplace_back(arg_name, type);
				arg_types.push_back(type);
				variables[arg_name] = type;
			}
		}

		expect(TokenType::RPAREN);

		VarType return_type = VarType::INT;

		if (tokens[pos].type == TokenType::COLON && tokens[pos + 1].type != TokenType::NEWLINE)
		{
			expect(TokenType::COLON);

			return_type = token_to_vartype(tokens[pos].type);

			expect(tokens[pos].type);
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

	unique_ptr<ASTNode> parse_print()
	{
		expect(TokenType::PRINT);
		expect(TokenType::LPAREN);

		VarType type;
		string value;

		if (tokens[pos].type == TokenType::STRING_LITERAL)
		{
			value = "\"" + expect(TokenType::STRING_LITERAL).value + "\"";
			type = VarType::STRING;
		}
		else if (tokens[pos].type == TokenType::IDENTIFIER)
		{
			string id = expect(TokenType::IDENTIFIER).value;

			if (variables.find(id) == variables.end())
				throw runtime_error("Undeclared Variable '" + id + "' at Line " + to_string(tokens[pos].line));

			value = id;
			type = variables[id];
		}
		else
		{
			auto expr = parse_expression();
			value = expr.first;
			type = expr.second;

			if (type == VarType::STRING)
				value = "\"" + value + "\"";
		}

		expect(TokenType::RPAREN);
		expect(TokenType::NEWLINE);

		return make_unique<PrintNode>(value, type);
	}

	unique_ptr<ASTNode> parse_assignment()
	{
		VarType type = token_to_vartype(tokens[pos].type);

		expect(tokens[pos].type);

		string var = expect(TokenType::IDENTIFIER).value;

		expect(TokenType::EQUALS);

		auto expr = parse_expression();

		if ((type == VarType::INT || type == VarType::FLOAT) && expr.second == VarType::STRING)
			throw runtime_error("Type Mismatch in Assignment at Line " + to_string(tokens[pos].line));

		if (type == VarType::STRING && expr.second != VarType::STRING)
			throw runtime_error("Type Mismatch in Assignment at Line " + to_string(tokens[pos].line));

		expect(TokenType::NEWLINE);

		bool is_declaration = variables.find(var) == variables.end();

		if (is_declaration)
			variables[var] = type;
		else if (variables[var] != type)
			throw runtime_error("Cannot Change Type of Variable '" + var + "' at Line " + to_string(tokens[pos].line));

		string value = expr.first;

		if (type == VarType::STRING && expr.second == VarType::STRING && value.find("strcat") == string::npos)
			value = "\"" + value + "\"";

		return make_unique<AssignNode>(var, value, type, is_declaration);
	}

	unique_ptr<ASTNode> parse_function_call()
	{
		string name = expect(TokenType::IDENTIFIER).value;

		if (functions.find(name) == functions.end())
			throw runtime_error("Undefined Function '" + name + "' at Line " + to_string(tokens[pos].line));

		expect(TokenType::LPAREN);

		vector<string> args;
		vector<VarType> provided_types;

		if (tokens[pos].type == TokenType::IDENTIFIER || tokens[pos].type == TokenType::NUMBER ||
			tokens[pos].type == TokenType::FLOATING || tokens[pos].type == TokenType::STRING_LITERAL ||
			tokens[pos].type == TokenType::LPAREN)
		{
			auto expr = parse_expression();
			args.push_back(expr.first);
			provided_types.push_back(expr.second);

			while (tokens[pos].type == TokenType::COMMA)
			{
				expect(TokenType::COMMA);

				expr = parse_expression();
				args.push_back(expr.first);
				provided_types.push_back(expr.second);
			}
		}

		expect(TokenType::RPAREN);
		expect(TokenType::NEWLINE);

		vector<VarType> arg_types = functions[name].first;

		if (arg_types.size() != provided_types.size())
			throw runtime_error("Incorrect Number of Arguments for Function '" + name + "' at Line " + to_string(tokens[pos].line));

		for (size_t i = 0; i < arg_types.size(); ++i)
		{
			if (arg_types[i] != provided_types[i])
				throw runtime_error("Type Mismatch in Argument " + to_string(i + 1) + " for Function '" + name + "' at Line " + to_string(tokens[pos].line));
		}

		return make_unique<CallNode>(name, args, functions[name].second);
	}

	unique_ptr<ASTNode> parse_return()
	{
		expect(TokenType::RETURN);

		auto expr = parse_expression();

		expect(TokenType::NEWLINE);

		return make_unique<ReturnNode>(expr.first, expr.second);
	}

	unique_ptr<ASTNode> parse_if()
	{
		expect(TokenType::IF);

		auto condition = parse_expression();

		if (condition.second != VarType::INT && condition.second != VarType::FLOAT)
			throw runtime_error("Condition must be Numeric at Line " + to_string(tokens[pos].line));

		expect(TokenType::COLON);
		expect(TokenType::NEWLINE);
		expect(TokenType::INDENT);

		auto if_node = make_unique<IfNode>(condition.first);

		while (tokens[pos].type != TokenType::DEDENT && tokens[pos].type != TokenType::EOF_TOKEN)
			if_node->body.push_back(parse_statement());

		expect(TokenType::DEDENT);

		return if_node;
	}

	unique_ptr<ASTNode> parse_for()
	{
		expect(TokenType::FOR);

		string var = expect(TokenType::IDENTIFIER).value;

		expect(TokenType::IN);
		expect(TokenType::RANGE);
		expect(TokenType::LPAREN);

		string start = expect(TokenType::NUMBER).value;

		expect(TokenType::COMMA);

		string end = expect(TokenType::NUMBER).value;

		expect(TokenType::RPAREN);
		expect(TokenType::COLON);
		expect(TokenType::NEWLINE);
		expect(TokenType::INDENT);

		auto for_node = make_unique<ForNode>(var, start, end);
		variables[var] = VarType::INT;

		while (tokens[pos].type != TokenType::DEDENT && tokens[pos].type != TokenType::EOF_TOKEN)
			for_node->body.push_back(parse_statement());

		expect(TokenType::DEDENT);

		return for_node;
	}

	unique_ptr<ASTNode> parse_while()
	{
		expect(TokenType::WHILE);

		auto condition = parse_expression();

		if (condition.second != VarType::INT && condition.second != VarType::FLOAT)
			throw runtime_error("Condition must be Numeric at Line " + to_string(tokens[pos].line));

		expect(TokenType::COLON);
		expect(TokenType::NEWLINE);
		expect(TokenType::INDENT);

		auto while_node = make_unique<WhileNode>(condition.first);

		while (tokens[pos].type != TokenType::DEDENT && tokens[pos].type != TokenType::EOF_TOKEN)
			while_node->body.push_back(parse_statement());

		expect(TokenType::DEDENT);

		return while_node;
	}

	pair<string, VarType> parse_expression(int min_prec = 1)
	{
		auto atom = parse_atom();
		string expr = atom.first;
		VarType type = atom.second;

		while (pos < tokens.size() && get_precedence(tokens[pos].type) >= min_prec)
		{
			TokenType op = tokens[pos].type;
			int prec = get_precedence(op);

			expect(op);

			int next_min_prec = prec + (is_right_associative(op) ? 0 : 1);
			auto rhs = parse_expression(next_min_prec);

			if (op == TokenType::PLUS && type == VarType::STRING && rhs.second == VarType::STRING)
			{
				string temp_var = "temp_str_" + to_string(string_temp_counter++);

				string code = "(char*)malloc(strlen(" + expr + ") + strlen(" + rhs.first + ") + 1); ";
				code += "strcpy(" + temp_var + ", " + expr + "); ";
				code += "strcat(" + temp_var + ", " + rhs.first + ")";

				expr = temp_var;
				type = VarType::STRING;
			}
			else if ((type == VarType::STRING || rhs.second == VarType::STRING) && (op == TokenType::PLUS || op == TokenType::MINUS || op == TokenType::MULT || op == TokenType::DIV))
				throw runtime_error("Invalid Operation on String at Line " + to_string(tokens[pos].line));
			else if (op == TokenType::AND || op == TokenType::OR)
			{
				if (type != VarType::INT && type != VarType::FLOAT)
					throw runtime_error("Logical Operator Requires Numeric Operands at Line " + to_string(tokens[pos].line));

				if (rhs.second != VarType::INT && rhs.second != VarType::FLOAT)
					throw runtime_error("Logical Operator Requires Numeric Operands at Line " + to_string(tokens[pos].line));

				expr = expr + " " + get_operator(op) + " " + rhs.first;
				type = VarType::INT;	//Logical ops yield int (0 or 1)
			}
			else
			{
				if (type == VarType::FLOAT || rhs.second == VarType::FLOAT)
					type = VarType::FLOAT;

				expr = expr + " " + get_operator(op) + " " + rhs.first;

				if (op == TokenType::EQ || op == TokenType::GREATER || op == TokenType::LESSER)
					type = VarType::INT;
			}
		}

		return{ expr, type };
	}

	/*
	//OLD
	string parse_expression()
	{
		string left = parse_simple_expression();

		if (tokens[pos].type == TokenType::GREATER || tokens[pos].type == TokenType::LESSER || tokens[pos].type == TokenType::EQ)
		{
			string op = expect(tokens[pos].type).value;
			string right = parse_simple_expression();

			return left + " " + op + " " + right;
		}
		else if (tokens[pos].type == TokenType::PLUS || tokens[pos].type == TokenType::MINUS || tokens[pos].type == TokenType::MULT || tokens[pos].type == TokenType::DIV)
		{
			string op = expect(tokens[pos].type).value;
			string right = parse_simple_expression();

			return left + " " + op + " " + right;
		}

		return left;
	}
	*/

	pair<string, VarType> parse_atom()
	{
		if (tokens[pos].type == TokenType::LPAREN)
		{
			expect(TokenType::LPAREN);

			auto expr = parse_expression();

			expect(TokenType::RPAREN);

			return{ "(" + expr.first + ")", expr.second };
		}
		else if (tokens[pos].type == TokenType::IDENTIFIER)
		{
			string id = expect(TokenType::IDENTIFIER).value;

			if (tokens[pos].type == TokenType::LPAREN)
			{
				string name = id;

				if (functions.find(name) == functions.end())
					throw runtime_error("Undefined Function '" + name + "' at Line " + to_string(tokens[pos].line));

				expect(TokenType::LPAREN);

				vector<string> args;
				vector<VarType> provided_types;

				if (tokens[pos].type == TokenType::IDENTIFIER || tokens[pos].type == TokenType::NUMBER ||
					tokens[pos].type == TokenType::FLOATING || tokens[pos].type == TokenType::STRING_LITERAL ||
					tokens[pos].type == TokenType::LPAREN)
				{
					auto expr = parse_expression();
					args.push_back(expr.first);
					provided_types.push_back(expr.second);

					while (tokens[pos].type == TokenType::COMMA)
					{
						expect(TokenType::COMMA);

						expr = parse_expression();
						args.push_back(expr.first);
						provided_types.push_back(expr.second);
					}
				}

				expect(TokenType::RPAREN);

				vector<VarType> arg_types = functions[name].first;

				if (arg_types.size() != provided_types.size())
					throw runtime_error("Incorrect Number of Arguments for Function '" + name + "' at Line " + to_string(tokens[pos].line));

				for (size_t i = 0; i < arg_types.size(); ++i)
				{
					if (arg_types[i] != provided_types[i])
						throw runtime_error("Type Mismatch in Argument " + to_string(i + 1) + " for Function '" + name + "' at Line " + to_string(tokens[pos].line));
				}

				string call = name + "(";

				for (size_t i = 0; i < args.size(); ++i)
				{
					call += args[i];

					if (i < args.size() - 1)
						call += ", ";
				}

				call += ")";

				return{ call, functions[name].second };
			}

			if (variables.find(id) == variables.end())
				throw runtime_error("Undeclared Variable '" + id + "' at Line " + to_string(tokens[pos].line));

			return{ id, variables[id] };
		}
		else if (tokens[pos].type == TokenType::NUMBER)
			return{ expect(TokenType::NUMBER).value, VarType::INT };
		else if (tokens[pos].type == TokenType::FLOATING)
			return{ expect(TokenType::FLOATING).value, VarType::FLOAT };
		else if (tokens[pos].type == TokenType::STRING_LITERAL)
		{
			string val = "\"" + expect(TokenType::STRING_LITERAL).value + "\"";
			return{ val, VarType::STRING };
		}

		throw runtime_error("Invalid Expression at Line " + to_string(tokens[pos].line));
	}

	int get_precedence(TokenType op)
	{
		switch (op)
		{
		case TokenType::PLUS:
			return 2;
		case TokenType::MINUS:
			return 2;
		case TokenType::MULT:
			return 3;
		case TokenType::DIV:
			return 3;
		case TokenType::GREATER:
			return 1;
		case TokenType::LESSER:
			return 1;
		case TokenType::GREATEREQ:
			return 1;
		case TokenType::LESSEREQ:
			return 1;
		case TokenType::EQ:
			return 1;
		case TokenType::AND:
			return 0;
		case TokenType::OR:
			return 0;
		default:
			return -1;
		}
	}

	bool is_right_associative(TokenType op)
	{
		return false;	//All operators are left-associative
	}

	string get_operator(TokenType op)
	{
		switch (op)
		{
		case TokenType::PLUS:
			return "+";
		case TokenType::MINUS:
			return "-";
		case TokenType::MULT:
			return "*";
		case TokenType::DIV:
			return "/";
		case TokenType::GREATER:
			return ">";
		case TokenType::LESSER:
			return "<";
		case TokenType::GREATEREQ:
			return ">=";
		case TokenType::LESSEREQ:
			return "<=";
		case TokenType::EQ:
			return "==";
		case TokenType::AND:
			return "&&";
		case TokenType::OR:
			return "||";
		default:
			throw runtime_error("Invalid Operator");
		}
	}

	string parse_simple_expression()
	{
		if (tokens[pos].type == TokenType::IDENTIFIER)
		{
			string id = expect(TokenType::IDENTIFIER).value;

			if (variables.find(id) == variables.end() && functions.find(id) == functions.end())
				throw runtime_error("Undeclared Variable or Function '" + id + "' at Line " + to_string(tokens[pos].line));

			if (tokens[pos].type == TokenType::LPAREN)
			{
				vector<string> args;

				expect(TokenType::LPAREN);

				if (tokens[pos].type == TokenType::IDENTIFIER || tokens[pos].type == TokenType::NUMBER)
				{
					args.push_back(parse_simple_expression());

					while (tokens[pos].type == TokenType::COMMA)
					{
						expect(TokenType::COMMA);
						args.push_back(parse_simple_expression());
					}
				}

				expect(TokenType::RPAREN);

				string call = id + "(";

				for (size_t i = 0; i < args.size(); ++i)
				{
					call += args[i];

					if (i < args.size() - 1)
						call += ", ";
				}

				call += ")";

				return call;
			}

			return id;
		}
		else if (tokens[pos].type == TokenType::NUMBER)
			return expect(TokenType::NUMBER).value;

		throw runtime_error("Invalid Expression at Line " + to_string(tokens[pos].line));
	}

	Token expect(TokenType type)
	{
		if (pos >= tokens.size() || tokens[pos].type != type)
			//throw runtime_error("Expected Token Type " + type + " at Line " + to_string(tokens[pos].line));
			throw runtime_error("Expected Token Type at Line " + to_string(tokens[pos].line));

		return tokens[pos++];
	}
};

//Main Function
int main()
{
	//Input Source Code File
	ifstream file("input.minipy");

	if (!file.is_open())
	{
		cerr << "Error opening file!" << endl;
		return 1;
	}

	string source((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

	try
	{
		//Lexing
		Lexer lexer(source);
		auto tokens = lexer.tokenize();

		//Parsing
		Parser parser(tokens);
		auto program = parser.parse_program();

		//Code Generation
		string c_code = "#include <stdio.h>\n";
		c_code += "#include <string.h>\n";
		c_code += "#include <stdlib.h>\n\n";

		c_code += "int main()\n{\n";

		for (const auto& node : program)
			c_code += node->generate_c_code();

		c_code += "\n    return 0;\n}\n";

		//c_code += "\nint main()\n{\n    return main();\n}\n";

		//Write to File
		ofstream out("output.c");
		out << c_code;
		out.close();

		cout << "Generated C Code:\n" << c_code << "\n";

		//Compile using cl.exe
		system("cl /EHsc output.c");
		cout << "Compiled to output.exe\n";
	}
	catch (const exception& e)
	{
		cerr << "Error: " << e.what() << "\n";
	}

	return 0;
}
