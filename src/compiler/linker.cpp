#include <compiler/linker.h>

#include <iostream>

#include <compiler/ast/ast.h>
#include <compiler/token.h>
#include <util.h>

struct LenghtEncodedType
{
	const size_t len;
	const std::string val;
};

const LenghtEncodedType encodeType(const Type& typeName)
{
	const Token& type = typeName.baseType;
	const size_t len = type.m_val.length();

	std::string ret;
	bool isIdentifier = false;

	switch (type.m_type)
	{
		case TokenType::TT_VOID: { ret = 'v'; break; }
		case TokenType::TT_INT:
		{
			if (type.m_val == "int8")  ret = "i8";
			if (type.m_val == "int16") ret = "i16";
			if (type.m_val == "int32") ret = "i32";
			if (type.m_val == "int64") ret = "i64";

			break;
		}
		case TokenType::TT_UINT:
		{
			if (type.m_val == "uint8")  ret = "u8";
			if (type.m_val == "uint16") ret = "u16";
			if (type.m_val == "uint32") ret = "u32";
			if (type.m_val == "uint64") ret = "u64";

			break;
		}
		case TokenType::TT_FLOAT:
		{
			if (type.m_val == "float32") ret = "f32";
			if (type.m_val == "float64") ret = "f64";

			break;
		}
		case TokenType::TT_STRING:    { ret = 's'; break; }
		case TokenType::TT_CHARACTER: { ret = 'c'; break; }

		// Is a custom type like a class
		case TokenType::TT_IDENTIFIER:
		{
			isIdentifier = true;
			ret += type.m_val;

			break;
		}

		default: {
			return { 0, "" };
		}
	}

	if (typeName.isPointer)
		ret += 'P';

	if (!isIdentifier)
		return { size_t(-1), ret };
	else
		return { len, ret };
}

const std::string Function::getSignature() const
{
	std::stringstream ss;

	ss << "_Hx" /* Hexagn mangled name signature */ << name.m_val.length() << name.m_val;

	const LenghtEncodedType& _return = encodeType(returnType);
	if (_return.len == size_t(-1))
		ss << _return.val;
	else
		ss << '_' << _return.len << _return.val;

	for (const auto& arg: argTypes)
	{
		const LenghtEncodedType& var = encodeType({ arg, false /* Temporary 'false' constant */ });
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
		const Token& retType = func.returnType.baseType;

		if (func.getSignature() == function.getSignature())
		{
			std::cerr << "Error: Duplicate function '" << function.name.m_val << "'\n";
			std::cerr << "Previous definition:\n";
			std::cerr << retType.m_lineno << ": " << getSourceLine(glob_src, retType.m_lineno);
			if (func.name.m_lineno != retType.m_lineno)
			{
				std::cerr << func.name.m_lineno << ": " << getSourceLine(glob_src, func.name.m_lineno);
				drawArrows(func.name.m_start, func.name.m_end, func.name.m_lineno);
			}
			else
				drawArrows(retType.m_start, func.name.m_end, retType.m_lineno);
			exit(-1);
		}

		if (func.name.m_val == function.name.m_val && retType.toString() != function.returnType.baseType.toString() && func.argTypes == function.argTypes)
		{
			std::cerr << "Cannot have functions with same arguments but different return types: " << function.name.m_val << '\n';
			std::cerr << "Previous definition:\n";
			std::cerr << retType.m_lineno << ": " << getSourceLine(glob_src, retType.m_lineno);
			if (func.name.m_lineno != retType.m_lineno)
			{
				std::cerr << func.name.m_lineno << ": " << getSourceLine(glob_src, func.name.m_lineno);
				drawArrows(func.name.m_start, func.name.m_end, func.name.m_lineno);
			}
			else
				drawArrows(retType.m_start, func.name.m_end, func.name.m_lineno);
			exit(-1);
		}
	}

	linkerFunctions.push_back(function);
}

const std::string getTypeName(const Token& type)
{
	switch (type.m_type)
	{
		case TokenType::TT_VOID     : return "void";
		case TokenType::TT_INT      : return type.m_val;
		case TokenType::TT_UINT     : return type.m_val;
		case TokenType::TT_NUM      : return "int";
		case TokenType::TT_FLOAT    : return type.m_val;
		case TokenType::TT_STRING   : return "string";
		case TokenType::TT_CHARACTER: return "char";

		default: return "";
	}
}

const Function Linker::getFunction(const std::string& src, const Token& name, const std::vector<Token>& argTypes) const
{
	for (const auto& func: linkerFunctions)
	{
		if (func.name.m_val == name.m_val)
		{
			if (func.argTypes.size() != argTypes.size()) break;
			for (size_t i = 0; i < func.argTypes.size(); ++i)
			{
				if (isIntegerDataType(func.argTypes[i]))
				{
					if (!isIntegerDataType(argTypes[i]) && !isNumber(argTypes[i]))
						goto nextFunc;
				}
				else if (isFloatDataType(func.argTypes[i]) && !isFloatDataType(argTypes[i]))
					goto nextFunc;
				else if (func.argTypes[i].m_type == TokenType::TT_STRING && argTypes[i].m_type != TokenType::TT_STRING)
					goto nextFunc;
			}

			return func;
		}

		nextFunc:;
	}

	std::cerr << "Error: Function '" << name.m_val << "' with arguments ";
	for (const Token& arg: argTypes)
		std::cerr << getTypeName(arg) << ' ';
	std::cerr << "does not exist at line " << name.m_lineno << '\n';
	std::cerr << name.m_lineno << ": " << getSourceLine(src, name.m_lineno);
	drawArrows(name.m_start, name.m_end, name.m_lineno);
	exit(-1);
}

const std::vector<Function>& Linker::getFunctions() const
{
	return linkerFunctions;
}
