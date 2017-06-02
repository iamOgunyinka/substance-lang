/***************************************************************************
 * Language parser
 *
 * Copyright (c) 2017 Joshua Ogunyinka
 * All rights reserved.
 */

#include <memory>
#include "parser.h"

using namespace compiler;

/****************************
 * Loads parsing error codes
 ****************************/

using std::wcout;
using std::wcerr;
using std::endl;

void Parser::LoadErrorCodes()
{
	error_msgs[ ScannerTokenType::TOKEN_IF_ID ] = L"Expected 'if'";
	error_msgs[ ScannerTokenType::TOKEN_IDENT ] = L"Expected identifier";
	error_msgs[ ScannerTokenType::TOKEN_OPEN_PAREN ] = L"Expected '('";
	error_msgs[ ScannerTokenType::TOKEN_CLOSED_PAREN ] = L"Expected ')'";
	error_msgs[ ScannerTokenType::TOKEN_OPEN_BRACKET ] = L"Expected '['";
	error_msgs[ ScannerTokenType::TOKEN_CLOSED_BRACKET ] = L"Expected ']'";
	error_msgs[ ScannerTokenType::TOKEN_OPEN_BRACE ] = L"Expected '{'";
	error_msgs[ ScannerTokenType::TOKEN_CLOSED_BRACE ] = L"Expected '}'";
	error_msgs[ ScannerTokenType::TOKEN_COLON ] = L"Expected ':'";
	error_msgs[ ScannerTokenType::TOKEN_COMMA ] = L"Expected ','";
	error_msgs[ ScannerTokenType::TOKEN_ASSIGN ] = L"Expected ':=' or '='";
	error_msgs[ ScannerTokenType::TOKEN_SEMI_COLON ] = L"Expected ';'";
	error_msgs[ ScannerTokenType::TOKEN_PERIOD ] = L"Expected '->'";
}

/****************************
 * Emits parsing error
 ****************************/
void Parser::ProcessError( ScannerTokenType type )
{
	wstring msg = error_msgs[ type ];
#ifdef _DEBUG
	wcout << L"\tError: " << GetFileName() << L":" << GetLineNumber() << L": "
		<< msg << endl;
#endif

	const wstring &str_line_num = ToString( GetLineNumber() );
	errors.insert( { GetLineNumber(), GetFileName() + L":" + str_line_num + L": " + msg } );
}

/****************************
 * Emits parsing error
 ****************************/
void Parser::ProcessError( const wstring &msg )
{
#ifdef _DEBUG
	wcout << L"\tError: " << GetFileName() << L":" << GetLineNumber() << L": " << msg << endl;
#endif

	const wstring &str_line_num = ToString( GetLineNumber() );
	errors.insert( { GetLineNumber(), GetFileName() + L":" + str_line_num + L": " + msg } );
}

/****************************
 * Emits parsing error
 ****************************/
void Parser::ProcessError( const wstring &msg, ScannerTokenType sync )
{
#ifdef _DEBUG
	wcout << L"\tError: " << GetFileName() << L":" << GetLineNumber() << L": "
		<< msg << endl;
#endif

	const wstring &str_line_num = ToString( GetLineNumber() );
	errors.insert( { GetLineNumber(), GetFileName() + L":" + str_line_num + L": " + msg } );
	ScannerTokenType token = GetToken();
	while ( token != sync && token != ScannerTokenType::TOKEN_END_OF_STREAM ) {
		NextToken();
		token = GetToken();
	}
}

/****************************
 * Emits parsing error
 ****************************/
void Parser::ProcessError( const wstring &msg, unsigned int const line_number )
{
#ifdef _DEBUG
	wcout << L"\tError: " << GetFileName() << L":" << line_number << L": " << msg << endl;
#endif

	const wstring &str_line_num = ToString( line_number );
	errors.insert( { line_number, GetFileName() + L":" + str_line_num + L": " + msg } );
}

/****************************
 * Checks for parsing errors
 * returns false if errors
 ****************************/
bool Parser::NoErrors()
{
	// check and process errors
	if ( errors.size() ) {
		for ( size_t i = 0; i < errors.size(); i++ ) {
			wcerr << errors[ i ] << endl;
		}
		// clean up
		return false;
	}

	return true;
}

// To-do Confirm exponentiation( x**2 ) precedence
static int GetBinaryOperatorPrecedence( ScannerTokenType tk )
{
	switch ( tk ){
	case ScannerTokenType::TOKEN_EXP:
		return 20;
	case ScannerTokenType::TOKEN_MUL:
	case ScannerTokenType::TOKEN_DIV:
	case ScannerTokenType::TOKEN_MOD:
		return 10;
	case ScannerTokenType::TOKEN_ADD:
	case ScannerTokenType::TOKEN_SUB:
		return 9;
	case ScannerTokenType::TOKEN_RSHIFT:
	case ScannerTokenType::TOKEN_LSHIFT:
		return 8;
	case ScannerTokenType::TOKEN_IN_ID:
	case ScannerTokenType::TOKEN_LES:
	case ScannerTokenType::TOKEN_GTR:
	case ScannerTokenType::TOKEN_LEQL:
	case ScannerTokenType::TOKEN_GEQL:
		return 7;
	case ScannerTokenType::TOKEN_EQL:
	case ScannerTokenType::TOKEN_NEQL:
		return 6;
	case ScannerTokenType::TOKEN_AND:
		return 5;
	case ScannerTokenType::TOKEN_XOR:
		return 4;
	case ScannerTokenType::TOKEN_OR:
		return 3;
	case ScannerTokenType::TOKEN_LAND:
		return 2;
	case ScannerTokenType::TOKEN_LOR:
		return 1;
		// One important note: this function must not return 0
	default:
		return -1;
	}
}

