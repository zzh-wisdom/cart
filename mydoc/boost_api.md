# Boost API

## BOOST_PP_IF

BOOST_PP_IF(cond, t, f)

条件扩展

样例：

```c
BOOST_PP_IF(10, a, b) // expands to a
BOOST_PP_IF(0, a, b) // expands to b
```

## BOOST_PP_SEQ_SIZE

BOOST_PP_SEQ_SIZE(seq)

计算seq表达式中的元素个数

样例：

```c
#define SEQ (a)(b)(c)
BOOST_PP_SEQ_SIZE(SEQ) // expands to 3
```

## BOOST_PP_SEQ_FOR_EACH

BOOST_PP_SEQ_FOR_EACH(macro, data, seq)

对seq表达式中的每一个元素重复一次macro宏扩展
This macro is a repetition construct.  If seq is (a)(b)(c), it expands to the sequence:
macro(r, data, a) macro(r, data, b) macro(r, data, c)

data直接传递给macro宏使用。

## BOOST_PP_SEQ_FIRST_N

BOOST_PP_SEQ_FIRST_N(n, seq)

获取seq的前n个元素

```c
#define SEQ (a)(b)(c)(d)(e)
BOOST_PP_SEQ_FIRST_N(2, SEQ) // expands to (a)(b)
```
