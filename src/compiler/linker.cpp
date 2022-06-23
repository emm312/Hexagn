#include <compiler/linker.h>

#include <iostream>

#include <compiler/parser.h>
#include <compiler/token.h>
#include <util.h>

struct LenghtEncodedType
{
	const size_t len;
	const std::string val;
};

const LenghtEncodedType encodeType(const Token& typeName)
{
	const size_t len = typeName.m_val.length();
	(void) len; // Temporary

	switch (typeName.m_type)
	{
		case TokenType::TT_VOID: return { len, "v" };
		case TokenType::TT_INT:
		{
			if (typeName.m_val == "int8")  return { size_t(-1), "i8"  };
			if (typeName.m_val == "int16") return { size_t(-1), "i16" };
			if (typeName.m_val == "int32") return { size_t(-1), "i32" };
		}
		case TokenType::TT_UINT:
		{
			if (typeName.m_val == "uint8")  return { size_t(-1), "u8"  };
			if (typeName.m_val == "uint16") return { size_t(-1), "u16" };
			if (typeName.m_val == "uint32") return { size_t(-1), "u32" };
		}
		case TokenType::TT_FLOAT:
		{
			if (typeName.m_val == "float32") return { size_t(-1), "f32" };
			if (typeName.m_val == "float64") return { size_t(-1), "f64" };
		}
		case TokenType::TT_STRING:    return { size_t(-1), "s" };
		case TokenType::TT_CHARACTER: return { size_t(-1), "c" };
		
		default:
			return { 0, "" };
	}
}

const std::string Function::getSignature() const
{
	std::stringstream ss;

	ss << "_Hx" /* Hexagn mangled name signature */ << name.m_val.length() << name.m_val;

	for (const auto& arg: argTypes)
	{
		const LenghtEncodedType& var = encodeType(arg);
		if (var.len == size_t(-1))
			ss << var.val;
		else
			ss << '_' << var.len << var.val;
	}

	return ss.str();
}

void Linker::addFunction(const Function& function)
{
	// Check for duplicate function
	for (const auto& func: linkerFunctions)
	{
		if (func.getSignature() == function.getSignature())
		{
			std::cerr << "Error: Duplicate function '" << function.name.m_val << "'\n";
			std::cerr << "Previous definition:\n";
			std::cerr << func.returnType.m_lineno << ": " << getSourceLine(glob_src, func.returnType.m_lineno);
			if (func.name.m_lineno != func.returnType.m_lineno)
			{
				std::cerr << func.name.m_lineno << ": " << getSourceLine(glob_src, func.name.m_lineno);
				drawArrows(func.name.m_start, func.name.m_end, func.name.m_lineno);
			}
			else
				drawArrows(func.returnType.m_start, func.name.m_end, func.returnType.m_lineno);
			exit(-1);
		}

		if (func.name.m_val == function.name.m_val && func.returnType.toString() != function.returnType.toString() && func.argTypes == function.argTypes)
		{
			std::cerr << "Cannot have functions with same arguments but different return types: " << function.name.m_val << '\n';
			std::cerr << "Previous definition:\n";
			std::cerr << func.returnType.m_lineno << ": " << getSourceLine(glob_src, func.returnType.m_lineno);
			if (func.name.m_lineno != func.returnType.m_lineno)
			{
				std::cerr << func.name.m_lineno << ": " << getSourceLine(glob_src, func.name.m_lineno);
				drawArrows(func.name.m_start, func.name.m_end, func.name.m_lineno);
			}
			else
				drawArrows(func.returnType.m_start, func.name.m_end, func.name.m_lineno);
			exit(-1);
		}
	}

	linkerFunctions.push_back(function);
}

const Function Linker::getFunction(const Token& name, const std::vector<Token>& argTypes) const
{
	for (const auto& func: linkerFunctions)
		if (func.name.m_val == name.m_val && func.argTypes == argTypes)
			return func;

	std::cerr << "Error: Function '" << name.m_val << "' does not exist\n";
	std::cerr << name.m_lineno << ": " << getSourceLine(glob_src, name.m_lineno);
	drawArrows(name.m_start, name.m_end, name.m_lineno);
	exit(-1);
}

const std::vector<Function>& Linker::getFunctions() const
{
	return linkerFunctions;
}
