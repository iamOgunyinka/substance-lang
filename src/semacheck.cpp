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
			case StatementType::VARIABLE_DECL_STMT:
			{
				Declaration *decl = dynamic_cast< Declaration* >( statement );
				if ( !scope->AddDeclaration( decl ) ){
					error_messages.push_back( L"variable '" + decl->GetName() + L"' redeclared here." );
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
			// make sure every function used is declared
			case StatementType::FUNCTION_DECL_STMT:
			{
				// to-do
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

	//	void CompoundStatement::Analyze( SemaCheck & sema )
	//	{
	//		Scope *temp = sema.GetCurrentScope();
	//		sema.SetCurrentScope( scope.get() );
	//		scope->Analyze( sema );
	//		sema.SetCurrentScope( temp );
	//	}
	//
	//	void Declaration::Analyze( SemaCheck & sema )
	//	{
	//		Scope* current_scope = sema.GetCurrentScope();
	//		if ( expression ){
	//			expression->Analyze( sema );
	//		}
	//	}
	//
	//	void DeclarationList::Analyze( SemaCheck & sema )
	//	{
	//		for ( auto & decl : declarations ){
	//			decl->Analyze( sema );
	//		}
	//	}
	//
	//	void ExpressionStatement::Analyze( SemaCheck & sema )
	//	{
	//		expression->Analyze( sema );
	//	}
	//
	//	void LabelledStatement::Analyze( SemaCheck & sema )
	//	{
	//		if ( !sema.IsParsingSelection() ){
	//			sema.AppendError( L"labelled statement only allowed in a switch statement" );
	//		}
	//	}
	//
	//	void CaseStatement::Analyze( SemaCheck & sema )
	//	{
	//		if ( !sema.IsParsingSelection() ){
	//			sema.AppendError( L"case statement only allowed in a switch statement" );
	//		}
	//		case_expression->Analyze( sema );
	//		case_statement->Analyze( sema );
	//	}
	//
	//	void ReturnStatement::Analyze( SemaCheck & sema )
	//	{
	//		if ( expression ){
	//			expression->Analyze( sema );
	//		}
	//	}
	//
	//	void ContinueStatement::Analyze( SemaCheck & sema )
	//	{
	//		if ( !sema.IsParsingIteration() ){
	//			sema.AppendError( L"continue statement outside body of an iteration statement" );
	//		}
	//	}
	//
	//	void BreakStatement::Analyze( SemaCheck & sema )
	//	{
	//		if ( !sema.IsParsingIteration() || !sema.IsParsingSelection() ){
	//			sema.AppendError( L"break statement outside body of an iteration statement" );
	//		}
	//	}
	//
	//	void Dump::Analyze( SemaCheck & sema )
	//	{
	//		expression->Analyze( sema );
	//	}
	//
	//	void SwitchStatement::Analyze( SemaCheck & sema )
	//	{
	//		bool temp = sema.IsParsingSelection();
	//		sema.SetParsingSelection( true );
	//
	//		conditional_expression->Analyze( sema );
	//		switch_statement->Analyze( sema );
	//
	//		sema.SetParsingSelection( temp );
	//	}
	//
	//	void DoWhileStatement::Analyze( SemaCheck & sema )
	//	{
	//		bool temp = sema.IsParsingIteration();
	//		sema.SetParsingIteration( true );
	//
	//		do_while_body->Analyze( sema );
	//		logical_expression->Analyze( sema );
	//
	//		sema.SetParsingIteration( temp );
	//	}
	//
	//	void WhileStatement::Analyze( SemaCheck & sema )
	//	{
	//		bool temp = sema.IsParsingIteration();
	//		sema.SetParsingIteration( true );
	//
	//		logical_expression->Analyze( sema );
	//		while_body->Analyze( sema );
	//
	//		sema.SetParsingIteration( temp );
	//	}
	//
	//	void ForEachStatement::Analyze( SemaCheck & sema )
	//	{
	//		bool temp = sema.IsParsingIteration();
	//		sema.SetParsingIteration( true );
	//
	//		expression->Analyze( sema );
	//		body_statement->Analyze( sema );
	//
	//		sema.SetParsingIteration( temp );
	//	}
	//
	//	void IfStatement::Analyze( SemaCheck & sema )
	//	{
	//		conditional_expression->Analyze( sema );
	//		then_statement->Analyze( sema );
	//		if ( else_statement ){
	//			else_statement->Analyze( sema );
	//		}
	//	}
	//
	//	void Scope::Analyze( SemaCheck & sema )
	//	{
	//		auto fn_arity_err_str = [] ( std::wstring const & func_name, int const first_line, unsigned int const second_line ){
	//			return L"Function '" + func_name + L"' declared on line " + IntToString( first_line ) + L" and " +
	//				IntToString( second_line ) + L" have the same number of parameters.";
	//		};
	//
	//		auto& functions = GetFunctions();
	//
	//		std::vector<std::wstring> function_names{};
	//		std::copy_if( node_names.cbegin(), node_names.cend(), std::begin( function_names ),
	//			[] ( std::pair<std::wstring, DeclarationType> const & name )
	//		{
	//			return name.second == DeclarationType::FUNCTION_DCLR;
	//		} );
	//
	//		for ( auto &func_name : function_names ){
	//			auto iter_range = functions.equal_range( func_name );
	//			for ( auto beg = iter_range.first, end = iter_range.second; beg != end; ++beg ){
	//
	//				// perform first check, number of arguments
	//				unsigned int const arity_size = beg->second->GetParameters()->Length();
	//				for ( auto new_beg = beg, new_end = end; new_beg != new_end; ++new_beg ){
	//					if ( arity_size == new_beg->second->GetParameters()->Length() && new_beg != beg ){
	//						sema.AppendError( fn_arity_err_str( func_name, new_beg->second->GetLineNumber(),
	//							beg->second->GetLineNumber() ) );
	//					}
	//				}
	//				// perform second check
	//				beg->second->Analyze( sema );
	//			}
	//		}
	//
	//		auto& global_statements = GetStatements();
	//		for ( auto & statement : global_statements ){
	//			statement->Analyze( sema );
	//		}
	//	}
	//
	//	void FunctionCall::Analyze( SemaCheck & sema )
	//	{
	//		Scope* current_scope = sema.GetCurrentScope();
	//
	//	}
}
