#include <compiler/ast/ast-compiler.h>

#include <iostream>
#include <sstream>
#include <optional>
#include <queue>
#include <map>

#include <compiler/linker.h>
#include <compiler/string.h>
#include <importer/importHelper.h>

bool functionExists(const Program& program, const std::string& name)
{
	for (const auto& func: program.statements)
	{
		const FunctionNode* funcNode = dynamic_cast<const FunctionNode*>(func);
		if (funcNode && funcNode->name->name == name) return true;
	}
	
	return false;
}

void VarStack::push(const std::string& name, TypeNode* type)
{
	if (!m_vars.empty())
		m_vars.push_back( { name, m_vars[m_vars.size() - 1].stackOffset + 1, type } );
	else
		m_vars.push_back( { name, 1, type } );

	frameCounter++;
}
void VarStack::pop()
{
	m_vars.pop_back();
}
void VarStack::pop(size_t num)
{
	for (size_t _ = 0; _ < num; ++_) m_vars.pop_back();
}

void VarStack::startFrame()
{
	frameCounter = 0;
}
size_t VarStack::popFrame()
{
	pop(frameCounter);
	return frameCounter;
}
size_t VarStack::getOffset(const std::string& name) const
{
	for (const auto& var: m_vars)
		if (var.name == name)
			return var.stackOffset;
	return -1;
}
std::optional<TypeNode*> VarStack::getType(const std::string& name) const
{
	for (const auto& var: m_vars)
		if (var.name == name)
			return var.type;
	return {};
}
size_t VarStack::getSize() const
{
	return m_vars.size();
}

void visit(std::ostringstream& code, VarStack& vars, const VarStack& funcArgs, const Linker& linker, VarDefineNode* node);
void visit(Linker& linker, FunctionNode* func);
void visit(std::ostringstream& code, const VarStack& vars, const VarStack& funcArgs, const Linker& linker, Node* expr, const size_t& mod);
void visit(std::ostringstream& code, const VarStack& vars, const VarStack& funcArgs, const Linker& linker, BiOpNode* node, const size_t& mod, size_t regCounter);
void visit(std::ostringstream& code, const VarStack& vars, const VarStack& funcArgs, const Linker& linker, FuncCallNode* node);
void visit(std::ostringstream& code, const VarStack& vars, const VarStack& funcArgs, Linker& linker, IfNode* node);
void visit(std::ostringstream& code, const VarStack& vars, const VarStack& funcArgs, Linker& linker, WhileNode* node);
void visit(std::ostringstream& code, Linker& linker, ImportNode* node);
void visit(std::ostringstream& code, UrclCodeblockNode* node);

std::string compileAst(const Program& program, const Arguments& args, Linker& linker, const VarStack& funcArgs)
{
	std::ostringstream code;
	VarStack vars;

	if (args.emitEntryPoint)
		code << "BITS == 32\n"
			 << "MINHEAP 4096\n"
			 << "MINSTACK 1024\n"
			 << "CAL ._Hx4maini8\n"
			 << "HLT\n\n";

	for (const auto statement: program.statements)
		switch (statement->nodeType)
		{
			case NodeType::NT_VarDefineNode:
			{
				visit(code, vars, funcArgs, linker, (VarDefineNode*) statement);
				break;
			}
			case NodeType::NT_FunctionNode: 
			{
				visit(linker, (FunctionNode*) statement);
				break;
			}
			case NodeType::NT_FuncCallNode:
			{
				visit(code, vars, funcArgs, linker, (FuncCallNode*) statement);
				break;
			}
			case NodeType::NT_IfNode: 
			{
				visit(code, vars, funcArgs, linker, (IfNode*) statement);
				break;
			}
			case NodeType::NT_WhileNode: {
				visit(code, vars, funcArgs, linker, (WhileNode*) statement);
				break;
			}
			case NodeType::NT_ImportNode: {
				visit(code, linker, (ImportNode*) statement);
				break;
			}
			case NodeType::NT_UrclCodeblockNode: {
				visit(code, (UrclCodeblockNode*) statement);
			}

			default: break;
		}

	if (args.emitEnd)
	{
		for (const Function& func: linker.getFunctions())
		{
			code << '.' << func.getSignature() << '\n';

			// cdecl calling convention entry
			code << "PSH R1\nMOV R1 SP\n\n";

			code << func.code;

			// cdecl calling convention exit
			code << "MOV SP R1\nPOP R1\nRET\n\n";
		}

		for (const std::string& str: getStrings())
			code << str << '\n';
	}

	return code.str();
}

void visit(std::ostringstream& code, VarStack& vars, const VarStack& funcArgs, const Linker& linker, VarDefineNode* node)
{
	vars.push(
		node->ident->name,
		node->type
	);

	size_t mod = 0;
	for (const char& c: node->type->val)
		if (isdigit(c))
		{
			mod *= 10;
			mod += c - '0';
		}

	visit(code, vars, funcArgs, linker, node->expr, mod);
}