int Parser::GetTokenPrecedence()
{
	return GetBinaryOperatorPrecedence( CurrentToken().GetType() );
}

/****************************
 * Starts the parsing process.
 ****************************/
std::unique_ptr<ParsedProgram> Parser::Parse()
{
#ifdef _DEBUG
	std::wcout << L"\n========== Scanning/Parsing =========" << std::endl;
#endif

	NextToken();

	std::unique_ptr<ParsedProgram> program{ new ParsedProgram };
	auto program_scope = ParseScope( program->GetGlobalScope() );
	if ( !program_scope ){
		return nullptr;
	}
	program_scope->SetScopeType( ScopeType::NAMESPACE_SCOPE );
	program->SetConstructs( program_scope );
	if ( NoErrors() ) {
		return program;
	}

	return nullptr;
}

Declaration* Parser::ParseClass( Scope *parent_scope, AccessType access_type, StorageType storage_type )
{
	unsigned int const line_num = GetLineNumber();
	std::wstring const &file_name = GetFileName();

	bool is_struct = CurrentToken().GetType() == ScannerTokenType::TOKEN_STRUCT_ID;
	NextToken(); // consume 'class' or 'struct'

	if ( !Match( ScannerTokenType::TOKEN_IDENT ) ) {
		ProcessError( ScannerTokenType::TOKEN_IDENT );
		return nullptr;
	}

#ifdef _DEBUG
	std::wcout << L"Class: name='" + scanner->GetToken()->GetIdentifier() + L"'\n";
#endif

	ClassDeclaration *klass = new ClassDeclaration( file_name, line_num, CurrentToken().GetIdentifier(), parent_scope, is_struct );
	NextToken(); // consume class name

	// we have a base/super class
	if ( Match( ScannerTokenType::TOKEN_COLON ) ){
		NextToken(); // consume ':'
		if ( !Match( ScannerTokenType::TOKEN_IDENT ) ){
			ProcessError( L"Expected a super-class name" );
		}
		else {
			klass->SetBaseClass( CurrentToken().GetIdentifier() );
			NextToken();
		}
	}

	if ( !Match( ScannerTokenType::TOKEN_OPEN_BRACE ) ) {
		ProcessError( ScannerTokenType::TOKEN_OPEN_BRACE );
		delete klass;
		klass = nullptr;

		return nullptr;
	}
	NextToken(); // consume '{'
	while ( !Match( ScannerTokenType::TOKEN_CLOSED_BRACE ) && !Match( ScannerTokenType::TOKEN_END_OF_STREAM ) ){
		Declaration *decl = ParseDeclaration( parent_scope );
		if ( !decl ){
			delete klass;
			klass = nullptr;
			return klass;
		}
		if ( !klass->AddDeclaration( decl ) ){
			ProcessError( L"'" + decl->GetName() + L"' already exist in this scope", decl->GetLineNumber() );
		}
	}
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_BRACE ) ){
		ProcessError( L"Expected a closing brace at the end of class declaration." );
		delete klass;
		klass = nullptr;
		return klass; // klass is nullptr
	}
	NextToken(); // consume '}'
	klass->SetStorageType( storage_type );
	klass->SetAccessType( access_type );
	return klass;
}

Declaration* Parser::ParseFunction( Scope *parent_scope, FunctionType function_type, AccessType access, StorageType storage )
{
	unsigned int const line_num = GetLineNumber();
	std::wstring const &file_name = GetFileName();
	std::wstring stype{};

	switch ( function_type )
	{
	case FunctionType::CONSTRUCTOR:
		stype = L"Constructor";
		if ( !Match( ScannerTokenType::TOKEN_CONSTRUCT_ID ) ){
			ProcessError( ScannerTokenType::TOKEN_CONSTRUCT_ID );
			return nullptr;
		}
		break;
	case FunctionType::FUNCTION:
		stype = L"Function";
		if ( !Match( ScannerTokenType::TOKEN_FUNC_ID ) ){
			ProcessError( ScannerTokenType::TOKEN_FUNC_ID );
			return nullptr;
		}
		break;
	case FunctionType::METHOD:
	default:
		stype = L"Method";
		if ( !Match( ScannerTokenType::TOKEN_METHOD_ID ) ){
			ProcessError( ScannerTokenType::TOKEN_METHOD_ID );
			return nullptr;
		}
		break;
	}

	NextToken(); // consume keywords: function, method or construct

	if ( !Match( ScannerTokenType::TOKEN_IDENT ) ) {
		ProcessError( ScannerTokenType::TOKEN_IDENT );
		return nullptr;
	}

	std::wstring const function_name = scanner->GetToken()->GetIdentifier();
	NextToken();

#ifdef _DEBUG
	std::wcout << stype + L": name='" + function_name + L"'\n";
#endif

	ExpressionList* parameters{};

	if ( !Match( ScannerTokenType::TOKEN_OPEN_PAREN ) ){
		ProcessError( L"Expected an open parenthesis for (possibly empty) parameters." );
		return nullptr;
	}
	NextToken(); // consume '('
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
		parameters = new ExpressionList( file_name, line_num );
	}
	while ( !Match( ScannerTokenType::TOKEN_END_OF_STREAM ) && !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
		Expression* expr{ ParseExpression() };
		if ( !expr ){
			delete parameters;
			parameters = nullptr;

			ProcessError( L"Could not process parameters to functions." );
			return nullptr;
		}
		parameters->AddExpression( expr );
		if ( Match( ScannerTokenType::TOKEN_COMMA ) ) {
			NextToken(); // consume ','
			if ( Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ) {
				ProcessError( L"Expected an expression before a closing parenthesis", CurrentToken().GetLineNumber() );
			}
		}
	}
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
		ProcessError( ScannerTokenType::TOKEN_CLOSED_PAREN );

		delete parameters;
		parameters = nullptr;

		return nullptr;
	}
	NextToken(); // consume ')'

	// let's parse the function body
	CompoundStatement* function_body = ParseCompoundStatement( parent_scope );
	if ( !function_body ){
		delete parameters;
		parameters = nullptr;

		return nullptr;
	}
	function_body->GetScope()->SetScopeType( ScopeType::FUNCTION_SCOPE );

	FunctionDeclaration* function{ new FunctionDeclaration( file_name, line_num, function_name, std::move( parameters ) ) };
	function->SetFunctionBody( function_body );
	function->SetFunctionType( function_type );
	function->SetAccess( access );
	function->SetStorageType( storage );
	return function;
}

