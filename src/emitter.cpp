/*****************************************
 * Instruction emitter
 *
 * Copyright (c) 2017 Joshua Ogunyinka
 * Copyright (c) 2013-2016 Randy Hollines
 * All rights reserved.
*/

#include "emitter.h"

using namespace compiler;
using std::wcout;
using std::wcerr;
using std::endl;

vector<Instruction*> Emitter::instruction_factory;

/****************************
 * Emits an error
 ****************************/
void Emitter::ProcessError( ParseNode* node, const wstring &msg )
{
#ifdef _DEBUG
	wcout << L"\tError: " << node->GetFileName() << L":" << node->GetLineNumber() << L": " << msg << endl;
#endif

	const wstring &str_line_num = IntToString( node->GetLineNumber() );
	errors.insert( pair<int, wstring>( node->GetLineNumber(), node->GetFileName() + L":" + str_line_num + L": " + msg ) );
}

/****************************
 * Emits an error
 ****************************/
void Emitter::ProcessError( const wstring &msg )
{
#ifdef _DEBUG
	wcout << L"\tError: " << msg << endl;
#endif

	errors.insert( pair<int, wstring>( 0, msg ) );
}

/****************************
 * Check for errors detected
 * during the contextual
 * analysis process.
 ****************************/
bool Emitter::NoErrors()
{
	// check and process errors
	if ( errors.size() ) {
		for ( auto error = errors.begin(); error != errors.end(); ++error ) {
			wcerr << error->second << endl;
		}

		return false;
	}

	return true;
}

/****************************
 * Emit program instructions
 ****************************/
std::unique_ptr<ExecutableProgram> Emitter::Emit()
{
	std::unique_ptr<ExecutableProgram> executable_program{ new ExecutableProgram };

	// emit classes
	vector<ParsedClass*> klasses = parsed_program->GetClasss();
	for ( size_t i = 0; i < klasses.size(); ++i ) {
		ParsedClass* klass = klasses[ i ];
		SymbolTable* klass_table = klass->GetSymbolTable();
		klass_table->SetIds();

		executable_program->AddClass( EmitClass( klass ) );
	}

	// emit global statements
	vector<Instruction*> block_instructions{}; // = new vector<Instruction*>;
	unordered_map<long, size_t> jump_table{};// = new unordered_map<long, size_t>;

	SymbolTable* global_table = parsed_program->GetGlobalSymbolTable();
	global_table->SetIds();

#ifdef _DEBUG
	wcout << L"---------- Emitting Global Statements ---------" << endl;
#endif
	std::set<size_t> leaders;
	EmitFunction( parsed_program->GetGlobal(), block_instructions, jump_table, leaders );
#ifdef _DEBUG
	wcout << ( block_instructions.size() + 1 ) << L": " << L"return" << endl;
#endif
	block_instructions.push_back( MakeInstruction( RTRN ) );

	const int local_count = parsed_program->GetLocalCount();
	ExecutableFunction* global = new ExecutableFunction( L"#GLOBAL#", NO_OP, local_count, 0, 
		std::move( block_instructions ), std::move( jump_table ), leaders, false );
	executable_program->SetMain( global );

	// emit functions
	vector<ParsedFunction*> functions = parsed_program->GetFunctions();
	for ( size_t i = 0; i < functions.size(); i++ ) {
		ParsedFunction* parsed_function = functions[ i ];
		SymbolTable* function_table = parsed_function->GetSymbolTable();
		function_table->SetIds();
		executable_program->AddFunction( EmitFunction( parsed_function ) );
	}
	
	// free the parsed_program
	parsed_program.reset();

	// check for errors
	if ( NoErrors() ) {
		return executable_program;
	}
	return nullptr;
}

ExecutableClass* Emitter::EmitClass( ParsedClass* parsed_klass )
{
#ifdef _DEBUG
	wcout << L"\n========== Emitting Class: name='" << parsed_klass->GetName() << L"' ==========" << endl;
#endif

	// TODO: emit instance statements
	vector<Statement*> instance_stmts = parsed_klass->GetDeclarations()->GetStatements();
	for ( size_t i = 0; i < instance_stmts.size(); ++i ) {
	}

	const int inst_count = static_cast< int >( parsed_klass->GetDeclarations()->GetStatements().size() );
	ExecutableClass* klass = new ExecutableClass( parsed_klass->GetName(), inst_count );

	// emit functions
	vector<ParsedFunction*> functions = parsed_klass->GetFunctions();
	for ( size_t i = 0; i < functions.size(); ++i ) {
		klass->AddFunction( EmitFunction( functions[ i ] ) );
	}

	return klass;
}