void visit(std::ostringstream& code, const VarStack& vars, const VarStack& funcArgs, const Linker& linker, Node* expr, const size_t& mod)
{
	if (!expr)
	{
		code << "DEC SP SP\n\n";
		return;
	}

	switch (expr->nodeType)
	{
		case NodeType::NT_BiOpNode:
		{
			BiOpNode* biOpNode = (BiOpNode*) expr;
			visit(code, vars, funcArgs, linker, biOpNode, mod, 2);
			break;
		}

		case NodeType::NT_NumberNode:
		{
			NumberNode* numNode = (NumberNode*) expr;
			code << "PSH " << numNode->val << "\n\n";
			break;
		}

		case NodeType::NT_IdentifierNode:
		{
			IdentifierNode* identNode = (IdentifierNode*) expr;
			const size_t offset = vars.getOffset(identNode->name);
			if (offset == size_t(-1))
				return;

			code << "LLOD R2 R1 -" << offset << '\n'
				<< "PSH R2\n\n";

			break;
		}

		case NodeType::NT_StringNode:
		{
			StringNode* stringNode = (StringNode*) expr;
			code << "PSH " << registerString(stringNode->val) << "\n\n";
			break;
		}

		case NodeType::NT_FuncCallNode:
		{
			FuncCallNode* funcCallNode = (FuncCallNode*) expr;
			visit(code, vars, funcArgs, linker, funcCallNode);
			code << "PSH R2\n\n";
			break;
		}

		default: break;
	}
}

void visit(Linker& linker, FunctionNode* func)
{
	Function linkerFunc { func->retType, func->name };
	std::vector<TypeNode*> argTypes;
	for (const auto& arg: func->args)
		argTypes.push_back(arg.type);
	linkerFunc.argTypes = argTypes;

	const std::string code = compileAst(
		*func->body,
		{
			false,
			false,
			false
		},
		linker
	);

	linkerFunc.code = code;

	linker.addFunction(linkerFunc);
}

std::string getOperation(const Operation& op)
{
	switch (op)
	{
		case Operation::ADD:  return "ADD ";

		case Operation::SUB:  return "SUB ";

		case Operation::MULT: return "MLT ";

		case Operation::DIV:  return "DIV ";

		// This is useless for now but i will keep it
		case Operation::MOD:  return "MOD ";
		
		default: return "";
	}
}

// Used for PSHing the regs in use when there is function call in expr
size_t maxRegCount = 2;

std::string parseExpr(BiOpNode* expr, size_t regCounter, const VarStack& vars, const VarStack& funcArgs, const Linker& linker, std::queue<std::string>& instrQueue)
{
	std::ostringstream ret;

	ret << getOperation(expr->op);
	ret << 'R' << regCounter << ' ';

	// lhs
	{
		Node* lhs = expr->lhs;
		switch (expr->lhs->nodeType)
		{
			case NodeType::NT_NumberNode:
			{
				NumberNode* numNode = (NumberNode*) lhs;
				ret << numNode->val << ' ';
				break;
			}

			case NodeType::NT_BiOpNode:
			{
				BiOpNode* biOpNode = (BiOpNode*) lhs;
				instrQueue.push(
					parseExpr(biOpNode, ++regCounter, vars, funcArgs, linker, instrQueue)
				);
				ret << 'R' << regCounter << ' ';

				maxRegCount = regCounter > maxRegCount ? regCounter : maxRegCount;
			}

			case NodeType::NT_IdentifierNode:
			{
				IdentifierNode* identNode = (IdentifierNode*) lhs;
				const size_t offset = vars.getOffset(identNode->name);
				if (offset != size_t(-1))
				{
					instrQueue.push(
						"LLOD R" + std::to_string(++regCounter) + " R1 -" + std::to_string(offset)
					);
					ret << 'R' << regCounter << ' ';
				}
			}

			case NodeType::NT_FuncCallNode:
			{
				FuncCallNode* funcCallNode = (FuncCallNode*) lhs;

				for (size_t i = regCounter; i < maxRegCount; ++i)
					instrQueue.push("PSH R" + std::to_string(i) + '\n');

				std::ostringstream temp;
				visit(temp, vars, funcArgs, linker, funcCallNode);
				instrQueue.push(temp.str());

				for (size_t i = regCounter; i < maxRegCount; ++i)
					instrQueue.push("POP R" + std::to_string(i) + '\n');

				ret << "R2";

				break;
			}

			default: break;
		}
	}

	// rhs
	{
		Node* rhs = expr->rhs;
		switch (rhs->nodeType)
		{
			case NodeType::NT_NumberNode:
			{
				NumberNode* numNode = (NumberNode*) rhs;
				ret << numNode->val << '\n';
				break;
			}

			case NodeType::NT_BiOpNode:
			{
				BiOpNode* biOpNode = (BiOpNode*) rhs;
				instrQueue.push(
					parseExpr(biOpNode, ++regCounter, vars, funcArgs, linker, instrQueue)
				);
				ret << 'R' << regCounter << '\n';

				maxRegCount = regCounter > maxRegCount ? regCounter : maxRegCount;
			}

			case NodeType::NT_IdentifierNode:
			{
				IdentifierNode* identNode = (IdentifierNode*) rhs;
				const size_t offset = vars.getOffset(identNode->name);
				if (offset != size_t(-1))
				{
					instrQueue.push(
						"LLOD R" + std::to_string(++regCounter) + " R1 -" + std::to_string(offset)
					);
					ret << 'R' << regCounter << '\n';
				}
			}

			case NodeType::NT_FuncCallNode:
			{
				FuncCallNode* funcCallNode = (FuncCallNode*) rhs;

				for (size_t i = regCounter; i < maxRegCount; ++i)
					instrQueue.push("PSH R" + std::to_string(i) + '\n');

				std::ostringstream temp;
				visit(temp, vars, funcArgs, linker, funcCallNode);
				instrQueue.push(temp.str());

				for (size_t i = regCounter; i < maxRegCount; ++i)
					instrQueue.push("POP R" + std::to_string(i) + '\n');

				ret << "R2\n";

				break;
			}

			default: break;
		}
	}

	return ret.str();
}