CompoundStatement* Parser::ParseCompoundStatement( Scope *parent )
{
	if ( !Match( ScannerTokenType::TOKEN_OPEN_BRACE ) ){
		ProcessError( ScannerTokenType::TOKEN_OPEN_BRACE );
		return nullptr;
	}
	NextToken(); // consume '{'
	unsigned int const line_num = GetLineNumber();
	auto const file_name = GetFileName();

	Scope* statement_scope{ ParseScope( parent ) };
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_BRACE ) ){
		ProcessError( ScannerTokenType::TOKEN_CLOSED_BRACE );

		delete statement_scope;
		statement_scope = nullptr;
		return nullptr;
	}
	NextToken(); // consume '}'

	return new CompoundStatement( file_name, line_num, statement_scope );
}

Scope* Parser::ParseScope( Scope *parent_scope )
{
	Scope* scope{ new Scope( parent_scope ) };
	while ( !Match( ScannerTokenType::TOKEN_END_OF_STREAM ) && !Match( ScannerTokenType::TOKEN_CLOSED_BRACE ) ) {
		Statement* statement = ParseStatement( parent_scope );
		if ( !statement ) {
			delete scope;
			scope = nullptr;
			return scope;
		}
		scope->AddStatement( statement );
	}
	return scope;
}

Statement* Parser::ParseStatement( Scope *parent_scope )
{
	switch ( CurrentToken().GetType() )
	{
	case ScannerTokenType::TOKEN_CASE_ID:
	case ScannerTokenType::TOKEN_ELSE_ID:
		return ParseLabelledStatement( parent_scope );

	case ScannerTokenType::TOKEN_BLOCK:
		NextToken(); // consume 'block'
		return ParseCompoundStatement( parent_scope );

	case ScannerTokenType::TOKEN_FOR_EACH_ID:
	case ScannerTokenType::TOKEN_FOR_ID:
	case ScannerTokenType::TOKEN_DO_ID:
	case ScannerTokenType::TOKEN_WHILE_ID:
	case ScannerTokenType::TOKEN_LOOP_ID:
		return ParseIterationStatement( parent_scope );

	case ScannerTokenType::TOKEN_SWITCH_ID:
		return ParseSwitchStatement( parent_scope );

	case ScannerTokenType::TOKEN_IF_ID:
		return ParseIfStatement( parent_scope );

	case ScannerTokenType::TOKEN_BREAK_ID:
	case ScannerTokenType::TOKEN_CONTINUE_ID:
	case ScannerTokenType::TOKEN_RETURN_ID:
		return ParseJumpStatement( parent_scope );

	case ScannerTokenType::TOKEN_CLASS_ID:
	case ScannerTokenType::TOKEN_STRUCT_ID:
	case ScannerTokenType::TOKEN_FUNC_ID:
	case ScannerTokenType::TOKEN_METHOD_ID:
	case ScannerTokenType::TOKEN_CONSTRUCT_ID:
	case ScannerTokenType::TOKEN_EXTERN_ID:
	case ScannerTokenType::TOKEN_STATIC_ID:
	case ScannerTokenType::TOKEN_PRIVATE_ID:
	case ScannerTokenType::TOKEN_PUBLIC_ID:
	case ScannerTokenType::TOKEN_PROTECTED_ID:
	case ScannerTokenType::TOKEN_VAR_ID:
	case ScannerTokenType::TOKEN_CONST_ID:
		return ParseDeclaration( parent_scope );
	case ScannerTokenType::TOKEN_SEMI_COLON:
		return ParseEmptyStatement( parent_scope );

	case ScannerTokenType::TOKEN_OPEN_BRACE:
	case ScannerTokenType::TOKEN_IDENT:
	case ScannerTokenType::TOKEN_NOT:
	case ScannerTokenType::TOKEN_CHAR_LIT:
	case ScannerTokenType::TOKEN_CHAR_STRING_LIT:
	case ScannerTokenType::TOKEN_DECR:
	case ScannerTokenType::TOKEN_INCR:
	case ScannerTokenType::TOKEN_OPEN_PAREN:
	case ScannerTokenType::TOKEN_TRUE_LIT:
	case ScannerTokenType::TOKEN_FALSE_LIT:
	case ScannerTokenType::TOKEN_INT_LIT:
	case ScannerTokenType::TOKEN_FLOAT_LIT:
	case ScannerTokenType::TOKEN_AT:
	case ScannerTokenType::TOKEN_NEW:
		return ParseExpressionStatement();
	case ScannerTokenType::TOKEN_SHOW_ID:
		return ParseShowStatement( parent_scope );
	default:
		ProcessError( L"Expected a statement here." );
		return nullptr;
	}
}

Statement* Parser::ParseShowStatement( Scope *parent_scope )
{
	Token const tok = CurrentToken();
	NextToken(); // consume 'show'
	Expression* expr{ ParseExpression() };
	if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
		ProcessError( ScannerTokenType::TOKEN_SEMI_COLON );

		delete expr;
		expr = nullptr;

		return nullptr;
	}
	NextToken(); // consume ';'
	return TreeFactory::MakeShowExpressionStatement( tok, expr );
}