ExecutableFunction* Emitter::EmitFunction( ParsedFunction* parsed_function )
{
#ifdef _DEBUG
	wcout << L"\n---------- Emitting Function: name='" << parsed_function->GetName() << L"' ---------" << endl;
#endif

	// create holders
	returns_value = -1;
	vector<Instruction*> block_instructions{};
	unordered_map<long, size_t> jump_table{};
	std::set<size_t> leaders;

	vector<Expression*> parameters = parsed_function->GetParameters()->GetExpressions();
	for ( size_t i = 0; i < parameters.size(); ++i ) {
		Reference* reference = static_cast< Reference* >( parameters[ i ] );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"store: name='" << reference->GetName()
			<< L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
		block_instructions.push_back( MakeInstruction( STOR_VAR, LOCL, reference->GetDeclaration()->GetId() ) );
	}

	// emit function
	StatementList* statement_list = parsed_function->GetStatements();
	EmitFunction( statement_list, block_instructions, jump_table, leaders );

	// new call
	if ( parsed_function->IsNew() ) {
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"load local, name='self', id=0" << endl;
#endif
		block_instructions.push_back( MakeInstruction( LOAD_VAR, LOCL, 0 ) );
	}

	// check return type
	if ( block_instructions.size() == 0 || block_instructions.back()->type != RTRN ) {
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"return" << endl;
#endif
		block_instructions.push_back( MakeInstruction( RTRN ) );

		if ( returns_value == 1 ) {
			ProcessError( statement_list->GetStatements().back(), L"Last statement must return a value" );
		}
	}
	else {
		if ( !static_cast< Return* >( statement_list->GetStatements().back() )->GetExpression() && returns_value == 1 ) {
			ProcessError( statement_list->GetStatements().back(), L"Last statement must return a value" );
		}
	}

#ifdef _DEBUG
	wcout << L"Leaders" << endl;
	for ( auto iter = leaders.begin(); iter != leaders.end(); ++iter ) {
		wcout << L"  " << *iter << endl;
	}
#endif

	const int local_count = parsed_function->GetLocalCount();
	return new ExecutableFunction( parsed_function->GetName(), parsed_function->GetOperation(), local_count, static_cast< int >( parameters.size() ),
		std::move( block_instructions ), std::move( jump_table ), leaders, returns_value > 0 );
}

/****************************
 * Emit code for a function
 ****************************/
void Emitter::EmitFunction( StatementList* block_statements, vector<Instruction*> & block_instructions,
	unordered_map<long, size_t>& jump_table, std::set<size_t> &leaders )
{
	EmitBlock( block_statements, block_instructions, jump_table );

	// create CFG
	leaders.insert( 0 );
	for ( size_t i = 0; i < block_instructions.size(); ++i ) {
		if ( block_instructions.at( i )->type == LBL ) {
			leaders.insert( i );
		}
		else if ( block_instructions.at( i )->type == JMP ) {
			leaders.insert( i + 1 );
		}
	}
}

/****************************
 * Emit code for a statement block
 ****************************/
void Emitter::EmitBlock( StatementList* block_statements, vector<Instruction*>& block_instructions,
	unordered_map<long, size_t>& jump_table )
{
	vector<Statement*> statements = block_statements->GetStatements();
	for ( size_t i = 0; i < statements.size(); i++ ) {
		Statement* statement = statements[ i ];
		switch ( statement->GetStatementType() ) {
		case ASSIGNMENT_STATEMENT:
			EmitAssignment( static_cast< Assignment* >( statement ), block_instructions, jump_table );
			break;

		case DECLARATION_STATEMENT:
			break;

		case RETURN_STATEMENT:
			if ( static_cast< Return* >( statement )->GetExpression() ) {
				EmitExpression( static_cast< Return* >( statement )->GetExpression(), block_instructions, jump_table );
				if ( returns_value == 0 ) {
					ProcessError( statement, L"Not all statements return a value" );
				}
				else {
					returns_value = 1;
				}
			}
			else {
				if ( returns_value == 1 ) {
					ProcessError( statement, L"Not all statements return a value" );
				}
				else {
					returns_value = 0;
				}
			}
#ifdef _DEBUG
			wcout << ( block_instructions.size() + 1 ) << L": " << L"return" << endl;
#endif
			block_instructions.push_back( MakeInstruction( RTRN ) );
			break;

		case FUNCTION_CALL_STATEMENT:
			EmitFunctionCall( static_cast< FunctionCall* >( statement ), block_instructions, jump_table );
			break;

		case IF_ELSE_STATEMENT:
			EmitIfElse( static_cast< IfElse* >( statement ), block_instructions, jump_table );
			break;

		case WHILE_STATEMENT:
			EmitWhile( static_cast< While* >( statement ), block_instructions, jump_table );
			break;

		case SHOW_STATEMENT: {
			// emit expression
			Expression* expression = static_cast< Dump* >( statement )->GetExpression();
			if ( expression->GetExpressionType() == FUNCTION_CALL_EXPR ) {
				FunctionCall* function_call = static_cast< FunctionCall* >( expression );
				function_call->ReturnsValue( true );
			}
			EmitExpression( expression, block_instructions, jump_table );

#ifdef _DEBUG
			wcout << ( block_instructions.size() + 1 ) << L": " << L"show value" << endl;
#endif

			block_instructions.push_back( MakeInstruction( SHOW_TYPE ) );
		}
							 break;

		default:
			// TODO: report error
			break;
		}
	}
}

