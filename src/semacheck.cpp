#include "semacheck.h"
#include "tree.h"

/* Copyright (c) 2017 Joshua Ogunyinka */
namespace compiler
{
	SemaCheck1::SemaCheck1() : is_parsing_loops( false ), is_parsing_function( false ),
		is_parsing_class( false ){
	}

	bool SemaCheck1::Visit( ParsedProgram* parsed_program )
	{
		AnalyzeScope( parsed_program->GetGlobalScope() );
		return error_messages.size() ? false : true;
	}

	void SemaCheck1::AnalyzeScope( Scope* scope )
	{
		Scope::list_of_ptrs<Statement> &statement_list = scope->GetStatements();
		for ( Statement* statement : statement_list )
		{
			switch ( statement->GetStatementType() )
			{
				// add this declaration to the symbol table
			case StatementType::CLASS_DECL_STMT:
			case StatementType::FUNCTION_DECL_STMT:
			case StatementType::VARIABLE_DECL_STMT:
			case StatementType::VDECL_LIST_STMT:
			{
				AnalyzeDeclaration( statement, scope );
				break;
			}
			case StatementType::BREAK_STATEMENT:
			case StatementType::CONTINUE_STATEMENT:
			case StatementType::RETURN_STATEMENT:
			{
				AnalyzeJumpStatement( statement, scope );
				break;
			}
			case StatementType::FOR_EACH_IN_STATEMENT:
			case StatementType::DO_WHILE_STATEMENT:
			case StatementType::LOOP_STATEMENT:
			case StatementType::WHILE_STATEMENT:
			{
				AnalyzeLoopingStatements( statement, scope );
				break;
			}
			case StatementType::SWITCH_STATEMENT:
			{
				AnalyzeSwitchStatement( statement, scope );
				break;
			}
			case StatementType::SHOW_STATEMENT:
			{
				AnalyzeShowStatement( statement, scope );
				break;
			}

			case StatementType::IF_ELSE_STATEMENT:
			{
				AnalyzeIfStatement( statement, scope );
				break;
			}
			//case StatementType::
			case StatementType::EXPR_STATEMENT:
			{
				Expression *expr = dynamic_cast< ExpressionStatement* >( statement )->GetExpression();
				AnalyzeExpression( expr, scope );
				break;
			}
			case StatementType::EMPTY_STMT:
			default:
				break;
			} // end-switch
		} // end-for
	} //end-function

	void SemaCheck1::AnalyzeIfStatement( Statement *statement, SCOPE )
	{
		IfStatement *if_statement = dynamic_cast< IfStatement* >( statement );
		AnalyzeExpression( if_statement->GetExpression(), scope );
		Scope *then_block = dynamic_cast< CompoundStatement* >( if_statement->GetIfBlock() )->GetScope();
		then_block->SetParentScope( scope );
		AnalyzeScope( then_block );

		if ( if_statement->GetElseBlock() ){
			Scope *else_block = dynamic_cast< CompoundStatement* >( if_statement->GetElseBlock() )->GetScope();
			else_block->SetParentScope( scope );
			AnalyzeScope( else_block );
		}
	}

	void SemaCheck1::AnalyzeSwitchStatement( Statement *statement, SCOPE )
	{
		bool temp = is_parsing_loops;
		is_parsing_loops = true;
		SwitchStatement* switch_statement = dynamic_cast< SwitchStatement* >( statement );
		AnalyzeExpression( switch_statement->GetExpression(), scope );
		AnalyzeScope( dynamic_cast< CompoundStatement* >( switch_statement->GetSwitchBlock() )->GetScope() );
		is_parsing_loops = temp;
	}

