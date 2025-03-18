//MSVC: /Zc:preprocessor
#define REMOVE_FIRST_COMMA_IMPL(X, ...) __VA_ARGS__
#define REMOVE_FIRST_COMMA(...) REMOVE_FIRST_COMMA_IMPL(__VA_ARGS__)

#define FOREACHI_1(F, X)      F(X)
#define FOREACHI_2(F, X, ...) F(X)FOREACHI_1(F, __VA_ARGS__)
#define FOREACHI_3(F, X, ...) F(X)FOREACHI_2(F, __VA_ARGS__)
#define FOREACHI_4(F, X, ...) F(X)FOREACHI_3(F, __VA_ARGS__)
#define FOREACHI_5(F, X, ...) F(X)FOREACHI_4(F, __VA_ARGS__)
#define FOREACHI_6(F, X, ...) F(X)FOREACHI_5(F, __VA_ARGS__)

#define FOREACHI_7(F, X, ...) F(X)FOREACHI_6(F, __VA_ARGS__)
#define FOREACHI_8(F, X, ...) F(X)FOREACHI_7(F, __VA_ARGS__)
#define FOREACHI_9(F, X, ...) F(X)FOREACHI_8(F, __VA_ARGS__)
#define FOREACHI_10(F, X, ...) F(X)FOREACHI_9(F, __VA_ARGS__)
#define FOREACHI_11(F, X, ...) F(X)FOREACHI_10(F, __VA_ARGS__)
#define FOREACHI_12(F, X, ...) F(X)FOREACHI_11(F, __VA_ARGS__)
#define FOREACHI_13(F, X, ...) F(X)FOREACHI_12(F, __VA_ARGS__)
#define FOREACHI_14(F, X, ...) F(X)FOREACHI_13(F, __VA_ARGS__)
#define FOREACHI_15(F, X, ...) F(X)FOREACHI_14(F, __VA_ARGS__)
#define FOREACHI_16(F, X, ...) F(X)FOREACHI_15(F, __VA_ARGS__)
#define FOREACHI_17(F, X, ...) F(X)FOREACHI_16(F, __VA_ARGS__)
#define FOREACHI_18(F, X, ...) F(X)FOREACHI_17(F, __VA_ARGS__)
#define FOREACHI_19(F, X, ...) F(X)FOREACHI_18(F, __VA_ARGS__)
#define FOREACHI_20(F, X, ...) F(X)FOREACHI_19(F, __VA_ARGS__)
#define FOREACHI_21(F, X, ...) F(X)FOREACHI_20(F, __VA_ARGS__)
#define FOREACHI_22(F, X, ...) F(X)FOREACHI_21(F, __VA_ARGS__)
#define FOREACHI_23(F, X, ...) F(X)FOREACHI_22(F, __VA_ARGS__)
#define FOREACHI_24(F, X, ...) F(X)FOREACHI_23(F, __VA_ARGS__)
#define FOREACHI_25(F, X, ...) F(X)FOREACHI_24(F, __VA_ARGS__)

#define FOREACHI(_1, _2, _3, _4, _5, _6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25, X, ...) FOREACHI_##X
#define FOREACH(F,...) FOREACHI(__VA_ARGS__, 25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6, 5, 4, 3, 2, 1)(F, __VA_ARGS__)

