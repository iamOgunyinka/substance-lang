/***************************************************************************
* Language model
*
* Copyright (c) 2017 Joshua Ogunyinka
* Copyright (c) 2013-2016, Randy Hollines
* All rights reserved.
*/

#ifndef __COMMON_H__
#define __COMMON_H__

// #include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stack>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <string>
#include <string.h>
#include <unordered_map>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <unordered_set>
using namespace stdext;
#elif _OSX
#include <pthread.h>
#include <stdint.h>
#else
#include <pthread.h>
#include <stdint.h>
#include <dlfcn.h>
#endif

class RuntimeClass;
class ExecutableClass;
struct _Value;

// basic datatypes
#define INT_T long
#define FLOAT_T double
#define CHAR_T wchar_t
#define BYTE_T char

// jump operands
#define JMP_TRUE 1
#define JMP_FALSE 0
#define JMP_UNCND -1

/****************************
* Utility functions
****************************/
inline std::wstring IntToString( int v ) {
	std::wostringstream str;
	str << v;
	return str.str();
}

/****************************
* Dynamic runtime instructions
****************************/
enum InstructionType {
	// literals
	LOAD_TRUE_LIT = -256,
	LOAD_FALSE_LIT,
	LOAD_INT_LIT,
	LOAD_FLOAT_LIT,
	// variables
	LOAD_VAR,
	LOAD_CLS,
	STOR_VAR,
	// logical operations
	EQL,
	NEQL,
	GTR,
	LES,
	GTR_EQL,
	LES_EQL,
	// mathematical operations
	ADD,
	SUB,
	MUL,
	DIV,
	MOD,
	// bitwise operations
	BIT_AND,
	BIT_OR,
	// conditionals
	JMP,
	LBL,
	// arrays
	NEW_ARRAY,
	NEW_STRING,
	NEW_HASH,
	STOR_ARY_VAR,
	LOAD_ARY_VAR,
	ARY_SIZE,
	// objects
	NEW_OBJ,
	// functions
	CALL_FUNC,
	RTRN,
	// misc
	SHOW_TYPE,
	NO_OP
};

enum VScope {
	LOCL = -512,
	INST,
	CLS
};

typedef struct _Instruction {
	InstructionType type;
	INT_T operand1;
	INT_T operand2;
	INT_T operand3;
	FLOAT_T operand4;
	std::wstring operand5;
	std::wstring operand6;
} Instruction;

/****************************
* Runtime types and values
****************************/
enum RuntimeType {
	UNINIT_TYPE = -50, // uinitialized type
	META_TYPE,
	// complex
	CLS_TYPE, // class type
	ARRAY_TYPE,
	STRING_TYPE,
	HASH_TYPE,
	// basic
	FLOAT_TYPE,
	BOOL_TYPE,
	INT_TYPE,
	CHAR_TYPE
};

/****************************
 * 'Abstract' value type
 ****************************/
class Value {
public:
	Value() {
		sys_klass = NULL;
		user_klass = NULL;
	}

	Value( RuntimeType t ) {
		type = t;
		sys_klass = NULL;
		user_klass = NULL;
	}

	RuntimeType type;
	RuntimeClass* sys_klass;
	ExecutableClass* user_klass;

	union _value {
		BYTE_T 	byte_value;
		CHAR_T 	char_value;
		INT_T 	int_value;
		FLOAT_T float_value;
		void* 	ptr_value;
	} value;
};

/****************************
 * Runtime function
 ****************************/
class ExecutableFunction {
	std::wstring name;
	InstructionType operation;
	int local_count;
	int parameter_count;
	std::vector<Instruction*> block_instructions;
	std::unordered_map<long, size_t> jump_table;
	bool returns_value;
	std::set<size_t> leaders;

public:
	ExecutableFunction( const std::wstring &name, InstructionType operation, int local_count, int parameter_count,
		std::vector<Instruction*> && block_instructions, std::unordered_map<long, size_t> && jump_table,
		std::set<size_t> &leaders, bool returns_value ) {
		this->name = name;
		this->operation = operation;
		this->parameter_count = parameter_count;
		this->local_count = local_count;
		this->block_instructions = std::move( block_instructions );
		this->jump_table = std::move( jump_table );
		this->leaders = leaders;
		this->returns_value = returns_value;
	}

	~ExecutableFunction() = default;

	inline const std::wstring GetName() {
		return name;
	}

	inline const InstructionType GetOperation() {
		return operation;
	}

	inline bool IsOperation() {
		return operation != NO_OP;
	}

	inline int GetParameterCount() {
		return parameter_count;
	}

	inline int GetLocalCount() {
		return local_count;
	}

