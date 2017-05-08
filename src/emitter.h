/***************************************************************************
 * Instruction emitter
 *
 * Copyright (c) 2017 Joshua Ogunyinka
 * Copyright (c) 2013-2016 Randy Hollines
 * All rights reserved.
*/

#ifndef __EMITTER_H__
#define __EMITTER_H__

#include <limits.h>
#include <memory>

#include "common.h"
#include "tree.h"

/****************************
 * Translate trees to instructions
 ****************************/

using std::set;

namespace compiler {
	class Emitter {
		std::map<int, wstring> errors;
		std::unique_ptr<ParsedProgram> parsed_program;
		static vector<Instruction*> instruction_factory;
		INT_T start_label_id;
		INT_T end_label_id;
		int returns_value;

		INT_T NextEndId() {
			return end_label_id++;
		}

		INT_T NextStartId() {
			return start_label_id++;
		}

		void ProcessError( ParseNode* node, const wstring &msg );
		void ProcessError( const wstring &msg );
		bool NoErrors();

		ExecutableClass* EmitClass( ParsedClass* parsed_klass );
		ExecutableFunction* EmitFunction( ParsedFunction* parsed_function );
		void EmitFunction( StatementList* block_statements, vector<Instruction*> & block_instructions, unordered_map<long, size_t>& jump_table, set<size_t> &leaders );
		void EmitBlock( StatementList* block_statements, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table );
		void EmitFunctionCallParameters( Reference* reference, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table );
		void EmitFunctionCall( FunctionCall* function_call, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table );
		void EmitNestedFunctionCall( Reference* reference, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table );
		void EmitIfElse( IfElse* if_else, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table );
		void EmitWhile( While* if_while, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table );
		void EmitAssignment( Assignment* assignment, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table );
		void EmitReference( Reference* reference, bool is_store, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table );
		void EmitExpression( Expression* expression, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table );

	public:
		Emitter( std::unique_ptr<ParsedProgram> && parsed_program ): parsed_program( std::move( parsed_program )) {
			start_label_id = 0;
			end_label_id = INT_MIN;
		}

		~Emitter() {
		}

		static Instruction* MakeInstruction( InstructionType type ) {
			Instruction* instruction = new Instruction;
			instruction->type = type;
			instruction_factory.push_back( instruction );

			return instruction;
		}

		static Instruction* MakeInstruction( InstructionType type, int operand ) {
			Instruction* instruction = new Instruction;
			instruction->type = type;
			instruction->operand1 = operand;
			instruction_factory.push_back( instruction );

			return instruction;
		}

		static Instruction* MakeInstruction( InstructionType type, int operand1, int operand2 ) {
			Instruction* instruction = new Instruction;
			instruction->type = type;
			instruction->operand1 = operand1;
			instruction->operand2 = operand2;
			instruction_factory.push_back( instruction );

			return instruction;

		}

		static Instruction* MakeInstruction( InstructionType type, double operand ) {
			Instruction* instruction = new Instruction;
			instruction->type = type;
			instruction->operand4 = operand;
			instruction_factory.push_back( instruction );

			return instruction;
		}

		static Instruction* MakeInstruction( InstructionType type, int operand1, int operand2, const wstring &operand5 ) {
			Instruction* instruction = new Instruction;
			instruction->type = type;
			instruction->operand1 = operand1;
			instruction->operand2 = operand2;
			instruction->operand5 = operand5;
			instruction_factory.push_back( instruction );

			return instruction;
		}

		static Instruction* MakeInstruction( InstructionType type, int operand1, int operand2, int operand3 ) {
			Instruction* instruction = new Instruction;
			instruction->type = type;
			instruction->operand1 = operand1;
			instruction->operand2 = operand2;
			instruction->operand3 = operand3;
			instruction_factory.push_back( instruction );

			return instruction;
		}

		static Instruction* MakeInstruction( InstructionType type, int operand1, int operand2, const wstring &operand5, const wstring &operand6 ) {
			Instruction* instruction = new Instruction;
			instruction->type = type;
			instruction->operand1 = operand1;
			instruction->operand2 = operand2;
			instruction->operand5 = operand5;
			instruction->operand6 = operand6;
			instruction_factory.push_back( instruction );

			return instruction;
		}

		INT_T GetLastLabelId() {
			return 0;
		}

		static void ClearInstructions();

		std::unique_ptr<ExecutableProgram> Emit();
	};
}

#endif
