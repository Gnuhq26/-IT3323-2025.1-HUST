/*
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */
#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "scanner.h"
#include "parser.h"
#include "semantics.h"
#include "error.h"
#include "debug.h"
#include "codegen.h"

Token *currentToken;
Token *lookAhead;

extern Type *intType;
extern Type *charType;
extern SymTab *symtab;

void scan(void)
{
  Token *tmp = currentToken;
  currentToken = lookAhead;
  lookAhead = getValidToken();
  free(tmp);
}

void eat(TokenType tokenType)
{
  if (lookAhead->tokenType == tokenType)
  {
    //    printToken(lookAhead);
    scan();
  }
  else
    missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);
}

void compileProgram(void)
{
  Object *program;

  eat(KW_PROGRAM);
  eat(TK_IDENT);

  program = createProgramObject(currentToken->string);
  program->progAttrs->codeAddress = getCurrentCodeAddress();
  enterBlock(program->progAttrs->scope);

  eat(SB_SEMICOLON);

  compileBlock();
  eat(SB_PERIOD);

  // Halt the program
  genHL();

  exitBlock();
}

void compileConstDecls(void)
{
  Object *constObj;
  ConstantValue *constValue;

  if (lookAhead->tokenType == KW_CONST)
  {
    eat(KW_CONST);
    do
    {
      eat(TK_IDENT);
      checkFreshIdent(currentToken->string);
      constObj = createConstantObject(currentToken->string);
      declareObject(constObj);

      eat(SB_EQ);
      constValue = compileConstant();
      constObj->constAttrs->value = constValue;

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);
  }
}

void compileTypeDecls(void)
{
  Object *typeObj;
  Type *actualType;

  if (lookAhead->tokenType == KW_TYPE)
  {
    eat(KW_TYPE);
    do
    {
      eat(TK_IDENT);

      checkFreshIdent(currentToken->string);
      typeObj = createTypeObject(currentToken->string);
      declareObject(typeObj);

      eat(SB_EQ);
      actualType = compileType();
      typeObj->typeAttrs->actualType = actualType;

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);
  }
}

void compileVarDecls(void)
{
  Object *varObj;
  Type *varType;

  if (lookAhead->tokenType == KW_VAR)
  {
    eat(KW_VAR);
    do
    {
      eat(TK_IDENT);
      checkFreshIdent(currentToken->string);
      varObj = createVariableObject(currentToken->string);
      eat(SB_COLON);
      varType = compileType();
      varObj->varAttrs->type = varType;
      declareObject(varObj);
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);
  }
}