	inline bool ReturnsValue() {
		return returns_value;
	}

	inline std::vector<Instruction*>& GetInstructions() {
		return block_instructions;
	}

	inline std::unordered_map<long, size_t>& GetJumpTable() {
		return jump_table;
	}
};

typedef void( *Operation )( Value &left, Value &right, Value &result );
typedef void( *Function )( Value &self, Value* execution_stack, size_t &execution_stack_pos, INT_T arg_count );

/****************************
 * Runtime class
 ****************************/
class ExecutableClass {
	std::wstring name;
	std::unordered_map<std::wstring, ExecutableFunction*> functions;
	std::unordered_map<long, ExecutableFunction*> operations;
	int inst_count;

public:
	ExecutableClass( const std::wstring name, int inst_count ) {
		this->name = name;
		this->inst_count = inst_count;
	}

	~ExecutableClass() {
		for ( auto & iter: functions ) {
			ExecutableFunction* tmp = iter.second;
			delete tmp;
			tmp = nullptr;
		}
		functions.clear();
	}

	const std::wstring GetName() {
		return name;
	}

	const int GetInstanceCount() {
		return inst_count;
	}

	void AddFunction( ExecutableFunction* function ) {
		if ( function->IsOperation() ) {
			operations.insert( { function->GetOperation(), function } );
		}
		else {
			std::wstring function_name = function->GetName();
			function_name += L':';
			function_name += IntToString( function->GetParameterCount() );
			functions.insert( { function_name, function } );
		}
	}

	ExecutableFunction* GetFunction( const std::wstring &name ) {
		auto const result = functions.find( name );
		if ( result != functions.cend() ) {
			return result->second;
		}

		return nullptr;
	}

	ExecutableFunction* GetOperation( InstructionType oper ) {
		auto const result = operations.find( oper );
		if ( result != operations.cend() ) {
			return result->second;
		}

		return nullptr;
	}
};

/****************************
* Holder for runtime program
****************************/
class ExecutableProgram {
	ExecutableFunction* main_function;
	std::unordered_map<std::wstring, ExecutableFunction*> functions;
	std::unordered_map<std::wstring, ExecutableClass*> classes;

public:
	ExecutableProgram() {
	}

	~ExecutableProgram() {
		if ( main_function ) {
			delete main_function;
			main_function = nullptr;
		}

		for ( auto & func_iter : functions ) {
			ExecutableFunction* tmp = func_iter.second;
			delete tmp;
			tmp = nullptr;
		}
	}

	void SetMain( ExecutableFunction* main_function ) {
		this->main_function = main_function;
	}

	ExecutableFunction* GetGlobal() {
		return main_function;
	}

	void AddClass( ExecutableClass* cls ) {
		classes.insert( { cls->GetName(), cls } );
	}

	ExecutableClass* GetClass( const std::wstring &name ) {
		auto result = classes.find( name );
		if ( result != classes.end() ) {
			return result->second;
		}

		return nullptr;
	}

	void AddFunction( ExecutableFunction* function ) {
		std::wstring function_name = function->GetName();
		function_name += L':';
		function_name += IntToString( function->GetParameterCount() );
		functions.insert( { function_name, function } );
	}

	ExecutableFunction* GetFunction( const std::wstring &name ) {
		auto result = functions.find( name );
		if ( result != functions.end() ) {
			return result->second;
		}

		return nullptr;
	}
};

/****************************
* Converts a UTF-8 bytes to
* native a unicode string
****************************/
static bool BytesToUnicode( const std::string &in, std::wstring &out ) {
#ifdef _WIN32
	// allocate space
	int wsize = MultiByteToWideChar( CP_UTF8, 0, in.c_str(), -1, NULL, 0 );
	if ( !wsize ) {
		return false;
	}
	wchar_t* buffer = new wchar_t[ wsize ];

	// convert
	int check = MultiByteToWideChar( CP_UTF8, 0, in.c_str(), -1, buffer, wsize );
	if ( !check ) {
		delete [] buffer;
		buffer = NULL;
		return false;
	}

	// create string
	out.append( buffer, wsize - 1 );

	// clean up
	delete [] buffer;
	buffer = NULL;
#else
	// allocate space
	size_t size = mbstowcs(NULL, in.c_str(), in.size());
	if(size == (size_t)-1) {
		return false;
	}
	wchar_t* buffer = new wchar_t[size + 1];

	// convert
	size_t check = mbstowcs(buffer, in.c_str(), in.size());
	if(check == (size_t)-1) {
		delete[] buffer;
		buffer = NULL;
		return false;
	}
	buffer[size] = L'\0';

	// create string
	out.append(buffer, size);

	// clean up
	delete[] buffer;
	buffer = NULL;
#endif

	return true;
}

