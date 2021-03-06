/***************************************************************************
 * Parse tree.
 *
 * Copyright (c) 2017 Joshua Ogunyinka
 * Copyright (c) 2013-2016 Randy Hollines
 * All rights reserved.
 **/

#ifndef __TREE_H__
#define __TREE_H__

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <set>
#include "common.h"
#include "scanner.h"


namespace compiler {
	class Declaration;
	class ExpressionList;
	class Scope;
	class Declaration;
	class DeclarationList;
	class Statement;
	class SemaCheck1;
	struct SemaType;

	enum class DeclarationType;
	enum class StatementType;
	/****************************
	 * Base class for all parse nodes
	 ****************************/

	using std::wstring;

	class ParseNode {
	protected:
		unsigned int line_num;
	public:
		ParseNode( const unsigned int line_number ) : line_num( line_number ), type( nullptr ) {
		}

		virtual ~ParseNode() = default;
		
		const int GetLineNumber() {
			return line_num;
		}
		SemaType	*type;
	};

	/****************************
	 * Expression types
	 ****************************/
	enum class ExpressionType {
		NULL_LIT_EXPR,
		CHAR_LIT_EXPR,
		INT_LIT_EXPR,
		FLOAT_LIT_EXPR,
		BOOLEAN_LIT_EXPR,
		CHAR_STR_EXPR,
		
		// Newly added expressions
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

	class Expression : public ParseNode {
	public:
		Expression( const unsigned int line_num ) : ParseNode( line_num ) {
		}

		virtual ~Expression() {
		}

		virtual const ExpressionType GetExpressionType() = 0;
	};

	class Statement : public ParseNode {
	public:
		Statement( const unsigned int line_num ) : ParseNode( line_num ) {
		}

		virtual ~Statement(){}
		virtual StatementType GetStatementType() const = 0;
	};

	class ExpressionList {
		std::deque<Expression*> expressions;
	public:
		ExpressionList( const unsigned int line_num ) {
		}

		std::deque<Expression*>& GetExpressions() {
			return expressions;
		}
		Expression* GetExpressionAt( unsigned int i ) {
			return expressions.at( i );
		}
		std::deque<Expression*>::size_type Length(){
			return expressions.size();
		}
		void AddExpression( Expression* e ) {
			expressions.push_back( e );
		}
		~ExpressionList(){
			for ( Expression *expression : expressions ){
				delete expression;
				expression = nullptr;
			}
			expressions.clear();
		}
	};