	void SemaCheck1::AnalyzeExpressionStatement( Statement *statement, SCOPE )
	{
		Expression *expression = dynamic_cast< ExpressionStatement* >( statement )->GetExpression();

		switch ( expression->GetExpressionType() )
		{
		case ExpressionType::ASSIGNMENT_EXPR:
		{
			AssignmentExpression *assign_expr = dynamic_cast< AssignmentExpression* >( expression );
			Expression *lhs_expr = assign_expr->GetLHSExpression();
			if ( lhs_expr->GetExpressionType() == ExpressionType::VARIABLE_EXPR ){
				std::wstring const variable_name = dynamic_cast< Variable* >( lhs_expr )->GetName();
				if ( !scope->FindDeclaration( variable_name ) ){
					auto decl = new VariableDeclaration( assign_expr->GetLineNumber(), variable_name,
						assign_expr->GetRHSExpression(), false );
					scope->AddDeclaration( decl );
				}
			}
			AnalyzeExpression( assign_expr->GetRHSExpression(), scope );
			break;
		}
		case ExpressionType::PRE_INCR_EXPR:
		{
			PreIncrExpression* pre_incr_expr = dynamic_cast< PreIncrExpression* >( expression );
			AnalyzeExpression( pre_incr_expr->GetExpression(), scope );
			break;
		}
		case ExpressionType::PRE_DECR_EXPR:
		{
			PreDecrExpression* pre_decr_expr = dynamic_cast< PreDecrExpression* >( expression );
			AnalyzeExpression( pre_decr_expr, scope );
			break;
		}
		case ExpressionType::UNARY_EXPR:
		{
			AnalyzeExpression( dynamic_cast< UnaryOperation* >( expression )->GetExpression(), scope );
			break;
		}
		case ExpressionType::POST_DECR_EXPR:
			AnalyzeExpression( dynamic_cast< PostDecrExpression* >( expression )->GetExpression(), scope );
			break;
		case ExpressionType::POST_INCR_EXPR:
			AnalyzeExpression( dynamic_cast< PostIncrExpression* >( expression )->GetExpression(), scope );
			break;
		case ExpressionType::FUNCTION_CALL_EXPR:
		{
			FunctionCall* function_call_expr = dynamic_cast< FunctionCall* >( expression );

			break;
		}
		}
	}

	void SemaCheck1::AppendError( std::wstring const & error )
	{
		error_messages.push_back( error );
	}

	void SemaCheck1::ReportErrors()
	{
		for ( auto const & error : error_messages ){
			std::wcerr << L"Error: " << error << "\n";
		}
	}

	void SemaCheck1::AnalyzeDeclaration( Statement *statement, Scope *scope )
	{
		if ( statement->GetStatementType() == StatementType::VDECL_LIST_STMT )
		{
			DeclarationList* decl_list = dynamic_cast< DeclarationList* >( statement );
			if ( scope->GetScopeType() != ScopeType::CLASS_SCOPE && ( decl_list->GetAccessType() != AccessType::NONE ) ){
				AppendError( L" On line " + IntToString( decl_list->GetLineNumber() )
					+ L": An access type is only expected in a class scope" );
			}

			for ( std::pair<std::wstring const, Declaration*>& declaration : decl_list->GetDeclarations() ){
				if ( !scope->AddDeclaration( declaration.second ) ){
					AppendError( L"variable '" + ( declaration.second )->GetName() + L"' has already been declared in this scope." );
				}
			}
		}
		else {
			Declaration *decl = dynamic_cast< Declaration* >( statement );
			StatementType const type = decl->GetStatementType();
			if ( !scope->AddDeclaration( decl ) ){
				AppendError( L"On line " + IntToString( decl->GetLineNumber() ) + L": " + decl->GetName() + L" redeclared" );
				return;
			}

			switch ( type )
			{
			case StatementType::CLASS_DECL_STMT:
				AnalyzeClassDeclaration( decl, scope );
				break;
			case StatementType::FUNCTION_DECL_STMT:
				AnalyzeFunctionDeclaration( decl, scope );
				break;
			default:
				break;
			}
		}
	} // end SemaCheck1::AnalyzeDeclaration

	void SemaCheck1::AnalyzeClassDeclaration( Declaration* decl, Scope *parent_scope )
	{
		bool temp_parsing_class = is_parsing_class;
		is_parsing_class = true;

		ClassDeclaration* class_declaration = dynamic_cast< ClassDeclaration* >( decl );
		Scope* class_scope = class_declaration->GetClassScope();
		class_scope->SetScopeType( ScopeType::CLASS_SCOPE );
		class_scope->SetParentScope( parent_scope );

		auto &decl_list = class_declaration->GetDeclList();
		for ( auto &class_elem_decl : decl_list ){ // class_elem_decl is a std::pair<std::wstring const, Declaration*> 
			AnalyzeDeclaration( class_elem_decl, class_scope );
		}
		is_parsing_class = temp_parsing_class;
	}

