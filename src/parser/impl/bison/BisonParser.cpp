#include "BisonParser.h"
#include "ParserContext.h"

#include "oberon.tab.h"

int yyparse(ParserContext* ctx);

bool BisonParser::parse(ITokenizer& tz) {
    ParserContext ctx;
    ctx.tz = &tz;
    return yyparse(&ctx) == 0;
}
