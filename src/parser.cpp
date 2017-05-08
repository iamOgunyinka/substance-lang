/***************************************************************************
 * Language parser
 *
 * Copyright (c) 2017 Joshua Ogunyinka
 * Copyright (c) 2013-2016 Randy Hollines
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
void Parser::ProcessError( const wstring &msg, ParseNode* node )
{
#ifdef _DEBUG
	wcout << L"\tError: " << node->GetFileName() << L":" << node->GetLineNumber()
		<< L": " << msg << endl;
#endif

	const wstring &str_line_num = ToString( node->GetLineNumber() );
	errors.insert( { node->GetLineNumber(), node->GetFileName() + L":" + str_line_num + L": " + msg } );
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

AccessType Parser::DetermineAccessType( ScannerTokenType tt )
{
	switch ( tt ){
	case ScannerTokenType::TOKEN_STATIC_ID:
		return AccessType::STATIC_ACCESS;
	case ScannerTokenType::TOKEN_PUBLIC_ID:
		return AccessType::PUBLIC_ACCESS;
	case ScannerTokenType::TOKEN_PRIVATE_ID:
		return AccessType::PRIVATE_ACCESS;
	case ScannerTokenType::TOKEN_PROTECTED_ID:
		return AccessType::PROTECTED_ACCESS;
	default:
		return AccessType::INVALID_ACCESS;
	}
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
	const unsigned int line_num = GetLineNumber();
	const wstring &file_name = GetFileName();

#ifdef _DEBUG
	std::wcout << L"\n========== Scanning/Parsing =========" << std::endl;
#endif

	NextToken();

	std::unique_ptr<ParsedProgram> program{ new ParsedProgram };
	auto program_scope = ParseScope( program->GetGlobalScope().get() );
	if ( !program_scope ){
		ProcessError( L"Fatal error. Unable to parse program." );
		std::exit( -1 );
	}
	program->SetConstructs( std::move( program_scope ) );
	if ( NoErrors() ) {
		return program;
	}

	return nullptr;
}

std::unique_ptr<ParsedClass> Parser::ParseClass( Scope *parent_scope )
{
	unsigned int const line_num = GetLineNumber();
	std::wstring const &file_name = GetFileName();

	NextToken(); // consume 'class'

	if ( !Match( ScannerTokenType::TOKEN_IDENT ) ) {
		ProcessError( ScannerTokenType::TOKEN_IDENT );
		return nullptr;
	}

#ifdef _DEBUG
	Show( L"Class: name='" + scanner->GetToken()->GetIdentifier() + L"'", 2 );
#endif

	std::unique_ptr<ParsedClass> klass{ new ParsedClass( file_name, line_num, scanner->GetToken()->GetIdentifier(), parent_scope ) };
	NextToken();

	if ( !Match( ScannerTokenType::TOKEN_OPEN_BRACE ) ) {
		ProcessError( ScannerTokenType::TOKEN_OPEN_BRACE );
		return nullptr;
	}
	NextToken(); // consume '{'

	auto function_parser_handler = [&] ( FunctionType func_type, std::wstring const & stype )->bool {
		std::unique_ptr<ParsedFunction> function{ ParseFunction( parent_scope, func_type ) };
		if ( !function ) return false;

		if ( !klass->AddFunction( std::move( function ) ) ){
			ProcessError( stype + L" with the same name and similar signature exists in this scope." );
			return false;
		}
		return true;
	};

	while ( !Match( ScannerTokenType::TOKEN_END_OF_STREAM ) && !Match( ScannerTokenType::TOKEN_CLOSED_BRACE ) )
	{
		ScannerTokenType const tk_type = CurrentToken().GetType();
		bool access_specifier_used = false;

		if ( tk_type == ScannerTokenType::TOKEN_PRIVATE_ID || tk_type == ScannerTokenType::TOKEN_PUBLIC_ID
			|| tk_type == ScannerTokenType::TOKEN_PROTECTED_ID || ScannerTokenType::TOKEN_STATIC_ID == tk_type ){
			access_specifier_used = true;
			NextToken();
		}
		switch ( CurrentToken().GetType() )
		{
		case ScannerTokenType::TOKEN_VAR_ID:
		{
			std::unique_ptr<Statement> declaration_statement{ ParseDeclaration( klass->GetScope().get(), tk_type ) };
			if ( !declaration_statement ){
				return nullptr;
			}
			klass->AddStatement( std::move( declaration_statement ) );
			break;
		}
		case ScannerTokenType::TOKEN_FUNC_ID:
		{
			// function_parser_handler is a local lambda declared above
			if ( !function_parser_handler( FunctionType::FUNCTION, L"function" ) ) return nullptr;
			break;
		}
		case ScannerTokenType::TOKEN_METHOD_ID:
		{
			if ( !function_parser_handler( FunctionType::METHOD, L"method" ) ) return nullptr;
			break;
		}
		case ScannerTokenType::TOKEN_CONSTRUCT_ID:
		{
			if ( !function_parser_handler( FunctionType::CONSTRUCTOR, L"ctor" ) ) return nullptr;
			break;
		}
		case ScannerTokenType::TOKEN_CLASS_ID:
		{
			std::unique_ptr<ParsedClass> parsed_class{ ParseClass( klass->GetScope().get() ) };
			if ( !parsed_class ){
				return nullptr;
			}
			if ( !klass->AddClass( std::move( parsed_class ) ) ){
				ProcessError( L"Class with an identical name exists in this scope." );
				return nullptr;
			}
			break;
		}
		default:
			ProcessError( L"Invalid construct found ", CurrentToken().GetType() );
			return nullptr;
		}
	}

	if ( !Match( ScannerTokenType::TOKEN_CLOSED_BRACE ) ) {
		ProcessError( ScannerTokenType::TOKEN_CLOSED_BRACE );
		return nullptr;
	}
	NextToken(); // consume '}'

	return klass;
}

std::unique_ptr<ParsedFunction> Parser::ParseFunction( Scope *parent_scope, FunctionType function_type )
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
	Show( stype + L": name='" + function_name + L"'", 2 );
#endif

	std::unique_ptr<ExpressionList> parameters{};

	// methods w/o parameters are allowed to NOT have opening and closing parenthesis
	if ( !Match( ScannerTokenType::TOKEN_OPEN_PAREN ) ){
		if ( function_type != FunctionType::METHOD ){
			ProcessError( ScannerTokenType::TOKEN_OPEN_PAREN );
			/**************************************************************************
			*	let's see if we can re-construct this function tree, user maybe thought
			*	functions/constructors are also allowed to have optional parenthesis
			*	consume every token in our way up-to but not including the function body '{'
			***************************************************************************/
			while ( !Match( ScannerTokenType::TOKEN_END_OF_STREAM ) && !Match( ScannerTokenType::TOKEN_OPEN_BRACE ) ){
				NextToken();
			}
		}
	}
	else {
		NextToken(); // consume '('
		if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			parameters.reset( new ExpressionList( file_name, line_num ) );
		}
		while ( ScannerTokenType::TOKEN_END_OF_STREAM != CurrentToken().GetType() && !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			std::unique_ptr<Expression> expr{ ParseExpression() };
			if ( !expr ){
				ProcessError( L"Could not process parameters to functions." );
				return nullptr;
			}
			parameters->AddExpression( std::move( expr ) );
			if ( Match( ScannerTokenType::TOKEN_COMMA ) ) {
				NextToken(); // consume ','
				if ( Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ) {
					ProcessError( L"Expected expression", parameters.get() );
				}
			}
		}
		if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			ProcessError( ScannerTokenType::TOKEN_CLOSED_PAREN );
			return nullptr;
		}
		NextToken(); // consume ')'
	}

	// let's parse the function body
	std::unique_ptr<Statement> function_body{ ParseCompoundStatement( parent_scope ) };
	if ( !function_body ){
		return nullptr;
	}

	std::unique_ptr<ParsedFunction> function{ new ParsedFunction( file_name, line_num, function_name, std::move( parameters ) ) };
	function->SetFunctionBody( std::move( function_body ) );

	return function;
}

