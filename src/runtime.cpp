/***************************************************************************
 * Runtime system
 *
 * Copyright (c) 2017 Joshua Ogunyinka
 * Copyright (c) 2013-2016 Randy Hollines
 */

#include "runtime.h"
#include "memory.h"

using namespace runtime;

#define HIT_THRESHOLD 3

// delegates operation to the appropriate type class
#define CALC(oper, left, right) {                                       \
  left = PopValue();                                                    \
  if(left.sys_klass) {                                                  \
    right = PopValue();                                                 \
    Operation call = left.sys_klass->GetOperation(oper);	              \
    (*call)(left, right, left);						                              \
    PushValue(left);							                                      \
    }                                                                     \
    else if(left.user_klass) {                                            \
    ExecutableFunction* callee = left.user_klass->GetOperation(oper);	  \
    FunctionCall(callee, left, 1, true, ip, current_function, locals, local_size);  \
  }                                                                     \
    else {                                                                \
    wcerr << L">>> Invalid operation <<<" << endl;                      \
    exit(1);                                                            \
  }                                                                     \
}                                                                       \

/****************************
 * TODO: doc
 ****************************/
void Runtime::Run()
{
#ifdef _DEBUG
	wcout << L"========== Executing Code =========" << endl;
#endif

	// set current function
	ExecutableFunction* current_function = program->GetGlobal();

	// setup locals
	size_t local_size = program->GetGlobal()->GetLocalCount();
	Value* locals = new Value[ local_size + 1 ];
	for ( size_t i = 1; i < local_size; ++i ) {
		locals[ i ].type = UNINIT_TYPE;
	}

	// initialize 'self'
	locals[ 0 ].type = UNINIT_TYPE;

	// start execution
	Value left, right;
	size_t ip = 0;
	bool halt = false;
	do {
		Instruction* instruction = current_function->GetInstructions().at( ip++ );
		switch ( instruction->type ) {
		case RTRN: {
			if ( call_stack_pos == 0 ) {
				halt = true;
			}
			else {
				Frame* frame = PopFrame();
				// locals
				delete [] locals;
				locals = frame->locals;
				local_size = frame->local_size;

				// ip
				ip = frame->ip;
				current_function = frame->function;

				// clean up orphan return value
				if ( frame->orphan_return ) {
					PopValue();
				}

				delete frame;
				frame = NULL;
#ifdef _DEBUG
				wcout << L"=== RTRN ===" << endl;
#endif
			}
		}
				   break;

		case CALL_FUNC:
			FunctionCall( instruction, ip, current_function, locals, local_size );
			break;

		case LOAD_TRUE_LIT:
			left.type = BOOL_TYPE;
			left.sys_klass = BooleanClass::Instance();
			left.user_klass = NULL;
			left.value.int_value = 1;
#ifdef _DEBUG
			wcout << L"LOAD_TRUE_LIT: value=true" << endl;
#endif
			PushValue( left );
			break;

		case NEW_ARRAY:
			NewArray( instruction, ip, current_function, locals, local_size );
			break;

		case NEW_STRING:
			left.type = STRING_TYPE;
			left.value.ptr_value = MemoryManager::Instance()->AllocateString( locals, local_size, call_stack, call_stack_pos );
			left.sys_klass = StringClass::Instance();
#ifdef _DEBUG
			wcout << L"NEW_STRING: address=" << left.value.ptr_value << endl;
#endif
			PushValue( left );
			break;

		case NEW_HASH:
			left.type = HASH_TYPE;
			left.value.ptr_value = MemoryManager::Instance()->AllocateHash( locals, local_size, call_stack, call_stack_pos );
			// TODO:
			// left.sys_klass = HashClass::Instance();
#ifdef _DEBUG
			wcout << L"NEW_HASH: address=" << left.value.ptr_value << endl;
#endif
			PushValue( left );
			break;

		case NEW_OBJ: {
			ExecutableClass* user_klass = program->GetClass( instruction->operand5 );
			if ( user_klass ) {
				left.type = CLS_TYPE;
				left.user_klass = user_klass;
				left.sys_klass = NULL;
				// TODO: memory manager
				Value* inst_values = MemoryManager::Instance()->AllocateClass( user_klass, locals, local_size, call_stack, call_stack_pos );
				left.value.ptr_value = inst_values;
#ifdef _DEBUG
				wcout << L"NEW_OBJ: address=" << inst_values << endl;
#endif
				PushValue( left );
			}
			else {
				wcerr << L">>> Undefiend class: name='" << instruction->operand5 << "' <<<" << endl;
				exit( 1 );
			}
		}
					  break;

		case LOAD_FALSE_LIT:
			left.type = BOOL_TYPE;
			left.sys_klass = BooleanClass::Instance();
			left.user_klass = NULL;
			left.value.int_value = 0;
#ifdef _DEBUG
			wcout << L"LOAD_FALSE_LIT: value=false" << endl;
#endif
			PushValue( left );
			break;

		case LOAD_INT_LIT:
			left.type = INT_TYPE;
			left.sys_klass = IntegerClass::Instance();
			left.user_klass = NULL;
			left.value.int_value = instruction->operand1;
#ifdef _DEBUG
			wcout << L"LOAD_INT_LIT: value=" << left.value.int_value << endl;
#endif
			PushValue( left );
			break;

		case LOAD_FLOAT_LIT:
			left.type = FLOAT_TYPE;
			left.sys_klass = FloatClass::Instance();
			left.user_klass = NULL;
			left.value.float_value = instruction->operand4;
#ifdef _DEBUG
			wcout << L"LOAD_FLOAT_LIT: value=" << left.value.float_value << endl;
#endif
			PushValue( left );
			break;

		case LOAD_VAR:
#ifdef _DEBUG
			wcout << L"LOAD_VAR: id=" << instruction->operand2 << endl;
#endif
			if ( instruction->operand1 == LOCL ) {
				left = locals[ instruction->operand2 ];
			}
			else {
				Value* instance = static_cast< Value* >( locals[ 0 ].value.ptr_value );
				left = instance[ instruction->operand2 ];
			}

			if ( !left.sys_klass && !left.user_klass ) {
				wcerr << L">>> Unknown variable type <<<" << endl;
				exit( 1 );
			}
			PushValue( left );
			break;

		case STOR_VAR:
#ifdef _DEBUG
			wcout << L"STOR_VAR: id=" << instruction->operand2 << L", local="
				<< ( instruction->operand1 == LOCL ? L"true" : L"false" ) << endl;
#endif
			left = PopValue();
			if ( instruction->operand1 == LOCL ) {
				locals[ instruction->operand2 ] = left;
			}
			else {
				Value* instance = static_cast< Value* >( locals[ 0 ].value.ptr_value );
				instance[ instruction->operand2 ] = left;
			}
			break;

		case LOAD_ARY_VAR: {
			if ( instruction->operand1 == LOCL ) {
				left = locals[ instruction->operand2 ];
			}
			else {
				Value* instance = static_cast< Value* >( locals[ 0 ].value.ptr_value );
				left = instance[ instruction->operand2 ];
			}

			if ( left.type != ARRAY_TYPE ) {
				wcerr << L">>> Operation requires Integer or Float type <<<" << endl;
			}

			Value* array = ( Value* ) left.value.ptr_value;
			const INT_T index = ArrayIndex( instruction, array, false );
#ifdef _DEBUG
			wcout << L"LOAD_ARY_VAR: id=" << instruction->operand2 << L", offset=" << index
				<< L", local=" << ( instruction->operand1 == LOCL ? L"true" : L"false" ) << endl;
#endif
			PushValue( array[ index ] );
		}
						   break;

		case STOR_ARY_VAR: {
			if ( instruction->operand1 == LOCL ) {
				left = locals[ instruction->operand2 ];
			}
			else {
				Value* instance = static_cast< Value* >( locals[ 0 ].value.ptr_value );
				left = instance[ instruction->operand2 ];
			}

			if ( left.type != ARRAY_TYPE ) {
				wcerr << L">>> Operation requires array type <<<" << endl;
				exit( 1 );
			}

			Value* array = ( Value* ) left.value.ptr_value;
			const INT_T index = ArrayIndex( instruction, array, true );
#ifdef _DEBUG
			wcout << L"STOR_ARY_VAR: id=" << instruction->operand2 << L", offset=" << index
				<< L", local=" << ( instruction->operand1 == LOCL ? L"true" : L"false" ) << endl;
#endif
			array[ index ] = PopValue();
		}
						   break;

						   // TODO: implement
		case ARY_SIZE:
#ifdef _DEBUG
			wcout << L"ARY_SIZE" << endl;
#endif
			break;

			// TODO: implement
		case LOAD_CLS:
#ifdef _DEBUG
			wcout << L"LOAD_CLS" << endl;
#endif
			break;

		case LBL:
#ifdef _DEBUG
			wcout << L"LBL: id=" << instruction->operand1 << L", hit_count=" << instruction->operand2 << endl;
#endif
			break;

		case JMP:
			size_t jmp_ip;
			switch ( instruction->operand2 ) {
				// unconditional jump
			case JMP_UNCND:
#ifdef _DEBUG
				wcout << L"JMP: unconditional, to=" << instruction->operand1 << endl;
#endif
				jmp_ip = GetLabelOffset( current_function, instruction->operand1 );
				ip = jmp_ip;
				break;

				// jump true
			case JMP_TRUE: {
#ifdef _DEBUG
				wcout << L"JMP: true, to=" << instruction->operand1 << endl;
#endif
				left = PopValue();
				if ( left.type != BOOL_TYPE ) {
					wcerr << L">>> Expected a boolean value <<<" << endl;
					exit( 1 );
				}
				// update ip
				if ( left.value.int_value ) {
					jmp_ip = GetLabelOffset( current_function, instruction->operand1 );
					if ( jmp_ip < ip ) {
						Instruction* label = current_function->GetInstructions().at( jmp_ip );
						label->operand2++;
					}
					ip = jmp_ip;
				}
			}
						   break;

						   // jump false
			case JMP_FALSE: {
#ifdef _DEBUG
				wcout << L"JMP: false, to=" << instruction->operand1 << endl;
#endif
				left = PopValue();
				if ( left.type != BOOL_TYPE ) {
					wcerr << L">>> Expected a boolean value <<<" << endl;
					exit( 1 );
				}
				// update ip
				if ( !left.value.int_value ) {
					jmp_ip = GetLabelOffset( current_function, instruction->operand1 );
					if ( jmp_ip < ip ) {
						Instruction* label = current_function->GetInstructions().at( jmp_ip );
						label->operand2++;
					}
					ip = jmp_ip;
				}
			}
							break;
			}
			break;

		case BIT_AND:
			break;

		case BIT_OR:
			break;

		case EQL:
#ifdef _DEBUG
			wcout << L"EQL" << endl;
#endif
			CALC( EQL, left, right );
			break;

		case NEQL:
#ifdef _DEBUG
			wcout << L"NEQL" << endl;
#endif
			CALC( NEQL, left, right );
			break;

		case GTR:
#ifdef _DEBUG
			wcout << L"GTR" << endl;
#endif
			CALC( GTR, left, right );
			break;

		case LES:
#ifdef _DEBUG
			wcout << L"LES" << endl;
#endif
			CALC( LES, left, right );
			break;

		case GTR_EQL:
#ifdef _DEBUG
			wcout << L"GTR_EQL" << endl;
#endif
			CALC( GTR_EQL, left, right );
			break;

		case LES_EQL:
#ifdef _DEBUG
			wcout << L"LES_EQL" << endl;
#endif
			CALC( LES_EQL, left, right );
			break;

		case ADD:
#ifdef _DEBUG
			wcout << L"ADD" << endl;
#endif
			CALC( ADD, left, right );
			break;

		case SUB:
#ifdef _DEBUG
			wcout << L"SUB" << endl;
#endif
			CALC( SUB, left, right );
			break;

		case MUL:
#ifdef _DEBUG
			wcout << L"MUL" << endl;
#endif
			CALC( MUL, left, right );
			break;

		case DIV:
#ifdef _DEBUG
			wcout << L"DIV" << endl;
#endif
			CALC( DIV, left, right );
			break;

		case MOD:
#ifdef _DEBUG
			wcout << L"MOD" << endl;
#endif
			CALC( MOD, left, right );
			break;

		case SHOW_TYPE:
#ifdef _DEBUG
			wcout << L"SHOW" << endl;
#endif
			left = PopValue();

			switch ( left.type ) {
			case BOOL_TYPE:
				wcout << L"type=boolean, value=" << ( left.value.int_value ? L"true" : L"false" ) << endl;
				break;

			case INT_TYPE:
				wcout << L"type=integer, value=" << left.value.int_value << endl;
				break;

			case FLOAT_TYPE:
				wcout << L"type=float, value=" << left.value.float_value << endl;
				break;

			case UNINIT_TYPE:
				wcout << L"type=uninit, value=Nil" << endl;
				break;

			default:
				wcerr << L"Invalid dump value" << endl;
				exit( 1 );
			}
			break;

		case NO_OP:
			break;
		}
	} while ( !halt );

	delete [] locals;
	locals = NULL;

#ifdef _DEBUG
	wcout << L"==========================" << endl;
	wcout << L"ending stack pos=" << execution_stack_pos << endl;
#endif
}

