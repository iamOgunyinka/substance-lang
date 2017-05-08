#include "semacheck.h"
#include "tree.h"

/* Copyright (c) 2017 Joshua Ogunyinka */

namespace compiler
{
	SemaCheck::SemaCheck() : parsingIteration( false ), parsingSelection( false )
	{
	}

	bool SemaCheck::Visit( std::unique_ptr<compiler::ParsedProgram> & parsed_program )
	{
		parsed_program->GetGlobalScope()->Analyze( *this );
		return error_messages.size() ? false : true;
	}

	void SemaCheck::ReportErrors()
	{

	}

	void CompoundStatement::Analyze( SemaCheck & sema )
	{
		scope->Analyze( sema );
	}

	void Declaration::Analyze( SemaCheck & sema )
	{
		Scope* current_scope = sema.GetCurrentScope();
		if ( !current_scope->AddDeclaration( shared_from_this() ) ){
			sema.AppendError( L"Variable '" + this->GetIdentifier() + L"' already declared in this scope" );
			return;
		}
		expression->Analyze( sema );
	}

	void DeclarationList::Analyze( SemaCheck & sema )
	{
		for ( auto & decl : declarations ){
			decl->Analyze( sema );
		}
	}

	void ExpressionStatement::Analyze( SemaCheck & sema )
	{
		expression->Analyze( sema );
	}

	void LabelledStatement::Analyze( SemaCheck & sema )
	{
		if ( !sema.IsParsingSelection() ){
			sema.AppendError( L"labelled statement only allowed in a switch statement" );
		}
	}

	void CaseStatement::Analyze( SemaCheck & sema )
	{
		if ( !sema.IsParsingSelection() ){
			sema.AppendError( L"case statement only allowed in a switch statement" );
		}
		case_expression->Analyze( sema );
		case_statement->Analyze( sema );
	}

	void ReturnStatement::Analyze( SemaCheck & sema )
	{
		if ( expression ){
			expression->Analyze( sema );
		}
	}

	void ContinueStatement::Analyze( SemaCheck & sema )
	{
		if ( !sema.IsParsingIteration() ){
			sema.AppendError( L"continue statement outside body of an iteration statement" );
		}
	}

	void BreakStatement::Analyze( SemaCheck & sema )
	{
		if ( !sema.IsParsingIteration() || !sema.IsParsingSelection() ){
			sema.AppendError( L"break statement outside body of an iteration statement" );
		}
	}

	void Dump::Analyze( SemaCheck & sema )
	{
		expression->Analyze( sema );
	}

	void SwitchStatement::Analyze( SemaCheck & sema )
	{
		bool temp = sema.IsParsingSelection();
		sema.SetParsingSelection( true );

		conditional_expression->Analyze( sema );
		switch_statement->Analyze( sema );

		sema.SetParsingSelection( temp );
	}

	void DoWhileStatement::Analyze( SemaCheck & sema )
	{
		bool temp = sema.IsParsingIteration();
		sema.SetParsingIteration( true );

		do_while_body->Analyze( sema );
		logical_expression->Analyze( sema );

		sema.SetParsingIteration( temp );
	}

	void WhileStatement::Analyze( SemaCheck & sema )
	{
		bool temp = sema.IsParsingIteration();
		sema.SetParsingIteration( true );

		logical_expression->Analyze( sema );
		while_body->Analyze( sema );

		sema.SetParsingIteration( temp );
	}

	void ForEachStatement::Analyze( SemaCheck & sema )
	{
		bool temp = sema.IsParsingIteration();
		sema.SetParsingIteration( true );

		expression->Analyze( sema );
		body_statement->Analyze( sema );

		sema.SetParsingIteration( temp );
	}

	void IfStatement::Analyze( SemaCheck & sema )
	{
		conditional_expression->Analyze( sema );
		then_statement->Analyze( sema );
		if ( else_statement ){
			else_statement->Analyze( sema );
		}
	}

	void Scope::Analyze( SemaCheck & sema )
	{
		auto& classes = GetClasses();
		for ( auto &klass : classes ){
			klass.second->Analyze( sema );
		}

		auto fn_arity_err_str = [] ( std::wstring const & func_name, int const first_line, unsigned int const second_line ){
			return L"Function '" + func_name + L"' declared on line " + IntToString( first_line ) + L" and " +
				IntToString( second_line ) + L" have the same number of parameters.";
		};

		auto& functions = GetFunctions();
		for ( auto &func_name : function_names ){
			auto iter_range = functions.equal_range( func_name );
			for ( auto beg = iter_range.first, end = iter_range.second; beg != end; ++beg ){

				// perform first check, number of arguments
				unsigned int const arity_size = beg->second->GetParameters()->Length();
				for ( auto new_beg = beg, new_end = end; new_beg != new_end; ++new_beg ){
					if ( arity_size == new_beg->second->GetParameters()->Length() && new_beg != beg ){
						sema.AppendError( fn_arity_err_str( func_name, new_beg->second->GetLineNumber(),
							beg->second->GetLineNumber() ) );
					}
				}
				// perform second check
				beg->second->Analyze( sema );
			}
		}

		auto& global_statements = GetStatements();
		for ( auto & global_statement : global_statements ){
			global_statement->Analyze( sema );
		}
	}

	void FunctionCall::Analyze( SemaCheck & sema )
	{
		Scope* current_scope = sema.GetCurrentScope();
		
	}
}