std::unique_ptr<Statement> Parser::ParseCompoundStatement( Scope *parent )
{
	if ( !Match( ScannerTokenType::TOKEN_OPEN_BRACE ) ){
		ProcessError( ScannerTokenType::TOKEN_OPEN_BRACE );
		return nullptr;
	}
	NextToken(); // consume '{'
	unsigned int const line_num = GetLineNumber();
	auto const file_name = GetFileName();

	std::unique_ptr<Scope> statement_scope{ ParseScope( parent ) };
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_BRACE ) ){
		ProcessError( ScannerTokenType::TOKEN_CLOSED_BRACE );
		return nullptr;
	}
	NextToken(); // consume '}'

	std::unique_ptr<Statement> compound_statement{ new CompoundStatement( file_name, line_num, std::move( statement_scope ) ) };
	return compound_statement;
}

std::unique_ptr<Scope> Parser::ParseScope( Scope *parent_scope )
{
	std::unique_ptr<Scope> scope{ new Scope( parent_scope ) };
	while ( !Match( ScannerTokenType::TOKEN_END_OF_STREAM ) && !Match( ScannerTokenType::TOKEN_CLOSED_BRACE ) ) {
		if ( Match( ScannerTokenType::TOKEN_CLASS_ID ) ) {
			std::unique_ptr<ParsedClass> klass( ParseClass( scope.get() ) );
			if ( !klass ) {
				return nullptr;
			}
			if ( !scope->AddClass( std::move( klass ) ) ) {
				ProcessError( L"Class with the same name already exist in this scope." );
			}
		}
		else if ( Match( ScannerTokenType::TOKEN_FUNC_ID ) ){
			std::unique_ptr<ParsedFunction> function{ ParseFunction( scope.get(), FunctionType::FUNCTION ) };
			if ( !function ){
				return nullptr;
			}
			if ( !scope->AddFunction( std::move( function ) ) ){
				ProcessError( L"Function with this name and signature exists" );
			}
		}
		else {
			std::unique_ptr<Statement> statement{ ParseStatement( scope.get() ) };
			if ( !statement ){
				return nullptr;
			}
			scope->AddStatement( std::move( statement ) );
		}
	}
	return scope;
}