void Runtime::NewArray( Instruction* instruction, size_t &ip, ExecutableFunction* &current_function, Value* &locals, size_t &local_size )
{
	FLOAT_T array_size = 0;
	vector<Value> dimensions;

	// calculate array size
	Value left = PopValue();
	dimensions.push_back( left );
	if ( left.type == INT_TYPE ) {
		array_size = static_cast< FLOAT_T >( left.value.int_value );
	}
	else if ( left.type == FLOAT_TYPE ) {
		array_size = left.value.float_value;
	}
	else {
		wcerr << L">>> Array dimension size must be a numeric value <<<" << endl;
		exit( 1 );
	}

	int count = static_cast< int >( instruction->operand1 );
	while ( --count ) {
		left = PopValue();
		dimensions.push_back( left );

		if ( left.type == INT_TYPE ) {
			array_size *= static_cast< FLOAT_T >( left.value.int_value );
		}
		else if ( left.type == FLOAT_TYPE ) {
			array_size *= left.value.float_value;
		}
		else {
			wcerr << L">>> Array dimension size must be a numeric value <<<" << endl;
			exit( 1 );
		}
	}

	// create array and set metadata
	Value* array_values = MemoryManager::Instance()->AllocateArray( static_cast< INT_T >( array_size ), dimensions, locals, local_size, call_stack, call_stack_pos );
	left.type = ARRAY_TYPE;
	left.sys_klass = ArrayClass::Instance();
	left.value.ptr_value = array_values;
#ifdef _DEBUG
	wcout << L"NEW_ARRAY: size=" << array_size << L", address=" << array_values << endl;
#endif
	PushValue( left );
}