Statement* Parser::ParseIfStatement( Scope *parent_scope )
{
	unsigned int const line_num = GetLineNumber();
	std::wstring const file_name = GetFileName();

	if ( !Match( ScannerTokenType::TOKEN_IF_ID ) ) {
		ProcessError( ScannerTokenType::TOKEN_IF_ID );
		return nullptr;
	}
	NextToken();

	if ( !Match( ScannerTokenType::TOKEN_OPEN_PAREN ) ) {
		ProcessError( ScannerTokenType::TOKEN_OPEN_PAREN );
		return nullptr;
	}
	NextToken();

	Expression* logical_expression{ ParseExpression() };
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ) {
		ProcessError( ScannerTokenType::TOKEN_CLOSED_PAREN );

		delete logical_expression;
		logical_expression = nullptr;
		return nullptr;
	}
	NextToken();

	Statement* then_statement{ ParseCompoundStatement( parent_scope ) };

	if ( !then_statement ){
		ProcessError( L"Unable to parse the statement in the IF statement." );

		delete logical_expression;
		logical_expression = nullptr;

		return  nullptr;
	}
	// let's see if there's an else part
	Statement* else_statement{};

	if ( Match( ScannerTokenType::TOKEN_ELSE_ID ) ){
		else_statement = ParseCompoundStatement( parent_scope );
		if ( !else_statement ){
			ProcessError( L"Error while processing the else part of the if statement." );

			delete logical_expression;
			delete then_statement;
			logical_expression = nullptr;
			then_statement = nullptr;

			return nullptr;
		}
	}

	IfStatement* if_statement{ new IfStatement( file_name, line_num, logical_expression, then_statement, else_statement ) };
	return if_statement;
}

Statement* Parser::ParseIterationStatement( Scope *parent_scope )
{
	unsigned int const line_num = GetLineNumber();
	std::wstring const file_name = GetFileName();

	Statement* iterative_statement{};
	ScannerTokenType const tk = CurrentToken().GetType();

	if ( tk == ScannerTokenType::TOKEN_LOOP_ID ){
		Token const token = CurrentToken();
		NextToken(); // consume 'loop'
		CompoundStatement* loop_body{ ParseCompoundStatement( parent_scope ) };
		if ( !loop_body ) return nullptr;
		return new LoopStatement( token.GetFileName(), token.GetLineNumber(), loop_body );
	}
	else if ( tk == ScannerTokenType::TOKEN_DO_ID ){
		NextToken(); // consume 'do'
		Statement* do_body{ ParseCompoundStatement( parent_scope ) };
		if ( !Match( ScannerTokenType::TOKEN_WHILE_ID ) ){
			ProcessError( ScannerTokenType::TOKEN_WHILE_ID );

			delete do_body;
			do_body = nullptr;
			return nullptr;
		}
		NextToken(); // consume token 'while'
		if ( !Match( ScannerTokenType::TOKEN_OPEN_PAREN ) ){
			ProcessError( L"Expected an open parenthesis after the `while` keyword" );

			delete do_body;
			do_body = nullptr;
			return nullptr;
		}
		NextToken(); // consume '('
		Expression* expression{ ParseExpression() };
		if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			delete expression;
			delete do_body;
			expression = nullptr;
			do_body = nullptr;
			ProcessError( L"Expected a closing parenthesis after the expression" );
			return nullptr;
		}
		NextToken(); // consume ')'
		if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
			ProcessError( L"Expected a semi-colon after the closing parenthesis" );
			delete expression;
			delete do_body;
			expression = nullptr;
			do_body = nullptr;

			return nullptr;
		}
		NextToken(); // consume ';'
		return new DoWhileStatement( file_name, line_num, do_body, expression );
	}
	else if ( tk == ScannerTokenType::TOKEN_WHILE_ID ){
		NextToken(); // consume 'while'
		if ( !Match( ScannerTokenType::TOKEN_OPEN_PAREN ) ){
			ProcessError( ScannerTokenType::TOKEN_OPEN_PAREN );
			return nullptr;
		}
		NextToken(); // consume '('
		Expression* expression{ ParseExpression() };
		if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			ProcessError( ScannerTokenType::TOKEN_CLOSED_PAREN );
			delete expression;
			expression = nullptr;
			return nullptr;
		}
		NextToken(); //consume ')'
		Statement* statement{ ParseCompoundStatement( parent_scope ) };
		if ( expression && statement ){
			return new WhileStatement( file_name, line_num, expression, statement );
		}
		delete expression;
		expression = nullptr;
		return nullptr;
	}
	// for_each statement, MAY be written( note the space ) as for each( ... ) or foreach( ... )
	bool is_two_word_foreach = ( tk == ScannerTokenType::TOKEN_FOR_ID && Match( ScannerTokenType::TOKEN_EACH_ID, SECOND_INDEX ) );
	if ( is_two_word_foreach ){
		NextToken(); // consume 'for'
	}
	NextToken(); // consume 'foreach' or in case of two worded for each, consume 'each'
	if ( !Match( ScannerTokenType::TOKEN_OPEN_PAREN ) ){
		ProcessError( ScannerTokenType::TOKEN_OPEN_PAREN );
		return nullptr;
	}
	NextToken(); // consume '('
	Expression* for_each_expr{ ParseExpression() };

	NextToken(); // consume ')'
	Statement* for_each_body_statement{ ParseCompoundStatement( parent_scope ) };

	if ( for_each_expr && for_each_body_statement ){
		return new ForEachStatement( file_name, line_num, for_each_expr, for_each_body_statement );
	}

	delete for_each_expr;
	delete for_each_body_statement;
	for_each_expr = nullptr;
	for_each_body_statement = nullptr;

	return nullptr;
}