std::unique_ptr<Statement> Parser::ParseStatement( Scope *parent_scope )
{
	switch ( CurrentToken().GetType() )
	{
	case ScannerTokenType::TOKEN_CASE_ID:
	case ScannerTokenType::TOKEN_ELSE_ID:
		return ParseLabelledStatement( parent_scope );
	case ScannerTokenType::TOKEN_OPEN_BRACE: // could this be a map or a scope?
		return ParseCompoundStatement( parent_scope );
	case ScannerTokenType::TOKEN_FOR_ID:
	case ScannerTokenType::TOKEN_DO_ID:
	case ScannerTokenType::TOKEN_WHILE_ID:
		return ParseIterationStatement( parent_scope );
	case ScannerTokenType::TOKEN_SWITCH_ID:
		return ParseSwitchStatement( parent_scope );
	case ScannerTokenType::TOKEN_IF_ID:
		return ParseIfStatement( parent_scope );
	case ScannerTokenType::TOKEN_BREAK_ID:
	case ScannerTokenType::TOKEN_RETURN_ID:
	case ScannerTokenType::TOKEN_CONTINUE_ID:
		return ParseJumpStatement( parent_scope );
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
		return ParseExpressionStatement();
	case ScannerTokenType::TOKEN_VAR_ID:
		return ParseDeclaration( parent_scope );
	case ScannerTokenType::TOKEN_SHOW_ID:
		return ParseShowStatement( parent_scope );
	default:
		ProcessError( L"Expected a statement here." );
		return nullptr;
	}
}

std::unique_ptr<Statement> Parser::ParseShowStatement( Scope *parent_scope )
{
	Token const tok = CurrentToken();
	NextToken(); // consume 'show'
	std::unique_ptr<Expression> expr{ ParseExpression() };
	if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
		ProcessError( ScannerTokenType::TOKEN_SEMI_COLON );
		return nullptr;
	}
	NextToken(); // consume ';'
	return TreeFactory::MakeShowExpressionStatement( tok, std::move( expr ) );
}

