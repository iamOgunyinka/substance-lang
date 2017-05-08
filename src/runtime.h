/***************************************************************************
 * Runtime system
 *
 * Copyright (c) 2017 Joshua Ogunyinka
 * All rights reserved.
 */

#ifndef __RUNTIME_H__
#define __RUNTIME_H__

#include <memory>
#include "classes.h"

namespace runtime {
	/****************************
	 * Call stack frame
	 ****************************/
	typedef struct _Frame {
		ExecutableFunction* function;
		size_t ip;
		Value* locals;
		size_t local_size;
		bool orphan_return;
	} Frame;

	/****************************
	 * Execution engine
	 ****************************/
	class Runtime {
		std::unique_ptr<ExecutableProgram> program;
		INT_T last_label_id;
		// execution stack and stack pointer
		std::unique_ptr<Value[]> execution_stack;
		size_t execution_stack_pos;
		// call stack
		Frame** call_stack;
		size_t call_stack_pos;

		//
		// Calculation stack operations
		//
		Value TopValue() {
			return execution_stack[ execution_stack_pos ];
		}

		void PushValue( Value &value ) {
			if ( execution_stack_pos >= EXECUTION_STACK_SIZE ) {
				wcerr << ">>> stack bounds exceeded <<<" << endl;
				exit( 1 );
			}

#ifdef _DEBUG
			wcout << L"  push: type=";
			switch ( value.type ) {
			case BOOL_TYPE:
				wcout << L"boolean; value=" << ( value.value.int_value ? L"true" : L"false" ) << L"; stack_pos=" << execution_stack_pos << endl;
				break;

			case INT_TYPE:
				wcout << L"integer; value=" << value.value.int_value << L"; stack_pos=" << execution_stack_pos << endl;
				break;

			case FLOAT_TYPE:
				wcout << L"float; value=" << value.value.float_value << L"; stack_pos=" << execution_stack_pos << endl;
				break;

			case CLS_TYPE:
				wcout << L"system object; address=" << value.value.ptr_value << L"; stack_pos=" << execution_stack_pos << endl;
				break;

			case ARRAY_TYPE:
				wcout << L"array; address=" << value.value.ptr_value << "; stack_pos=" << execution_stack_pos << endl;
				break;

			case STRING_TYPE:
				wcout << L"string; address=" << value.value.ptr_value << "; stack_pos=" << execution_stack_pos << endl;
				break;

			case UNINIT_TYPE:
				wcout << L"uninitialized" << endl;
				break;

				// TODO:
			default:
				break;
			}
#endif

			execution_stack[ execution_stack_pos++ ] = value;
		}

		Value &PopValue() {
			if ( execution_stack_pos == 0 ) {
				wcerr << ">>> stack bounds exceeded <<<" << endl;
				exit( 1 );
			}

#ifdef _DEBUG
			Value &value = execution_stack[ execution_stack_pos - 1 ];
			wcout << L"  pop: type=";
			switch ( value.type ) {
			case BOOL_TYPE:
				wcout << L"boolean; value=" << ( value.value.int_value ? L"true" : L"false" ) << L"; stack_pos=" << ( execution_stack_pos - 1 ) << endl;
				break;

			case INT_TYPE:
				wcout << L"integer; value=" << value.value.int_value << L"; stack_pos=" << ( execution_stack_pos - 1 ) << endl;
				break;

			case FLOAT_TYPE:
				wcout << L"float; value=" << value.value.float_value << L"; stack_pos=" << ( execution_stack_pos - 1 ) << endl;
				break;

			case CLS_TYPE:
				wcout << L"system object; address=" << value.value.ptr_value << L"; stack_pos=" << ( execution_stack_pos - 1 ) << endl;
				break;

			case ARRAY_TYPE:
				wcout << L"array; address=" << value.value.ptr_value << L"; stack_pos=" << ( execution_stack_pos - 1 ) << endl;
				break;

			case STRING_TYPE:
				wcout << L"string; address=" << value.value.ptr_value << L"; stack_pos=" << ( execution_stack_pos - 1 ) << endl;
				break;

			case UNINIT_TYPE:
				wcout << L"uninitialized" << endl;
				break;

				// TODO:
			default:
				break;
			}
#endif

			return execution_stack[ --execution_stack_pos ];
		}

