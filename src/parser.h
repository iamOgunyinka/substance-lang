/***************************************************************************
 * Language parser
 *
 * Copyright (c) 2013-2016 Randy Hollines
 * Copyright (c) 2017 Joshua Ogunyinka
 * All rights reserved.
 */

#ifndef __PARSER_H__
#define __PARSER_H__

#include "scanner.h"
#include "tree.h"

#define SECOND_INDEX 1
#define THIRD_INDEX 2

#define PARENT_SCOPE Scope* parent_scope

namespace compiler {
	
	using std::map;
	using std::wstring;
	using std::wcout;
	using std::endl;

	class Parser {
		std::wstring						input;
		std::unique_ptr<Scanner>			scanner;
		std::map<ScannerTokenType, wstring> error_msgs;
		std::map<size_t, std::wstring>		errors;
		int									local_count;
		Token								*current_token;

	private:
		inline void NextToken() {
			scanner->NextToken();
		}

		inline bool Match( ScannerTokenType type, int index = 0 ) {
			return scanner->GetToken( index )->GetType() == type;
		}

		inline Token& CurrentToken( int index = 0 ){
			current_token = ( scanner->GetToken( index ) );
			return *current_token;
		}

		inline int GetLineNumber() {
			return scanner->GetToken()->GetLineNumber();
		}

		inline const wstring GetFileName() {
			return scanner->GetToken()->GetFileName();
		}

		inline ScannerTokenType GetToken( int index = 0 ) {
			return scanner->GetToken( index )->GetType();
		}

		void Show( const wstring &msg, int depth ) {
			for ( int i = 0; i < depth; i++ ) {
				wcout << L"  ";
			}
			wcout << msg << endl;
		}

		inline wstring ToString( int v ) {
			std::wostringstream str;
			str << v;
			return str.str();
		}
		int GetTokenPrecedence();

		// error processing
		void LoadErrorCodes();
		void ProcessError( const ScannerTokenType type );
		void ProcessError( const wstring &msg );
		void ProcessError( const wstring &msg, ParseNode* node );
		void ProcessError( const wstring &msg, const ScannerTokenType sync );
		bool NoErrors();

		// parsing operations
		Scope*			ParseScope( Scope *parent_scope );
		Statement*		ParseStatement( Scope *parent );
		Statement*		ParseIfStatement( Scope *parent );
		Statement*		ParseIterationStatement( Scope *parent_scope );
		Statement*		ParseSwitchStatement( Scope *parent );
		Statement*		ParseJumpStatement( Scope *parent );
		Statement*		ParseShowStatement( Scope *parent_scope );
		CompoundStatement*	ParseCompoundStatement( Scope *parent_scope );
		
		Declaration*	ParseClass( Scope *parent_scope, AccessType, StorageType );
		Declaration*	ParseFunction( Scope *parent_scope, FunctionType func_type, AccessType, StorageType );
		Declaration*	ParseVariableDeclaration( Scope *parent_scope, AccessType, StorageType );
		Declaration*	ParseDeclaration( Scope *parent_scope );
		Declaration*	ParseEmptyStatement( Scope * );

		Statement*		ParseLabelledStatement( Scope *parent );
		Statement*		ParseExpressionStatement();
		Expression*		ParseExpression();
		Expression*		ParseConditionalExpression();
		Expression*		ParseAssignmentExpression();
		Expression*		ParseBinaryExpression();
		Expression*		ParseBinaryOpExpression( int const precedence, Expression* );
		Expression*		ParseUnaryExpression();
		Expression*		ParsePostfixExpression();
		Expression*		ParsePrimaryExpression();
		ExpressionList*	ParseArgumentExpressionList();
		Expression*		ParseLambdaExpression();
		Expression*		ParseListExpression();
		Expression*		ParseDictionaryExpression();
	public:
		Parser( const wstring &input ){
			this->input = input;
			this->scanner.reset( new Scanner( input ) );
			this->local_count = -1;
			LoadErrorCodes();
		}

		~Parser() = default;

		std::unique_ptr<ParsedProgram> Parse();
	};
}

#endif
