typedef struct {
    AstBinaryOperator operator;
    AstExpression    *left;
    AstExpression    *right;
} AstBinaryExpression;

typedef struct {
    AstUnaryOperator operator;
    AstExpression   *operand;
} AstUnaryExpression;

