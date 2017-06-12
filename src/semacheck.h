#pragma once

#include <vector>

#define SCOPE Scope *scope

namespace compiler {
	// forward declarations
	class Scope;
	class Statement;
	class Expression;
	class ParsedProgram;
	class Declaration;
	class ExpressionList;

	class SemaCheck1
	{
		std::vector<std::wstring>	error_messages;

		bool is_parsing_loops;
		bool is_parsing_function;
		bool is_parsing_class;
	public:
		SemaCheck1();
		~SemaCheck1() = default;
		void ReportErrors();
		bool Visit( ParsedProgram* program );

	private:
		void AnalyzeScope( SCOPE );
		void AnalyzeExpressionStatement( Statement *expression_statement, SCOPE );
		void AnalyzeJumpStatement( Statement *statement, SCOPE );
		
		void AnalyzeLoopingStatements( Statement * statement, SCOPE );
		void AnalyzeForEachStatement( Statement *statement, SCOPE );
		void AnalyzeInfiniteLoopStatement( Statement *statement, SCOPE );
		void AnalyzeDoWhileStatement( Statement *statement, SCOPE );
		void AnalyzeWhileStatement( Statement *statement, SCOPE );
		void AnalyzeShowStatement( Statement *statement, SCOPE );
		void AnalyzeIfStatement( Statement *, SCOPE );
		void AnalyzeSwitchStatement( Statement *statement, SCOPE );
		
		void AnalyzeBlockStatement( Statement *statement, SCOPE );
		void AnalyzeExpression( Expression *expr, SCOPE );
		void AppendError( std::wstring const & );

		void AnalyzeDeclaration( Statement *decl, SCOPE );
		void AnalyzeClassDeclaration( Declaration* decl, SCOPE );
		void AnalyzeFunctionDeclaration( Declaration* decl, SCOPE );
	private:
		bool CheckParameterDuplicates( ExpressionList *parameters, unsigned int const line_number );
	};
}