static std::wstring BytesToUnicode( const std::string &in ) {
	std::wstring out;
	if ( BytesToUnicode( in, out ) ) {
		return out;
	}

	return L"";
}

/****************************
* Converts UTF-8 bytes to
* native a unicode character
****************************/
static bool BytesToCharacter( const std::string &in, wchar_t &out ) {
	std::wstring buffer;
	if ( !BytesToUnicode( in, buffer ) ) {
		return false;
	}

	if ( buffer.size() != 1 ) {
		return false;
	}

	out = buffer[ 0 ];
	return true;
}

/****************************
* Converts a native string
* to UTF-8 bytes
****************************/
static bool UnicodeToBytes( const std::wstring &in, std::string &out ) {
#ifdef _WIN32
	// allocate space
	int size = WideCharToMultiByte( CP_UTF8, 0, in.c_str(), -1, NULL, 0, NULL, NULL );
	if ( !size ) {
		return false;
	}
	char* buffer = new char[ size ];

	// convert std::string
	int check = WideCharToMultiByte( CP_UTF8, 0, in.c_str(), -1, buffer, size, NULL, NULL );
	if ( !check ) {
		delete [] buffer;
		buffer = NULL;
		return false;
	}

	// append output
	out.append( buffer, size - 1 );

	// clean up
	delete [] buffer;
	buffer = NULL;
#else
	// convert std::string
	size_t size = wcstombs(NULL, in.c_str(), in.size());
	if(size == (size_t)-1) {
		return false;
	}
	char* buffer = new char[size + 1];

	wcstombs(buffer, in.c_str(), size);
	if(size == (size_t)-1) {
		delete[] buffer;
		buffer = NULL;
		return false;
	}
	buffer[size] = '\0';

	// create std::string      
	out.append(buffer, size);

	// clean up
	delete[] buffer;
	buffer = NULL;
#endif

	return true;
}

static std::string UnicodeToBytes( const std::wstring &in ) {
	std::string out;
	if ( UnicodeToBytes( in, out ) ) {
		return out;
	}

	return "";
}

/****************************
* Converts a native character
* to UTF-8 bytes
****************************/
static bool CharacterToBytes( wchar_t in, std::string &out ) {
	if ( in == L'\0' ) {
		return true;
	}

	wchar_t buffer[ 2 ];
	buffer[ 0 ] = in;
	buffer[ 1 ] = L'\0';

	if ( !UnicodeToBytes( buffer, out ) ) {
		return false;
	}

	return true;
}

/****************************
* Loads a UTF-8 file into memory
* and converts content into native
* Unicode format
****************************/
static wchar_t* LoadFileBuffer( const std::wstring &name, size_t& buffer_size ) {
	char* buffer;
	std::string open_name( name.begin(), name.end() );

	std::ifstream in{ open_name, std::ios_base::in | std::ios_base::binary | std::ios_base::ate };
	if ( in ) {
		// get file size
		in.seekg( 0, std::ios::end );
		buffer_size = ( size_t ) in.tellg();
		in.seekg( 0, std::ios::beg );
		buffer = ( char* ) calloc( buffer_size + 1, sizeof( char ) );
		in.read( buffer, buffer_size );
		// close file
		in.close();
	}
	else {
		std::wcerr << L"Unable to open source file: " << name << std::endl;
		exit( 1 );
	}

	// convert unicode
#ifdef _WIN32
	int wsize = MultiByteToWideChar( CP_UTF8, 0, buffer, -1, NULL, 0 );
	if ( !wsize ) {
		std::wcerr << L"Unable to open source file: " << name << std::endl;
		exit( 1 );
	}
	wchar_t* wbuffer = new wchar_t[ wsize ];
	int check = MultiByteToWideChar( CP_UTF8, 0, buffer, -1, wbuffer, wsize );
	if ( !check ) {
		std::wcerr << L"Unable to open source file: " << name << std::endl;
		exit( 1 );
	}
#else
	size_t wsize = mbstowcs(NULL, buffer, buffer_size);
	if(wsize == (size_t)-1) {
		delete buffer;
		wcerr << L"Unable to open source file: " << name << endl;
		exit(1);
	}
	wchar_t* wbuffer = new wchar_t[wsize + 1];
	size_t check = mbstowcs(wbuffer, buffer, buffer_size);
	if(check == (size_t)-1) {
		delete buffer;
		delete[] wbuffer;
		wcerr << L"Unable to open source file: " << name << endl;
		exit(1);
	}
	wbuffer[wsize] = L'\0';
#endif

	free( buffer );
	return wbuffer;
}


#endif