Statement* Parser::ParseLabelledStatement( Scope *parent )
{
	Token const tok = CurrentToken();

	switch ( tok.GetType() ){
	case ScannerTokenType::TOKEN_ELSE_ID:
		NextToken();
		if ( !Match( ScannerTokenType::TOKEN_COLON ) ){
			ProcessError( ScannerTokenType::TOKEN_COLON );
			return nullptr;
		}
		NextToken(); // consume ':'
		break;
	case ScannerTokenType::TOKEN_CASE_ID:
	{
		NextToken(); // consume 'case'
		Expression* expr{ ParseConditionalExpression() };
		if ( !expr ){
			return nullptr;
		}
		if ( !Match( ScannerTokenType::TOKEN_COLON ) ){
			ProcessError( ScannerTokenType::TOKEN_COLON );
			delete expr;
			expr = nullptr;
			return nullptr;
		}
		NextToken(); // consume ':'
		Statement* statement{ ParseStatement( parent ) };
		if ( !statement ){
			delete expr;
			expr = nullptr;
			return nullptr;
		}
		return new CaseStatement( tok.GetFileName(), tok.GetLineNumber(), expr, statement );
	}
	default:
		ProcessError( L"Identifier allowed in this scope is 'else' and 'case'" );
		return nullptr;
	}
	Statement* statement{ ParseStatement( parent ) };
	if ( !statement ){
		return nullptr;
	}
	return new LabelledStatement( tok.GetFileName(), tok.GetLineNumber(), tok.GetIdentifier(), statement );
}

Statement* Parser::ParseSwitchStatement( Scope *parent_scope )
{
	Token const tok = CurrentToken();
	NextToken(); // consume 'switch'
	if ( !Match( ScannerTokenType::TOKEN_OPEN_PAREN ) ){
		ProcessError( ScannerTokenType::TOKEN_OPEN_PAREN );
		return nullptr;
	}
	NextToken(); // consume '('
	Expression* switch_expression{ ParseExpression() };
	if ( !switch_expression ) return nullptr;

	if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
		ProcessError( ScannerTokenType::TOKEN_CLOSED_PAREN );
		delete switch_expression;
		switch_expression = nullptr;
		return nullptr;
	}
	NextToken(); // consume ')'
	Statement* switch_body{ ParseCompoundStatement( parent_scope ) };
	if ( !switch_body ) {
		delete switch_expression;
		return nullptr;
	}
	return new SwitchStatement( tok.GetFileName(), tok.GetLineNumber(), switch_expression, switch_body );
}

Statement* Parser::ParseJumpStatement( Scope *parent )
{
	Token const tok = CurrentToken();
	switch ( tok.GetType() ){
	case ScannerTokenType::TOKEN_CONTINUE_ID:
		NextToken(); // consume 'continue'
		if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
			ProcessError( ScannerTokenType::TOKEN_SEMI_COLON );
			return nullptr;
		}
		NextToken(); // consume ';'
		return TreeFactory::MakeContinueStatement( tok.GetFileName(), tok.GetLineNumber() );
	case ScannerTokenType::TOKEN_BREAK_ID:
		NextToken(); // consume 'break'
		if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
			ProcessError( ScannerTokenType::TOKEN_SEMI_COLON );
			return nullptr;
		}
		NextToken(); // consume ';'
		return TreeFactory::MakeBreakStatement( tok.GetFileName(), tok.GetLineNumber() );
	case ScannerTokenType::TOKEN_RETURN_ID:
	{
		NextToken(); // consume 'return'
		Expression* expression{};
		if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
			expression = ParseExpression();
		}
		if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
			ProcessError( ScannerTokenType::TOKEN_SEMI_COLON );
			delete expression;
			expression = nullptr;
			return nullptr;
		}
		NextToken(); // consume ';'
		return TreeFactory::MakeReturnStatement( tok.GetFileName(), tok.GetLineNumber(), expression );
	}
	default:
		ProcessError( L"Unexpected statement" );
		return nullptr;
	}
}

Statement* Parser::ParseExpressionStatement()
{
	Token const tok = CurrentToken();
	Expression* expression{ ParseExpression() };

	if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
		ProcessError( ScannerTokenType::TOKEN_SEMI_COLON );
		delete expression;
		expression = nullptr;
		return nullptr;
	}
	NextToken(); // consume ';'
	return TreeFactory::MakeExpressionStatement( tok.GetFileName(), tok.GetLineNumber(), expression );
}

Expression* Parser::ParseExpression()
{
	return ParseAssignmentExpression();
}

Expression* Parser::ParseAssignmentExpression()
{
	Expression* expr = ParseConditionalExpression();

	switch ( CurrentToken().GetType() ){
	case ScannerTokenType::TOKEN_ASSIGN:
	case ScannerTokenType::TOKEN_ADD_EQL:
	case ScannerTokenType::TOKEN_SUB_EQL:
	case ScannerTokenType::TOKEN_MUL_EQL:
	case ScannerTokenType::TOKEN_DIV_EQL:
	{
		auto const tok = CurrentToken();
		NextToken();
		return TreeFactory::MakeAssignmentExpression( tok, expr, ParseAssignmentExpression() );
	}
	default:;
	}
	return expr;
}

Expression*	Parser::ParseConditionalExpression()
{
	Expression* expression{ ParseBinaryExpression() };
	if ( !expression ){
		return nullptr;
	}
	if ( CurrentToken().GetType() == ScannerTokenType::TOKEN_QUESTION_MARK ){
		Token const tok = CurrentToken();
		NextToken(); // consume '?'
		Expression* lhs_expression{ ParseExpression() };

		if ( !Match( ScannerTokenType::TOKEN_COLON ) ){
			ProcessError( ScannerTokenType::TOKEN_COLON );

			delete expression;
			delete lhs_expression;
			expression = nullptr;
			lhs_expression = nullptr;
			return nullptr;
		}
		NextToken(); // consume ':'
		Expression* rhs_expression{ ParseExpression() };
		if ( lhs_expression && rhs_expression ){
			return new ConditionalExpression( tok.GetFileName(), tok.GetLineNumber(), expression, lhs_expression, rhs_expression );
		}
		delete expression;
		delete lhs_expression;
		delete rhs_expression;
		expression = nullptr;
		lhs_expression = nullptr;
		rhs_expression = nullptr;
		return nullptr;
	}
	return expression;
}