		//
		// Calculate array offset
		//
		// TODO: bounds check each dimension
		inline INT_T ArrayIndex( Instruction* instruction, Value* array, bool is_store ) {
			INT_T index;
			Value value = PopValue();
			switch ( value.type ) {
			case INT_TYPE:
				index = value.value.int_value;
				break;

			case FLOAT_TYPE:
				index = ( INT_T ) value.value.float_value;
				break;

			default:
				wcerr << L">>> Operation requires a numeric value <<<" << endl;
				exit( 1 );
			}

			// check dimensions
			const int dimensions = static_cast< int >( instruction->operand3 );
			const int meta_offset = -( dimensions + 2 + 1 );

			if ( array[ meta_offset + 1 ].value.int_value != dimensions ) {
				wcerr << L">>> Mismatch array dimensions <<<" << endl;
				exit( 1 );
			}

			// TODO: encode array with bounds
			for ( int i = 1; i < dimensions; i++ ) {
				index *= array[ meta_offset + 2 + i ].value.int_value;
				Value value = PopValue();
				switch ( value.type ) {
				case INT_TYPE:
					index += value.value.int_value;
					break;

				case FLOAT_TYPE:
					index += ( INT_T ) value.value.float_value;
					break;

				default:
					wcerr << L">>> Operation requires a numeric value <<<" << endl;
					exit( 1 );
				}
			}

			if ( index >= array[ meta_offset ].value.int_value ) {
				wcerr << L">>> Array index out-of-bounds: index=" << index << L", max_bounds=" << array[ meta_offset ].value.int_value << L" <<<" << endl;
				exit( 1 );
			}

			return index;
		}

		//
		// Stack frame operations
		//
		void PushFrame( Frame* frame ) {
#ifdef _DEBUG
			wcout << L"pushing frame: address=" << frame << endl;
			assert( call_stack_pos < CALL_STACK_SIZE );
#endif
			call_stack[ call_stack_pos++ ] = frame;
		}

		Frame* PopFrame() {
#ifdef _DEBUG
			wcout << L"popping frame: address=" << call_stack[ call_stack_pos - 1 ] << endl;
			assert( call_stack_pos - 1 >= 0 );
#endif
			return call_stack[ --call_stack_pos ];
		}

#ifdef _DEBUG
		void DumpValue( Value* value, bool is_push ) {
			if ( is_push ) {
				wcout << L"  push: ";
			}
			else {
				wcout << L"  pop: ";
			}

			switch ( value->type ) {
			case BOOL_TYPE:
				wcout << L"boolean=" << ( value->value.int_value ? L"true" : L"false" ) << endl;
				break;

			case INT_TYPE:
				wcout << L"integer=" << value->value.int_value << endl;
				break;

			case FLOAT_TYPE:
				wcout << L"float=" << value->value.float_value << endl;
				break;

			case ARRAY_TYPE:
				wcout << L"array: address=" << value->value.ptr_value << endl;
				break;

				// TODO:
			default:
				break;
			}
		}
#endif

		inline size_t GetLabelOffset( ExecutableFunction* current_function, INT_T label ) {
			auto result = current_function->GetJumpTable().find( label );
			if ( result == current_function->GetJumpTable().end() ) {
				wcerr << ">>> Invalid label identifier <<<" << endl;
				exit( 1 );
			}

			return result->second;
		}

		// member operations
		inline void NewArray( Instruction* instruction, size_t &ip, ExecutableFunction* &current_function, Value* &locals, size_t &local_size );
		inline void FunctionCall( Instruction* instruction, size_t &ip, ExecutableFunction* &current_function, Value* &locals, size_t &local_size );
		inline void FunctionCall( ExecutableFunction* callee, Value &left, long param_count, bool has_return,
			size_t &ip, ExecutableFunction* &current_function, Value* &locals, size_t &local_size );

	public:
		Runtime( std::unique_ptr<ExecutableProgram> p, INT_T last_label_id ): program( std::move( p ) ) {
			this->last_label_id = last_label_id;
			// execution stack
			execution_stack.reset( new Value[ EXECUTION_STACK_SIZE ] );
			execution_stack_pos = 0;
			// call stack
			call_stack = new Frame*[ CALL_STACK_SIZE ];
			call_stack_pos = 0;
		}

		~Runtime() = default;
		void Run();
	};
}

#endif
