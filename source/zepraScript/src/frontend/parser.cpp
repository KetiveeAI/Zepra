/**
 * @file parser.cpp
 * @brief JavaScript recursive descent parser implementation
 */

#include "zeprascript/frontend/parser.hpp"
#include "zeprascript/frontend/ast.hpp"
#include <stdexcept>

namespace Zepra::Frontend {

Parser::Parser(const SourceCode* source) : lexer_(source) {
    currentToken_ = lexer_.nextToken();
}

std::unique_ptr<Program> Parser::parseProgram() {
    std::vector<StmtPtr> body;
    
    while (!isAtEnd()) {
        try {
            auto stmt = parseDeclaration();
            if (stmt) {
                body.push_back(std::move(stmt));
            }
        } catch (...) {
            synchronize();
        }
    }
    
    return std::make_unique<Program>(std::move(body));
}

// Token handling
Token& Parser::current() { return currentToken_; }

const Token& Parser::peek() { 
    return lexer_.peek(); 
}

Token Parser::advance() {
    previousToken_ = currentToken_;
    if (!isAtEnd()) {
        currentToken_ = lexer_.nextToken();
    }
    return previousToken_;
}

bool Parser::check(TokenType type) {
    return currentToken_.type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    error(currentToken_, message);
    return currentToken_;
}

bool Parser::isAtEnd() {
    return currentToken_.type == TokenType::EndOfFile;
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previousToken_.type == TokenType::Semicolon) return;
        
        switch (currentToken_.type) {
            case TokenType::Function:
            case TokenType::Var:
            case TokenType::Let:
            case TokenType::Const:
            case TokenType::Class:
            case TokenType::For:
            case TokenType::If:
            case TokenType::While:
            case TokenType::Return:
                return;
            default:
                break;
        }
        
        advance();
    }
}

void Parser::error(const std::string& message) {
    error(currentToken_, message);
}

void Parser::error(const Token& token, const std::string& message) {
    std::string err = std::to_string(token.start.line) + ":" +
                      std::to_string(token.start.column) + ": " + message;
    errors_.push_back(err);
}

// Declaration parsing
StmtPtr Parser::parseDeclaration() {
    if (match(TokenType::Var) || match(TokenType::Let) || match(TokenType::Const)) {
        return parseVariableDeclaration();
    }
    if (match(TokenType::Function)) {
        return parseFunctionDeclaration();
    }
    if (match(TokenType::Class)) {
        return parseClassDeclaration();
    }
    if (match(TokenType::Import)) {
        return parseImportDeclaration();
    }
    if (match(TokenType::Export)) {
        return parseExportDeclaration();
    }
    return parseStatement();
}

StmtPtr Parser::parseVariableDeclaration() {
    VariableDecl::Kind kind;
    if (previousToken_.type == TokenType::Var) {
        kind = VariableDecl::Kind::Var;
    } else if (previousToken_.type == TokenType::Let) {
        kind = VariableDecl::Kind::Let;
    } else {
        kind = VariableDecl::Kind::Const;
    }
    
    std::vector<VariableDeclarator> declarators;
    
    do {
        Token name = consume(TokenType::Identifier, "Expected variable name");
        
        ExprPtr id = std::make_unique<IdentifierExpr>(name.value, name.start);
        ExprPtr init = nullptr;
        
        if (match(TokenType::Assign)) {
            init = parseAssignmentExpression();
        }
        
        declarators.push_back({std::move(id), std::move(init)});
    } while (match(TokenType::Comma));
    
    // Semicolon is optional in some contexts
    match(TokenType::Semicolon);
    
    return std::make_unique<VariableDecl>(kind, std::move(declarators));
}

