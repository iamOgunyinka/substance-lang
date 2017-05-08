/***************************************************************************
* Base class for all runtime classes
* 
* Copyright (c) 2017 Joshua Ogunyinka
* Copyright (c) 2013-2016 Randy Hollines
* All rights reserved.
*/

#ifndef __CLASS_H__
#define __CLASS_H__

#include "common.h"

/****************************
* Runtime support structures
****************************/
#define EXECUTION_STACK_SIZE 128
#define CALL_STACK_SIZE 64

/****************************
* Base class for built-in types
****************************/

using std::wstring;
using std::endl;
using std::wcout;
using std::wcerr;

class RuntimeClass {
	wstring name;
	std::unordered_map<wstring, Function> methods;

protected:
	RuntimeClass( const wstring &n ) {
		name = n;
	}

	virtual ~RuntimeClass() {
	}

	void AddFunction( const wstring &name, Function method ) {
		methods.insert( { name, method } );
	}

	static void PushValue( Value &value, Value* execution_stack, size_t &execution_stack_pos ) {
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

	Value &PopValue( Value* execution_stack, size_t &execution_stack_pos ) {
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

public:
	virtual Operation GetOperation( InstructionType oper ) = 0;

	const wstring GetName() {
		return name;
	}

	Function GetFunction( const wstring name ) {
		auto result = methods.find( name );
		if ( result != methods.end() ) {
			return result->second;
		}

		return NULL;
	}
};

/****************************
* Boolean class
****************************/
class BooleanClass : public RuntimeClass {
	static BooleanClass* instance;

	BooleanClass( const wstring &name ) : RuntimeClass( name ) {
	}

	~BooleanClass() {
	}

public:
	static BooleanClass* Instance() {
		if ( !instance ) {
			instance = new BooleanClass( L"Boolean" );
		}

		return instance;
	}

	virtual Operation GetOperation( InstructionType type ) {
		switch ( type ) {
		case EQL:
			return Equal;

		case NEQL:
			return NotEqual;

		default:
			return NULL;
		}
	}

	static void Equal( Value &left, Value &right, Value &result );
	static void NotEqual( Value &left, Value &right, Value &result );
};

/****************************
* Integer class
****************************/
class IntegerClass : public RuntimeClass {
	static IntegerClass* instance;

	IntegerClass( const wstring &name ) : RuntimeClass( name ) {
		AddFunction( L"abs:0", Abs );
	}

	~IntegerClass() {
	}

public:
	static IntegerClass* Instance() {
		if ( !instance ) {
			instance = new IntegerClass( L"Integer" );
		}

		return instance;
	}

	virtual Operation GetOperation( InstructionType type ) {
		switch ( type ) {
		case ADD:
			return Add;

		case SUB:
			return Subtract;

		case MUL:
			return Multiply;

		case DIV:
			return Divide;

		case EQL:
			return Equal;

		case NEQL:
			return NotEqual;

		case LES:
			return Less;

		case GTR:
			return Greater;

		case LES_EQL:
			return LessEqual;

		case GTR_EQL:
			return GreaterEqual;

		case MOD:
			return Modulo;

		default:
			return NULL;
		}
	}

	// operations
	static void Add( Value &left, Value &right, Value &result );
	static void Subtract( Value &left, Value &right, Value &result );
	static void Multiply( Value &left, Value &right, Value &result );
	static void Divide( Value &left, Value &right, Value &result );
	static void Modulo( Value &left, Value &right, Value &result );
	static void Equal( Value &left, Value &right, Value &result );
	static void NotEqual( Value &left, Value &right, Value &result );
	static void Less( Value &left, Value &right, Value &result );
	static void Greater( Value &left, Value &right, Value &result );
	static void LessEqual( Value &left, Value &right, Value &result );
	static void GreaterEqual( Value &left, Value &right, Value &result );

	// methods
	static void Abs( Value &self, Value* execution_stack, size_t &execution_stack_pos, INT_T arg_count );
};

/****************************
* Float class
****************************/
class FloatClass : public RuntimeClass {
	static FloatClass* instance;

	FloatClass( const wstring &name ) : RuntimeClass( name ) {
	}

	~FloatClass() {
	}

public:
	static FloatClass* Instance() {
		if ( !instance ) {
			instance = new FloatClass( L"Float" );
		}

		return instance;
	}

	virtual Operation GetOperation( InstructionType type ) {
		switch ( type ) {
		case ADD:
			return Add;

		case SUB:
			return Subtract;

		case MUL:
			return Multiply;

		case DIV:
			return Divide;

		case EQL:
			return Equal;

		case NEQL:
			return NotEqual;

		case LES:
			return Less;

		case GTR:
			return Greater;

		case LES_EQL:
			return LessEqual;

		case GTR_EQL:
			return GreaterEqual;

		case MOD:
			return Modulo;

		default:
			return NULL;
		}
	}

	// operations
	static void Add( Value &left, Value &right, Value &result );
	static void Subtract( Value &left, Value &right, Value &result );
	static void Multiply( Value &left, Value &right, Value &result );
	static void Divide( Value &left, Value &right, Value &result );
	static void Modulo( Value &left, Value &right, Value &result );
	static void Equal( Value &left, Value &right, Value &result );
	static void NotEqual( Value &left, Value &right, Value &result );
	static void Less( Value &left, Value &right, Value &result );
	static void Greater( Value &left, Value &right, Value &result );
	static void LessEqual( Value &left, Value &right, Value &result );
	static void GreaterEqual( Value &left, Value &right, Value &result );
	// methods
	static void ToInteger( Value &self, Value* execution_stack, size_t &execution_stack_pos, INT_T arg_count );
};

/****************************
* Array class
****************************/
class ArrayClass : public RuntimeClass {
	static ArrayClass* instance;

public:
	ArrayClass( const wstring &name ) : RuntimeClass( name ) {
	}

	~ArrayClass() {
	}

	static ArrayClass* Instance() {
		if ( !instance ) {
			instance = new ArrayClass( L"Array" );
		}

		return instance;
	}

	virtual Operation GetOperation( InstructionType oper ) {
		return NULL;
	}

	// methods
	static void New( Value &self, Value* execution_stack, size_t &execution_stack_pos, INT_T arg_count );
};

/****************************
* String class
****************************/
class StringClass : public RuntimeClass {
	static StringClass* instance;

public:
	StringClass( const wstring &name ) : RuntimeClass( name ) {
		AddFunction( L"size:0", Size );
	}

	~StringClass() {
	}

	static StringClass* Instance() {
		if ( !instance ) {
			instance = new StringClass( L"String" );
		}

		return instance;
	}

	virtual Operation GetOperation( InstructionType oper ) {
		switch ( oper ) {
		case ADD:
			return Add;

		default:
			return NULL;
		}
	}

	// methods
	static void Add( Value &left, Value &right, Value &result );
	static void Size( Value &self, Value* execution_stack, size_t &execution_stack_pos, INT_T arg_count );
};

#endif
