#include "ir.h"

typedef struct ir_optimizer
{
	ir_builder* builder;
} ir_optimizer;

ir_optimizer create_optimizer(ir_builder *const builder);

void optimizer_emit_module(ir_optimizer *const ir_optimizer);

