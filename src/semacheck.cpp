#include "semacheck.h"
#include "tree.h"

/* Copyright (c) 2017 Joshua Ogunyinka */
namespace compiler
{
	SemaCheck1::SemaCheck1() : is_parsing_loops( false ), is_parsing_function( false ){
	}

	bool SemaCheck1::Visit( ParsedProgram* parsed_program )
	{
		AnalyzeScope( parsed_program->GetGlobalScope() );
		return error_messages.size() ? false : true;
	}

	void SemaCheck1::AnalyzeScope( Scope* scope )
	{
		Scope::list_of_ptrs<Statement> &statement_list = scope->GetStatements();
		for ( Statement* statement : statement_list ){
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
			//case StatementType::
			case StatementType::EXPR_STATEMENT:
			{
				Expression *expr = dynamic_cast< ExpressionStatement* >( statement )->GetExpression();
				switch ( expr->GetExpressionType() )
				{
				case ExpressionType::ASSIGNMENT_EXPR:
				{
					AssignmentExpression *assign_expr = dynamic_cast< AssignmentExpression* >( expr );
					auto *lhs_expr = assign_expr->GetLHSExpression();
					if ( lhs_expr->GetExpressionType() == ExpressionType::VARIABLE_EXPR ){
						std::wstring const variable_name = dynamic_cast< Variable* >( lhs_expr )->GetName();
						if ( !scope->FindDeclaration( variable_name ) ){
							auto decl = new VariableDeclaration( assign_expr->GetLineNumber(), variable_name, 
								assign_expr->GetRHSExpression(), false );
							scope->AddDeclaration( decl );
						}
					}
					break;
				}
				default:
					break;
				}
			}
			default: break;
			}
		}
	}

	// to-do
	void SemaCheck1::AnalyzeExpressionStatement( Statement *statement )
	{
		Expression *expression = dynamic_cast< ExpressionStatement* >( statement )->GetExpression();
	}

	void SemaCheck1::AppendError( std::wstring const & error )
	{
		error_messages.push_back( error );
	}

	void SemaCheck1::ReportErrors()
	{
		for ( auto const & error : error_messages ){
			std::wcerr << error << "\n";
		}
	}

	void SemaCheck1::AnalyzeDeclaration( Statement *statement, Scope *scope )
	{
		if ( statement->GetStatementType() == StatementType::VDECL_LIST_STMT )
		{
			DeclarationList* decl_list = dynamic_cast< DeclarationList* >( statement );
			for ( std::pair<std::wstring const, Declaration*>& declaration : decl_list->GetDeclarations() ){
				if ( !scope->AddDeclaration( declaration.second ) ){
					AppendError( L"variable '" + ( declaration.second )->GetName() + L"' has already been declared in this scope." );
				}
			}
		}
		else {
			Declaration *decl = dynamic_cast< Declaration* >( statement );
			StatementType const type = decl->GetStatementType();
			std::wstring const type_name = type == StatementType::FUNCTION_DECL_STMT ? L"function '" :
				type == StatementType::CLASS_DECL_STMT ? L"class '" : L"variable '";
			if ( !scope->AddDeclaration( decl ) ){
				AppendError( type_name + decl->GetName() + L"' redeclared here on line " + IntToString( decl->GetLineNumber() ) );
			}
		}
	} // end SemaCheck1::AnalyzeDeclaration

	void SemaCheck1::AnalyzeJumpStatement( Statement * statement, SCOPE )
	{
		StatementType const type = statement->GetStatementType();
		if ( type == StatementType::RETURN_STATEMENT )
		{
			if ( !is_parsing_function ){
				AppendError( L"A return statement not expected outside of an enclosing function" );
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
}

#undef SCOPE