void visit(std::ostringstream& code, const VarStack& vars, const VarStack& funcArgs, const Linker& linker, BiOpNode* node, const size_t& mod, size_t regCounter)
{
	std::queue<std::string> instrQueue;

	const std::string expr = parseExpr(node, 2, vars, funcArgs, linker, instrQueue);

	while (!instrQueue.empty())
	{
		const std::string exprCode = instrQueue.front();
 		code << exprCode;
		instrQueue.pop();
	}
	
	code << expr;
	code << "AND R2 R2 0x" << std::hex << ((1 << mod) - 1) << '\n' << std::dec;
	code << "PSH R2\n\n";
}

void visit(std::ostringstream& code, const VarStack& vars, const VarStack& funcArgs, const Linker& linker, FuncCallNode* node)
{
	std::vector<TypeNode*> funcArgTypes;
	for (const auto& arg: node->args)
	{
		visit(code, vars, funcArgs, linker, arg, (1 << 32) - 1);
		// Remove the extra '\n' from the visit() call
		code.seekp(-1, code.cur);
		switch (arg->nodeType)
		{
			case NodeType::NT_NumberNode:
			{
				funcArgTypes.push_back(new TypeNode { "int", false });
				break;
			}

			case NodeType::NT_StringNode:
			{
				std::cout << "arg has str node tyep\n";
				funcArgTypes.push_back(new TypeNode { "string", false });
			}

			case NodeType::NT_IdentifierNode:
			{
				IdentifierNode* ident = (IdentifierNode*) arg;
				TypeNode* type;
				{
					auto optional = vars.getType(ident->name);
					if (!optional.has_value())
						optional = funcArgs.getType(ident->name);
					type = optional.value();
				}
				funcArgTypes.push_back(type);
				break;
			}
			

			default: break;
		}
	}

	const Function& func = linker.getFunction("", node->name->name, funcArgTypes);

	code << "CAL ." << func.getSignature() << '\n'
		 << "ADD SP SP " << funcArgTypes.size() << '\n';
}

size_t ifCount = 0;

void visit(std::ostringstream& code, const VarStack& vars, const VarStack& funcArgs, Linker& linker, IfNode* node)
{
	/*
		.if
			branch if expr == 0
			
			body
			if Else:
				jmp .endElse
		.endIf
			code
		.endElse
			repeat jmp .endElse
	*/
	const std::string body = compileAst(
		node->body,
		Arguments {
			false,
			false,
			false
		},
		linker,
		funcArgs
	);

	code << ".if" << ifCount << '\n';
	visit(code, vars, funcArgs, linker, node->condition, (1 << 32) - 1);
	code << "BRZ .endif" << ifCount << " R2\n";

	code << body;
	code << ".endif" << ifCount << "\n\n";
}

size_t whileCount = 0;

void visit(std::ostringstream& code, const VarStack& vars, const VarStack& funcArgs, Linker& linker, WhileNode* node)
{
	const std::string body = compileAst(
		node->body,
		Arguments {
			false,
			false,
			false
		},
		linker,
		funcArgs
	);
	code << ".while" << whileCount << '\n';
	visit(code, vars, funcArgs, linker, node->condition, (1 << 32) - 1);
	// no i gotta update a couple of function args
	// can we still work on cpp hexag together :pleading_face: no this is last commit
	// liveshare is fun
}

void visit(std::ostringstream& code, Linker& linker, ImportNode* node) {
	importLibrary(linker, node->library);
}

void visit(std::ostringstream& code, UrclCodeblockNode* node) {
	code << node->code << '\n';
}