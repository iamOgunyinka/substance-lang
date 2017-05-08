/***************************************************************************
 * Parse tree.
 *
 * Copyright (c) 2017 Joshua Ogunyinka
 * Copyright (c) 2013-2016 Randy Hollines
 * All rights reserved.
 **/

#ifndef __TREE_H__
#define __TREE_H__

#include <memory>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <set>
#include "common.h"
#include "scanner.h"

namespace compiler {
	class TreeFactory;
	class SymbolTable;
	class Reference;
	class Declaration;
	class ExpressionList;
	class Scope;
	class Declaration;
	class ParsedClass;
	class ParsedFunction;
	class Statement;
	class SemaCheck;
	/****************************
	 * Base class for all parse nodes
	 ****************************/
	
	using std::wstring;

	class ParseNode {
	protected:
		std::wstring file_name;
		unsigned int line_num;

	public:
		ParseNode( const std::wstring &file_name, const unsigned int line_num ) {
			this->file_name = file_name;
			this->line_num = line_num;
		}

		virtual ~ParseNode() {
		}

		const std::wstring GetFileName() {
			return file_name;
		}

		const int GetLineNumber() {
			return line_num;
		}
	};

	/****************************
	 * Expression types
	 ****************************/
	enum class ExpressionType {
		REF_EXPR = -100,
		NULL_LIT_EXPR,
		CHAR_LIT_EXPR,
		INT_LIT_EXPR,
		FLOAT_LIT_EXPR,
		BOOLEAN_LIT_EXPR,
		AND_EXPR,
		OR_EXPR,
		EQL_EXPR,
		NEQL_EXPR,
		LES_EXPR,
		GTR_EQL_EXPR,
		LES_EQL_EXPR,
		GTR_EXPR,
		ADD_EXPR,
		SUB_EXPR,
		MUL_EXPR,
		DIV_EXPR,
		MOD_EXPR,
		// Newly added expressions
		CHAR_STR_EXPR,
		FUNCTION_CALL_EXPR,
		ASSIGNMENT_EXPR,
		VARIABLE_EXPR,
		BINARY_EXPR,
		UNARY_EXPR,
		CONDITIONAL_EXPR,
		SUBSCRIPT_EXPR,
		DOT_EXPRESSION,
		POST_INCR_EXPR,
		POST_DECR_EXPR,
		PRE_INCR_EXPR,
		PRE_DECR_EXPR,
		LAMBDA_EXPR,
		LIST_EXPR,
		MAP_EXPR,
		NEW_EXPR
	};

	/****************************
	 * Expression base class
	 ****************************/
	class Expression : public ParseNode {
		friend class TreeFactory;

	public:
		Expression( const std::wstring &file_name, const unsigned int line_num ) : ParseNode( file_name, line_num ) {
		}

		virtual ~Expression() {
		}

		virtual void  Analyze( SemaCheck & ) = 0;
		virtual const ExpressionType GetExpressionType() = 0;
	};

	/****************************
	* ExpressionList class
	****************************/
	class ExpressionList : public ParseNode {
		std::deque<std::unique_ptr<Expression>> expressions;
	public:
		ExpressionList( const std::wstring &file_name, const unsigned int line_num ) : ParseNode( file_name, line_num ) {
		}

		~ExpressionList() {
		}

		std::deque<std::unique_ptr<Expression>>& GetExpressions() {
			return expressions;
		}

		std::deque<std::unique_ptr<Expression>>::size_type Length(){
			return expressions.size();
		}
		void AddExpression( std::unique_ptr<Expression> e ) {
			expressions.push_back( std::move( e ) );
		}

		void Analyze( SemaCheck & );
	};

	/****************************
	 * CharacterString class
	 ****************************/
	class CharacterString : public Expression {
		friend class TreeFactory;
		int id;
		std::wstring char_string;

		CharacterString( const std::wstring &file_name, const unsigned int line_num, const std::wstring &orig )
			: Expression( file_name, line_num ) {
			int skip = 2;
			for ( size_t i = 0; i < orig.size(); i++ ) {
				wchar_t c = orig[ i ];
				if ( skip > 1 && c == L'\\' && i + 1 < orig.size() ) {
					wchar_t cc = orig[ i + 1 ];
					switch ( cc ) {
					case L'"':
						char_string += L'\"';
						skip = 0;
						break;

					case L'\\':
						char_string += L'\\';
						skip = 0;
						break;

					case L'n':
						char_string += L'\n';
						skip = 0;
						break;

					case L'r':
						char_string += L'\r';
						skip = 0;
						break;

					case L't':
						char_string += L'\t';
						skip = 0;
						break;

					case L'0':
						char_string += L'\0';
						skip = 0;
						break;

					default:
						if ( skip > 1 ) {
							char_string += c;
						}
						else {
							skip++;
						}
						break;
					}
				}

				if ( skip > 1 ) {
					char_string += c;
				}
				else {
					skip++;
				}
			}
			id = -1;
		}

		~CharacterString() {
		}

	public:
		const ExpressionType GetExpressionType() {
			return ExpressionType::CHAR_STR_EXPR;
		}

		void SetId( int i ) {
			id = i;
		}

		int GetId() {
			return id;
		}

		const std::wstring& GetString() const {
			return char_string;
		}
		void Analyze( SemaCheck & );
	};

	/****************************
	 * Reference class
	 ****************************/
	enum class ReferenceType {
		REF_TYPE,
		SELF_TYPE,
		NEW_LIST_TYPE,
		NEW_HASH_TYPE,
		NEW_OBJ_TYPE
	};

	/****************************
	* BooleanLiteral class
	****************************/
	class BooleanLiteral : public Expression {
		friend class TreeFactory;
		bool value;

		BooleanLiteral( const std::wstring &file_name, const unsigned int line_num, bool v )
			: Expression( file_name, line_num ), value( v ) {
		}

		~BooleanLiteral() {
		}

	public:
		const ExpressionType GetExpressionType() {
			return ExpressionType::BOOLEAN_LIT_EXPR;
		}