	void SemaCheck1::AnalyzeFunctionDeclaration( Declaration* decl, Scope *parent_scope )
	{
		bool temp_in_function = is_parsing_function;
		is_parsing_function = true;

		FunctionDeclaration* function_decl = dynamic_cast< FunctionDeclaration* >( decl );
		ExpressionList *parameters = function_decl->GetParameters();
		FunctionType const function_type = function_decl->GetFunctionType();

		if ( parent_scope->GetScopeType() != ScopeType::CLASS_SCOPE && function_decl->GetAccessType() != AccessType::NONE ){
			AppendError( L"On line " + IntToString( function_decl->GetLineNumber() ) + L": access type outside an immediate enclosing class" );
		}
		if ( parent_scope->GetScopeType() != ScopeType::CLASS_SCOPE && ( function_type == FunctionType::CONSTRUCTOR
			|| function_type == FunctionType::METHOD ) )
		{
			AppendError( L"On line " + IntToString( function_decl->GetLineNumber() ) + L": " + std::wstring( L"A " )
				+ ( function_type == FunctionType::CONSTRUCTOR ? L"constructor" : L"method" ) +
				L" cannot be used when the enclosing scope isn't a class definition" );
		}

		// if there are no duplicates/invalid-expressions in the paramter, proceed to analyzing the body
		if ( CheckParameterDuplicates( parameters, function_decl->GetLineNumber() ) ){
			Scope *function_scope = function_decl->GetFunctionBody()->GetScope();
			function_scope->SetScopeType( ScopeType::FUNCTION_SCOPE );
			function_scope->SetParentScope( parent_scope );
			AnalyzeScope( function_scope );
		}

		is_parsing_function = temp_in_function;
	}

	void SemaCheck1::AnalyzeJumpStatement( Statement * statement, SCOPE )
	{
		StatementType const type = statement->GetStatementType();
		if ( type == StatementType::RETURN_STATEMENT )
		{
			if ( !is_parsing_function ){
				AppendError( L"On line " + IntToString( statement->GetLineNumber() ) + L": A return statement not expected outside of "
					L"an enclosing function" );
			}
			else {
				Expression *ret_expression = dynamic_cast< ReturnStatement* >( statement )->GetExpression();
				if ( ret_expression ){
					AnalyzeExpression( ret_expression, scope );
				}
			}
			return;
		}

		if ( !is_parsing_loops ){
			std::wstring const type_name = type == StatementType::BREAK_STATEMENT ? L"break" : L"continue";
			AppendError( L"A " + type_name + L" statement not expected outside of a looping construct." );
		}
	}

	void SemaCheck1::AnalyzeLoopingStatements( Statement *statement, SCOPE )
	{
		bool temp = is_parsing_loops;
		is_parsing_loops = true;

		switch ( statement->GetStatementType() ){
		case StatementType::FOR_EACH_IN_STATEMENT:
			AnalyzeForEachStatement( statement, scope );
			break;
		case StatementType::DO_WHILE_STATEMENT:
			AnalyzeDoWhileStatement( statement, scope );
			break;
		case StatementType::WHILE_STATEMENT:
			AnalyzeWhileStatement( statement, scope );
			break;
		case StatementType::LOOP_STATEMENT:
			AnalyzeInfiniteLoopStatement( statement, scope );
			break;
		default:
			assert( 0 && "we should never get to this point in the looping statements" );
		}
		is_parsing_loops = temp;
	}