std::unique_ptr<Statement> Parser::ParseIfStatement( Scope *parent_scope )
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

	std::unique_ptr<Expression> logical_expression{ ParseExpression() };
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ) {
		ProcessError( ScannerTokenType::TOKEN_CLOSED_PAREN );
		return nullptr;
	}
	NextToken();

	std::unique_ptr<Statement> then_statement{ ParseStatement( parent_scope ) };

	if ( !then_statement ){
		ProcessError( L"Unable to parse the statement in the IF statement." );
		return  nullptr;
	}
	// let's see if there's an else part
	std::unique_ptr<Statement> else_statement{ nullptr };
	if ( Match( ScannerTokenType::TOKEN_ELSE_ID ) ){
		std::unique_ptr<Statement> else_temp{ ParseStatement( parent_scope ) };
		if ( !else_temp ){
			ProcessError( L"Error while processing the else part of the if statement." );
			return nullptr;
		}
		else_statement.reset( else_temp.release() );
	}

	std::unique_ptr<Statement> if_statement{ new IfStatement( file_name, line_num, std::move( logical_expression ),
		std::move( then_statement ), std::move( else_statement ) ) };
	return if_statement;
}

std::unique_ptr<Statement> Parser::ParseIterationStatement( Scope *parent_scope )
{
	unsigned int const line_num = GetLineNumber();
	std::wstring const file_name = GetFileName();
	std::unique_ptr<Statement> iterative_statement{};

	ScannerTokenType const tk = CurrentToken().GetType();

	if ( tk == ScannerTokenType::TOKEN_DO_ID ){
		NextToken(); // consume 'do'
		std::unique_ptr<Statement> do_body{ ParseStatement( parent_scope ) };
		if ( !Match( ScannerTokenType::TOKEN_WHILE_ID ) ){
			ProcessError( ScannerTokenType::TOKEN_WHILE_ID );
			return nullptr;
		}
		NextToken(); // consume token 'while'
		if ( !Match( ScannerTokenType::TOKEN_OPEN_PAREN ) ){
			ProcessError( L"Expected an open parenthesis after the `while` keyword" );
			return nullptr;
		}
		NextToken(); // consume '('
		std::unique_ptr<Expression> expression{ ParseExpression() };
		if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			ProcessError( L"Expected a closing parenthesis after the expression" );
			return nullptr;
		}
		NextToken(); // consume ')'
		if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
			ProcessError( L"Expected a semi-colon after the closing parenthesis" );
			return nullptr;
		}
		NextToken(); // consume ';'
		std::unique_ptr<Statement> do_while{ new DoWhileStatement( file_name, line_num, std::move( do_body ), std::move( expression ) ) };
		return do_while;
	}
	else if ( tk == ScannerTokenType::TOKEN_WHILE_ID ){
		NextToken(); // consume 'while'
		if ( !Match( ScannerTokenType::TOKEN_OPEN_PAREN ) ){
			ProcessError( ScannerTokenType::TOKEN_OPEN_PAREN );
			return nullptr;
		}
		NextToken(); // consume '('
		std::unique_ptr<Expression> expression{ ParseExpression() };
		if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			ProcessError( ScannerTokenType::TOKEN_CLOSED_PAREN );
			return nullptr;
		}
		NextToken(); //consume ')'
		std::unique_ptr<Statement> statement{ ParseStatement( parent_scope ) };
		if ( expression && statement ){
			std::unique_ptr<Statement> while_statement{ new WhileStatement( file_name, line_num, std::move( expression ),
				std::move( statement ) ) };
			return while_statement;
		}
		return nullptr;
	}
	// for_each statement, MAY be written( note the space ) as for each( ... ) or foreach( ... )
	bool is_two_word_foreach = ( tk == ScannerTokenType::TOKEN_FOR_ID && Match( ScannerTokenType::TOKEN_EACH_ID, SECOND_INDEX ) );
	if ( is_two_word_foreach || tk == ScannerTokenType::TOKEN_FOR_EACH_ID ){
		if ( is_two_word_foreach ){
			NextToken();
		}
		NextToken();
	}
	if ( !Match( ScannerTokenType::TOKEN_OPEN_PAREN ) ){
		ProcessError( ScannerTokenType::TOKEN_OPEN_PAREN );
		return nullptr;
	}
	NextToken(); // consume '('
	std::unique_ptr<Expression> for_each_expr{ ParseExpression() };

	NextToken(); // consume ')'
	std::unique_ptr<Statement> for_each_body_statement{ ParseStatement( parent_scope ) };
	if ( for_each_expr && for_each_body_statement ){
		std::unique_ptr<Statement> for_each_statement{ new ForEachStatement( file_name, line_num, std::move( for_each_expr ),
			std::move( for_each_body_statement ) ) };
		return for_each_statement;
	}
	return nullptr;
}

