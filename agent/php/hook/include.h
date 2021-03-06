#pragma once

#include "openrasp_hook.h"

#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4)
#define OPENRASP_OP1_TYPE(n) ((n)->op1.op_type)
#define OPENRASP_OP2_TYPE(n) ((n)->op2.op_type)
#define OPENRASP_OP1_VAR(n) ((n)->op1.u.var)
#define OPENRASP_OP2_VAR(n) ((n)->op2.u.var)
#define OPENRASP_OP1_CONSTANT_PTR(n) (&(n)->op1.u.constant)
#define OPENRASP_OP2_CONSTANT_PTR(n) (&(n)->op2.u.constant)
#define OPENRASP_INCLUDE_OR_EVAL_TYPE(n) (Z_LVAL(n->op2.u.constant))
#else
#define OPENRASP_OP1_TYPE(n) ((n)->op1_type)
#define OPENRASP_OP2_TYPE(n) ((n)->op2_type)
#define OPENRASP_OP1_VAR(n) ((n)->op1.var)
#define OPENRASP_OP2_VAR(n) ((n)->op2.var)
#define OPENRASP_OP1_CONSTANT_PTR(n) ((n)->op1.zv)
#define OPENRASP_OP2_CONSTANT_PTR(n) ((n)->op2.zv)
#define OPENRASP_INCLUDE_OR_EVAL_TYPE(n) (n->extended_value)
#endif

#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 5)
#define OPENRASP_T(offset) (*(temp_variable *)((char *)execute_data->Ts + offset))
#define OPENRASP_CV(i) (execute_data->CVs[i])
#define OPENRASP_CV_OF(i) (EG(current_execute_data)->CVs[i])
#else
#define OPENRASP_T(offset) (*EX_TMP_VAR(execute_data, offset))
#define OPENRASP_CV(i) (*EX_CV_NUM(execute_data, i))
#define OPENRASP_CV_OF(i) (*EX_CV_NUM(EG(current_execute_data), i))
#endif

int include_handler(ZEND_OPCODE_HANDLER_ARGS);
int eval_handler(ZEND_OPCODE_HANDLER_ARGS);
int include_or_eval_handler(ZEND_OPCODE_HANDLER_ARGS)
{
    if (OPENRASP_INCLUDE_OR_EVAL_TYPE(execute_data->opline) == ZEND_EVAL)
    {
        return eval_handler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
    }
    else
    {
        return include_handler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
    }
}
int eval_handler(ZEND_OPCODE_HANDLER_ARGS)
{
    zend_op *opline = execute_data->opline;
    if (!openrasp_check_type_ignored(ZEND_STRL("webshell") TSRMLS_CC) &&
        OPENRASP_OP1_TYPE(opline) == IS_VAR &&
        openrasp_zval_in_request(OPENRASP_T(OPENRASP_OP1_VAR(opline)).var.ptr TSRMLS_CC))
    {
        zval *attack_params;
        MAKE_STD_ZVAL(attack_params);
        ZVAL_STRING(attack_params, "", 1);
        zval *plugin_message = NULL;
        MAKE_STD_ZVAL(plugin_message);
        ZVAL_STRING(plugin_message, _("China Chopper WebShell"), 1);
        openrasp_buildin_php_risk_handle(1, "webshell", 100, attack_params, plugin_message TSRMLS_CC);
    }
    return ZEND_USER_OPCODE_DISPATCH;
}
int include_handler(ZEND_OPCODE_HANDLER_ARGS)
{
    if (openrasp_check_type_ignored(ZEND_STRL("include") TSRMLS_CC))
    {
        return ZEND_USER_OPCODE_DISPATCH;
    }
    zend_op *opline = execute_data->opline;
    zval *op1 = nullptr;
    switch (OPENRASP_OP1_TYPE(opline))
    {
    case IS_TMP_VAR:
    {
        op1 = &OPENRASP_T(OPENRASP_OP1_VAR(opline)).tmp_var;
        break;
    }
    case IS_VAR:
    {
        // whether the parameter is the user input data
        eval_handler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
        op1 = OPENRASP_T(OPENRASP_OP1_VAR(opline)).var.ptr;
        break;
    }
    case IS_CV:
    {
        zval **t = OPENRASP_CV(OPENRASP_OP1_VAR(opline));
        if (t && *t)
        {
            op1 = *t;
        }
        else if (EG(active_symbol_table))
        {
            zend_compiled_variable *cv = &EG(active_op_array)->vars[OPENRASP_OP1_VAR(opline)];
            if (zend_hash_quick_find(EG(active_symbol_table), cv->name, cv->name_len + 1, cv->hash_value, (void **)&t) == SUCCESS)
            {
                op1 = *t;
            }
        }
        break;
    }
    case IS_CONST:
    {
        op1 = OPENRASP_OP1_CONSTANT_PTR(opline);
        break;
    }
    default:
    {
        return ZEND_USER_OPCODE_DISPATCH;
    }
    }
    zval *path;
    MAKE_STD_ZVAL(path);
    MAKE_COPY_ZVAL(&op1, path);
    convert_to_string(path);
    char *real_path = php_resolve_path(Z_STRVAL_P(path),
                                       Z_STRLEN_P(path),
                                       PG(include_path) TSRMLS_CC);
    zval *params;
    MAKE_STD_ZVAL(params);
    array_init(params);
    add_assoc_zval(params, "path", path);
    add_assoc_zval(params, "url", path);
    Z_ADDREF_P(path);
    if (real_path)
    {
        add_assoc_string(params, "realpath", real_path, 0);
    }
    else
    {
        add_assoc_string(params, "realpath", "", 1);
    }
    char *function = nullptr;
    switch (OPENRASP_INCLUDE_OR_EVAL_TYPE(execute_data->opline))
    {
    case ZEND_INCLUDE:
        function = "include";
        break;
    case ZEND_INCLUDE_ONCE:
        function = "include_once";
        break;
    case ZEND_REQUIRE:
        function = "require";
        break;
    case ZEND_REQUIRE_ONCE:
        function = "require_once";
        break;
    default:
        function = "";
        break;
    }
    add_assoc_string(params, "function", function, 1);
    check("include", params TSRMLS_CC);

    return ZEND_USER_OPCODE_DISPATCH;
}