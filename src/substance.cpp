/***************************************************************************
 * Language starting point
 * 
 * Copyright (c) 2017 Joshua Ogunyinka
 * All rights reserved.
 */

#include <memory>
#include "parser.h"
#include "semacheck.h"

/*
#include "emitter.h"
#include "runtime.h"
*/

int main( int argc, const char* argv [] ) {
	if ( argc == 2 ) {
		
		using compiler::Parser;
		using compiler::ParsedProgram;
		//using compiler::SemaCheck;

		Parser parser( BytesToUnicode( argv[1] ) );
		std::unique_ptr<ParsedProgram> parsed_program{ parser.Parse() };
		
		if ( parsed_program ) {
				/*
			auto sema = SemaCheck{};
			bool passed_sema_check = parsed_program->Visit( sema );
			if ( !passed_sema_check ){
				sema.ReportErrors();
			}
			compiler::Emitter emitter{ std::move( parsed_program ) };
			std::unique_ptr<ExecutableProgram> executable_program{ emitter.Emit() };
			if ( executable_program ) {
				
				runtime::Runtime runtime{ std::move( executable_program ), emitter.GetLastLabelId() };
				runtime.Run();
				return 0;
			}
		*/
			// clean up
			//compiler::Emitter::ClearInstructions();
		}
	}

	return 1;
}