void Runtime::FunctionCall( Instruction* instruction, size_t &ip, ExecutableFunction* &current_function, Value* &locals, size_t &local_size )
{
	Value left = PopValue();

	if ( instruction->operand6.size() == 0 && !left.user_klass ) {
		ExecutableFunction* callee = program->GetFunction( instruction->operand5 );
		if ( callee ) {
#ifdef _DEBUG
			wcout << L"=== CALL_FUNC: function='" << instruction->operand5 << L"' ===" << endl;
#endif
			FunctionCall( callee, left, instruction->operand1, instruction->operand2 != 0, ip, current_function, locals, local_size );
		}
		else {
#ifdef _DEBUG
			wcout << L"=== CALL_FUNC: class='" << left.sys_klass->GetName() << L"', method='" << instruction->operand5 << L"'" << endl;
#endif
			Function function = left.sys_klass->GetFunction( instruction->operand5 );
			if ( !function ) {
				wcerr << L">>> Uninitialized function reference <<<" << endl;
				exit( 1 );
			}
			function( left, execution_stack.get(), execution_stack_pos, instruction->operand1 );
		}
	}
	else if ( left.type == CLS_TYPE ) {
#ifdef _DEBUG
		wcout << L"=== CALL_FUNC: class='" << left.user_klass->GetName() << L"', method='" << instruction->operand5 << L"'" << endl;
#endif
		ExecutableFunction* callee = left.user_klass->GetFunction( instruction->operand5 );
		FunctionCall( callee, left, instruction->operand1, instruction->operand2 != 0, ip, current_function, locals, local_size );
	}
	else {
		wcerr << L">>> Uninitialized function reference <<<" << endl;
		exit( 1 );
	}
}

void Runtime::FunctionCall( ExecutableFunction* callee, Value &left, long param_count,
	bool has_return, size_t &ip, ExecutableFunction* &current_function, Value* &locals, size_t &local_size )
{
	if ( !callee ) {
		wcerr << L">>> Unknown function <<<" << endl;
		exit( 1 );
	}

	if ( callee->GetParameterCount() != param_count ) {
		wcerr << L">>> Incorrect number of calling parameters <<<" << endl;
		exit( 1 );
	}

	// push stack frame
	Frame* frame = new Frame;
	frame->ip = ip;
	frame->function = current_function;
	frame->locals = locals;
	frame->local_size = local_size;

	// function returns an orphan value
	if ( callee->ReturnsValue() && !has_return ) {
		frame->orphan_return = true;
	}
	else {
		frame->orphan_return = false;
	}
	PushFrame( frame );

	current_function = callee;
	const size_t size = current_function->GetLocalCount() + 1;
	locals = new Value[ size ];
	local_size = size;
	locals[ 0 ].type = left.type;
	locals[ 0 ].user_klass = left.user_klass;
	locals[ 0 ].value.ptr_value = left.value.ptr_value;
	ip = 0;
}
