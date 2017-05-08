#include "classes.h"

/**
	Copyright (c) 2017 Joshua Ogunyinka
**/

/****************************
* Boolean class
****************************/
BooleanClass* BooleanClass::instance;

void BooleanClass::Equal( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value == right.value.int_value;
		break;

	default:
		wcerr << L">>> invalid logical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void BooleanClass::NotEqual( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value != right.value.int_value;
		break;

	default:
		wcerr << L">>> invalid logical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

/****************************
 * Integer class
 ****************************/
IntegerClass* IntegerClass::instance;

void IntegerClass::Add( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = INT_TYPE;
		result.sys_klass = right.sys_klass;
		result.value.int_value = left.value.int_value + right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = FLOAT_TYPE;
		result.sys_klass = right.sys_klass;
		result.value.float_value = left.value.int_value + right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void IntegerClass::Subtract( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = INT_TYPE;
		result.sys_klass = right.sys_klass;
		result.value.int_value = left.value.int_value - right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = FLOAT_TYPE;
		result.sys_klass = right.sys_klass;
		result.value.float_value = left.value.int_value - right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void IntegerClass::Multiply( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = INT_TYPE;
		result.sys_klass = right.sys_klass;
		result.value.int_value = left.value.int_value * right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = FLOAT_TYPE;
		result.sys_klass = right.sys_klass;
		result.value.float_value = left.value.int_value * right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void IntegerClass::Divide( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = INT_TYPE;
		result.sys_klass = right.sys_klass;
		result.value.int_value = left.value.int_value / right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = FLOAT_TYPE;
		result.sys_klass = right.sys_klass;
		result.value.float_value = left.value.int_value / right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void IntegerClass::Modulo( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = INT_TYPE;
		result.sys_klass = right.sys_klass;
		result.value.int_value = left.value.int_value % right.value.int_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void IntegerClass::Less( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value < right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value < right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid logical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void IntegerClass::Greater( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value > right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value > right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid logical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void IntegerClass::Equal( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value == right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value == right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid logical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void IntegerClass::NotEqual( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value != right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value != right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid logical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void IntegerClass::LessEqual( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value <= right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value <= right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid logical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void IntegerClass::GreaterEqual( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value >= right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.int_value >= right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid logical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

// methods
void IntegerClass::Abs( Value &self, Value* execution_stack, size_t &execution_stack_pos, INT_T arg_count ) {
	if ( self.type != INT_TYPE || arg_count != 0 ) {
		wcerr << L">>> expected integer type <<<" << endl;
		exit( 1 );
	}

	Value value( INT_TYPE );
	value.value.int_value = labs( self.value.int_value );
	PushValue( value, execution_stack, execution_stack_pos );
}

/****************************
* Float class
****************************/
FloatClass* FloatClass::instance;

void FloatClass::Add( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = FLOAT_TYPE;
		result.value.float_value = left.value.float_value + right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = FLOAT_TYPE;
		result.value.float_value = left.value.float_value + right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void FloatClass::Subtract( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = FLOAT_TYPE;
		result.value.float_value = left.value.float_value - right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = FLOAT_TYPE;
		result.value.float_value = left.value.float_value - right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void FloatClass::Multiply( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = FLOAT_TYPE;
		result.value.float_value = left.value.float_value * right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = FLOAT_TYPE;
		result.value.float_value = left.value.float_value * right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void FloatClass::Divide( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = FLOAT_TYPE;
		result.value.float_value = left.value.float_value - right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = FLOAT_TYPE;
		result.value.float_value = left.value.float_value - right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void FloatClass::Modulo( Value &left, Value &right, Value &result ) {
	wcerr << L">>> invalid mathematical operation <<<" << endl;
	exit( 1 );
}

void FloatClass::Less( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.float_value < right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.float_value < right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void FloatClass::Greater( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.float_value > right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.float_value > right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void FloatClass::Equal( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.float_value == right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.float_value == right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void FloatClass::NotEqual( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.float_value != right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.float_value != right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void FloatClass::LessEqual( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.float_value <= right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.float_value <= right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void FloatClass::GreaterEqual( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.float_value >= right.value.int_value;
		break;

	case FLOAT_TYPE:
		result.type = BOOL_TYPE;
		result.sys_klass = BooleanClass::Instance();
		result.value.int_value = left.value.float_value >= right.value.float_value;
		break;

	default:
		wcerr << L">>> invalid mathematical operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void FloatClass::ToInteger( Value &self, Value* execution_stack, size_t &execution_stack_pos, INT_T arg_count ) {
	Value left;
	left.type = INT_TYPE;
	left.sys_klass = IntegerClass::Instance();
	left.value.int_value = ( INT_T ) self.value.float_value;
#ifdef _DEBUG
	wcout << L"Integer->ToInteger()" << endl;
#endif
	execution_stack[ execution_stack_pos++ ] = left;
}

/****************************
* Array class
****************************/
ArrayClass* ArrayClass::instance;

void ArrayClass::New( Value &self, Value* execution_stack, size_t &execution_stack_pos, INT_T arg_count ) {
	if ( execution_stack_pos == 0 ) {
		wcerr << L">>> Array size not specified" << endl;
		exit( 1 );
	}

	// calculate array size
	size_t array_size = 1;
	std::vector<INT_T> dimensions;
	for ( INT_T i = 0; i < arg_count; i++ ) {
		Value value = execution_stack[ --execution_stack_pos ];
		switch ( value.type ) {
		case INT_TYPE:
			array_size *= value.value.int_value;
			dimensions.push_back( value.value.int_value );
			break;

		case FLOAT_TYPE:
			array_size *= ( INT_T ) value.value.float_value;
			dimensions.push_back( ( INT_T ) value.value.float_value );
			break;

		default:
			wcerr << L">>> Operation requires Integer or Float type <<<" << endl;
			exit( 1 );
		}
	}

	// allocate array
	// layout: [data_offset (0)][max_size (1)][dimensions (2-n)][<- data -> (data_offset - n)]
	const size_t meta_offset = ( arg_count + 2 );
	const size_t meta_size = sizeof( INT_T ) * meta_offset;
	const size_t data_size = sizeof( Value ) * array_size;
	void* memory = calloc( meta_size + data_size, 1 );

	// set metadata
	INT_T* meta_ptr = ( INT_T* ) memory;
	meta_ptr[ 0 ] = ( INT_T ) meta_offset;
	meta_ptr[ 1 ] = ( INT_T ) array_size;
	for ( size_t i = 0; i < dimensions.size(); i++ ) {
		meta_ptr[ i + 2 ] = ( INT_T ) dimensions[ i ];
	}
#ifdef _DEBUG
	wcout << L"  Array->New" << L"[" << array_size << L"], address=" << memory << endl;
#endif

	// set value
	Value left;
	left.type = ARRAY_TYPE;
	left.sys_klass = ArrayClass::Instance();
	left.value.ptr_value = memory;
	execution_stack[ execution_stack_pos++ ] = left;
}

/****************************
* String class
****************************/
StringClass* StringClass::instance;

void StringClass::Add( Value &left, Value &right, Value &result ) {
	switch ( right.type ) {
	case INT_TYPE: {
		result.type = STRING_TYPE;
		result.sys_klass = left.sys_klass;
		Value* left_value = static_cast< Value* >( left.value.ptr_value );
		static_cast< wstring* >( left_value->value.ptr_value )->append( std::to_wstring( right.value.int_value ) );
		result.value.ptr_value = left.value.ptr_value;
#ifdef _DEBUG
		wcout << L"=== DWACK_INT: " << *static_cast< wstring* >( left_value->value.ptr_value ) << L" ===" << endl;
#endif
	}
				   break;

	case FLOAT_TYPE: {
		result.type = STRING_TYPE;
		result.sys_klass = left.sys_klass;
		Value* left_value = static_cast< Value* >( left.value.ptr_value );
		static_cast< wstring* >( left_value->value.ptr_value )->append( std::to_wstring( right.value.float_value ) );
		result.value.ptr_value = left.value.ptr_value;
#ifdef _DEBUG
		wcout << L"=== DWACK_FLOAT: " << *static_cast< wstring* >( left_value->value.ptr_value ) << L" ===" << endl;
#endif
	}
					 break;

	case STRING_TYPE: {
		result.type = STRING_TYPE;
		result.sys_klass = left.sys_klass;
		Value* left_value = static_cast< Value* >( left.value.ptr_value );
		Value* right_value = static_cast< Value* >( right.value.ptr_value );
		static_cast< wstring* >( left_value->value.ptr_value )->append( *static_cast< wstring* >( right_value->value.ptr_value ) );
		result.value.ptr_value = left.value.ptr_value;
#ifdef _DEBUG
		wcout << L"=== DWACK_STRING: " << *static_cast< wstring* >( left_value->value.ptr_value ) << L" ===" << endl;
#endif
	}
		break;

	default:
		wcerr << L">>> invalid string operation <<<" << endl;
		exit( 1 );
		break;
	}
}

void StringClass::Size( Value &self, Value* execution_stack, size_t &execution_stack_pos, INT_T arg_count )
{
	if ( self.type != STRING_TYPE || arg_count != 0 ) {
		wcerr << L">>> expected string type <<<" << endl;
		exit( 1 );
	}

	Value* self_value = static_cast< Value* >( self.value.ptr_value );

	Value value( INT_TYPE );
	value.value.int_value = ( long )static_cast< wstring* >( self_value->value.ptr_value )->size();
	PushValue( value, execution_stack, execution_stack_pos );
}