	void SemaCheck1::AnalyzeForEachStatement( Statement *statement, SCOPE )
	{
		ForEachStatement *for_each_statement = dynamic_cast< ForEachStatement* >( statement );
		Expression *cond_expression = for_each_statement->GetExpression();

		if ( BinaryExpression *bin_expression = dynamic_cast< BinaryExpression* >( cond_expression ) ){
			if ( bin_expression->GetLHSExpression()->GetExpressionType() != ExpressionType::VARIABLE_EXPR ){
				AppendError( L"The left hand side of a foreach looping statement is a variable definition" );
			}
			else {
				Variable *variable = dynamic_cast< Variable* >( bin_expression->GetLHSExpression() );
				for_each_statement->decl = new VariableDeclaration( variable->GetLineNumber(), variable->GetName(), nullptr, false );
			}
			if ( bin_expression->GetToken().GetType() != ScannerTokenType::TOKEN_IN_ID ){
				AppendError( L"foreach looping statement should be separated by an `in` keyword" );
			}
			else
				AnalyzeExpression( bin_expression->GetRHSExpression(), scope );
		}
		else {
			AppendError( L"A binary expression conjoined by an `in` keyword is expected in a foreach looping statement" );
		}
		CompoundStatement *body_statement = dynamic_cast< CompoundStatement* >( for_each_statement->GetStatement() );
		if ( !body_statement ){
			AppendError( L"A ( possibly empty? ) compound statement is expected as the body of a foreach looping statement" );
		}
		else {
			body_statement->GetScope()->SetParentScope( scope );
			AnalyzeScope( body_statement->GetScope() );
		}
	}

	void SemaCheck1::AnalyzeDoWhileStatement( Statement *statement, SCOPE )
	{
		DoWhileStatement *do_while_statement = dynamic_cast< DoWhileStatement* >( statement );
		Scope *do_while_scope = dynamic_cast< CompoundStatement* >( do_while_statement->GetStatement() )->GetScope();
		do_while_scope->SetParentScope( scope );
		AnalyzeScope( do_while_scope );

		AnalyzeExpression( do_while_statement->GetExpression(), scope );
	}

	void SemaCheck1::AnalyzeWhileStatement( Statement *statement, SCOPE )
	{
		WhileStatement *while_statement = dynamic_cast< WhileStatement* >( statement );
		AnalyzeExpression( while_statement->GetExpression(), scope );
		Scope* while_scope = dynamic_cast< CompoundStatement* >( while_statement->GetStatement() )->GetScope();
		while_scope->SetParentScope( scope );
		AnalyzeScope( while_scope );
	}

	void SemaCheck1::AnalyzeInfiniteLoopStatement( Statement *statement, SCOPE )
	{
		LoopStatement* infinite_loop_stmt = dynamic_cast< LoopStatement* >( statement );
		infinite_loop_stmt->GetLoopBody()->GetScope()->SetParentScope( scope );
		AnalyzeScope( infinite_loop_stmt->GetLoopBody()->GetScope() );
	}

	void SemaCheck1::AnalyzeShowStatement( Statement* statement, SCOPE )
	{
		DumpStatement *dump_statement = dynamic_cast< DumpStatement* >( statement );
		AnalyzeExpression( dump_statement->GetExpression(), scope );
	}
	/*
	*	If there are duplicates in the parameter list or use of a non-variable as parameters
	*	returns false, otherwise true
	*/
	bool SemaCheck1::CheckParameterDuplicates( ExpressionList *parameters, unsigned int const line_number )
	{
		bool result = true;
		if ( parameters ){
			if ( parameters->Length() > 1 ){
				for ( unsigned int r = 0; r < parameters->Length(); ++r ){
					if ( Variable* var = dynamic_cast< Variable* >( parameters->GetExpressionAt( r ) ) ){
						for ( unsigned int c = r + 1; c < parameters->Length(); ++c ){
							if ( Variable *next_var = dynamic_cast< Variable* >( parameters->GetExpressionAt( c ) ) ){
								if ( var->GetName() == next_var->GetName() ){
									result = false;
									AppendError(L"On line " + IntToString(next_var->GetLineNumber()) + L": "
										L"Duplicate name '" + next_var->GetName() + L"' found in parameter list" );
									continue;
								}
							}
							else {
								result = false;
								AppendError( L"On line " + IntToString( var->GetLineNumber() ) + L": "
									L"Formal parameters must only contain variable names" );
								continue;
							}
						}
					}
					else {
						result = false;
						AppendError( L"On line " + IntToString( line_number ) + L": "
							L"Formal parameters must only contain variable names" );
						continue;
					}
				}
			}
		}
		return result;
	}

	void SemaCheck1::AnalyzeExpression( Expression *expression, SCOPE )
	{
		switch ( expression->GetExpressionType() )
		{
		default:;
		}
	}
}

#undef SCOPE