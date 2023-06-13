#include "optimizer.h"

ir_optimizer create_optimizer(ir_builder *const builder)
{
	ir_optimizer optimizer;

	optimizer.builder = builder;

	return optimizer;
}

typedef node ir_extern;

static size_t ir_module_get_extern_count(const ir_module *const module)
{
	return node_get_amount(&module->externs_root);
}
static ir_extern ir_module_index_extern(const ir_module *const module, size_t idx)
{
	return node_get_child(&module->externs_root, idx);
}

typedef node ir_global;

static size_t ir_module_get_global_count(const ir_module *const module)
{
	return node_get_amount(&module->globals_root);
}

static ir_global ir_module_index_global(const ir_module *const module, size_t idx)
{
	return node_get_child(&module->globals_root, idx);
}

typedef node ir_function;

static size_t ir_module_get_function_count(const ir_module *const module)
{
	return node_get_amount(&module->functions_root);
}
static ir_function ir_module_index_function(const ir_module *const module, const size_t idx)
{
	return node_get_child(&module->functions_root, idx);
}

typedef node ir_value;

static size_t ir_module_get_value_count(const ir_module *const module)
{
	return node_get_amount(&module->values_root);
}
static ir_value ir_module_index_value(const ir_module *const module, const size_t idx)
{
	return node_get_child(&module->values_root, idx);
}
static ir_value ir_module_get_value(const ir_module *const module, const item_t id)
{
	return ir_value_load(&module->values, id);
}

typedef enum ir_label_kind
{
	IR_LABEL_KIND_BEGIN,
	IR_LABEL_KIND_THEN,
	IR_LABEL_KIND_ELSE,
	IR_LABEL_KIND_END,
	IR_LABEL_KIND_BEGIN_CYCLE,
	IR_LABEL_KIND_NEXT,
	IR_LABEL_KIND_AND,
	IR_LABEL_KIND_OR,
	IR_LABEL_KIND_CASE
} ir_label_kind;

typedef node ir_label;

const item_t IR_LABEL_NULL = -1;

static ir_label create_ir_label(const node *const nd, ir_label_kind kind)
{
	static item_t id = 0;

	ir_label label_ = node_add_child(nd, kind);
	node_add_arg(&label_, ++id);
	return label_;
}

static ir_label_kind ir_label_get_kind(const ir_label *const label)
{
	return node_get_type(label);
}
static item_t ir_label_get_id(const ir_label *const label)
{
	return node_get_arg(label, 0);
}
static ir_label ir_label_load(const vector *const tree, const item_t id)
{
	return node_load(tree, id);
}
static size_t ir_module_get_label_count(const ir_module *const module)
{
	return node_get_amount(&module->labels_root);
}
static ir_label ir_module_index_label(const ir_module *const module, const size_t idx)
{
	return node_get_child(&module->labels_root, idx);
}
static ir_label ir_module_get_label(const ir_module *const module, const item_t id)
{
	return ir_label_load(&module->labels, id);
}
static item_t ir_idents_get(const ir_module *const module, const item_t id)
{
	return hash_get(&module->idents, id, 0);
}

void optimizer_emit_module(ir_optimizer *const optimizer)
{
	ir_module *module = optimizer->builder->module;
	for (size_t i = 0; i < ir_module_get_extern_count(module); i++)
	{
		ir_extern extern_ = ir_(module, i);
		ir_gen_extern(&ctx, &extern_);
	}
	for (size_t i = 0; i < optimizer_get_global_count(module); i++)
	{
		ir_global global = ir_module_index_global(module, i);
		ir_gen_global(&ctx, &global);
	}
	for (size_t i = 0; i < optimizer_get_function_count(module); i++)
	{
		ir_function function = ir_module_index_function(module, i);
		ir_gen_function(&ctx, &function);
	}
}