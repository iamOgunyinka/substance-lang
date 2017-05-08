/***************************************************************************
* Mark-and sweep garbage collector
* 
* Copyright (c) 2017 Joshua Ogunyinka 
* Copyright (c) 2013-2016 Randy Hollines
* All rights reserved.
*/

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "memory.h"
#include "runtime.h"

using namespace runtime;

using std::vector;
using std::list;

class Mark {
public:
	bool is_marked;
	size_t array_size;
	ExecutableClass* klass;

	Mark( ExecutableClass* k ) {
		is_marked = false;
		klass = k;
		array_size = 0;
	}

	Mark( size_t s ) {
		is_marked = false;
		array_size = s;
		klass = NULL;
	}

	~Mark() {
	}
};

class MemoryManager {
	static MemoryManager* instance;
	list<Value*> allocated;
	std::set<Value*> marked;

public:
	MemoryManager() {
	}

	~MemoryManager() {
	}



	static MemoryManager* Instance() {
		if ( !instance ) {
			instance = new MemoryManager;
		}

		return instance;
	}

	Value* AllocateString( Value* locals, const size_t local_size, Frame** call_stack, size_t call_stack_pos );
	Value* AllocateHash( Value* locals, const size_t local_size, Frame** call_stack, size_t call_stack_pos );
	Value* AllocateArray( INT_T array_size, vector<Value> &dimensions, Value* locals, const size_t local_size, Frame** call_stack, size_t call_stack_pos );
	Value* AllocateClass( ExecutableClass* klass, Value* locals, const size_t local_size, Frame** call_stack, size_t call_stack_pos );

	void MarkMemory( Value* locals, const size_t local_size, Frame** call_stack, size_t call_stack_pos );
	void MarkMemory( Value* values, RuntimeType type, int depth );

	void SweepMemory();
};

#endif