StmtPtr Parser::parseFunctionDeclaration() {
    Token name = consume(TokenType::Identifier, "Expected function name");
    
    consume(TokenType::LeftParen, "Expected '(' after function name");
    std::vector<FunctionParam> params = parseParameters();
    consume(TokenType::RightParen, "Expected ')' after parameters");
    
    consume(TokenType::LeftBrace, "Expected '{' before function body");
    auto body = std::unique_ptr<BlockStmt>(
        static_cast<BlockStmt*>(parseBlockStatement().release())
    );
    
    return std::make_unique<FunctionDecl>(name.value, std::move(params), std::move(body));
}

StmtPtr Parser::parseClassDeclaration() {
    // Stub for class declaration
    error("Class declarations not yet implemented");
    return nullptr;
}

std::vector<FunctionParam> Parser::parseParameters() {
    std::vector<FunctionParam> params;
    
    if (!check(TokenType::RightParen)) {
        do {
            bool rest = match(TokenType::DotDotDot);
            Token name = consume(TokenType::Identifier, "Expected parameter name");
            
            ExprPtr defaultValue = nullptr;
            if (match(TokenType::Assign)) {
                defaultValue = parseAssignmentExpression();
            }
            
            FunctionParam param;
            param.pattern = std::make_unique<IdentifierExpr>(name.value, name.start);
            param.defaultValue = std::move(defaultValue);
            param.rest = rest;
            
            params.push_back(std::move(param));
            
            if (rest) break; // Rest parameter must be last
        } while (match(TokenType::Comma));
    }
    
    return params;
}

// Statement parsing
StmtPtr Parser::parseStatement() {
    if (match(TokenType::LeftBrace)) {
        return parseBlockStatement();
    }
    if (match(TokenType::If)) {
        return parseIfStatement();
    }
    if (match(TokenType::While)) {
        return parseWhileStatement();
    }
    if (match(TokenType::Do)) {
        return parseDoWhileStatement();
    }
    if (match(TokenType::For)) {
        return parseForStatement();
    }
    if (match(TokenType::Return)) {
        return parseReturnStatement();
    }
    if (match(TokenType::Break)) {
        return parseBreakStatement();
    }
    if (match(TokenType::Continue)) {
        return parseContinueStatement();
    }
    if (match(TokenType::Throw)) {
        return parseThrowStatement();
    }
    if (match(TokenType::Try)) {
        return parseTryStatement();
    }
    if (match(TokenType::Switch)) {
        return parseSwitchStatement();
    }
    if (match(TokenType::Semicolon)) {
        return std::make_unique<EmptyStmt>();
    }
    
    return parseExpressionStatement();
}

StmtPtr Parser::parseBlockStatement() {
    std::vector<StmtPtr> statements;
    
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        auto stmt = parseDeclaration();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    
    consume(TokenType::RightBrace, "Expected '}' after block");
    
    return std::make_unique<BlockStmt>(std::move(statements));
}

StmtPtr Parser::parseIfStatement() {
    consume(TokenType::LeftParen, "Expected '(' after 'if'");
    ExprPtr condition = parseExpression();
    consume(TokenType::RightParen, "Expected ')' after condition");
    
    StmtPtr consequent = parseStatement();
    StmtPtr alternate = nullptr;
    
    if (match(TokenType::Else)) {
        alternate = parseStatement();
    }
    
    return std::make_unique<IfStmt>(std::move(condition), std::move(consequent), std::move(alternate));
}