/****************************
* Emit parameters for a method call
****************************/
void Emitter::EmitFunctionCallParameters( Reference* reference, vector<Instruction*>& block_instructions,
	unordered_map<long, size_t>& jump_table )
{
	Reference* last = reference;
	while ( last->GetReference() ) {
		last->GetReference()->SetPerviousReference( last );
		last = last->GetReference();
	}

	// emit calling parameters
	while ( last ) {
		if ( last->GetCallingParameters() ) {
			vector<Expression*> parameters = last->GetCallingParameters()->GetExpressions();
			for ( std::vector<Expression*>::reverse_iterator iter = parameters.rbegin(); iter != parameters.rend(); ++iter ) {
				EmitExpression( *iter, block_instructions, jump_table );
			}
		}
		// update
		last = last->GetPerviousReference();
	}
}

/****************************
 * Emit code for a method call
 ****************************/
void Emitter::EmitFunctionCall( FunctionCall* function_call, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table )
{
	Reference* reference = function_call->GetReference();

	// emit calling parameters
	EmitFunctionCallParameters( reference, block_instructions, jump_table );

	// array indices
	if ( reference->GetIndices() ) {
		vector<Expression*> indices = reference->GetIndices()->GetExpressions();
		for ( size_t i = 0; i < indices.size(); i++ ) {
			EmitExpression( indices[ i ], block_instructions, jump_table );
		}
	}

	// new instance
	vector<Expression*> parameters;
	if ( function_call->IsNew() ) {
		parameters = reference->GetReference()->GetCallingParameters()->GetExpressions();

		// new string
		if ( reference->GetName() == L"String" && reference->GetReference() && reference->GetReference()->GetName() == L"new" ) {
#ifdef _DEBUG
			wcout << ( block_instructions.size() + 1 ) << L": " << L"new string" << endl;
#endif
			block_instructions.push_back( MakeInstruction( NEW_STRING ) );
		}
		// TODO: new hash
		else if ( reference->GetName() == L"Hash" && reference->GetReference() && reference->GetReference()->GetName() == L"new" ) {
#ifdef _DEBUG
			wcout << ( block_instructions.size() + 1 ) << L": " << L"new hash" << endl;
#endif
			block_instructions.push_back( MakeInstruction( NEW_HASH ) );
		}
		else {
#ifdef _DEBUG
			wcout << ( block_instructions.size() + 1 ) << L": " << L"new instance: name='" << reference->GetName() << L"'" << endl;
#endif
			block_instructions.push_back( MakeInstruction( NEW_OBJ, static_cast< int >( parameters.size() ), 1, reference->GetName() ) );

#ifdef _DEBUG
			wcout << ( block_instructions.size() + 1 ) << L": " << L"function call: class='"
				<< function_call->GetReference()->GetName() << L"', method='" << function_call->GetCallerName( 0 ) << L"'" << endl;
#endif      
			block_instructions.push_back( MakeInstruction( CALL_FUNC, static_cast< int >( parameters.size() ), function_call->ReturnsValue() ? 1 : 0, function_call->GetCallerName( 0 ) ) );
		}

		// nested call
		if ( reference->GetReference() && reference->GetReference()->GetReference() ) {
			EmitNestedFunctionCall( reference->GetReference()->GetReference(), block_instructions, jump_table );
		}
	}
	else if ( reference->GetReference() ) {
		if ( reference->GetExpressionType() == REF_EXPR ) {
#ifdef _DEBUG
			wcout << ( block_instructions.size() + 1 ) << L": " << L"load local, name='" << reference->GetName() << L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
			block_instructions.push_back( MakeInstruction( LOAD_VAR, LOCL, reference->GetDeclaration()->GetId() ) );

#ifdef _DEBUG
			wcout << ( block_instructions.size() + 1 ) << L": " << L"function call: name='" << function_call->GetCallerName( 0 ) << L"'" << endl;
#endif
			parameters = reference->GetReference()->GetCallingParameters()->GetExpressions();
			block_instructions.push_back( MakeInstruction( CALL_FUNC, static_cast< int >( parameters.size() ), function_call->ReturnsValue() ? 1 : 0, function_call->GetCallerName( 0 ) ) );

			// nested call
			if ( reference->GetReference()->GetReference() ) {
				EmitNestedFunctionCall( reference->GetReference()->GetReference(), block_instructions, jump_table );
			}
		}
		else {
			// TODO: finish
			// literal function calls
			switch ( reference->GetExpressionType() ) {
			case NIL_LIT_EXPR:
				break;

			case CHAR_LIT_EXPR:
				break;

			case INT_LIT_EXPR: {
#ifdef _DEBUG
				wcout << ( block_instructions.size() + 1 ) << L": " << L"load literal: type=integer, value="
					<< static_cast< IntegerLiteral* >( reference )->GetValue() << endl;
#endif
				block_instructions.push_back( MakeInstruction( LOAD_INT_LIT, static_cast< int >( static_cast< IntegerLiteral* >( reference )->GetValue() ) ) );
				block_instructions.push_back( MakeInstruction( CALL_FUNC, static_cast< int >( parameters.size() ), function_call->ReturnsValue() ? 1 : 0, function_call->GetCallerName( 0 ) ) );
			}
							   break;

			case FLOAT_LIT_EXPR:
				break;

			default:
				break;
			}
		}
	}
	// emit function
	else {
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"function call: name='" << reference->GetName() << L"'" << endl;
#endif
		parameters = reference->GetCallingParameters()->GetExpressions();
		block_instructions.push_back( MakeInstruction( LOAD_INT_LIT, 0 ) ); // instance
		block_instructions.push_back( MakeInstruction( CALL_FUNC, static_cast< int >( parameters.size() ), function_call->ReturnsValue() ? 1 : 0, function_call->GetCallerName( 0 ) ) );
	}
}

