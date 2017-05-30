#include "semacheck.h"
#include "tree.h"

/* Copyright (c) 2017 Joshua Ogunyinka */

namespace compiler
{
	SemaCheck1::SemaCheck1(){
	}

	bool SemaCheck1::Visit( compiler::ParsedProgram* parsed_program )
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
			{
				Declaration *decl = dynamic_cast< Declaration* >( statement );
				StatementType const type = decl->GetStatementType();
				std::wstring const type_name = type == StatementType::FUNCTION_DECL_STMT ? L"function '" :
					type == StatementType::CLASS_DECL_STMT ? L"class '" : L"variable '";
				if ( !scope->AddDeclaration( decl ) ){
					error_messages.push_back( type_name + decl->GetName() + L"' redeclared here." );
				}
				break;
			}

			case StatementType::VDECL_LIST_STMT:
			{
				DeclarationList* decl_list = dynamic_cast< DeclarationList* >( statement );
				for ( Declaration* declaration : decl_list->GetDeclarations() ){
					if ( !scope->AddDeclaration( declaration ) ){
						error_messages.push_back( L"variable '" + declaration->GetName() +
							L"' has already been declared in this scope." );
					}
				}
				break;
			}
			// to-do
			// make sure the LHS of the assignment op is at least declared and if not so, declare a new variable in that scope
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
							auto decl = new VariableDeclaration( assign_expr->GetFileName(), assign_expr->GetLineNumber(),
								variable_name, assign_expr->GetRHSExpression(), false );
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

	//to-do
	void SemaCheck1::ReportErrors()
	{
		for ( auto const & error : error_messages ){
			std::wcerr << error << "\n";
		}
	}

	SemaCheck2::SemaCheck2() : current_scope( nullptr ), parsingIteration( false ), parsingSelection( false ){
	}
	//to-do
	void SemaCheck2::ReportErrors(){

	}

	bool SemaCheck2::Visit( ParsedProgram* )
	{
		return true;
	}
}