Expression* Parser::ParseBinaryExpression()
{
	Expression* unary_expr{ ParseUnaryExpression() };
	int const lowest_precedence = 0;

	return ParseBinaryOpExpression( lowest_precedence, unary_expr );
}

Expression* Parser::ParseDictionaryExpression()
{
	if ( !Match( ScannerTokenType::TOKEN_OPEN_BRACE ) ){
		ProcessError( L"A map type begins with an opening brace and ends with a corresponding opening brace" );
		return nullptr;
	}

	auto clear_vector = [] ( std::vector<MapExpression::expression_ptr_pair_t> & vec ){
		if ( !vec.empty() ){
			for ( MapExpression::expression_ptr_pair_t &key_value : vec ){
				delete key_value.first;
				delete key_value.second;
				key_value.first = nullptr;
				key_value.second = nullptr;
			}
		}
	};
	Token const token = CurrentToken();
	NextToken(); // consume '{'
	std::vector<MapExpression::expression_ptr_pair_t> key_datum_list{};
	while ( !Match( ScannerTokenType::TOKEN_END_OF_STREAM ) && !Match( ScannerTokenType::TOKEN_CLOSED_BRACE ) ){
		auto key_expression = ParseExpression();
		if ( !Match( ScannerTokenType::TOKEN_COLON ) ){
			ProcessError( L"Expects a colon as a map separator" );
			delete key_expression;
			key_expression = nullptr;
			clear_vector( key_datum_list );
			return nullptr;
		}
		NextToken(); // consume ':'
		auto value_expression = ParseExpression();
		if ( !( key_expression && value_expression ) ){
			ProcessError( L"Unable to parse key/value expression for map" );
			delete key_expression;
			delete value_expression;
			key_expression = nullptr;
			value_expression = nullptr;
			clear_vector( key_datum_list );
			return nullptr;
		}
		key_datum_list.push_back( { std::move( key_expression ), std::move( value_expression ) } );
		if ( Match( ScannerTokenType::TOKEN_COMMA ) ){
			NextToken(); // consume ','
		}
	}
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_BRACE ) ){
		ProcessError( L"Expected a closing brace before expression", CurrentToken().GetType() );
	}
	NextToken(); // consume '}' preferably or any encounterred token
	return TreeFactory::MakeMapExpression( token, std::move( key_datum_list ) );
}

Expression* Parser::ParseListExpression()
{
	if ( !Match( ScannerTokenType::TOKEN_OPEN_BRACKET ) ){
		ProcessError( ScannerTokenType::TOKEN_OPEN_BRACKET );
		return nullptr;
	}
	Token const tok = CurrentToken();
	NextToken(); // consume '['
	ExpressionList* list_params{};
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_BRACKET ) ){
		list_params = new ExpressionList( CurrentToken().GetFileName(), CurrentToken().GetLineNumber() );
	}
	while ( !Match( ScannerTokenType::TOKEN_END_OF_STREAM ) && !Match( ScannerTokenType::TOKEN_CLOSED_BRACKET ) ){
		list_params->AddExpression( ParseExpression() );
		if ( !Match( ScannerTokenType::TOKEN_COMMA ) ) break;
		NextToken(); // consume ','
	}
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_BRACKET ) ){
		ProcessError( ScannerTokenType::TOKEN_CLOSED_BRACKET );
		if ( list_params ){
			delete list_params;
			list_params = nullptr;
		}
		return nullptr;
	}
	NextToken(); // consume ']'
	return TreeFactory::MakeListExpression( tok, list_params );
}

Expression* Parser::ParseUnaryExpression()
{
	Token const token = CurrentToken();
	switch ( token.GetType() ){
	case ScannerTokenType::TOKEN_INCR:
		NextToken(); // consume ++
		return TreeFactory::MakePreIncrExpression( token, ParseUnaryExpression() );
	case ScannerTokenType::TOKEN_DECR:
		NextToken(); // consume --
		return TreeFactory::MakePreDecrExpression( token, ParseUnaryExpression() );
	case ScannerTokenType::TOKEN_NOT:
	case ScannerTokenType::TOKEN_SUB:
		NextToken(); // consume operator not '!' or unary minus '-'
		return TreeFactory::MakeUnaryOperation( token, ParseUnaryExpression() );
	default:;
	}

	return ParsePostfixExpression();
}

