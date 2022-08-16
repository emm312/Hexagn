#include <compiler/ast/ast-compiler.h>

bool functionExists(const Program& program, const std::string& name)
{
	for (const auto& func: program.functions)
		if (func.name.name == name) return true;
	
	return false;
}

std::string compileAst(const Program& program)
{
	for (const auto& statement: program.statements)
	{
		
	}
}
