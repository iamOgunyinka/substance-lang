/***************************************************************************
 * Mark-and sweep garbage collector
 * 
 * Copyright (c) 2017 Joshua Ogunyinka
 * Copyright (c) 2013-2016 Randy Hollines
 */

#include "memory.h"

MemoryManager* MemoryManager::instance;
using std::wcout;
using std::endl;

Value* MemoryManager::AllocateString( Value* locals, const size_t local_size, Frame** call_stack, size_t call_stack_pos )
{
	// type
	Value* values = new Value[ 2 ];
	values[ 0 ].type = META_TYPE;
	Mark* mark = new Mark( 1 );
	mark->is_marked = true;
	values[ 0 ].value.ptr_value = mark;
	++values;

	// set string
	values[ 0 ].type = STRING_TYPE;
	values[ 0 ].value.ptr_value = new std::wstring;

	allocated.push_back( values );

	/*
	MarkMemory(locals, local_size, call_stack, call_stack_pos);
	SweepMemory();
	*/

	return values;
}

Value* MemoryManager::AllocateHash( Value* locals, const size_t local_size, Frame** call_stack, size_t call_stack_pos )
{
	// type
	Value* values = new Value[ 2 ];
	values[ 0 ].type = META_TYPE;
	Mark* mark = new Mark( 1 );
	mark->is_marked = true;
	values[ 0 ].value.ptr_value = mark;
	++values;

	// TODO: set hash
	values[ 0 ].type = HASH_TYPE;
	values[ 0 ].value.ptr_value = NULL;

	allocated.push_back( values );

	/*
	MarkMemory(locals, local_size, call_stack, call_stack_pos);
	SweepMemory();
	*/

	return values;
}

Value* MemoryManager::AllocateClass( ExecutableClass* klass, Value* locals, const size_t local_size, Frame** call_stack, size_t call_stack_pos )
{
	// type
	Value* inst_values = new Value[ klass->GetInstanceCount() + 1 ];
	inst_values[ 0 ].type = META_TYPE;
	Mark* mark = new Mark( klass );
	mark->is_marked = true;
	inst_values[ 0 ].value.ptr_value = mark;
	++inst_values;

	const int inst_count = klass->GetInstanceCount();
	for ( int i = 0; i < inst_count; ++i ) {
		inst_values[ i ].type = UNINIT_TYPE;
	}

	allocated.push_back( inst_values );

	/*
	MarkMemory(locals, local_size, call_stack, call_stack_pos);
	SweepMemory();
	*/

	return inst_values;
}

Value* MemoryManager::AllocateArray( INT_T array_size, std::vector<Value> &dimensions, Value* locals,
	const size_t local_size, Frame** call_stack, size_t call_stack_pos )
{
	const int dimensions_size = static_cast< int >( dimensions.size() );
	const int meta_size = dimensions_size + 2;

	// type
	Value* array_values = new Value[ array_size + meta_size + 1 ];
	array_values[ 0 ].type = META_TYPE;
	array_values[ 0 ].value.int_value = static_cast< int >( array_size );

	// dimension
	array_values[ 1 ].value.int_value = dimensions_size;

	for ( int i = 0; i < dimensions_size; ++i ) {
		array_values[ i + 2 ] = dimensions[ i ];
	}
	array_values += meta_size;

	// mark record
	Mark* mark = new Mark( array_size );
	mark->is_marked = true;
	array_values[ 0 ].value.ptr_value = mark;
	++array_values;

	// initialize elements
	for ( int i = 0; i < array_size; ++i ) {
		array_values[ i ].type = UNINIT_TYPE;
	}

	allocated.push_back( array_values );

	MarkMemory( locals, local_size, call_stack, call_stack_pos );
	SweepMemory();

	return array_values;
}