	/****************************
	 * CharacterString class
	 ****************************/
	class CharacterString : public Expression {
		int id;
		std::wstring char_string;
	public:
		CharacterString( const unsigned int line_num, const std::wstring &orig )
			: Expression( line_num ) {
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
		bool value;
	public:
		BooleanLiteral( const unsigned int line_num, bool v )
			: Expression( line_num ), value( v ) {
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
	};

	/****************************
	* NullLiteral class
	****************************/
	class NullLiteral : public Expression {
	public:
		NullLiteral( unsigned int const line_number )
			: Expression( line_number ) {
		}

		~NullLiteral() = default;
	public:
		const ExpressionType GetExpressionType() {
			return ExpressionType::NULL_LIT_EXPR;
		}
	};

	class LambdaExpression : public Expression
	{
		ExpressionList* parameters;
		Statement*		body;
	public:
		LambdaExpression( Token const & tok, ExpressionList* params, Statement* lambda_body )
			: Expression( tok.GetLineNumber() ), parameters( params ), body( lambda_body ){
		}
		ExpressionType const GetExpressionType() override {
			return ExpressionType::LAMBDA_EXPR;
		}
		Statement* GetLambdaBody(){
			return body;
		}
		ExpressionList* GetParamaters(){
			return parameters;
		}
		~LambdaExpression(){
			delete body;
			delete parameters;

			body = nullptr;
			parameters = nullptr;
		}
	};

	class MapExpression : public Expression
	{
	public:
		using expression_ptr_pair_t = std::pair<Expression*, Expression*>;
	private:
		std::vector<expression_ptr_pair_t> list_of_expressions;
	public:
		MapExpression( unsigned int const line_number, std::vector<expression_ptr_pair_t> && list ) :
			Expression( line_number ), list_of_expressions( std::move( list ) ){
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
		~MapExpression(){
			for ( expression_ptr_pair_t ptr_pair : list_of_expressions ){
				delete ptr_pair.first;
				delete ptr_pair.second;
				ptr_pair.first = ptr_pair.second = nullptr;
			}
			list_of_expressions.clear();
		}
	};

	class NewExpression : public Expression
	{
		Expression *expression;

	public:
		NewExpression( unsigned int const line_number, Expression* expr ) :
			Expression( line_number ), expression( expr ){
		}

		ExpressionType const GetExpressionType() override {
			return ExpressionType::NEW_EXPR;
		}
		~NewExpression(){
			delete expression;
			expression = nullptr;
		}
	};
	/****************************
	* CharacterLiteral class
	****************************/
	class CharacterLiteral : public Expression {
		CHAR_T value;

	public:
		CharacterLiteral( const unsigned int line_num, CHAR_T v )
			: Expression( line_num ), value( v ) {
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
	};

	/****************************
	* IntegerLiteral class
	****************************/
	class IntegerLiteral : public Expression {
		INT_T value;
	public:

		IntegerLiteral( const unsigned int line_num, INT_T v )
			: Expression( line_num ), value( v ) {
		}

		~IntegerLiteral() {
		}

		INT_T GetValue() {
			return value;
		}

		const ExpressionType GetExpressionType() {
			return ExpressionType::INT_LIT_EXPR;
		}
	};

	/****************************
	* FloatLiteral class
	****************************/
	class FloatLiteral : public Expression {
		FLOAT_T value;
	public:
		FloatLiteral( const unsigned int line_num, FLOAT_T v )
			: Expression( line_num ), value( v ) {
			value = v;
		}

		~FloatLiteral() {
		}
		FLOAT_T GetValue() {
			return value;
		}

		const ExpressionType GetExpressionType() {
			return ExpressionType::FLOAT_LIT_EXPR;
		}
	};

	/****************************
	 * StatementType enum
	 ****************************/
	enum class StatementType {
		ASSIGNMENT_STATEMENT = -200,
		FUNCTION_CALL_STATEMENT,
		IF_ELSE_STATEMENT,
		DO_WHILE_STATEMENT,
		WHILE_STATEMENT,
		FOR_EACH_IN_STATEMENT,
		LOOP_STATEMENT,

		// declaration
		VARIABLE_DECL_STMT,
		VDECL_LIST_STMT,
		CLASS_DECL_STMT,
		FUNCTION_DECL_STMT,

		COMPOUND_STATEMENT,
		SWITCH_STATEMENT,
		RETURN_STATEMENT,
		CONTINUE_STATEMENT,
		BREAK_STATEMENT,
		LABELLED_STATEMENT,
		CASE_STATEMENT,
		SHOW_STATEMENT,
		EXPR_STATEMENT,
		EMPTY_STMT
	};

	enum class ScopeType {
		NAMESPACE_SCOPE,
		CLASS_SCOPE,
		FUNCTION_SCOPE,
		TEMP_SCOPE
	};

	/**************************************************************************
	*	Scope for constructs; all scopes can have classes, functions and
	*	variable declarations defined in them. The parent scope is the scope
	*	directly above the scope we're referencing, it forms a chained list of
	*	scopes.
	***************************************************************************/
	class Scope {
	public:
		template<typename T>
		using list_of_ptrs = std::vector<T*>;

	private:
		Scope*					parent;			// parent scope
		size_t					local_count;	// number of local variables
		DeclarationList*		declarations;	// all variable declarations for the current scope
		list_of_ptrs<Statement>	statements;		// statements for that scope
		ScopeType				type;
	public:
		explicit Scope( Scope * );
		~Scope();
		Scope*	GetParentScope();
		void	SetParentScope( Scope *parent_scope );
		void	SetScopeType( ScopeType );
		void	AddStatement( Statement* statement );
		bool	AddDeclaration( Declaration* declaration );
		int		LocalCount() const;
		void	SetLocalCount( int count );

		ScopeType					GetScopeType() const;
		DeclarationList*			GetDeclarationList();
		list_of_ptrs<Statement>&	GetStatements();
		Declaration*				FindDeclaration( std::wstring const & identifier );
	};

	enum class DeclarationType {
		INSTANCE_DCLR,
		FUNCTION_DCLR,
		CLASS_DCLR
	};

	enum class AccessType {
		PUBLIC_ACCESS,
		PROTECTED_ACCESS,
		PRIVATE_ACCESS,
		NONE
	};

	enum class StorageType {
		STATIC_STORAGE,
		EXTERN_STORAGE,
		NONE
	};

	class Declaration : public Statement {
		std::wstring name;
		StorageType  storage;
		AccessType   access;
	public:
		Declaration( const unsigned int line_num, std::wstring const &ident = {} ) : Statement( line_num ),
			name( ident ), storage( StorageType::NONE ), access( AccessType::NONE) {
		}

		void SetStorageType( StorageType type ){
			storage = type;
		}
		StorageType GetStorageType() const {
			return storage;
		}
		
		void SetAccessType( AccessType type ){
			access = type;
		}

		AccessType GetAccessType() const {
			return access;
		}

		void SetName( std::wstring const & name_ ){
			name = name_;
		}

		std::wstring const GetName() const {
			return name;
		}
		virtual ~Declaration() = default;
	};


	class DeclarationList : public Declaration
	{
	public:
		using declaration_list_t = std::unordered_multimap<std::wstring, Declaration*>;
	private:
		declaration_list_t declarations;
	public:

		DeclarationList( unsigned int const line_number, AccessType access, StorageType storage, declaration_list_t && decl_list = {} ) :
			Declaration( line_number ), declarations( std::move( decl_list ) ){
			SetAccessType( access );
			SetStorageType( storage );
		}
		~DeclarationList(){
			for ( std::pair< std::wstring const, Declaration*> &decl : declarations ){
				if ( decl.second ){
					delete decl.second;
					decl.second = nullptr;
				}
			}
			declarations.clear();
		}

		bool AddDeclaration( Declaration * decl );

		declaration_list_t& GetDeclarations(){
			return declarations;
		}

		StatementType GetStatementType() const override {
			return StatementType::VDECL_LIST_STMT;
		}
	};

	class CompoundStatement : public Statement {
		Scope* scope;
	public:
		CompoundStatement( unsigned int const line_num, Scope* s ) : Statement( line_num ), scope( s ){
		}
		~CompoundStatement(){
			delete scope;
			scope = nullptr;
		}

		Scope::list_of_ptrs<Statement>& GetStatementList(){
			return scope->GetStatements();
		}
		Scope* GetScope(){
			return scope;
		}

		StatementType GetStatementType() const final override {
			return StatementType::COMPOUND_STATEMENT;
		}
	};


	class ExpressionStatement : public Statement {
		Expression* expression;

	public:
		ExpressionStatement( unsigned int const line_num, Expression* expr ) : Statement( line_num ), expression( expr ){
		}
		~ExpressionStatement(){
			delete expression;
			expression = nullptr;
		}

		Expression* GetExpression(){
			return expression;
		}

		StatementType GetStatementType() const final override {
			return StatementType::EXPR_STATEMENT;
		}
	};

	class EmptyStatement : public Statement {
	public:
		EmptyStatement( unsigned int line_number ) : Statement( line_number ){
		}

		StatementType GetStatementType() const final override{
			return StatementType::EMPTY_STMT;
		}
	};

	class LabelledStatement : public Statement {
		Statement*			label_statement;
		std::wstring const	label_name;

	public:
		LabelledStatement( unsigned int const line_num, std::wstring const & label, Statement* statement ) : Statement( line_num ),
			label_statement( statement ), label_name( label ){
		}
		~LabelledStatement(){
			delete label_statement;
			label_statement = nullptr;
		}

		Statement* GetStatement(){
			return label_statement;
		}

		std::wstring const GetLabelName() const {
			return label_name;
		}
		StatementType GetStatementType() const final override {
			return StatementType::LABELLED_STATEMENT;
		}
	};

	class CaseStatement : public Statement {
		Expression* case_expression;
		Statement*	case_statement;
	public:
		CaseStatement( unsigned int const line_number, Expression* expression, Statement* statement ) : Statement( line_number ),
			case_expression( expression ), case_statement( statement ){
		}
		~CaseStatement(){
			delete case_expression;
			delete case_statement;
			case_expression = nullptr;
			case_statement = nullptr;
		}

		Expression*	GetExpression(){ return case_expression; }
		Statement*	GetStatement() { return case_statement; }
		StatementType GetStatementType() const final override{ return StatementType::CASE_STATEMENT; }
	};

	class ReturnStatement : public Statement {
		Expression* expression;

	public:
		ReturnStatement( const unsigned int line_num, Expression* expr ) : Statement( line_num ), expression( expr ) {
		}
		~ReturnStatement(){
			delete expression;
			expression = nullptr;
		}

		Expression* GetExpression() {
			return expression;
		}

		StatementType GetStatementType() const final override {
			return StatementType::RETURN_STATEMENT;
		}
	};


	class ContinueStatement : public Statement {
	public:
		ContinueStatement( unsigned int const line_num )
			: Statement( line_num ){
		}

		StatementType GetStatementType() const final override {
			return StatementType::CONTINUE_STATEMENT;
		}
	};


	class BreakStatement : public Statement {
	public:
		BreakStatement( const unsigned int line_num ) : Statement( line_num ){
		}

		StatementType GetStatementType() const final override {
			return StatementType::BREAK_STATEMENT;
		}
	};


	class FunctionCall : public Expression {
		Expression*		function;
		ExpressionList* arguments;
		std::wstring	caller;
		bool			returns_value;
	public:
		FunctionCall( const unsigned int line_num, Expression* func, ExpressionList* args ) : Expression( line_num ), 
			function( func ), arguments( args ), caller( L"" ), returns_value( false ){
		}
		~FunctionCall(){
			delete function;
			delete arguments;

			function = nullptr;
			arguments = nullptr;
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

		Expression* GetFunctionExpression(){
			return function;
		}

		ExpressionList* GetArgumentList(){
			return arguments;
		}
	};


	class DumpStatement : public Statement {
		Expression* expression;

	public:
		DumpStatement( unsigned int const line_num, Expression* expr ) : Statement( line_num ), expression( expr ) {
		}
		~DumpStatement(){
			delete expression;
			expression = nullptr;
		}

		Expression* GetExpression() {
			return expression;
		}

		StatementType GetStatementType() const final override {
			return StatementType::SHOW_STATEMENT;
		}
	};

	class SwitchStatement : public Statement {
		Expression* conditional_expression;
		Statement*	switch_statement;

	public:
		SwitchStatement( unsigned int const line_num, Expression* logical_exp, Statement* body ) :
			Statement( line_num ), conditional_expression( logical_exp ),
			switch_statement( body ){
		}
		~SwitchStatement(){
			delete conditional_expression;
			delete switch_statement;

			conditional_expression = nullptr;
			switch_statement = nullptr;
		}

		Expression* GetExpression() {
			return conditional_expression;
		}

		Statement* GetSwitchBlock() {
			return switch_statement;
		}

		StatementType GetStatementType() const final override {
			return StatementType::SWITCH_STATEMENT;
		}
	};


	class DoWhileStatement : public Statement {
		Statement*	do_while_body;
		Expression* logical_expression;
	public:
		DoWhileStatement( unsigned int const line_num, Statement* body, Expression* expr ) : Statement( line_num ), 
			do_while_body( body ), logical_expression( expr ){
		}
		~DoWhileStatement(){
			delete do_while_body;
			delete logical_expression;

			do_while_body = nullptr;
			logical_expression = nullptr;
		}
		Statement* GetStatement(){
			return do_while_body;
		}
		Expression* GetExpression(){
			return logical_expression;
		}

		StatementType GetStatementType() const final override {
			return StatementType::DO_WHILE_STATEMENT;
		}
	};

	class WhileStatement : public Statement {
		Expression*	logical_expression;
		Statement*	while_body;

	public:
		WhileStatement( unsigned int const line_num, Expression* logical_expr, Statement* block ) : Statement( line_num ), 
			logical_expression( logical_expr ), while_body( block ){
		}
		~WhileStatement(){
			delete logical_expression;
			delete while_body;

			logical_expression = nullptr;
			while_body = nullptr;
		}

		Expression* GetExpression() {
			return logical_expression;
		}

		Statement* GetStatement() {
			return while_body;
		}

		StatementType GetStatementType() const final override {
			return StatementType::WHILE_STATEMENT;
		}
	};

	class LoopStatement : public Statement {
		CompoundStatement* loop_body;
	public:
		LoopStatement( unsigned int const line_number, CompoundStatement* body ) : Statement( line_number ), loop_body( body ){
		}
		~LoopStatement(){
			delete loop_body;
			loop_body = nullptr;
		}

		StatementType GetStatementType() const final override {
			return StatementType::LOOP_STATEMENT;
		}
		CompoundStatement* GetLoopBody(){
			return loop_body;
		}
	};

	class ForEachStatement : public Statement {
		Expression* expression;
		Statement*	body_statement;
	public:
		Declaration* decl;
		ForEachStatement( unsigned int const line_number, Expression* expr, Statement* body ) : Statement( line_number ), 
			expression( expr ), body_statement( body ), decl( nullptr ){
		}
		~ForEachStatement(){
			delete expression;
			delete body_statement;
			if ( decl ){
				delete decl;
				decl = nullptr;
			}
			expression = nullptr;
			body_statement = nullptr;
		}

		Expression* GetExpression() const { return expression; }
		Statement*	GetStatement() const  { return body_statement; }

		StatementType GetStatementType() const final override {
			return StatementType::FOR_EACH_IN_STATEMENT;
		}
	};

	class IfStatement : public Statement {
		Expression* conditional_expression;
		Statement*	then_statement;
		Statement*	else_statement;

	public:
		IfStatement( const unsigned int line_num, Expression* logical_exp, Statement* then_part, Statement* else_part )
			: Statement( line_num ), conditional_expression( logical_exp ),
			then_statement( then_part ), else_statement( else_part ){
		}
		~IfStatement(){
			delete conditional_expression;
			delete then_statement;
			delete else_statement;

			conditional_expression = nullptr;
			then_statement = nullptr;
			else_statement = nullptr;
		}

		Expression* GetExpression() {
			return conditional_expression;
		}

		Statement* GetIfBlock() {
			return then_statement;
		}

		Statement* GetElseBlock() {
			return else_statement;
		}

		StatementType GetStatementType() const final override {
			return StatementType::IF_ELSE_STATEMENT;
		}
	};

	class UnaryExpression : public Expression {
	protected:
		UnaryExpression( unsigned int const line_num ) : Expression( line_num ){
		}
		virtual ~UnaryExpression() = default;
	};

	class BinaryExpression : public Expression {
	protected:
		Token const	token;
		Expression*	lhs;
		Expression* rhs;
	public:
		BinaryExpression( Token const & tok, Expression* lhs_expression, Expression* rhs_expression ) :
			Expression( token.GetLineNumber() ),
			token( tok ), lhs( lhs_expression ), rhs( rhs_expression ){
		}

		virtual ~BinaryExpression(){
			delete lhs;
			delete rhs;

			lhs = nullptr;
			rhs = nullptr;
		}

		Token const GetToken() const { return token;  }
		Expression* GetLHSExpression() {
			return lhs;
		}
		Expression* GetRHSExpression() {
			return rhs;
		}
		virtual ExpressionType const GetExpressionType() override {
			return ExpressionType::BINARY_EXPR;
		}
	};

	/****************************
	 * Assignment Expression
	 ****************************/
	class AssignmentExpression : public BinaryExpression {
	public:
		AssignmentExpression( Token const & tok, Expression* lhs_expression, Expression* rhs_expression ) :
			BinaryExpression( tok, lhs_expression, rhs_expression ){
		}

		ScannerTokenType GetAssignmentType() {
			return token.GetType();
		}

		Expression* GetLHSExpression() {
			return lhs;
		}

		Expression* GetRHSExpression() {
			return rhs;
		}

		ExpressionType const GetExpressionType() override {
			return ExpressionType::ASSIGNMENT_EXPR;
		}
	};

	class Variable : public Expression
	{
		std::wstring const variable_name;
	public:
		explicit Variable( Token const &tok ) : Expression( tok.GetLineNumber() ), 
			variable_name( tok.GetIdentifier() ){
		}

		Variable( unsigned int const line_number, std::wstring const & identifier ) :
			Expression( line_number ), variable_name( identifier ){
		}

		ExpressionType const GetExpressionType() override {
			return ExpressionType::VARIABLE_EXPR;
		}
		std::wstring const GetName(){
			return variable_name;
		}
	};

	class ConditionalExpression : public Expression
	{
		Expression* conditional_expression;
		Expression*	lhs_expression;
		Expression* rhs_expression;
	public:
		ConditionalExpression( unsigned int const line_num, Expression* conditional, Expression* lhs, Expression* rhs ) 
			: Expression( line_num ), conditional_expression( conditional ), lhs_expression( lhs ), rhs_expression( rhs ){
		}
		~ConditionalExpression(){
			delete conditional_expression;
			delete lhs_expression;
			delete rhs_expression;

			conditional_expression = nullptr;
			lhs_expression = nullptr;
			rhs_expression = nullptr;
		}

		Expression* GetConditionalExpression(){
			return conditional_expression;
		}

		Expression* GetRhsExpression(){
			return rhs_expression;
		}

		Expression* GetLhsExpression(){
			return lhs_expression;
		}

		ExpressionType const GetExpressionType() override {
			return ExpressionType::CONDITIONAL_EXPR;
		}
	};

	class UnaryOperation : public UnaryExpression {
		Expression			*expression;
		ScannerTokenType	type;
	public:
		UnaryOperation( unsigned int const line_num, ScannerTokenType t, Expression* expr ) :
			UnaryExpression( line_num ), type( t ),
			expression( expr ){
		}

		~UnaryOperation(){
			delete expression;
			expression = nullptr;
		}

		Expression* GetExpression(){
			return expression;
		}

		ScannerTokenType OperationType() const {
			return type;
		}

		ExpressionType const GetExpressionType() final override {
			return ExpressionType::UNARY_EXPR;
		}
	};

	class PostfixExpression : public UnaryExpression
	{
	protected:
		PostfixExpression( unsigned int const line_number ) :
			UnaryExpression( line_number ){
		}
		virtual ~PostfixExpression() = default;
	};

	class SubscriptExpression : public PostfixExpression
	{
		Expression* expression;
		Expression* array_index;
	public:
		SubscriptExpression( unsigned int const line_number, Expression* expr, Expression* index )
			: PostfixExpression( line_number ), expression( expr ), array_index( index ){
		}
		~SubscriptExpression(){
			delete expression;
			delete array_index;

			expression = nullptr;
			array_index = nullptr;
		}

		Expression* GetExpression(){
			return expression;
		}
		Expression* GetIndex(){
			return array_index;
		}
		ExpressionType const GetExpressionType() final override {
			return ExpressionType::SUBSCRIPT_EXPR;
		}
	};

	class DotExpression : public PostfixExpression
	{
		Token const	variable_id;
		Expression* expression;
	public:
		DotExpression( unsigned int const line_number, Token const & id, Expression* expr )
			: PostfixExpression( line_number ), variable_id( id ), expression( expr ){
		}
		~DotExpression(){
			delete expression;
			expression = nullptr;
		}

		Expression* GetExpression(){
			return expression;
		}

		ExpressionType const GetExpressionType() override {
			return ExpressionType::DOT_EXPRESSION;
		}
	};

	class PostIncrExpression : public PostfixExpression
	{
		Expression* expression;
	public:
		PostIncrExpression( unsigned int const line_number, Expression* expr ) :
			PostfixExpression( line_number ), expression( expr ){
		}
		~PostIncrExpression(){
			delete expression;
			expression = nullptr;
		}

		Expression* GetExpression(){
			return expression;
		}

		ExpressionType const GetExpressionType() final override {
			return ExpressionType::POST_INCR_EXPR;
		}
	};

	class PostDecrExpression : public PostfixExpression
	{
		Expression* expression;
	public:
		PostDecrExpression( unsigned int const line_number, Expression* expr ) :
			PostfixExpression( line_number ), expression( expr ){
		}
		~PostDecrExpression(){
			delete expression;
			expression = nullptr;
		}

		Expression* GetExpression(){
			return expression;
		}

		ExpressionType const GetExpressionType() final override {
			return ExpressionType::POST_DECR_EXPR;
		}
	};

	class PreIncrExpression : public UnaryExpression
	{
		Expression* expression;
	public:
		PreIncrExpression( unsigned int const line_number, Expression* expr ) : UnaryExpression( line_number ), expression( expr ){
		}
		~PreIncrExpression(){
			delete expression;
			expression = nullptr;
		}

		Expression* GetExpression(){
			return expression;
		}

		ExpressionType const GetExpressionType() final override {
			return ExpressionType::PRE_INCR_EXPR;
		}
	};

	class PreDecrExpression : public UnaryExpression
	{
		Expression* expression;
	public:
		PreDecrExpression( unsigned int const line_number, Expression* expr ) :
			UnaryExpression( line_number ), expression( expr ){
		}
		~PreDecrExpression(){
			delete expression;
			expression = nullptr;
		}

		Expression* GetExpression(){
			return expression;
		}

		ExpressionType const GetExpressionType() final override {
			return ExpressionType::PRE_DECR_EXPR;
		}
	};

	class ListExpression : public Expression
	{
		ExpressionList* expression_list;
	public:
		ListExpression( Token const & token, ExpressionList* expr_list ) :
			Expression( token.GetLineNumber() ), expression_list( expr_list ){
		}
		~ListExpression(){
			delete expression_list;
			expression_list = nullptr;
		}

		ExpressionType const GetExpressionType() override {
			return ExpressionType::LIST_EXPR;
		}

		ExpressionList* GetExpressionList(){
			return expression_list;
		}
	};
	/****************************
	 * ParsedFunction class
	 ****************************/
	enum class FunctionType {
		CONSTRUCTOR,
		FUNCTION,
		METHOD
	};

	class FunctionDeclaration : public Declaration {
		StorageType			storage;
		AccessType			access;
		FunctionType		function_type;
		ExpressionList*		parameters;
		CompoundStatement*	function_body;
		unsigned int		local_count;
		unsigned int		nparams_count;

	public:
		FunctionDeclaration( const unsigned int line_num, const std::wstring &function_name,
			ExpressionList* params ) : Declaration( line_num, function_name ), storage( StorageType::NONE ),
			access( AccessType::NONE ), function_type( FunctionType::FUNCTION ), parameters( params ),
			function_body( nullptr ), local_count( 0 ), nparams_count( 0 ){
		}

		~FunctionDeclaration(){
			delete function_body;
			delete parameters;

			function_body = nullptr;
			parameters = nullptr;
		}

		CompoundStatement* GetFunctionBody() const { return function_body;  }
		StatementType GetStatementType() const final override {
			return StatementType::FUNCTION_DECL_STMT;
		}

		ExpressionList* GetParameters() const {
			return parameters;
		}

		inline void SetFunctionBody( CompoundStatement* body ){
			if ( function_body ){
				// who knows? I may be stupid enough to actually do this more than once
				delete function_body;
				function_body = nullptr;
			}
			function_body = body;
		}

		inline Scope::list_of_ptrs<Statement>& GetStatements() {
			return function_body->GetScope()->GetStatements();
		}

		int GetLocalCount() {
			return local_count;
		}

		void SetLocalCount( int local_count ) {
			this->local_count = local_count;
		}

		void SetAccess( AccessType access_type ) {
			access = access_type;
		}

		void SetStorageType( StorageType storage_type ){
			storage = storage_type;
		}

		AccessType GetAccessType() {
			return access;
		}

		StorageType GetStorageType() {
			return storage;
		}

		void SetFunctionType( FunctionType ftype ){
			function_type = ftype;
		}

		FunctionType GetFunctionType(){
			return function_type;
		}
		// to-do
		bool AddDeclaration( Declaration* );
	};

	class VariableDeclaration : public Declaration
	{
		bool is_const_;
		Expression* value_expr;
	public:
		VariableDeclaration( unsigned int const line_number, std::wstring const & id, Expression* expr, bool is_const ) : 
			Declaration( line_number, id ), is_const_( is_const ),
			value_expr( expr ){
		}
		~VariableDeclaration(){
			delete value_expr;
			value_expr = nullptr;
		}

		StatementType GetStatementType() const final override {
			return StatementType::VARIABLE_DECL_STMT;
		}
		Expression* GetExpression(){
			return value_expr;
		}
	};

	class ClassDeclaration : public Declaration {
		bool				is_struct_;
		std::wstring		base_class_name;
		Scope				*scope; // functions, methods, variables, ctors;
		unsigned int		instance_variable_count;
		unsigned int		static_variable_count;

		std::vector<Declaration*> decl_list;
	public:
		ClassDeclaration( const unsigned int line_num, const std::wstring &name, Scope *parent, bool is_struct ) 
			:Declaration( line_num, name ), is_struct_( is_struct ),
			scope( new Scope( parent ) ),
			instance_variable_count(0), static_variable_count( 0 )
		{
		}
		~ClassDeclaration(){
			if ( !decl_list.empty() ){
				for ( auto & decl : decl_list ){
					delete decl;
					decl = nullptr;
				}
				decl_list.clear();
			}

			delete scope;
			scope = nullptr;
		}

		Scope* GetClassScope(){
			return scope;
		}
		
		DeclarationList* GetDeclarations() const {
			return scope->GetDeclarationList();
		}

		std::vector<Declaration*>& GetDeclList() { return decl_list; }
		void AddStatement( Declaration *stmt ){
			decl_list.push_back( stmt );
		}
		bool AddDeclaration( Declaration* decl ){
			return scope->AddDeclaration( decl );
		}

		StatementType GetStatementType() const override final {
			return StatementType::CLASS_DECL_STMT;
		}

		bool IsStruct(){
			return is_struct_;
		}

		void SetBaseClass( std::wstring const & id ){
			base_class_name = id;
		}

		std::wstring GetBaseClassName()const {
			return base_class_name;
		}
	};

	/****************************
	 * Parsed program class
	 ****************************/
	class ParsedProgram {
		Scope*  global_scope;

		inline void checkAndThrow(){
			if ( !global_scope ){
				throw std::bad_exception( "The global scope has not been set yet." );
			}
		}

	public:
		ParsedProgram(): global_scope( nullptr ){
		}

		~ParsedProgram(){
			delete global_scope;
			global_scope = nullptr;
		}

		template<typename Func, typename ...Args>
		bool Visit( Func & visitor, Args &&...args ){
			return visitor.Visit( this, std::forward<Args>( args )... );
		}

		void AddStatement( Statement* statement ){
			checkAndThrow();
			global_scope->AddStatement( statement );
		}

		inline bool AddDeclaration( Declaration* decl )
		{
			return global_scope->AddDeclaration( decl );
		}

		Scope* GetGlobalScope() {
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

		void SetConstructs( Scope* scope ){
			if ( global_scope ){
				delete global_scope;
				global_scope = nullptr;
			}
			global_scope = scope;
		}
	};

	/****************************
	 * TreeFactory class
	 ****************************/
	struct TreeFactory {
		static Statement* MakeReturnStatement( unsigned int line_num, Expression* expression ){
			Statement* return_statement{ new ReturnStatement( line_num, expression ) };
			return return_statement;
		}

		static Statement* MakeContinueStatement( unsigned int line_num ){
			Statement* continue_statement{ new ContinueStatement( line_num ) };
			return continue_statement;
		}

		static Statement* MakeBreakStatement( unsigned int line_num ) {
			Statement* break_statement{ new BreakStatement( line_num ) };
			return break_statement;
		}

		static Statement* MakeExpressionStatement( unsigned int line_num, Expression* expression )
		{
			Statement* expression_statement{ new ExpressionStatement( line_num, expression ) };
			return expression_statement;
		}

		static Expression* MakeAssignmentExpression( Token const & token, Expression* lhs_expression,
			Expression* rhs_expression )
		{
			Expression* assign_expr{ new AssignmentExpression( token, lhs_expression, rhs_expression ) };
			return assign_expr;
		}

		static Expression* MakePreIncrExpression( Token const & token, Expression* expr )
		{
			Expression* pre_incr_expression{ new PreIncrExpression( token.GetLineNumber(), expr ) };
			return pre_incr_expression;
		}

		static Statement* MakeShowExpressionStatement( Token const & tok, Expression* expr )
		{
			Statement* dump_statement{ new DumpStatement( tok.GetLineNumber(), expr ) };
			return dump_statement;
		}

		static Expression* MakeVariable( Token const & tok )
		{
			Expression* var{ new Variable( tok ) };
			return var;
		}

		static Expression* MakePreDecrExpression( Token const & token, Expression* expr )
		{
			Expression* pre_decr_expression{ new PreDecrExpression( token.GetLineNumber(), expr ) };
			return pre_decr_expression;
		}

		static Expression* MakeUnaryOperation( Token const & token, Expression* expr )
		{
			Expression* unary_op{ new UnaryOperation( token.GetLineNumber(), token.GetType(), expr ) };
			return unary_op;
		}

		static Expression* MakeIntegerLiteral( Token const & tok )
		{
			Expression* int_expr{ new IntegerLiteral( tok.GetLineNumber(), tok.GetIntLit() ) };
			return int_expr;
		}

		static Expression* MakeFloatLiteral( Token const & tok )
		{
			Expression* float_expr{ new FloatLiteral( tok.GetLineNumber(), tok.GetFloatLit() ) };
			return float_expr;
		}

		static Expression* MakeStringLiteral( Token const & tok )
		{
			Expression* string_expr{ new CharacterString( tok.GetLineNumber(),
				tok.GetIdentifier() ) };
			return string_expr;
		}

		static Expression* MakeCharLiteral( Token const & token )
		{
			Expression* char_expr{ new CharacterLiteral( token.GetLineNumber(), token.GetCharLit() ) };
			return char_expr;
		}

		static Expression* MakeBooleanLiteral( Token const & token )
		{
			bool const value = token.GetType() == ScannerTokenType::TOKEN_FALSE_LIT ? false : true;
			BooleanLiteral* tmp = new BooleanLiteral( token.GetLineNumber(), value );
			return tmp;
		}

		static Statement* MakeDeclaration( Declaration* decl )
		{
			Statement* decl_statement{ decl };
			return decl_statement;
		}

		static Expression* MakeListExpression( Token const & tok, ExpressionList* expr )
		{
			Expression* list_expression{ new ListExpression( tok, expr ) };
			return list_expression;
		}

		static Expression* MakeMapExpression( Token const & tok, std::vector<MapExpression::expression_ptr_pair_t> &&
			list )
		{
			Expression* map_expression{ new MapExpression( tok.GetLineNumber(), std::move( list ) ) };
			return map_expression;
		}

		static Expression* MakeNewExpression( Token const & tok, Expression* expr )
		{
			Expression* expression{ new NewExpression( tok.GetLineNumber(), expr ) };
			return expression;
		}

		static Expression* MakeNullLitExpression( Token const & token )
		{
			Expression* nullExpr{ new NullLiteral( token.GetLineNumber() ) };
			return nullExpr;
		}
	};
}

#endif
