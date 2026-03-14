#include "BisonParser.h"
#include "ParserContext.h"

#include "oberon.tab.h"

int yyparse(ParserContext *ctx);

ModulePtr BisonParser::parse(ITokenizerPtr tz) {
    ParserContext ctx;
    ctx.tz = tz;
    yyparse(&ctx);
    return std::move(ctx.module);
    ;
}
