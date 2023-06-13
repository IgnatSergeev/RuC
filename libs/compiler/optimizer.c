#include "optimizer.h"

ir_optimizer create_optimizer(ir_builder *const builder)
{
	ir_optimizer optimizer;

	optimizer.builder = builder;

	return optimizer;
}

void optimizer_emit_module(ir_optimizer *const ir_optimizer)
{
	ir_module *module = ir_optimizer->builder->module;
	for (size_t i = 0; i < optimizer_get_extern_count(module); i++)
	{
		ir_extern extern_ = ir_module_index_extern(module, i);
		ir_gen_extern(&ctx, &extern_);
	}
	for (size_t i = 0; i < ir_module_get_global_count(module); i++)
	{
		ir_global global = ir_module_index_global(module, i);
		ir_gen_global(&ctx, &global);
	}
	for (size_t i = 0; i < ir_module_get_function_count(module); i++)
	{
		ir_function function = ir_module_index_function(module, i);
		ir_gen_function(&ctx, &function);
	}
}