Expression* Parser::ParsePrimaryExpression()
{
	Token const tok = CurrentToken();
	switch ( tok.GetType() )
	{
	case ScannerTokenType::TOKEN_NEW:
	{
		NextToken();
		return TreeFactory::MakeNewExpression( tok, ParseExpression() );
	}
	case ScannerTokenType::TOKEN_NULL:
		NextToken();
		return TreeFactory::MakeNullLitExpression( tok );
	case ScannerTokenType::TOKEN_IDENT:
		NextToken();
		return TreeFactory::MakeVariable( tok );
	case ScannerTokenType::TOKEN_INT_LIT:
		NextToken();
		return TreeFactory::MakeIntegerLiteral( tok );
	case ScannerTokenType::TOKEN_FLOAT_LIT:
		NextToken();
		return TreeFactory::MakeFloatLiteral( tok );

	case ScannerTokenType::TOKEN_CHAR_STRING_LIT:
		NextToken();
		return TreeFactory::MakeStringLiteral( tok );

	case ScannerTokenType::TOKEN_CHAR_LIT:
		NextToken();
		return TreeFactory::MakeCharLiteral( tok );
	case ScannerTokenType::TOKEN_TRUE_LIT:
	case ScannerTokenType::TOKEN_FALSE_LIT:
		NextToken();
		return TreeFactory::MakeBooleanLiteral( tok );
	case ScannerTokenType::TOKEN_AT:
		return ParseLambdaExpression();
	case ScannerTokenType::TOKEN_OPEN_BRACE:
		return ParseDictionaryExpression();
	case ScannerTokenType::TOKEN_OPEN_BRACKET:
		return ParseListExpression();
	case ScannerTokenType::TOKEN_OPEN_PAREN:
	{
		NextToken(); // consume '('
		Expression* expr{ ParseExpression() };
		if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			ProcessError( L"Expected a closing parenthesis before", scanner->GetToken( SECOND_INDEX )->GetType() );
			delete expr;
			expr = nullptr;
			return nullptr;
		}
		NextToken(); // consume ')'
		return expr;
	}
	default:
		ProcessError( L"Expected an identifier, constant, string-literal, or a parenthesized expression." );
	} // end-switch
	return nullptr;
}

ExpressionList* Parser::ParseArgumentExpressionList()
{
	ExpressionList* argList{};
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
		argList = new ExpressionList( CurrentToken().GetFileName(), CurrentToken().GetLineNumber() );
		while ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			Expression* expr{ ParseAssignmentExpression() };
			if ( !expr ) {
				delete argList;
				argList = nullptr;

				return nullptr;
			}
			argList->AddExpression( expr );
			if ( Match( ScannerTokenType::TOKEN_COMMA ) ){
				NextToken();
			}
		}
	}
	return argList;
}

Expression* Parser::ParseLambdaExpression()
{
	Token const token = CurrentToken();
	if ( !Match( ScannerTokenType::TOKEN_AT ) ){
		ProcessError( L"Expected a lambda expresion to start with an @ symbol" );
		return nullptr;
	}
	ExpressionList* lambda_parameters{};
	NextToken(); // consume '@'
	// parameter body, i.e. (),  is optional
	if ( Match( ScannerTokenType::TOKEN_OPEN_PAREN ) ){
		NextToken(); // consume '('
		if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			lambda_parameters = new ExpressionList( token.GetFileName(), token.GetLineNumber() );
		}
		while ( !Match( ScannerTokenType::TOKEN_END_OF_STREAM ) && !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			lambda_parameters->AddExpression( ParseExpression() );
			if ( !Match( ScannerTokenType::TOKEN_COMMA ) ) break;
			NextToken(); // consume ','
		}
		if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			ProcessError( ScannerTokenType::TOKEN_CLOSED_PAREN );
		}
		NextToken(); // consume ')'
	}

	return new LambdaExpression( token, lambda_parameters, ParseCompoundStatement( nullptr ) );
}

Expression* Parser::ParsePostfixExpression()
{
	Expression* expr{ ParsePrimaryExpression() };

	while ( true )
	{
		switch ( CurrentToken().GetType() )
		{
		case ScannerTokenType::TOKEN_OPEN_PAREN: {// function call
			auto const token = CurrentToken();
			NextToken(); // consume '('
			// parse argument_list and expect ')'
			ExpressionList* argExprList{ ParseArgumentExpressionList() };
			if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ) {
				ProcessError( L"Expects a closing parenthesis before the next token." );
				delete expr;
				delete argExprList;
				expr = nullptr;
				argExprList = nullptr;
				return nullptr;
			}
			NextToken(); // consume ')'

			expr = new FunctionCall( token.GetFileName(), token.GetLineNumber(), expr, argExprList );
			break;
		}
		case ScannerTokenType::TOKEN_OPEN_BRACKET: // array subscript
		{
			NextToken(); // consume '['
			Expression* subscript_expression{ ParseExpression() };
			if ( !Match( ScannerTokenType::TOKEN_CLOSED_BRACKET ) ){
				ProcessError( ScannerTokenType::TOKEN_CLOSED_BRACKET );
				delete expr;
				expr = nullptr;
				return nullptr;
			}
			NextToken(); // consume ']'
			expr = new SubscriptExpression( CurrentToken().GetFileName(), CurrentToken().GetLineNumber(), expr, subscript_expression );
			break;
		}
		case ScannerTokenType::TOKEN_PERIOD:
		{
			NextToken(); // consume '.'
			Token const curr_token = CurrentToken();
			if ( !Match( ScannerTokenType::TOKEN_IDENT ) ){
				ProcessError( L"Expected an identifier after the dot operator." );
				delete expr;
				expr = nullptr;
				return nullptr;
			}
			NextToken(); // consume identifier
			expr = new DotExpression( curr_token.GetFileName(), curr_token.GetLineNumber(), curr_token, expr );
			break;
		}
		case ScannerTokenType::TOKEN_INCR:
		{
			NextToken(); // consume '++'
			auto curr_token = CurrentToken();
			expr = new PostIncrExpression( curr_token.GetFileName(), curr_token.GetLineNumber(), expr );
			break;
		}
		case ScannerTokenType::TOKEN_DECR:
		{
			NextToken(); // consume '--'
			auto curr_token = CurrentToken();
			expr = new PostDecrExpression( curr_token.GetFileName(), curr_token.GetLineNumber(), expr );
			break;
		}
		default:
			return expr;

		} // end-switch
	} // end-while
	return expr;
} // end-ParsePostfixExpression

