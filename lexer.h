#pragma once
#include "ASTNodes.h"

using namespace std;

//---LEXER---
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
				tokens.emplace_back(TokenType::NEWLINE, "", line);
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
				tokens.push_back(read_identifier_or_keyword());
			else if (isdigit(current) || current == '.')
				tokens.push_back(read_number_or_float());
			else if (current == '"')
				tokens.push_back(read_string());
			else if (current == 'f' && pos + 1 < source.size() && source[pos + 1] == '"')
				read_fstring(tokens);
			else if (current == ':')
			{
				tokens.emplace_back(TokenType::COLON, ":", line);
				pos++;
			}
			else if (current == '=')
			{
				if (pos + 1 < source.size() && source[pos + 1] == '=')
				{
					tokens.emplace_back(TokenType::EQ, "==", line);
					pos += 2;
				}
				else
				{
					tokens.emplace_back(TokenType::EQUALS, "=", line);
					pos++;
				}
			}
			else if (current == '!')
			{
				if (pos + 1 < source.size() && source[pos + 1] == '=')
				{
					tokens.emplace_back(TokenType::NOTEQ, "!=", line);
					pos += 2;
				}
				else
					throw runtime_error("Invalid Character '!' at Line " + to_string(line));
			}
			else if (current == '>')
			{
				if (pos + 1 < source.size() && source[pos + 1] == '=')
				{
					tokens.emplace_back(TokenType::GREATEREQ, ">=", line);
					pos += 2;
				}
				else
				{
					tokens.emplace_back(TokenType::GREATER, ">", line);
					pos++;
				}
			}
			else if (current == '<')
			{
				if (pos + 1 < source.size() && source[pos + 1] == '=')
				{
					tokens.emplace_back(TokenType::LESSEREQ, "<=", line);
					pos += 2;
				}
				else
				{
					tokens.emplace_back(TokenType::LESSER, "<", line);
					pos++;
				}
			}
			else if (current == '+')
			{
				tokens.emplace_back(TokenType::PLUS, "+", line);
				pos++;
			}
			else if (current == '-')
			{
				tokens.emplace_back(TokenType::MINUS, "-", line);
				pos++;
			}
			else if (current == '*')
			{
				tokens.emplace_back(TokenType::MULT, "*", line);
				pos++;
			}
			else if (current == '/')
			{
				tokens.emplace_back(TokenType::DIV, "/", line);
				pos++;
			}
			else if (current == '(')
			{
				tokens.emplace_back(TokenType::LPAREN, "(", line);
				pos++;
			}
			else if (current == ')')
			{
				tokens.emplace_back(TokenType::RPAREN, ")", line);
				pos++;
			}
			else if (current == '[')
			{
				tokens.emplace_back(TokenType::LBRACKET, "[", line);
				pos++;
			}
			else if (current == ']')
			{
				tokens.emplace_back(TokenType::RBRACKET, "]", line);
				pos++;
			}
			else if (current == '{')
			{
				tokens.emplace_back(TokenType::LBRACE, "{", line);
				pos++;
			}
			else if (current == '}')
			{
				tokens.emplace_back(TokenType::RBRACE, "}", line);
				pos++;
			}
			else if (current == ',')
			{
				tokens.emplace_back(TokenType::COMMA, ",", line);
				pos++;
			}
			else if (current == '.')
			{
				tokens.emplace_back(TokenType::DOT, ".", line);
				pos++;
			}
			else
				throw runtime_error("Invalid Character at Line " + to_string(line));
		}

		while (indent_stack.back() > 0)
		{
			indent_stack.pop_back();
			tokens.emplace_back(TokenType::DEDENT, "", line);
		}

		tokens.emplace_back(TokenType::EOF_TOKEN, "", line);

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

		if (value == "return")
			return{ TokenType::RETURN, value, line };

		if (value == "print")
			return{ TokenType::PRINT, value, line };

		if (value == "int")
			return{ TokenType::INT, value, line };

		if (value == "float")
			return{ TokenType::FLOAT, value, line };

		if (value == "string")
			return{ TokenType::STRING, value, line };

		if (value == "bool")
			return{ TokenType::BOOL, value, line };

		if (value == "list")
			return{ TokenType::LIST, value, line };

		if (value == "tuple")
			return{ TokenType::TUPLE, value, line };

		if (value == "dict")
			return{ TokenType::DICT, value, line };

		if (value == "if")
			return{ TokenType::IF, value, line };

		if (value == "elif")
			return{ TokenType::ELIF, value, line };

		if (value == "else")
			return{ TokenType::ELSE, value, line };

		if (value == "for")
			return{ TokenType::FOR, value, line };

		if (value == "in")
			return{ TokenType::IN, value, line };

		if (value == "range")
			return{ TokenType::RANGE, value, line };

		if (value == "while")
			return{ TokenType::WHILE, value, line };

		if (value == "match")
			return{ TokenType::MATCH, value, line };

		if (value == "case")
			return{ TokenType::CASE, value, line };

		if (value == "true")
			return{ TokenType::TRUE, value, line };

		if (value == "false")
			return{ TokenType::FALSE, value, line };

		if (value == "and")
			return{ TokenType::AND, value, line };

		if (value == "or")
			return{ TokenType::OR, value, line };

		if (value == "not")
			return{ TokenType::NOT, value, line };

		if (value == "sep")
			return{ TokenType::SEP, value, line };

		if (value == "len")
			return{ TokenType::LEN, value, line };

		if (value == "append" || value == "upper" || value == "lower" || value == "strip" ||
			value == "replace" || value == "split" || value == "find")
			return{ TokenType::CALL_METHOD, value, line };

		return{ TokenType::IDENTIFIER, value, line };
	}

	Token read_number_or_float()
	{
		string value;
		bool has_decimal = false;

		while (pos < source.size() && (isdigit(source[pos]) || source[pos] == '.'))
		{
			if (source[pos] == '.')
			{
				if (has_decimal)
					throw runtime_error("Invalid Number at Line" + to_string(line));

				has_decimal = true;
			}

			value += source[pos++];
		}

		return has_decimal ? Token{ TokenType::FLOATING, value, line } : Token{ TokenType::NUMBER, value, line };
	}

	Token read_string()
	{
		string value;
		pos++;

		while (pos < source.size() && source[pos] != '"')
			value += source[pos++];

		pos++;

		return{ TokenType::STRING_LITERAL, value, line };
	}

	void read_fstring(vector<Token>& tokens)
	{
		pos += 2;
		tokens.emplace_back(TokenType::FSTRING_START, "", line);

		while (pos < source.size() && source[pos] != '"')
		{
			if (source[pos] == '{')
			{
				tokens.emplace_back(TokenType::FSTRING_EXPR_START, "{", line);
				pos++;
			}
			else if (source[pos] == '}')
			{
				tokens.emplace_back(TokenType::FSTRING_EXPR_END, "}", line);
				pos++;
			}
			else if (source[pos] == ':')
			{
				string format_spec;
				pos++;

				while (pos < source.size() && source[pos] != '}' && source[pos] != '"')
					format_spec += source[pos++];

				tokens.emplace_back(TokenType::FSTRING_FORMAT_SPEC, format_spec, line);
				pos--;
			}
			else
			{
				string value;

				while (pos < source.size() && source[pos] != '"' && source[pos] != '{' && source[pos] != '}' && source[pos] != ':')
					value += source[pos++];

				if (!value.empty())
					tokens.emplace_back(TokenType::STRING_LITERAL, value, line);
			}
		}

		pos++;
		tokens.emplace_back(TokenType::FSTRING_END, "", line);
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
				tokens.emplace_back(TokenType::INDENT, "", line);
			}
			else if (spaces < indent_stack.back())
			{
				while (spaces != indent_stack.back())
				{
					indent_stack.pop_back();
					tokens.emplace_back(TokenType::DEDENT, "", line);
				}

				if (spaces != indent_stack.back())
					throw runtime_error("Inconsistent Indentation at Line " + to_string(line));
			}
		}
	}
};