		bool GetValue() {
			return value;
		}
		void Analyze( SemaCheck & );
	};

	/****************************
	* NullLiteral class
	****************************/
	class NullLiteral : public Expression {
	public:
		NullLiteral( const std::wstring &file_name, unsigned int const line_number )
			: Expression( file_name, line_number ) {
		}

		~NullLiteral() = default;
	public:
		const ExpressionType GetExpressionType() {
			return ExpressionType::NULL_LIT_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	class LambdaExpression : public Expression
	{
		std::unique_ptr<ExpressionList> parameters;
		std::unique_ptr<Statement> body;
	public:
		LambdaExpression( Token const & tok, std::unique_ptr<ExpressionList> params,
			std::unique_ptr<Statement> lambda_body ) : Expression( tok.GetFileName(), tok.GetLineNumber() ),
			parameters( std::move( params ) ), body( std::move( lambda_body ) ){
		}
		ExpressionType const GetExpressionType() override {
			return ExpressionType::LAMBDA_EXPR;
		}
		std::unique_ptr<Statement>& GetLambdaBody(){
			return body;
		}
		std::unique_ptr<ExpressionList>& GetParamaters(){
			return parameters;
		}
		void Analyze( SemaCheck & );
	};

	class MapExpression : public Expression
	{
	public:
		using expression_ptr_pair_t = std::pair<std::shared_ptr<Expression>, std::shared_ptr<Expression>>;
	private:
		std::vector<expression_ptr_pair_t> list_of_expressions;
	public:
		MapExpression( std::wstring const & filename, unsigned int const line_number, std::vector<expression_ptr_pair_t> list ) :
			Expression( filename, line_number ), list_of_expressions( std::move( list ) ){
		}
		ExpressionType const GetExpressionType() override {
			return ExpressionType::MAP_EXPR;
		}

		std::vector<expression_ptr_pair_t>::size_type GetMapSize() const {
			return list_of_expressions.size();
		}

		std::vector<expression_ptr_pair_t>::iterator begin(){
			return list_of_expressions.begin();
		}
		
		std::vector<expression_ptr_pair_t>::iterator end(){
			return list_of_expressions.end();
		}
		expression_ptr_pair_t& GetKeyDataAtIndex( unsigned int const index ) {
			return list_of_expressions.at( index );
		}
		void Analyze( SemaCheck & );
	};

	class NewExpression : public Expression
	{
		std::unique_ptr<Expression> expression;

	public:
		NewExpression( std::wstring const & filename, unsigned int const line_number, std::unique_ptr<Expression> expr ) :
			Expression( filename, line_number ), expression( std::move( expr ) ){
		}

		ExpressionType const GetExpressionType() override {
			return ExpressionType::NEW_EXPR;
		}
		void Analyze( SemaCheck & );
	};
	/****************************
	* CharacterLiteral class
	****************************/
	class CharacterLiteral : public Expression {
		friend class TreeFactory;
		CHAR_T value;

		CharacterLiteral( const std::wstring &file_name, const unsigned int line_num, CHAR_T v )
			: Expression( file_name, line_num ), value( v ) {
			value = v;
		}

		~CharacterLiteral() {
		}

	public:
		CHAR_T GetValue() {
			return value;
		}

		const ExpressionType GetExpressionType() {
			return ExpressionType::CHAR_LIT_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	/****************************
	* IntegerLiteral class
	****************************/
	class IntegerLiteral : public Expression {
		friend class TreeFactory;
		INT_T value;
	public:

		IntegerLiteral( const std::wstring &file_name, const unsigned int line_num, INT_T v )
			: Expression( file_name, line_num ), value( v ) {
		}

		~IntegerLiteral() {
		}

		INT_T GetValue() {
			return value;
		}

		const ExpressionType GetExpressionType() {
			return ExpressionType::INT_LIT_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	/****************************
	* FloatLiteral class
	****************************/
	class FloatLiteral : public Expression {
		friend class TreeFactory;
		FLOAT_T value;

		FloatLiteral( const std::wstring &file_name, const unsigned int line_num, FLOAT_T v )
			: Expression( file_name, line_num ), value( v ) {
			value = v;
		}

		~FloatLiteral() {
		}

	public:
		FLOAT_T GetValue() {
			return value;
		}

		const ExpressionType GetExpressionType() {
			return ExpressionType::FLOAT_LIT_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	/****************************
	 * StatementType enum
	 ****************************/
	enum StatementType {
		ASSIGNMENT_STATEMENT = -200,
		FUNCTION_CALL_STATEMENT,
		IF_ELSE_STATEMENT,
		DO_WHILE_STATEMENT,
		WHILE_STATEMENT,
		FOR_EACH_IN_STATEMENT,
		FOR_OF_STATEMENT,
		DECLARATION_STATEMENT,
		DECL_LIST_STATEMENT,
		COMPOUND_STATEMENT,
		SWITCH_STATEMENT,
		RETURN_STATEMENT,
		CONTINUE_STATEMENT,
		BREAK_STATEMENT,
		LABELLED_STATEMENT,
		CASE_STATEMENT,
		SHOW_STATEMENT,
		EXPR_STATEMENT
	};

	enum class ScopeType {
		NAMESPACE_SCOPE,
		CLASS_SCOPE,
		FUNCTION_SCOPE,
		TEMP_SCOPE
	};

	/**************************************************************************
	*	Scope for a specific construct; all scopes can have classes, functions,
	*	declarations defined in them. The parent scope is the scope directly
	*	above the scope we're referencing, it forms a chained list of scopes.
	***************************************************************************/
	class Scope {
	public:
		template<typename T>
		using list_of_ptrs = std::vector<std::unique_ptr<T>>;

		template<typename Type>
		using list_of_sptrs = std::vector<std::shared_ptr<Type>>;
		
		template<typename T>
		using omap_ptrs =		std::unordered_multimap<std::wstring, std::shared_ptr<T>>;
		using unordered_class = std::unordered_map<std::wstring, std::shared_ptr<ParsedClass>>;

	private:
		Scope						*parent;			// parent scope
		size_t						local_count;		// number of local variables
		unordered_class				parsed_classes;		// classes available in a scope
		list_of_sptrs<Declaration>	declarations;		// all variable declarations for the current scope
		list_of_ptrs<Statement>		statements;			// statements for that scope
		omap_ptrs<ParsedFunction>	parsed_functions;	// functions available in this scope
		std::set<std::wstring>		function_names;     // distinct names of functions
		ScopeType					type;
	public:
		Scope::Scope( Scope *p ) : parent( p ), local_count( 0 ){
		}

		Scope*	GetParentScope();
		void	SetParentScope( Scope *parent_scope );
		void	SetScopeType( ScopeType );
		bool	AddClass( std::unique_ptr<ParsedClass> klass );
		bool	AddFunction( std::unique_ptr<ParsedFunction> function );
		void	AddStatement( std::unique_ptr<Statement> statement );
		bool	AddDeclaration( std::shared_ptr<Declaration> declaration );
		int		LocalCount() const;
		void	SetLocalCount( int count );
		void	Analyze( SemaCheck & );
		
		unordered_class&				GetClasses();
		list_of_ptrs<Statement>&		GetStatements();
		list_of_sptrs<Declaration>&		GetDeclarations();
		omap_ptrs<ParsedFunction>&		GetFunctions();
		Declaration*					FindVariableDeclaration( std::wstring const & identifier );
		std::shared_ptr<ParsedFunction>	FindFunctionDefinition( std::wstring const & function_name, unsigned int const arity );
		std::shared_ptr<ParsedClass>	FindClassDefinition( std::wstring const & class_name );
	};

	
	class Statement : public ParseNode {
		friend class TreeFactory;

	public:
		Statement( const std::wstring &file_name, const unsigned int line_num ) : ParseNode( file_name, line_num ) {
		}

		virtual ~Statement() {}
		virtual void Analyze( SemaCheck & ) = 0; // most of the definitions appear in semacheck
		virtual const StatementType GetStatementType() = 0;
	};

	enum class DeclarationType {
		INST_DCLR,
		CLS_DCLR,
		LOCL_DCLR
	};

	enum class AccessType {
		PUBLIC_ACCESS,
		PROTECTED_ACCESS,
		PRIVATE_ACCESS,
		STATIC_ACCESS,
		INVALID_ACCESS = -1
	};


	class Declaration : public Statement, public std::enable_shared_from_this<Declaration> {
		std::wstring				identifier;
		std::shared_ptr<Expression> expression; // optional value assigned
		DeclarationType				type;
		AccessType					access_specifier;
		int							id;

	public:
		Declaration( const std::wstring &file_name, const unsigned int line_num,
			const std::wstring &ident, std::unique_ptr<Expression> expr, AccessType access )
			: Statement( file_name, line_num ), identifier( ident ), expression( std::move( expr )) {
			this->type = type;
			this->id = -1;
			access_specifier = access;
		}

		const StatementType GetStatementType() {
			return DECLARATION_STATEMENT;
		}

		std::wstring const GetIdentifier() {
			return this->identifier;
		}

		void SetAccessType( AccessType t ){
			access_specifier = t;
		}

		AccessType GetAccessType() {
			return access_specifier;
		}

		DeclarationType GetType() {
			return this->type;
		}

		void SetId( int id ) {
			this->id = id;
		}

		int GetId() {
			return this->id;
		}

		bool IsStatic() {
			return access_specifier == AccessType::STATIC_ACCESS;
		}

		// only useful in class scopes
		void SetAccessSpecifier( AccessType access_type ){
			access_specifier = access_type;
		}

		AccessType GetAccessSpecifier(){
			return access_specifier;
		}
		void Analyze( SemaCheck & ) override;
	};


	class DeclarationList : public Statement
	{
	public:
		using declaration_list_t = std::vector<std::shared_ptr<Declaration>>;
	private:
		declaration_list_t declarations;
	public:
		
		DeclarationList( std::wstring const & filename, unsigned int const line_number, declaration_list_t decls ) :
			Statement( filename, line_number ), declarations( std::move( decls )){
		}
		void AddDeclaration( std::unique_ptr<Declaration> decl ){
			assert( decl->GetStatementType() == DECLARATION_STATEMENT );
			declarations.push_back( std::move( decl ) );
		}

		declaration_list_t& GetDeclarations(){
			return declarations;
		}

		StatementType const GetStatementType() override {
			return StatementType::DECL_LIST_STATEMENT;
		}

		void Analyze( SemaCheck & ) override;
	};


	class CompoundStatement : public Statement {
		std::unique_ptr<Scope> scope;
	public:
		CompoundStatement( std::wstring const & file_name, unsigned int const line_num, std::unique_ptr<Scope> s ) :
			Statement( file_name, line_num ), scope( std::move( s ) ){
		}
		~CompoundStatement() = default;
		
		Scope::list_of_ptrs<Statement>& GetStatementList(){
			return scope->GetStatements();
		}
		std::unique_ptr<Scope>& GetScope(){
			return scope;
		}

		StatementType const GetStatementType() final override {
			return StatementType::COMPOUND_STATEMENT;
		}

		void Analyze( SemaCheck & ) override;
	};


	class ExpressionStatement : public Statement {
		std::unique_ptr<Expression> expression;

	public:
		ExpressionStatement( std::wstring const & file_name, unsigned int const line_num, std::unique_ptr<Expression> expr ) :
			Statement( file_name, line_num ), expression( std::move( expr ) ){
		}

		std::unique_ptr<Expression>& GetExpression(){
			return expression;
		}

		StatementType const GetStatementType() override {
			return StatementType::EXPR_STATEMENT;
		}

		void Analyze( SemaCheck & ) override;
	};


	class LabelledStatement : public Statement {
		std::unique_ptr<Statement>	label_statement;
		std::wstring const			label_name;

	public:
		LabelledStatement( std::wstring const & filename, unsigned int const line_num, std::wstring const & label,
			std::unique_ptr<Statement> statement ) : Statement( filename, line_num ),
			label_statement( std::move( statement ) ), label_name( label ){
		}

		std::unique_ptr<Statement>& GetStatement(){
			return label_statement;
		}

		std::wstring const GetLabelName() const {
			return label_name;
		}
		StatementType const GetStatementType() override {
			return StatementType::LABELLED_STATEMENT;
		}

		void Analyze( SemaCheck & ) override;
	};

	class CaseStatement : public Statement {
		std::unique_ptr<Expression> case_expression;
		std::unique_ptr<Statement>	case_statement;
	public:
		CaseStatement( std::wstring const & filename, unsigned int const line_number, std::unique_ptr<Expression> expression,
			std::unique_ptr<Statement> statement ) : Statement( filename, line_number ),
			case_expression( std::move( expression ) ), case_statement( std::move( statement ) ){
		}

		std::unique_ptr<Expression>&	GetExpression(){ return case_expression; }
		std::unique_ptr<Statement>&		GetStatement() { return case_statement; }
		StatementType const				GetStatementType() override{ return StatementType::CASE_STATEMENT; }
		void Analyze( SemaCheck & ) override;
	};

	
	class ReturnStatement : public Statement {
		std::unique_ptr<Expression> expression;

	public:
		ReturnStatement( const std::wstring &file_name, const unsigned int line_num, std::unique_ptr<Expression> expr )
			: Statement( file_name, line_num ), expression( std::move( expr ) ) {
		}

		std::unique_ptr<Expression>& GetExpression() {
			return expression;
		}

		const StatementType GetStatementType() override {
			return RETURN_STATEMENT;
		}
		void Analyze( SemaCheck & ) override;
	};
	
	
	class ContinueStatement : public Statement {
		friend class TreeFactory;

	public:
		ContinueStatement( const std::wstring &file_name, const unsigned int line_num )
			: Statement( file_name, line_num ){
		}

		const StatementType GetStatementType() override {
			return StatementType::CONTINUE_STATEMENT;
		}
		void Analyze( SemaCheck & ) override;
	};
	
	
	class BreakStatement : public Statement {
		friend class TreeFactory;

	public:
		BreakStatement( const std::wstring &file_name, const unsigned int line_num )
			: Statement( file_name, line_num ){
		}

		const StatementType GetStatementType() override {
			return StatementType::BREAK_STATEMENT;
		}
		void Analyze( SemaCheck & ) override;
	};


	class FunctionCall : public Expression {
		std::unique_ptr<Expression>		function;
		std::unique_ptr<ExpressionList> arguments;
		std::wstring					caller;
		bool							returns_value;
	public:
		FunctionCall( const std::wstring &file_name, const unsigned int line_num, std::unique_ptr<Expression> func,
			std::unique_ptr<ExpressionList> args ) : Expression( file_name, line_num ), function( std::move( func ) ),
			arguments( std::move( args ) ), caller( L"" ), returns_value( false ){
		}

		const ExpressionType GetExpressionType() {
			return ExpressionType::FUNCTION_CALL_EXPR;
		}

		void SetReturnsValue( bool r ) {
			returns_value = r;
		}

		bool ReturnsValue() {
			return returns_value;
		}
		void Analyze( SemaCheck & );
	};

	
	class Dump : public Statement {
		std::unique_ptr<Expression> expression;

	public:
		Dump( const std::wstring &file_name, const unsigned int line_num, std::unique_ptr<Expression> expr )
			: Statement( file_name, line_num ), expression( std::move( expr )) {
		}

		std::unique_ptr<Expression>& GetExpression() {
			return expression;
		}

		const StatementType GetStatementType() {
			return SHOW_STATEMENT;
		}
		void Analyze( SemaCheck & ) override;
	};

	
	class SwitchStatement : public Statement {
		std::unique_ptr<Expression> conditional_expression;
		std::unique_ptr<Statement>	switch_statement;

	public:
		SwitchStatement( const std::wstring &file_name, const unsigned int line_num,
			std::unique_ptr<Expression> logical_exp, std::unique_ptr<Statement> body ) :
			Statement( file_name, line_num ), conditional_expression( std::move( logical_exp ) ),
			switch_statement( std::move( body ) ){
		}

		std::unique_ptr<Expression>& GetExpression() {
			return conditional_expression;
		}

		std::unique_ptr<Statement>& GetSwitchBlock() {
			return switch_statement;
		}

		StatementType const GetStatementType() override {
			return StatementType::SWITCH_STATEMENT;
		}
		void Analyze( SemaCheck & ) override;
	};


	class DoWhileStatement : public Statement {
		std::unique_ptr<Statement>	do_while_body;
		std::unique_ptr<Expression> logical_expression;
	public:
		DoWhileStatement( std::wstring const & filename, unsigned int const line_num, std::unique_ptr<Statement> body,
			std::unique_ptr<Expression> expr ) : Statement( filename, line_num ),
			do_while_body( std::move( body ) ), logical_expression( std::move( expr ) ){
		}

		std::unique_ptr<Statement>& GetStatement(){
			return do_while_body;
		}
		std::unique_ptr<Expression> & GetExpression(){
			return logical_expression;
		}

		StatementType const GetStatementType() override {
			return StatementType::DO_WHILE_STATEMENT;
		}
		void Analyze( SemaCheck & ) override;
	};

	class WhileStatement : public Statement {
		std::unique_ptr<Expression>	logical_expression;
		std::unique_ptr<Statement>	while_body;

	public:
		WhileStatement( std::wstring const &file_name, unsigned int const line_num, std::unique_ptr<Expression> logical_expr,
			std::unique_ptr<Statement> block )
			: Statement( file_name, line_num ), logical_expression( std::move( logical_expr ) ),
			while_body( std::move( block ) ){
		}

		std::unique_ptr<Expression>& GetExpression() {
			return logical_expression;
		}

		std::unique_ptr<Statement>& GetStatement() {
			return while_body;
		}

		const StatementType GetStatementType() {
			return WHILE_STATEMENT;
		}
		void Analyze( SemaCheck & ) override;
	};

	class ForEachStatement : public Statement {
		std::unique_ptr<Expression> expression;
		std::unique_ptr<Statement>	body_statement;
	public:
		ForEachStatement( std::wstring const & filename, unsigned int const line_number,
			std::unique_ptr<Expression> expr, std::unique_ptr<Statement> body ) :
			Statement( filename, line_number ), expression( std::move( expr ) ),
			body_statement( std::move( body ) ){
		}

		StatementType const GetStatementType() override {
			return StatementType::FOR_EACH_IN_STATEMENT;
		}
		void Analyze( SemaCheck & ) override;
	};

	class IfStatement : public Statement {
		friend class TreeFactory;
		std::unique_ptr<Expression> conditional_expression;
		std::unique_ptr<Statement>	then_statement;
		std::unique_ptr<Statement>	else_statement;

	public:
		IfStatement( const std::wstring &file_name, const unsigned int line_num, std::unique_ptr<Expression> logical_exp,
			std::unique_ptr<Statement> then_part, std::unique_ptr<Statement> else_part )
			: Statement( file_name, line_num ), conditional_expression( std::move( logical_exp ) ),
			then_statement( std::move( then_part ) ), else_statement( std::move( else_part ) ){
		}

		std::unique_ptr<Expression>& GetExpression() {
			return conditional_expression;
		}

		std::unique_ptr<Statement>& GetIfBlock() {
			return then_statement;
		}

		std::unique_ptr<Statement>& GetElseBlock() {
			return else_statement;
		}

		const StatementType GetStatementType() {
			return IF_ELSE_STATEMENT;
		}
		void Analyze( SemaCheck & ) override;
	};

	class UnaryExpression : public Expression {
	protected:
		UnaryExpression( std::wstring const & filename, unsigned int const line_num ) :
			Expression( filename, line_num ){
		}
		virtual ~UnaryExpression() = default;
	};

	class BinaryExpression : public Expression {
	protected:
		Token const					token;
		std::unique_ptr<Expression> lhs;
		std::unique_ptr<Expression> rhs;
	public:
		BinaryExpression( Token const tok, std::unique_ptr<Expression> lhs_expression,
			std::unique_ptr<Expression> rhs_expression ) : Expression( token.GetFileName(), token.GetLineNumber() ),
			token( token ), lhs( std::move( lhs_expression ) ), rhs( std::move( rhs_expression ) ){
		}
		virtual ~BinaryExpression() = default;
		std::unique_ptr<Expression>& GetLHSExpression() {
			return lhs;
		}
		std::unique_ptr<Expression>& GetRHSExpression() {
			return rhs;
		}
		virtual ExpressionType const GetExpressionType() override {
			return ExpressionType::BINARY_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	/****************************
	 * Assignment Expression
	 ****************************/
	class AssignmentExpression : public BinaryExpression {
		friend class TreeFactory;
	public:
		AssignmentExpression( Token const tok, std::unique_ptr<Expression> lhs_expression,
			std::unique_ptr<Expression> rhs_expression ) :
			BinaryExpression( tok, std::move( lhs_expression ), std::move( rhs_expression ) ){
		}

		ScannerTokenType GetAssignmentType() {
			return token.GetType();
		}

		std::unique_ptr<Expression>& GetLHSExpression() {
			return lhs;
		}

		std::unique_ptr<Expression>& GetRHSExpression() {
			return rhs;
		}

		ExpressionType const GetExpressionType() override {
			return ExpressionType::ASSIGNMENT_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	class Variable : public Expression
	{
		std::wstring const variable_name;
	public:
		Variable( Token const &tok ) : Expression( tok.GetFileName(), tok.GetLineNumber() ), variable_name( tok.GetIdentifier() ){
		}
		ExpressionType const GetExpressionType() override {
			return ExpressionType::VARIABLE_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	class ConditionalExpression : public Expression
	{
		std::unique_ptr<Expression> conditional_expression;
		std::unique_ptr<Expression>	lhs_expression;
		std::unique_ptr<Expression> rhs_expression;
	public:
		ConditionalExpression( std::wstring const & filename, unsigned int const line_num,
			std::unique_ptr<Expression> conditional, std::unique_ptr<Expression> lhs,
			std::unique_ptr<Expression> rhs ) : Expression( filename, line_num ),
			conditional_expression( std::move( conditional ) ), lhs_expression( std::move( lhs ) ),
			rhs_expression( std::move( rhs ) )
		{
		}

		std::unique_ptr<Expression>& GetConditionalExpression(){
			return conditional_expression;
		}

		std::unique_ptr<Expression>& GetRhsExpression(){
			return rhs_expression;
		}
		std::unique_ptr<Expression>& GetLhsExpression(){
			return lhs_expression;
		}

		ExpressionType const GetExpressionType() override {
			return ExpressionType::CONDITIONAL_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	class UnaryOperation : public UnaryExpression {
		std::unique_ptr<Expression>	expression;
		ScannerTokenType			type;
	public:
		UnaryOperation( std::wstring const & filename, unsigned int const line_num, 
			ScannerTokenType t, std::unique_ptr<Expression> expr ) :
			UnaryExpression( filename, line_num ), type( t ), 
			expression( std::move( expr ) ){
		}

		std::unique_ptr<Expression>& GetExpression(){
			return expression;
		}

		ScannerTokenType OperationType() const {
			return type;
		}

		ExpressionType const GetExpressionType() final override {
			return ExpressionType::UNARY_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	class PostfixExpression : public UnaryExpression
	{
	protected:
		PostfixExpression( std::wstring const & filename, unsigned int const line_number ) :
			UnaryExpression( filename, line_number ){
		}
		virtual ~PostfixExpression() = default;
	};

	class SubscriptExpression : public PostfixExpression
	{
		std::unique_ptr<Expression> expression;
		std::unique_ptr<Expression> array_index;
	public:
		SubscriptExpression( std::wstring const & filename, unsigned int const line_number,
			std::unique_ptr<Expression> expr, std::unique_ptr<Expression> index ) :
			PostfixExpression( filename, line_number ), expression( std::move( expr ) ),
			array_index( std::move( index ) ){
		}

		std::unique_ptr<Expression>& GetExpression(){
			return expression;
		}
		std::unique_ptr<Expression>& GetIndex(){
			return array_index;
		}
		ExpressionType const GetExpressionType() final override {
			return ExpressionType::SUBSCRIPT_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	class DotExpression : public PostfixExpression
	{
		Token const					variable_id;
		std::unique_ptr<Expression> expression;
	public:
		DotExpression( std::wstring const & filename, unsigned int const line_number, Token const id,
			std::unique_ptr<Expression> expr ) : PostfixExpression( filename, line_number ),
			variable_id( id ), expression( std::move( expr ) ){
		}

		std::unique_ptr<Expression>& GetExpression(){
			return expression;
		}

		ExpressionType const GetExpressionType() override {
			return ExpressionType::DOT_EXPRESSION;
		}
		void Analyze( SemaCheck & );
	};

	class PostIncrExpression : public PostfixExpression
	{
		std::unique_ptr<Expression> expression;
	public:
		PostIncrExpression( std::wstring const & filename, unsigned int const line_number, std::unique_ptr<Expression> expr ) :
			PostfixExpression( filename, line_number ), expression( std::move( expr ) ){
		}

		std::unique_ptr<Expression>& GetExpression(){
			return expression;
		}

		ExpressionType const GetExpressionType() final override {
			return ExpressionType::POST_INCR_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	class PostDecrExpression : public PostfixExpression
	{
		std::unique_ptr<Expression> expression;
	public:
		PostDecrExpression( std::wstring const & filename, unsigned int const line_number, std::unique_ptr<Expression> expr ) :
			PostfixExpression( filename, line_number ), expression( std::move( expr ) ){
		}

		std::unique_ptr<Expression>& GetExpression(){
			return expression;
		}

		ExpressionType const GetExpressionType() final override {
			return ExpressionType::POST_DECR_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	class PreIncrExpression : public UnaryExpression
	{
		std::unique_ptr<Expression> expression;
	public:
		PreIncrExpression( std::wstring const & filename, unsigned int const line_number, std::unique_ptr<Expression> expr ) :
			UnaryExpression( filename, line_number ), expression( std::move( expr ) ){
		}

		std::unique_ptr<Expression>& GetExpression(){
			return expression;
		}

		ExpressionType const GetExpressionType() final override {
			return ExpressionType::PRE_INCR_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	class PreDecrExpression : public UnaryExpression
	{
		std::unique_ptr<Expression> expression;
	public:
		PreDecrExpression( std::wstring const & filename, unsigned int const line_number, std::unique_ptr<Expression> expr ) :
			UnaryExpression( filename, line_number ), expression( std::move( expr ) ){
		}

		std::unique_ptr<Expression>& GetExpression(){
			return expression;
		}

		ExpressionType const GetExpressionType() final override {
			return ExpressionType::PRE_DECR_EXPR;
		}
		void Analyze( SemaCheck & );
	};

	class ListExpression : public Expression
	{
		std::unique_ptr<ExpressionList> expression_list;
	public:
		ListExpression( Token const & token, std::unique_ptr<ExpressionList> expr_list ) :
			Expression( token.GetFileName(), token.GetLineNumber() ), expression_list( std::move( expr_list ) ){
		}
		ExpressionType const GetExpressionType() override {
			return ExpressionType::LIST_EXPR;
		}

		std::unique_ptr<ExpressionList>& GetExpressionList(){
			return expression_list;
		}
		void Analyze( SemaCheck & );
	};
	/****************************
	 * ParsedFunction class
	 ****************************/
	enum class FunctionType {
		CONSTRUCTOR,
		FUNCTION,
		METHOD
	};

	class ParsedFunction : public ParseNode {
		std::wstring const				name;
		std::wstring					callee_name;
		std::unique_ptr<ExpressionList>	parameters;
		std::unique_ptr<Statement>		function_body;
		AccessType						access;
		FunctionType					function_type;
		int								local_count;
		InstructionType					operation;
		bool							is_static;

	public:
		ParsedFunction( const std::wstring &file_name, const unsigned int line_num, const std::wstring &name,
			std::unique_ptr<ExpressionList> parameters );
		ParsedFunction( const std::wstring &file_name, const unsigned int line_num, InstructionType operation,
			std::unique_ptr<ExpressionList> parameters );

		~ParsedFunction() = default;

		inline std::wstring GetName() {
			return name;
		}

		inline InstructionType GetOperation() {
			return operation;
		}

		inline bool IsOperation() {
			return operation != NO_OP;
		}

		inline std::wstring GetCalleeName() {
			if ( callee_name.size() == 0 ) {
				callee_name = name + L":" + IntToString( ( int ) parameters->GetExpressions().size() );
			}

			return callee_name;
		}

		inline bool IsNew() {
			return name == L"new";
		}

		inline std::unique_ptr<ExpressionList> & GetParameters() {
			return parameters;
		}

		inline void SetFunctionBody( std::unique_ptr<Statement> scope ){
			function_body = std::move( scope );
		}

		inline Scope::list_of_ptrs<Statement>& GetStatements() {
			return dynamic_cast<CompoundStatement*>( function_body.get() )->GetScope()->GetStatements();
		}

		int GetLocalCount() {
			return local_count;
		}

		void SetLocalCount( int local_count ) {
			this->local_count = local_count;
		}

		bool SetAccess( AccessType access ) {
			if ( this->access != AccessType::PUBLIC_ACCESS ) {
				return false;
			}

			this->access = access;
			return true;
		}

		void SetStatic( bool is_static ) {
			this->is_static = is_static;
		}

		bool IsStatic() {
			return this->is_static;
		}

		AccessType GetAccess() {
			return this->access;
		}
		void SetFunctionType( FunctionType ftype ){
			function_type = ftype;
		}

		FunctionType GetFunctionType(){
			return function_type;
		}
		void Analyze( SemaCheck & );
	};

	/****************************
	 * ParsedClass class
	 ****************************/
	class ParsedClass : public ParseNode {
		std::wstring					class_name;
		std::unique_ptr<Scope>			scope;
		std::unique_ptr<ParsedClass>	inherited_class;
	public:
		ParsedClass( const std::wstring &file_name, const unsigned int line_num, const std::wstring &name,
			Scope *parent_scope )
			:ParseNode( file_name, line_num ), class_name( name ), scope( new Scope( parent_scope ) )
		{
		}

		void AddStatement( std::unique_ptr<Statement> statement ){
			scope->AddStatement( std::move( statement ) );
		}

		bool AddClass( std::unique_ptr<ParsedClass> klass ){
			return scope->AddClass( std::move( klass ) );
		}

		bool AddFunction( std::unique_ptr<ParsedFunction> function ){
			return scope->AddFunction( std::move( function ) );
		}

		std::unique_ptr<Scope>& GetScope(){
			return scope;
		}
		Scope* GetParentScope(){
			return scope->GetParentScope();
		}

		std::wstring const GetName() const {
			return class_name;
		}

		~ParsedClass() = default;
		void Analyze( SemaCheck & );
	};

	/****************************
	 * Parsed program class
	 ****************************/
	class ParsedProgram {
		std::unique_ptr<Scope>  global_scope;

		inline void checkAndThrow(){
			if ( !global_scope ){
				throw std::bad_exception( "The global scope has not been set yet." );
			}
		}

	public:
		ParsedProgram(){
		}

		~ParsedProgram() = default;
		template<typename Func>
		bool Visit( Func const & visitor ){
			return visitor.Visit( *this );
		}

		bool AddClass( std::unique_ptr<ParsedClass> klass ) {
			if ( !global_scope ) return nullptr;
			return global_scope->AddClass( std::move( klass ) );
		}

		ParsedClass* GetClass( std::wstring const &name ) {
			if ( !global_scope ) return nullptr;
			auto & classes = global_scope->GetClasses();
			auto& result = classes.find( name );
			if ( result != classes.end() ) {
				return ( result->second ).get();
			}

			return nullptr;
		}

		void AddStatement( std::unique_ptr<Statement> statement ){
			checkAndThrow();
			global_scope->AddStatement( std::move( statement ) );
		}

		inline bool AddFunction( std::unique_ptr<ParsedFunction> function ) {
			if ( !global_scope ) return false;
			return global_scope->AddFunction( std::move( function ) );
		}

		ParsedFunction* GetFunction( const std::wstring &name ) {
			if ( !global_scope ) return nullptr;
			auto& function_table = global_scope->GetFunctions();
			auto& result = function_table.find( name );
			if ( result != function_table.end() ) {
				return result->second.get();
			}

			return nullptr;
		}

		std::unique_ptr<Scope>& GetGlobalScope() {
			return global_scope;
		}

		int GetLocalCount() {
			if ( !global_scope ) return 0;
			return global_scope->LocalCount();
		}

		void SetLocalCount( int local_count ) {
			checkAndThrow();
			global_scope->SetLocalCount( local_count );
		}

		void SetConstructs( std::unique_ptr<Scope> scope ){
			global_scope.reset( scope.release() );
		}
	};

	/****************************
	 * TreeFactory class
	 ****************************/
	class TreeFactory {
	public:
		static std::unique_ptr<Statement> MakeReturnStatement( std::wstring const &file_name, unsigned int line_num,
			std::unique_ptr<Expression> expression ){
			std::unique_ptr<Statement> return_statement{ new ReturnStatement( file_name, line_num, std::move( expression ) ) };
			return return_statement;
		}

		static std::unique_ptr<Statement> MakeContinueStatement( std::wstring const &file_name, unsigned int line_num ){
			std::unique_ptr<Statement> continue_statement{ new ContinueStatement( file_name, line_num ) };
			return continue_statement;
		}

		static std::unique_ptr<Statement> MakeBreakStatement( std::wstring const &file_name, unsigned int line_num ) {
			std::unique_ptr<Statement> break_statement{ new BreakStatement( file_name, line_num ) };
			return break_statement;
		}

		static std::unique_ptr<Statement> MakeExpressionStatement( std::wstring const &file_name, unsigned int line_num,
			std::unique_ptr<Expression> expression )
		{
			std::unique_ptr<Statement> expression_statement{ new ExpressionStatement( file_name, line_num, std::move( expression ) ) };
			return expression_statement;
		}

		static std::unique_ptr<Expression> MakeAssignmentExpression( Token const token, std::unique_ptr<Expression> lhs_expression,
			std::unique_ptr<Expression> rhs_expression )
		{
			std::unique_ptr<Expression> assign_expr{ new AssignmentExpression( token, std::move( lhs_expression ),
				std::move( rhs_expression ) ) };
			return assign_expr;
		}

		static std::unique_ptr<Expression> MakePreIncrExpression( Token const & token, std::unique_ptr<Expression> expr )
		{
			std::unique_ptr<Expression> pre_incr_expression{ new PreIncrExpression( token.GetFileName(), token.GetLineNumber(),
				std::move( expr ) ) };
			return pre_incr_expression;
		}

		static std::unique_ptr<Statement> MakeShowExpressionStatement( Token const & tok, std::unique_ptr<Expression> expr )
		{
			std::unique_ptr<Statement> dump_statement{ new Dump( tok.GetFileName(), tok.GetLineNumber(), std::move( expr ) ) };
			return dump_statement;
		}

		static std::unique_ptr<Expression> MakeVariable( Token const & tok )
		{
			std::unique_ptr<Expression> var{ new Variable( tok ) };
			return var;
		}

		static std::unique_ptr<Expression> MakePreDecrExpression( Token const & token, std::unique_ptr<Expression> expr )
		{
			std::unique_ptr<Expression> pre_decr_expression{ new PreDecrExpression( token.GetFileName(), token.GetLineNumber(),
				std::move( expr ) ) };
			return pre_decr_expression;
		}
		
		static std::unique_ptr<Expression> MakeUnaryOperation( Token const & token, std::unique_ptr<Expression> expr )
		{
			std::unique_ptr<Expression> unary_op{ new UnaryOperation(token.GetFileName(), token.GetLineNumber(), token.GetType(),
				std::move( expr ) ) };
			return unary_op;
		}

		static std::unique_ptr<Expression> MakeIntegerLiteral( Token const & tok ) 
		{
			std::unique_ptr<Expression> int_expr{ new IntegerLiteral( tok.GetFileName(), tok.GetLineNumber(), tok.GetIntLit() ) };
			return int_expr;
		}

		static std::unique_ptr<Expression> MakeFloatLiteral( Token const & tok ) 
		{
			std::unique_ptr<Expression> float_expr{ new FloatLiteral( tok.GetFileName(), tok.GetLineNumber(), tok.GetFloatLit() ) };
			return float_expr;
		}

		static std::unique_ptr<Expression> MakeStringLiteral( Token const & tok )
		{
			std::unique_ptr<Expression> string_expr{ new CharacterString( tok.GetFileName(), tok.GetLineNumber(),
				tok.GetIdentifier() ) };
			return string_expr;
		}
		
		static std::unique_ptr<Expression> MakeCharLiteral( Token const & token )
		{
			std::unique_ptr<Expression> char_expr{ new CharacterLiteral( token.GetFileName(), token.GetLineNumber(), token.GetCharLit() ) };
			return char_expr;
		}
		
		static std::unique_ptr<Expression> MakeBooleanLiteral( Token const & token ) 
		{
			bool const value = token.GetType() == ScannerTokenType::TOKEN_FALSE_LIT ? false : true;
			BooleanLiteral* tmp = new BooleanLiteral( token.GetFileName(), token.GetLineNumber(), value );
			std::unique_ptr<Expression> boolean_expr{ tmp };
			return boolean_expr;
		}

		static std::unique_ptr<Statement> MakeDeclarationList( Token const & tok, DeclarationList::declaration_list_t && decl_list )
		{
			std::unique_ptr<Statement> decl_list_statement{ new DeclarationList( tok.GetFileName(), tok.GetLineNumber(),
				std::move( decl_list ) ) };
			return decl_list_statement;
		}

		static std::unique_ptr<Statement> MakeDeclaration( std::unique_ptr<Declaration> decl )
		{
			std::unique_ptr<Statement> decl_statement{ std::move( decl ) };
			return decl_statement;
		}

		static std::unique_ptr<Expression> MakeListExpression( Token const & tok, std::unique_ptr<ExpressionList> expr )
		{
			std::unique_ptr<Expression> list_expression{ new ListExpression( tok, std::move( expr ) ) };
			return list_expression;
		}
		
		static std::unique_ptr<Expression> MakeMapExpression( Token const & tok, std::vector<MapExpression::expression_ptr_pair_t>
			list )
		{
			std::unique_ptr<Expression> map_expression{ new MapExpression( tok.GetFileName(), tok.GetLineNumber(), std::move( list ) ) };
			return map_expression;
		}

		static std::unique_ptr<Expression> MakeNewExpression( Token const & tok, std::unique_ptr<Expression> expr )
		{
			std::unique_ptr<Expression> expression{ new NewExpression( tok.GetFileName(), tok.GetLineNumber(), std::move( expr ) ) };
			return expression;
		}

		static std::unique_ptr<Expression> MakeNullLitExpression( Token const & token )
		{
			std::unique_ptr<Expression> nullExpr{ new NullLiteral( token.GetFileName(), token.GetLineNumber() ) };
			return nullExpr;
		}
	};

	/****************************
	 * Local symbol table
	 ****************************/
	class InnerTable {
		std::unordered_map<wstring, Declaration*> table;

	public:
		InnerTable() {
		}

		~InnerTable() {
		}

		void MakeEntry( Declaration* declaration ) {
			table.insert( { declaration->GetIdentifier(), declaration } );
		}

		Declaration* GetDeclaration( const wstring &name ) {
			auto result = table.find( name );
			if ( result != table.end() ) {
				return result->second;
			}

			return nullptr;
		}

		void SetIds( int offset ) {
			std::unordered_map<wstring, Declaration*>::iterator iter;
			for ( iter = table.begin(); iter != table.end(); ++iter ) {
				Declaration* declaration = iter->second;
				declaration->SetId( offset++ );
			}
		}
	};

	/****************************
	 * Hierarchical symbol table
	 ****************************/
	class SymbolTable {
		std::deque<InnerTable*> table_hierarchy;
		std::set<Declaration*> declarations;

	public:
		SymbolTable( bool g = false ) {
		}

		~SymbolTable() {

		}

		// TODO: level
		void NewScope() {
			InnerTable* table = new InnerTable;
			table_hierarchy.push_front( table );
		}

		bool PreviousScope() {
			if ( !table_hierarchy.empty() ) {
				table_hierarchy.pop_front();
				return true;
			}

			return false;
		}

		Declaration* GetDeclaration( const wstring &variable ) {
			for ( size_t i = 0; i < table_hierarchy.size(); ++i ) {
				Declaration* value = table_hierarchy[ i ]->GetDeclaration( variable );
				if ( value ) {
					return value;
				}
			}

			return nullptr;
		}

		bool HasEntry( const wstring &name ) {
			return GetDeclaration( name ) != nullptr;
		}

		void SetIds() {
			int local_id = 1;
			int instance_id = 0;
			std::set<Declaration*>::iterator iter;
			for ( iter = declarations.begin(); iter != declarations.end(); ++iter ) {
				Declaration* declaration = *iter;
				if ( declaration->GetType() == DeclarationType::LOCL_DCLR ) {
					// TODO: HACK!!!
					if ( declaration->GetIdentifier() != L"Array" && declaration->GetIdentifier() != L"String"
						&& declaration->GetIdentifier() != L"Bar" ) {
						declaration->SetId( local_id++ );
					}
				}
				else {
					declaration->SetId( instance_id++ );
				}
			}
		}
	};
}

#endif