// Precedence Climbing
Expression* Parser::ParseBinaryOpExpression( int const precedence, Expression* unary_expr )
{
	while ( true ){
		int const currentPrecedence = GetTokenPrecedence();
		if ( currentPrecedence < precedence ){
			return unary_expr;
		}

		Token const tok = CurrentToken();
		NextToken(); // consume current operator
		Expression* second_expr{ ParseUnaryExpression() };
		int const nextPrecedence = GetTokenPrecedence();
		if ( currentPrecedence < nextPrecedence ){
			second_expr = ParseBinaryOpExpression( currentPrecedence + 1, second_expr );
		}
		unary_expr = new BinaryExpression( tok, unary_expr, second_expr );
	}
}

Declaration* Parser::ParseDeclaration( Scope *parent_scope )
{
	AccessType access_type = AccessType::NONE;
	StorageType storage_type = StorageType::NONE;
	ScannerTokenType tt = CurrentToken().GetType();

	switch ( tt ){
	case ScannerTokenType::TOKEN_PRIVATE_ID:
		access_type = AccessType::PRIVATE_ACCESS;
		NextToken();
		break;
	case ScannerTokenType::TOKEN_PUBLIC_ID:
		access_type = AccessType::PUBLIC_ACCESS;
		NextToken();
		break;
	case ScannerTokenType::TOKEN_PROTECTED_ID:
		access_type = AccessType::PROTECTED_ACCESS;
		NextToken();
		break;
	default:;
	}

	if ( CurrentToken().GetType() == ScannerTokenType::TOKEN_STATIC_ID ||
		CurrentToken().GetType() == ScannerTokenType::TOKEN_EXTERN_ID ){
		storage_type = CurrentToken().GetType() == ScannerTokenType::TOKEN_STATIC_ID ? StorageType::STATIC_STORAGE :
			StorageType::EXTERN_STORAGE;
		NextToken(); // consume 'extern' or 'static'
	}

	if ( Match( ScannerTokenType::TOKEN_SEMI_COLON ) && storage_type != StorageType::NONE && access_type != AccessType::NONE ){
		ProcessError( L"access or storage specifier cannot be used here" );
	}

	switch ( CurrentToken().GetType() ){
	case ScannerTokenType::TOKEN_CLASS_ID:
	case ScannerTokenType::TOKEN_STRUCT_ID:
		return ParseClass( parent_scope, access_type, storage_type );

	case ScannerTokenType::TOKEN_FUNC_ID:
	case ScannerTokenType::TOKEN_METHOD_ID:
	case ScannerTokenType::TOKEN_CONSTRUCT_ID: {
		auto const type = CurrentToken().GetType();
		FunctionType function_type = type == ScannerTokenType::TOKEN_FUNC_ID ? FunctionType::FUNCTION :
			type == ScannerTokenType::TOKEN_METHOD_ID ? FunctionType::METHOD : FunctionType::CONSTRUCTOR;

		return ParseFunction( parent_scope, function_type, access_type, storage_type );
	}
	case ScannerTokenType::TOKEN_VAR_ID:
	case ScannerTokenType::TOKEN_CONST_ID:
		return ParseVariableDeclaration( parent_scope, access_type, storage_type );
	default:
		ProcessError( L"Expected a variable, class/struct, function declaration here." );
		return nullptr;
	}
}

Declaration* Parser::ParseVariableDeclaration( Scope *parent_scope, AccessType access_type, StorageType storage_type )
{
	bool const is_const = CurrentToken().GetType() == ScannerTokenType::TOKEN_CONST_ID;
	Token const token = CurrentToken();
	NextToken(); // consume 'var' or 'const'
	DeclarationList::declaration_list_t decl_list{};
#ifdef _DEBUG
	std::wcout << L"\n===========Declaration of variable==============\n\t" << std::endl;
#endif

	auto clear_list = [] ( DeclarationList::declaration_list_t & list ){
		for ( auto &element : list ){
			delete element.second;
			element.second = nullptr;
		}
		list.clear();
	};
	do {
		if ( !Match( ScannerTokenType::TOKEN_IDENT ) ){
			ProcessError( L"expected an valid identifier" );
			clear_list( decl_list );
			return nullptr;
		}
		Token curr_token = CurrentToken();

		NextToken(); // consume the identifier
		Expression* assignment_expr{};

		if ( Match( ScannerTokenType::TOKEN_ASSIGN ) ){
			NextToken(); // consume '='
			assignment_expr = ParseExpression();
			if ( !assignment_expr ){
				clear_list( decl_list );
				return nullptr;
			}
		}

		VariableDeclaration* decl{ new VariableDeclaration( curr_token.GetFileName(), curr_token.GetLineNumber(), 
			curr_token.GetIdentifier(), assignment_expr, is_const ) };
		decl->SetAccessType( access_type );
		decl->SetStorageType( storage_type );
#ifdef _DEBUG
		std::wcout << curr_token.GetIdentifier() << ", " << std::endl;
#endif

		decl_list.insert( { curr_token.GetIdentifier(), decl } );
		if ( Match( ScannerTokenType::TOKEN_COMMA ) ){
			NextToken(); // consume ','
		}
	} while ( !Match( ScannerTokenType::TOKEN_END_OF_STREAM ) && !Match( ScannerTokenType::TOKEN_SEMI_COLON ) );
#ifdef _DEBUG
	std::wcout << L"\n========== End of variable declaration =========" << std::endl;
#endif
	if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
		ProcessError( L"Expected a semi-colon(;) at the end of variable/constant declaration." );
		clear_list( decl_list );
		return nullptr;
	}
	NextToken(); // consume ';'
	return new DeclarationList( token.GetFileName(), token.GetLineNumber(), std::move( decl_list ) );
}

Statement* Parser::ParseEmptyStatement( Scope * )
{
	auto token = CurrentToken();
	NextToken(); // consume ';'
	return new EmptyStatement( token.GetFileName(), token.GetLineNumber() );
}