std::unique_ptr<Statement> Parser::ParseLabelledStatement( Scope *parent )
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
		std::unique_ptr<Expression> expr{ ParseConditionalExpression() };
		if ( !expr ){
			return nullptr;
		}
		if ( !Match( ScannerTokenType::TOKEN_COLON ) ){
			ProcessError( ScannerTokenType::TOKEN_COLON );
			return nullptr;
		}
		std::unique_ptr<Statement> statement{ ParseStatement( parent ) };
		if ( !statement ){
			return nullptr;
		}
		std::unique_ptr<Statement> case_{ new CaseStatement( tok.GetFileName(), tok.GetLineNumber(), std::move( expr ),
			std::move( statement ) ) };
		return case_;
	}
	break;
	default:
		ProcessError( L"Identifier allowed in this scope is 'else' and 'case'" );
		return nullptr;
	}
	std::unique_ptr<Statement> statement{ ParseStatement( parent ) };
	if ( !statement ){
		return nullptr;
	}
	std::unique_ptr<Statement> case_{ new LabelledStatement( tok.GetFileName(), tok.GetLineNumber(), tok.GetIdentifier(),
		std::move( statement ) ) };
	return case_;
}

std::unique_ptr<Statement> Parser::ParseSwitchStatement( Scope *parent_scope )
{
	Token const tok = CurrentToken();
	NextToken(); // consume 'switch'
	if ( !Match( ScannerTokenType::TOKEN_OPEN_PAREN ) ){
		ProcessError( ScannerTokenType::TOKEN_OPEN_PAREN );
		return nullptr;
	}
	NextToken(); // consume '('
	std::unique_ptr<Expression> switch_expression{ ParseExpression() };
	if ( !switch_expression ) return nullptr;

	if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
		ProcessError( ScannerTokenType::TOKEN_CLOSED_PAREN );
		return nullptr;
	}
	NextToken(); // consume ')'
	std::unique_ptr<Statement> switch_body{ ParseCompoundStatement( parent_scope ) };
	if ( !switch_body ) return nullptr;

	std::unique_ptr<Statement> switch_statement{ new SwitchStatement( tok.GetFileName(), tok.GetLineNumber(),
		std::move( switch_expression ), std::move( switch_body ) ) };
	return switch_statement;
}

std::unique_ptr<Statement> Parser::ParseJumpStatement( Scope *parent )
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
		std::unique_ptr<Expression> expression{};
		if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
			expression = ParseExpression();
		}
		if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
			ProcessError( ScannerTokenType::TOKEN_SEMI_COLON );
			return nullptr;
		}
		NextToken(); // consume ';'
		return TreeFactory::MakeReturnStatement( tok.GetFileName(), tok.GetLineNumber(), std::move( expression ) );
	}
	default:
		ProcessError( L"Unexpected statement" );
		return nullptr;
	}
}

std::unique_ptr<Statement> Parser::ParseExpressionStatement()
{
	Token const tok = CurrentToken();
	std::unique_ptr<Expression> expression{};
	if ( tok.GetType() != ScannerTokenType::TOKEN_SEMI_COLON ){
		expression = ParseExpression();
	}
	if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
		ProcessError( ScannerTokenType::TOKEN_SEMI_COLON );
		return nullptr;
	}
	NextToken(); // consume ';'
	return TreeFactory::MakeExpressionStatement( tok.GetFileName(), tok.GetLineNumber(), std::move( expression ) );
}

std::unique_ptr<Expression> Parser::ParseExpression()
{
	return ParseAssignmentExpression();
}