void compileBlock(void)
{
  Instruction *jmp;
  // Jump to the body of the block
  jmp = genJ(DC_VALUE);

  compileConstDecls();
  compileTypeDecls();
  compileVarDecls();
  compileSubDecls();

  // Update the jmp label
  updateJ(jmp, getCurrentCodeAddress());
  // Skip the stack frame
  genINT(symtab->currentScope->frameSize);

  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileSubDecls(void)
{
  while ((lookAhead->tokenType == KW_FUNCTION) || (lookAhead->tokenType == KW_PROCEDURE))
  {
    if (lookAhead->tokenType == KW_FUNCTION)
      compileFuncDecl();
    else
      compileProcDecl();
  }
}

void compileFuncDecl(void)
{
  Object *funcObj;
  Type *returnType;

  eat(KW_FUNCTION);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  funcObj = createFunctionObject(currentToken->string);
  funcObj->funcAttrs->codeAddress = getCurrentCodeAddress();
  declareObject(funcObj);

  enterBlock(funcObj->funcAttrs->scope);

  compileParams();

  eat(SB_COLON);
  returnType = compileBasicType();
  funcObj->funcAttrs->returnType = returnType;

  eat(SB_SEMICOLON);

  compileBlock();

  // Exit function
  genEF();

  eat(SB_SEMICOLON);

  exitBlock();
}

void compileProcDecl(void)
{
  Object *procObj;

  eat(KW_PROCEDURE);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  procObj = createProcedureObject(currentToken->string);
  procObj->procAttrs->codeAddress = getCurrentCodeAddress();
  declareObject(procObj);

  enterBlock(procObj->procAttrs->scope);

  compileParams();

  eat(SB_SEMICOLON);
  compileBlock();

  // Exit procedure
  genEP();

  eat(SB_SEMICOLON);

  exitBlock();
}

ConstantValue *compileUnsignedConstant(void)
{
  ConstantValue *constValue;
  Object *obj;

  switch (lookAhead->tokenType)
  {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);

    obj = checkDeclaredConstant(currentToken->string);
    constValue = duplicateConstantValue(obj->constAttrs->value);

    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

ConstantValue *compileConstant(void)
{
  ConstantValue *constValue;

  switch (lookAhead->tokenType)
  {
  case SB_PLUS:
    eat(SB_PLUS);
    constValue = compileConstant2();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    constValue = compileConstant2();
    constValue->intValue = -constValue->intValue;
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:
    constValue = compileConstant2();
    break;
  }
  return constValue;
}

ConstantValue *compileConstant2(void)
{
  ConstantValue *constValue;
  Object *obj;

  switch (lookAhead->tokenType)
  {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredConstant(currentToken->string);
    if (obj->constAttrs->value->type == TP_INT)
      constValue = duplicateConstantValue(obj->constAttrs->value);
    else
      error(ERR_UNDECLARED_INT_CONSTANT, currentToken->lineNo, currentToken->colNo);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

Type *compileType(void)
{
  Type *type;
  Type *elementType;
  int arraySize;
  Object *obj;

  switch (lookAhead->tokenType)
  {
  case KW_INTEGER:
    eat(KW_INTEGER);
    type = makeIntType();
    break;
  case KW_CHAR:
    eat(KW_CHAR);
    type = makeCharType();
    break;
  case KW_ARRAY:
    eat(KW_ARRAY);
    eat(SB_LSEL);
    eat(TK_NUMBER);

    arraySize = currentToken->value;

    eat(SB_RSEL);
    eat(KW_OF);
    elementType = compileType();
    type = makeArrayType(arraySize, elementType);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredType(currentToken->string);
    type = duplicateType(obj->typeAttrs->actualType);
    break;
  default:
    error(ERR_INVALID_TYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

Type *compileBasicType(void)
{
  Type *type;

  switch (lookAhead->tokenType)
  {
  case KW_INTEGER:
    eat(KW_INTEGER);
    type = makeIntType();
    break;
  case KW_CHAR:
    eat(KW_CHAR);
    type = makeCharType();
    break;
  default:
    error(ERR_INVALID_BASICTYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

void compileParams(void)
{
  if (lookAhead->tokenType == SB_LPAR)
  {
    eat(SB_LPAR);
    compileParam();
    while (lookAhead->tokenType == SB_SEMICOLON)
    {
      eat(SB_SEMICOLON);
      compileParam();
    }
    eat(SB_RPAR);
  }
}

void compileParam(void)
{
  Object *param;
  Type *type;
  enum ParamKind paramKind = PARAM_VALUE;

  if (lookAhead->tokenType == KW_VAR)
  {
    paramKind = PARAM_REFERENCE;
    eat(KW_VAR);
  }

  eat(TK_IDENT);
  checkFreshIdent(currentToken->string);
  param = createParameterObject(currentToken->string, paramKind);
  eat(SB_COLON);
  type = compileBasicType();
  param->paramAttrs->type = type;
  declareObject(param);
}

void compileStatements(void)
{
  compileStatement();
  while (lookAhead->tokenType == SB_SEMICOLON)
  {
    eat(SB_SEMICOLON);
    compileStatement();
  }
}

void compileStatement(void)
{
  switch (lookAhead->tokenType)
  {
  case TK_IDENT:
    compileAssignSt();
    break;
  case KW_CALL:
    compileCallSt();
    break;
  case KW_BEGIN:
    compileGroupSt();
    break;
  case KW_IF:
    compileIfSt();
    break;
  case KW_WHILE:
    compileWhileSt();
    break;
  case KW_FOR:
    compileForSt();
    break;
    // EmptySt needs to check FOLLOW tokens
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
    break;
    // Error occurs
  default:
    error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

Type *compileLValue(void)
{
  Object *var;
  Type *varType;

  eat(TK_IDENT);

  var = checkDeclaredLValueIdent(currentToken->string);

  switch (var->kind)
  {
  case OBJ_VARIABLE:
    genVariableAddress(var);
    if (var->varAttrs->type->typeClass == TP_ARRAY)
    {
      // compute the element address
      varType = compileIndexes(var->varAttrs->type);
    }
    else
      varType = var->varAttrs->type;
    break;
  case OBJ_PARAMETER:
  {
    Scope *currentScope = symtab->currentScope;
    Scope *paramScope = PARAMETER_SCOPE(var);
    int level = 0;

    while (currentScope != NULL && currentScope != paramScope)
    {
      level++;
      currentScope = currentScope->outer;
    }

    if (var->paramAttrs->kind == PARAM_REFERENCE)
      genLV(level, PARAMETER_OFFSET(var));
    else
      genLA(level, PARAMETER_OFFSET(var));

    if (var->paramAttrs->type->typeClass == TP_ARRAY)
    {
      varType = compileIndexes(var->paramAttrs->type);
    }
    else
    {
      varType = var->paramAttrs->type;
    }
  }
  break;
  case OBJ_FUNCTION:
    if (var != symtab->currentScope->owner)
    {
      error(ERR_INVALID_LVALUE, currentToken->lineNo, currentToken->colNo);
    }
    genLA(0, RETURN_VALUE_OFFSET);
    varType = var->funcAttrs->returnType;
    break;
  default:
    error(ERR_INVALID_LVALUE, currentToken->lineNo, currentToken->colNo);
  }

  return varType;
}

void compileAssignSt(void)
{
  Type *varType;
  Type *expType;

  varType = compileLValue();

  eat(SB_ASSIGN);
  expType = compileExpression();
  checkTypeEquality(varType, expType);

  // Store the value at the address
  genST();
}

void compileCallSt(void)
{
  Object *proc;

  eat(KW_CALL);
  eat(TK_IDENT);

  proc = checkDeclaredProcedure(currentToken->string);

  if (isPredefinedProcedure(proc))
  {
    compileArguments(proc->procAttrs->paramList);
    genPredefinedProcedureCall(proc);
  }
  else
  {
    Scope *currentScope = symtab->currentScope;
    Scope *procScope = PROCEDURE_SCOPE(proc);
    int level = 0;

    while (currentScope != NULL && currentScope != procScope)
    {
      level++;
      currentScope = currentScope->outer;
    }

    compileArguments(proc->procAttrs->paramList);
    genCALL(level, proc->procAttrs->codeAddress);
  }
}

void compileGroupSt(void)
{
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileIfSt(void)
{
  Instruction *fjLabel;
  Instruction *jLabel;

  eat(KW_IF);
  compileCondition();

  // False jump: if condition is false, jump to else or end
  fjLabel = genFJ(DC_VALUE);

  eat(KW_THEN);
  compileStatement();

  if (lookAhead->tokenType == KW_ELSE)
  {
    // Jump to end of if-else
    jLabel = genJ(DC_VALUE);

    // Update false jump to point to else clause
    updateFJ(fjLabel, getCurrentCodeAddress());

    eat(KW_ELSE);
    compileStatement();

    // Update jump to point to end
    updateJ(jLabel, getCurrentCodeAddress());
  }
  else
  {
    // No else clause, update false jump to point to end
    updateFJ(fjLabel, getCurrentCodeAddress());
  }
}

void compileWhileSt(void)
{
  CodeAddress beginLoop;
  Instruction *fjLabel;

  eat(KW_WHILE);

  beginLoop = getCurrentCodeAddress();
  compileCondition();

  // If condition is false, jump to end
  fjLabel = genFJ(DC_VALUE);

  eat(KW_DO);
  compileStatement();

  // Jump back to condition
  genJ(beginLoop);

  // Update false jump to point to end
  updateFJ(fjLabel, getCurrentCodeAddress());
}

void compileForSt(void)
{
  Type *varType;
  Type *type;
  CodeAddress beginLoop;
  Instruction *fjLabel;
  Object *controlVar;
  Token *varToken;

  eat(KW_FOR);
  eat(TK_IDENT);

  // Save the control variable
  varToken = currentToken;
  controlVar = checkDeclaredLValueIdent(currentToken->string);

  // Generate code to store initial value
  if (controlVar->kind == OBJ_VARIABLE)
  {
    genVariableAddress(controlVar);
  }
  else if (controlVar->kind == OBJ_PARAMETER)
  {
    Scope *currentScope = symtab->currentScope;
    Scope *paramScope = PARAMETER_SCOPE(controlVar);
    int level = 0;

    while (currentScope != NULL && currentScope != paramScope)
    {
      level++;
      currentScope = currentScope->outer;
    }

    if (controlVar->paramAttrs->kind == PARAM_REFERENCE)
      genLV(level, PARAMETER_OFFSET(controlVar));
    else
      genLA(level, PARAMETER_OFFSET(controlVar));
  }

  varType = (controlVar->kind == OBJ_VARIABLE) ? controlVar->varAttrs->type : controlVar->paramAttrs->type;

  eat(SB_ASSIGN);

  type = compileExpression();
  checkTypeEquality(varType, type);

  // Store initial value
  genST();

  eat(KW_TO);

  // Begin of loop: load control variable value
  beginLoop = getCurrentCodeAddress();
  if (controlVar->kind == OBJ_VARIABLE)
  {
    genVariableValue(controlVar);
  }
  else if (controlVar->kind == OBJ_PARAMETER)
  {
    Scope *currentScope = symtab->currentScope;
    Scope *paramScope = PARAMETER_SCOPE(controlVar);
    int level = 0;

    while (currentScope != NULL && currentScope != paramScope)
    {
      level++;
      currentScope = currentScope->outer;
    }

    genLV(level, PARAMETER_OFFSET(controlVar));
    genLI();
  }

  type = compileExpression();
  checkTypeEquality(varType, type);

  // Compare: if control variable > limit, exit loop
  genLE();
  fjLabel = genFJ(DC_VALUE);

  eat(KW_DO);
  compileStatement();

  // Increment control variable
  if (controlVar->kind == OBJ_VARIABLE)
  {
    genVariableAddress(controlVar);
    genVariableValue(controlVar);
  }
  else if (controlVar->kind == OBJ_PARAMETER)
  {
    Scope *currentScope = symtab->currentScope;
    Scope *paramScope = PARAMETER_SCOPE(controlVar);
    int level = 0;

    while (currentScope != NULL && currentScope != paramScope)
    {
      level++;
      currentScope = currentScope->outer;
    }

    if (controlVar->paramAttrs->kind == PARAM_REFERENCE)
      genLV(level, PARAMETER_OFFSET(controlVar));
    else
      genLA(level, PARAMETER_OFFSET(controlVar));

    genLV(level, PARAMETER_OFFSET(controlVar));
    genLI();
  }

  genLC(1);
  genAD();
  genST();

  // Jump back to condition
  genJ(beginLoop);

  // Update false jump to point to end
  updateFJ(fjLabel, getCurrentCodeAddress());
}

void compileArgument(Object *param)
{
  Type *type;

  if (param->paramAttrs->kind == PARAM_VALUE)
  {
    type = compileExpression();
    checkTypeEquality(type, param->paramAttrs->type);
  }
  else
  {
    type = compileLValue();
    checkTypeEquality(type, param->paramAttrs->type);
  }
}

void compileArguments(ObjectNode *paramList)
{
  ObjectNode *node = paramList;

  switch (lookAhead->tokenType)
  {
  case SB_LPAR:
    eat(SB_LPAR);
    if (node == NULL)
      error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
    compileArgument(node->object);
    node = node->next;

    while (lookAhead->tokenType == SB_COMMA)
    {
      eat(SB_COMMA);
      if (node == NULL)
        error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
      compileArgument(node->object);
      node = node->next;
    }

    if (node != NULL)
      error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);

    eat(SB_RPAR);
    break;
    // Check FOLLOW set
  case SB_TIMES:
  case SB_SLASH:
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    if (node != NULL)
      error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
    break;

  default:
    error(ERR_INVALID_ARGUMENTS, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileCondition(void)
{
  Type *type1;
  Type *type2;
  TokenType op;

  type1 = compileExpression();
  checkBasicType(type1);

  op = lookAhead->tokenType;
  switch (op)
  {
  case SB_EQ:
    eat(SB_EQ);
    break;
  case SB_NEQ:
    eat(SB_NEQ);
    break;
  case SB_LE:
    eat(SB_LE);
    break;
  case SB_LT:
    eat(SB_LT);
    break;
  case SB_GE:
    eat(SB_GE);
    break;
  case SB_GT:
    eat(SB_GT);
    break;
  default:
    error(ERR_INVALID_COMPARATOR, lookAhead->lineNo, lookAhead->colNo);
  }

  type2 = compileExpression();
  checkTypeEquality(type1, type2);

  // Generate comparison instruction
  switch (op)
  {
  case SB_EQ:
    genEQ();
    break;
  case SB_NEQ:
    genNE();
    break;
  case SB_LE:
    genLE();
    break;
  case SB_LT:
    genLT();
    break;
  case SB_GE:
    genGE();
    break;
  case SB_GT:
    genGT();
    break;
  }
}

Type *compileExpression(void)
{
  // TODO: generate code for expression
  Type *type;

  switch (lookAhead->tokenType)
  {
  case SB_PLUS:
    eat(SB_PLUS);
    type = compileExpression2();
    checkIntType(type);
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    type = compileExpression2();
    genNEG();
    checkIntType(type);
    break;
  default:
    type = compileExpression2();
  }
  return type;
}

Type *compileExpression2(void)
{
  Type *type;

  type = compileTerm();
  type = compileExpression3(type);

  return type;
}

Type *compileExpression3(Type *argType1)
{
  // TODO: generate code for expression3
  Type *argType2;
  Type *resultType;

  switch (lookAhead->tokenType)
  {
  case SB_PLUS:
    eat(SB_PLUS);
    checkIntType(argType1);
    argType2 = compileTerm();
    genAD();
    checkIntType(argType2);

    resultType = compileExpression3(argType1);
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    checkIntType(argType1);
    argType2 = compileTerm();
    genSB();
    checkIntType(argType2);

    resultType = compileExpression3(argType1);
    break;
    // check the FOLLOW set
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    resultType = argType1;
    break;
  default:
    error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
  }
  return resultType;
}

Type *compileTerm(void)
{
  Type *type;
  type = compileFactor();
  type = compileTerm2(type);

  return type;
}

Type *compileTerm2(Type *argType1)
{
  // TODO: generate code for term2
  Type *argType2;
  Type *resultType;

  switch (lookAhead->tokenType)
  {
  case SB_TIMES:
    eat(SB_TIMES);
    checkIntType(argType1);
    argType2 = compileFactor();
    checkIntType(argType2);
    genML();

    resultType = compileTerm2(argType1);
    break;
  case SB_SLASH:
    eat(SB_SLASH);
    checkIntType(argType1);
    argType2 = compileFactor();
    checkIntType(argType2);
    genDV();

    resultType = compileTerm2(argType1);
    break;
    // check the FOLLOW set
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    resultType = argType1;
    break;
  default:
    error(ERR_INVALID_TERM, lookAhead->lineNo, lookAhead->colNo);
  }
  return resultType;
}

Type *compileFactor(void)
{
  // TODO: generate code for factor
  Type *type;
  Object *obj;

  switch (lookAhead->tokenType)
  {
  case TK_NUMBER:
    eat(TK_NUMBER);
    genLC(currentToken->value);
    type = intType;
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    genLC(currentToken->string[0]);
    type = charType;
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredIdent(currentToken->string);

    switch (obj->kind)
    {
    case OBJ_CONSTANT:
      if (obj->constAttrs->value->type == TP_INT)
      {
        genLC(obj->constAttrs->value->intValue);
        type = intType;
      }
      else
      {
        genLC(obj->constAttrs->value->charValue);
        type = charType;
      }
      break;
    case OBJ_VARIABLE:
      if (obj->varAttrs->type->typeClass == TP_ARRAY)
      {
        genVariableAddress(obj);
        type = compileIndexes(obj->varAttrs->type);
        genLI();
      }
      else
      {
        genVariableValue(obj);
        type = obj->varAttrs->type;
      }
      break;
    case OBJ_PARAMETER:
    {
      Scope *currentScope = symtab->currentScope;
      Scope *paramScope = PARAMETER_SCOPE(obj);
      int level = 0;

      while (currentScope != NULL && currentScope != paramScope)
      {
        level++;
        currentScope = currentScope->outer;
      }

      if (obj->paramAttrs->type->typeClass == TP_ARRAY)
      {
        if (obj->paramAttrs->kind == PARAM_REFERENCE)
          genLV(level, PARAMETER_OFFSET(obj));
        else
          genLA(level, PARAMETER_OFFSET(obj));
        type = compileIndexes(obj->paramAttrs->type);
        genLI();
      }
      else
      {
        genLV(level, PARAMETER_OFFSET(obj));
        genLI();
        type = obj->paramAttrs->type;
      }
    }
    break;
    case OBJ_FUNCTION:
      if (isPredefinedFunction(obj))
      {
        compileArguments(obj->funcAttrs->paramList);
        genPredefinedFunctionCall(obj);
      }
      else
      {
        Scope *currentScope = symtab->currentScope;
        Scope *funcScope = FUNCTION_SCOPE(obj);
        int level = 0;

        while (currentScope != NULL && currentScope != funcScope)
        {
          level++;
          currentScope = currentScope->outer;
        }

        compileArguments(obj->funcAttrs->paramList);
        genCALL(level, obj->funcAttrs->codeAddress);
      }
      type = obj->funcAttrs->returnType;
      break;
    default:
      error(ERR_INVALID_FACTOR, currentToken->lineNo, currentToken->colNo);
      break;
    }
    break;
  case SB_LPAR:
    eat(SB_LPAR);
    type = compileExpression();
    eat(SB_RPAR);
    break;
  default:
    error(ERR_INVALID_FACTOR, lookAhead->lineNo, lookAhead->colNo);
  }

  return type;
}

Type *compileIndexes(Type *arrayType)
{
  Type *type;

  while (lookAhead->tokenType == SB_LSEL)
  {
    eat(SB_LSEL);
    type = compileExpression();
    checkIntType(type);
    checkArrayType(arrayType);

    // Calculate array element address
    // Address = base + (index - 1) * elementSize
    genLC(1);
    genSB(); // index - 1
    genLC(sizeOfType(arrayType->elementType));
    genML(); // (index - 1) * elementSize
    genAD(); // base + offset

    arrayType = arrayType->elementType;
    eat(SB_RSEL);
  }
  checkBasicType(arrayType);
  return arrayType;
}

int compile(char *fileName)
{
  if (openInputStream(fileName) == IO_ERROR)
    return IO_ERROR;

  currentToken = NULL;
  lookAhead = getValidToken();

  initSymTab();

  compileProgram();

  cleanSymTab();
  free(currentToken);
  free(lookAhead);
  closeInputStream();
  return IO_SUCCESS;
}