void Emitter::EmitNestedFunctionCall( Reference* reference, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table )
{
	while ( reference ) {
		vector<Expression*> parameters = reference->GetCallingParameters()->GetExpressions();
		wstring function_name = reference->GetName() + L':' + IntToString( static_cast< int >( parameters.size() ) );

#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"function call: method='" << function_name << L"'" << endl;
#endif
		block_instructions.push_back( MakeInstruction( CALL_FUNC, static_cast< int >( parameters.size() ), 1, function_name ) );

		// next
		reference = reference->GetReference();
	}
}
/****************************
 * Emit 'if/else' code
 ****************************/
void Emitter::EmitIfElse( IfElse* if_else, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table )
{
	const long end_label = NextEndId();
	long next_label = NextStartId();

	// emit test

	// emit expression
	if ( if_else->GetExpression()->GetExpressionType() == FUNCTION_CALL_EXPR ) {
		FunctionCall* function_call = static_cast< FunctionCall* >( if_else->GetExpression() );
		function_call->ReturnsValue( true );
	}
	EmitExpression( if_else->GetExpression(), block_instructions, jump_table );

	// basic 'if' statement
	vector<IfElse*> else_ifs = if_else->GetElseIfs();
	StatementList* else_block = if_else->GetElseBlock();
	if ( else_ifs.size() == 0 && !else_block ) {
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"jump false: id=" << end_label << endl;
#endif
		block_instructions.push_back( MakeInstruction( JMP, static_cast< int >( end_label ), JMP_FALSE ) );
		// emit 'if' block
		EmitBlock( if_else->GetIfBlock(), block_instructions, jump_table );
	}
	else {
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"jump false: id=" << next_label << endl;
#endif
		block_instructions.push_back( MakeInstruction( JMP, static_cast< int >( next_label ), JMP_FALSE ) );
		// emit 'if' block
		EmitBlock( if_else->GetIfBlock(), block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"jump: id=" << end_label << endl;
#endif
		block_instructions.push_back( MakeInstruction( JMP, static_cast< int >( end_label ), -1 ) );
	}

	// 'if-else' blocks
	if ( else_ifs.size() > 0 ) {
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"label: id=" << next_label
			<< L", pos=" << block_instructions.size() << endl;
#endif
		block_instructions.push_back( MakeInstruction( LBL, static_cast< int >( next_label ), 0 ) );
		jump_table.insert( pair<long, size_t>( next_label, block_instructions.size() - 1 ) );

		for ( size_t i = 0; i < else_ifs.size(); i++ ) {
			IfElse* else_if = else_ifs[ i ];
			// emit test
			EmitExpression( else_if->GetExpression(), block_instructions, jump_table );
#ifdef _DEBUG
			wcout << ( block_instructions.size() + 1 ) << L": " << L"jump false: id=" << ( next_label + 1 ) << endl;
#endif
			block_instructions.push_back( MakeInstruction( JMP, static_cast< int >( next_label + 1 ), JMP_FALSE ) );

			// emit 'if' block
			EmitBlock( else_if->GetIfBlock(), block_instructions, jump_table );
#ifdef _DEBUG
			wcout << ( block_instructions.size() + 1 ) << L": " << L"jump: id=" << end_label << endl;
#endif
			// jump to end
			block_instructions.push_back( MakeInstruction( JMP, static_cast< int >( end_label ), -1 ) );

			next_label = NextStartId();
#ifdef _DEBUG
			wcout << ( block_instructions.size() + 1 ) << L": " << L"label: id=" << next_label
				<< L", pos=" << block_instructions.size() << endl;
#endif
			block_instructions.push_back( MakeInstruction( LBL, static_cast< int >( next_label ), 0 ) );
			jump_table.insert( pair<long, size_t>( next_label, block_instructions.size() - 1 ) );
		}
	}

	// 'else' blocks
	if ( else_block ) {
		if ( else_ifs.size() == 0 ) {
#ifdef _DEBUG
			wcout << ( block_instructions.size() + 1 ) << L": " << L"label: id=" << next_label
				<< L", pos=" << block_instructions.size() << endl;
#endif
			block_instructions.push_back( MakeInstruction( LBL, static_cast< int >( next_label ), 0 ) );
			jump_table.insert( pair<long, size_t>( next_label, block_instructions.size() - 1 ) );
		}
		// 'else' block
		EmitBlock( if_else->GetElseBlock(), block_instructions, jump_table );
	}

	// end label
#ifdef _DEBUG
	wcout << ( block_instructions.size() + 1 ) << L": " << L"label: id=" << end_label
		<< L", pos=" << block_instructions.size() << endl;
#endif
	block_instructions.push_back( MakeInstruction( LBL, static_cast< int >( end_label ), 0 ) );
	jump_table.insert( pair<long, size_t>( end_label, block_instructions.size() - 1 ) );
}

/****************************
 * TODO: doc
 ****************************/
void Emitter::EmitWhile( While* if_while, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table )
{
	const long top_label = NextStartId();
	const long end_label = NextEndId();

	// top label
#ifdef _DEBUG
	wcout << ( block_instructions.size() + 1 ) << L": " << L"label: id=" << top_label
		<< L", pos=" << block_instructions.size() << endl;
#endif
	block_instructions.push_back( MakeInstruction( LBL, static_cast< int >( top_label ), 0 ) );
	jump_table.insert( pair<long, size_t>( top_label, block_instructions.size() - 1 ) );

	// emit test
	EmitExpression( if_while->GetExpression(), block_instructions, jump_table );

	// jump end
#ifdef _DEBUG
	wcout << ( block_instructions.size() + 1 ) << L": " << L"jump false: id=" << end_label << endl;
#endif
	block_instructions.push_back( MakeInstruction( JMP, static_cast< int >( end_label ), JMP_FALSE ) );

	// emit block
	EmitBlock( if_while->GetBlock(), block_instructions, jump_table );

#ifdef _DEBUG
	wcout << ( block_instructions.size() + 1 ) << L": " << L"jump: id=" << top_label << endl;
#endif
	block_instructions.push_back( MakeInstruction( JMP, static_cast< int >( top_label ), JMP_UNCND ) );

	// end label
#ifdef _DEBUG
	wcout << ( block_instructions.size() + 1 ) << L": " << L"label: id=" << end_label
		<< L", pos=" << block_instructions.size() << endl;
#endif
	block_instructions.push_back( MakeInstruction( LBL, static_cast< int >( end_label ), 0 ) );
	jump_table.insert( pair<long, size_t>( end_label, block_instructions.size() - 1 ) );
}

/****************************
 * Emit assignment code
 ****************************/
void Emitter::EmitAssignment( Assignment* assignment, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table )
{
	// emit expression
	if ( assignment->GetExpression()->GetExpressionType() == FUNCTION_CALL_EXPR ) {
		FunctionCall* function_call = static_cast< FunctionCall* >( assignment->GetExpression() );
		function_call->ReturnsValue( true );
	}
	EmitExpression( assignment->GetExpression(), block_instructions, jump_table );

	switch ( assignment->GetAssignmentType() ) {
	case ScannerTokenType::TOKEN_ADD_EQL:
		EmitReference( assignment->GetReference(), false, block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '+'" << endl;
#endif
		block_instructions.push_back( MakeInstruction( ADD ) );
		break;

	case ScannerTokenType::TOKEN_SUB_EQL:
		EmitReference( assignment->GetReference(), false, block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '-'" << endl;
#endif
		block_instructions.push_back( MakeInstruction( SUB ) );
		break;

	case ScannerTokenType::TOKEN_MUL_EQL:
		EmitReference( assignment->GetReference(), false, block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '*'" << endl;
#endif
		block_instructions.push_back( MakeInstruction( MUL ) );
		break;

	case ScannerTokenType::TOKEN_DIV_EQL:
		EmitReference( assignment->GetReference(), false, block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '/'" << endl;
#endif
		block_instructions.push_back( MakeInstruction( DIV ) );
		break;

	default:
		break;
	}

	// emit reference
	EmitReference( assignment->GetReference(), true, block_instructions, jump_table );
}

/****************************
 * Emit variable reference
 ****************************/
void Emitter::EmitReference( Reference* reference, bool is_store, vector<Instruction*>& block_instructions, unordered_map<long, size_t>& jump_table )
{
	switch ( reference->GetReferenceType() ) {
	case SELF_TYPE:
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"load local, name='self', id=0" << endl;
#endif
		block_instructions.push_back( MakeInstruction( LOAD_VAR, LOCL, 0 ) );
		break;

	case NEW_LIST_TYPE:
		break;

	case NEW_HASH_TYPE:
		break;

	case NEW_OBJ_TYPE:
		break;

		// reference
	case REF_TYPE: {
		// store
		if ( is_store ) {
			// array element
			if ( reference->GetIndices() ) {
				vector<Expression*> indices = reference->GetIndices()->GetExpressions();
				for ( size_t i = 0; i < indices.size(); ++i ) {
					EmitExpression( indices[ i ], block_instructions, jump_table );
				}

				switch ( reference->GetDeclaration()->GetType() ) {
				case LOCL_DCLR:
#ifdef _DEBUG
					wcout << ( block_instructions.size() + 1 ) << L": " << L"store array element, local, name='" << reference->GetName() << L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
					block_instructions.push_back( MakeInstruction( STOR_ARY_VAR, LOCL, reference->GetDeclaration()->GetId(), static_cast< int >( indices.size() ) ) );
					break;

				case INST_DCLR:
#ifdef _DEBUG
					wcout << ( block_instructions.size() + 1 ) << L": " << L"store array element, instance, name='" << reference->GetName() << L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
					block_instructions.push_back( MakeInstruction( STOR_ARY_VAR, INST, reference->GetDeclaration()->GetId(), static_cast< int >( indices.size() ) ) );
					break;

				case CLS_DCLR:
#ifdef _DEBUG
					wcout << ( block_instructions.size() + 1 ) << L": " << L"store array element, class, name='" << reference->GetName() << L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
					// TODO: load class memory
					block_instructions.push_back( MakeInstruction( STOR_ARY_VAR, CLS, reference->GetDeclaration()->GetId(), static_cast< int >( indices.size() ) ) );
					break;
				}
			}
			// element
			else {
				switch ( reference->GetDeclaration()->GetType() ) {
				case LOCL_DCLR:
#ifdef _DEBUG
					wcout << ( block_instructions.size() + 1 ) << L": " << L"store element, local, name='" << reference->GetName() << L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
					block_instructions.push_back( MakeInstruction( STOR_VAR, LOCL, reference->GetDeclaration()->GetId() ) );
					break;

				case INST_DCLR:
#ifdef _DEBUG
					wcout << ( block_instructions.size() + 1 ) << L": " << L"store element, instance, name='" << reference->GetName() << L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
					block_instructions.push_back( MakeInstruction( STOR_VAR, INST, reference->GetDeclaration()->GetId() ) );
					break;

				case CLS_DCLR:
#ifdef _DEBUG
					wcout << ( block_instructions.size() + 1 ) << L": " << L"store element, class, name='" << reference->GetName() << L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
					// TODO: load class memory
					block_instructions.push_back( MakeInstruction( STOR_VAR, CLS, reference->GetDeclaration()->GetId() ) );
					break;
				}
			}
		}
		// load
		else {
			// array element
			if ( reference->GetIndices() ) {
				vector<Expression*> indices = reference->GetIndices()->GetExpressions();
				for ( size_t i = 0; i < indices.size(); ++i ) {
					EmitExpression( indices[ i ], block_instructions, jump_table );
				}

				switch ( reference->GetDeclaration()->GetType() ) {
				case LOCL_DCLR:
#ifdef _DEBUG
					wcout << ( block_instructions.size() + 1 ) << L": " << L"load local variable, name='" << reference->GetName() << L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
					block_instructions.push_back( MakeInstruction( LOAD_ARY_VAR, LOCL, reference->GetDeclaration()->GetId(), static_cast< int >( indices.size() ) ) );

					break;

				case INST_DCLR:
#ifdef _DEBUG
					wcout << ( block_instructions.size() + 1 ) << L": " << L"load instance variable: name='" << reference->GetName() << L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
					block_instructions.push_back( MakeInstruction( LOAD_ARY_VAR, INST, reference->GetDeclaration()->GetId(), static_cast< int >( indices.size() ) ) );
					break;

				case CLS_DCLR:
#ifdef _DEBUG
					wcout << ( block_instructions.size() + 1 ) << L": " << L"load class variable: name='" << reference->GetName() << L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
					// TODO: local class memory
					block_instructions.push_back( MakeInstruction( LOAD_ARY_VAR, CLS, reference->GetDeclaration()->GetId(), static_cast< int >( indices.size() ) ) );
					break;
				}
			}
			// new array
			else if ( reference->GetName() == L"Array" && reference->GetReference() && reference->GetReference()->GetName() == L"new" ) {
				vector<Expression*> dimensions = reference->GetReference()->GetIndices()->GetExpressions();
				if ( dimensions.size() > 0 ) {
					EmitExpression( dimensions[ 0 ], block_instructions, jump_table );
					for ( size_t i = 1; i < dimensions.size(); ++i ) {
						EmitExpression( dimensions[ i ], block_instructions, jump_table );
					}

#ifdef _DEBUG
					wcout << ( block_instructions.size() + 1 ) << L": " << L"new array: dimensions=" << dimensions.size() << endl;
#endif
					block_instructions.push_back( MakeInstruction( NEW_ARRAY, static_cast< int >( dimensions.size() ) ) );
				}
				else {
					ProcessError( reference->GetReference(), L"Array size not specified" );
				}
			}
			// element
			else {
				switch ( reference->GetDeclaration()->GetType() ) {
				case LOCL_DCLR:
#ifdef _DEBUG
					wcout << ( block_instructions.size() + 1 ) << L": " << L"load local variable, name='" << reference->GetName() << L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
					block_instructions.push_back( MakeInstruction( LOAD_VAR, LOCL, reference->GetDeclaration()->GetId() ) );
					break;

				case INST_DCLR:
#ifdef _DEBUG
					wcout << ( block_instructions.size() + 1 ) << L": " << L"load variable, instance, name='" << reference->GetName() << L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
					block_instructions.push_back( MakeInstruction( LOAD_VAR, INST, reference->GetDeclaration()->GetId() ) );
					break;

				case CLS_DCLR:
#ifdef _DEBUG
					wcout << ( block_instructions.size() + 1 ) << L": " << L"load variable, class, name='" << reference->GetName() << L"', id=" << reference->GetDeclaration()->GetId() << endl;
#endif
					// TODO: local class memory
					block_instructions.push_back( MakeInstruction( LOAD_VAR, CLS, reference->GetDeclaration()->GetId() ) );
					break;
				}
			}
		}
	}
				   break;
	}
}

/****************************
 * Emit expression
 ****************************/
void Emitter::EmitExpression( Expression* expression, vector<Instruction*>& block_instructions,
	unordered_map<long, size_t>& jump_table )
{
	switch ( expression->GetExpressionType() ) {
	case REF_EXPR:
		EmitReference( static_cast< Reference* >( expression ), false, block_instructions, jump_table );
		break;

	case FUNCTION_CALL_EXPR:
		EmitFunctionCall( static_cast< FunctionCall* >( expression ), block_instructions, jump_table );
		break;

	case BOOLEAN_LIT_EXPR:
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"load literal: type=boolean, value="
			<< ( static_cast< BooleanLiteral* >( expression )->GetValue() ? L"true" : L"false" ) << endl;
#endif     
		if ( static_cast< BooleanLiteral* >( expression )->GetValue() ) {
			block_instructions.push_back( MakeInstruction( LOAD_TRUE_LIT ) );
		}
		else {
			block_instructions.push_back( MakeInstruction( LOAD_FALSE_LIT ) );
		}
		break;

	case CHAR_LIT_EXPR:
		// TODO: implement
		break;

	case INT_LIT_EXPR:
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"load literal: type=integer, value="
			<< static_cast< IntegerLiteral* >( expression )->GetValue() << endl;
#endif
		block_instructions.push_back( MakeInstruction( LOAD_INT_LIT, static_cast< int >( static_cast< IntegerLiteral* >( expression )->GetValue() ) ) );
		break;

	case FLOAT_LIT_EXPR:
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"load literal: type=float, value="
			<< static_cast< FloatLiteral* >( expression )->GetValue() << endl;
#endif
		block_instructions.push_back( MakeInstruction( LOAD_FLOAT_LIT, ( FLOAT_T )static_cast< FloatLiteral* >( expression )->GetValue() ) );
		break;

	case AND_EXPR: {
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetRight(), block_instructions, jump_table );
		const long next_label = NextEndId();
		const long end_label = NextEndId();
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"jump true: id=" << next_label << endl;
#endif
		block_instructions.push_back( MakeInstruction( JMP, static_cast< int >( next_label ), JMP_TRUE ) );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"load literal: type=boolean, value=false" << endl;
#endif
		block_instructions.push_back( MakeInstruction( LOAD_FALSE_LIT ) );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"jump: id=" << end_label << endl;
#endif
		block_instructions.push_back( MakeInstruction( JMP, static_cast< int >( end_label ), JMP_UNCND ) );

#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"label: id=" << next_label
			<< L", pos=" << block_instructions.size() << endl;
#endif
		block_instructions.push_back( MakeInstruction( LBL, static_cast< int >( next_label ), JMP_FALSE ) );
		jump_table.insert( pair<long, size_t>( next_label, block_instructions.size() - 1 ) );
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetLeft(), block_instructions, jump_table );

#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"label: id=" << end_label
			<< L", pos=" << block_instructions.size() << endl;
#endif
		block_instructions.push_back( MakeInstruction( LBL, static_cast< int >( end_label ), 0 ) );
		jump_table.insert( pair<long, size_t>( end_label, block_instructions.size() - 1 ) );

#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '&'" << endl;
#endif
	}
				   break;

	case OR_EXPR: {
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetRight(), block_instructions, jump_table );
		const long next_label = NextEndId();
		const long end_label = NextEndId();
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"jump false: id=" << next_label << endl;
#endif
		block_instructions.push_back( MakeInstruction( JMP, static_cast< int >( next_label ), JMP_FALSE ) );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"load literal: type=boolean, value=false" << endl;
#endif
		block_instructions.push_back( MakeInstruction( LOAD_FALSE_LIT ) );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"jump: id=" << end_label << endl;
#endif
		block_instructions.push_back( MakeInstruction( JMP, static_cast< int >( end_label ), JMP_UNCND ) );

#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"label: id=" << next_label
			<< L", pos=" << block_instructions.size() << endl;
#endif
		block_instructions.push_back( MakeInstruction( LBL, static_cast< int >( next_label ), JMP_FALSE ) );
		jump_table.insert( pair<long, size_t>( next_label, block_instructions.size() - 1 ) );
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetLeft(), block_instructions, jump_table );

#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"label: id=" << end_label
			<< L", pos=" << block_instructions.size() << endl;
#endif
		block_instructions.push_back( MakeInstruction( LBL, static_cast< int >( end_label ), 0 ) );
		jump_table.insert( pair<long, size_t>( end_label, block_instructions.size() - 1 ) );

#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '|'" << endl;
#endif
	}
				  break;

	case EQL_EXPR: {
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetRight(), block_instructions, jump_table );
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetLeft(), block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '=='" << endl;
#endif
		block_instructions.push_back( MakeInstruction( EQL ) );
	}
				   break;

	case NEQL_EXPR: {
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetRight(), block_instructions, jump_table );
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetLeft(), block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '!='" << endl;
#endif
		block_instructions.push_back( MakeInstruction( NEQL ) );
	}
					break;

	case LES_EXPR: {
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetRight(), block_instructions, jump_table );
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetLeft(), block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '<'" << endl;
#endif
		block_instructions.push_back( MakeInstruction( LES ) );
	}
				   break;

	case GTR_EQL_EXPR: {
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetRight(), block_instructions, jump_table );
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetLeft(), block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '>='" << endl;
#endif
		block_instructions.push_back( MakeInstruction( GTR_EQL ) );
	}
					   break;

	case LES_EQL_EXPR: {
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetRight(), block_instructions, jump_table );
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetLeft(), block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '<='" << endl;
#endif
		block_instructions.push_back( MakeInstruction( LES_EQL ) );
	}
					   break;

	case GTR_EXPR: {
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetRight(), block_instructions, jump_table );
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetLeft(), block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << block_instructions.size() << L": " << L"operator: '>'" << endl;
#endif
		block_instructions.push_back( MakeInstruction( GTR ) );
	}
				   break;

	case ADD_EXPR: {
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetRight(), block_instructions, jump_table );
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetLeft(), block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '+'" << endl;
#endif
		block_instructions.push_back( MakeInstruction( ADD ) );
	}
				   break;

	case SUB_EXPR: {
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetRight(), block_instructions, jump_table );
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetLeft(), block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '-'" << endl;
#endif
		block_instructions.push_back( MakeInstruction( SUB ) );
	}
				   break;

	case MUL_EXPR: {
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetRight(), block_instructions, jump_table );
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetLeft(), block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '*'" << endl;
#endif
		block_instructions.push_back( MakeInstruction( MUL ) );
	}
				   break;

	case DIV_EXPR: {
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetRight(), block_instructions, jump_table );
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetLeft(), block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '/'" << endl;
#endif
		block_instructions.push_back( MakeInstruction( DIV ) );
	}
				   break;

	case MOD_EXPR: {
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetRight(), block_instructions, jump_table );
		EmitExpression( static_cast< CalculatedExpression* >( expression )->GetLeft(), block_instructions, jump_table );
#ifdef _DEBUG
		wcout << ( block_instructions.size() + 1 ) << L": " << L"operator: '%'" << endl;
#endif
		block_instructions.push_back( MakeInstruction( MOD ) );
	}
				   break;

	default:
		// TODO: error
		break;
	}
}

/****************************
 * Clear parse tree nodes
 ****************************/
void Emitter::ClearInstructions() {
	while ( !instruction_factory.empty() ) {
		Instruction* tmp = instruction_factory.front();
		instruction_factory.erase( instruction_factory.begin() );
		// delete
		delete tmp;
		tmp = NULL;
	}
}