std::unique_ptr<Expression> Parser::ParseAssignmentExpression()
{
	std::unique_ptr<Expression> expr = ParseConditionalExpression();

	switch ( CurrentToken().GetType() ){
	case ScannerTokenType::TOKEN_ASSIGN:
	case ScannerTokenType::TOKEN_ADD_EQL:
	case ScannerTokenType::TOKEN_SUB_EQL:
	case ScannerTokenType::TOKEN_MUL_EQL:
	case ScannerTokenType::TOKEN_DIV_EQL:
	{
		auto const tok = CurrentToken();
		NextToken();
		return TreeFactory::MakeAssignmentExpression( tok, std::move( expr ), ParseAssignmentExpression() );
	}
	default:;
	}
	return expr;
}

std::unique_ptr<Expression>	Parser::ParseConditionalExpression()
{
	std::unique_ptr<Expression> expression{ ParseBinaryExpression() };
	if ( !expression ){
		return nullptr;
	}
	if ( CurrentToken().GetType() == ScannerTokenType::TOKEN_QUESTION_MARK ){
		Token const tok = CurrentToken();
		NextToken(); // consume '?'
		std::unique_ptr<Expression> lhs_expression{ ParseExpression() };
		if ( !Match( ScannerTokenType::TOKEN_COLON ) ){
			ProcessError( ScannerTokenType::TOKEN_COLON );
			return nullptr;
		}
		NextToken(); // consume ':'
		std::unique_ptr<Expression> rhs_expression{ ParseExpression() };
		if ( lhs_expression && rhs_expression ){
			std::unique_ptr<Expression> cond_expr{ new ConditionalExpression( tok.GetFileName(), tok.GetLineNumber(),
				std::move( expression ), std::move( lhs_expression ), std::move( rhs_expression ) ) };
			return cond_expr;
		}
	}
	return expression;
}

std::unique_ptr<Expression> Parser::ParseBinaryExpression()
{
	std::unique_ptr<Expression> unary_expr{ ParseUnaryExpression() };
	int const lowest_precedence = 0;

	return ParseBinaryOpExpression( lowest_precedence, std::move( unary_expr ));
}

std::unique_ptr<Expression> Parser::ParseDictionaryExpression()
{
	if ( !Match( ScannerTokenType::TOKEN_OPEN_BRACE ) ){
		ProcessError( L"A map type begins with an opening brace and ends with a corresponding opening brace" );
		return nullptr;
	}
	Token const token = CurrentToken();
	NextToken(); // consume '{'
	std::vector<MapExpression::expression_ptr_pair_t> key_datum_list{};
	while ( !Match( ScannerTokenType::TOKEN_END_OF_STREAM ) && !Match( ScannerTokenType::TOKEN_CLOSED_BRACE ) ){
		auto key_expression = ParseExpression();
		if ( !Match( ScannerTokenType::TOKEN_COLON ) ){
			ProcessError( L"Expects a colon as a map separator" );
			return nullptr;
		}
		NextToken(); // consume ':'
		auto value_expression = ParseExpression();
		if ( !( key_expression && value_expression ) ){
			ProcessError( L"Unable to parse key/value expression for map" );
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

std::unique_ptr<Expression> Parser::ParseListExpression()
{
	if ( !Match( ScannerTokenType::TOKEN_OPEN_BRACKET ) ){
		ProcessError( ScannerTokenType::TOKEN_OPEN_BRACKET );
		return nullptr;
	}
	Token const tok = CurrentToken();
	NextToken(); // consume '['
	std::unique_ptr<ExpressionList> list_params{};
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_BRACKET ) ){
		list_params.reset( new ExpressionList( CurrentToken().GetFileName(), CurrentToken().GetLineNumber() ) );
	}
	while ( !Match( ScannerTokenType::TOKEN_END_OF_STREAM ) && !Match( ScannerTokenType::TOKEN_CLOSED_BRACKET ) ){
		list_params->AddExpression( ParseExpression() );
		if ( !Match( ScannerTokenType::TOKEN_COMMA ) ) break;
		NextToken(); // consume ','
	}
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_BRACKET ) ){
		ProcessError( ScannerTokenType::TOKEN_CLOSED_BRACKET );
		return nullptr;
	}
	NextToken(); // consume ']'
	return TreeFactory::MakeListExpression( tok, std::move( list_params ) );
}