void MemoryManager::MarkMemory( Value* global_locals, const size_t global_local_size, Frame** call_stack, size_t call_stack_pos )
{
#ifdef _DEBUG
	std::wcout << L"\n======================================" << std::endl;
	std::wcout << L"======== Start Marking Memory ========" << std::endl;
	std::wcout << L"======================================" << std::endl;

#endif
	for ( size_t i = 0; i < global_local_size; ++i ) {
		Value local = global_locals[ i ];
		switch ( local.type ) {
			// follow
		case CLS_TYPE:
		case ARRAY_TYPE:
		case STRING_TYPE:
		case HASH_TYPE:
			MarkMemory( static_cast< Value* >( local.value.ptr_value ), local.type, 0 );
			break;

		default:
			break;
		}
	}

	// stack
	while ( call_stack_pos-- ) {
		Frame* frame = call_stack[ call_stack_pos ];
#ifdef _DEBUG
		std::wcout << L"------------------------------------" << std::endl;
		std::wcout << L"Function: name='" << frame->function->GetName() << "'" << std::endl;
		std::wcout << L"------------------------------------" << std::endl;
#endif

		Value* fun_locals = frame->locals;
		const size_t fun_local_size = frame->local_size;
		for ( size_t i = 0; i < fun_local_size; ++i ) {
			Value local = fun_locals[ i ];
			switch ( local.type ) {
				// follow
			case CLS_TYPE:
			case ARRAY_TYPE:
			case STRING_TYPE:
			case HASH_TYPE:
				MarkMemory( static_cast< Value* >( local.value.ptr_value ), local.type, 0 );
				break;

			default:
				break;
			}
		}
	}

#ifdef _DEBUG
	wcout << L"marked: count=" << marked.size() << endl;
	wcout << L"======================================" << endl;
	wcout << L"========= End Marking Memory =========" << endl;
	wcout << L"======================================\n" << endl;
#endif
}

void MemoryManager::MarkMemory( Value* values, RuntimeType type, int depth )
{
	Mark* mark = static_cast< Mark* >( ( values - 1 )->value.ptr_value );
	if ( !mark->is_marked ) {
		// mark
		mark->is_marked = true;
		marked.insert( values );

		// determine type
		size_t value_size;

#ifdef _DEBUG
		for ( int i = 0; i < depth; ++i ) {
			wcout << " ";
		}
#endif

		switch ( type ) {
		case CLS_TYPE:
			value_size = mark->klass->GetInstanceCount();
#ifdef _DEBUG
			wcout << L"type=CLS_TYPE, size=" << value_size << L", address=" << values << endl;
#endif
			break;

		case STRING_TYPE:
			value_size = 1;
#ifdef _DEBUG
			wcout << L"type=STRING_TYPE, address=" << values << endl;
#endif
			break;

		case HASH_TYPE:
			value_size = 1;
#ifdef _DEBUG
			wcout << L"type=HASH_TYPE, address=" << values << endl;
#endif
			break;

		case ARRAY_TYPE:
			value_size = mark->array_size;
#ifdef _DEBUG
			wcout << L"type=ARRAY_TYPE, size=" << value_size << L", address=" << values << endl;
#endif
			break;
		}

		for ( size_t i = 0; i < value_size; ++i ) {
			Value local = values[ i ];
			switch ( local.type ) {
				// follow
			case CLS_TYPE:
			case ARRAY_TYPE:
				MarkMemory( static_cast< Value* >( local.value.ptr_value ), local.type, depth + 1 );
				break;

			default:
				break;
			}
		}
	}
}

void MemoryManager::SweepMemory()
{
	std::list<Value*>::iterator iter = allocated.begin();
	while ( iter != allocated.end() ) {
		Value* values = *iter;

		Mark* mark = static_cast< Mark* >( values[ -1 ].value.ptr_value );
		if ( mark->is_marked ) {
			mark->is_marked = false;
			// update
			++iter;
		}
		else {
			// find meta start
			while ( values->type != META_TYPE ) {
				--values;
			}
			// delete mark
			delete mark;
			mark = NULL;

			// delete string or hash
			if ( values->type == STRING_TYPE || values->type == HASH_TYPE ) {
				delete values->value.ptr_value;
				values->value.ptr_value = NULL;
			}

			// delete value array
			delete [] values;
			values = NULL;

			// update
			iter = allocated.erase( iter );
		}
	}
}