StmtPtr Parser::parseWhileStatement() {
    consume(TokenType::LeftParen, "Expected '(' after 'while'");
    ExprPtr condition = parseExpression();
    consume(TokenType::RightParen, "Expected ')' after condition");
    
    bool prevInLoop = inLoop_;
    inLoop_ = true;
    StmtPtr body = parseStatement();
    inLoop_ = prevInLoop;
    
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

StmtPtr Parser::parseDoWhileStatement() {
    bool prevInLoop = inLoop_;
    inLoop_ = true;
    StmtPtr body = parseStatement();
    inLoop_ = prevInLoop;
    
    consume(TokenType::While, "Expected 'while' after do body");
    consume(TokenType::LeftParen, "Expected '(' after 'while'");
    ExprPtr condition = parseExpression();
    consume(TokenType::RightParen, "Expected ')' after condition");
    match(TokenType::Semicolon);
    
    return std::make_unique<DoWhileStmt>(std::move(body), std::move(condition));
}

StmtPtr Parser::parseForStatement() {
    consume(TokenType::LeftParen, "Expected '(' after 'for'");
    
    // Initializer
    ASTNodePtr init = nullptr;
    if (match(TokenType::Semicolon)) {
        // No initializer
    } else if (match({TokenType::Var, TokenType::Let, TokenType::Const})) {
        init = parseVariableDeclaration();
    } else {
        init = parseExpression();
        consume(TokenType::Semicolon, "Expected ';' after for initializer");
    }
    
    // Condition
    ExprPtr condition = nullptr;
    if (!check(TokenType::Semicolon)) {
        condition = parseExpression();
    }
    consume(TokenType::Semicolon, "Expected ';' after for condition");
    
    // Update
    ExprPtr update = nullptr;
    if (!check(TokenType::RightParen)) {
        update = parseExpression();
    }
    consume(TokenType::RightParen, "Expected ')' after for clauses");
    
    bool prevInLoop = inLoop_;
    inLoop_ = true;
    StmtPtr body = parseStatement();
    inLoop_ = prevInLoop;
    
    return std::make_unique<ForStmt>(std::move(init), std::move(condition), 
                                      std::move(update), std::move(body));
}

StmtPtr Parser::parseSwitchStatement() {
    // Stub
    error("Switch statements not yet implemented");
    return nullptr;
}

StmtPtr Parser::parseTryStatement() {
    consume(TokenType::LeftBrace, "Expected '{' after 'try'");
    auto block = std::unique_ptr<BlockStmt>(
        static_cast<BlockStmt*>(parseBlockStatement().release())
    );
    
    std::unique_ptr<CatchClause> handler = nullptr;
    std::unique_ptr<BlockStmt> finalizer = nullptr;
    
    if (match(TokenType::Catch)) {
        handler = std::make_unique<CatchClause>();
        
        if (match(TokenType::LeftParen)) {
            Token param = consume(TokenType::Identifier, "Expected catch parameter");
            handler->param = param.value;
            consume(TokenType::RightParen, "Expected ')' after catch parameter");
        }
        
        consume(TokenType::LeftBrace, "Expected '{' after catch");
        handler->body = std::unique_ptr<BlockStmt>(
            static_cast<BlockStmt*>(parseBlockStatement().release())
        );
    }
    
    if (match(TokenType::Finally)) {
        consume(TokenType::LeftBrace, "Expected '{' after finally");
        finalizer = std::unique_ptr<BlockStmt>(
            static_cast<BlockStmt*>(parseBlockStatement().release())
        );
    }
    
    return std::make_unique<TryStmt>(std::move(block), std::move(handler), std::move(finalizer));
}

StmtPtr Parser::parseThrowStatement() {
    ExprPtr argument = parseExpression();
    match(TokenType::Semicolon);
    return std::make_unique<ThrowStmt>(std::move(argument));
}

StmtPtr Parser::parseReturnStatement() {
    ExprPtr argument = nullptr;
    
    if (!check(TokenType::Semicolon) && !check(TokenType::RightBrace) && !isAtEnd()) {
        argument = parseExpression();
    }
    
    match(TokenType::Semicolon);
    return std::make_unique<ReturnStmt>(std::move(argument));
}

StmtPtr Parser::parseBreakStatement() {
    std::string label;
    if (check(TokenType::Identifier)) {
        label = advance().value;
    }
    match(TokenType::Semicolon);
    return std::make_unique<BreakStmt>(label);
}

StmtPtr Parser::parseContinueStatement() {
    std::string label;
    if (check(TokenType::Identifier)) {
        label = advance().value;
    }
    match(TokenType::Semicolon);
    return std::make_unique<ContinueStmt>(label);
}

StmtPtr Parser::parseExpressionStatement() {
    ExprPtr expr = parseExpression();
    match(TokenType::Semicolon);
    return std::make_unique<ExprStmt>(std::move(expr));
}

// Expression parsing - uses precedence climbing
ExprPtr Parser::parseExpression() {
    return parseAssignmentExpression();
}

ExprPtr Parser::parseAssignmentExpression() {
    ExprPtr left = parseConditionalExpression();
    
    if (match({TokenType::Assign, TokenType::PlusAssign, TokenType::MinusAssign,
               TokenType::StarAssign, TokenType::SlashAssign, TokenType::PercentAssign,
               TokenType::AmpersandAssign, TokenType::PipeAssign, TokenType::CaretAssign})) {
        TokenType op = previousToken_.type;
        ExprPtr right = parseAssignmentExpression();
        return std::make_unique<AssignmentExpr>(op, std::move(left), std::move(right));
    }
    
    return left;
}

ExprPtr Parser::parseConditionalExpression() {
    ExprPtr condition = parseLogicalOrExpression();
    
    if (match(TokenType::Question)) {
        ExprPtr consequent = parseAssignmentExpression();
        consume(TokenType::Colon, "Expected ':' in conditional expression");
        ExprPtr alternate = parseAssignmentExpression();
        return std::make_unique<ConditionalExpr>(std::move(condition), 
                                                  std::move(consequent), 
                                                  std::move(alternate));
    }
    
    return condition;
}

ExprPtr Parser::parseLogicalOrExpression() {
    ExprPtr left = parseLogicalAndExpression();
    
    while (match({TokenType::Or, TokenType::QuestionQuestion})) {
        TokenType op = previousToken_.type;
        ExprPtr right = parseLogicalAndExpression();
        left = std::make_unique<LogicalExpr>(op, std::move(left), std::move(right));
    }
    
    return left;
}

ExprPtr Parser::parseLogicalAndExpression() {
    ExprPtr left = parseBitwiseOrExpression();
    
    while (match(TokenType::And)) {
        ExprPtr right = parseBitwiseOrExpression();
        left = std::make_unique<LogicalExpr>(TokenType::And, std::move(left), std::move(right));
    }
    
    return left;
}

ExprPtr Parser::parseBitwiseOrExpression() {
    ExprPtr left = parseBitwiseXorExpression();
    
    while (match(TokenType::Pipe)) {
        ExprPtr right = parseBitwiseXorExpression();
        left = std::make_unique<BinaryExpr>(TokenType::Pipe, std::move(left), std::move(right));
    }
    
    return left;
}

ExprPtr Parser::parseBitwiseXorExpression() {
    ExprPtr left = parseBitwiseAndExpression();
    
    while (match(TokenType::Caret)) {
        ExprPtr right = parseBitwiseAndExpression();
        left = std::make_unique<BinaryExpr>(TokenType::Caret, std::move(left), std::move(right));
    }
    
    return left;
}

ExprPtr Parser::parseBitwiseAndExpression() {
    ExprPtr left = parseEqualityExpression();
    
    while (match(TokenType::Ampersand)) {
        ExprPtr right = parseEqualityExpression();
        left = std::make_unique<BinaryExpr>(TokenType::Ampersand, std::move(left), std::move(right));
    }
    
    return left;
}

ExprPtr Parser::parseEqualityExpression() {
    ExprPtr left = parseRelationalExpression();
    
    while (match({TokenType::Equal, TokenType::NotEqual, 
                  TokenType::StrictEqual, TokenType::StrictNotEqual})) {
        TokenType op = previousToken_.type;
        ExprPtr right = parseRelationalExpression();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    
    return left;
}

ExprPtr Parser::parseRelationalExpression() {
    ExprPtr left = parseShiftExpression();
    
    while (match({TokenType::Less, TokenType::LessEqual, 
                  TokenType::Greater, TokenType::GreaterEqual,
                  TokenType::Instanceof, TokenType::In})) {
        TokenType op = previousToken_.type;
        ExprPtr right = parseShiftExpression();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    
    return left;
}

ExprPtr Parser::parseShiftExpression() {
    ExprPtr left = parseAdditiveExpression();
    
    while (match({TokenType::LeftShift, TokenType::RightShift, TokenType::UnsignedRightShift})) {
        TokenType op = previousToken_.type;
        ExprPtr right = parseAdditiveExpression();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    
    return left;
}

ExprPtr Parser::parseAdditiveExpression() {
    ExprPtr left = parseMultiplicativeExpression();
    
    while (match({TokenType::Plus, TokenType::Minus})) {
        TokenType op = previousToken_.type;
        ExprPtr right = parseMultiplicativeExpression();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    
    return left;
}

ExprPtr Parser::parseMultiplicativeExpression() {
    ExprPtr left = parseExponentiationExpression();
    
    while (match({TokenType::Star, TokenType::Slash, TokenType::Percent})) {
        TokenType op = previousToken_.type;
        ExprPtr right = parseExponentiationExpression();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    
    return left;
}

ExprPtr Parser::parseExponentiationExpression() {
    ExprPtr left = parseUnaryExpression();
    
    if (match(TokenType::StarStar)) {
        ExprPtr right = parseExponentiationExpression(); // Right associative
        return std::make_unique<BinaryExpr>(TokenType::StarStar, std::move(left), std::move(right));
    }
    
    return left;
}

ExprPtr Parser::parseUnaryExpression() {
    // Handle await expression
    if (match(TokenType::Await)) {
        ExprPtr argument = parseUnaryExpression();
        return std::make_unique<AwaitExpr>(std::move(argument));
    }
    
    if (match({TokenType::Not, TokenType::Minus, TokenType::Plus, 
               TokenType::Tilde, TokenType::Typeof, TokenType::Void, 
               TokenType::Delete})) {
        TokenType op = previousToken_.type;
        ExprPtr argument = parseUnaryExpression();
        return std::make_unique<UnaryExpr>(op, std::move(argument), true);
    }
    
    return parseUpdateExpression();
}

ExprPtr Parser::parseUpdateExpression() {
    // Prefix increment/decrement
    if (match({TokenType::PlusPlus, TokenType::MinusMinus})) {
        TokenType op = previousToken_.type;
        ExprPtr argument = parseLeftHandSideExpression();
        return std::make_unique<UpdateExpr>(op, std::move(argument), true);
    }
    
    ExprPtr expr = parseLeftHandSideExpression();
    
    // Postfix increment/decrement (no line terminator between)
    if (match({TokenType::PlusPlus, TokenType::MinusMinus})) {
        TokenType op = previousToken_.type;
        return std::make_unique<UpdateExpr>(op, std::move(expr), false);
    }
    
    return expr;
}

ExprPtr Parser::parseLeftHandSideExpression() {
    ExprPtr expr;
    
    if (match(TokenType::New)) {
        expr = parseNewExpression();
    } else {
        expr = parseMemberExpression();
    }
    
    // Call expressions
    while (true) {
        if (match(TokenType::LeftParen)) {
            expr = parseCallExpression(std::move(expr));
        } else if (match(TokenType::Dot)) {
            Token property = consume(TokenType::Identifier, "Expected property name");
            expr = std::make_unique<MemberExpr>(
                std::move(expr),
                std::make_unique<IdentifierExpr>(property.value),
                false
            );
        } else if (match(TokenType::LeftBracket)) {
            ExprPtr property = parseExpression();
            consume(TokenType::RightBracket, "Expected ']'");
            expr = std::make_unique<MemberExpr>(std::move(expr), std::move(property), true);
        } else if (match(TokenType::QuestionDot)) {
            if (match(TokenType::LeftParen)) {
                std::vector<ExprPtr> args = parseArguments();
                consume(TokenType::RightParen, "Expected ')' after arguments");
                expr = std::make_unique<CallExpr>(std::move(expr), std::move(args), true);
            } else {
                Token property = consume(TokenType::Identifier, "Expected property name");
                expr = std::make_unique<MemberExpr>(
                    std::move(expr),
                    std::make_unique<IdentifierExpr>(property.value),
                    false, true
                );
            }
        } else {
            break;
        }
    }
    
    return expr;
}

ExprPtr Parser::parseCallExpression(ExprPtr callee) {
    std::vector<ExprPtr> args = parseArguments();
    consume(TokenType::RightParen, "Expected ')' after arguments");
    return std::make_unique<CallExpr>(std::move(callee), std::move(args));
}

std::vector<ExprPtr> Parser::parseArguments() {
    std::vector<ExprPtr> args;
    
    if (!check(TokenType::RightParen)) {
        do {
            if (match(TokenType::DotDotDot)) {
                // Spread element
                ExprPtr arg = parseAssignmentExpression();
                // Wrap in spread - for simplicity just add normally
                args.push_back(std::move(arg));
            } else {
                args.push_back(parseAssignmentExpression());
            }
        } while (match(TokenType::Comma));
    }
    
    return args;
}

ExprPtr Parser::parseMemberExpression() {
    ExprPtr expr = parsePrimaryExpression();
    
    while (true) {
        if (match(TokenType::Dot)) {
            Token property = consume(TokenType::Identifier, "Expected property name");
            expr = std::make_unique<MemberExpr>(
                std::move(expr),
                std::make_unique<IdentifierExpr>(property.value),
                false
            );
        } else if (match(TokenType::LeftBracket)) {
            ExprPtr property = parseExpression();
            consume(TokenType::RightBracket, "Expected ']'");
            expr = std::make_unique<MemberExpr>(std::move(expr), std::move(property), true);
        } else {
            break;
        }
    }
    
    return expr;
}

ExprPtr Parser::parseNewExpression() {
    ExprPtr callee = parseMemberExpression();
    
    std::vector<ExprPtr> args;
    if (match(TokenType::LeftParen)) {
        args = parseArguments();
        consume(TokenType::RightParen, "Expected ')' after arguments");
    }
    
    return std::make_unique<NewExpr>(std::move(callee), std::move(args));
}

ExprPtr Parser::parsePrimaryExpression() {
    // Literals
    if (match(TokenType::Number)) {
        return std::make_unique<LiteralExpr>(previousToken_.numericValue, previousToken_.start);
    }
    if (match(TokenType::String)) {
        return std::make_unique<LiteralExpr>(previousToken_.value, previousToken_.start);
    }
    if (match(TokenType::True)) {
        return std::make_unique<LiteralExpr>(true, previousToken_.start);
    }
    if (match(TokenType::False)) {
        return std::make_unique<LiteralExpr>(false, previousToken_.start);
    }
    if (match(TokenType::Null)) {
        return std::make_unique<LiteralExpr>(nullptr, previousToken_.start);
    }
    if (match(TokenType::Undefined)) {
        return std::make_unique<LiteralExpr>(LiteralExpr::LiteralValue{}, previousToken_.start);
    }
    
    // this
    if (match(TokenType::This)) {
        return std::make_unique<ThisExpr>(previousToken_.start);
    }
    
    // Identifier
    if (match(TokenType::Identifier)) {
        return std::make_unique<IdentifierExpr>(previousToken_.value, previousToken_.start);
    }
    
    // Parenthesized expression or arrow function
    if (match(TokenType::LeftParen)) {
        ExprPtr expr = parseExpression();
        consume(TokenType::RightParen, "Expected ')'");
        return expr;
    }
    
    // Array literal
    if (match(TokenType::LeftBracket)) {
        return parseArrayLiteral();
    }
    
    // Object literal
    if (match(TokenType::LeftBrace)) {
        return parseObjectLiteral();
    }
    
    // Function expression
    if (match(TokenType::Function)) {
        return parseFunctionExpression();
    }
    
    error("Unexpected token: " + std::string(Token::typeName(currentToken_.type)));
    return nullptr;
}

ExprPtr Parser::parseArrayLiteral() {
    std::vector<ExprPtr> elements;
    
    while (!check(TokenType::RightBracket) && !isAtEnd()) {
        if (match(TokenType::Comma)) {
            // Elision
            elements.push_back(nullptr);
            continue;
        }
        
        if (match(TokenType::DotDotDot)) {
            ExprPtr arg = parseAssignmentExpression();
            elements.push_back(std::move(arg));
        } else {
            elements.push_back(parseAssignmentExpression());
        }
        
        if (!check(TokenType::RightBracket)) {
            consume(TokenType::Comma, "Expected ',' between array elements");
        }
    }
    
    consume(TokenType::RightBracket, "Expected ']' after array");
    return std::make_unique<ArrayExpr>(std::move(elements));
}

ExprPtr Parser::parseObjectLiteral() {
    std::vector<ObjectProperty> properties;
    
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        ObjectProperty prop;
        
        // Property key
        if (match(TokenType::LeftBracket)) {
            // Computed property
            prop.key = parseAssignmentExpression();
            consume(TokenType::RightBracket, "Expected ']'");
            prop.computed = true;
        } else if (match(TokenType::String)) {
            prop.key = std::make_unique<LiteralExpr>(previousToken_.value);
        } else if (match(TokenType::Number)) {
            prop.key = std::make_unique<LiteralExpr>(previousToken_.numericValue);
        } else {
            Token name = consume(TokenType::Identifier, "Expected property name");
            prop.key = std::make_unique<IdentifierExpr>(name.value);
            
            // Shorthand: { x } = { x: x }
            if (!check(TokenType::Colon) && !check(TokenType::LeftParen)) {
                prop.value = std::make_unique<IdentifierExpr>(name.value);
                prop.shorthand = true;
                properties.push_back(std::move(prop));
                
                if (!check(TokenType::RightBrace)) {
                    consume(TokenType::Comma, "Expected ',' between properties");
                }
                continue;
            }
        }
        
        // Method shorthand: { foo() {} }
        if (match(TokenType::LeftParen)) {
            std::vector<FunctionParam> params = parseParameters();
            consume(TokenType::RightParen, "Expected ')'");
            consume(TokenType::LeftBrace, "Expected '{'");
            auto body = std::unique_ptr<BlockStmt>(
                static_cast<BlockStmt*>(parseBlockStatement().release())
            );
            prop.value = std::make_unique<FunctionExpr>("", std::move(params), std::move(body));
            prop.method = true;
        } else {
            consume(TokenType::Colon, "Expected ':' after property key");
            prop.value = parseAssignmentExpression();
        }
        
        properties.push_back(std::move(prop));
        
        if (!check(TokenType::RightBrace)) {
            consume(TokenType::Comma, "Expected ',' between properties");
        }
    }
    
    consume(TokenType::RightBrace, "Expected '}' after object");
    return std::make_unique<ObjectExpr>(std::move(properties));
}

ExprPtr Parser::parseFunctionExpression() {
    std::string name;
    if (check(TokenType::Identifier)) {
        name = advance().value;
    }
    
    consume(TokenType::LeftParen, "Expected '(' after function");
    std::vector<FunctionParam> params = parseParameters();
    consume(TokenType::RightParen, "Expected ')' after parameters");
    
    consume(TokenType::LeftBrace, "Expected '{' before function body");
    auto body = std::unique_ptr<BlockStmt>(
        static_cast<BlockStmt*>(parseBlockStatement().release())
    );
    
    return std::make_unique<FunctionExpr>(std::move(name), std::move(params), std::move(body));
}

ExprPtr Parser::parseArrowFunction() {
    // This is called when we've already identified it's an arrow function
    // For simplicity, this is handled in parsePrimaryExpression for now
    return nullptr;
}

ExprPtr Parser::parseClassExpression() {
    error("Class expressions not yet implemented");
    return nullptr;
}

// Free function helpers
std::unique_ptr<Program> parse(const SourceCode* source) {
    Parser parser(source);
    return parser.parseProgram();
}

std::unique_ptr<Program> parse(std::string_view code, std::string_view filename) {
    auto source = SourceCode::fromString(std::string(code), std::string(filename));
    return parse(source.get());
}

// Module import/export parsing
StmtPtr Parser::parseImportDeclaration() {
    SourceLocation loc = currentToken_.start;
    std::vector<ImportSpecifier> specifiers;
    
    // import foo from 'module' (default import)
    if (check(TokenType::Identifier)) {
        ImportSpecifier spec;
        spec.local = advance().value;
        spec.imported = "default";
        spec.isDefault = true;
        specifiers.push_back(spec);
        
        // Can have: import foo, { bar } from 'module'
        if (match(TokenType::Comma)) {
            if (!check(TokenType::LeftBrace)) {
                error("Expected '{' after ','");
            }
        }
    }
    
    // import { foo, bar as baz } from 'module'
    if (match(TokenType::LeftBrace)) {
        do {
            if (check(TokenType::RightBrace)) break;
            
            ImportSpecifier spec;
            spec.imported = consume(TokenType::Identifier, "Expected import specifier").value;
            
            if (match(TokenType::As)) {
                spec.local = consume(TokenType::Identifier, "Expected local name after 'as'").value;
            } else {
                spec.local = spec.imported;
            }
            specifiers.push_back(spec);
        } while (match(TokenType::Comma));
        
        consume(TokenType::RightBrace, "Expected '}' after import specifiers");
    }
    
    consume(TokenType::From, "Expected 'from' after import specifiers");
    std::string source = consume(TokenType::String, "Expected module path string").value;
    
    // Remove quotes from source
    if (source.size() >= 2) {
        source = source.substr(1, source.size() - 2);
    }
    
    // Optional semicolon
    match(TokenType::Semicolon);
    
    return std::make_unique<ImportDecl>(std::move(specifiers), std::move(source), loc);
}

StmtPtr Parser::parseExportDeclaration() {
    SourceLocation loc = currentToken_.start;
    
    // export default ...
    if (match(TokenType::Default)) {
        if (match(TokenType::Function)) {
            auto decl = parseFunctionDeclaration();
            return std::make_unique<ExportDecl>(std::move(decl), true, loc);
        }
        // export default expression
        auto expr = parseExpression();
        match(TokenType::Semicolon);
        auto exprStmt = std::make_unique<ExprStmt>(std::move(expr));
        return std::make_unique<ExportDecl>(std::move(exprStmt), true, loc);
    }
    
    // export function foo() {}
    if (match(TokenType::Function)) {
        auto decl = parseFunctionDeclaration();
        return std::make_unique<ExportDecl>(std::move(decl), false, loc);
    }
    
    // export const/let/var ...
    if (match(TokenType::Const) || match(TokenType::Let) || match(TokenType::Var)) {
        auto decl = parseVariableDeclaration();
        return std::make_unique<ExportDecl>(std::move(decl), false, loc);
    }
    
    // export { foo, bar }
    if (match(TokenType::LeftBrace)) {
        std::vector<ExportSpecifier> specifiers;
        do {
            if (check(TokenType::RightBrace)) break;
            
            ExportSpecifier spec;
            spec.local = consume(TokenType::Identifier, "Expected export specifier").value;
            
            if (match(TokenType::As)) {
                spec.exported = consume(TokenType::Identifier, "Expected exported name after 'as'").value;
            } else {
                spec.exported = spec.local;
            }
            specifiers.push_back(spec);
        } while (match(TokenType::Comma));
        
        consume(TokenType::RightBrace, "Expected '}' after export specifiers");
        match(TokenType::Semicolon);
        
        return std::make_unique<ExportDecl>(std::move(specifiers), loc);
    }
    
    error("Expected export declaration");
    return nullptr;
}

} // namespace Zepra::Frontend
