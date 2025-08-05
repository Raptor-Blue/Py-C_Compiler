#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include "lexer.h"
#include "Parser.h"

//---MAIN---
int main(int argc, char* argv[])
{
	//Check for Input
	if (argc != 2)
	{
		cerr << "Usage: " << argv[0] << " <input.minipy>" << endl;
		return 1;
	}

	//Read MiniPy Source File
	string input_file = argv[1];
	ifstream file(input_file);

	if (!file.is_open())
	{
		cerr << "Error: Could not open input file " << input_file << endl;
		return 1;
	}

	string source((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

	file.close();

	try
	{
		//---Lexer---
		Lexer lexer(source);
		vector<Token> tokens = lexer.tokenize();

		//---Parser---
		Parser parser(tokens);
		vector<unique_ptr<ASTNode>> ast = parser.parse_program();

		//---Code Generator---
		vector<string> gc_strings;
		string c_code;

		//Function Definitions
		for (const auto& node : ast)
		{
			if (dynamic_cast<FunctionNode*>(node.get()))
				c_code += node->generate_c_code(gc_strings) + "\n";
		}

		//Create main()
		c_code += "int main()\n{\n";

		for (const auto& node : ast)
		{
			if (!dynamic_cast<FunctionNode*>(node.get()))
				c_code += "    " + node->generate_c_code(gc_strings) + "\n";
		}

		//Cleanup
		for (const auto& var : gc_strings)
		{
			auto it = parser.get_variables().find(var);

			if (it != parser.get_variables().end())
			{
				CollectionType type = it->second;

				if (type.base_type == VarType::STRING)
					c_code += "    free_string(" + var + ");\n";
				else if (type.base_type == VarType::LIST)
					c_code += "    free_list_" + Parser::vartype_to_c(type.element_type) + "(" + var + ");\n";
				else if (type.base_type == VarType::TUPLE)
					c_code += "    free_tuple_" + Parser::vartype_to_c(type.element_type) + "(" + var + ");\n";
				else if (type.base_type == VarType::DICT)
					c_code += "    free_dict_string_" + Parser::vartype_to_c(type.value_type) + "(" + var + ");\n";
			}
		}

		c_code += "    return 0;\n}\n";

		//Write Code to File
		ofstream out_file("output.c");

		if (!out_file.is_open())
		{
			cerr << "Error: Could not open output file 'output.c'" << endl;
			return 1;
		}

		out_file << c_code;
		out_file.close();

		//Compile
		string compile_command = "\"C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\bin\\cl.exe\" output.c /Feoutput.exe";
		int result = system(compile_command.c_str());

		if (result != 0)
		{
			cerr << "Error: Compilation Failed" << endl;
			return 1;
		}

		cout << "Compilation Successful.\nExecutable: output.exe" << endl;
	}
	catch (const exception& e)
	{
		cerr << "Error: " << e.what() << endl;
		return 1;
	}

	return 0;
}