std::unique_ptr<Expression> Parser::ParseUnaryExpression()
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

std::unique_ptr<Expression> Parser::ParsePrimaryExpression()
{
	Token const tok = CurrentToken();
	switch ( tok.GetType() )
	{
	case ScannerTokenType::TOKEN_NEW:
	{
		NextToken();
		std::unique_ptr<Expression> expr{ ParseExpression() };
		return TreeFactory::MakeNewExpression( tok, std::move( expr ) );
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
		std::unique_ptr<Expression> expr{ ParseExpression() };
		if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			ProcessError( L"Expected a closing parenthesis before", scanner->GetToken( SECOND_INDEX )->GetType() );
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

std::unique_ptr<ExpressionList> Parser::ParseArgumentExpressionList()
{
	std::unique_ptr<ExpressionList> argList{};
	if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
		argList = std::make_unique<ExpressionList>( CurrentToken().GetFileName(), CurrentToken().GetLineNumber() );
		while ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			std::unique_ptr<Expression> expr{ ParseAssignmentExpression() };
			if ( !expr ) return nullptr;
			argList->AddExpression( std::move( expr ) );
			if ( Match( ScannerTokenType::TOKEN_COMMA ) ){
				NextToken();
			}
		}
	}
	return argList;
}

std::unique_ptr<Expression> Parser::ParseLambdaExpression()
{
	Token const token = CurrentToken();
	if ( !Match( ScannerTokenType::TOKEN_AT ) ){
		ProcessError( L"Expected a lambda expresion to start with an @ symbol" );
		return nullptr;
	}
	std::unique_ptr<ExpressionList> lambda_parameters{};
	NextToken(); // consume '@'
	// parameter body, i.e. (),  is optional
	if ( Match( ScannerTokenType::TOKEN_OPEN_PAREN ) ){
		NextToken(); // consume '('
		if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ){
			lambda_parameters.reset( new ExpressionList( token.GetFileName(), token.GetLineNumber() ) );
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

	std::unique_ptr<Expression> lambda_expression{ new LambdaExpression( token, std::move( lambda_parameters ),
		ParseCompoundStatement( nullptr ) ) };
	return lambda_expression;
}

std::unique_ptr<Expression> Parser::ParsePostfixExpression()
{
	std::unique_ptr<Expression> expr{ ParsePrimaryExpression() };

	while ( true )
	{
		switch ( CurrentToken().GetType() )
		{
		case ScannerTokenType::TOKEN_OPEN_PAREN: {// function call
			auto const token = CurrentToken();
			NextToken(); // consume '('
			// parse argument_list and expect ')'
			std::unique_ptr<ExpressionList> argExprList{ ParseArgumentExpressionList() };
			if ( !Match( ScannerTokenType::TOKEN_CLOSED_PAREN ) ) {
				ProcessError( L"Expects a closing parenthesis before the next token." );
				return nullptr;
			}
			NextToken(); // consume ')'
			std::unique_ptr<Expression> new_expr{ new FunctionCall( token.GetFileName(), token.GetLineNumber(), std::move( expr ),
				std::move( argExprList ) ) };
			expr = std::move( new_expr );
			break;
		}
		case ScannerTokenType::TOKEN_OPEN_BRACKET: // array subscript
		{
			NextToken(); // consume '['
			std::unique_ptr<Expression> subscript_expression{ ParseExpression() };
			if ( !Match( ScannerTokenType::TOKEN_CLOSED_BRACKET ) ){
				ProcessError( ScannerTokenType::TOKEN_CLOSED_BRACKET );
				return nullptr;
			}
			NextToken(); // consume ']'
			std::unique_ptr<Expression> expression{ new SubscriptExpression( CurrentToken().GetFileName(),
				CurrentToken().GetLineNumber(), std::move( expr ), std::move( subscript_expression ) ) };
			expr = std::move( expression );
			break;
		}
		case ScannerTokenType::TOKEN_PERIOD:
		{
			NextToken(); // consume '.'
			Token const curr_token = CurrentToken();
			if ( !Match( ScannerTokenType::TOKEN_IDENT ) ){
				ProcessError( L"Expected an identifier after the dot operator." );
				return nullptr;
			}
			NextToken(); // consume identifier
			std::unique_ptr<Expression> period_expr{ new DotExpression( curr_token.GetFileName(), curr_token.GetLineNumber(),
				curr_token, std::move( expr ) ) };
			expr = std::move( period_expr );
			break;
		}
		case ScannerTokenType::TOKEN_INCR:
		{
			NextToken(); // consume '++'
			std::unique_ptr<Expression> expr_temp{ std::move( expr ) };
			auto curr_token = CurrentToken();
			expr.reset( new PostIncrExpression( curr_token.GetFileName(), curr_token.GetLineNumber(), std::move( expr_temp ) ) );
			break;
		}
		case ScannerTokenType::TOKEN_DECR:
		{
			NextToken(); // consume '--'
			std::unique_ptr<Expression> expr_temp{ std::move( expr ) };
			auto curr_token = CurrentToken();
			expr.reset( new PostDecrExpression( curr_token.GetFileName(), curr_token.GetLineNumber(), std::move( expr_temp ) ) );
			break;
		}
		default:
			return expr;

		} // end-switch
	} // end-while
	return expr;
} // end-ParsePostfixExpression

// Precedence Climbing
std::unique_ptr<Expression> Parser::ParseBinaryOpExpression( int const precedence, std::unique_ptr<Expression> unary_expr )
{
	while ( true ){
		int const currentPrecedence = GetTokenPrecedence();
		if ( currentPrecedence < precedence ){
			return unary_expr;
		}

		Token const tok = CurrentToken();
		NextToken(); // consume current operator
		std::unique_ptr<Expression> second_expr{ ParseUnaryExpression() };
		int const nextPrecedence = GetTokenPrecedence();
		if ( currentPrecedence < nextPrecedence ){
			std::unique_ptr<Expression> new_result{ ParseBinaryOpExpression( currentPrecedence + 1, std::move( second_expr )) };
			second_expr = std::move( new_result );
		}
		std::unique_ptr<Expression> unary_expr_temp{ std::move( unary_expr ) };
		unary_expr.reset( new BinaryExpression( tok, std::move( unary_expr_temp ), std::move( second_expr ) ) );
	}
}

std::unique_ptr<Statement> Parser::ParseDeclaration( Scope *parent_scope, ScannerTokenType def )
{
	NextToken(); // consume keyword 'var'
	Scope::list_of_sptrs<Declaration> declaration_list{};
	std::unique_ptr<Declaration> declaration{};

	AccessType access_type = DetermineAccessType( def );

	do {
		Token const token = CurrentToken();
		if ( !Match( ScannerTokenType::TOKEN_IDENT ) ){
			ProcessError( L"Expected a variable name" );
			return nullptr;
		}
		std::wstring const variable_name = CurrentToken().GetIdentifier();
		NextToken(); // consume variable name
		std::unique_ptr<Expression> expr{};
		if ( Match( ScannerTokenType::TOKEN_ASSIGN ) ){
			NextToken(); // consume '='
			expr = ParseExpression();
		}
		declaration.reset( new Declaration( token.GetFileName(), token.GetLineNumber(), variable_name,
			std::move( expr ), access_type ) );
		if ( !Match( ScannerTokenType::TOKEN_COMMA ) ) break;
		NextToken(); // consume ','
		declaration_list.push_back( std::move( declaration ) );
	} while ( !Match( ScannerTokenType::TOKEN_END_OF_STREAM ) && !Match( ScannerTokenType::TOKEN_SEMI_COLON ) );
	if ( !Match( ScannerTokenType::TOKEN_SEMI_COLON ) ){
		ProcessError( L"Expected a semi-colon at the end of the variable declaration(s)" );
	}
	else {
		NextToken(); // consume ';'
	}
	bool is_decl_list = declaration_list.size() != 0;
	return is_decl_list ? TreeFactory::MakeDeclarationList( CurrentToken(), std::move( declaration_list ) ) :
		TreeFactory::MakeDeclaration( std::move( declaration ) );
}
